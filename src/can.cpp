#include "can.h"
#include "can2040.h"
#include "hardware/pio.h"
#include "pico.h"
#include "hardware/irq.h"
#include "consts.h"

static struct can2040 cbus;

static void can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg) {

}

static void PIOx_IRQHandler(void) {
    can2040_pio_irq_handler(&cbus);
}

void can_task(void* params) {
  uint32_t bitrate = 1000000;
  uint32_t gpio_tx = 6, gpio_rx = 7;

  can2040_setup(&cbus, CAN2040_PIO_NUM);
  can2040_callback_config(&cbus, can2040_cb);

  irq_set_exclusive_handler(PIO0_IRQ_0, PIOx_IRQHandler);
  irq_set_priority(PIO0_IRQ_0, 1);
  irq_set_enabled(PIO0_IRQ_0, true);

  can2040_start(&cbus, CUR_SYS_CLK, bitrate, gpio_rx, gpio_tx);

  while(1) {}
}