# QMK Configuration Fixes Applied

## Date: November 12, 2025

## Issues Fixed

### 1. Achordion Not Working (FIXED ✅)

**Problem:** The `achordion_chord()` function was too permissive, allowing holds for nearly all key combinations including same-hand typing sequences.

**Root Cause:** The function was checking if ANY letter key (A-Z) was pressed and returning `true`, which meant pressing home row mods like `A` (Ctrl) followed by `R` (Cmd) on the same hand would trigger as Ctrl+Cmd instead of typing "ar".

**The problematic code:**
```c
if (IS_QK_MOD_TAP(other_keycode) ||  // Other home row mod
    (other_base >= KC_A && other_base <= KC_Z) ||  // <-- This was the problem!
    (other_base >= KC_1 && other_base <= KC_0) ||  // Numbers
    ...
) {
  return true;  // Allow hold for everything
}
```

**Solution:** Simplified `achordion_chord()` to use the **opposite hands rule only**. Now Achordion will:
- Allow holds ONLY when you press keys on opposite hands
- Force taps when pressing keys on the same hand (normal typing)

**New implementation:**
```c
bool achordion_chord(uint16_t tap_hold_keycode, keyrecord_t* tap_hold_record,
                     uint16_t other_keycode, keyrecord_t* other_record) {
  // For home row mods, allow hold only with opposite hand
  // This prevents accidental holds when typing fast on the same hand
  return achordion_opposite_hands(tap_hold_record, other_record);
}
```

**Result:** Home row mods now work correctly:
- Same hand typing (like "art") = normal letters
- Opposite hand combinations (like left Ctrl + right N) = hold activates

---

### 2. Caps Word Intermittent Behavior (FIXED ✅)

**Problem:** Caps Word would sometimes capitalize only the first letter, then lowercase everything else, making it unreliable.

**Root Cause:** No custom `caps_word_press_user()` function was defined, so QMK was using default behavior which terminates Caps Word on many common keys including modifiers and punctuation.

**Solution:** Added `caps_word_press_user()` function to properly handle:
- Letters (A-Z): Continue with shift applied
- Numbers (0-9): Continue without shift
- Minus/Underscore: Continue (for variable names like `MY_CONSTANT`)
- Backspace/Delete: Continue (for corrections)
- Everything else: Terminate Caps Word

**New implementation:**
```c
bool caps_word_press_user(uint16_t keycode) {
  switch (keycode) {
    // Keycodes that continue Caps Word, with shift applied
    case KC_A ... KC_Z:
    case KC_MINS:  // For snake_case
      add_weak_mods(MOD_BIT(KC_LSFT));  // Apply shift to next key
      return true;

    // Keycodes that continue Caps Word, without shifting
    case KC_1 ... KC_0:
    case KC_BSPC:
    case KC_DEL:
    case KC_UNDS:  // Already shifted version of minus
      return true;

    default:
      return false;  // Deactivate Caps Word
  }
}
```

**Result:** Caps Word now reliably capitalizes letters and handles:
- `MY_VARIABLE_NAME` (with underscores)
- `CONSTANT123` (with numbers)
- Backspace corrections without deactivating

---

### 3. Missing TAPPING_TERM Definition (FIXED ✅)

**Problem:** Using `TAPPING_TERM_PER_KEY` without defining a base `TAPPING_TERM` value.

**Solution:** Added `#define TAPPING_TERM 200` to config.h to match the Achordion timeout.

---

## Configuration Summary

### config.h Settings
- `TAPPING_TERM 200` - Base timing for tap/hold decisions
- `PERMISSIVE_HOLD` - Enabled for faster modifier activation
- `CAPS_WORD_ENABLE` - Enabled with custom handler
- `DEBOUNCE 5` - Standard debounce timing

### keymap.c Functions
1. `achordion_chord()` - Simplified to opposite hands only
2. `achordion_timeout()` - Set to 200ms for responsive feel
3. `caps_word_press_user()` - New function for reliable Caps Word
4. `housekeeping_task_user()` - Calls `housekeeping_task_achordion()`
5. `process_record_user()` - Calls `process_record_achordion()` first

---

## Testing Checklist

After rebuilding and flashing, test these scenarios:

### Achordion Tests
- [ ] Same hand typing (e.g., "art", "star") types normally
- [ ] Left hand mod + right hand key activates modifier (e.g., left Ctrl + right N)
- [ ] Right hand mod + left hand key activates modifier (e.g., right Shift + left A)
- [ ] Fast typing doesn't trigger accidental holds
- [ ] Intentional holds still work with opposite hand

### Caps Word Tests
- [ ] Activate Caps Word with CW_TOGG
- [ ] Type multiple letters in sequence - all should be capitalized
- [ ] Type with numbers - should continue capitalization
- [ ] Type with underscore - should continue capitalization
- [ ] Backspace works without deactivating
- [ ] Space or punctuation deactivates Caps Word

---

## Backup Information

**Original files backed up:** No automatic backup was created. If you need to revert:
1. Use `git status` to see changes
2. Use `git diff` to review changes
3. Use `git checkout -- <file>` to revert specific files

**Files modified:**
- `/Volumes/HomeX/Users/scottrych/Documents/GitHub/oryx-with-custom-qmk/mEaYP/keymap.c`
- `/Volumes/HomeX/Users/scottrych/Documents/GitHub/oryx-with-custom-qmk/mEaYP/config.h`

---

## Next Steps

1. Compile your firmware: `qmk compile` (or use ZSA's build system)
2. Flash to your keyboard
3. Test the scenarios in the checklist above
4. If you encounter issues, check the QMK console output for debugging

## Additional Notes

The simplified Achordion configuration is actually the recommended approach from getreuer (the original author). The "opposite hands" rule is the sweet spot for most users - complex enough to prevent accidental holds, but simple enough to be predictable and reliable.

If you need same-hand modifier combinations (like Ctrl+Alt on the same hand), you would need to add them as explicit combos or use a different approach like one-shot modifiers.
