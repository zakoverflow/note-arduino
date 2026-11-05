// Integration harness: real Notecard.cpp + real Note*_Arduino.cpp units
// against the FFF note-c and Arduino HAL fakes under test/. Spec/tests first.
#include <catch2/catch_test_macros.hpp>

#include <stdint.h>

#include "arduino_fakes.hpp"
#include "NoteI2c.hpp"
#include "Notecard.h"

// Prove Notecard::begin over I2C wires the real NoteI2c_Arduino into the
// fake note-c I2C hooks: a call through those hooks drives the Wire bus.
TEST_CASE("test_notecard_begin_over_i2c_registers_note_c_hooks_that_drive_the_arduino_wire_bus", "[Notecard][integration][i2c]")
{
    resetArduinoFakes();

    // Arrange: real factory builds a real NoteI2c_Arduino on the fake Wire.
    TwoWire_endTransmission_fake.return_val = 0;
    Notecard notecard;
    NoteI2c * i2c = make_note_i2c(Wire);
    REQUIRE(i2c != nullptr);
    REQUIRE(TwoWire_begin_fake.call_count == 1);

    // Action: begin registers the Notecard I2C thunks with note-c.
    notecard.begin(i2c, NOTE_I2C_ADDR_DEFAULT, NOTE_I2C_MTU_DEFAULT);

    // Assert: note-c got non-null I2C hooks (fake note-c records the call).
    REQUIRE(NoteSetFnI2CDefault_fake.call_count == 1);
    i2cTransmitFn transmit_hook = NoteSetFnI2CDefault_fake.arg3_val;
    i2cResetFn reset_hook = NoteSetFnI2CDefault_fake.arg2_val;
    i2cReceiveFn receive_hook = NoteSetFnI2CDefault_fake.arg4_val;
    REQUIRE(transmit_hook != nullptr);
    REQUIRE(reset_hook != nullptr);
    REQUIRE(receive_hook != nullptr);

    // Assert: a call through the registered transmit hook drives Wire via
    // the real NoteI2c_Arduino::transmit (not a stubbed interface).
    uint8_t payload[] = {0x10, 0x20, 0x30};
    const uint16_t address = static_cast<uint16_t>(NOTE_I2C_ADDR_DEFAULT);
    const char * tx_error = transmit_hook(address, payload, sizeof(payload));
    REQUIRE(tx_error == nullptr);
    REQUIRE(TwoWire_beginTransmission_fake.call_count == 1);
    REQUIRE(TwoWire_beginTransmission_fake.arg0_val == static_cast<uint8_t>(address));
    REQUIRE(TwoWire_write_byte_fake.call_count == 1);
    REQUIRE(TwoWire_write_byte_fake.arg0_val == static_cast<uint8_t>(sizeof(payload)));
    REQUIRE(TwoWire_write_buffer_fake.call_count == 1);
    REQUIRE(TwoWire_write_buffer_fake.arg1_val == sizeof(payload));
    REQUIRE(TwoWire_endTransmission_fake.call_count == 1);

    // notecard destructor releases the singleton via make_note_i2c(nullptr).
}
