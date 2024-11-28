#pragma once
#include <stdint.h>
#include <hardware/i2c.h>

class i2c_bus {
public:
  i2c_bus(i2c_inst_t* bus, uint8_t sda, uint8_t scl); // hardware i2c bus
  i2c_bus(uint8_t sda, uint8_t scl); // software i2c bus
  void init();
  void write(uint8_t address, uint8_t* data, uint8_t length, bool nostop = false);
  void read(uint8_t address, uint8_t* data, uint8_t length, bool nostop = false);

  private:
    i2c_inst_t* bus = NULL;
    uint8_t sda;
    uint8_t scl;
    uint8_t isSoftware = 100;

    void init_impl_hardware();
    void init_impl_software();

    void write_impl_hardware(uint8_t address, uint8_t* data, uint8_t length, bool nostop);
    void write_impl_software(uint8_t address, uint8_t* data, uint8_t length, bool nostop);

    void read_impl_hardware(uint8_t address, uint8_t* data, uint8_t length, bool nostop);
    void read_impl_software(uint8_t address, uint8_t* data, uint8_t length, bool nostop);
};