#include <cmath>
#include <cstdio>
#include <cstring>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"

#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "task.h"

#include "gs_usb_task.h"

#include "src/generated/raspberry.h"
#include "src/generated/testimg1.h"
#include "src/generated/testimg2.h"
#include "sh1106.h"
#include "ws2812.pio.h"
#include "disp_config.h"

#include "bsp/board_api.h"
#include "tusb.h"

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

void run_oled_display(__unused void* params) {
    sh1106_init();
    vTaskDelay(1000);

    uint8_t buf[DISP_BUF_LEN] = {0};

    // intro sequence: flash the screen 3 times
    for (int i = 0; i < 3; i++) {
        sh1106_set_all_white(true);    // Set all pixels on
        vTaskDelay(100);
        sh1106_set_all_white(false); // go back to following RAM for pixel state
        vTaskDelay(100);
    }

    uint8_t i = 0;
    while(1) {
        printf("Hello World! %u\n", i);
        //checkerboard pattern
        for(int i = 0; i < DISP_BUF_LEN; i++) {
            // buf[i] = i % 2 == 0 ? 0b10101010 : 0b01010101;
            buf[i] = 0x00;
        }
        // buf[SH1106_BUF_LEN-i-1] = 0b11000000;
        // buf[DISP_BUF_LEN-i-1] = 0xFF;

        // struct render_area area = {
        //     start_col: 0,
        //     end_col: 26-1,
        //     start_page: 0,
        //     end_page: (32 / DISP_PAGE_HEIGHT) - 1
        // };

        // sh1106_blit_data(buf, &area, raspberry, 0, 0);

        struct render_area area = {
            start_col: 0,
            end_col: 128-1,
            start_page: 0 + i,
            end_page: (64 / DISP_PAGE_HEIGHT) - 1 + i
        };

        sh1106_blit_data(buf, &area, testimg1, 0, 0);

        sh1106_render_buf(buf);
        i++;
        // if(i >= DISP_BUF_LEN) i = 0;
        if (i >= (TESTIMG1_HEIGHT / DISP_PAGE_HEIGHT) - DISP_PAGE_HEIGHT) i = 0;
        vTaskDelay(200);
    }
}

void tinyusb_task(__unused void* params) {
    while(1) {
        tud_task();
    }
}

int main()
{
    board_init();
    tusb_init();
    stdio_init_all();

    TaskHandle_t task_handle_main_task = NULL;
    // TaskHandle_t task_handle_ws2812 = NULL;
    TaskHandle_t task_handle_oled_display = NULL;
    TaskHandle_t task_handle_tinyusb = NULL;
    TaskHandle_t task_handle_gs_usb = NULL;
    xTaskCreate(main_task, "Main Task", 2048, NULL, 1, &task_handle_main_task);
    // xTaskCreate(runws2812, "run the ws2812 led lmao", 2048, NULL, 1, &task_handle_ws2812);
    xTaskCreate(run_oled_display, "Oled Disp", 2048, NULL, 1, &task_handle_oled_display);
    xTaskCreate(tinyusb_task, "TinyUSB", 2048, NULL, 1, &task_handle_tinyusb);
    xTaskCreate(gs_usb_task, "GS USB", 2048, NULL, 1, &task_handle_gs_usb);
    vTaskCoreAffinitySet(task_handle_main_task, 1);
    vTaskCoreAffinitySet(task_handle_tinyusb, 1);
    vTaskCoreAffinitySet(task_handle_gs_usb, 1);
    // vTaskCoreAffinitySet(task_handle_ws2812, 1);
    vTaskCoreAffinitySet(task_handle_oled_display, 0x01);
    vTaskStartScheduler();
}
