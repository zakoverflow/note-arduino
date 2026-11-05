#ifndef FAKE_CLOCK_HPP
#define FAKE_CLOCK_HPP

// A fake clock for the tests that need time to pass.
//
// The clock is one state struct. FFF custom fakes read and move it:
//   NoteGetMs (note-c) and millis (Arduino) read the clock.
//   NoteDelayMs (note-c) and delay (Arduino) move the clock forward.
// The clock is a uint32_t. It wraps from 0xFFFFFFFF to 0 as the real clock does.
// Call fake_clock::start_at() AFTER resetFakes(). resetFakes() removes the
// custom fakes.
//
// Use the clock to test ELAPSED time (now() - start). Do not test the exact
// arguments of a delay call.

#include <stdint.h>

#include "arduino_fakes.hpp"

namespace fake_clock {

struct State {
    uint32_t now_ms;
    // How far one read of NoteGetMs() moves the clock. A loop that reads the
    // clock and never calls a delay needs a value above 0 to make time pass.
    uint32_t ms_per_read;
    // Totals of the delay calls.
    uint32_t delay_calls;
    uint32_t delayed_ms;
};

inline State & state(void)
{
    static State clock_state = {0, 0, 0, 0};
    return clock_state;
}

inline uint32_t now(void)
{
    return state().now_ms;
}

inline void advance_ms(uint32_t ms)
{
    state().now_ms += ms;  // wraps at 2^32
}

inline uint32_t elapsed_since(uint32_t start_ms)
{
    return static_cast<uint32_t>(state().now_ms - start_ms);
}

namespace detail {
inline uint32_t noteGetMs(void)
{
    State & clock_state = state();
    const uint32_t reading = clock_state.now_ms;
    clock_state.now_ms += clock_state.ms_per_read;
    return reading;
}
inline unsigned long millis(void)
{
    return state().now_ms;
}
inline void noteDelayMs(uint32_t ms)
{
    State & clock_state = state();
    ++clock_state.delay_calls;
    clock_state.delayed_ms += ms;
    clock_state.now_ms += ms;
}
inline void delay(unsigned long ms)
{
    noteDelayMs(static_cast<uint32_t>(ms));
}
}  // namespace detail

// Set the clock and connect it to the four fakes.
inline void start_at(uint32_t start_ms, uint32_t ms_per_read = 0)
{
    State & clock_state = state();
    clock_state.now_ms = start_ms;
    clock_state.ms_per_read = ms_per_read;
    clock_state.delay_calls = 0;
    clock_state.delayed_ms = 0;
    NoteGetMs_fake.custom_fake = detail::noteGetMs;
    NoteDelayMs_fake.custom_fake = detail::noteDelayMs;
    millis_fake.custom_fake = detail::millis;
    delay_fake.custom_fake = detail::delay;
}

}  // namespace fake_clock

#endif  // FAKE_CLOCK_HPP
