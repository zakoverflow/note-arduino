/*!
 * @file NoteTxn_Arduino_test.cpp
 *
 * Catch2 unit tests for NoteTxn_Arduino, migrated from the hand-rolled
 * test/NoteTxn_Arduino.test.cpp harness. Each TEST_CASE keeps the original test
 * function name and assertion. Dependencies are FFF fakes.
 */

#include <catch2/catch_test_macros.hpp>
#include "fff.h"

#include "NoteTxn_Arduino.hpp"
#include <cstring>
#include <vector>
#include "Arduino.h"
#include "note-c/note.h"

DEFINE_FFF_GLOBALS

namespace
{

void resetFakes(void)
{
    ARDUINO_FAKES_LIST(RESET_FAKE)
    NOTE_C_FAKES_LIST(RESET_FAKE)
    FFF_RESET_HISTORY();

    // Like a real, open Arduino serial port, `Serial` evaluates as true.
    HardwareSerial_operator_bool_fake.return_val = true;
}

// The NoteTxn tests reason about calls per pin. These helpers rebuild that
// per-pin view from the FFF argument history of the pinMode() and
// digitalWrite() fakes.
std::vector<uint8_t> pinModeCalls(uint8_t pin)
{
    std::vector<uint8_t> modes;
    const unsigned int count = (pinMode_fake.call_count < FFF_ARG_HISTORY_LEN) ? pinMode_fake.call_count : FFF_ARG_HISTORY_LEN;
    for (unsigned int i = 0; i < count; ++i) {
        if (pinMode_fake.arg0_history[i] == pin) {
            modes.push_back(pinMode_fake.arg1_history[i]);
        }
    }
    return modes;
}

std::vector<uint8_t> digitalWriteCalls(uint8_t pin)
{
    std::vector<uint8_t> vals;
    const unsigned int count = (digitalWrite_fake.call_count < FFF_ARG_HISTORY_LEN) ? digitalWrite_fake.call_count : FFF_ARG_HISTORY_LEN;
    for (unsigned int i = 0; i < count; ++i) {
        if (digitalWrite_fake.arg0_history[i] == pin) {
            vals.push_back(digitalWrite_fake.arg1_history[i]);
        }
    }
    return vals;
}

unsigned int digitalReadCalls(uint8_t pin)
{
    unsigned int reads = 0;
    const unsigned int count = (digitalRead_fake.call_count < FFF_ARG_HISTORY_LEN) ? digitalRead_fake.call_count : FFF_ARG_HISTORY_LEN;
    for (unsigned int i = 0; i < count; ++i) {
        if (digitalRead_fake.arg0_history[i] == pin) {
            ++reads;
        }
    }
    return reads;
}

} // namespace



TEST_CASE("test_make_note_txn_instantiates_notetxn_object")
{
  resetFakes();
  // Arrange
  NoteTxn * notetxn = nullptr;
  uint8_t txn_pins[2] = {19, 79};

  // Action
  notetxn = make_note_txn(txn_pins);

  // Assert
  CHECK(nullptr != notetxn);

  // Clean-up
  uint8_t invalid_pins[2] = {0};
  make_note_txn(invalid_pins);
}

TEST_CASE("test_make_note_txn_enforces_singleton_by_returning_same_notetxn_object_for_all_calls")
{
  resetFakes();
  // Arrange
  uint8_t txn_pins_1[2] = {19, 79};
  NoteTxn * const notetxn_1 = make_note_txn(txn_pins_1);

  // Action
  uint8_t txn_pins_2[2] = {9, 17};
  NoteTxn * const notetxn_2 = make_note_txn(txn_pins_2);

  // Assert
  CHECK(notetxn_1 == notetxn_2);

  // Clean-up
  uint8_t invalid_pins[2] = {0};
  make_note_txn(invalid_pins);
}

//int test_make_note_txn_returns_nullptr_when_same_pins_are_passed_as_parameter()
TEST_CASE("test_make_note_txn_deletes_singleton_when_same_pins_are_passed_as_parameter")
{
  resetFakes();
  // Arrange
  uint8_t txn_pins[2] = {19, 79};
  NoteTxn * notetxn = make_note_txn(txn_pins);
  REQUIRE(notetxn);

  // Action
  uint8_t invalid_pins[2] = {0};
  notetxn = make_note_txn(invalid_pins);

  // Assert
  CHECK(nullptr == notetxn);
}

TEST_CASE("test_notetxn_arduino_constructor_floats_ctx_pin")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  RESET_FAKE(pinMode);

  // Action
  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);

  // Assert
  CHECK(pinModeCalls(CTX_PIN).back() == INPUT);
}

TEST_CASE("test_notetxn_arduino_constructor_floats_rtx_pin")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  RESET_FAKE(pinMode);

  // Action
  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);

  // Assert
  CHECK(pinModeCalls(RTX_PIN).back() == INPUT);
}

TEST_CASE("test_notetxn_arduino_start_initially_configures_ctx_pin_as_input_pullup")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  const size_t TIMEOUT_MS = 917;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalRead);
  digitalRead_fake.return_val = HIGH;
  RESET_FAKE(pinMode);

  // Action
  notetxn.start(TIMEOUT_MS);

  // Assert
  CHECK((!pinModeCalls(CTX_PIN).empty() && pinModeCalls(CTX_PIN)[0] == INPUT_PULLUP));
}

TEST_CASE("test_notetxn_arduino_start_configures_rtx_pin_as_output")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  const size_t TIMEOUT_MS = 917;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalRead);
  digitalRead_fake.return_val = HIGH;
  RESET_FAKE(pinMode);

  // Action
  notetxn.start(TIMEOUT_MS);

  // Assert
  CHECK((!pinModeCalls(RTX_PIN).empty() && pinModeCalls(RTX_PIN).back() == OUTPUT));
}

TEST_CASE("test_notetxn_arduino_start_sets_rtx_pin_high")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  const size_t TIMEOUT_MS = 917;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalWrite);
  RESET_FAKE(digitalRead);
  digitalRead_fake.return_val = HIGH;

  // Action
  notetxn.start(TIMEOUT_MS);

  // Assert
  CHECK((!digitalWriteCalls(RTX_PIN).empty() && digitalWriteCalls(RTX_PIN).back() == HIGH));
}

TEST_CASE("test_notetxn_arduino_start_blocks_until_ctx_pin_goes_high")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  const size_t TIMEOUT_MS = 917;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalRead);
  int digitalRead_results[] = {LOW, LOW, LOW, HIGH};
  SET_RETURN_SEQ(digitalRead, digitalRead_results, (sizeof(digitalRead_results) / sizeof(digitalRead_results[0])));
  RESET_FAKE(NoteGetMs);
  NoteGetMs_fake.return_val = (TIMEOUT_MS - 1);

  // Action
  notetxn.start(TIMEOUT_MS);

  // Assert
  // Only returns value less than `TIMEOUT_MS`
  CHECK((digitalReadCalls(CTX_PIN) == static_cast<unsigned int>(digitalRead_fake.return_val_seq_len) && digitalRead_results[(digitalReadCalls(CTX_PIN) - 1)] == HIGH && (0 == NoteGetMs_fake.return_val_seq_len)));
}

TEST_CASE("test_notetxn_arduino_start_blocks_until_timeout_ms")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  const size_t TIMEOUT_MS = 917;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalRead);
  digitalRead_fake.return_val = LOW;
  RESET_FAKE(NoteDelayMs);
  RESET_FAKE(NoteGetMs);
  uint32_t NoteGetMs_results[] = {36, 274, 515, 801, TIMEOUT_MS + 36};
  SET_RETURN_SEQ(NoteGetMs, NoteGetMs_results, (sizeof(NoteGetMs_results) / sizeof(NoteGetMs_results[0])));

  // Action
  notetxn.start(TIMEOUT_MS);

  // Assert
  // Called at least once
  // Only returns `LOW`
  CHECK((digitalReadCalls(CTX_PIN) && (0 == digitalRead_fake.return_val_seq_len) && NoteGetMs_fake.call_count == static_cast<unsigned int>(NoteGetMs_fake.return_val_seq_len)));
}

TEST_CASE("test_notetxn_arduino_start_leaves_rtx_pin_as_output_when_ctx_responds_high")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  const size_t TIMEOUT_MS = 917;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalRead);
  digitalRead_fake.return_val = HIGH;
  RESET_FAKE(millis);
  RESET_FAKE(pinMode);

  // Action
  notetxn.start(TIMEOUT_MS);

  // Assert
  // Called at least once
  // Only returns `LOW`
  CHECK((digitalReadCalls(CTX_PIN) && (0 == digitalRead_fake.return_val_seq_len) && pinModeCalls(RTX_PIN).back() == OUTPUT));
}

TEST_CASE("test_notetxn_arduino_start_leaves_rtx_pin_high_when_ctx_responds_high")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  const size_t TIMEOUT_MS = 917;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalRead);
  digitalRead_fake.return_val = HIGH;
  RESET_FAKE(digitalWrite);
  RESET_FAKE(NoteGetMs);

  // Action
  notetxn.start(TIMEOUT_MS);

  // Assert
  // Called at least once
  // Only returns `LOW`
  CHECK((digitalReadCalls(CTX_PIN) && (0 == digitalRead_fake.return_val_seq_len) && digitalWriteCalls(RTX_PIN).back() == HIGH));
}

TEST_CASE("test_notetxn_arduino_start_leaves_ctx_pin_floating_when_ctx_responds_high")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  const size_t TIMEOUT_MS = 917;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalRead);
  digitalRead_fake.return_val = HIGH;
  RESET_FAKE(pinMode);

  // Action
  notetxn.start(TIMEOUT_MS);

  // Assert
  // Called at least twice
  CHECK((pinModeCalls(CTX_PIN).size() > 1 && pinModeCalls(CTX_PIN).back() == INPUT));
}

TEST_CASE("test_notetxn_arduino_start_leaves_ctx_pin_floating_on_timeout")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  const size_t TIMEOUT_MS = 917;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalRead);
  digitalRead_fake.return_val = LOW;
  RESET_FAKE(NoteGetMs);
  uint32_t NoteGetMs_results[] = {36, 274, 515, 801, TIMEOUT_MS + 36};
  SET_RETURN_SEQ(NoteGetMs, NoteGetMs_results, (sizeof(NoteGetMs_results) / sizeof(NoteGetMs_results[0])));
  RESET_FAKE(pinMode);

  // Action
  notetxn.start(TIMEOUT_MS);

  // Assert
  // Called at least once
  // Only returns `LOW`
  // Called at least twice
  CHECK((digitalReadCalls(CTX_PIN) && (0 == digitalRead_fake.return_val_seq_len) && NoteGetMs_fake.call_count == static_cast<unsigned int>(NoteGetMs_fake.return_val_seq_len) && pinModeCalls(CTX_PIN).size() > 1 && pinModeCalls(CTX_PIN).back() == INPUT));
}

TEST_CASE("test_notetxn_arduino_start_floats_rtx_pin_on_timeout")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;
  const size_t TIMEOUT_MS = 917;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalRead);
  digitalRead_fake.return_val = LOW;
  RESET_FAKE(NoteGetMs);
  uint32_t NoteGetMs_results[] = {36, 274, 515, 801, TIMEOUT_MS + 36};
  SET_RETURN_SEQ(NoteGetMs, NoteGetMs_results, (sizeof(NoteGetMs_results) / sizeof(NoteGetMs_results[0])));
  RESET_FAKE(pinMode);

  // Action
  notetxn.start(TIMEOUT_MS);

  // Assert
  // Called at least once
  // Only returns `LOW`
  CHECK((digitalReadCalls(CTX_PIN) && (0 == digitalRead_fake.return_val_seq_len) && NoteGetMs_fake.call_count == static_cast<unsigned int>(NoteGetMs_fake.return_val_seq_len) && pinModeCalls(RTX_PIN).back() == INPUT));
}

TEST_CASE("test_notetxn_arduino_stop_floats_rtx_pin")
{
  resetFakes();
  // Arrange
  const uint8_t CTX_PIN = 19;
  const uint8_t RTX_PIN = 79;

  NoteTxn_Arduino notetxn(CTX_PIN, RTX_PIN);
  RESET_FAKE(digitalWrite);
  RESET_FAKE(pinMode);

  // Action
  notetxn.stop();

  // Assert
  CHECK((pinModeCalls(RTX_PIN).size() && pinModeCalls(RTX_PIN).back() == INPUT));
}

