#include <catch2/catch_test_macros.hpp>

#include "arduino_fakes.hpp"
#include "NoteSerial_Arduino.hpp"
#include "fake_clock.hpp"

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

// Stateful fake of a serial port (transmit + constructor readiness).
// It is built on FFF fakes HardwareSerial_write and HardwareSerial_operator_bool
// (custom_fake). The state is a plain struct, because an FFF custom_fake is a
// function pointer. Field layout matches the X5 FakeSerialPort shape (#14–#17)
// on the transmit side; readiness fields serve the constructor wait tests.
// Every write call appends the bytes the port accepted to bytes_written.
// operator bool reports whether the port is ready for the constructor wait.
struct FakeSerialPort {
    std::string bytes_written;       // everything the port accepted, in order
    size_t max_bytes_per_write;      // the port accepts at most this many bytes per write call
    bool never_ready;                // when true, operator bool is always false
};
FakeSerialPort fake_serial_port;

size_t fakeSerialPortWrite(uint8_t * buffer, size_t size)
{
    const size_t accepted = std::min(size, fake_serial_port.max_bytes_per_write);
    fake_serial_port.bytes_written.append(reinterpret_cast<const char *>(buffer), accepted);
    return accepted;
}

// Each readiness poll advances the fake clock by 1 ms. The constructor wait
// loop has no delay call; advancing here makes elapsed time match the timeout
// bound (a 3499 ms mutant yields used_ms 3499, not 3500).
bool fakeSerialPortIsReady(void)
{
    if (!fake_serial_port.never_ready) {
        return true;
    }
    fake_clock::advance_ms(1);
    return false;
}

// Install the fake port write side. Call after resetFakes().
void useFakeSerialPort(size_t max_bytes_per_write = SIZE_MAX)
{
    fake_serial_port.bytes_written.clear();
    fake_serial_port.max_bytes_per_write = max_bytes_per_write;
    HardwareSerial_write_fake.custom_fake = fakeSerialPortWrite;
}

const uint32_t SLACK_MS = 10;
const uint32_t PORT_WAIT_LIMIT_MS = 3500;

// Install a port that never becomes ready. Call after resetFakes().
// ms_per_read is 0: time moves only on readiness polls (see fakeSerialPortIsReady).
void useFakeSerialPortNeverReady(uint32_t clock_start_ms)
{
    fake_clock::start_at(clock_start_ms, /*ms_per_read=*/0);
    fake_serial_port.never_ready = true;
    HardwareSerial_operator_bool_fake.custom_fake = fakeSerialPortIsReady;
}

// Construct NoteSerial_Arduino and return the fake-clock elapsed time it used.
uint32_t millisecondsUsedByConstructor(void)
{
    const uint32_t start_ms = fake_clock::now();
    NoteSerial_Arduino<HardwareSerial> noteserial(Serial, 9600);
    (void)noteserial;
    return fake_clock::elapsed_since(start_ms);
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
    const size_t EXPECTED_SIZE = 12;
    REQUIRE(EXPECTED_SIZE == strlen(buffer));

    // Action
    const size_t ACTUAL_RESULT = noteserial.transmit(reinterpret_cast<uint8_t *>(buffer), EXPECTED_SIZE, true);

    // Assert
    CHECK("Hello, Test!" == fake_serial_port.bytes_written);
    CHECK(EXPECTED_SIZE == ACTUAL_RESULT);
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

// SPEC-NOTE: Decision Q-D7a option A. The lower bound of the serial constructor
// SPEC-NOTE: wait is 3500 ms: when the port never becomes ready, the constructor
// SPEC-NOTE: waits at least 3500 ms (elapsed on the fake clock) before giving up.
// SPEC-NOTE: Source: src/NoteSerial_Arduino.cpp:13 (NOTE_C_SERIAL_TIMEOUT_MS 3500)
// SPEC-NOTE: and :71-74 (wait loop). Headers and docs do not state the bound.
// SPEC-NOTE: The tests lock elapsed time. They do not lock how the loop spins.
// SPEC-NOTE: Early return when the port becomes ready is not locked here: it is
// SPEC-NOTE: not stated in NoteSerial.hpp / NoteSerial_Arduino.hpp or docs.
TEST_CASE("test_noteserial_arduino_constructor_returns_after_3500_ms_when_the_port_never_becomes_ready", "[NoteSerial_Arduino][constructor]")
{
  resetFakes();
    // Arrange
    useFakeSerialPortNeverReady(1000);

    // Action
    const uint32_t used_ms = millisecondsUsedByConstructor();

    // Assert — lower bound is the requirement; slack is measurement hygiene
    CHECK(used_ms >= PORT_WAIT_LIMIT_MS);
    CHECK(used_ms <= (PORT_WAIT_LIMIT_MS + SLACK_MS));
}

// SPEC-NOTE: Decision Q-D7b option A. When millis() rolls over during the wait,
// SPEC-NOTE: the wait still lasts at least 3500 ms. Known defect: startMs is a
// SPEC-NOTE: size_t (NoteSerial_Arduino.cpp:71). On a 64-bit host the wait ends
// SPEC-NOTE: at the wrap. On AVR (16-bit size_t) the wait is also wrong. Fix:
// SPEC-NOTE: uint32_t startMs (as note-c does in n_serial.c:70 / n_i2c.c:53).
// SPEC-NOTE: Owner: Tools & SDKs Engineer. [!shouldfail] until the src fix lands.
TEST_CASE("test_noteserial_arduino_constructor_waits_at_least_3500_ms_when_the_port_never_becomes_ready_and_the_clock_wraps", "[NoteSerial_Arduino][constructor][!shouldfail]")
{
  resetFakes();
    // Arrange — clock wraps 256 ms after start
    const uint32_t CLOCK_START_MS = 0xFFFFFF00;
    useFakeSerialPortNeverReady(CLOCK_START_MS);

    // Action
    const uint32_t used_ms = millisecondsUsedByConstructor();

    // Assert — across the wrap the wait must still last at least 3500 ms
    CHECK(used_ms >= PORT_WAIT_LIMIT_MS);
}
