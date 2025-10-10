#pragma once

#include "quantum.h"

#ifdef __cplusplus
extern "C" {
#endif

// Main Achordion processing function
bool process_record_achordion(uint16_t keycode, keyrecord_t* record);

// Housekeeping task for timeouts
void housekeeping_task_achordion(void);

// Callback functions (can be overridden in keymap.c)
bool achordion_chord(uint16_t tap_hold_keycode, keyrecord_t* tap_hold_record,
                     uint16_t other_keycode, keyrecord_t* other_record);

uint16_t achordion_timeout(uint16_t tap_hold_keycode);

bool achordion_eager_mod(uint8_t mod);

// Utility function for opposite hands detection
bool achordion_opposite_hands(const keyrecord_t* tap_hold_record,
                              const keyrecord_t* other_record);

#ifdef __cplusplus
}
#endif
