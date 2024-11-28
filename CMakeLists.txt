# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

if(EXISTS ${USERHOME}/.pico-sdk)
# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)

include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# ====================================================================================
endif()
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(picozerotest C CXX ASM)

set(FAMILY rp2040)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

find_package(Python3 REQUIRED)

cmake_policy(SET CMP0169 OLD)
FetchContent_Declare(
  freertos_kernel
  GIT_REPOSITORY https://github.com/raspberrypi/FreeRTOS-Kernel
  GIT_TAG main # meh, idk why but using a sha didn't work on another machine
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(freertos_kernel)
message("FreeRTOS_Kernel_POPULATED: ${freertos_kernel_POPULATED}")
if(NOT FreeRTOS_Kernel_POPULATED)
  FetchContent_Populate(freertos_kernel)
endif()

set(FREERTOS_KERNEL_PATH ${freertos_kernel_SOURCE_DIR})
include(${CMAKE_CURRENT_LIST_DIR}/FreeRTOS_Kernel_import.cmake)

# Add executable. Default name is the project name, version 0.1

add_executable(picozerotest
  FreeRTOS-Plus-CLI/FreeRTOS_CLI.c
  can2040/can2040.c
  src/i2c_bus.cpp
  src/mpr121.cpp
  src/picozerotest.cpp
  src/usb_descriptors.c
)

pico_set_program_name(picozerotest "picozerotest")
pico_set_program_version(picozerotest "0.1")

# Generate PIO header

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(picozerotest 1)
pico_enable_stdio_usb(picozerotest 1)

# Add the standard library to the build
target_link_libraries(picozerotest
        pico_async_context_freertos
        FreeRTOS-Kernel-Heap4
        pico_stdlib)


target_compile_options(picozerotest PUBLIC -Wno-narrowing)

target_compile_definitions(picozerotest PUBLIC PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=1 PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR=0)

# Add the standard include files to the build
target_include_directories(picozerotest PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Plus-CLI/
  ${CMAKE_CURRENT_LIST_DIR}/can2040
  ${CMAKE_CURRENT_LIST_DIR}/src
)

# Add any user requested libraries
target_link_libraries(picozerotest
        pico_stdlib pico_malloc pico_mem_ops pico_unique_id
        tinyusb_device tinyusb_board
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_interp
        hardware_timer
        hardware_watchdog
        hardware_clocks
        cmsis_core
        )

pico_add_extra_outputs(picozerotest)