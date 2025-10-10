#pragma once

#include "quantum.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Handler function for Achordion.
 * Call this from process_record_user:
 *   if (!process_achordion(keycode, record)) return false;
 */
bool process_achordion(uint16_t keycode, keyrecord_t *record);

/** 
 * Call from matrix_scan_user to allow Achordion’s internals to run.
 */
void achordion_task(void);

/** 
 * Optional callback: decide whether a tap-hold key + a second key should
 * settle as a **hold** (true) or **tap** (false).
 *
 * Called: achordion_chord(tap_hold_keycode, tap_hold_record, other_keycode, other_record)
 */
bool achordion_chord(uint16_t tap_hold_keycode, keyrecord_t *tap_hold_record,
                     uint16_t other_keycode, keyrecord_t *other_record);

/** 
 * Optional: customize timeout (ms) for a tap-hold key.
 */
uint16_t achordion_timeout(uint16_t tap_hold_keycode);

/** 
 * Optional: determine if a mod (MOD_LCTL, MOD_LSFT, etc) should have “eager mod” behavior.
 */
bool achordion_eager_mod(uint8_t mod);

#ifdef __cplusplus
}
#endif
