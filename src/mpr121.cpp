#include "mpr121.h"
#include "mpr121_regs.h"
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>

MPR121::MPR121(std::shared_ptr<i2c_bus> i2c, uint8_t irq): MPR121(0x5A, std::move(i2c), irq) {}

MPR121::MPR121(uint8_t address, std::shared_ptr<i2c_bus> i2c, uint8_t irq) {
  this->address = address;
  this->bus = std::move(i2c);
  this->irq = irq;
}

void MPR121::writeRegister(uint8_t reg, uint8_t value) {
  uint8_t data[2] = {reg, value};
  bus->write(address, data, 2);
}

uint8_t MPR121::readRegister(uint8_t reg) {
  bus->write(address, &reg, 1);
  uint8_t out;
  bus->read(address, &out, 1);
  return out;
}

/* so the adafruit library does this:

bool Adafruit_MPR121::begin(uint8_t i2caddr, TwoWire *theWire,
                            uint8_t touchThreshold, uint8_t releaseThreshold) {

  if (i2c_dev) {
    delete i2c_dev;
  }
  i2c_dev = new Adafruit_I2CDevice(i2caddr, theWire);

  if (!i2c_dev->begin()) {
    return false;
  }



  return true;
}

*/


bool MPR121::init() {

  // soft reset
  writeRegister(MPR121_SOFTRESET, 0x63);
  vTaskDelay(50);
  for (uint8_t i = 0; i < 0x7F; i++) {
    //  Serial.print("$"); Serial.print(i, HEX);
    //  Serial.print(": 0x"); Serial.println(readRegister8(i));
  }

  writeRegister(MPR121_ECR, 0x0);

  uint8_t c = readRegister(MPR121_CONFIG2);

  // if (c != 0x24)
  //   return false;

  // setThresholds(touchThreshold, releaseThreshold);
  writeRegister(MPR121_MHDR, 0x01); // maximum half delta rising, 
  writeRegister(MPR121_NHDR, 0x01); // noise half delta
  writeRegister(MPR121_NCLR, 0x0E);
  writeRegister(MPR121_FDLR, 0x00);

  writeRegister(MPR121_MHDF, 0x01);
  writeRegister(MPR121_NHDF, 0x05);
  writeRegister(MPR121_NCLF, 0x01);
  writeRegister(MPR121_FDLF, 0x00);

  writeRegister(MPR121_NHDT, 0x00);
  writeRegister(MPR121_NCLT, 0x00);
  writeRegister(MPR121_FDLT, 0x00);

  writeRegister(MPR121_DEBOUNCE, 0);
  writeRegister(MPR121_CONFIG1, 0b11010000); // default, 16uA charge current
  writeRegister(MPR121_CONFIG2, 0b00100001); // 0.5uS encoding, 1ms period

#define AUTOCONFIG
#ifdef AUTOCONFIG
  writeRegister(MPR121_AUTOCONFIG0, 0b11001011);

  // correct values for Vdd = 3.3V
  writeRegister(MPR121_UPLIMIT, 200);     // ((Vdd - 0.7)/Vdd) * 256
  writeRegister(MPR121_TARGETLIMIT, 131); // UPLIMIT * 0.9
  writeRegister(MPR121_LOWLIMIT, 130);    // UPLIMIT * 0.65
#endif

  // enable X electrodes and start MPR121
  uint8_t ECR_SETTING =
      0b10000000 + 0b0001; // 5 bits for baseline tracking & proximity disabled + X
                      // amount of electrodes running (1)
  writeRegister(MPR121_ECR, ECR_SETTING); // start with above ECR setting
  return true;
}

uint16_t MPR121::readELE0() {
  uint8_t low = readRegister(MPR121_FILTDATA_0L);
  uint8_t high = readRegister(MPR121_FILTDATA_0H);
  printf("baseline: %d\n", readRegister(MPR121_BASELINE_0) << 2);
  return (high << 8) | low;
}

void MPR121::startReading() {
  uint8_t ecr = readRegister(MPR121_ECR);
  writeRegister(MPR121_ECR, ecr | 0b0000001); // todo: support more than 1 electrode
}

void MPR121::stopReading() {
  uint8_t ecr = readRegister(MPR121_ECR);
  writeRegister(MPR121_ECR, ecr & 0b11110000);
}