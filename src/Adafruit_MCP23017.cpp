// ==============================
// File: Adafruit_MCP23017.cpp
// ==============================
/***************************************************
 This is a library for the MCP23017 i2c port expander

 Written by Limor Fried/Ladyada for Adafruit Industries.
 BSD license, all text above must be included in any redistribution
 ****************************************************/

#ifdef __AVR
  #include <avr/pgmspace.h>
#elif defined(ESP8266)
  #include <pgmspace.h>
#endif

#include "Adafruit_MCP23017.h"

#if ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

void Adafruit_MCP23017::wireSend(uint8_t x) {
#if ARDUINO >= 100
  #if !defined(ARDUINO_AVR_GEMMA)
    _wire->write((uint8_t)x);
  #else
    Wire.write((uint8_t)x);
  #endif
#else
  #if !defined(ARDUINO_AVR_GEMMA)
    _wire->send(x);
  #else
    Wire.send(x);
  #endif
#endif
}

uint8_t Adafruit_MCP23017::wireRecv() {
#if ARDUINO >= 100
  #if !defined(ARDUINO_AVR_GEMMA)
    return (uint8_t)_wire->read();
  #else
    return (uint8_t)Wire.read();
  #endif
#else
  #if !defined(ARDUINO_AVR_GEMMA)
    return (uint8_t)_wire->receive();
  #else
    return (uint8_t)Wire.receive();
  #endif
#endif
}

uint8_t Adafruit_MCP23017::bitForPin(uint8_t pin) {
  return (uint8_t)(pin & 0x07);
}

uint8_t Adafruit_MCP23017::regForPin(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr) {
  return (pin < 8) ? portAaddr : portBaddr;
}

uint8_t Adafruit_MCP23017::readRegister(uint8_t addr) {
#if !defined(ARDUINO_AVR_GEMMA)
  _wire->beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(addr);
  _wire->endTransmission();

  _wire->requestFrom((uint8_t)(MCP23017_ADDRESS | i2caddr), (uint8_t)1);
  return wireRecv();
#else
  Wire.beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(addr);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)(MCP23017_ADDRESS | i2caddr), (uint8_t)1);
  return wireRecv();
#endif
}

void Adafruit_MCP23017::writeRegister(uint8_t regAddr, uint8_t regValue) {
#if !defined(ARDUINO_AVR_GEMMA)
  _wire->beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(regAddr);
  wireSend(regValue);
  _wire->endTransmission();
#else
  Wire.beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(regAddr);
  wireSend(regValue);
  Wire.endTransmission();
#endif
}

void Adafruit_MCP23017::updateRegisterBit(uint8_t pin, uint8_t pValue, uint8_t portAaddr, uint8_t portBaddr) {
  uint8_t regAddr = regForPin(pin, portAaddr, portBaddr);
  uint8_t bit = bitForPin(pin);
  uint8_t regValue = readRegister(regAddr);

  bitWrite(regValue, bit, pValue);
  writeRegister(regAddr, regValue);
}

void Adafruit_MCP23017::begin(uint8_t addr) {
  if (addr > 7) addr = 7;
  i2caddr = addr;

  // Driver should not call Wire.begin() or setClock() globally.
  // You do that once in your sketch for each bus.

  // Defaults: all inputs A/B.
  writeRegister(MCP23017_IODIRA, 0xFF);
  writeRegister(MCP23017_IODIRB, 0xFF);
}

void Adafruit_MCP23017::begin(void) {
  begin(0);
}

#if !defined(ARDUINO_AVR_GEMMA)
void Adafruit_MCP23017::begin(TwoWire &wirePort) {
  begin(0, wirePort);
}

void Adafruit_MCP23017::begin(uint8_t addr, TwoWire &wirePort) {
  _wire = &wirePort;
  begin(addr);
}
#endif

void Adafruit_MCP23017::pinMode(uint8_t p, uint8_t d) {
  updateRegisterBit(p, (d == INPUT), MCP23017_IODIRA, MCP23017_IODIRB);
}

uint16_t Adafruit_MCP23017::readINTCAPAB() {
  uint16_t ba = 0;
  uint8_t a = 0;

#if !defined(ARDUINO_AVR_GEMMA)
  _wire->beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(MCP23017_INTCAPA);
  _wire->endTransmission();

  _wire->requestFrom((uint8_t)(MCP23017_ADDRESS | i2caddr), (uint8_t)1);
  a = wireRecv();

  _wire->beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(MCP23017_INTCAPB);
  _wire->endTransmission();

  _wire->requestFrom((uint8_t)(MCP23017_ADDRESS | i2caddr), (uint8_t)1);
  ba = wireRecv();
#else
  Wire.beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(MCP23017_INTCAPA);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)(MCP23017_ADDRESS | i2caddr), (uint8_t)1);
  a = wireRecv();

  Wire.beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(MCP23017_INTCAPB);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)(MCP23017_ADDRESS | i2caddr), (uint8_t)1);
  ba = wireRecv();
#endif

  ba <<= 8u;
  ba |= a;
  return ba;
}

uint16_t Adafruit_MCP23017::readGPIOAB() {
  uint16_t ba = 0;
  uint8_t a = 0;

#if !defined(ARDUINO_AVR_GEMMA)
  _wire->beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(MCP23017_GPIOA);
  _wire->endTransmission();

  _wire->requestFrom((uint8_t)(MCP23017_ADDRESS | i2caddr), (uint8_t)2);
  a = wireRecv();
  ba = wireRecv();
#else
  Wire.beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(MCP23017_GPIOA);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)(MCP23017_ADDRESS | i2caddr), (uint8_t)2);
  a = wireRecv();
  ba = wireRecv();
#endif

  ba <<= 8u;
  ba |= a;
  return ba;
}

uint8_t Adafruit_MCP23017::readGPIO(uint8_t b) {
#if !defined(ARDUINO_AVR_GEMMA)
  _wire->beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend((b == 0) ? MCP23017_GPIOA : MCP23017_GPIOB);
  _wire->endTransmission();

  _wire->requestFrom((uint8_t)(MCP23017_ADDRESS | i2caddr), (uint8_t)1);
  return wireRecv();
#else
  Wire.beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend((b == 0) ? MCP23017_GPIOA : MCP23017_GPIOB);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)(MCP23017_ADDRESS | i2caddr), (uint8_t)1);
  return wireRecv();
#endif
}

void Adafruit_MCP23017::writeGPIOAB(uint16_t ba) {
#if !defined(ARDUINO_AVR_GEMMA)
  _wire->beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(MCP23017_GPIOA);
  wireSend((uint8_t)(ba & 0xFF));
  wireSend((uint8_t)(ba >> 8));
  _wire->endTransmission();
#else
  Wire.beginTransmission((uint8_t)(MCP23017_ADDRESS | i2caddr));
  wireSend(MCP23017_GPIOA);
  wireSend((uint8_t)(ba & 0xFF));
  wireSend((uint8_t)(ba >> 8));
  Wire.endTransmission();
#endif
}

void Adafruit_MCP23017::digitalWrite(uint8_t pin, uint8_t d) {
  uint8_t bit = bitForPin(pin);

  uint8_t regAddr = regForPin(pin, MCP23017_OLATA, MCP23017_OLATB);
  uint8_t gpio = readRegister(regAddr);

  bitWrite(gpio, bit, d);

  regAddr = regForPin(pin, MCP23017_GPIOA, MCP23017_GPIOB);
  writeRegister(regAddr, gpio);
}

void Adafruit_MCP23017::pullUp(uint8_t p, uint8_t d) {
  updateRegisterBit(p, d, MCP23017_GPPUA, MCP23017_GPPUB);
}

uint8_t Adafruit_MCP23017::digitalRead(uint8_t pin) {
  uint8_t bit = bitForPin(pin);
  uint8_t regAddr = regForPin(pin, MCP23017_GPIOA, MCP23017_GPIOB);
  return (uint8_t)((readRegister(regAddr) >> bit) & 0x01);
}

void Adafruit_MCP23017::setupInterrupts(uint8_t mirroring, uint8_t openDrain, uint8_t polarity) {
  uint8_t ioconfValue = readRegister(MCP23017_IOCONA);
  bitWrite(ioconfValue, 6, mirroring);
  bitWrite(ioconfValue, 2, openDrain);
  bitWrite(ioconfValue, 1, polarity);
  writeRegister(MCP23017_IOCONA, ioconfValue);

  ioconfValue = readRegister(MCP23017_IOCONB);
  bitWrite(ioconfValue, 6, mirroring);
  bitWrite(ioconfValue, 2, openDrain);
  bitWrite(ioconfValue, 1, polarity);
  writeRegister(MCP23017_IOCONB, ioconfValue);
}

void Adafruit_MCP23017::setupInterruptPin(uint8_t pin, uint8_t mode) {
  updateRegisterBit(pin, (mode != CHANGE), MCP23017_INTCONA, MCP23017_INTCONB);
  updateRegisterBit(pin, (mode == FALLING), MCP23017_DEFVALA, MCP23017_DEFVALB);
  updateRegisterBit(pin, HIGH, MCP23017_GPINTENA, MCP23017_GPINTENB);
}

uint8_t Adafruit_MCP23017::getLastInterruptPin() {
  uint8_t intf = readRegister(MCP23017_INTFA);
  for (int i = 0; i < 8; i++) if (bitRead(intf, i)) return (uint8_t)i;

  intf = readRegister(MCP23017_INTFB);
  for (int i = 0; i < 8; i++) if (bitRead(intf, i)) return (uint8_t)(i + 8);

  return MCP23017_INT_ERR;
}

uint8_t Adafruit_MCP23017::getLastInterruptPinValue() {
  uint8_t intPin = getLastInterruptPin();
  if (intPin != MCP23017_INT_ERR) {
    uint8_t intcapreg = regForPin(intPin, MCP23017_INTCAPA, MCP23017_INTCAPB);
    uint8_t bit = bitForPin(intPin);
    return (uint8_t)((readRegister(intcapreg) >> bit) & 0x01);
  }
  return MCP23017_INT_ERR;
}