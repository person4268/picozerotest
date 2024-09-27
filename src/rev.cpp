#include "rev.h"
#include "revconsts.h"
#include <bit>
#include <stdio.h>
#include "FreeRTOS.h"
#include "FreeRTOS-Plus-CLI/FreeRTOS_CLI.h"
#include "task.h"

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
 
// i will die if i somehow have to port this to a big endian system or spark maxes start using big endian
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
    uint8_t temperature; //degC
    uint32_t voltage: 12; // volts times 128
    uint32_t current: 12; // amps times 128
  } pf1;
  struct [[gnu::packed]] {
    float position;
    uint32_t : 32;
  } pf2;
  struct [[gnu::packed]] {
    uint32_t analog_sensor_voltage: 10;
    uint32_t analog_sensor_velocity: 22;
    float analog_sensor_position;
  } pf3;
  struct [[gnu::packed]] {
    float alternate_encoder_velocity;
    float alternate_encoder_position;
  } pf4;
  struct [[gnu::packed]] {
    float duty_cycle_position;
    uint16_t duty_cycle_absolute_angle;
    uint16_t : 16;
  } pf5;
  struct [[gnu::packed]] {
    float duty_cycle_velocity;
    uint16_t duty_cycle_frequency;
    uint16_t : 16;
  } pf6;
  struct [[gnu::packed]] {
    uint8_t data[8]; // genuinely no idea what this is it isnt documented anywhere but the motor is sending it and revlib has some very light javadocs abt it
  } pf7;
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
  auto pf2 = data->pf2;
  auto pf3 = data->pf3;
  auto pf4 = data->pf4;
  auto pf5 = data->pf5;
  auto pf6 = data->pf6;
  auto pf7 = data->pf7;
  switch(api) {
    case PERIODIC_STATUS_0: // so this has different meaning depending on who's sending it but we have no way of knowing that :)
      if(pf0.applied_output == 0) break;
      // printf("Received periodic status 0 frame with applied output %d, faults %d, sticky faults %d, is follower %d\n", pf0.applied_output, pf0.faults, pf0.sticky_faults, pf0.is_follower);
      break;
    case PERIODIC_STATUS_1:
      if(pf1.velocity == 0) break;
      // printf("Received periodic status 1 frame with velocity %f, temperature %d, voltage %d, current %d\n", pf1.velocity, pf1.temperature, pf1.voltage, pf1.current);
      break;
    case PERIODIC_STATUS_2:
      // printf("Received periodic status 2 frame with position %f\n", pf2.position);
      break;
    case PERIODIC_STATUS_3:
      // printf("Received periodic status 3 frame with analog sensor voltage %d, analog sensor velocity %d, analog sensor position %f\n", pf3.analog_sensor_voltage, pf3.analog_sensor_velocity, pf3.analog_sensor_position);
      break;
    case PERIODIC_STATUS_4:
      // printf("Received periodic status 4 frame with alternate encoder velocity %f, alternate encoder position %f\n", pf4.alternate_encoder_velocity, pf4.alternate_encoder_position);
      break;
    case PERIODIC_STATUS_5:
      // printf("Received periodic status 5 frame with duty cycle position %f, duty cycle absolute angle %d\n", pf5.duty_cycle_position, pf5.duty_cycle_absolute_angle);
      break;
    case PERIODIC_STATUS_6:
      // printf("Received periodic status 6 frame with duty cycle velocity %f, duty cycle frequency %d\n", pf6.duty_cycle_velocity, pf6.duty_cycle_frequency);
      break;
    case PERIODIC_STATUS_7:
      // printf("Received periodic status 7 frame with data %02x %02x %02x %02x %02x %02x %02x %02x\n", data->pf7.data[0], data->pf7.data[1], data->pf7.data[2], data->pf7.data[3], data->pf7.data[4], data->pf7.data[5], data->pf7.data[6], data->pf7.data[7]);
      break;
    case NON_RIO_HEARTBEAT:
      // printf("heartheat data: %02x %02x %02x %02x %02x %02x %02x %02x\n", data->data[0], data->data[1], data->data[2], data->data[3], data->data[4], data->data[5], data->data[6], data->data[7]);
    default:
      // printf("Received frame to/from %s %s #%d. cl %02x id %02x API %s\n", manu_name, device_name, id.device_number, id.api_class, id.api_index, get_spark_max_can_api_name(api));
      break;
  };
}

static bool heartbeat_enabled = false;

static BaseType_t enable_heartbeat(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString) {
  printf("Enabling heartbeat\n");
  heartbeat_enabled = true;
  return pdFALSE;
}

static const CLI_Command_Definition_t xRevCommand = {
  "e",
  "e: start sending heartbeat\r\n",
  enable_heartbeat,
  0
};

static BaseType_t disable_heartbeat(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString) {
  printf("Disabling heartbeat\n");
  heartbeat_enabled = false;
  return pdFALSE;
}

const CLI_Command_Definition_t xRevCommand2 = {
  "d",
  "d: stop sending heartbeat\r\n",
  disable_heartbeat,
  0
};

void rev_register_commands() {
  FreeRTOS_CLIRegisterCommand(&xRevCommand);
  FreeRTOS_CLIRegisterCommand(&xRevCommand2);
}

void rev_fun_task(__unused void* params) {
  while(1) {
    if(heartbeat_enabled) {
      frc_msg_id id;
      id.api = NON_RIO_HEARTBEAT;
      id.device_number = 5;
      id.device_type = MOTOR_CONTROLLER;
      id.manufacturer_code = FRC_MANUFACTURER_REV_ROBOTICS;
      can_msg msg = { 0 };
      msg.id = id.can_msg_id;
      msg.dlc = 8;
      msg.data32[0] = 0xFFFFFFFF;
      msg.data32[1] = 0xFFFFFFFF;
      can_send_msg(&msg);

      id.api = DUTY_CYCLE_SET;
      msg.id = id.can_msg_id;
      float speed = 0.1f;
      msg.data32[0] = *(uint32_t*) &speed;
      msg.data32[1] = 0;
      can_send_msg(&msg);

    }
    vTaskDelay(10);
  }
}