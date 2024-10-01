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

#include "u8g2_oled.h"
#include "ws2812.pio.h"
#include "consts.h"
#include "rev.h"
#include "can.h"
#include "quadrature.pio.h"

#include "bsp/board_api.h"
#include "tusb.h"

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(WS2812_PIO, 1, pixel_grb << 8u);
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

static BaseType_t prvRelayOffCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString) {
    printf("Turning off relay\n");
    gpio_put(16, 0);
    return pdFALSE;
}

static const CLI_Command_Definition_t xRelayOffCommand =
{
    "2",
    "2: Turn off relay\r\n",
    prvRelayOffCommand,
    0
};

static BaseType_t prvRelayOnCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString) {
    printf("Turning on relay\n");
    gpio_put(16, 1);
    return pdFALSE;
}

static const CLI_Command_Definition_t xRelayOnCommand =
{
    "1",
    "1: Turn on relay\r\n",
    prvRelayOnCommand,
    0
};

void main_task(__unused void* params) {
    // cli interpreter
    FreeRTOS_CLIRegisterCommand(&xTasksCommand);
    FreeRTOS_CLIRegisterCommand(&xBootromCommand);
    FreeRTOS_CLIRegisterCommand(&xResetCommand);
    FreeRTOS_CLIRegisterCommand(&xRelayOffCommand);
    FreeRTOS_CLIRegisterCommand(&xRelayOnCommand);
    rev_register_commands();
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
    PIO pio = WS2812_PIO;
    uint sm = 1;
    uint offset = pio_add_program(pio, &ws2812_program);
    uint pin = 16;
    ws2812_program_init(pio, sm, offset, pin, 800000, false);
    while(1) {
        for (int hue = 0; hue < 360; hue = hue + 1) {
            put_pixel(hsv_to_urgb(hue, 1, 0.5));
            vTaskDelay(50);
        }
    }
}

void tinyusb_task(__unused void* params) {
    while(1) {
        tud_task();
    }
}


void quadrature_testing_task(__unused void* params) {
    gpio_init(8);
    gpio_set_pulls(8, true, false);


    int pos = 0;
    bool clicked = false;
    pio_add_program(pio0, &quadrature_encoder_program);
    // pins 4 and 5
    quadrature_encoder_program_init(pio0, 0, 4, 0);
    while(1) {
        int new_pos = quadrature_encoder_get_count(pio0, 0);
        int new_clicked = !gpio_get(8);

        if(new_pos != pos || new_clicked != clicked) {
            pos = new_pos;
            clicked = new_clicked;
            printf("Position: %d Clicked: %d\n", pos, clicked);
        }

        vTaskDelay(50);
    }
}

int main()
{
    board_init();
    tusb_init();
    stdio_init_all();
    set_sys_clock_hz(CUR_SYS_CLK, true);
    gpio_set_dir(16, GPIO_OUT);
    gpio_put(16, 0);
    gpio_set_function(16, GPIO_FUNC_SIO);

    TaskHandle_t task_handle_main_task = NULL;
    TaskHandle_t task_handle_ws2812 = NULL;
    TaskHandle_t task_handle_oled_display = NULL;
    TaskHandle_t task_handle_tinyusb = NULL;
    TaskHandle_t task_handle_gs_usb = NULL;
    TaskHandle_t task_handle_can = NULL;
    TaskHandle_t task_handle_quadrature = NULL;
    TaskHandle_t rev_fun = NULL;
    xTaskCreate(main_task, "Main Task", 2048, NULL, 1, &task_handle_main_task);
    xTaskCreate(quadrature_testing_task, "Quadrature", 2048, NULL, 1, &task_handle_quadrature);
    xTaskCreate(runws2812, "run the ws2812 led lmao", 2048, NULL, 1, &task_handle_ws2812);
    xTaskCreate(u8g2_test_task, "Oled Disp", 2048, NULL, 1, &task_handle_oled_display);
    xTaskCreate(tinyusb_task, "TinyUSB", 2048, NULL, 1, &task_handle_tinyusb);
    xTaskCreate(gs_usb_task, "GS USB", 2048, NULL, 1, &task_handle_gs_usb);
    xTaskCreate(can_task, "CAN", 2048, NULL, 1, &task_handle_can);
    xTaskCreate(rev_fun_task, "Rev Fun", 2048, NULL, 1, &rev_fun);
    vTaskCoreAffinitySet(task_handle_main_task, 1);
    vTaskCoreAffinitySet(task_handle_tinyusb, 1);
    // vTaskCoreAffinitySet(task_handle_gs_usb, 1);
    vTaskCoreAffinitySet(task_handle_ws2812, 1);
    vTaskCoreAffinitySet(task_handle_can, 1);
    vTaskCoreAffinitySet(task_handle_oled_display, 0x01);
    vTaskStartScheduler();
}
