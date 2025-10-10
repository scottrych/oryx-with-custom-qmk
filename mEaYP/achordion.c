// Simple Achordion implementation for Voyager
// Based on getreuer's achordion but simplified to avoid module system conflicts

#include "achordion.h"

// Internal state tracking
static keyrecord_t tap_hold_record;
static uint16_t tap_hold_keycode = KC_NO;
static uint16_t hold_timer = 0;
static bool pressed_another_key_before_release = false;

enum {
  STATE_RELEASED,
  STATE_UNSETTLED,
  STATE_RECURSING,
} achordion_state = STATE_RELEASED;

// Returns true if `pos` is on the left hand of the keyboard, false if right.
static bool on_left_hand(keypos_t pos) {
#ifdef SPLIT_KEYBOARD
  return pos.row < MATRIX_ROWS / 2;
#else
  return (MATRIX_COLS > MATRIX_ROWS) ? pos.col < MATRIX_COLS / 2
                                     : pos.row < MATRIX_ROWS / 2;
#endif
}

bool achordion_opposite_hands(const keyrecord_t* tap_hold_record,
                              const keyrecord_t* other_record) {
  return on_left_hand(tap_hold_record->event.key) !=
         on_left_hand(other_record->event.key);
}

// Default chord function - hold only if opposite hands
__attribute__((weak)) bool achordion_chord(uint16_t tap_hold_keycode,
                                           keyrecord_t* tap_hold_record,
                                           uint16_t other_keycode,
                                           keyrecord_t* other_record) {
  return achordion_opposite_hands(tap_hold_record, other_record);
}

// Default timeout
__attribute__((weak)) uint16_t achordion_timeout(uint16_t tap_hold_keycode) {
  return 1000;
}

// Default eager mod behavior
__attribute__((weak)) bool achordion_eager_mod(uint8_t mod) {
  return (mod & (MOD_LALT | MOD_LGUI)) == 0;
}

// Main processing function
bool process_record_achordion(uint16_t keycode, keyrecord_t* record) {
  // Don't process events that Achordion generated
  if (achordion_state == STATE_RECURSING) {
    return true;
  }

  const bool is_tap_hold = IS_QK_MOD_TAP(keycode) || IS_QK_LAYER_TAP(keycode);
  const bool is_key_event = IS_KEYEVENT(record->event);

  // Event while no tap-hold key is active
  if (achordion_state == STATE_RELEASED) {
    if (is_tap_hold && record->tap.count == 0 && record->event.pressed && is_key_event) {
      const uint16_t timeout = achordion_timeout(keycode);
      if (timeout > 0) {
        achordion_state = STATE_UNSETTLED;
        tap_hold_keycode = keycode;
        tap_hold_record = *record;
        hold_timer = record->event.time + timeout;
        pressed_another_key_before_release = false;
        return false;  // Skip default handling
      }
    }
    return true;
  }

  // Handle tap-hold key release
  if (keycode == tap_hold_keycode && !record->event.pressed) {
    achordion_state = STATE_RELEASED;
    tap_hold_keycode = KC_NO;
    return false;
  }

  // Handle other key press while tap-hold is unsettled
  if (achordion_state == STATE_UNSETTLED && record->event.pressed && keycode != tap_hold_keycode) {
    pressed_another_key_before_release = true;
    
    if (achordion_chord(tap_hold_keycode, &tap_hold_record, keycode, record)) {
      // Settle as hold
      achordion_state = STATE_RECURSING;
      process_record(&tap_hold_record);
      achordion_state = STATE_RELEASED;
      tap_hold_keycode = KC_NO;
    } else {
      // Settle as tap
      achordion_state = STATE_RECURSING;
      tap_hold_record.event.pressed = true;
      tap_hold_record.tap.count = 1;
      tap_hold_record.tap.interrupted = true;
      process_record(&tap_hold_record);
      
      tap_hold_record.event.pressed = false;
      process_record(&tap_hold_record);
      achordion_state = STATE_RELEASED;
      tap_hold_keycode = KC_NO;
    }
    
    // Re-process the current event
    achordion_state = STATE_RECURSING;
    process_record(record);
    achordion_state = STATE_RELEASED;
    return false;
  }

  return true;
}

// Housekeeping task for timeouts
void housekeeping_task_achordion(void) {
  if (achordion_state == STATE_UNSETTLED &&
      timer_expired(timer_read(), hold_timer)) {
    // Timeout expired, settle as hold
    achordion_state = STATE_RECURSING;
    process_record(&tap_hold_record);
    achordion_state = STATE_RELEASED;
    tap_hold_keycode = KC_NO;
  }
}