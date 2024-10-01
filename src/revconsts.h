enum frc_device_type {
  BROADCAST_MESSAGES = 0,
  ROBOT_CONTROLLER = 1,
  MOTOR_CONTROLLER = 2,
  RELAY_CONTROLLER = 3,
  GYRO_SENSOR = 4,
  ACCELEROMETER = 5,
  ULTRASONIC_SENSOR = 6,
  GEAR_TOOTH_SENSOR = 7,
  POWER_DISTRIBUTION_MODULE = 8,
  PNEUMATICS_CONTROLLER = 9,
  MISCELLANEOUS = 10,
  IO_BREAKOUT = 11,
  RESERVED_12_30 = 12, // Reserved range 12-30
  FIRMWARE_UPDATE = 31
};

static const char* frc_device_type_names[] = {
  "Broadcast Messages",
  "Robot Controller",
  "Motor Controller",
  "Relay Controller",
  "Gyro Sensor",
  "Accelerometer",
  "Ultrasonic Sensor",
  "Gear Tooth Sensor",
  "Power Distribution Module",
  "Pneumatics Controller",
  "Miscellaneous",
  "IO Breakout",
  "Reserved 12-30",
  "Firmware Update"
};

enum frc_manufacturer {
  FRC_MANUFACTURER_BROADCAST = 0,
  FRC_MANUFACTURER_NI = 1,
  FRC_MANUFACTURER_LUMINARY_MICRO = 2,
  FRC_MANUFACTURER_DEKA = 3,
  FRC_MANUFACTURER_CTR_ELECTRONICS = 4,
  FRC_MANUFACTURER_REV_ROBOTICS = 5,
  FRC_MANUFACTURER_GRAPPLE = 6,
  FRC_MANUFACTURER_MIND_SENSORS = 7,
  FRC_MANUFACTURER_TEAM_USE = 8,
  FRC_MANUFACTURER_KAUAI_LABS = 9,
  FRC_MANUFACTURER_COPPERFORGE = 10,
  FRC_MANUFACTURER_PLAYING_WITH_FUSION = 11,
  FRC_MANUFACTURER_STUDICA = 12,
  FRC_MANUFACTURER_THE_THRIFTY_BOT = 13,
  FRC_MANUFACTURER_REDUX_ROBOTICS = 14,
  FRC_MANUFACTURER_ANDYMARK = 15,
  FRC_MANUFACTURER_VIVID_HOSTING = 16,
  FRC_MANUFACTURER_RESERVED = 17 // Reserved range 17-255
};

static const char* frc_manufacturer_names[] = {
  "Broadcast",
  "NI",
  "Luminary Micro",
  "DEKA",
  "CTR Electronics",
  "REV Robotics",
  "Grapple",
  "MindSensors",
  "Team Use",
  "Kauai Labs",
  "Copperforge",
  "Playing With Fusion",
  "Studica",
  "The Thrifty Bot",
  "Redux Robotics",
  "AndyMark",
  "Vivid Hosting",
  "Reserved"
};

enum SPARK_MAX_CAN_API {
    // Broadcast commands
    BROADCAST_DISABLE = 0x00,
    BROADCAST_SYSTEM_HALT = 0x01,

    // Commands for individual devices
    SETPOINT_SET = 0x01,
    DUTY_CYCLE_SET = 0x02,
    SPEED_SET = 0x12,
    SMART_VELOCITY_SET = 0x13,
    POSITION_SET = 0x32,
    VOLTAGE_SET = 0x42,
    CURRENT_SET = 0x43,
    SMART_MOTION_SET = 0x52,
    PERIODIC_STATUS_0 = 0x60,
    PERIODIC_STATUS_1 = 0x61,
    PERIODIC_STATUS_2 = 0x62,
    PERIODIC_STATUS_3 = 0x63,
    PERIODIC_STATUS_4 = 0x64,
    PERIODIC_STATUS_5 = 0x65,
    PERIODIC_STATUS_6 = 0x66,
    PERIODIC_STATUS_7 = 0x67,
    DRV_STATUS = 0x6A,
    CLEAR_FAULTS = 0x6E,
    CONFIG_BURN_FLASH = 0x72,
    SET_FOLLOWER_MODE = 0x73,
    CONFIG_FACTORY_DEFAULTS = 0x74,
    CONFIG_FACTORY_RESET = 0x75,
    IDENTIFY = 0x76,
    NACK_GENERAL = 0x80,
    ACK_GENERAL = 0x81,
    FIRMWARE_VERSION = 0x98,
    REV_ENUMERATE = 0x99,
    ROBORIO_LOCK = 0x9B,
    TELEMETRY_UPDATE_MECHANICAL_POSITION_ENCODER_PORT = 0xA0,
    TELEMETRY_UPDATE_I_ACCUM = 0xA2,
    TELEMETRY_UPDATE_MECHANICAL_POSITION_ANALOG = 0xA3,
    TELEMETRY_UPDATE_MECHANICAL_POSITION_ALT_ENCODER = 0xA4,
    NON_RIO_LOCK = 0xB1,
    USB_ONLY_IDENTIFY = 0xB3,
    PARAMETER_ACCESS = 0x300,

    // Commands for all REV motor controllers
    HEARTBEAT = 0x92,
    SYNC = 0x93,
    ID_QUERY = 0x94,
    ID_ASSIGN = 0x95,
    NON_RIO_HEARTBEAT = 0xB2,

    // Not actual commands, but included for completeness
    BROADCAST_NOT_A_COMMAND = 0x90,
    NON_ROBORIO_BROADCAST_NOT_A_COMMAND = 0xB0
};
#include <map>
#include <string>

static const std::map<enum SPARK_MAX_CAN_API, std::string> spark_max_can_api_names = {
  {BROADCAST_DISABLE, "BROADCAST_DISABLE"},
  {BROADCAST_SYSTEM_HALT, "BROADCAST_SYSTEM_HALT"},
  {SETPOINT_SET, "SETPOINT_SET"},
  {DUTY_CYCLE_SET, "DUTY_CYCLE_SET"},
  {SPEED_SET, "SPEED_SET"},
  {SMART_VELOCITY_SET, "SMART_VELOCITY_SET"},
  {POSITION_SET, "POSITION_SET"},
  {VOLTAGE_SET, "VOLTAGE_SET"},
  {CURRENT_SET, "CURRENT_SET"},
  {SMART_MOTION_SET, "SMART_MOTION_SET"},
  {PERIODIC_STATUS_0, "PERIODIC_STATUS_0"},
  {PERIODIC_STATUS_1, "PERIODIC_STATUS_1"},
  {PERIODIC_STATUS_2, "PERIODIC_STATUS_2"},
  {PERIODIC_STATUS_3, "PERIODIC_STATUS_3"},
  {PERIODIC_STATUS_4, "PERIODIC_STATUS_4"},
  {PERIODIC_STATUS_5, "PERIODIC_STATUS_5"},
  {PERIODIC_STATUS_6, "PERIODIC_STATUS_6"},
  {PERIODIC_STATUS_7, "PERIODIC_STATUS_7"},
  {DRV_STATUS, "DRV_STATUS"},
  {CLEAR_FAULTS, "CLEAR_FAULTS"},
  {CONFIG_BURN_FLASH, "CONFIG_BURN_FLASH"},
  {SET_FOLLOWER_MODE, "SET_FOLLOWER_MODE"},
  {CONFIG_FACTORY_DEFAULTS, "CONFIG_FACTORY_DEFAULTS"},
  {CONFIG_FACTORY_RESET, "CONFIG_FACTORY_RESET"},
  {IDENTIFY, "IDENTIFY"},
  {NACK_GENERAL, "NACK_GENERAL"},
  {ACK_GENERAL, "ACK_GENERAL"},
  {FIRMWARE_VERSION, "FIRMWARE_VERSION"},
  {REV_ENUMERATE, "REV_ENUMERATE"},
  {ROBORIO_LOCK, "ROBORIO_LOCK"},
  {TELEMETRY_UPDATE_MECHANICAL_POSITION_ENCODER_PORT, "TELEMETRY_UPDATE_MECHANICAL_POSITION_ENCODER_PORT"},
  {TELEMETRY_UPDATE_I_ACCUM, "TELEMETRY_UPDATE_I_ACCUM"},
  {TELEMETRY_UPDATE_MECHANICAL_POSITION_ANALOG, "TELEMETRY_UPDATE_MECHANICAL_POSITION_ANALOG"},
  {TELEMETRY_UPDATE_MECHANICAL_POSITION_ALT_ENCODER, "TELEMETRY_UPDATE_MECHANICAL_POSITION_ALT_ENCODER"},
  {NON_RIO_LOCK, "NON_RIO_LOCK"},
  {USB_ONLY_IDENTIFY, "USB_ONLY_IDENTIFY"},
  {PARAMETER_ACCESS, "PARAMETER_ACCESS"},
  {HEARTBEAT, "HEARTBEAT"},
  {SYNC, "SYNC"},
  {ID_QUERY, "ID_QUERY"},
  {ID_ASSIGN, "ID_ASSIGN"},
  {NON_RIO_HEARTBEAT, "NON_RIO_HEARTBEAT"},
  {BROADCAST_NOT_A_COMMAND, "BROADCAST_NOT_A_COMMAND"},
  {NON_ROBORIO_BROADCAST_NOT_A_COMMAND, "NON_ROBORIO_BROADCAST_NOT_A_COMMAND"}
};

static bool is_valid_spark_max_can_api(int value) {
  return spark_max_can_api_names.find((enum SPARK_MAX_CAN_API)value) != spark_max_can_api_names.end();
}

static enum SPARK_MAX_CAN_API int_to_spark_max_can_api(int value, bool* success) {
  if (is_valid_spark_max_can_api(value)) {
    if (success) *success = true;
    return (enum SPARK_MAX_CAN_API)value;
  } else {
    if (success) *success = false;
    return BROADCAST_DISABLE; // Default value
  }
}

static const char* get_spark_max_can_api_name(enum SPARK_MAX_CAN_API api) {
  return spark_max_can_api_names.at(api).c_str();
}
