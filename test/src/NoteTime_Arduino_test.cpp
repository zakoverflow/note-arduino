#include <catch2/catch_test_macros.hpp>

#include "arduino_fakes.hpp"
#include "NoteTime.h"

#include <cstdint>


// Called first in every TEST_CASE: RESET_FAKE on every FFF fake, then
// FFF_RESET_HISTORY() (see resetArduinoFakes()).
static void resetFakes(void)
{
  resetArduinoFakes();
}

// A model of the Arduino clock for the tests that need time to pass. The
// state is one number. The two FFF custom fakes read and move it.
namespace {
struct ArduinoClock {
    unsigned long now_ms;
};
ArduinoClock arduino_clock;

unsigned long arduinoClockMillis(void)
{
    return arduino_clock.now_ms;
}

void arduinoClockDelay(unsigned long ms)
{
    arduino_clock.now_ms += ms;
}

void startArduinoClockAt(unsigned long start_ms)
{
    arduino_clock.now_ms = start_ms;
    millis_fake.custom_fake = arduinoClockMillis;
    delay_fake.custom_fake = arduinoClockDelay;
}
}

TEST_CASE("test_notetime_arduino_notemillis_returns_the_arduino_millisecond_count", "[NoteTime_Arduino]")
{
  resetFakes();
    // Arrange
    uint32_t reported_ms = 0;
    uint32_t expected_ms = 0;

    SECTION("Clock at zero") {
        expected_ms = 0;
    }
    SECTION("Clock at one") {
        expected_ms = 1;
    }
    SECTION("Clock at an ordinary count") {
        expected_ms = 123456;
    }
    SECTION("Clock at the largest 32-bit count") {
        expected_ms = UINT32_MAX;
    }
    millis_fake.return_val = expected_ms;

    // Action
    reported_ms = noteMillis();

    // Assert
    CHECK(expected_ms == reported_ms);
}

TEST_CASE("test_notetime_arduino_notemillis_reads_the_arduino_clock_again_on_every_call", "[NoteTime_Arduino]")
{
  resetFakes();
    // Arrange
    unsigned long clock_readings[] = {10, 25, 25, 4000};
    SET_RETURN_SEQ(millis, clock_readings, 4);

    // Action
    const uint32_t first_ms = noteMillis();
    const uint32_t second_ms = noteMillis();
    const uint32_t third_ms = noteMillis();
    const uint32_t fourth_ms = noteMillis();

    // Assert
    CHECK(10 == first_ms);
    CHECK(25 == second_ms);
    CHECK(25 == third_ms);
    CHECK(4000 == fourth_ms);
}

TEST_CASE("test_notetime_arduino_notedelay_returns_after_the_requested_milliseconds_have_passed", "[NoteTime_Arduino]")
{
  resetFakes();
    // Arrange
    uint32_t requested_ms = 0;
    startArduinoClockAt(5000);

    SECTION("Delay of zero") {
        requested_ms = 0;
    }
    SECTION("Delay of one") {
        requested_ms = 1;
    }
    SECTION("Delay of one second") {
        requested_ms = 1000;
    }
    SECTION("Delay of the largest 32-bit count") {
        requested_ms = UINT32_MAX;
        startArduinoClockAt(0);
    }
    const uint32_t before_ms = noteMillis();

    // Action
    noteDelay(requested_ms);

    // Assert
    const uint32_t after_ms = noteMillis();
    CHECK(requested_ms == static_cast<uint32_t>(after_ms - before_ms));
}

TEST_CASE("test_notetime_arduino_notedelay_adds_up_when_called_repeatedly", "[NoteTime_Arduino]")
{
  resetFakes();
    // Arrange
    startArduinoClockAt(0);

    // Action
    noteDelay(100);
    noteDelay(250);
    noteDelay(0);
    noteDelay(1);

    // Assert
    CHECK(351 == noteMillis());
}
