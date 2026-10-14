/*!
 * @file note_c_fakes.cpp
 *
 * FFF fake definitions for the note-c API used by note-arduino.
 */

#include "note-c/note.h"

DEFINE_FAKE_VOID_FUNC(NoteDebug, const char *)
DEFINE_FAKE_VALUE_FUNC(bool, NoteDebugSyncStatus, int, int)
DEFINE_FAKE_VOID_FUNC(NoteDebugWithLevel, uint8_t, const char *)
DEFINE_FAKE_VOID_FUNC(NoteDebugWithLevelLn, uint8_t, const char *)
DEFINE_FAKE_VOID_FUNC(NoteDelayMs, uint32_t)
DEFINE_FAKE_VOID_FUNC(NoteDeleteResponse, J *)
DEFINE_FAKE_VOID_FUNC(NoteGetFnI2C, uint32_t *, uint32_t *, i2cResetFn *, i2cTransmitFn *, i2cReceiveFn *)
DEFINE_FAKE_VOID_FUNC(NoteGetFnSerial, serialResetFn *, serialTransmitFn *, serialAvailableFn *, serialReceiveFn *)
DEFINE_FAKE_VALUE_FUNC(uint32_t, NoteGetMs)
DEFINE_FAKE_VALUE_FUNC(J *, NoteNewCommand, const char *)
DEFINE_FAKE_VALUE_FUNC(J *, NoteNewRequest, const char *)
DEFINE_FAKE_VALUE_FUNC(bool, NoteRequest, J *)
DEFINE_FAKE_VALUE_FUNC(J *, NoteRequestResponse, J *)
DEFINE_FAKE_VALUE_FUNC(J *, NoteRequestResponseWithRetry, J *, uint32_t)
DEFINE_FAKE_VALUE_FUNC(bool, NoteRequestWithRetry, J *, uint32_t)
DEFINE_FAKE_VALUE_FUNC(bool, NoteResponseError, J *)
DEFINE_FAKE_VOID_FUNC(NoteSetFn, mallocFn, freeFn, delayMsFn, getMsFn)
DEFINE_FAKE_VOID_FUNC(NoteSetFnDebugOutput, debugOutputFn)
DEFINE_FAKE_VOID_FUNC(NoteSetFnDefault, mallocFn, freeFn, delayMsFn, getMsFn)
DEFINE_FAKE_VOID_FUNC(NoteSetFnI2C, uint32_t, uint32_t, i2cResetFn, i2cTransmitFn, i2cReceiveFn)
DEFINE_FAKE_VOID_FUNC(NoteSetFnI2CDefault, uint32_t, uint32_t, i2cResetFn, i2cTransmitFn, i2cReceiveFn)
DEFINE_FAKE_VOID_FUNC(NoteSetFnI2CMutex, mutexFn, mutexFn)
DEFINE_FAKE_VOID_FUNC(NoteSetFnNoteMutex, mutexFn, mutexFn)
DEFINE_FAKE_VOID_FUNC(NoteSetFnSerial, serialResetFn, serialTransmitFn, serialAvailableFn, serialReceiveFn)
DEFINE_FAKE_VOID_FUNC(NoteSetFnSerialDefault, serialResetFn, serialTransmitFn, serialAvailableFn, serialReceiveFn)
DEFINE_FAKE_VOID_FUNC(NoteSetFnTransaction, txnStartFn, txnStopFn)
DEFINE_FAKE_VOID_FUNC(NoteSetI2CAddress, uint32_t)
DEFINE_FAKE_VOID_FUNC(NoteSetI2CMtu, uint32_t)
DEFINE_FAKE_VOID_FUNC(NoteSetUserAgent, char *)
