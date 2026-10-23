#include <catch2/catch_test_macros.hpp>

#include "arduino_fakes.hpp"
#include "NoteSerial_Arduino.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>


// Called first in every TEST_CASE: RESET_FAKE on every FFF fake, then
// FFF_RESET_HISTORY() (see resetArduinoFakes()).
static void resetFakes(void)
{
  resetArduinoFakes();
}

namespace {

// Stateful fake of the transmit side of a serial port.
// It is built on the FFF fake HardwareSerial_write (custom_fake). The state is
// a plain struct, because an FFF custom_fake is a function pointer.
// Every write call appends the bytes the port accepted to bytes_written.
struct FakeSerialPort {
    std::string bytes_written;       // everything the port accepted, in order
    size_t max_bytes_per_write;      // the port accepts at most this many bytes per write call
};
FakeSerialPort fake_serial_port;

size_t fakeSerialPortWrite(uint8_t * buffer, size_t size)
{
    const size_t accepted = std::min(size, fake_serial_port.max_bytes_per_write);
    fake_serial_port.bytes_written.append(reinterpret_cast<const char *>(buffer), accepted);
    return accepted;
}

// Install the fake port in place of the capture fake. Call after resetFakes().
void useFakeSerialPort(size_t max_bytes_per_write = SIZE_MAX)
{
    fake_serial_port.bytes_written.clear();
    fake_serial_port.max_bytes_per_write = max_bytes_per_write;
    HardwareSerial_write_fake.custom_fake = fakeSerialPortWrite;
}

} // namespace

TEST_CASE("test_make_note_serial_instantiates_noteserial_object", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    NoteSerial * noteserial = nullptr;
    MakeNoteSerial_ArduinoParameters<HardwareSerial> arduino_parameters(Serial, 9600);

    // Action
    noteserial = make_note_serial<MakeNoteSerial_ArduinoParameters<HardwareSerial>>(arduino_parameters);

    // Assert
    CHECK(nullptr != noteserial);

    // Clean-up
    make_note_serial(nullptr);
}

TEST_CASE("test_make_note_serial_enforces_singleton_by_returning_same_noteserial_object_for_all_calls", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    MakeNoteSerial_ArduinoParameters<HardwareSerial> arduino_parameters(Serial, 9600);
    NoteSerial * const noteserial_1 = make_note_serial<MakeNoteSerial_ArduinoParameters<HardwareSerial>>(arduino_parameters);

    // Action
    NoteSerial * const noteserial_2 = make_note_serial<MakeNoteSerial_ArduinoParameters<HardwareSerial>>(arduino_parameters);

    // Assert
    CHECK(noteserial_1 == noteserial_2);

    // Clean-up
    make_note_serial(nullptr);
}

//int test_make_note_serial_returns_nullptr_when_nullptr_is_passed_as_parameter()
TEST_CASE("test_make_note_serial_deletes_singleton_when_nullptr_is_passed_as_parameter", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    MakeNoteSerial_ArduinoParameters<HardwareSerial> arduino_parameters(Serial, 9600);
    NoteSerial * noteserial = make_note_serial<MakeNoteSerial_ArduinoParameters<HardwareSerial>>(arduino_parameters);
    assert(noteserial);

    // Action
    noteserial = make_note_serial(nullptr);

    // Assert
    CHECK(nullptr == noteserial);
}

TEST_CASE("test_noteserial_arduino_constructor_invokes_hardware_serial_parameter_begin_method", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    RESET_FAKE(HardwareSerial_begin);

    // Action
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);

    // Assert
    CHECK(HardwareSerial_begin_fake.call_count);
}

TEST_CASE("test_noteserial_arduino_constructor_does_not_modify_baud_parameter_before_passing_to_hardware_serial_begin", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    const size_t EXPECTED_BAUD_RATE = 9600;

    RESET_FAKE(HardwareSerial_begin);

    // Action
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, EXPECTED_BAUD_RATE);

    // Assert
    CHECK(EXPECTED_BAUD_RATE == HardwareSerial_begin_fake.arg0_val);
}

TEST_CASE("test_noteserial_arduino_deconstructor_invokes_hardware_serial_end_method", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    RESET_FAKE(HardwareSerial_end);
    NoteSerial_Arduino<HardwareSerial> * noteserial = new NoteSerial_Arduino<HardwareSerial>(Serial, 9600);

    // Action
    delete noteserial;

    // Assert
    CHECK(HardwareSerial_end_fake.call_count);
}

TEST_CASE("test_noteserial_arduino_available_invokes_hardware_serial_available", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);
    RESET_FAKE(HardwareSerial_available);

    // Action
    noteserial.available();

    // Assert
    CHECK(HardwareSerial_available_fake.call_count);
}

TEST_CASE("test_noteserial_arduino_available_does_not_modify_hardware_serial_available_result_value_before_returning_to_caller", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);
    const size_t EXPECTED_RESULT = 79;

    RESET_FAKE(HardwareSerial_available);
    HardwareSerial_available_fake.return_val = EXPECTED_RESULT;

    // Action
    const size_t ACTUAL_RESULT = noteserial.available();

    // Assert
    CHECK(ACTUAL_RESULT == EXPECTED_RESULT);
}

TEST_CASE("test_noteserial_arduino_receive_invokes_hardware_serial_read", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);

    RESET_FAKE(HardwareSerial_read);

    // Action
    noteserial.receive();

    // Assert
    CHECK(HardwareSerial_read_fake.call_count);
}

TEST_CASE("test_noteserial_arduino_receive_does_not_modify_hardware_serial_read_result_value_before_returning_to_caller", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    const char EXPECTED_RESULT = 'z';
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);

    RESET_FAKE(HardwareSerial_read);
    HardwareSerial_read_fake.return_val = EXPECTED_RESULT;

    // Action
    const char ACTUAL_RESULT = noteserial.receive();

    // Assert
    CHECK(ACTUAL_RESULT == EXPECTED_RESULT);
}

TEST_CASE("test_noteserial_arduino_reset_invokes_hardware_serial_end", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);
    RESET_FAKE(HardwareSerial_end);

    // Action
    noteserial.reset();

    // Assert
    CHECK(HardwareSerial_end_fake.call_count);
}

TEST_CASE("test_noteserial_arduino_reset_invokes_hardware_serial_begin", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);
    RESET_FAKE(HardwareSerial_begin);

    // Action
    noteserial.reset();

    // Assert
    CHECK(HardwareSerial_begin_fake.call_count);
}

TEST_CASE("test_noteserial_arduino_reset_invokes_hardware_serial_begin_with_the_baud_parameter_that_was_originally_supplied_to_the_constructor", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    const size_t EXPECTED_BAUD_RATE = 9600;
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, EXPECTED_BAUD_RATE);

    RESET_FAKE(HardwareSerial_begin);

    // Action
    noteserial.reset();

    // Assert
    CHECK(EXPECTED_BAUD_RATE == HardwareSerial_begin_fake.arg0_val);
}

TEST_CASE("test_noteserial_arduino_reset_always_returns_true", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);

    // Action
    const bool ACTUAL_RESULT = noteserial.reset();

    // Assert
    CHECK(ACTUAL_RESULT);
}

TEST_CASE("test_noteserial_arduino_transmit_returns_the_number_of_bytes_the_serial_port_accepted_when_the_port_accepts_fewer_than_requested", "[NoteSerial_Arduino][transmit]")
{
  resetFakes();
    // Arrange
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);
    useFakeSerialPort(5);
    char buffer[] = "Hello, Test!";

    // Action
    const size_t ACTUAL_RESULT = noteserial.transmit(reinterpret_cast<uint8_t *>(buffer), strlen(buffer), true);

    // Assert
    CHECK("Hello" == fake_serial_port.bytes_written);
    CHECK(5 == ACTUAL_RESULT);
}

TEST_CASE("test_noteserial_arduino_transmit_writes_the_given_bytes_to_the_serial_port_in_order_and_returns_the_size", "[NoteSerial_Arduino][transmit]")
{
  resetFakes();
    // Arrange
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);
    useFakeSerialPort();
    char buffer[] = "Hello, Test!";
    const size_t SIZE = strlen(buffer);
    REQUIRE(12 == SIZE);

    // Action
    const size_t ACTUAL_RESULT = noteserial.transmit(reinterpret_cast<uint8_t *>(buffer), SIZE, true);

    // Assert
    CHECK("Hello, Test!" == fake_serial_port.bytes_written);
    CHECK(12 == ACTUAL_RESULT);
}

TEST_CASE("test_noteserial_arduino_transmit_invokes_hardware_serial_flush_when_flush_parameter_is_true", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);
    RESET_FAKE(HardwareSerial_flush);

    // Action
    noteserial.transmit(reinterpret_cast<uint8_t *>(const_cast<char *>("Hello, Test!")), 13, true);

    // Assert
    CHECK(HardwareSerial_flush_fake.call_count);
}

TEST_CASE("test_noteserial_arduino_transmit_does_not_invoke_hardware_serial_flush_when_flush_parameter_is_false", "[NoteSerial_Arduino]")
{
  resetFakes();
    // Arrange
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);
    RESET_FAKE(HardwareSerial_flush);

    // Action
    noteserial.transmit(reinterpret_cast<uint8_t *>(const_cast<char *>("Hello, Test 14!")), 16, false);

    // Assert
    CHECK(!HardwareSerial_flush_fake.call_count);
}
