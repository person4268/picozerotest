#include "rev.h"
#include "revconsts.h"
#include <bit>
#include <stdio.h>
#include <map>
#include "FreeRTOS.h"
#include "FreeRTOS-Plus-CLI/FreeRTOS_CLI.h"
#include "task.h"
#include "gs_usb_task.h"

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

struct rev_motor_info {
  int16_t applied_output;
  float velocity;
  float position;
  float current;
  float voltage;
  uint8_t temperature;
  uint16_t faults;
  uint16_t sticky_faults;
  uint8_t follower_data;
  TickType_t last_pf0;
  TickType_t last_pf1;
  TickType_t last_pf2;
  TickType_t last_pf3;
  TickType_t last_pf4;
  TickType_t last_pf5;
  TickType_t last_pf6;
  TickType_t last_pf7;
};

static std::map<uint32_t, rev_motor_info> rev_motor_infos{};

rev_motor_info* get_rev_motor_info(int dev_num) {
  auto it = rev_motor_infos.find(dev_num);
  if(it == rev_motor_infos.end()) {
    return NULL;
  }
  return &it->second;
}

bool rev_motor_fell_off(int dev_num) {
  auto info = get_rev_motor_info(dev_num);
  if(info == NULL) return true;
  return abs(xTaskGetTickCount() - info->last_pf0) > pdMS_TO_TICKS(1000);
}

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
      // if(pf0.applied_output == 0) break;
      // printf("Received periodic status 0 frame with applied output %d, faults %d, sticky faults %d, is follower %d\n", pf0.applied_output, pf0.faults, pf0.sticky_faults, pf0.is_follower);
      rev_motor_infos[id.device_number].applied_output = pf0.applied_output;
      rev_motor_infos[id.device_number].faults = pf0.faults;
      rev_motor_infos[id.device_number].sticky_faults = pf0.sticky_faults;
      rev_motor_infos[id.device_number].follower_data = pf0.is_follower;
      rev_motor_infos[id.device_number].last_pf0 = xTaskGetTickCount();
      break;
    case PERIODIC_STATUS_1:
      // if(pf1.velocity == 0) break;
      // printf("Received periodic status 1 frame with velocity %f, temperature %d, voltage %d, current %d\n", pf1.velocity, pf1.temperature, pf1.voltage, pf1.current);
      rev_motor_infos[id.device_number].velocity = pf1.velocity;
      rev_motor_infos[id.device_number].temperature = pf1.temperature;
      rev_motor_infos[id.device_number].voltage = pf1.voltage / 128.0;
      rev_motor_infos[id.device_number].current = pf1.current / 128.0;
      rev_motor_infos[id.device_number].last_pf1 = xTaskGetTickCount();
      break;
    case PERIODIC_STATUS_2:
      // printf("Received periodic status 2 frame with position %f\n", pf2.position);
      rev_motor_infos[id.device_number].position = pf2.position;
      rev_motor_infos[id.device_number].last_pf2 = xTaskGetTickCount();
      break;
    case PERIODIC_STATUS_3:
      // printf("Received periodic status 3 frame with analog sensor voltage %d, analog sensor velocity %d, analog sensor position %f\n", pf3.analog_sensor_voltage, pf3.analog_sensor_velocity, pf3.analog_sensor_position);
      rev_motor_infos[id.device_number].last_pf3 = xTaskGetTickCount();
      break;
    case PERIODIC_STATUS_4:
      // printf("Received periodic status 4 frame with alternate encoder velocity %f, alternate encoder position %f\n", pf4.alternate_encoder_velocity, pf4.alternate_encoder_position);
      rev_motor_infos[id.device_number].last_pf4 = xTaskGetTickCount();
      break;
    case PERIODIC_STATUS_5:
      // printf("Received periodic status 5 frame with duty cycle position %f, duty cycle absolute angle %d\n", pf5.duty_cycle_position, pf5.duty_cycle_absolute_angle);
      rev_motor_infos[id.device_number].last_pf5 = xTaskGetTickCount();
      break;
    case PERIODIC_STATUS_6:
      // printf("Received periodic status 6 frame with duty cycle velocity %f, duty cycle frequency %d\n", pf6.duty_cycle_velocity, pf6.duty_cycle_frequency);
      rev_motor_infos[id.device_number].last_pf6 = xTaskGetTickCount();
      break;
    case PERIODIC_STATUS_7:
      // printf("Received periodic status 7 frame with data %02x %02x %02x %02x %02x %02x %02x %02x\n", data->pf7.data[0], data->pf7.data[1], data->pf7.data[2], data->pf7.data[3], data->pf7.data[4], data->pf7.data[5], data->pf7.data[6], data->pf7.data[7]);
      rev_motor_infos[id.device_number].last_pf7 = xTaskGetTickCount();
      break;
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

void rev_send_heartbeat(int dev_num) {
  frc_msg_id id;
  id.api = NON_RIO_HEARTBEAT;
  id.device_number = dev_num;
  id.device_type = MOTOR_CONTROLLER;
  id.manufacturer_code = FRC_MANUFACTURER_REV_ROBOTICS;
  can_msg msg = { 0 };
  msg.id = id.can_msg_id;
  msg.dlc = 8;
  msg.data32[0] = 0xFFFFFFFF;
  msg.data32[1] = 0xFFFFFFFF;
  can_send_msg(&msg);
  gs_usb_send_can_frame(&msg);
}

void rev_send_duty_cycle(int dev_num, float speed) {
  frc_msg_id id;
  id.api = DUTY_CYCLE_SET;
  id.device_number = dev_num;
  id.device_type = MOTOR_CONTROLLER;
  id.manufacturer_code = FRC_MANUFACTURER_REV_ROBOTICS;
  can_msg msg = { 0 };
  msg.id = id.can_msg_id;
  msg.dlc = 8;
  msg.data32[0] = *(uint32_t*) &speed;
  msg.data32[1] = 0;
  can_send_msg(&msg);
  gs_usb_send_can_frame(&msg);
}


TickType_t lastHeartbeatTime = 0;
TickType_t lastPrintTime = 0;

static unsigned int motor_controller_id = 5;

static float pid_setpoint = 0.0;
static float pid_kp = 0.0;
static float pid_ki = 0.0;
static float pid_kd = 0.0;
void pid_task(__unused void* params) {
  unsigned int dt = 20;
  float i_accum;
  float last_error;
  while(1) {
    auto info = get_rev_motor_info(motor_controller_id);
    if(info == NULL) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    float error = pid_setpoint - info->position;
    i_accum += error * dt;
    float p_term = error * pid_kp;
    float i_term = i_accum * pid_ki;
    float d_term = (error - last_error) / dt;
    last_error = error;

    float out = p_term + i_term + d_term;
    out /= 12; // volts / rotation is more ergonomic than percent / rotation
    rev_send_duty_cycle(motor_controller_id, out);
    vTaskDelay(pdMS_TO_TICKS(dt));
  }
}

float rev_get_position() {
  auto info = get_rev_motor_info(motor_controller_id);
  if(info == NULL) return 0.0;
  return info->position;
}

float rev_get_velocity() {
  auto info = get_rev_motor_info(motor_controller_id);
  if(info == NULL) return 0.0;
  return info->velocity;
}

float rev_get_error() {
  auto info = get_rev_motor_info(motor_controller_id);
  if(info == NULL) return 0.0;
  return pid_setpoint - info->position;
}

void rev_set_setpoint(float setpoint) {
  pid_setpoint = setpoint;
}

void rev_fun_task(__unused void* params) {
  xTaskCreate(pid_task, "PID Task", 2048, NULL, 1, NULL);
  while(1) {
    if(heartbeat_enabled) {
      if(abs(xTaskGetTickCount() - lastHeartbeatTime) > pdMS_TO_TICKS(10)) {
        lastHeartbeatTime = xTaskGetTickCount();
        rev_send_heartbeat(motor_controller_id);
      }
    }
    if(abs(xTaskGetTickCount() - lastPrintTime) > pdMS_TO_TICKS(200)) {
      lastPrintTime = xTaskGetTickCount();
      for(auto& [dev_num, info] : rev_motor_infos) {
        if(rev_motor_fell_off(dev_num)) {
          printf("Motor %d fell off %d\n", dev_num, xTaskGetTickCount() - info.last_pf0);
          continue;
        }
        printf("Motor %d: Applied output: %d, Velocity: %f, Position: %f, Current: %f, Voltage: %f, Temperature: %d, Faults: %d, Sticky faults: %d, Follower data: %d\n", dev_num, info.applied_output, info.velocity, info.position, info.current, info.voltage, info.temperature, info.faults, info.sticky_faults, info.follower_data);
      }
    }
  }
}

void rev_set_kp(float kp) {
  pid_kp = kp;
}

void rev_set_ki(float ki) {
  pid_ki = ki;
}

void rev_set_kd(float kd) {
  pid_kd = kd;
}

float rev_get_kp() {
  return pid_kp;
}

float rev_get_ki() {
  return pid_ki;
}

float rev_get_kd() {
  return pid_kd;
}