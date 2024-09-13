#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/malloc.h"
#include "pico/mem_ops.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "sh1106_config.h"
#include <cmath>
#include <cstring>
#include "raspberry.h"

#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "task.h"

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

static inline uint32_t urgbw_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            ((uint32_t) (w) << 24) |
            (uint32_t) (b);
}

uint32_t hsv_to_urgb(uint32_t hue, float s, float v) {
    float c = s * v;
    float x = c * (1 - fabsf(fmodf(hue / 60.0f, 2) - 1));
    float m = v - c;
    
    float r, g, b;
    
    if (hue >= 0 && hue < 60) {
        r = c;
        g = x;
        b = 0;
    } else if (hue >= 60 && hue < 120) {
        r = x;
        g = c;
        b = 0;
    } else if (hue >= 120 && hue < 180) {
        r = 0;
        g = c;
        b = x;
    } else if (hue >= 180 && hue < 240) {
        r = 0;
        g = x;
        b = c;
    } else if (hue >= 240 && hue < 300) {
        r = x;
        g = 0;
        b = c;
    } else {
        r = c;
        g = 0;
        b = x;
    }
    
    uint8_t red = (uint8_t)((r + m) * 255);
    uint8_t green = (uint8_t)((g + m) * 176);
    uint8_t blue = (uint8_t)((b + m) * 240);
    
    return urgb_u32(red, green, blue);
}

static const int MAX_STRLEN = 200;
char pcOutputString[configCOMMAND_INT_MAX_OUTPUT_SIZE];

void dump_string_as_hex(const char* str, int length) {
    printf("[DEBUG] String as hex: ");
    for (int i = 0; i < length; i++) {
        printf("%02X ", (unsigned char)str[i]);  // Print each character as 2-digit hex
    }
    printf("\n");
}

static BaseType_t prvTasksCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString) {
    vTaskListTasks(pcWriteBuffer, xWriteBufferLen);
    return pdFALSE; // no more to write
}

static const CLI_Command_Definition_t xTasksCommand =
{
    "tasks",
    "tasks: Lists running tasks\r\n",
    prvTasksCommand,
    0
};

static BaseType_t prvBootromCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString) {
    printf("Entering bootloader mode\n");
    rom_reset_usb_boot(0, 0);
    return pdFALSE;
}

static const CLI_Command_Definition_t xBootromCommand =
{
    "br",
    "br: Reboot into bootrom\r\n",
    prvBootromCommand,
    0
};

static BaseType_t prvResetCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString) {
    printf("Resetting the device\n");
    watchdog_enable(1, 1);
    while(1);
    return pdFALSE;
}

static const CLI_Command_Definition_t xResetCommand =
{
    "r",
    "r: Reset the device\r\n",
    prvResetCommand,
    0
};

void main_task(__unused void* params) {
    // cli interpreter
    FreeRTOS_CLIRegisterCommand(&xTasksCommand);
    FreeRTOS_CLIRegisterCommand(&xBootromCommand);
    FreeRTOS_CLIRegisterCommand(&xResetCommand);
    vTaskDelay(2500);
    printf("\n\nOh god this is a serial console\n# ");
    char str[MAX_STRLEN] = {0xFF};
    int stri = 0;
    while(true) {
        int c = getchar();
        if(c == EOF || c == '\0') continue; // For some reason we recieve a null character on startup
        bool xMoreDataToFollow = false;
        if (c == '\r') {
            printf("\n");
            if(stri == 0) 
                goto end;

            do {
                xMoreDataToFollow = FreeRTOS_CLIProcessCommand(str, pcOutputString, configCOMMAND_INT_MAX_OUTPUT_SIZE);
                int strlength = strlen(pcOutputString);
                strlength = strlength > configCOMMAND_INT_MAX_OUTPUT_SIZE ? configCOMMAND_INT_MAX_OUTPUT_SIZE : strlength;
                stdio_put_string(pcOutputString, strlength, false, false);
            } while (xMoreDataToFollow);
end:
            printf("# ");
            memset(str, 0, MAX_STRLEN);
            stri = 0;
        } else if (c == '\b') {
            if (stri > 0) {
                stri--;
                putchar('\b');
                putchar(' ');
                putchar('\b');
            }
        } else {
            if(stri >= MAX_STRLEN)
                continue; // stop accepting characters
            str[stri] = c;
            stri++;
            putchar(c);
        }
    }
}

void runws2812(__unused void* params) {
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    uint pin = 16;
    ws2812_program_init(pio, sm, offset, pin, 800000, false);
    while(1) {
        for (int hue = 0; hue < 360; hue = hue + 1) {
            put_pixel(hsv_to_urgb(hue, 1, 1));
            vTaskDelay(1);
        }
    }
}

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    // area->buflen = (area->end_col - area->start_col + 1) * (area->end_page - area->start_page + 1);
};

void sh1106_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(i2c_default, SH1106_I2C_ADDR, buf, 2, false);
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
    i2c_write_blocking(i2c_default, SH1106_I2C_ADDR, temp_buf, buflen + 1, false);
    xTaskResumeAll();
    free(temp_buf);
}

void sh1106_send_cmd_list(uint8_t *buf, int num) {
    for (int i=0;i<num;i++)
        sh1106_send_cmd(buf[i]);
}

void sh1106_12864_to_13264_buf(uint8_t *in , uint8_t *out) {
    // convert a 128x64 buffer to a 132x64 buffer
    // we need to add 4 columns to the sides, then swap the left and right halves
    memset(out, 0, SH1106_BUF_LEN);
    for (int i = 0; i < SH1106_NUM_PAGES; i++) {
        for (int j = 0; j < SH1106_ACTUAL_WIDTH; j++) {
            int in_idx = i * SH1106_ACTUAL_WIDTH + j;
            int out_idx = i * SH1106_WIDTH + j + 4;
            out[out_idx] = in[in_idx];
        }
    }

    uint8_t* temp = (uint8_t*)malloc(SH1106_BUF_LEN);
    memcpy(temp, out, SH1106_BUF_LEN);

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
    assert(x >= 0 && x < SH1106_ACTUAL_WIDTH && y >=0 && y < sh1106_HEIGHT);

    // The calculation to determine the correct bit to set depends on which address
    // mode we are in. This code assumes horizontal

    // The video ram on the sh1106 is split up in to 8 rows, one bit per pixel.
    // Each row is 128 long by 8 pixels high, each byte vertically arranged, so byte 0 is x=0, y=0->7,
    // byte 1 is x = 1, y=0->7 etc

    // This code could be optimised, but is like this for clarity. The compiler
    // should do a half decent job optimising it anyway.

    const int BytesPerRow = SH1106_ACTUAL_WIDTH ; // x pixels, 1bpp, but each row is 8 pixel high, so (x / 8) * 8

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
            int source_idx = i * width_cols + j;
            int dest_idx = (dest_page + i) * SH1106_ACTUAL_WIDTH + dest_col + j;
            buf[dest_idx] = data[source_idx];
        }
    }
}

void run_oled_display(__unused void* params) {
    vTaskSuspendAll();
    i2c_init(SH1106_I2C, SH1106_I2C_FREQ);
    gpio_set_function(SH1106_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SH1106_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SH1106_I2C_SDA_PIN);
    gpio_pull_up(SH1106_I2C_SCL_PIN);

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

    vTaskDelay(1000);

    uint8_t buf[SH1106_ACTUAL_BUF_LEN] = {0};

    // intro sequence: flash the screen 3 times
    for (int i = 0; i < 3; i++) {
        sh1106_send_cmd(SH1106_SET_ALL_ON);    // Set all pixels on
        vTaskDelay(100);
        sh1106_send_cmd(SH1106_SET_ENTIRE_ON); // go back to following RAM for pixel state
        vTaskDelay(100);
    }

    int i = 0;
    while(1) {
        printf("Hello World! %u\n", i);
        //checkerboard pattern
        for(int i = 0; i < SH1106_ACTUAL_BUF_LEN; i++) {
            // buf[i] = i % 2 == 0 ? 0b10101010 : 0b01010101;
            buf[i] = 0x00;
        }
        // buf[SH1106_BUF_LEN-i-1] = 0b11000000;
        buf[SH1106_ACTUAL_BUF_LEN-i-1] = 0xFF;

        struct render_area area = {
            start_col: 0,
            end_col: 26-1,
            start_page: 0,
            end_page: (32 / SH1106_PAGE_HEIGHT) - 1
        };

        sh1106_blit_data(buf, &area, raspberry26x32, 0, 0);

        sh1106_render_buf(buf);
        i++;
        if(i >= SH1106_ACTUAL_BUF_LEN) i = 0;
        vTaskDelay(200);
    }
}

int main()
{
    stdio_init_all();

    TaskHandle_t task_handle_main_task = NULL;
    // TaskHandle_t task_handle_ws2812 = NULL;
    TaskHandle_t task_handle_oled_display = NULL;
    xTaskCreate(main_task, "Main Task", 2048, NULL, 1, &task_handle_main_task);
    // xTaskCreate(runws2812, "run the ws2812 led lmao", 2048, NULL, 1, &task_handle_ws2812);
    xTaskCreate(run_oled_display, "Oled Disp", 2048, NULL, 1, &task_handle_oled_display);
    vTaskCoreAffinitySet(task_handle_main_task, 1);
    // vTaskCoreAffinitySet(task_handle_ws2812, 1);
    vTaskCoreAffinitySet(task_handle_oled_display, 1);
    vTaskStartScheduler();
}
