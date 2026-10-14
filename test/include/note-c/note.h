/*!
 * @file note.h (test wrapper)
 *
 * The unit tests compile note-arduino against the real note-c header, but
 * never link note-c itself. Every note-c function used by note-arduino is an
 * FFF fake (see test/src/fakes/note_c_fakes.cpp).
 *
 * note-c implements NoteDeleteResponse() and NoteResponseError() as macros,
 * which FFF cannot fake, so this wrapper turns them into plain functions.
 */

#ifndef NOTE_ARDUINO_TEST_NOTE_H
#define NOTE_ARDUINO_TEST_NOTE_H

#include "../../../src/note-c/note.h"
#include "fff.h"

#undef NoteDeleteResponse
#undef NoteResponseError

DECLARE_FAKE_VOID_FUNC(NoteDebug, const char *)
DECLARE_FAKE_VALUE_FUNC(bool, NoteDebugSyncStatus, int, int)
DECLARE_FAKE_VOID_FUNC(NoteDebugWithLevel, uint8_t, const char *)
DECLARE_FAKE_VOID_FUNC(NoteDebugWithLevelLn, uint8_t, const char *)
DECLARE_FAKE_VOID_FUNC(NoteDelayMs, uint32_t)
DECLARE_FAKE_VOID_FUNC(NoteDeleteResponse, J *)
DECLARE_FAKE_VOID_FUNC(NoteGetFnI2C, uint32_t *, uint32_t *, i2cResetFn *, i2cTransmitFn *, i2cReceiveFn *)
DECLARE_FAKE_VOID_FUNC(NoteGetFnSerial, serialResetFn *, serialTransmitFn *, serialAvailableFn *, serialReceiveFn *)
DECLARE_FAKE_VALUE_FUNC(uint32_t, NoteGetMs)
DECLARE_FAKE_VALUE_FUNC(J *, NoteNewCommand, const char *)
DECLARE_FAKE_VALUE_FUNC(J *, NoteNewRequest, const char *)
DECLARE_FAKE_VALUE_FUNC(bool, NoteRequest, J *)
DECLARE_FAKE_VALUE_FUNC(J *, NoteRequestResponse, J *)
DECLARE_FAKE_VALUE_FUNC(J *, NoteRequestResponseWithRetry, J *, uint32_t)
DECLARE_FAKE_VALUE_FUNC(bool, NoteRequestWithRetry, J *, uint32_t)
DECLARE_FAKE_VALUE_FUNC(bool, NoteResponseError, J *)
DECLARE_FAKE_VOID_FUNC(NoteSetFn, mallocFn, freeFn, delayMsFn, getMsFn)
DECLARE_FAKE_VOID_FUNC(NoteSetFnDebugOutput, debugOutputFn)
DECLARE_FAKE_VOID_FUNC(NoteSetFnDefault, mallocFn, freeFn, delayMsFn, getMsFn)
DECLARE_FAKE_VOID_FUNC(NoteSetFnI2C, uint32_t, uint32_t, i2cResetFn, i2cTransmitFn, i2cReceiveFn)
DECLARE_FAKE_VOID_FUNC(NoteSetFnI2CDefault, uint32_t, uint32_t, i2cResetFn, i2cTransmitFn, i2cReceiveFn)
DECLARE_FAKE_VOID_FUNC(NoteSetFnI2CMutex, mutexFn, mutexFn)
DECLARE_FAKE_VOID_FUNC(NoteSetFnNoteMutex, mutexFn, mutexFn)
DECLARE_FAKE_VOID_FUNC(NoteSetFnSerial, serialResetFn, serialTransmitFn, serialAvailableFn, serialReceiveFn)
DECLARE_FAKE_VOID_FUNC(NoteSetFnSerialDefault, serialResetFn, serialTransmitFn, serialAvailableFn, serialReceiveFn)
DECLARE_FAKE_VOID_FUNC(NoteSetFnTransaction, txnStartFn, txnStopFn)
DECLARE_FAKE_VOID_FUNC(NoteSetI2CAddress, uint32_t)
DECLARE_FAKE_VOID_FUNC(NoteSetI2CMtu, uint32_t)
DECLARE_FAKE_VOID_FUNC(NoteSetUserAgent, char *)


// Apply X to every fake above (e.g. X = RESET_FAKE)
#define NOTE_C_FAKES_LIST(X) \
    X(NoteDebug); \
    X(NoteDebugSyncStatus); \
    X(NoteDebugWithLevel); \
    X(NoteDebugWithLevelLn); \
    X(NoteDelayMs); \
    X(NoteDeleteResponse); \
    X(NoteGetFnI2C); \
    X(NoteGetFnSerial); \
    X(NoteGetMs); \
    X(NoteNewCommand); \
    X(NoteNewRequest); \
    X(NoteRequest); \
    X(NoteRequestResponse); \
    X(NoteRequestResponseWithRetry); \
    X(NoteRequestWithRetry); \
    X(NoteResponseError); \
    X(NoteSetFn); \
    X(NoteSetFnDebugOutput); \
    X(NoteSetFnDefault); \
    X(NoteSetFnI2C); \
    X(NoteSetFnI2CDefault); \
    X(NoteSetFnI2CMutex); \
    X(NoteSetFnNoteMutex); \
    X(NoteSetFnSerial); \
    X(NoteSetFnSerialDefault); \
    X(NoteSetFnTransaction); \
    X(NoteSetI2CAddress); \
    X(NoteSetI2CMtu); \
    X(NoteSetUserAgent);

#endif // NOTE_ARDUINO_TEST_NOTE_H
