#include "gs_usb_task.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "FreeRTOS.h"
#include "task.h"

struct gs_host_config config;
struct gs_device_bittiming bt;
struct gs_device_mode mode;

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
void gs_breq_host_format(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  if(stage == CONTROL_STAGE_SETUP) {
    tud_control_xfer(rhport, req, &config, sizeof(config));
    return;
  } else if (stage == CONTROL_STAGE_ACK) {
    printf("Host format: %d\n", config.byte_order); 
    return;
  }
}

void gs_breq_device_config(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  if(stage == CONTROL_STAGE_SETUP) {
    tud_control_xfer(rhport, req, &device_config, sizeof(device_config));
    return;
  }
}

void gs_breq_bt_const(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  if(stage == CONTROL_STAGE_SETUP) {
    tud_control_xfer(rhport, req, &bt_const, sizeof(bt_const));
    return;
  }
}

void gs_breq_bittiming(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  if(stage == CONTROL_STAGE_SETUP) {
    tud_control_xfer(rhport, req, &bt, sizeof(bt));
    return;
  } else if(stage == CONTROL_STAGE_ACK) {
    printf("Bittiming: %d %d %d %d %d\n", bt.prop_seg, bt.phase_seg1, bt.phase_seg2, bt.sjw, bt.brp);
    return;
  }
}

void gs_breq_mode(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  if(stage == CONTROL_STAGE_SETUP) {
    tud_control_xfer(rhport, req, &mode, sizeof(mode));
    return;
  } else if(stage == CONTROL_STAGE_ACK) {
    printf("Mode: %d %d\n", mode.mode, mode.flags);
    return;
  }
}


bool ranOnce = false;

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * req) {
  /* So, gs_usb will try and claim the vendor interface for pico flashing and resetting when manually bound.
   * That'd lead to 2 can interfaces, the first one being nonfunctional. So, when that one (probably) is being
   * enumerated, we just y'know, play dumb and pretend we don't know what's going on so it gives up and leaves
   * us alone. Yes, this is a hack. No, it doesn't work 100% of the time.
  */
  if(!ranOnce) {
    ranOnce = true;
    return false;
  }
  switch(req->bRequest) {
    case GS_USB_BREQ_HOST_FORMAT: gs_breq_host_format(rhport, stage, req); break;
    case GS_USB_BREQ_DEVICE_CONFIG: gs_breq_device_config(rhport, stage, req); break;
    case GS_USB_BREQ_BT_CONST: gs_breq_bt_const(rhport, stage, req); break;
    case GS_USB_BREQ_BITTIMING: gs_breq_bittiming(rhport, stage, req); break;
    case GS_USB_BREQ_MODE: gs_breq_mode(rhport, stage, req); break;
    default: return false; // stall unsupported request
  }
  return true;
}


static struct gs_host_frame frame = {
      .echo_id = 0xFFFFFFFF,
      .can_id = 0x123,
      .can_dlc = 8,
      .flags = 0,
      .reserved = 0,
      .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    };


void gs_usb_task(__unused void *params) {
  while(1) {
    tud_vendor_n_write(0, &frame, sizeof(frame));
    tud_vendor_n_write_flush(0);
    vTaskDelay(500);
  }
}