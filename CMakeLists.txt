# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

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
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(picozerotest C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

cmake_policy(SET CMP0169 OLD)
FetchContent_Declare(
  freertos_kernel
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel
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

add_executable(picozerotest FreeRTOS-Plus-CLI/FreeRTOS_CLI.c src/picozerotest.cpp )

pico_set_program_name(picozerotest "picozerotest")
pico_set_program_version(picozerotest "0.1")

# Generate PIO header
pico_generate_pio_header(picozerotest ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(picozerotest 1)
pico_enable_stdio_usb(picozerotest 1)

# Add the standard library to the build
target_link_libraries(picozerotest
        pico_async_context_freertos
        FreeRTOS-Kernel-Heap4
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(picozerotest PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Plus-CLI/
  ${CMAKE_CURRENT_LIST_DIR}/src
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(picozerotest
        pico_stdlib pico_malloc pico_mem_ops
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_interp
        hardware_timer
        hardware_watchdog
        hardware_clocks
        )

pico_add_extra_outputs(picozerotest)