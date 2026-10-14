/*!
 * @file note_arduino_fakes.hpp
 *
 * FFF fakes for the note-arduino interfaces (NoteI2c, NoteLog, NoteSerial,
 * NoteTxn, NoteTime) consumed by the Notecard class. The *_Fake classes are
 * stateless stubs that forward every virtual method to an FFF fake, because
 * FFF cannot fake C++ member functions directly. The make_note_*(nullptr)
 * overloads are likewise C++ overloads and forward to FFF fakes.
 */

#ifndef NOTE_ARDUINO_FAKES_HPP
#define NOTE_ARDUINO_FAKES_HPP

#include <stddef.h>
#include <stdint.h>

#include "fff.h"

#include "NoteI2c.hpp"
#include "NoteLog.hpp"
#include "NoteSerial.hpp"
#include "NoteTime.h"
#include "NoteTxn.hpp"

DECLARE_FAKE_VALUE_FUNC(const char *, NoteI2c_receive, uint16_t, uint8_t *, uint16_t, uint32_t *)
DECLARE_FAKE_VALUE_FUNC(bool, NoteI2c_reset, uint16_t)
DECLARE_FAKE_VALUE_FUNC(const char *, NoteI2c_transmit, uint16_t, uint8_t *, uint16_t)
DECLARE_FAKE_VALUE_FUNC(size_t, NoteLog_print, const char *)
DECLARE_FAKE_VALUE_FUNC(size_t, NoteSerial_available)
DECLARE_FAKE_VALUE_FUNC(char, NoteSerial_receive)
DECLARE_FAKE_VALUE_FUNC(bool, NoteSerial_reset)
DECLARE_FAKE_VALUE_FUNC(size_t, NoteSerial_transmit, uint8_t *, size_t, bool)
DECLARE_FAKE_VALUE_FUNC(bool, NoteTxn_start, uint32_t)
DECLARE_FAKE_VOID_FUNC(NoteTxn_stop)
DECLARE_FAKE_VALUE_FUNC(NoteI2c *, make_note_i2c_nullptr)
DECLARE_FAKE_VALUE_FUNC(NoteLog *, make_note_log_nullptr)
DECLARE_FAKE_VALUE_FUNC(NoteSerial *, make_note_serial_nullptr)
DECLARE_FAKE_VALUE_FUNC(NoteTxn *, make_note_txn_nullptr)
DECLARE_FAKE_VOID_FUNC(noteDelay, uint32_t)
DECLARE_FAKE_VALUE_FUNC(uint32_t, noteMillis)

class NoteI2c_Fake final : public NoteI2c
{
public:
    const char * receive(uint16_t device_address, uint8_t * buffer, uint16_t requested_byte_count, uint32_t * available) override
    { return NoteI2c_receive(device_address, buffer, requested_byte_count, available); }
    bool reset(uint16_t device_address) override
    { return NoteI2c_reset(device_address); }
    const char * transmit(uint16_t device_address, uint8_t * buffer, uint16_t size) override
    { return NoteI2c_transmit(device_address, buffer, size); }
};

class NoteLog_Fake final : public NoteLog
{
public:
    size_t print(const char * message) override
    { return NoteLog_print(message); }
};

class NoteSerial_Fake final : public NoteSerial
{
public:
    size_t available(void) override { return NoteSerial_available(); }
    char receive(void) override { return NoteSerial_receive(); }
    bool reset(void) override { return NoteSerial_reset(); }
    size_t transmit(uint8_t * buffer, size_t size, bool flush) override
    { return NoteSerial_transmit(buffer, size, flush); }
};

class NoteTxn_Fake final : public NoteTxn
{
public:
    bool start(uint32_t timeout_ms) override { return NoteTxn_start(timeout_ms); }
    void stop(void) override { NoteTxn_stop(); }
};


// Apply X to every fake above (e.g. X = RESET_FAKE)
#define NOTE_ARDUINO_FAKES_LIST(X) \
    X(NoteI2c_receive); \
    X(NoteI2c_reset); \
    X(NoteI2c_transmit); \
    X(NoteLog_print); \
    X(NoteSerial_available); \
    X(NoteSerial_receive); \
    X(NoteSerial_reset); \
    X(NoteSerial_transmit); \
    X(NoteTxn_start); \
    X(NoteTxn_stop); \
    X(make_note_i2c_nullptr); \
    X(make_note_log_nullptr); \
    X(make_note_serial_nullptr); \
    X(make_note_txn_nullptr); \
    X(noteDelay); \
    X(noteMillis);

#endif // NOTE_ARDUINO_FAKES_HPP
