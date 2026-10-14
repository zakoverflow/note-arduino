/*!
 * @file Notecard_test.cpp
 *
 * Catch2 unit tests for Notecard, migrated from the hand-rolled
 * test/Notecard.test.cpp harness. Each TEST_CASE keeps the original test
 * function name and assertion. Dependencies are FFF fakes.
 */

#include <catch2/catch_test_macros.hpp>
#include "fff.h"

#include "Notecard.h"
#include <cstring>
#include <cstdlib>
#include <string>
#include "Arduino.h"
#include "note_arduino_fakes.hpp"

DEFINE_FFF_GLOBALS

namespace
{

void resetFakes(void)
{
    ARDUINO_FAKES_LIST(RESET_FAKE)
    NOTE_C_FAKES_LIST(RESET_FAKE)
    NOTE_ARDUINO_FAKES_LIST(RESET_FAKE)
    FFF_RESET_HISTORY();

    // Like a real, open Arduino serial port, `Serial` evaluates as true.
    HardwareSerial_operator_bool_fake.return_val = true;
}

// note-c reports the active I2C/Serial hooks through out-parameters, which
// FFF cannot fill in by itself. These custom fakes report a non-null
// transmit hook, i.e. "the interface has been instantiated".
void NoteGetFnI2C_reportsTransmitFn(uint32_t *, uint32_t *, i2cResetFn *, i2cTransmitFn * transmitFn, i2cReceiveFn *)
{
    if (transmitFn) {
        *transmitFn = reinterpret_cast<i2cTransmitFn>(0x1);
    }
}

void NoteGetFnSerial_reportsTransmitFn(serialResetFn *, serialTransmitFn * transmitFn, serialAvailableFn *, serialReceiveFn *)
{
    if (transmitFn) {
        *transmitFn = reinterpret_cast<serialTransmitFn>(0x1);
    }
}

// logDebugf() formats into a temporary buffer, so the message has to be
// copied while NoteDebug() is running to be checked afterwards.
std::string NoteDebug_message_cache;
void NoteDebug_cacheMessage(const char * message)
{
    NoteDebug_message_cache = (message ? message : "");
}

} // namespace




TEST_CASE("test_notecard_begin_i2c_sets_user_agent_to_note_arduino")
{
  resetFakes();
   // Arrange
  ////////////

  static const char * const EXPECTED_USER_AGENT = "note-arduino " NOTE_ARDUINO_VERSION;
  Notecard notecard;
  NoteI2c_Fake mockI2c;

  RESET_FAKE(NoteSetUserAgent);

   // Action
  ///////////

  notecard.begin(&mockI2c);

   // Assert
  ///////////

  CHECK((NoteSetUserAgent_fake.arg0_val && !strcmp(EXPECTED_USER_AGENT, NoteSetUserAgent_fake.arg0_val)));
}

TEST_CASE("test_notecard_begin_i2c_sets_user_agent_to_note_arduino_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  static const char * const EXPECTED_USER_AGENT = "note-arduino " NOTE_ARDUINO_VERSION;
  Notecard notecard;
  RESET_FAKE(NoteSetUserAgent);

   // Action
  ///////////

  notecard.begin(static_cast<NoteI2c *>(nullptr));

   // Assert
  ///////////

  CHECK((NoteSetUserAgent_fake.arg0_val && !strcmp(EXPECTED_USER_AGENT, NoteSetUserAgent_fake.arg0_val)));
}

TEST_CASE("test_notecard_begin_i2c_shares_a_memory_allocation_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(&mockI2c);

   // Assert
  ///////////

  CHECK(NoteSetFnDefault_fake.arg0_val);
}

TEST_CASE("test_notecard_begin_i2c_sets_memory_allocation_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(static_cast<NoteI2c *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnDefault_fake.arg0_val);
}

TEST_CASE("test_notecard_begin_i2c_shares_a_memory_free_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(&mockI2c);

   // Assert
  ///////////

  CHECK(NoteSetFnDefault_fake.arg1_val);
}

TEST_CASE("test_notecard_begin_i2c_sets_memory_free_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(static_cast<NoteI2c *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnDefault_fake.arg1_val);
}

TEST_CASE("test_notecard_begin_i2c_shares_a_delay_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(&mockI2c);

   // Arrange
  ////////////

  CHECK(NoteSetFnDefault_fake.arg2_val);
}

TEST_CASE("test_notecard_begin_i2c_sets_delay_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(static_cast<NoteI2c *>(nullptr));

   // Arrange
  ////////////

  CHECK(!NoteSetFnDefault_fake.arg2_val);
}

TEST_CASE("test_notecard_begin_i2c_shares_a_millis_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(&mockI2c);

   // Assert
  ///////////

  CHECK(NoteSetFnDefault_fake.arg3_val);
}

TEST_CASE("test_notecard_begin_i2c_sets_millis_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(static_cast<NoteI2c *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnDefault_fake.arg3_val);
}

TEST_CASE("test_notecard_begin_i2c_does_not_modify_i2c_address_parameter_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  // 0x1B binary representation => 0001 1011
  const uint16_t EXPECTED_ADDRESS = 0x1B;

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnI2CDefault);

   // Action
  ///////////

  notecard.begin(&mockI2c, EXPECTED_ADDRESS);

   // Assert
  ///////////

  CHECK(EXPECTED_ADDRESS == NoteSetFnI2CDefault_fake.arg0_val);
}

TEST_CASE("test_notecard_begin_i2c_sets_i2c_address_parameter_to_zero_before_passing_to_note_c_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  // 0x1B binary representation => 0001 1011
  const uint16_t SUPPLIED_ADDRESS = 0x1B;

  Notecard notecard;
  RESET_FAKE(NoteSetFnI2C);
  NoteSetFnI2C_fake.arg0_val = NOTE_I2C_ADDR_DEFAULT;

   // Action
  ///////////

  notecard.begin(static_cast<NoteI2c *>(nullptr), SUPPLIED_ADDRESS);

   // Assert
  ///////////

  CHECK(!NoteSetFnI2C_fake.arg0_val);
}

TEST_CASE("test_notecard_begin_i2c_does_not_modify_i2c_mtu_parameter_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  const uint32_t EXPECTED_RESULT = 79;

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnI2CDefault);

   // Action
  ///////////

  notecard.begin(&mockI2c, 0x1B, EXPECTED_RESULT);

   // Assert
  ///////////

  CHECK(EXPECTED_RESULT == NoteSetFnI2CDefault_fake.arg1_val);
}

TEST_CASE("test_notecard_begin_i2c_sets_i2c_mtu_parameter_to_zero_before_passing_to_note_c_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const uint32_t SUPPLIED_VALUE = 79;

  Notecard notecard;
  RESET_FAKE(NoteSetFnI2C);
  NoteSetFnI2C_fake.arg1_val = NOTE_I2C_MAX_DEFAULT;

   // Action
  ///////////

  notecard.begin(static_cast<NoteI2c *>(nullptr), 0x1B, SUPPLIED_VALUE);

   // Assert
  ///////////

  CHECK(!NoteSetFnI2C_fake.arg1_val);
}

TEST_CASE("test_notecard_begin_i2c_shares_an_i2c_reset_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnI2CDefault);

   // Action
  ///////////

  notecard.begin(&mockI2c);

   // Assert
  ///////////

  CHECK(NoteSetFnI2CDefault_fake.arg2_val);
}

TEST_CASE("test_notecard_begin_i2c_sets_i2c_reset_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnI2C);
  NoteSetFnI2C_fake.arg2_val = reinterpret_cast<i2cResetFn>(0x19790917);

   // Action
  ///////////

  notecard.begin(static_cast<NoteI2c *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnI2C_fake.arg2_val);
}

TEST_CASE("test_notecard_begin_i2c_shares_an_i2c_transmit_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnI2CDefault);

   // Action
  ///////////

  notecard.begin(&mockI2c);

   // Assert
  ///////////

  CHECK(NoteSetFnI2CDefault_fake.arg3_val);
}

TEST_CASE("test_notecard_begin_i2c_sets_i2c_transmit_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnI2C);
  NoteSetFnI2C_fake.arg3_val = reinterpret_cast<i2cTransmitFn>(0x19790917);

   // Action
  ///////////

  notecard.begin(static_cast<NoteI2c *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnI2C_fake.arg3_val);
}

TEST_CASE("test_notecard_begin_i2c_shares_an_i2c_receive_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnI2CDefault);

   // Action
  ///////////

  notecard.begin(&mockI2c);

   // Assert
  ///////////

  CHECK(NoteSetFnI2CDefault_fake.arg4_val);
}

TEST_CASE("test_notecard_begin_i2c_sets_i2c_receive_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnI2C);
  NoteSetFnI2C_fake.arg4_val = reinterpret_cast<i2cReceiveFn>(0x19790917);

   // Action
  ///////////

  notecard.begin(static_cast<NoteI2c *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnI2C_fake.arg4_val);
}

TEST_CASE("test_notecard_begin_i2c_default_parameter_for_i2cMax_is_passed_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnI2CDefault);
  NoteSetFnI2CDefault_fake.arg1_val = (NOTE_I2C_MAX_DEFAULT - 1);

   // Action
  ///////////

  notecard.begin(&mockI2c, 0x1B);

   // Assert
  ///////////

  CHECK(NOTE_I2C_MAX_DEFAULT == NoteSetFnI2CDefault_fake.arg1_val);
}

TEST_CASE("test_notecard_begin_i2c_default_parameter_for_i2cAddress_is_passed_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnI2CDefault);
  NoteSetFnI2CDefault_fake.arg0_val = (NOTE_I2C_ADDR_DEFAULT - 1);

   // Action
  ///////////

  notecard.begin(&mockI2c);

   // Assert
  ///////////

  CHECK(NOTE_I2C_ADDR_DEFAULT == NoteSetFnI2CDefault_fake.arg0_val);
}

TEST_CASE("test_notecard_begin_serial_sets_user_agent_to_note_arduino")
{
  resetFakes();
   // Arrange
  ////////////

  static const char * const EXPECTED_USER_AGENT = "note-arduino " NOTE_ARDUINO_VERSION;
  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(NoteSetUserAgent);

   // Action
  ///////////

  notecard.begin(&mockSerial);

   // Assert
  ///////////

  CHECK((NoteSetUserAgent_fake.arg0_val && !strcmp(EXPECTED_USER_AGENT, NoteSetUserAgent_fake.arg0_val)));
}

TEST_CASE("test_notecard_begin_serial_sets_user_agent_to_note_arduino_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  static const char * const EXPECTED_USER_AGENT = "note-arduino " NOTE_ARDUINO_VERSION;
  Notecard notecard;
  RESET_FAKE(NoteSetUserAgent);

   // Action
  ///////////

  notecard.begin(static_cast<NoteSerial *>(nullptr));

   // Assert
  ///////////

  CHECK((NoteSetUserAgent_fake.arg0_val && !strcmp(EXPECTED_USER_AGENT, NoteSetUserAgent_fake.arg0_val)));
}

TEST_CASE("test_notecard_begin_serial_shares_a_memory_allocation_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(&mockSerial);

   // Assert
  ///////////

  CHECK(NoteSetFnDefault_fake.arg0_val);
}

TEST_CASE("test_notecard_begin_serial_sets_memory_allocation_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(static_cast<NoteSerial *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnDefault_fake.arg0_val);
}

TEST_CASE("test_notecard_begin_serial_shares_a_memory_free_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(&mockSerial);

   // Assert
  ///////////

  CHECK(NoteSetFnDefault_fake.arg1_val);
}

TEST_CASE("test_notecard_begin_serial_sets_memory_free_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(static_cast<NoteSerial *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnDefault_fake.arg1_val);
}

TEST_CASE("test_notecard_begin_serial_shares_a_delay_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(&mockSerial);

   // Assert
  ///////////

  CHECK(NoteSetFnDefault_fake.arg2_val);
}

TEST_CASE("test_notecard_begin_serial_sets_delay_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(static_cast<NoteSerial *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnDefault_fake.arg2_val);
}

TEST_CASE("test_notecard_begin_serial_shares_a_millis_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(&mockSerial);

   // Assert
  ///////////

  CHECK(NoteSetFnDefault_fake.arg3_val);
}

TEST_CASE("test_notecard_begin_serial_sets_millis_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnDefault);

   // Action
  ///////////

  notecard.begin(static_cast<NoteSerial *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnDefault_fake.arg3_val);
}

TEST_CASE("test_notecard_begin_serial_shares_a_serial_reset_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(NoteSetFnSerialDefault);

   // Action
  ///////////

  notecard.begin(&mockSerial);

   // Assert
  ///////////

  CHECK(NoteSetFnSerialDefault_fake.arg0_val);
}

TEST_CASE("test_notecard_begin_serial_sets_serial_reset_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnSerial);
  NoteSetFnSerial_fake.arg0_val = reinterpret_cast<serialResetFn>(0x19790917);

   // Action
  ///////////

  notecard.begin(static_cast<NoteSerial *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnSerial_fake.arg0_val);
}

TEST_CASE("test_notecard_begin_serial_shares_a_serial_transmit_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(NoteSetFnSerialDefault);

   // Action
  ///////////

  notecard.begin(&mockSerial);

   // Assert
  ///////////

  CHECK(NoteSetFnSerialDefault_fake.arg1_val);
}

TEST_CASE("test_notecard_begin_serial_sets_transmit_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnSerial);
  NoteSetFnSerial_fake.arg1_val = reinterpret_cast<serialTransmitFn>(0x19790917);

   // Action
  ///////////

  notecard.begin(static_cast<NoteSerial *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnSerial_fake.arg1_val);
}

TEST_CASE("test_notecard_begin_serial_shares_a_serial_available_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(NoteSetFnSerialDefault);

   // Action
  ///////////

  notecard.begin(&mockSerial);

   // Assert
  ///////////

  CHECK(NoteSetFnSerialDefault_fake.arg2_val);
}

TEST_CASE("test_notecard_begin_serial_sets_serial_available_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnSerial);
  NoteSetFnSerial_fake.arg2_val = reinterpret_cast<serialAvailableFn>(0x19790917);

   // Action
  ///////////

  notecard.begin(static_cast<NoteSerial *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnSerial_fake.arg2_val);
}

TEST_CASE("test_notecard_begin_serial_shares_a_serial_receive_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(NoteSetFnSerialDefault);

   // Action
  ///////////

  notecard.begin(&mockSerial);

   // Assert
  ///////////

  CHECK(NoteSetFnSerialDefault_fake.arg3_val);
}

TEST_CASE("test_notecard_begin_serial_sets_serial_receive_function_pointer_to_nullptr_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnSerial);
  NoteSetFnSerial_fake.arg3_val = reinterpret_cast<serialReceiveFn>(0x19790917);

   // Action
  ///////////

  notecard.begin(static_cast<NoteSerial *>(nullptr));

   // Assert
  ///////////

  CHECK(!NoteSetFnSerial_fake.arg3_val);
}

TEST_CASE("test_notecard_end_does_not_call_wire_end_when_the_i2c_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;
  RESET_FAKE(NoteSetFnSerial);
  RESET_FAKE(TwoWire_end);
  notecard.begin(&mockSerial);
  RESET_FAKE(NoteGetFnI2C);
  RESET_FAKE(NoteGetFnSerial);
  RESET_FAKE(make_note_i2c_nullptr);

   // Action
  ///////////

  notecard.end();

   // Assert
  ///////////

  CHECK(!make_note_i2c_nullptr_fake.call_count);
}

TEST_CASE("test_notecard_end_clears_all_i2c_interface_function_pointers_when_the_i2c_interface_has_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFnI2C);
  notecard.begin(&mockI2c);
  RESET_FAKE(NoteGetFnSerial);
  RESET_FAKE(NoteGetFnI2C);
  NoteGetFnI2C_fake.custom_fake = NoteGetFnI2C_reportsTransmitFn;

   // Action
  ///////////

  notecard.end();

   // Assert
  ///////////

  CHECK((!NoteSetFnI2C_fake.arg2_val && !NoteSetFnI2C_fake.arg3_val && !NoteSetFnI2C_fake.arg4_val));
}

TEST_CASE("test_notecard_end_does_not_call_serial_end_when_the_serial_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockI2c;
  RESET_FAKE(NoteSetFnDefault);
  notecard.begin(&mockI2c);
  RESET_FAKE(HardwareSerial_end);
  RESET_FAKE(NoteGetFnI2C);
  RESET_FAKE(NoteGetFnSerial);
  RESET_FAKE(make_note_serial_nullptr);

   // Action
  ///////////

  notecard.end();

   // Assert
  ///////////

  CHECK(!make_note_serial_nullptr_fake.call_count);
}

TEST_CASE("test_notecard_end_clears_all_serial_interface_function_pointers_when_the_serial_interface_has_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(NoteSetFnDefault);
  notecard.begin(&mockSerial);
  RESET_FAKE(NoteGetFnI2C);
  RESET_FAKE(NoteGetFnSerial);
  NoteGetFnSerial_fake.custom_fake = NoteGetFnSerial_reportsTransmitFn;

   // Action
  ///////////

  notecard.end();

   // Assert
  ///////////

  CHECK((!NoteSetFnSerial_fake.arg0_val && !NoteSetFnSerial_fake.arg1_val && !NoteSetFnSerial_fake.arg2_val && !NoteSetFnSerial_fake.arg3_val));
}

TEST_CASE("test_notecard_end_clears_all_platform_interface_function_pointers")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  RESET_FAKE(NoteSetFn);
  notecard.begin(&mockI2c);

   // Action
  ///////////

  notecard.end();

   // Assert
  ///////////

  CHECK((!NoteSetFn_fake.arg0_val && !NoteSetFn_fake.arg1_val && !NoteSetFn_fake.arg2_val && !NoteSetFn_fake.arg3_val));
}

TEST_CASE("test_notecard_end_provides_nullptr_to_make_note_i2c_to_free_associated_memory_when_the_i2c_interface_has_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  RESET_FAKE(make_note_i2c_nullptr);
  notecard.begin(&mockI2c);
  RESET_FAKE(NoteGetFnSerial);
  RESET_FAKE(NoteGetFnI2C);
  NoteGetFnI2C_fake.custom_fake = NoteGetFnI2C_reportsTransmitFn;

   // Action
  ////////////

  notecard.end();

   // Assert
  ////////////

  // The fake only stands in for make_note_i2c(nullptr), so a call means
  // nullptr was provided.
  CHECK(0 != make_note_i2c_nullptr_fake.call_count);
}

TEST_CASE("test_notecard_end_invokes_make_note_serial_nullptr_method_to_free_associated_memory_when_the_serial_interface_has_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  RESET_FAKE(make_note_serial_nullptr);
  notecard.begin(&mockSerial);
  RESET_FAKE(NoteGetFnI2C);
  RESET_FAKE(NoteGetFnSerial);
  NoteGetFnSerial_fake.custom_fake = NoteGetFnSerial_reportsTransmitFn;

   // Action
  ////////////

  notecard.end();

   // Assert
  ////////////

  // The fake only stands in for make_note_serial(nullptr), so a call means
  // nullptr was provided.
  CHECK(0 != make_note_serial_nullptr_fake.call_count);
}

TEST_CASE("test_notecard_setDebugOutputStream_shares_a_debug_log_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteLog_Fake mockLog;
  RESET_FAKE(NoteSetFnDebugOutput);

   // Action
  ///////////

  notecard.setDebugOutputStream(&mockLog);

   // Assert
  ///////////

  CHECK(NoteSetFnDebugOutput_fake.arg0_val);
}

TEST_CASE("test_notecard_setDebugOutputStream_clears_the_debug_log_function_pointer_when_nullptr_is_provided")
{
  resetFakes();
  int sentinel = 0;

   // Arrange
  ////////////

  Notecard notecard;
  RESET_FAKE(NoteSetFnDebugOutput);
  NoteSetFnDebugOutput_fake.arg0_val = reinterpret_cast<debugOutputFn>(&sentinel);

   // Action
  ///////////

  notecard.setDebugOutputStream(nullptr);

   // Assert
  ///////////

  CHECK(!NoteSetFnDebugOutput_fake.arg0_val);
}

TEST_CASE("test_notecard_clearDebugOutputStream_clears_the_debug_log_function_pointer")
{
  resetFakes();
  int sentinel = 0;

   // Arrange
  ////////////

  Notecard notecard;

  RESET_FAKE(NoteSetFnDebugOutput);
  NoteSetFnDebugOutput_fake.arg0_val = reinterpret_cast<debugOutputFn>(&sentinel);

   // Action
  ///////////

  notecard.clearDebugOutputStream();

   // Assert
  ///////////

  CHECK(!NoteSetFnDebugOutput_fake.arg0_val);
}

TEST_CASE("test_notecard_setFn_shares_a_memory_allocation_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  const mallocFn mockMallocFn = reinterpret_cast<mallocFn>(0x19790917);

  Notecard notecard;
  RESET_FAKE(NoteSetFn);

   // Action
  ///////////

  notecard.setFn(mockMallocFn, nullptr, nullptr, nullptr);

   // Assert
  ///////////

  CHECK(NoteSetFn_fake.arg0_val);
}

TEST_CASE("test_notecard_setFn_shares_a_memory_deallocation_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  const freeFn mockFreeFn = reinterpret_cast<freeFn>(0x19790917);

  Notecard notecard;
  RESET_FAKE(NoteSetFn);

   // Action
  ///////////

  notecard.setFn(nullptr, mockFreeFn, nullptr, nullptr);

   // Assert
  ///////////

  CHECK(NoteSetFn_fake.arg1_val);
}

TEST_CASE("test_notecard_setFn_shares_a_delay_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  const delayMsFn mockDelayFn = reinterpret_cast<delayMsFn>(0x19790917);

  Notecard notecard;
  RESET_FAKE(NoteSetFn);

   // Action
  ///////////

  notecard.setFn(nullptr, nullptr, mockDelayFn, nullptr);

   // Assert
  ///////////

  CHECK(NoteSetFn_fake.arg2_val);
}

TEST_CASE("test_notecard_setFn_shares_a_millis_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  const getMsFn mockMillisFn = reinterpret_cast<getMsFn>(0x19790917);

  Notecard notecard;
  RESET_FAKE(NoteSetFn);

   // Action
  ///////////

  notecard.setFn(nullptr, nullptr, nullptr, mockMillisFn);

   // Assert
  ///////////

  CHECK(NoteSetFn_fake.arg3_val);
}

TEST_CASE("test_notecard_setFnI2cMutex_does_not_modify_locking_mutex_func_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////
  const mutexFn lockFn = reinterpret_cast<mutexFn>(0x19790917);
  Notecard notecard;
  RESET_FAKE(NoteSetFnI2CMutex);

   // Action
  ///////////

  notecard.setFnI2cMutex(lockFn,nullptr);

   // Assert
  ///////////

  CHECK(lockFn == NoteSetFnI2CMutex_fake.arg0_val);
}

TEST_CASE("test_notecard_setFnI2cMutex_does_not_modify_unlocking_mutex_func_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////
  const mutexFn unlockFn = reinterpret_cast<mutexFn>(0x19790917);
  Notecard notecard;
  RESET_FAKE(NoteSetFnI2CMutex);

   // Action
  ///////////

  notecard.setFnI2cMutex(nullptr,unlockFn);

   // Assert
  ///////////

  CHECK(unlockFn == NoteSetFnI2CMutex_fake.arg1_val);
}

TEST_CASE("test_notecard_setFnNoteMutex_does_not_modify_locking_mutex_func_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////
  const mutexFn lockFn = reinterpret_cast<mutexFn>(0x19790917);
  Notecard notecard;
  RESET_FAKE(NoteSetFnNoteMutex);

   // Action
  ///////////

  notecard.setFnNoteMutex(lockFn,nullptr);

   // Assert
  ///////////

  CHECK(lockFn == NoteSetFnNoteMutex_fake.arg0_val);
}

TEST_CASE("test_notecard_setFnNoteMutex_does_not_modify_unlocking_mutex_func_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////
  const mutexFn unlockFn = reinterpret_cast<mutexFn>(0x19790917);
  Notecard notecard;
  RESET_FAKE(NoteSetFnNoteMutex);

   // Action
  ///////////

  notecard.setFnNoteMutex(nullptr,unlockFn);

   // Assert
  ///////////

  CHECK(unlockFn == NoteSetFnNoteMutex_fake.arg1_val);
}

TEST_CASE("test_notecard_setTransactionPins_shares_a_transaction_start_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteTxn_Fake mockTxn;
  RESET_FAKE(NoteSetFnTransaction);

   // Action
  ///////////

  notecard.setTransactionPins(&mockTxn);

   // Assert
  ///////////

  CHECK(NoteSetFnTransaction_fake.arg0_val);
}

TEST_CASE("test_notecard_setTransactionPins_shares_a_transaction_stop_function_pointer")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteTxn_Fake mockTxn;
  RESET_FAKE(NoteSetFnTransaction);

   // Action
  ///////////

  notecard.setTransactionPins(&mockTxn);

   // Assert
  ///////////

  CHECK(NoteSetFnTransaction_fake.arg1_val);
}

TEST_CASE("test_notecard_setTransactionPins_clears_the_transaction_start_function_pointer_when_nullptr_is_provided")
{
  resetFakes();
  int sentinel = 0;

   // Arrange
  ////////////

  Notecard notecard;
  NoteTxn_Fake mockTxn;
  RESET_FAKE(NoteSetFnTransaction);
  NoteSetFnTransaction_fake.arg0_val = reinterpret_cast<txnStartFn>(&sentinel);

   // Action
  ///////////

  notecard.setTransactionPins(nullptr);

   // Assert
  ///////////

  CHECK(!NoteSetFnTransaction_fake.arg0_val);
}

TEST_CASE("test_notecard_setTransactionPins_clears_the_transaction_stop_function_pointer_when_nullptr_is_provided")
{
  resetFakes();
  int sentinel = 0;

   // Arrange
  ////////////

  Notecard notecard;
  NoteTxn_Fake mockTxn;
  RESET_FAKE(NoteSetFnTransaction);
  NoteSetFnTransaction_fake.arg1_val = reinterpret_cast<txnStopFn>(&sentinel);

   // Action
  ///////////

  notecard.setTransactionPins(nullptr);

   // Assert
  ///////////

  CHECK(!NoteSetFnTransaction_fake.arg1_val);
}

TEST_CASE("test_notecard_newRequest_does_not_modify_string_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  const char * const EXPECTED_RESULT = "Hello, Test!";

  RESET_FAKE(NoteNewRequest);

   // Action
  ///////////

  notecard.newRequest(EXPECTED_RESULT);

   // Assert
  ///////////

  CHECK(!strcmp(EXPECTED_RESULT, NoteNewRequest_fake.arg0_val));
}

TEST_CASE("test_notecard_newRequest_does_not_modify_note_c_result_value_before_returning_to_caller")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  J * const EXPECTED_JSON = reinterpret_cast<J *>(malloc(sizeof(J))); // JCreateObject();
  REQUIRE(nullptr != EXPECTED_JSON);

  RESET_FAKE(NoteNewRequest);
  memset(EXPECTED_JSON, 0x55, sizeof(J)); //REQUIRE(nullptr != JAddStringToObject(EXPECTED_JSON,"key1", "value1"));
  {
    NoteNewRequest_fake.return_val = reinterpret_cast<J *>(malloc(sizeof(J)));
    REQUIRE(nullptr != NoteNewRequest_fake.return_val);
    memcpy(NoteNewRequest_fake.return_val, EXPECTED_JSON, sizeof(J));
  } //REQUIRE(nullptr != (NoteNewRequest_fake.return_val = JDuplicate(EXPECTED_JSON,true)));

   // Action
  ///////////

  const J * const ACTUAL_RESULT = notecard.newRequest(nullptr);

   // Assert
  ///////////

  CHECK(!memcmp(ACTUAL_RESULT, EXPECTED_JSON, sizeof(J)) /*JCompare(EXPECTED_JSON, json_result, false)*/);

  free(NoteNewRequest_fake.return_val);
  free(EXPECTED_JSON);
}

TEST_CASE("test_notecard_sendRequest_does_not_modify_j_object_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  J * EXPECTED_JSON = reinterpret_cast<J *>(malloc(sizeof(J))); // JCreateObject();
  J * json_cpy;
  REQUIRE(nullptr != EXPECTED_JSON);

  RESET_FAKE(NoteRequest);
  memset(EXPECTED_JSON, 0x55, sizeof(J)); //REQUIRE(nullptr != JAddStringToObject(EXPECTED_JSON,"key1", "value1"));
  {
    json_cpy = reinterpret_cast<J *>(malloc(sizeof(J)));
    REQUIRE(nullptr != json_cpy);
    memcpy(json_cpy, EXPECTED_JSON, sizeof(J));
  } //REQUIRE(nullptr != (json_cpy = JDuplicate(EXPECTED_JSON,true)));

   // Action
  ///////////

  notecard.sendRequest(json_cpy);

   // Assert
  ///////////

  CHECK(!memcmp(EXPECTED_JSON, NoteRequest_fake.arg0_val, sizeof(J)) /*JCompare(EXPECTED_JSON, noteSendRequest_Parameters.req, false)*/);

  free(json_cpy);
  free(EXPECTED_JSON);
}

TEST_CASE("test_notecard_sendRequest_does_not_modify_note_c_result_value_before_returning_to_caller")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  const bool EXPECTED_RESULT = true;

  RESET_FAKE(NoteRequest);
  NoteRequest_fake.return_val = EXPECTED_RESULT;

   // Action
  ///////////

  const bool ACTUAL_RESULT = notecard.sendRequest(nullptr);

   // Assert
  ///////////

  CHECK(EXPECTED_RESULT == ACTUAL_RESULT);
}

TEST_CASE("test_notecard_sendRequestWithRetry_does_not_modify_j_object_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  J * EXPECTED_JSON = reinterpret_cast<J *>(malloc(sizeof(J)));
  J * json_cpy;
  REQUIRE(nullptr != EXPECTED_JSON);

  RESET_FAKE(NoteRequestWithRetry);
  memset(EXPECTED_JSON, 0x55, sizeof(J));
  {
    json_cpy = reinterpret_cast<J *>(malloc(sizeof(J)));
    REQUIRE(nullptr != json_cpy);
    memcpy(json_cpy, EXPECTED_JSON, sizeof(J));
  }

   // Action
  ///////////

  notecard.sendRequestWithRetry(json_cpy, 0);

   // Assert
  ///////////

  CHECK(!memcmp(EXPECTED_JSON, NoteRequestWithRetry_fake.arg0_val, sizeof(J)));

  free(json_cpy);
  free(EXPECTED_JSON);
}

TEST_CASE("test_notecard_sendRequestWithRetry_does_not_modify_timeout_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  uint32_t EXPECTED_TIMEOUT = 10;
  RESET_FAKE(NoteRequestWithRetry);

   // Action
  ///////////

  notecard.sendRequestWithRetry(nullptr, EXPECTED_TIMEOUT);

   // Assert
  ///////////

  CHECK(EXPECTED_TIMEOUT == NoteRequestWithRetry_fake.arg1_val);
}

TEST_CASE("test_notecard_sendRequestWithRetry_does_not_modify_note_c_result_value_before_returning_to_caller")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  const bool EXPECTED_RESULT = true;

  RESET_FAKE(NoteRequestWithRetry);
  NoteRequestWithRetry_fake.return_val = EXPECTED_RESULT;

   // Action
  ///////////

  const bool ACTUAL_RESULT = notecard.sendRequestWithRetry(nullptr, 0);

   // Assert
  ///////////

  CHECK(EXPECTED_RESULT == ACTUAL_RESULT);
}

TEST_CASE("test_notecard_requestAndResponse_does_not_modify_j_object_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  J *EXPECTED_JSON = reinterpret_cast<J *>(malloc(sizeof(J))); // JCreateObject();
  J *json_cpy;
  REQUIRE(nullptr != EXPECTED_JSON);

  RESET_FAKE(NoteRequestResponse);
  memset(EXPECTED_JSON, 0x55, sizeof(J)); //REQUIRE(nullptr != JAddStringToObject(EXPECTED_JSON,"key1", "value1"));
  {
    json_cpy = reinterpret_cast<J *>(malloc(sizeof(J)));
    REQUIRE(nullptr != json_cpy);
    memcpy(json_cpy, EXPECTED_JSON, sizeof(J));
  } //REQUIRE(nullptr != (json_cpy = JDuplicate(EXPECTED_JSON,true)));

   // Action
  ///////////

  notecard.requestAndResponse(json_cpy);

   // Assert
  ///////////

  CHECK(!memcmp(EXPECTED_JSON, NoteRequestResponse_fake.arg0_val, sizeof(J)) /*JCompare(EXPECTED_JSON, NoteRequestResponse_fake.arg0_val, false)*/);

  free(json_cpy);
  free(EXPECTED_JSON);
}

TEST_CASE("test_notecard_requestAndResponse_does_not_modify_note_c_result_value_before_returning_to_caller")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  J * EXPECTED_JSON = reinterpret_cast<J *>(malloc(sizeof(J))); // JCreateObject();
  REQUIRE(nullptr != EXPECTED_JSON);

  RESET_FAKE(NoteRequestResponse);
  memset(EXPECTED_JSON, 0x55, sizeof(J)); //REQUIRE(nullptr != JAddStringToObject(EXPECTED_JSON,"key1", "value1"));
  {
    NoteRequestResponse_fake.return_val = reinterpret_cast<J *>(malloc(sizeof(J)));
    REQUIRE(nullptr != NoteRequestResponse_fake.return_val);
    memcpy(NoteRequestResponse_fake.return_val, EXPECTED_JSON, sizeof(J));
  } //REQUIRE(nullptr != (NoteRequestResponse_fake.return_val = JDuplicate(EXPECTED_JSON,true)));

   // Action
  ///////////

  const J * const ACTUAL_RESULT = notecard.requestAndResponse(nullptr);

   // Assert
  ///////////

  CHECK(!memcmp(EXPECTED_JSON, ACTUAL_RESULT, sizeof(J)) /*JCompare(EXPECTED_JSON, json_result, false)*/);

  free(NoteRequestResponse_fake.return_val);
  free(EXPECTED_JSON);
}

TEST_CASE("test_notecard_requestAndResponseWithRetry_does_not_modify_j_object_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  J *EXPECTED_JSON = reinterpret_cast<J *>(malloc(sizeof(J)));
  J *json_cpy;
  REQUIRE(nullptr != EXPECTED_JSON);

  RESET_FAKE(NoteRequestResponseWithRetry);
  memset(EXPECTED_JSON, 0x55, sizeof(J));
  {
    json_cpy = reinterpret_cast<J *>(malloc(sizeof(J)));
    REQUIRE(nullptr != json_cpy);
    memcpy(json_cpy, EXPECTED_JSON, sizeof(J));
  }

   // Action
  ///////////

  notecard.requestAndResponseWithRetry(json_cpy, 0);

   // Assert
  ///////////

  CHECK(!memcmp(EXPECTED_JSON, NoteRequestResponseWithRetry_fake.arg0_val, sizeof(J)));

  free(json_cpy);
  free(EXPECTED_JSON);
}

TEST_CASE("test_notecard_requestAndResponseWithRetry_does_not_modify_timeout_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  uint32_t EXPECTED_TIMEOUT = 10;
  RESET_FAKE(NoteRequestResponseWithRetry);

   // Action
  ///////////

  notecard.requestAndResponseWithRetry(nullptr, EXPECTED_TIMEOUT);

   // Assert
  ///////////

  CHECK(EXPECTED_TIMEOUT == NoteRequestResponseWithRetry_fake.arg1_val);
}

TEST_CASE("test_notecard_requestAndResponseWithRetry_does_not_modify_note_c_result_value_before_returning_to_caller")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  J * EXPECTED_JSON = reinterpret_cast<J *>(malloc(sizeof(J)));
  REQUIRE(nullptr != EXPECTED_JSON);

  RESET_FAKE(NoteRequestResponseWithRetry);
  memset(EXPECTED_JSON, 0x55, sizeof(J));
  {
    NoteRequestResponseWithRetry_fake.return_val = reinterpret_cast<J *>(malloc(sizeof(J)));
    REQUIRE(nullptr != NoteRequestResponseWithRetry_fake.return_val);
    memcpy(NoteRequestResponseWithRetry_fake.return_val, EXPECTED_JSON, sizeof(J));
  }

   // Action
  ///////////

  const J * const ACTUAL_RESULT = notecard.requestAndResponseWithRetry(nullptr, 0);

   // Assert
  ///////////

  CHECK(!memcmp(EXPECTED_JSON, ACTUAL_RESULT, sizeof(J)));

  free(NoteRequestResponseWithRetry_fake.return_val);
  free(EXPECTED_JSON);
}

TEST_CASE("test_notecard_deleteResponse_does_not_modify_j_object_parameter_pointer_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  J *const EXPECTED_POINTER = reinterpret_cast<J *>(0x19790917);

  RESET_FAKE(NoteDeleteResponse);

   // Action
  ///////////

  notecard.deleteResponse(EXPECTED_POINTER);

   // Assert
  ///////////

  CHECK(EXPECTED_POINTER == NoteDeleteResponse_fake.arg0_val);
}

TEST_CASE("test_notecard_logDebug_does_not_modify_string_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  const char str[] = "Hello, Test!";
  Notecard notecard;

  RESET_FAKE(NoteDebug);

   // Action
  ///////////

  notecard.logDebug(str);

   // Assert
  ///////////

  CHECK(!strcmp(str, NoteDebug_fake.arg0_val));
}

TEST_CASE("test_notecard_logDebugf_does_not_modify_string_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  const char str[] = "Hello, Test 123!";
  Notecard notecard;

  RESET_FAKE(NoteDebug);
  NoteDebug_message_cache.clear();
  NoteDebug_fake.custom_fake = NoteDebug_cacheMessage;

   // Action
  ///////////

  notecard.logDebugf("Hello, %s %d%c", "Test", 123, '!');

   // Assert
  ///////////

  CHECK(!strcmp(str, NoteDebug_message_cache.c_str()));
}

TEST_CASE("test_notecard_debugSyncStatus_does_not_modify_pollFrequencyMs_parameter_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  const int EXPECTED_RESULT = 79;

  RESET_FAKE(NoteDebugSyncStatus);

   // Action
  ///////////

  notecard.debugSyncStatus(EXPECTED_RESULT, 0);

   // Assert
  ///////////

  CHECK(EXPECTED_RESULT == NoteDebugSyncStatus_fake.arg0_val);
}

TEST_CASE("test_notecard_debugSyncStatus_does_not_modify_maxLevel_parameter_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  const int EXPECTED_RESULT = 79;

  RESET_FAKE(NoteDebugSyncStatus);

   // Action
  ///////////

  notecard.debugSyncStatus(0, EXPECTED_RESULT);

   // Assert
  ///////////

  CHECK(EXPECTED_RESULT == NoteDebugSyncStatus_fake.arg1_val);
}

TEST_CASE("test_notecard_debugSyncStatus_does_not_modify_note_c_result_value_before_returning_to_caller")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  const bool EXPECTED_RESULT = true;

  RESET_FAKE(NoteDebugSyncStatus);
  NoteDebugSyncStatus_fake.return_val = EXPECTED_RESULT;

   // Action
  ///////////

  const bool ACTUAL_RESULT = notecard.debugSyncStatus(0, 0);

   // Assert
  ///////////

  CHECK(EXPECTED_RESULT == ACTUAL_RESULT);
}

TEST_CASE("test_notecard_responseError_does_not_modify_j_object_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  J * EXPECTED_JSON = reinterpret_cast<J *>(malloc(sizeof(J))); // JCreateObject();
  J * json_cpy;
  REQUIRE(nullptr != EXPECTED_JSON);

  RESET_FAKE(NoteResponseError);
  memset(EXPECTED_JSON, 0x55, sizeof(J)); //REQUIRE(nullptr != JAddStringToObject(EXPECTED_JSON,"key1", "value1"));
  {
    json_cpy = reinterpret_cast<J *>(malloc(sizeof(J)));
    REQUIRE(nullptr != json_cpy);
    memcpy(json_cpy, EXPECTED_JSON, sizeof(J));
  } //REQUIRE(nullptr != (json_cpy = JDuplicate(EXPECTED_JSON,true)));

   // Action
  ///////////

  notecard.responseError(json_cpy);

   // Assert
  ///////////

  CHECK(!memcmp(EXPECTED_JSON, NoteResponseError_fake.arg0_val, sizeof(J)) /*JCompare(EXPECTED_JSON, noteRequestResponse_Parameters.rsp, false)*/);

  free(json_cpy);
  free(EXPECTED_JSON);
}

TEST_CASE("test_notecard_responseError_does_not_modify_note_c_result_value_before_returning_to_caller")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  const bool EXPECTED_RESULT = true;

  RESET_FAKE(NoteResponseError);
  NoteResponseError_fake.return_val = EXPECTED_RESULT;

   // Action
  ///////////

  const bool ACTUAL_RESULT = notecard.responseError(nullptr);

   // Assert
  ///////////

  CHECK(EXPECTED_RESULT == ACTUAL_RESULT);
}

TEST_CASE("test_notecard_newCommand_does_not_modify_string_parameter_value_before_passing_to_note_c")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  const char EXPECTED_RESULT[] = "Hello, Test!";

  RESET_FAKE(NoteNewCommand);

   // Action
  ///////////

  notecard.newCommand(EXPECTED_RESULT);

   // Assert
  ///////////

  CHECK(!strcmp(EXPECTED_RESULT, NoteNewCommand_fake.arg0_val));
}

TEST_CASE("test_notecard_newCommand_does_not_modify_note_c_result_value_before_returning_to_caller")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  J * EXPECTED_JSON = reinterpret_cast<J *>(malloc(sizeof(J))); // JCreateObject();
  REQUIRE(nullptr != EXPECTED_JSON);

  RESET_FAKE(NoteNewCommand);
  memset(EXPECTED_JSON, 0x55, sizeof(J)); //REQUIRE(nullptr != JAddStringToObject(EXPECTED_JSON,"key1", "value1"));
  {
    NoteNewCommand_fake.return_val = reinterpret_cast<J *>(malloc(sizeof(J)));
    REQUIRE(nullptr != NoteNewCommand_fake.return_val);
    memcpy(NoteNewCommand_fake.return_val, EXPECTED_JSON, sizeof(J));
  } //REQUIRE(nullptr != (NoteNewCommand_fake.return_val = JDuplicate(EXPECTED_JSON,true)));

   // Action
  ///////////

  const J * const ACTUAL_RESULT = notecard.newCommand(nullptr);

   // Assert
  ///////////

  CHECK(!memcmp(EXPECTED_JSON, ACTUAL_RESULT, sizeof(J)) /*JCompare(EXPECTED_JSON, ACTUAL_RESULT, false)*/);

  free(NoteNewCommand_fake.return_val);
  free(EXPECTED_JSON);
}

TEST_CASE("test_static_callback_note_i2c_receive_invokes_notei2c_receive")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x17;
  const uint16_t SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cReceiveFn noteI2cReceive = NoteSetFnI2CDefault_fake.arg4_val;
  RESET_FAKE(NoteI2c_receive);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cReceive(ADDRESS, response_buffer, SIZE, &actual_response_size);

   // Assert
  ///////////

  CHECK(NoteI2c_receive_fake.call_count);
}

TEST_CASE("test_static_callback_note_i2c_receive_does_not_modify_device_address_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  // 0x1B binary representation => 0001 1011
  const uint16_t EXPECTED_ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cReceiveFn noteI2cReceive = NoteSetFnI2CDefault_fake.arg4_val;
  RESET_FAKE(NoteI2c_receive);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cReceive(EXPECTED_ADDRESS, response_buffer, SIZE, &actual_response_size);

   // Assert
  ///////////

  CHECK(EXPECTED_ADDRESS == NoteI2c_receive_fake.arg0_val);
}

TEST_CASE("test_static_callback_note_i2c_receive_does_not_modify_buffer_parameter_address_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cReceiveFn noteI2cReceive = NoteSetFnI2CDefault_fake.arg4_val;
  RESET_FAKE(NoteI2c_receive);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cReceive(ADDRESS, response_buffer, SIZE, &actual_response_size);

   // Assert
  ///////////

  CHECK(response_buffer == NoteI2c_receive_fake.arg1_val);
}

TEST_CASE("test_static_callback_note_i2c_receive_does_not_modify_size_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t EXPECTED_SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cReceiveFn noteI2cReceive = NoteSetFnI2CDefault_fake.arg4_val;
  RESET_FAKE(NoteI2c_receive);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cReceive(ADDRESS, response_buffer, EXPECTED_SIZE, &actual_response_size);

   // Assert
  ///////////

  CHECK(EXPECTED_SIZE == NoteI2c_receive_fake.arg2_val);
}

TEST_CASE("test_static_callback_note_i2c_receive_does_not_modify_available_parameter_address_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cReceiveFn noteI2cReceive = NoteSetFnI2CDefault_fake.arg4_val;
  RESET_FAKE(NoteI2c_receive);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cReceive(ADDRESS, response_buffer, SIZE, &actual_response_size);

   // Assert
  ///////////

  CHECK(&actual_response_size == NoteI2c_receive_fake.arg3_val);
}

TEST_CASE("test_static_callback_note_i2c_receive_does_not_modify_interface_method_return_value")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cReceiveFn noteI2cReceive = NoteSetFnI2CDefault_fake.arg4_val;
  RESET_FAKE(NoteI2c_receive);  // Clear the structure for testing results
  NoteI2c_receive_fake.return_val = "i2c: fake test error!";

   // Action
  ///////////

  const char * const ACTUAL_RESULT = noteI2cReceive(ADDRESS, response_buffer, SIZE, &actual_response_size);

   // Assert
  ///////////

  CHECK(!strcmp(ACTUAL_RESULT, NoteI2c_receive_fake.return_val));
}

TEST_CASE("test_static_callback_note_i2c_receive_does_not_call_interface_method_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cReceiveFn noteI2cReceive = NoteSetFnI2CDefault_fake.arg4_val;

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteI2c *>(nullptr));
  RESET_FAKE(NoteI2c_receive);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cReceive(ADDRESS, response_buffer, SIZE, &actual_response_size);

   // Assert
  ///////////

  CHECK(!NoteI2c_receive_fake.call_count);
}

TEST_CASE("test_static_callback_note_i2c_receive_returns_error_message_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t response_buffer[32];
  uint32_t actual_response_size;
  const char * const EXPECTED_RESULT = "i2c: A call to Notecard::begin() is required. {io}";

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cReceiveFn noteI2cReceive = NoteSetFnI2CDefault_fake.arg4_val;

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteI2c *>(nullptr));
  RESET_FAKE(NoteI2c_receive);  // Clear the structure for testing results

   // Action
  ///////////

  const char * const ACTUAL_RESULT = noteI2cReceive(ADDRESS, response_buffer, SIZE, &actual_response_size);

   // Assert
  ///////////

  CHECK(!strcmp(ACTUAL_RESULT,EXPECTED_RESULT));
}

TEST_CASE("test_static_callback_note_i2c_reset_invokes_notei2c_reset")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x17;

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cResetFn noteI2cReset = NoteSetFnI2CDefault_fake.arg2_val;
  RESET_FAKE(NoteI2c_reset);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cReset(ADDRESS);

   // Assert
  ///////////

  CHECK(NoteI2c_reset_fake.call_count);
}

TEST_CASE("test_static_callback_note_i2c_reset_does_not_modify_device_address_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  // 0x1B binary representation => 0001 1011
  const uint16_t EXPECTED_ADDRESS = 0x1B;

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cResetFn noteI2cReset = NoteSetFnI2CDefault_fake.arg2_val;
  RESET_FAKE(NoteI2c_reset);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cReset(EXPECTED_ADDRESS);

   // Assert
  ///////////

  CHECK(EXPECTED_ADDRESS == NoteI2c_reset_fake.arg0_val);
}

TEST_CASE("test_static_callback_note_i2c_reset_does_not_modify_interface_method_return_value")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cResetFn noteI2cReset = NoteSetFnI2CDefault_fake.arg2_val;
  RESET_FAKE(NoteI2c_reset);  // Clear the structure for testing results
  NoteI2c_reset_fake.return_val = true;

   // Action
  ///////////

  const bool ACTUAL_RESULT = noteI2cReset(ADDRESS);

   // Assert
  ///////////

  CHECK(ACTUAL_RESULT == NoteI2c_reset_fake.return_val);
}

TEST_CASE("test_static_callback_note_i2c_reset_does_not_call_interface_method_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cResetFn noteI2cReset = NoteSetFnI2CDefault_fake.arg2_val;

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteI2c *>(nullptr));
  RESET_FAKE(NoteI2c_reset);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cReset(ADDRESS);

   // Assert
  ///////////

  CHECK(!NoteI2c_reset_fake.call_count);
}

TEST_CASE("test_static_callback_note_i2c_reset_returns_false_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cResetFn noteI2cReset = NoteSetFnI2CDefault_fake.arg2_val;

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteI2c *>(nullptr));
  RESET_FAKE(NoteI2c_reset);  // Clear the structure for testing results
  NoteI2c_reset_fake.return_val = true;  // Ensure that if the mock were to be invoked the test would fail

   // Action
  ///////////

  const bool ACTUAL_RESULT = noteI2cReset(ADDRESS);

   // Assert
  ///////////

  CHECK(!ACTUAL_RESULT);
}

TEST_CASE("test_static_callback_note_i2c_transmit_invokes_notei2c_transmit")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x17;
  const uint16_t SIZE = 13;
  uint8_t transmit_buffer[] = "Transmissible data";

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cTransmitFn noteI2cTransmit = NoteSetFnI2CDefault_fake.arg3_val;
  RESET_FAKE(NoteI2c_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cTransmit(ADDRESS, transmit_buffer, SIZE);

   // Assert
  ///////////

  CHECK(NoteI2c_transmit_fake.call_count);
}

TEST_CASE("test_static_callback_note_i2c_transmit_does_not_modify_device_address_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  // 0x1B binary representation => 0001 1011
  const uint16_t EXPECTED_ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t transmit_buffer[] = "Transmissible data";

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cTransmitFn noteI2cTransmit = NoteSetFnI2CDefault_fake.arg3_val;
  RESET_FAKE(NoteI2c_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cTransmit(EXPECTED_ADDRESS, transmit_buffer, SIZE);

   // Assert
  ///////////

  CHECK(EXPECTED_ADDRESS == NoteI2c_transmit_fake.arg0_val);
}

TEST_CASE("test_static_callback_note_i2c_transmit_does_not_modify_buffer_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t transmit_buffer[] = "Test Passed!";

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cTransmitFn noteI2cTransmit = NoteSetFnI2CDefault_fake.arg3_val;
  RESET_FAKE(NoteI2c_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cTransmit(ADDRESS, transmit_buffer, SIZE);

   // Assert
  ///////////

  CHECK((!strcmp(reinterpret_cast<char *>(transmit_buffer),reinterpret_cast<char *>(NoteI2c_transmit_fake.arg1_val))));
}

TEST_CASE("test_static_callback_note_i2c_transmit_does_not_modify_size_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t EXPECTED_SIZE = 13;
  uint8_t transmit_buffer[] = "Transmissible data";

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cTransmitFn noteI2cTransmit = NoteSetFnI2CDefault_fake.arg3_val;
  RESET_FAKE(NoteI2c_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cTransmit(ADDRESS, transmit_buffer, EXPECTED_SIZE);

   // Assert
  ///////////

  CHECK(EXPECTED_SIZE == NoteI2c_transmit_fake.arg2_val);
}

TEST_CASE("test_static_callback_note_i2c_transmit_does_not_modify_interface_method_return_value")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t transmit_buffer[] = "Transmissible data";

  Notecard notecard;
  NoteI2c_Fake mockI2c;  // Instantiate NoteI2c (mocked)
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cTransmitFn noteI2cTransmit = NoteSetFnI2CDefault_fake.arg3_val;
  RESET_FAKE(NoteI2c_transmit);  // Clear the structure for testing results
  NoteI2c_transmit_fake.return_val = "i2c: fake test error!";

   // Action
  ///////////

  const char * const ACTUAL_RESULT = noteI2cTransmit(ADDRESS, transmit_buffer, SIZE);

   // Assert
  ///////////

  CHECK(!strcmp(ACTUAL_RESULT, NoteI2c_transmit_fake.return_val));
}

TEST_CASE("test_static_callback_note_i2c_transmit_does_not_call_interface_method_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t transmit_buffer[] = "Transmissible data";

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cTransmitFn noteI2cTransmit = NoteSetFnI2CDefault_fake.arg3_val;

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteI2c *>(nullptr));
  RESET_FAKE(NoteI2c_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  noteI2cTransmit(ADDRESS, transmit_buffer, SIZE);

   // Assert
  ///////////

  CHECK(!NoteI2c_transmit_fake.call_count);
}

TEST_CASE("test_static_callback_note_i2c_transmit_returns_error_message_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const uint16_t ADDRESS = 0x1B;
  const uint16_t SIZE = 13;
  uint8_t transmit_buffer[] = "Transmissible data";
  const char * const EXPECTED_RESULT = "i2c: A call to Notecard::begin() is required. {io}";

  Notecard notecard;
  NoteI2c_Fake mockI2c;
  notecard.begin(&mockI2c);  // Provides access to the hidden static callback methods through `note-c` mocks
  i2cTransmitFn noteI2cTransmit = NoteSetFnI2CDefault_fake.arg3_val;

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteI2c *>(nullptr));
  RESET_FAKE(NoteI2c_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  const char * const ACTUAL_RESULT = noteI2cTransmit(ADDRESS, transmit_buffer, SIZE);

   // Assert
  ///////////

  CHECK(!strcmp(ACTUAL_RESULT,EXPECTED_RESULT));
}

TEST_CASE("test_static_callback_note_log_print_invokes_notelog_print")
{
  resetFakes();
   // Arrange
  ////////////

  const char message[] = "Test passed!";

  Notecard notecard;
  NoteLog_Fake mockLog;  // Instantiate NoteLog (mocked)
  notecard.setDebugOutputStream(&mockLog);  // Provides access to the hidden static callback methods through `note-c` mocks
  debugOutputFn noteLogPrint = NoteSetFnDebugOutput_fake.arg0_val;  // Capture the internal Notecard log function, `noteLogPrint`
  RESET_FAKE(NoteLog_print);  // Clear the structure for testing NoteLog::print results

   // Action
  ///////////

  noteLogPrint(message);

   // Assert
  ///////////

  CHECK(NoteLog_print_fake.call_count);
}

TEST_CASE("test_static_callback_note_log_print_does_not_modify_message_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  const char message[] = "Test passed!";

  Notecard notecard;
  NoteLog_Fake mockLog;  // Instantiate NoteLog (mocked)
  notecard.setDebugOutputStream(&mockLog);  // Provides access to the hidden static callback methods through `note-c` mocks
  debugOutputFn noteLogPrint = NoteSetFnDebugOutput_fake.arg0_val;  // Capture the internal Notecard log function, `noteLogPrint`
  RESET_FAKE(NoteLog_print);  // Clear the structure for testing NoteLog::print results

   // Action
  ///////////

  noteLogPrint(message);

   // Assert
  ///////////

  CHECK(!strcmp(message, NoteLog_print_fake.arg0_val));
}

TEST_CASE("test_static_callback_note_log_print_does_not_modify_interface_method_return_value")
{
  resetFakes();
   // Arrange
  ////////////

  const char message[] = "Test passed!";

  Notecard notecard;
  NoteLog_Fake mockLog;  // Instantiate NoteLog (mocked)
  notecard.setDebugOutputStream(&mockLog);  // Provides access to the hidden static callback methods through `note-c` mocks
  debugOutputFn noteLogPrint = NoteSetFnDebugOutput_fake.arg0_val;  // Capture the internal Notecard log function, `noteLogPrint`
  RESET_FAKE(NoteLog_print);  // Clear the structure for testing NoteLog::print results
  NoteLog_print_fake.return_val = sizeof(message);

   // Action
  ///////////

  const size_t ACTUAL_RESULT = noteLogPrint(message);

   // Assert
  ///////////

  CHECK(ACTUAL_RESULT == NoteLog_print_fake.return_val);
}

TEST_CASE("test_static_callback_note_log_print_does_not_call_interface_method_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const char message[] = "Test passed!";

  Notecard notecard;

  // Capture the internal Notecard log function, `noteLogPrint`
  NoteLog_Fake mockLog;
  notecard.setDebugOutputStream(&mockLog);  // Provides access to the internal static callback methods through `note-c` mocks
  debugOutputFn noteLogPrint = NoteSetFnDebugOutput_fake.arg0_val;  // Capture the internal Notecard log function, `noteLogPrint`

  // Reset to ensure interface is not instantiated
  notecard.setDebugOutputStream(nullptr);
  RESET_FAKE(NoteLog_print);  // Clear the structure for testing NoteLog::print results

   // Action
  ///////////

  noteLogPrint(message);

   // Assert
  ///////////

  CHECK(!NoteLog_print_fake.call_count);
}

TEST_CASE("test_static_callback_note_log_print_returns_false_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const char message[] = "Test passed!";

  Notecard notecard;

  // Capture the internal Notecard log function, `noteLogPrint`
  NoteLog_Fake mockLog;
  notecard.setDebugOutputStream(&mockLog);  // Provides access to the internal static callback methods through `note-c` mocks
  debugOutputFn noteLogPrint = NoteSetFnDebugOutput_fake.arg0_val;  // Capture the internal Notecard log function, `noteLogPrint`

  // Reset to ensure interface is not instantiated
  notecard.setDebugOutputStream(nullptr);
  RESET_FAKE(NoteLog_print);  // Clear the structure for testing NoteLog::print results
  NoteLog_print_fake.return_val = sizeof(message);  // Ensure that if the mock were to be invoked the test would fail

   // Action
  ///////////

  const size_t ACTUAL_RESULT = noteLogPrint(message);

   // Assert
  ///////////

  CHECK((!ACTUAL_RESULT && !NoteLog_print_fake.call_count));
}

TEST_CASE("test_static_callback_note_serial_available_invokes_noteserial_available")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialAvailableFn noteSerialAvailable = NoteSetFnSerialDefault_fake.arg2_val;  // Capture the internal Notecard serial function, `noteSerialAvailable`
  RESET_FAKE(NoteSerial_available);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialAvailable();

   // Assert
  ///////////

  CHECK(NoteSerial_available_fake.call_count);
}

TEST_CASE("test_static_callback_note_serial_available_does_not_modify_interface_method_return_value")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialAvailableFn noteSerialAvailable = NoteSetFnSerialDefault_fake.arg2_val;  // Capture the internal Notecard serial function, `noteSerialAvailable`
  RESET_FAKE(NoteSerial_available);  // Clear the structure for testing results
  NoteSerial_available_fake.return_val = true;

   // Action
  ///////////

  const bool ACTUAL_RESULT = noteSerialAvailable();

   // Assert
  ///////////

  CHECK(ACTUAL_RESULT == NoteSerial_available_fake.return_val);
}

TEST_CASE("test_static_callback_note_serial_available_does_not_call_interface_method_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;

  // Capture the internal Notecard log function, `noteSerialAvailable`
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialAvailableFn noteSerialAvailable = NoteSetFnSerialDefault_fake.arg2_val;  // Capture the internal Notecard serial function, `noteSerialAvailable`

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteSerial *>(nullptr));
  RESET_FAKE(NoteSerial_available);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialAvailable();

   // Assert
  ///////////

  CHECK(!NoteSerial_available_fake.call_count);
}

TEST_CASE("test_static_callback_note_serial_available_returns_false_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;

  // Capture the internal Notecard log function, `noteSerialAvailable`
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialAvailableFn noteSerialAvailable = NoteSetFnSerialDefault_fake.arg2_val;  // Capture the internal Notecard serial function, `noteSerialAvailable`

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteSerial *>(nullptr));
  RESET_FAKE(NoteSerial_available);  // Clear the structure for testing results
  NoteSerial_available_fake.return_val = true;  // Ensure that if the mock were to be invoked the test would fail

   // Action
  ///////////

  const bool ACTUAL_RESULT = noteSerialAvailable();

   // Assert
  ///////////

  CHECK(!ACTUAL_RESULT);
}

TEST_CASE("test_static_callback_note_serial_receive_invokes_noteserial_receive")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialReceiveFn noteSerialReceive = NoteSetFnSerialDefault_fake.arg3_val;  // Capture the internal Notecard serial function, `noteSerialReceive`
  RESET_FAKE(NoteSerial_receive);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialReceive();

   // Assert
  ///////////

  CHECK(NoteSerial_receive_fake.call_count);
}

TEST_CASE("test_static_callback_note_serial_receive_does_not_modify_interface_method_return_value")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialReceiveFn noteSerialReceive = NoteSetFnSerialDefault_fake.arg3_val;  // Capture the internal Notecard serial function, `noteSerialReceive`
  RESET_FAKE(NoteSerial_receive);  // Clear the structure for testing results
  NoteSerial_receive_fake.return_val = 'Z';

   // Action
  ///////////

  const char ACTUAL_RESULT = noteSerialReceive();

   // Assert
  ///////////

  CHECK(ACTUAL_RESULT == NoteSerial_receive_fake.return_val);
}

TEST_CASE("test_static_callback_note_serial_receive_does_not_call_interface_method_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;

  // Capture the internal Notecard log function, `noteSerialAvailable`
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialReceiveFn noteSerialReceive = NoteSetFnSerialDefault_fake.arg3_val;  // Capture the internal Notecard serial function, `noteSerialReceive`

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteSerial *>(nullptr));
  RESET_FAKE(NoteSerial_receive);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialReceive();

   // Assert
  ///////////

  CHECK(!NoteSerial_receive_fake.call_count);
}

TEST_CASE("test_static_callback_note_serial_receive_returns_false_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  static const char EXPECTED_RESULT = '\0';
  Notecard notecard;

  // Capture the internal Notecard log function, `noteSerialAvailable`
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialReceiveFn noteSerialReceive = NoteSetFnSerialDefault_fake.arg3_val;  // Capture the internal Notecard serial function, `noteSerialReceive`

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteSerial *>(nullptr));
  RESET_FAKE(NoteSerial_receive);  // Clear the structure for testing results
  NoteSerial_receive_fake.return_val = 'Z';

   // Action
  ///////////

  const char ACTUAL_RESULT = noteSerialReceive();

   // Assert
  ///////////

  CHECK(ACTUAL_RESULT == EXPECTED_RESULT);
}

TEST_CASE("test_static_callback_note_serial_reset_invokes_noteserial_reset")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialResetFn noteSerialReset = NoteSetFnSerialDefault_fake.arg0_val;  // Capture the internal Notecard serial function, `noteSerialReset`
  RESET_FAKE(NoteSerial_reset);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialReset();

   // Assert
  ///////////

  CHECK(NoteSerial_reset_fake.call_count);
}

TEST_CASE("test_static_callback_note_serial_reset_does_not_modify_interface_method_return_value")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialResetFn noteSerialReset = NoteSetFnSerialDefault_fake.arg0_val;  // Capture the internal Notecard serial function, `noteSerialReset`
  RESET_FAKE(NoteSerial_reset);  // Clear the structure for testing results
  NoteSerial_reset_fake.return_val = true;

   // Action
  ///////////

  const bool ACTUAL_RESULT = noteSerialReset();

   // Assert
  ///////////

  CHECK(ACTUAL_RESULT == NoteSerial_reset_fake.return_val);
}

TEST_CASE("test_static_callback_note_serial_reset_does_not_call_interface_method_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;

  // Capture the internal Notecard log function, `noteSerialAvailable`
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialResetFn noteSerialReset = NoteSetFnSerialDefault_fake.arg0_val;  // Capture the internal Notecard serial function, `noteSerialReset`

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteSerial *>(nullptr));
  RESET_FAKE(NoteSerial_reset);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialReset();

   // Assert
  ///////////

  CHECK(!NoteSerial_reset_fake.call_count);
}

TEST_CASE("test_static_callback_note_serial_reset_returns_false_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;

  // Capture the internal Notecard log function, `noteSerialAvailable`
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialResetFn noteSerialReset = NoteSetFnSerialDefault_fake.arg0_val;  // Capture the internal Notecard serial function, `noteSerialReset`

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteSerial *>(nullptr));
  RESET_FAKE(NoteSerial_reset);  // Clear the structure for testing results
  NoteSerial_reset_fake.return_val = true;

   // Action
  ///////////

  const bool ACTUAL_RESULT = noteSerialReset();

   // Assert
  ///////////

  CHECK(!ACTUAL_RESULT);
}

TEST_CASE("test_static_callback_note_serial_transmit_invokes_noteserial_transmit")
{
  resetFakes();
   // Arrange
  ////////////

  uint8_t text[] = "Test passed!";
  static const size_t LEN = sizeof(text);
  static const bool FLUSH = true;

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialTransmitFn noteSerialTransmit = NoteSetFnSerialDefault_fake.arg1_val;  // Capture the internal Notecard serial function, `noteSerialTransmit`
  RESET_FAKE(NoteSerial_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialTransmit(text, LEN, FLUSH);

   // Assert
  ///////////

  CHECK(NoteSerial_transmit_fake.call_count);
}

TEST_CASE("test_static_callback_note_serial_transmit_does_not_modify_text_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  uint8_t expected_text[] = "Test passed!";
  const size_t LEN = sizeof(expected_text);
  const bool FLUSH = true;

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialTransmitFn noteSerialTransmit = NoteSetFnSerialDefault_fake.arg1_val;  // Capture the internal Notecard serial function, `noteSerialTransmit`
  RESET_FAKE(NoteSerial_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialTransmit(expected_text, LEN, FLUSH);

   // Assert
  ///////////

  CHECK((!strcmp(reinterpret_cast<char *>(expected_text),reinterpret_cast<char *>(NoteSerial_transmit_fake.arg0_val))));
}

TEST_CASE("test_static_callback_note_serial_transmit_does_not_modify_length_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  uint8_t text[] = "Test passed!";
  const size_t EXPECTED_LEN = sizeof(text);
  const bool FLUSH = true;

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialTransmitFn noteSerialTransmit = NoteSetFnSerialDefault_fake.arg1_val;  // Capture the internal Notecard serial function, `noteSerialTransmit`
  RESET_FAKE(NoteSerial_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialTransmit(text, EXPECTED_LEN, FLUSH);

   // Assert
  ///////////

  CHECK(EXPECTED_LEN == NoteSerial_transmit_fake.arg1_val);
}

TEST_CASE("test_static_callback_note_serial_transmit_does_not_modify_flush_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  uint8_t text[] = "Test passed!";
  const size_t LEN = sizeof(text);
  const bool EXPECTED_FLUSH = true;

  Notecard notecard;
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialTransmitFn noteSerialTransmit = NoteSetFnSerialDefault_fake.arg1_val;  // Capture the internal Notecard serial function, `noteSerialTransmit`
  RESET_FAKE(NoteSerial_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialTransmit(text, LEN, EXPECTED_FLUSH);

   // Assert
  ///////////

  CHECK(EXPECTED_FLUSH == NoteSerial_transmit_fake.arg2_val);
}

TEST_CASE("test_static_callback_note_serial_transmit_does_not_call_interface_method_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  uint8_t text[] = "Test passed!";
  const size_t LEN = sizeof(text);
  const bool EXPECTED_FLUSH = true;

  Notecard notecard;

  // Capture the internal Notecard log function, `noteSerialAvailable`
  NoteSerial_Fake mockSerial;  // Instantiate NoteSerial (mocked)
  notecard.begin(&mockSerial);  // Provides access to the hidden static callback methods through `note-c` mocks
  serialTransmitFn noteSerialTransmit = NoteSetFnSerialDefault_fake.arg1_val;  // Capture the internal Notecard serial function, `noteSerialTransmit`

  // Reset to ensure the interface is not instantiated
  notecard.begin(static_cast<NoteSerial *>(nullptr));
  RESET_FAKE(NoteSerial_transmit);  // Clear the structure for testing results

   // Action
  ///////////

  noteSerialTransmit(text, LEN, EXPECTED_FLUSH);

   // Assert
  ///////////

  CHECK(!NoteSerial_transmit_fake.call_count);
}

TEST_CASE("test_static_callback_note_txn_start_invokes_notetxn_start")
{
  resetFakes();
   // Arrange
  ////////////

  const size_t TIMEOUT_MS = 917;

  Notecard notecard;

  // Capture the internal Notecard transaction function, `noteTransactionStart`
  NoteTxn_Fake mockTxn;  // Instantiate NoteTxn (mocked)
  notecard.setTransactionPins(&mockTxn);  // Provides access to the hidden static callback methods through `note-c` mocks
  txnStartFn noteTransactionStart = NoteSetFnTransaction_fake.arg0_val;  // Capture the internal Notecard serial function, `noteTransactionStart`
  RESET_FAKE(NoteTxn_start);  // Clear the structure for testing results

   // Action
  ///////////

  noteTransactionStart(TIMEOUT_MS);

   // Assert
  ///////////

  CHECK(NoteTxn_start_fake.call_count);
}

TEST_CASE("test_static_callback_note_txn_start_does_not_modify_timeout_ms_parameter_before_passing_to_interface_method")
{
  resetFakes();
   // Arrange
  ////////////

  const size_t TIMEOUT_MS = 917;

  Notecard notecard;

  // Capture the internal Notecard transaction function, `noteTransactionStart`
  NoteTxn_Fake mockTxn;  // Instantiate NoteTxn (mocked)
  notecard.setTransactionPins(&mockTxn);  // Provides access to the hidden static callback methods through `note-c` mocks
  txnStartFn noteTransactionStart = NoteSetFnTransaction_fake.arg0_val;  // Capture the internal Notecard serial function, `noteTransactionStart`
  RESET_FAKE(NoteTxn_start);  // Clear the structure for testing results

   // Action
  ///////////

  noteTransactionStart(TIMEOUT_MS);

   // Assert
  ///////////

  CHECK(NoteTxn_start_fake.arg0_val == TIMEOUT_MS);
}

TEST_CASE("test_static_callback_note_txn_start_does_not_modify_interface_method_return_value")
{
  resetFakes();
   // Arrange
  ////////////

  const size_t TIMEOUT_MS = 917;

  Notecard notecard;

  // Capture the internal Notecard transaction function, `noteTransactionStart`
  NoteTxn_Fake mockTxn;  // Instantiate NoteTxn (mocked)
  notecard.setTransactionPins(&mockTxn);  // Provides access to the hidden static callback methods through `note-c` mocks
  txnStartFn noteTransactionStart = NoteSetFnTransaction_fake.arg0_val;  // Capture the internal Notecard serial function, `noteTransactionStart`
  RESET_FAKE(NoteTxn_start);  // Clear the structure for testing results
  NoteTxn_start_fake.return_val = true;

   // Action
  ///////////

  const bool ACTUAL_RESULT = noteTransactionStart(TIMEOUT_MS);

   // Assert
  ///////////

  CHECK(NoteTxn_start_fake.return_val == ACTUAL_RESULT);
}

TEST_CASE("test_static_callback_note_txn_start_does_not_call_interface_method_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const size_t TIMEOUT_MS = 917;

  Notecard notecard;

  // Capture the internal Notecard transaction function, `noteTransactionStart`
  NoteTxn_Fake mockTxn;  // Instantiate NoteTxn (mocked)
  notecard.setTransactionPins(&mockTxn);  // Provides access to the hidden static callback methods through `note-c` mocks
  txnStartFn noteTransactionStart = NoteSetFnTransaction_fake.arg0_val;  // Capture the internal Notecard serial function, `noteTransactionStart`

  // Reset to ensure the interface is not instantiated
  notecard.setTransactionPins(static_cast<NoteTxn *>(nullptr));
  RESET_FAKE(NoteTxn_start);  // Clear the structure for testing results

   // Action
  ///////////

  noteTransactionStart(TIMEOUT_MS);

   // Assert
  ///////////

  CHECK(!NoteTxn_start_fake.call_count);
}

TEST_CASE("test_static_callback_note_txn_start_returns_true_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  const bool EXPECTED_RESULT = true;
  const size_t TIMEOUT_MS = 917;

  Notecard notecard;

  // Capture the internal Notecard transaction function, `noteTransactionStart`
  NoteTxn_Fake mockTxn;  // Instantiate NoteTxn (mocked)
  notecard.setTransactionPins(&mockTxn);  // Provides access to the hidden static callback methods through `note-c` mocks
  txnStartFn noteTransactionStart = NoteSetFnTransaction_fake.arg0_val;  // Capture the internal Notecard serial function, `noteTransactionStart`

  // Reset to ensure the interface is not instantiated
  notecard.setTransactionPins(static_cast<NoteTxn *>(nullptr));
  RESET_FAKE(NoteTxn_start);  // Clear the structure for testing results
  NoteTxn_start_fake.return_val = false;

   // Action
  ///////////

  const bool ACTUAL_RESULT = noteTransactionStart(TIMEOUT_MS);

   // Assert
  ///////////

  CHECK(ACTUAL_RESULT == EXPECTED_RESULT);
}

TEST_CASE("test_static_callback_note_txn_stop_invokes_notetxn_stop")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;

  // Capture the internal Notecard transaction function, `noteTransactionStop`
  NoteTxn_Fake mockTxn;  // Instantiate NoteTxn (mocked)
  notecard.setTransactionPins(&mockTxn);  // Provides access to the hidden static callback methods through `note-c` mocks
  txnStopFn noteTransactionStop = NoteSetFnTransaction_fake.arg1_val;  // Capture the internal Notecard serial function, `noteTransactionStop`
  RESET_FAKE(NoteTxn_stop);  // Clear the structure for testing results

   // Action
  ///////////

  noteTransactionStop();

   // Assert
  ///////////

  CHECK(NoteTxn_stop_fake.call_count);
}

TEST_CASE("test_static_callback_note_txn_stop_does_not_call_interface_method_when_interface_has_not_been_instantiated")
{
  resetFakes();
   // Arrange
  ////////////

  Notecard notecard;

  // Capture the internal Notecard transaction function, `noteTransactionStop`
  NoteTxn_Fake mockTxn;  // Instantiate NoteTxn (mocked)
  notecard.setTransactionPins(&mockTxn);  // Provides access to the hidden static callback methods through `note-c` mocks
  txnStopFn noteTransactionStop = NoteSetFnTransaction_fake.arg1_val;  // Capture the internal Notecard serial function, `noteTransactionStop`

  // Reset to ensure the interface is not instantiated
  notecard.setTransactionPins(static_cast<NoteTxn *>(nullptr));
  RESET_FAKE(NoteTxn_stop);  // Clear the structure for testing results

   // Action
  ///////////

  noteTransactionStop();

   // Assert
  ///////////

  CHECK(!NoteTxn_stop_fake.call_count);
}

