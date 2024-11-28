#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"

#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "task.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include "i2c_bus.h"
#include "mpr121.h"

static const int MAX_STRLEN = 200;
char pcOutputString[configCOMMAND_INT_MAX_OUTPUT_SIZE];

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
    vTaskDelay(1000);
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

void tinyusb_task(__unused void* params) {
    while(1) {
        tud_task();
    }
}

void electrode_task(__unused void* params) {
    std::shared_ptr<i2c_bus> bus = std::make_shared<i2c_bus>(i2c0, 0, 1);
    bus->init();
    MPR121 mpr121(bus, 0);
    vTaskDelay(2000);
    printf("bruh moment!!\n");
    if(!mpr121.init()) {
        while(1) {
            printf("Failed to initialize MPR121\n");
            vTaskDelay(1000);
        }
    }
    while(1) {
        vTaskDelay(25);
        printf("ELE0: %d\n", mpr121.readELE0());
    }
}

int main()
{
    board_init();
    tusb_init();
    stdio_init_all();

    TaskHandle_t task_handle_main_task = NULL;
    TaskHandle_t task_handle_tinyusb = NULL;
    TaskHandle_t electrode_task_handle = NULL;
    xTaskCreate(main_task, "Main Task", 2048, NULL, 1, &task_handle_main_task);
    xTaskCreate(tinyusb_task, "TinyUSB", 2048, NULL, 1, &task_handle_tinyusb);
    xTaskCreate(electrode_task, "Electrode Task", 2048, NULL, 1, &electrode_task_handle);
    vTaskCoreAffinitySet(task_handle_main_task, 1);
    vTaskCoreAffinitySet(task_handle_tinyusb, 1);
    vTaskCoreAffinitySet(electrode_task_handle, 1);
    vTaskStartScheduler();
}
