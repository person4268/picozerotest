#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include "i2c_bus.h"

i2c_bus::i2c_bus(i2c_inst_t* bus, uint8_t sda, uint8_t scl) {
  this->bus = bus;
  this->sda = sda;
  this->scl = scl;
  this->isSoftware = false;
}

i2c_bus::i2c_bus(uint8_t sda, uint8_t scl) {
  this->sda = sda;
  this->scl = scl;
  this->isSoftware = true;
}

void i2c_bus::init() {
  if (this->isSoftware) {
    init_impl_software();
  } else {
    init_impl_hardware();
  }
}

void i2c_bus::init_impl_hardware() {
  i2c_init(this->bus, 100000);
  gpio_set_function(this->sda, GPIO_FUNC_I2C);
  gpio_set_function(this->scl, GPIO_FUNC_I2C);
  gpio_pull_up(this->sda);
  gpio_pull_up(this->scl);
}

void i2c_bus::init_impl_software() {
  // not implemented
}

#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>

void i2c_bus::read(uint8_t address, uint8_t* data, uint8_t length, bool nostop) {
  if (this->isSoftware) {
    read_impl_software(address, data, length, nostop);
  } else {
    read_impl_hardware(address, data, length, nostop);
  }
}

void i2c_bus::read_impl_hardware(uint8_t address, uint8_t* data, uint8_t length, bool nostop) {
  i2c_read_timeout_us(this->bus, address, data, length, nostop, 1000000);
}

void i2c_bus::read_impl_software(uint8_t address, uint8_t* data, uint8_t length, bool nostop) {
  // not implemented
}

void i2c_bus::write(uint8_t address, uint8_t* data, uint8_t length, bool nostop) {
  if (this->isSoftware) {
    write_impl_software(address, data, length, nostop);
  } else {
    write_impl_hardware(address, data, length, nostop);
  }
}

void i2c_bus::write_impl_hardware(uint8_t address, uint8_t* data, uint8_t length, bool nostop) {
  i2c_write_timeout_us(this->bus, address, data, length, nostop, 1000000);
}

void i2c_bus::write_impl_software(uint8_t address, uint8_t* data, uint8_t length, bool nostop) {
  // not implemented
}