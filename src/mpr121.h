#pragma once
#include <stdint.h>
#include "i2c_bus.h"
#include <memory>

class MPR121 {
public:
  MPR121(uint8_t address, std::shared_ptr<i2c_bus> i2c, uint8_t irq);
  MPR121(std::shared_ptr<i2c_bus> i2c, uint8_t irq);
  bool init();
  uint16_t readELE0();
  void startReading();
  void stopReading();

private:
  uint8_t address;
  std::shared_ptr<i2c_bus> bus;
  uint8_t irq;

  void writeRegister(uint8_t reg, uint8_t value);
  uint8_t readRegister(uint8_t reg);
};