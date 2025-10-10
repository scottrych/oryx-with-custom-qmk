// achordion_test.h — Test-specific declarations for Achordion unit tests
// This header exposes internal state variables for testing purposes

#pragma once

#include "achordion.h"

#ifdef ACHORDION_TESTING

// Internal state enum (copy from achordion.c)
enum achordion_state_t {
  STATE_RELEASED,
  STATE_UNSETTLED,
  STATE_RECURSING,
};

// Expose internal state variables for testing
extern enum achordion_state_t achordion_state;
extern keyrecord_t tap_hold_record;
extern uint16_t tap_hold_keycode;
extern uint16_t hold_timer;
extern bool pressed_another_key_before_release;

// Test helper functions
void reset_achordion_state_for_testing(void);

#endif // ACHORDION_TESTING