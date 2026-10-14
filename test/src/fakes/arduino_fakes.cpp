/*!
 * @file arduino_fakes.cpp
 *
 * FFF fake definitions for the Arduino stand-in (test/include/Arduino.h),
 * plus the stateless class-method stubs that forward to them.
 */

#include "Arduino.h"

DEFINE_FAKE_VOID_FUNC(delay, unsigned long)
DEFINE_FAKE_VALUE_FUNC(int, digitalRead, uint8_t)
DEFINE_FAKE_VOID_FUNC(digitalWrite, uint8_t, uint8_t)
DEFINE_FAKE_VALUE_FUNC(unsigned long, millis)
DEFINE_FAKE_VOID_FUNC(pinMode, uint8_t, uint8_t)

DEFINE_FAKE_VALUE_FUNC(size_t, Stream_print, const char *)
DEFINE_FAKE_VALUE_FUNC(bool, HardwareSerial_operator_bool)
DEFINE_FAKE_VALUE_FUNC(unsigned int, HardwareSerial_available)
DEFINE_FAKE_VOID_FUNC(HardwareSerial_begin, unsigned int)
DEFINE_FAKE_VOID_FUNC(HardwareSerial_end)
DEFINE_FAKE_VOID_FUNC(HardwareSerial_flush)
DEFINE_FAKE_VALUE_FUNC(char, HardwareSerial_read)
DEFINE_FAKE_VALUE_FUNC(size_t, HardwareSerial_write, uint8_t *, size_t)
DEFINE_FAKE_VOID_FUNC(TwoWire_begin)
DEFINE_FAKE_VOID_FUNC(TwoWire_beginTransmission, uint8_t)
DEFINE_FAKE_VOID_FUNC(TwoWire_end)
DEFINE_FAKE_VALUE_FUNC(uint8_t, TwoWire_endTransmission)
DEFINE_FAKE_VALUE_FUNC(int, TwoWire_read)
DEFINE_FAKE_VALUE_FUNC(uint8_t, TwoWire_requestFrom, int, int)
DEFINE_FAKE_VALUE_FUNC(size_t, TwoWire_writeByte, uint8_t)
DEFINE_FAKE_VALUE_FUNC(size_t, TwoWire_writeBuffer, uint8_t *, size_t)

// Global Arduino objects (singletons)
HardwareSerial Serial;
TwoWire Wire;

// Stateless stubs: FFF cannot fake C++ member functions, so each one simply
// forwards to its FFF fake.
size_t Stream::print(const char * str) { return Stream_print(str); }

HardwareSerial::operator bool() { return HardwareSerial_operator_bool(); }
unsigned int HardwareSerial::available(void) { return HardwareSerial_available(); }
void HardwareSerial::begin(unsigned int baud) { HardwareSerial_begin(baud); }
void HardwareSerial::end(void) { HardwareSerial_end(); }
void HardwareSerial::flush(void) { HardwareSerial_flush(); }
char HardwareSerial::read(void) { return HardwareSerial_read(); }
size_t HardwareSerial::write(uint8_t * buffer, size_t size) { return HardwareSerial_write(buffer, size); }

void TwoWire::begin(void) { TwoWire_begin(); }
void TwoWire::beginTransmission(uint8_t address) { TwoWire_beginTransmission(address); }
void TwoWire::end(void) { TwoWire_end(); }
uint8_t TwoWire::endTransmission(void) { return TwoWire_endTransmission(); }
int TwoWire::read(void) { return TwoWire_read(); }
uint8_t TwoWire::requestFrom(int address, int quantity) { return TwoWire_requestFrom(address, quantity); }
size_t TwoWire::write(uint8_t c) { return TwoWire_writeByte(c); }
size_t TwoWire::write(uint8_t * buffer, size_t size) { return TwoWire_writeBuffer(buffer, size); }
