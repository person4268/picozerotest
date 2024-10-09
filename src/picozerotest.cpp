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
#include "src/generated/ROBOT.h"
#include "sh1106.h"
#include "ssd1306.h"
#include "ws2812.pio.h"
#include "disp_config.h"
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

#include <string>

static int quad_pos = 0;
static bool quad_clicked = false;

void run_oled_display(__unused void* params) {
    disp_init();
    vTaskDelay(1000);

    uint8_t buf[DISP_BUF_LEN] = {0};

    // intro sequence: flash the screen 3 times
    for (int i = 0; i < 3; i++) {
        disp_set_all_white(true);    // Set all pixels on
        vTaskDelay(100);
        disp_set_all_white(false); // go back to following RAM for pixel state
        vTaskDelay(100);
    }

    disp_render_buf(buf);

    int i = 0;
    bool last_clicked = false;
    int last_quad_pos = 0;
    float setpoint;
    float kP;
    float kI;
    float kD;
    unsigned int mode = 0;
    const char* modes[] = {"Setpoint", "kP", "kI", "kD"};
    while(1) {
        struct render_area area = {
            start_col: 0,
            end_col : DISP_WIDTH - 1,
            start_page : 0,
            end_page : DISP_NUM_PAGES - 1
        };
        calc_render_area_buflen(&area);
        memset(buf, 0, DISP_BUF_LEN);
        std::string t = "Setting: " + std::string{modes[mode]};
        ssd1306_write_str(buf, 0, 0, (char*) t.c_str());
        t = "Pos: " + std::to_string(rev_get_position());
        ssd1306_write_str(buf, 0, 8, (char*) t.c_str());
        t = "Sp:  " + std::to_string(setpoint);
        ssd1306_write_str(buf, 0, 16, (char*) t.c_str());
        t = "Err: " + std::to_string(rev_get_error());
        ssd1306_write_str(buf, 0, 24, (char*) t.c_str());
        t = "Vel: " + std::to_string(rev_get_velocity());
        ssd1306_write_str(buf, 0, 32, (char*) t.c_str());
        t = "kP: " + std::to_string(kP);
        ssd1306_write_str(buf, 0, 40, (char*) t.c_str());
        t = "kI: " + std::to_string(kI);
        ssd1306_write_str(buf, 0, 48, (char*) t.c_str());
        t = "kD: " + std::to_string(kD);
        ssd1306_write_str(buf, 0, 56, (char*) t.c_str());
        if(!last_clicked && quad_clicked) {
            mode ++;
            if(mode > 3) mode = 0;
        }

        if(mode == 0) {
            int diff = quad_pos - last_quad_pos;
            setpoint += (diff / 4.0) / 10.0; 
            rev_set_setpoint(setpoint);
        } else if(mode == 1) {
            int diff = quad_pos - last_quad_pos;
            kP += (diff / 4.0) / 10.0;
            rev_set_kp(kP);
        } else if(mode == 2) {
            int diff = quad_pos - last_quad_pos;
            kI += (diff / 4.0) / 10.0;
            rev_set_ki(kI);
        } else if(mode == 3) {
            int diff = quad_pos - last_quad_pos;
            kD += (diff / 4.0) / 160.0;
            rev_set_kd(kD);
        }

        // t = "mode " + std::to_string(mode);
        // ssd1306_write_str(buf, 0, 8, (char*) t.c_str());

        last_clicked = quad_clicked;
        last_quad_pos = quad_pos;
        ssd1306_render_buf(buf, &area);
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

    bool clicked = false;
    pio_add_program(pio0, &quadrature_encoder_program);
    // pins 4 and 5
    quadrature_encoder_program_init(pio0, 0, 4, 0);
    while(1) {
        quad_pos = -quadrature_encoder_get_count(pio0, 0);
        quad_clicked = !gpio_get(8);
        vTaskDelay(5);
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
    xTaskCreate(run_oled_display, "Oled Disp", 2048, NULL, 1, &task_handle_oled_display);
    xTaskCreate(tinyusb_task, "TinyUSB", 2048, NULL, 1, &task_handle_tinyusb);
    xTaskCreate(gs_usb_task, "GS USB", 2048, NULL, 1, &task_handle_gs_usb);
    xTaskCreate(can_task, "CAN", 2048, NULL, 1, &task_handle_can);
    xTaskCreate(rev_fun_task, "Rev Fun", 2048, NULL, 1, &rev_fun);
    vTaskCoreAffinitySet(task_handle_main_task, 1);
    vTaskCoreAffinitySet(task_handle_tinyusb, 1);
    vTaskCoreAffinitySet(task_handle_gs_usb, 1);
    vTaskCoreAffinitySet(task_handle_ws2812, 1);
    vTaskCoreAffinitySet(task_handle_can, 1);
    vTaskCoreAffinitySet(task_handle_oled_display, 0x01);
    vTaskCoreAffinitySet(task_handle_quadrature, 0x01);
    vTaskCoreAffinitySet(rev_fun, 0x01);
    vTaskStartScheduler();
}
