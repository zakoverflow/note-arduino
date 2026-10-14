/*!
 * @file NoteI2c_Arduino_test.cpp
 *
 * Catch2 unit tests for NoteI2c_Arduino, migrated from the hand-rolled
 * test/NoteI2c_Arduino.test.cpp harness. Each TEST_CASE keeps the original test
 * function name and assertion. Dependencies are FFF fakes.
 */

#include <catch2/catch_test_macros.hpp>
#include "fff.h"

#include "NoteI2c_Arduino.hpp"
#include <cstring>
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

} // namespace



TEST_CASE("test_make_note_i2c_instantiates_notei2c_object")
{
  resetFakes();
  // Arrange
  NoteI2c * notei2c = nullptr;

  // Action
  notei2c = make_note_i2c(Wire);

  // Assert
  CHECK(nullptr != notei2c);

  // Clean-up
  make_note_i2c(nullptr);
}

TEST_CASE("test_make_note_i2c_enforces_singleton_by_returning_same_notei2c_object_for_all_calls")
{
  resetFakes();
  // Arrange
  NoteI2c * const notei2c_1 = make_note_i2c(Wire);

  // Action
  NoteI2c * const notei2c_2 = make_note_i2c(Wire);

  // Assert
  CHECK(notei2c_1 == notei2c_2);

  // Clean-up
  make_note_i2c(nullptr);
}

//int test_make_note_i2c_returns_nullptr_when_nullptr_is_passed_as_parameter()
TEST_CASE("test_make_note_i2c_deletes_singleton_when_nullptr_is_passed_as_parameter")
{
  resetFakes();
  // Arrange
  NoteI2c * notei2c = make_note_i2c(Wire);
  REQUIRE(notei2c);

  // Action
  notei2c = make_note_i2c(nullptr);

  // Assert
  CHECK(nullptr == notei2c);
}

TEST_CASE("test_notei2c_arduino_constructor_invokes_twowire_parameter_begin_method")
{
  resetFakes();
  // Arrange
  RESET_FAKE(TwoWire_begin);

  // Action
  NoteI2c_Arduino notei2c(Wire);

  // Assert
  CHECK(TwoWire_begin_fake.call_count);
}

#if not defined(WIRE_HAS_END)

TEST_CASE("test_notei2c_arduino_deconstructor_does_not_invoke_twowire_end_method")
{
  resetFakes();
  // Arrange
  NoteI2c_Arduino * notei2c = new NoteI2c_Arduino(Wire);
  RESET_FAKE(TwoWire_end);

  // Action
  delete notei2c;

  // Assert
  CHECK(!TwoWire_end_fake.call_count);
}

#else // defined(WIRE_HAS_END)

TEST_CASE("test_notei2c_arduino_deconstructor_invokes_twowire_end_method")
{
  resetFakes();
  // Arrange
  NoteI2c_Arduino * notei2c = new NoteI2c_Arduino(Wire);
  RESET_FAKE(TwoWire_end);

  // Action
  delete notei2c;

  // Assert
  CHECK(TwoWire_end_fake.call_count);
}

#endif // not defined(WIRE_HAS_END)

TEST_CASE("test_notei2c_arduino_receive_requests_response_data_from_notecard")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {0, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &actual_response_size
  );

  // Assert
  CHECK((TwoWire_beginTransmission_fake.arg0_val == EXPECTED_ADDRESS && TwoWire_writeByte_fake.arg0_history[0] == 0 && TwoWire_writeByte_fake.arg0_history[1] == REQUEST_SIZE && TwoWire_endTransmission_fake.call_count));
}

TEST_CASE("test_notei2c_arduino_receive_will_retry_transmission_on_i2c_failure")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_RESULT = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {1,0, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_RESULT,
    response_buffer,
    REQUEST_SIZE,
    &actual_response_size
  );

  // Assert
  CHECK((TwoWire_beginTransmission_fake.call_count > 1 && TwoWire_writeByte_fake.call_count > 2 && TwoWire_endTransmission_fake.call_count > 1));
}

TEST_CASE("test_notei2c_arduino_receive_will_not_retry_transmission_on_i2c_success")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_RESULT = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {0, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_RESULT,
    response_buffer,
    REQUEST_SIZE,
    &actual_response_size
  );

  // Assert
  CHECK((TwoWire_beginTransmission_fake.call_count == 1 && TwoWire_writeByte_fake.call_count == 2 && TwoWire_endTransmission_fake.call_count == 1));
}

TEST_CASE("test_notei2c_arduino_receive_will_only_retry_i2c_transmission_thrice")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_RESULT = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {1,1,1,1,0, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_RESULT,
    response_buffer,
    REQUEST_SIZE,
    &actual_response_size
  );

  // Assert
  // One (1) attempt + three (3) retries = Four (4) total
  CHECK((TwoWire_beginTransmission_fake.call_count == 4 && TwoWire_writeByte_fake.call_count == 8 && TwoWire_endTransmission_fake.call_count == 4));
}

TEST_CASE("test_notei2c_arduino_receive_reads_response_data_from_notecard")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESPONSE = "Test Passed!";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((TwoWire_requestFrom_fake.arg0_val == EXPECTED_ADDRESS && TwoWire_requestFrom_fake.arg1_val == (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE) && !memcmp(response_buffer, EXPECTED_RESPONSE, REQUEST_SIZE)));
}

TEST_CASE("test_notei2c_arduino_receive_updates_available_parameter_with_remaining_bytes")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 8;
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {5, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK(bytes_remaining == 5);
}

TEST_CASE("test_notei2c_arduino_receive_does_not_request_or_read_i2c_when_trasmission_error_occurs")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {1,1,1,1, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((!TwoWire_requestFrom_fake.call_count && !TwoWire_read_fake.call_count));
}

TEST_CASE("test_notei2c_arduino_receive_will_not_attempt_to_read_when_i2c_port_request_from_returns_zero")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = 0;
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((TwoWire_requestFrom_fake.call_count && !TwoWire_read_fake.call_count));
}

TEST_CASE("test_notei2c_arduino_receive_will_not_attempt_to_read_remaining_bytes_when_first_call_to_two_wire_read_returns_available_value_greater_than_note_i2c_request_max_size_minus_note_i2c_request_header_size")
{
  resetFakes();
  // Arrange
  const uint8_t AVAILABLE_SIZE = ((NoteI2c_Arduino::REQUEST_MAX_SIZE - NoteI2c_Arduino::REQUEST_HEADER_SIZE) + 1);
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {AVAILABLE_SIZE, AVAILABLE_SIZE, AVAILABLE_SIZE, AVAILABLE_SIZE, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((TwoWire_requestFrom_fake.call_count && (4 >= TwoWire_read_fake.call_count)));
}

TEST_CASE("test_notei2c_arduino_receive_will_not_attempt_to_read_when_i2c_port_request_from_returns_value_other_than_buffer_size_plus_note_i2c_request_header_size")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = sizeof(response_buffer);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((TwoWire_requestFrom_fake.call_count && !TwoWire_read_fake.call_count));
}

TEST_CASE("test_notei2c_arduino_receive_will_not_read_full_response_when_i2c_over_serial_protocol_returns_value_other_than_buffer_size")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, sizeof(response_buffer), 0, sizeof(response_buffer), 0, sizeof(response_buffer), 0, sizeof(response_buffer), 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((TwoWire_requestFrom_fake.call_count && TwoWire_read_fake.call_count == 8));
}

TEST_CASE("test_notei2c_arduino_receive_returns_nullptr_on_success")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESPONSE = "Test Passed!";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((!memcmp(response_buffer, EXPECTED_RESPONSE, REQUEST_SIZE) && ACTUAL_RESULT == nullptr));
}

TEST_CASE("test_notei2c_arduino_receive_returns_error_message_on_i2c_transmission_failure_1")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESULT = "i2c|rx: data too long to fit in transmit buffer {io}{i2c}";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {1,1,1,1, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_receive_returns_error_message_on_i2c_transmission_failure_2")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESULT = "i2c|rx: received NACK on transmit of address {io}{i2c}";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {2,2,2,2, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_receive_returns_error_message_on_i2c_transmission_failure_3")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESULT = "i2c|rx: received NACK on transmit of data {io}{i2c}";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {3,3,3,3, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_receive_returns_error_message_on_i2c_transmission_failure_4")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESULT = "i2c|rx: unknown error on TwoWire::endTransmission() {io}{i2c}";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {4,4,4,4, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_receive_returns_error_message_on_i2c_transmission_failure_5")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESULT = "i2c|rx: timeout {io}{i2c}";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {5,5,5,5, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_receive_returns_error_message_on_unexpected_i2c_transmission_failure")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESULT = "i2c|rx: unknown error encountered during I2C transmission {io}{i2c}";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {6,6,6,6, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_receive_returns_error_message_on_i2c_protocol_failure_to_respond")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESULT = "i2c|rx: no response to read request {io}{i2c}";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_receive_returns_error_message_on_i2c_protocol_unexpected_raw_byte_count")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESULT = "i2c|rx: unexpected raw byte count {io}{i2c}";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = sizeof(response_buffer);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {0, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_receive_returns_error_message_on_serial_over_i2c_protocol_unexpected_available_byte_count")
{
  resetFakes();
  // Arrange
  const uint8_t AVAILABLE_SIZE = ((NoteI2c_Arduino::REQUEST_MAX_SIZE - NoteI2c_Arduino::REQUEST_HEADER_SIZE) + 1);
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESULT = "serial-over-i2c|rx: available byte count greater than max allowed {io}{i2c}";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {AVAILABLE_SIZE, AVAILABLE_SIZE, AVAILABLE_SIZE, AVAILABLE_SIZE, REQUEST_SIZE, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 's', 'e', 'd', '!', '\0', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_receive_returns_error_message_on_serial_over_i2c_protocol_unexpected_protocol_byte_count")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  const uint8_t REQUEST_SIZE = 13;
  const char * EXPECTED_RESULT = "serial-over-i2c|rx: unexpected protocol byte count {io}{i2c}";
  uint8_t response_buffer[32];
  uint32_t bytes_remaining;

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_endTransmission);
  RESET_FAKE(TwoWire_requestFrom);
  TwoWire_requestFrom_fake.return_val = (REQUEST_SIZE + NoteI2c::REQUEST_HEADER_SIZE);
  RESET_FAKE(TwoWire_read);
  // Trailing 0: the call returns 0 once the scripted results run out
  int TwoWire_read_results[] = {5, 8, 'T', 'e', 's', 't', ' ', 'P', 'a', 's', 0};
  SET_RETURN_SEQ(TwoWire_read, TwoWire_read_results, (sizeof(TwoWire_read_results) / sizeof(TwoWire_read_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.receive(
    EXPECTED_ADDRESS,
    response_buffer,
    REQUEST_SIZE,
    &bytes_remaining
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_reset_invokes_begin_method_on_constructor_twowire_parameter")
{
  resetFakes();
  // Arrange
  const uint16_t DEVICE_ADDRESS = 0x17;
  NoteI2c_Arduino notei2c(Wire);
  RESET_FAKE(TwoWire_begin);

  // Action
  notei2c.reset(DEVICE_ADDRESS);

  // Assert
  CHECK(TwoWire_begin_fake.call_count);
}

TEST_CASE("test_notei2c_arduino_reset_returns_true")
{
  resetFakes();
  // Arrange
  const uint16_t DEVICE_ADDRESS = 0x17;
  NoteI2c_Arduino notei2c(Wire);
  RESET_FAKE(TwoWire_begin);

  // Action
  const bool ACTUAL_RESULT = notei2c.reset(DEVICE_ADDRESS);

  // Assert
  CHECK(ACTUAL_RESULT);
}

#if not defined(WIRE_HAS_END)

TEST_CASE("test_notei2c_arduino_reset_does_not_invoke_end_method_on_constructor_twowire_parameter_ifndef_WIRE_HAS_END")
{
  resetFakes();
  // Arrange
  const uint16_t DEVICE_ADDRESS = 0x17;
  NoteI2c_Arduino notei2c(Wire);
  RESET_FAKE(TwoWire_begin);
  RESET_FAKE(TwoWire_end);

  // Action
  notei2c.reset(DEVICE_ADDRESS);

  // Assert
  CHECK(!TwoWire_end_fake.call_count);
}

#else // defined(WIRE_HAS_END)

TEST_CASE("test_notei2c_arduino_reset_invokes_end_method_on_constructor_twowire_parameter_ifdef_WIRE_HAS_END")
{
  resetFakes();
  // Arrange
  const uint16_t DEVICE_ADDRESS = 0x17;
  NoteI2c_Arduino notei2c(Wire);
  RESET_FAKE(TwoWire_begin);
  RESET_FAKE(TwoWire_end);

  // Action
  notei2c.reset(DEVICE_ADDRESS);

  // Assert
  CHECK(TwoWire_end_fake.call_count);
}

#endif // not defined(WIRE_HAS_END)

TEST_CASE("test_notei2c_arduino_transmit_translates_parameters_for_arduino_two_wire")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  uint8_t write_buffer[17] = {'H','e','l','l','o',',',' ','N','o','t','e','c','a','r','d','!','\0'};
  const uint8_t REQUEST_SIZE = sizeof(write_buffer);

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  TwoWire_writeByte_fake.return_val = 1;
  RESET_FAKE(TwoWire_writeBuffer);
  TwoWire_writeBuffer_fake.return_val = REQUEST_SIZE;
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {0, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  notei2c.transmit(
    EXPECTED_ADDRESS,
    write_buffer,
    REQUEST_SIZE
  );

  // Assert
  CHECK((TwoWire_beginTransmission_fake.arg0_val == EXPECTED_ADDRESS && TwoWire_writeByte_fake.arg0_history[0] == REQUEST_SIZE && !memcmp(TwoWire_writeBuffer_fake.arg0_val, write_buffer, REQUEST_SIZE) && TwoWire_endTransmission_fake.call_count));
}

TEST_CASE("test_notei2c_arduino_transmit_returns_nullptr_on_success")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  uint8_t write_buffer[17] = {'H','e','l','l','o',',',' ','N','o','t','e','c','a','r','d','!','\0'};
  const uint8_t REQUEST_SIZE = sizeof(write_buffer);

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  TwoWire_writeByte_fake.return_val = 1;
  RESET_FAKE(TwoWire_writeBuffer);
  TwoWire_writeBuffer_fake.return_val = REQUEST_SIZE;
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {0, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.transmit(
    EXPECTED_ADDRESS,
    write_buffer,
    REQUEST_SIZE
  );

  // Assert
  CHECK(ACTUAL_RESULT == nullptr);
}

TEST_CASE("test_notei2c_arduino_transmit_returns_error_message_on_i2c_transmission_failure1")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  uint8_t write_buffer[17] = {'H','e','l','l','o',',',' ','N','o','t','e','c','a','r','d','!','\0'};
  const uint8_t REQUEST_SIZE = sizeof(write_buffer);
  const char * EXPECTED_RESULT = "i2c|tx: data too long to fit in transmit buffer {io}{i2c}";

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_writeBuffer);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {1, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.transmit(
    EXPECTED_ADDRESS,
    write_buffer,
    REQUEST_SIZE
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_transmit_returns_error_message_on_i2c_transmission_failure2")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  uint8_t write_buffer[17] = {'H','e','l','l','o',',',' ','N','o','t','e','c','a','r','d','!','\0'};
  const uint8_t REQUEST_SIZE = sizeof(write_buffer);
  const char * EXPECTED_RESULT = "i2c|tx: received NACK on transmit of address {io}{i2c}";

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_writeBuffer);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {2, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.transmit(
    EXPECTED_ADDRESS,
    write_buffer,
    REQUEST_SIZE
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_transmit_returns_error_message_on_i2c_transmission_failure3")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  uint8_t write_buffer[17] = {'H','e','l','l','o',',',' ','N','o','t','e','c','a','r','d','!','\0'};
  const uint8_t REQUEST_SIZE = sizeof(write_buffer);
  const char * EXPECTED_RESULT = "i2c|tx: received NACK on transmit of data {io}{i2c}";

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_writeBuffer);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {3, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.transmit(
    EXPECTED_ADDRESS,
    write_buffer,
    REQUEST_SIZE
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_transmit_returns_error_message_on_i2c_transmission_failure4")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  uint8_t write_buffer[17] = {'H','e','l','l','o',',',' ','N','o','t','e','c','a','r','d','!','\0'};
  const uint8_t REQUEST_SIZE = sizeof(write_buffer);
  const char * EXPECTED_RESULT = "i2c|tx: unknown error on TwoWire::endTransmission() {io}{i2c}";

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_writeBuffer);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {4, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.transmit(
    EXPECTED_ADDRESS,
    write_buffer,
    REQUEST_SIZE
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_transmit_returns_error_message_on_i2c_transmission_failure5")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  uint8_t write_buffer[17] = {'H','e','l','l','o',',',' ','N','o','t','e','c','a','r','d','!','\0'};
  const uint8_t REQUEST_SIZE = sizeof(write_buffer);
  const char * EXPECTED_RESULT = "i2c|tx: timeout {io}{i2c}";

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_writeBuffer);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {5, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.transmit(
    EXPECTED_ADDRESS,
    write_buffer,
    REQUEST_SIZE
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

TEST_CASE("test_notei2c_arduino_transmit_returns_error_message_on_unexpected_i2c_transmission_failure")
{
  resetFakes();
  // Arrange
  const uint16_t EXPECTED_ADDRESS = 0x17;
  uint8_t write_buffer[17] = {'H','e','l','l','o',',',' ','N','o','t','e','c','a','r','d','!','\0'};
  const uint8_t REQUEST_SIZE = sizeof(write_buffer);
  const char * EXPECTED_RESULT = "i2c|tx: unknown error encountered during I2C transmission {io}{i2c}";

  RESET_FAKE(TwoWire_beginTransmission);
  RESET_FAKE(TwoWire_writeByte);
  RESET_FAKE(TwoWire_writeBuffer);
  RESET_FAKE(TwoWire_endTransmission);
  // Trailing 0: the call returns 0 once the scripted results run out
  uint8_t TwoWire_endTransmission_results[] = {6, 0};
  SET_RETURN_SEQ(TwoWire_endTransmission, TwoWire_endTransmission_results, (sizeof(TwoWire_endTransmission_results) / sizeof(TwoWire_endTransmission_results[0])));
  NoteI2c_Arduino notei2c(Wire);

  // Action
  const char * const ACTUAL_RESULT = notei2c.transmit(
    EXPECTED_ADDRESS,
    write_buffer,
    REQUEST_SIZE
  );

  // Assert
  CHECK((ACTUAL_RESULT && !memcmp(ACTUAL_RESULT, EXPECTED_RESULT, strlen(EXPECTED_RESULT))));
}

