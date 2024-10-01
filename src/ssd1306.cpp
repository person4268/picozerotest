#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "disp_config.h"
#include "ssd1306.h"
#include "ssd1306_font.h"
// The SH1106 driver was originated from and became entirely rewritten from the pico-sdk code.
// This one is almost a verbatim copy.

void ssd1306_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(DISP_I2C, DISP_I2C_ADDR, buf, 2, false);
}

void ssd1306_send_cmd_list(uint8_t *buf, int num) {
    for (int i=0;i<num;i++)
        ssd1306_send_cmd(buf[i]);
}

void ssd1306_send_buf(uint8_t buf[], int buflen) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one gooooooo!

    // copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning

    uint8_t *temp_buf = (uint8_t*)malloc(buflen + 1);

    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, buflen);

    i2c_write_blocking(DISP_I2C, DISP_I2C_ADDR, temp_buf, buflen + 1, false);

    free(temp_buf);
}

void ssd1306_init() {

    i2c_init(DISP_I2C, DISP_I2C_FREQ);
    gpio_set_function(DISP_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DISP_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DISP_I2C_SDA_PIN);
    gpio_pull_up(DISP_I2C_SCL_PIN);

    // Some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are shown here
    // to demonstrate what the initialization sequence looks like
    // Some configuration values are recommended by the board manufacturer

    uint8_t cmds[] = {
        SSD1306_SET_DISP,               // set display off
        /* memory mapping */
        SSD1306_SET_MEM_MODE,           // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
        0x00,                           // horizontal addressing mode
        /* resolution and layout */
        SSD1306_SET_DISP_START_LINE,    // set display start line to 0
        SSD1306_SET_SEG_REMAP | 0x01,   // set segment re-map, column address 127 is mapped to SEG0
        SSD1306_SET_MUX_RATIO,          // set multiplex ratio
        SSD1306_HEIGHT - 1,             // Display height - 1
        SSD1306_SET_COM_OUT_DIR | 0x08, // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
        SSD1306_SET_DISP_OFFSET,        // set display offset
        0x00,                           // no offset
        SSD1306_SET_COM_PIN_CFG,        // set COM (common) pins hardware configuration. Board specific magic number.
                                        // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
        0x02,
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
        0x12,
#else
        0x02,
#endif
        /* timing and driving scheme */
        SSD1306_SET_DISP_CLK_DIV,       // set display clock divide ratio
        0x80,                           // div ratio of 1, standard freq
        SSD1306_SET_PRECHARGE,          // set pre-charge period
        0xF1,                           // Vcc internally generated on our board
        SSD1306_SET_VCOM_DESEL,         // set VCOMH deselect level
        0x30,                           // 0.83xVcc
        /* display */
        SSD1306_SET_CONTRAST,           // set contrast control
        0xFF,
        SSD1306_SET_ENTIRE_ON,          // set entire display on to follow RAM content
        SSD1306_SET_NORM_DISP,           // set normal (not inverted) display
        SSD1306_SET_CHARGE_PUMP,        // set charge pump
        0x14,                           // Vcc internally generated on our board
        SSD1306_SET_SCROLL | 0x00,      // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
        SSD1306_SET_DISP | 0x01, // turn display on
    };

    ssd1306_send_cmd_list(cmds, count_of(cmds));
}

void ssd1306_scroll(bool on) {
    // configure horizontal scrolling
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00, // dummy byte
        0x00, // start page 0
        0x00, // time interval
        0x03, // end page 3 SSD1306_NUM_PAGES ??
        0x00, // dummy byte
        0xFF, // dummy byte
        SSD1306_SET_SCROLL | (on ? 0x01 : 0) // Start/stop scrolling
    };

    ssd1306_send_cmd_list(cmds, count_of(cmds));
}

void ssd1306_render_buf(uint8_t *buf, struct render_area *area) {
    // update a portion of the display with a render area
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page
    };

    ssd1306_send_cmd_list(cmds, count_of(cmds));
    ssd1306_send_buf(buf, area->buflen);
}

void ssd1306_set_all_white(bool on) {
    ssd1306_send_cmd(on ? SSD1306_SET_ALL_ON : SSD1306_SET_ENTIRE_ON);
}

// assumes entire display
void ssd1306_render_buf(uint8_t* buf) {
    struct render_area area = {
        start_col: 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
    };

    calc_render_area_buflen(&area);
    ssd1306_render_buf(buf, &area);
}

void ssd1306_set_pixel(uint8_t *buf, int x, int y, bool on) {
    assert(x >= 0 && x < SSD1306_WIDTH && y >=0 && y < SSD1306_HEIGHT);

    // The calculation to determine the correct bit to set depends on which address
    // mode we are in. This code assumes horizontal

    // The video ram on the ssd1306 is split up in to 8 rows, one bit per pixel.
    // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x=0, y=0->7,
    // byte 1 is x = 1, y=0->7 etc

    // This code could be optimised, but is like this for clarity. The compiler
    // should do a half decent job optimising it anyway.

    const int BytesPerRow = SSD1306_WIDTH ; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

    int byte_idx = (y / 8) * BytesPerRow + x;
    uint8_t byte = buf[byte_idx];

    if (on)
        byte |=  1 << (y % 8);
    else
        byte &= ~(1 << (y % 8));

    buf[byte_idx] = byte;
}

void ssd1306_blit_data(uint8_t* buf, struct render_area* source_area, uint8_t* data, int dest_col, int dest_page) {
    int width_cols = source_area->end_col - source_area->start_col + 1;
    for(int i = 0; i < source_area->end_page - source_area->start_page + 1; i++) {
        for(int j = 0; j < width_cols; j++) {
            int source_idx = (i + source_area->start_page) * width_cols + j + source_area->start_col;
            int dest_idx = (dest_page + i) * DISP_WIDTH + dest_col + j;
            buf[dest_idx] = data[source_idx];
        }
    }
}


static void ssd1306_write_char(uint8_t *buf, int16_t x, int16_t y, uint8_t ch) {
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    // For the moment, only write on Y row boundaries (every 8 vertical pixels)
    y = y/8;

    auto chr = font[ch];
    int fb_idx = y * 128 + x;

    for (int i=0;i<6;i++) {
        buf[fb_idx++] = chr[i];
    }
}

void ssd1306_write_str(uint8_t *buf, int16_t x, int16_t y, char *str) {
    // Cull out any string off the screen
    if (x > SSD1306_WIDTH - 8 || y > SSD1306_HEIGHT - 8)
        return;

    while (*str) {
        ssd1306_write_char(buf, x, y, *str++);
        x+=6;
    }
}