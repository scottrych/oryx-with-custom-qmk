#include "achordion.h"

// Internal states
enum {
    STATE_RELEASED,
    STATE_UNSETTLED,
    STATE_RECURSING,
} achordion_state = STATE_RELEASED;

// Storage for active tap-hold key events
static uint16_t tap_hold_keycode = KC_NO;
static keyrecord_t tap_hold_record = {0};
static uint16_t hold_timer = 0;
static bool pressed_another_key_before_release = false;

/**
 * process_achordion: Called from process_record_user
 */
bool process_achordion(uint16_t keycode, keyrecord_t *record) {
    bool is_tap_hold = IS_MOD_TAP(keycode) || IS_LAYER_TAP(keycode);
    bool is_key_event = record->event.pressed;
  
    // If currently no pending tap-hold, and this event is such:
    if (achordion_state == STATE_RELEASED && is_key_event && is_tap_hold) {
        tap_hold_keycode = keycode;
        tap_hold_record = *record;
        hold_timer = record->event.time + achordion_timeout(keycode);
        achordion_state = STATE_UNSETTLED;
        pressed_another_key_before_release = false;
        // Defer letting QMK act on this until resolved
        return true;
    }

    // If unsettled and another key pressed:
    if (achordion_state == STATE_UNSETTLED && is_key_event && keycode != tap_hold_keycode) {
        pressed_another_key_before_release = true;
        if (achordion_chord(tap_hold_keycode, &tap_hold_record, keycode, record)) {
            // resolve as hold
            // send hold event now
            // then continue with record = this other key
        } else {
            // resolve as tap
            // send tap event of tap_hold_keycode
            // then pass this key as normal
        }
        achordion_state = STATE_RECURSING;
    }

    // If in recursing mode — we re-inject events
    if (achordion_state == STATE_RECURSING) {
        // Re-process the record
        process_record(record);
        return false;  // block further default processing
    }

    // Timeout: if no other key was pressed by hold_timer
    if (achordion_state == STATE_UNSETTLED
        && record->event.time > hold_timer
        && !pressed_another_key_before_release) {
        // settle to hold
        achordion_state = STATE_RECURSING;
        // fire hold
        process_record(&tap_hold_record);
        return true;
    }

    // Otherwise, let QMK handle it normally
    return true;
}

/**
 * matrix_scan_user: must call this
 */
void achordion_task(void) {
    if (achordion_state == STATE_UNSETTLED) {
        uint16_t now = timer_read();
        if (now > hold_timer && !pressed_another_key_before_release) {
            // same logic as in process above
            achordion_state = STATE_RECURSING;
            process_record(&tap_hold_record);
        }
    }
}

/** Default implementations of optional overrides **/

bool achordion_chord(uint16_t tap_hold_keycode, keyrecord_t *tap_hold_record,
                     uint16_t other_keycode, keyrecord_t *other_record) {
    // Default: treat chord as “hold” if other key is on the *other hand*
    return on_right_hand(other_record->event.key)
        != on_right_hand(tap_hold_record->event.key);
}

uint16_t achordion_timeout(uint16_t tap_hold_keycode) {
    return 1000;  // default 1 second
}

bool achordion_eager_mod(uint8_t mod) {
    switch (mod) {
        case MOD_LSFT:
        case MOD_RSFT:
        case MOD_LCTL:
        case MOD_RCTL:
            return true;
        default:
            return false;
    }
}
