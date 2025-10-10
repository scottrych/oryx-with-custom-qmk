#pragma once

// Bridge header for Achordion integration
// This file ensures compatibility with keymap.c while using the getreuer module

#include "modules/getreuer/achordion/achordion.h"

// Re-export all the functions we need
// These are already defined in the getreuer module, so we just need to make sure
// they're accessible with the expected names

// Main Achordion function (already defined in getreuer module)
// bool process_record_achordion(uint16_t keycode, keyrecord_t* record);

// Housekeeping task (already defined in getreuer module)  
// void housekeeping_task_achordion(void);

// Callback functions (already defined in getreuer module)
// bool achordion_chord(uint16_t tap_hold_keycode, keyrecord_t* tap_hold_record,
//                      uint16_t other_keycode, keyrecord_t* other_record);
// uint16_t achordion_timeout(uint16_t tap_hold_keycode);
// bool achordion_eager_mod(uint8_t mod);
// bool achordion_opposite_hands(const keyrecord_t* tap_hold_record,
//                               const keyrecord_t* other_record);

// All functions are provided by the getreuer module - this header just ensures
// the include structure works correctly for your keymap.c