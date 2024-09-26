#include "rev.h"
#include "revconsts.h"
#include <stdio.h>

union frc_msg_id {
  uint32_t can_msg_id;
  struct [[gnu::packed]] {
    uint32_t device_number: 6;
    uint32_t api_index: 4;
    uint32_t api_class: 6;
    uint32_t manufacturer_code: 8;
    uint32_t device_type: 5;
  };
  struct [[gnu::packed]] {
    uint32_t : 6;
    uint32_t api: 10;  // Combined api_index and api_class
    uint32_t : 8;
    uint32_t : 5;
  };
};
 

union rev_data_frame_interpretations {
  uint8_t data[8];
  struct [[gnu::packed]] {
    int16_t applied_output;
    uint16_t faults;
    uint16_t sticky_faults;
    uint8_t : 8;
    uint8_t is_follower;
  } pf0;
  struct [[gnu::packed]] {
    float velocity; // 32bits
    uint8_t temperature;
    uint32_t voltage: 12;
    uint32_t current: 12;
  } pf1;
};

void rev_can_frame_callback(struct can_msg* frame) {
  frc_msg_id id;
  id.can_msg_id = frame->id;
  // 5 0 6 5 2
  // 5 1 6 5 2
  const char* manu_name = frc_manufacturer_names[id.manufacturer_code];
  const char* device_name = frc_device_type_names[id.device_type];
  // printf("Received frame with id: %d %d %d %d %d\n", id.device_number, id.api_index, id.api_class, id.manufacturer_code, id.device_type);
  // printf("Received frame to/from %s's %s #%d. id %02x cl %02x API %s\n", manu_name, device_name, id.device_number, id.api_index, id.api_class, get_spark_max_can_api_name(int_to_spark_max_can_api(id.api, nullptr)));
  enum SPARK_MAX_CAN_API api = int_to_spark_max_can_api(id.api, nullptr);
  rev_data_frame_interpretations* data = (rev_data_frame_interpretations*)frame->data;
  auto pf0 = data->pf0;
  auto pf1 = data->pf1;
  switch(api) {
    case PERIODIC_STATUS_0: // so this has different meaning depending on who's sending it but we have no way of knowing that :)
      if(pf0.applied_output == 0) break;
      printf("Received periodic status 0 frame with applied output %d, faults %d, sticky faults %d, is follower %d\n", pf0.applied_output, pf0.faults, pf0.sticky_faults, pf0.is_follower);
      break;
    case PERIODIC_STATUS_1:
      if(pf1.velocity == 0) break;
      printf("Received periodic status 1 frame with velocity %f, temperature %d, voltage %d, current %d\n", pf1.velocity, pf1.temperature, pf1.voltage, pf1.current);
      break;
  };
}