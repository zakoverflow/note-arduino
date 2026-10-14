/*!
 * @file Arduino.h
 *
 * Minimal host-side stand-in for the Arduino core, used only by the unit
 * tests. Free functions are FFF fakes; C++ class methods (which FFF cannot
 * fake directly) are stateless stubs that forward to FFF fakes defined in
 * test/src/fakes/arduino_fakes.cpp.
 */

#ifndef NOTE_ARDUINO_TEST_ARDUINO_H
#define NOTE_ARDUINO_TEST_ARDUINO_H

#include <stddef.h>
#include <stdint.h>

#include "fff.h"

#ifndef NOTE_C_STRINGIZE
#define _NOTE_C_STRINGIZE(x) #x
#define NOTE_C_STRINGIZE(x) _NOTE_C_STRINGIZE(x)
#endif

#define SERIAL_RX_BUFFER_SIZE 79

enum PinMode {
    INPUT = 0x19,
    INPUT_PULLUP,
    OUTPUT,
};

enum PinState {
    HIGH = 0x79,
    LOW,
};

struct Stream {
    virtual size_t print(const char * str);
};

struct HardwareSerial : public Stream {
    operator bool();
    unsigned int available(void);
    void begin(unsigned int baud);
    void end(void);
    void flush(void);
    char read(void);
    size_t write(uint8_t * buffer, size_t size);
};

struct TwoWire {
    void begin(void);
    void beginTransmission(uint8_t address);
    void end(void);
    uint8_t endTransmission(void);
    int read(void);
    uint8_t requestFrom(int address, int quantity);
    size_t write(uint8_t c);
    size_t write(uint8_t * buffer, size_t size);
};

extern HardwareSerial Serial;
extern TwoWire Wire;

// Arduino core free functions
DECLARE_FAKE_VOID_FUNC(delay, unsigned long)
DECLARE_FAKE_VALUE_FUNC(int, digitalRead, uint8_t)
DECLARE_FAKE_VOID_FUNC(digitalWrite, uint8_t, uint8_t)
DECLARE_FAKE_VALUE_FUNC(unsigned long, millis)
DECLARE_FAKE_VOID_FUNC(pinMode, uint8_t, uint8_t)

// Class method fakes (forwarded to by the stubs above)
DECLARE_FAKE_VALUE_FUNC(size_t, Stream_print, const char *)
DECLARE_FAKE_VALUE_FUNC(bool, HardwareSerial_operator_bool)
DECLARE_FAKE_VALUE_FUNC(unsigned int, HardwareSerial_available)
DECLARE_FAKE_VOID_FUNC(HardwareSerial_begin, unsigned int)
DECLARE_FAKE_VOID_FUNC(HardwareSerial_end)
DECLARE_FAKE_VOID_FUNC(HardwareSerial_flush)
DECLARE_FAKE_VALUE_FUNC(char, HardwareSerial_read)
DECLARE_FAKE_VALUE_FUNC(size_t, HardwareSerial_write, uint8_t *, size_t)
DECLARE_FAKE_VOID_FUNC(TwoWire_begin)
DECLARE_FAKE_VOID_FUNC(TwoWire_beginTransmission, uint8_t)
DECLARE_FAKE_VOID_FUNC(TwoWire_end)
DECLARE_FAKE_VALUE_FUNC(uint8_t, TwoWire_endTransmission)
DECLARE_FAKE_VALUE_FUNC(int, TwoWire_read)
DECLARE_FAKE_VALUE_FUNC(uint8_t, TwoWire_requestFrom, int, int)
DECLARE_FAKE_VALUE_FUNC(size_t, TwoWire_writeByte, uint8_t)
DECLARE_FAKE_VALUE_FUNC(size_t, TwoWire_writeBuffer, uint8_t *, size_t)


// Apply X to every fake above (e.g. X = RESET_FAKE)
#define ARDUINO_FAKES_LIST(X) \
    X(delay); \
    X(digitalRead); \
    X(digitalWrite); \
    X(millis); \
    X(pinMode); \
    X(Stream_print); \
    X(HardwareSerial_operator_bool); \
    X(HardwareSerial_available); \
    X(HardwareSerial_begin); \
    X(HardwareSerial_end); \
    X(HardwareSerial_flush); \
    X(HardwareSerial_read); \
    X(HardwareSerial_write); \
    X(TwoWire_begin); \
    X(TwoWire_beginTransmission); \
    X(TwoWire_end); \
    X(TwoWire_endTransmission); \
    X(TwoWire_read); \
    X(TwoWire_requestFrom); \
    X(TwoWire_writeByte); \
    X(TwoWire_writeBuffer);

#endif // NOTE_ARDUINO_TEST_ARDUINO_H
