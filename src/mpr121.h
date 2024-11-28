#pragma once
#include <stdint.h>
#include "i2c_bus.h"

class MPR121 {
public:
  MPR121(uint8_t address, i2c_bus i2c, uint8_t irq);
  void init();
  uint16_t readELE1();
  void startReading();
  void stopReading();

private:
  uint8_t address;
  i2c_bus bus;
  uint8_t irq;

  void writeRegister(uint8_t reg, uint8_t value);
  void readRegister(uint8_t reg, uint8_t* value);
};