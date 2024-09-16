#pragma once
#include "hardware/pio.h"
#define WS2812_PIO pio0
#define CAN2040_PIO pio1
#define CAN2040_PIO_NUM 1
#define CAN2040_PIO_IRQ PIO1_IRQ_0
#define CUR_SYS_CLK 250 * 1000000