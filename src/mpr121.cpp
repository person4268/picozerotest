#include "mpr121.h"
#include "mpr121_regs.h"

MPR121::MPR121(i2c_bus* i2c, uint8_t irq) {
  MPR121(0x5A, i2c, irq); // default address
}

MPR121::MPR121(uint8_t address, i2c_bus* i2c, uint8_t irq) {
  this->address = address;
  this->bus = i2c;
  this->irq = irq;
}

void MPR121::writeRegister(uint8_t reg, uint8_t value) {
  bus->write(address, (uint8_t[]){reg, value}, 2);
}

void MPR121::readRegister(uint8_t reg, uint8_t* value) {
  bus->write(address, (uint8_t[]){reg}, 1);
  uint8_t out;
  bus->read(address, &out, 1);
}

void MPR121::init() {
}