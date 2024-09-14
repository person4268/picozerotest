#include "sh1106.h"
#include "sh1106_config.h"
#include "disp_config.h"

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "pico/mem_ops.h"

#include "FreeRTOS.h"
#include "task.h"

#include <cstdlib>
#include <cstring>


void sh1106_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_default, DISP_I2C_ADDR, buf, 2, false);
}

void sh1106_send_buf(uint8_t buf[], int buflen) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one gooooooo! EDIT: THE PAGE ADDRESS DOESNT GET AUTOINCREMENTED FOR THE SH1106

    // copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning
    
    uint8_t *temp_buf = (uint8_t*)malloc(buflen + 1);

    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, buflen);
    vTaskSuspendAll();
    i2c_write_blocking(i2c_default, DISP_I2C_ADDR, temp_buf, buflen + 1, false);
    xTaskResumeAll();
    free(temp_buf);
}

void sh1106_send_cmd_list(uint8_t *buf, int num) {
    for (int i=0;i<num;i++)
        sh1106_send_cmd(buf[i]);
}


void sh1106_init() {
    vTaskSuspendAll();
    i2c_init(DISP_I2C, DISP_I2C_FREQ);
    gpio_set_function(DISP_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DISP_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DISP_I2C_SDA_PIN);
    gpio_pull_up(DISP_I2C_SCL_PIN);

    // Init sequence
    uint8_t commands[] = {
        SH1106_SET_DISP, // Turn display off
        SH1106_SET_MEM_MODE, 0x00, // Horizontal addressing mode
        SH1106_SET_DISP_START_LINE | 0x00, // Start line
        SH1106_SET_SEG_REMAP, // Segment remap
        SH1106_SET_MUX_RATIO, 64 -1, // 64 COM lines
        SH1106_SET_COM_OUT_DIR_FLIP | 0x01, // Scan from COM[N-1] to COM0
        SH1106_SET_DISP_OFFSET, 0x00, // 0 offset
        SH1106_SET_COM_PIN_CFG, 0x12, // Sequential COM pin config, experiment with this if it doesn't work
        SH1106_SET_DISP_CLK_DIV, 0x80, // Set clock divide ratio/oscillator frequency
        SH1106_SET_PRECHARGE, 0xF1, // Set pre-charge period
        SH1106_SET_VCOM_DESEL, 0x30, // Set VCOMH deselect level
        SH1106_SET_CONTRAST, 0xFF, // Set contrast
        SH1106_SET_ENTIRE_ON, // Set entire display on
        SH1106_SET_NORM_DISP, // Set normal display
        SH1106_SET_CHARGE_PUMP, 0x14, // Enable charge pump
        SH1106_SET_SCROLL | 0x00, // Disable scroll
        SH1106_SET_DISP | 0x01, // Turn display on
    };

    sh1106_send_cmd_list(commands, count_of(commands));
    
    xTaskResumeAll();
}

void sh1106_set_all_white(bool on) {
    sh1106_send_cmd(on ? SH1106_SET_ALL_ON : SH1106_SET_ENTIRE_ON);
}

inline uint8_t reverse_bits(uint8_t byte) {
    byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
    byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
    byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
    return byte;
}

void sh1106_12864_to_13264_buf(uint8_t *in , uint8_t *out) {
    // convert a 128x64 buffer to a 132x64 buffer

    uint8_t* disp_flipped = (uint8_t*)malloc(DISP_BUF_LEN);
    memcpy(disp_flipped, in, DISP_BUF_LEN);

#ifdef SH1106_VERTICAL_FLIP
    // now we flip the display vertically
    for (int i = 0; i < DISP_NUM_PAGES; i++) {
        for (int j = 0; j < DISP_WIDTH; j++) {
            // each page too needs to be flipped (have bit order reversed)
            disp_flipped[i * DISP_WIDTH + j] = reverse_bits(in[(DISP_NUM_PAGES - i - 1) * DISP_WIDTH + (DISP_WIDTH - j - 1)]);
        }
    }
#endif

    // we need to add 4 columns to the sides, then swap the left and right halves
    memset(out, 0, SH1106_BUF_LEN);
    for (int i = 0; i < SH1106_NUM_PAGES; i++) {
        for (int j = 0; j < DISP_WIDTH; j++) {
            int in_idx = i * DISP_WIDTH + j;
            int out_idx = i * SH1106_WIDTH + j + 4 - 1; // idk exactly why -1 precisely but it makes it work so who cares
            out[out_idx] = disp_flipped[in_idx];
        }
    }

    free(disp_flipped);
    uint8_t* temp = (uint8_t*)malloc(SH1106_BUF_LEN);
    memcpy(temp, out, SH1106_BUF_LEN);

#ifdef SH1106_MIDDLE_FLIP
    // now we swap the left and right halves of out
    for (int i = 0; i < SH1106_NUM_PAGES; i++) {
        for (int j = 0; j < SH1106_WIDTH; j++) {
            if (j < SH1106_WIDTH / 2) {
                out[i * SH1106_WIDTH + j] = temp[i * SH1106_WIDTH + j + SH1106_WIDTH / 2];
            } else {
                out[i * SH1106_WIDTH + j] = temp[i * SH1106_WIDTH + j - SH1106_WIDTH / 2];
            }
        }
    }
#endif

    free(temp);
}

void sh1106_render_buf(uint8_t *buf) {
    // update the whole of the display with a render area

    uint8_t* disp_buf = (uint8_t*)malloc(SH1106_BUF_LEN);
    sh1106_12864_to_13264_buf(buf, disp_buf);

    struct render_area area = {
        start_col: 0,
        end_col : SH1106_WIDTH - 1,
        start_page : 0,
        end_page : SH1106_NUM_PAGES - 1
    };

    const int BytesPerRow = area.end_col - area.start_col + 1;

    for(int i = area.start_page; i <= area.end_page; i++) {
        sh1106_send_cmd(SH1106_SET_COL_ADDR_LOW || (area.start_col & 0x0F));
        sh1106_send_cmd(SH1106_SET_COL_ADDR_HIGH || ((area.start_col >> 4) & 0x0F));
        sh1106_send_cmd(SH1106_SET_PAGE_ADDR | (i & 0x0F));
        sh1106_send_cmd(SH1106_RMW_MODE); // increment column addr on write
        sh1106_send_buf(disp_buf + (i * BytesPerRow), BytesPerRow); // ugh, send a row at once
        sh1106_send_cmd(SH1106_END_RMW); // end read modify write
    }

    free(disp_buf);
}

void sh1106_set_pixel(uint8_t *buf, int x, int y, bool on) {
    assert(x >= 0 && x < DISP_WIDTH && y >=0 && y < DISP_HEIGHT);

    // The calculation to determine the correct bit to set depends on which address
    // mode we are in. This code assumes horizontal

    // The video ram on the sh1106 is split up in to 8 rows, one bit per pixel.
    // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x=0, y=0->7,
    // byte 1 is x = 1, y=0->7 etc

    // This code could be optimised, but is like this for clarity. The compiler
    // should do a half decent job optimising it anyway.

    const int BytesPerRow = DISP_WIDTH ; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on)
        byte |=  1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    buf[byte_idx] = byte;
}

void sh1106_blit_data(uint8_t* buf, struct render_area* source_area, uint8_t* data, int dest_col, int dest_page) {
    int width_cols = source_area->end_col - source_area->start_col + 1;
    for(int i = 0; i < source_area->end_page - source_area->start_page + 1; i++) {
        for(int j = 0; j < width_cols; j++) {
            int source_idx = (i + source_area->start_page) * width_cols + j + source_area->start_col;
            int dest_idx = (dest_page + i) * DISP_WIDTH + dest_col + j;
            buf[dest_idx] = data[source_idx];
        }
    }
}