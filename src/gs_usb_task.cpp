#include "gs_usb_task.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "FreeRTOS.h"
#include "task.h"
#include "can.h"
#include "rev.h"

static struct gs_host_config config;
static struct gs_device_bittiming bt;
static struct gs_device_mode mode;

static struct gs_device_config device_config = {
    .sw_version = 2,
    .hw_version = 1,
};

static struct gs_device_bt_const bt_const = {
    // These are just dummy values for now. linux don't even use them for anything lmao
    .feature = 0,
    .fclk_can = 48000000,
    .tseg1_min = 1,
    .tseg1_max = 16,
    .tseg2_min = 1,
    .tseg2_max = 8,
    .sjw_max = 4,
    .brp_min = 1,
    .brp_max = 1024,
    .brp_inc = 1,
};


/* We really don't care about the byte order since most assumes just LE only */
static void gs_breq_host_format(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  if(stage == CONTROL_STAGE_SETUP) {
    tud_control_xfer(rhport, req, &config, sizeof(config));
    return;
  } else if (stage == CONTROL_STAGE_ACK) {
    printf("Host format: %d\n", config.byte_order); 
    return;
  }
}

static void gs_breq_device_config(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  if(stage == CONTROL_STAGE_SETUP) {
    tud_control_xfer(rhport, req, &device_config, sizeof(device_config));
    return;
  }
}

static void gs_breq_bt_const(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  if(stage == CONTROL_STAGE_SETUP) {
    tud_control_xfer(rhport, req, &bt_const, sizeof(bt_const));
    return;
  }
}

static void gs_breq_bittiming(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  if(stage == CONTROL_STAGE_SETUP) {
    tud_control_xfer(rhport, req, &bt, sizeof(bt));
    return;
  } else if(stage == CONTROL_STAGE_ACK) {
    printf("Bittiming: %d %d %d %d %d\n", bt.prop_seg, bt.phase_seg1, bt.phase_seg2, bt.sjw, bt.brp);
    return;
  }
}

static void gs_breq_mode(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  if(stage == CONTROL_STAGE_SETUP) {
    tud_control_xfer(rhport, req, &mode, sizeof(mode));
    return;
  } else if(stage == CONTROL_STAGE_ACK) {
    printf("Mode: %d %d\n", mode.mode, mode.flags);
    return;
  }
}


static int runCount = 0;

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  /* So, gs_usb will try and claim the vendor interface for pico flashing and resetting when manually bound.
   * That'd lead to 2 can interfaces, the first one being nonfunctional. So, when that one (probably) is being
   * enumerated, we just y'know, play dumb and pretend we don't know what's going on so it gives up and leaves
   * us alone. Yes, this is a hack. No, it doesn't work 100% of the time.
  */

 // okay sooooo gs_usb is now trying to eat even the CDC ADM INTERFACE FOR GODS SAKE
#if PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
  // if(runCount < 2) {
  if (runCount < 1) { // tf i don't need to have it be 2 on my laptop??
#else
  // if(runCount < 1) {
  if (runCount < 0) {
#endif
    runCount++;
    return false;
  }

  switch(req->bRequest) {
    case GS_USB_BREQ_HOST_FORMAT: gs_breq_host_format(rhport, stage, req); break;
    case GS_USB_BREQ_DEVICE_CONFIG: gs_breq_device_config(rhport, stage, req); break;
    case GS_USB_BREQ_BT_CONST: gs_breq_bt_const(rhport, stage, req); break;
    case GS_USB_BREQ_BITTIMING: gs_breq_bittiming(rhport, stage, req); break;
    case GS_USB_BREQ_MODE: gs_breq_mode(rhport, stage, req); break;
    default:
      printf("Unknown bRequest: %d %d %d %d\n", req->bRequest, req->wValue, req->wIndex, req->wLength);
      return false;
  }
  return true;
}

static bool recv_thing = false;

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes) {
  // printf("Sent %d bytes with interface %d\n", sent_bytes, itf);
}

// so the way it works is we send out frames with echo_id -1 and we have to echo back frames we recieve with their own echo id to ack them.

static struct gs_host_frame [[gnu::packed]] frame = {
      .echo_id = -1,
      .can_id = 0x123,
      .can_dlc = 8,
      .flags = 0,
      .reserved = 0,
      .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
};

void gs_usb_send_can_frame(struct can_msg *msg) {
  frame.can_id = msg->id;
  frame.can_dlc = msg->dlc;
  frame.data32[0] = msg->data32[0];
  frame.data32[1] = msg->data32[1];
  tud_vendor_n_write(0, &frame, sizeof(frame));
  tud_vendor_n_write_flush(0);
}

void gs_usb_task(__unused void *params) {
  gs_host_frame frame_buf[20] = {0};
  while(1) {
    if(!tud_inited()) continue;

    if(uint32_t b = tud_vendor_n_available(0)) {
      // printf("we have data available!! it's about %d uint32_t's long\n", b);
      // struct gs_host_frame test_frame;
      // tud_vendor_n_read(0, &test_frame, sizeof(test_frame));
      // tud_vendor_n_read_flush(0);
      // // printf("Received! frame!: %d %d %d %d %d %d %d %d %d\n", test_frame.echo_id, test_frame.can_id, test_frame.can_dlc,
      // // test_frame.channel, test_frame.flags, test_frame.reserved, test_frame.data[0], test_frame.data[1], test_frame.data[2]);
      // if(can_can_send_msg()) {
      //   struct can_msg msg = {
      //     .id = test_frame.can_id,
      //     .dlc = test_frame.can_dlc,
      //     .data32 = {test_frame.data32[0], test_frame.data32[1]}
      //   };
      //   // can_send_msg(&msg);
      //   rev_can_frame_callback(&msg);
      //   // echo back
      //   tud_vendor_n_write(0, &test_frame, sizeof(test_frame));
      //   tud_vendor_n_write_flush(0);
      //   // printf("okie we sent it\n");
      // } else {
      //   printf("CAN TX queue full :(\n");
      // }

      while(b > 0) {
        uint32_t n_read = tud_vendor_n_read(0, &frame_buf, sizeof(frame_buf));
        // tud_vendor_n_read_flush(0);
        // printf("b: %d n_read: %d\n", b, n_read);
        for(int i = 0; i < n_read / sizeof(gs_host_frame); i++) {
          struct gs_host_frame *recvd_frame = &frame_buf[i];
          recvd_frame->can_id = recvd_frame->can_id & 0b0001'1111'1111'1111'1111'1111'1111'1111; // can id is 29 bits
          struct can_msg msg = {
            .id = recvd_frame->can_id,
            .dlc = recvd_frame->can_dlc,
            .data32 = {recvd_frame->data32[0], recvd_frame->data32[1]}
          };
          if(can_can_send_msg()) {
            // can_send_msg(&msg);
          } else {
            printf("CAN TX queue full :( dropping frame i guess...\n");
          }

          rev_can_frame_callback(&msg);
          // printf("msg id (hex): %08X   data: %02X %02X %02X %02X %02X %02X %02X %02X\n", recvd_frame->can_id, msg.data[0], msg.data[1], msg.data[2], msg.data[3], msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
          // echo back
          tud_vendor_n_write(0, recvd_frame, sizeof(struct gs_host_frame));
          // printf("okie we sent it\n");

          tud_vendor_n_write_flush(0);
        }
        b -= n_read;
      }
    }
  }
}