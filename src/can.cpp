#include "can.h"
#include "can2040.h"
#include "hardware/pio.h"
#include "pico.h"
#include <cstdio>
#include "hardware/irq.h"
#include "consts.h"
#include "FreeRTOS.h"
#include "task.h"
#include "fifo.h"
#include "gs_usb_task.h"

static struct can2040 cbus;

#define CAN_TX_QUEUE_LENGTH 64

bool can_can_send_msg() {
  return can2040_check_transmit(&cbus);
}

// returns -1 if queue full
int can_send_msg(struct can_msg *msg) {

  struct can2040_msg cmsg = {
    .id = msg->id,
    .dlc = msg->dlc,
    .data32 = {msg->data32[0], msg->data32[1]}
  };
  vTaskSuspendAll();
  int res = can2040_transmit(&cbus, &cmsg);
  xTaskResumeAll();
  return res;
}

static fifo_t can_recv_queue;
static bool can_recv_notify = false;

static void can2040_cb(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg) {
  struct can_msg cmsg = {
    .id = msg->id,
    .dlc = msg->dlc,
    .data32 = {msg->data32[0], msg->data32[1]}
  };
  switch(notify) {
    case CAN2040_NOTIFY_RX:
      // printf("CAN RX: %08X %08X %08X\n", cmsg.id, cmsg.data32[0], cmsg.data32[1]);
      fifo_enqueue(&can_recv_queue, cmsg);
      can_recv_notify = true;
      break;
    case CAN2040_NOTIFY_TX:
      // printf("CAN TX: %08X %08X %08X success\n", msg->id, msg->data32[0], msg->data32[1]);
      break;
    case CAN2040_NOTIFY_ERROR:
      // printf("CAN ERROR BRUH MOMENT\n");
      break;
  }
}

static void PIOx_IRQHandler(void) {
    can2040_pio_irq_handler(&cbus);
}

void can_task(void* params) {
  uint32_t bitrate = 1000000;
  uint32_t gpio_tx = 6, gpio_rx = 7;
  fifo_init(&can_recv_queue);

  can2040_setup(&cbus, CAN2040_PIO_NUM);
  can2040_callback_config(&cbus, can2040_cb);

  irq_set_exclusive_handler(CAN2040_PIO_IRQ, PIOx_IRQHandler);
  irq_set_priority(CAN2040_PIO_IRQ, 1);
  irq_set_enabled(CAN2040_PIO_IRQ, true);

  can2040_start(&cbus, CUR_SYS_CLK, bitrate, gpio_rx, gpio_tx);

  while(1) {
    if(can_recv_notify == true) {
      printf("we recieved some stuff (queue size is %d)\n", fifo_size(&can_recv_queue));
      struct can_msg out = {0};
      fifo_dequeue(&can_recv_queue, &out);
      printf("CAN RX: %08X %08X %08X\n", out.id, out.data32[0], out.data32[1]);
      gs_usb_send_can_frame(&out);
      can_recv_notify = false;
    }
  }
}