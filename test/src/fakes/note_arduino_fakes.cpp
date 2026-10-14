/*!
 * @file note_arduino_fakes.cpp
 *
 * FFF fake definitions for the note-arduino interfaces used by Notecard.
 */

#include "note_arduino_fakes.hpp"

DEFINE_FAKE_VALUE_FUNC(const char *, NoteI2c_receive, uint16_t, uint8_t *, uint16_t, uint32_t *)
DEFINE_FAKE_VALUE_FUNC(bool, NoteI2c_reset, uint16_t)
DEFINE_FAKE_VALUE_FUNC(const char *, NoteI2c_transmit, uint16_t, uint8_t *, uint16_t)
DEFINE_FAKE_VALUE_FUNC(size_t, NoteLog_print, const char *)
DEFINE_FAKE_VALUE_FUNC(size_t, NoteSerial_available)
DEFINE_FAKE_VALUE_FUNC(char, NoteSerial_receive)
DEFINE_FAKE_VALUE_FUNC(bool, NoteSerial_reset)
DEFINE_FAKE_VALUE_FUNC(size_t, NoteSerial_transmit, uint8_t *, size_t, bool)
DEFINE_FAKE_VALUE_FUNC(bool, NoteTxn_start, uint32_t)
DEFINE_FAKE_VOID_FUNC(NoteTxn_stop)
DEFINE_FAKE_VALUE_FUNC(NoteI2c *, make_note_i2c_nullptr)
DEFINE_FAKE_VALUE_FUNC(NoteLog *, make_note_log_nullptr)
DEFINE_FAKE_VALUE_FUNC(NoteSerial *, make_note_serial_nullptr)
DEFINE_FAKE_VALUE_FUNC(NoteTxn *, make_note_txn_nullptr)
DEFINE_FAKE_VOID_FUNC(noteDelay, uint32_t)
DEFINE_FAKE_VALUE_FUNC(uint32_t, noteMillis)

// make_note_*(nullptr) are C++ overloads, so they forward to FFF fakes.
NoteI2c * make_note_i2c(nullptr_t) { return make_note_i2c_nullptr(); }
NoteLog * make_note_log(nullptr_t) { return make_note_log_nullptr(); }
NoteSerial * make_note_serial(nullptr_t) { return make_note_serial_nullptr(); }
NoteTxn * make_note_txn(nullptr_t) { return make_note_txn_nullptr(); }
