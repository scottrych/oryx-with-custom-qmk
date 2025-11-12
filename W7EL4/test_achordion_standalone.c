// test_achordion_standalone.c — Standalone Achordion unit tests
// This file contains all necessary mocks and can be compiled independently

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// ─────────────────────────────────────────────────────────────────────────────
// QMK Mock Types and Constants
// ─────────────────────────────────────────────────────────────────────────────

// Mock QMK types
typedef struct {
    uint8_t col;
    uint8_t row;
} keypos_t;

typedef struct {
    keypos_t key;
    bool pressed;
    uint16_t time;
} keyevent_t;

typedef struct {
    uint8_t count;
    bool interrupted;
} tap_t;

typedef struct {
    keyevent_t event;
    tap_t tap;
} keyrecord_t;

// Mock QMK constants
#define KC_NO 0
#define KC_A 4
#define KC_S 22
#define KC_J 13
#define KC_TAB 43
#define KC_Z 29
#define KC_1 30
#define KC_0 39
#define KC_MINS 45
#define KC_UNDS 46
#define KC_BSPC 42
#define KC_DEL 76
#define KC_SPC 44

#define MOD_LCTL 0x01
#define MOD_LALT 0x02
#define MOD_LGUI 0x04
#define MOD_LSFT 0x08

#define KC_LSFT 0xE1

// Mock QMK macros
#define QK_MOD_TAP 0x4000
#define QK_LAYER_TAP 0x4800
#define IS_QK_MOD_TAP(kc) (((kc) & 0xFF00) == 0x4000)
#define IS_QK_LAYER_TAP(kc) (((kc) & 0xFF00) == 0x4800)
#define IS_KEYEVENT(event) true
#define MT(mod, kc) (0x4000 | ((kc) & 0xFF))
#define LT(layer, kc) (0x4800 | ((kc) & 0xFF))

#define MATRIX_ROWS 12
#define MATRIX_COLS 7
#define SPLIT_KEYBOARD

// ─────────────────────────────────────────────────────────────────────────────
// Embedded Achordion Implementation (simplified for testing)
// ─────────────────────────────────────────────────────────────────────────────

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

// Mock timer functions
static uint16_t mock_timer = 0;

uint16_t timer_read(void) {
    return mock_timer;
}

bool timer_expired(uint16_t current, uint16_t target) {
    return current >= target;
}

void set_mock_timer(uint16_t time) {
    mock_timer = time;
}

// Mock process_record function
static bool mock_process_record_called = false;
static keyrecord_t mock_processed_record;
static keyrecord_t mock_tap_press_record;  // Capture the tap press specifically
static bool mock_tap_press_captured = false;

void process_record(keyrecord_t* record) {
    mock_process_record_called = true;
    mock_processed_record = *record;
    
    // Capture tap press events (with tap.count > 0 and pressed = true)
    if (record->tap.count > 0 && record->event.pressed) {
        mock_tap_press_record = *record;
        mock_tap_press_captured = true;
    }
}

// Mock weak mods tracking for Caps Word
static uint8_t mock_weak_mods = 0;

void add_weak_mods(uint8_t mods) {
    mock_weak_mods |= mods;
}

uint8_t get_weak_mods(void) {
    return mock_weak_mods;
}

void clear_weak_mods(void) {
    mock_weak_mods = 0;
}

#define MOD_BIT(kc) (1 << ((kc) & 0x0F))

// Caps Word implementation (from keymap.c)
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
bool achordion_chord(uint16_t tap_hold_keycode, keyrecord_t* tap_hold_record,
                     uint16_t other_keycode, keyrecord_t* other_record) {
  return achordion_opposite_hands(tap_hold_record, other_record);
}

// Default timeout
uint16_t achordion_timeout(uint16_t tap_hold_keycode) {
  return 1000;
}

// Main processing function
bool process_record_achordion(uint16_t keycode, keyrecord_t* record) {
  // Don't process events that Achordion generated
  if (achordion_state == STATE_RECURSING) {
    return true;
  }

  const bool is_tap_hold = IS_QK_MOD_TAP(keycode) || IS_QK_LAYER_TAP(keycode);
  const bool is_key_event = IS_KEYEVENT(record->event);
  
  // Debug output for test troubleshooting
  #ifdef DEBUG_ACHORDION
  printf("DEBUG: keycode=0x%04X, is_tap_hold=%d, is_mod_tap=%d, is_layer_tap=%d\n", 
         keycode, is_tap_hold, IS_QK_MOD_TAP(keycode), IS_QK_LAYER_TAP(keycode));
  #endif

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

// ─────────────────────────────────────────────────────────────────────────────
// Test Infrastructure
// ─────────────────────────────────────────────────────────────────────────────

static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

#define TEST_ASSERT(condition, message) do { \
    test_count++; \
    if (condition) { \
        test_passed++; \
        printf("✓ Test %d: %s\n", test_count, message); \
    } else { \
        test_failed++; \
        printf("✗ Test %d: %s\n", test_count, message); \
    } \
} while(0)

// Helper functions to create test records
keyrecord_t create_keyrecord(uint16_t keycode, bool pressed, uint8_t col, uint8_t row, uint16_t time) {
    keyrecord_t record = {0};
    record.event.key.col = col;
    record.event.key.row = row;
    record.event.pressed = pressed;
    record.event.time = time;
    record.tap.count = 0;
    record.tap.interrupted = false;
    return record;
}

keyrecord_t create_tap_hold_record(uint16_t keycode, bool pressed, uint8_t col, uint8_t row, uint16_t time) {
    keyrecord_t record = create_keyrecord(keycode, pressed, col, row, time);
    record.tap.count = 0;  // tap-hold keys start with 0 tap count
    return record;
}

// Mock reset function
void reset_mocks(void) {
    mock_timer = 0;
    mock_process_record_called = false;
    mock_tap_press_captured = false;
    memset(&mock_processed_record, 0, sizeof(mock_processed_record));
    memset(&mock_tap_press_record, 0, sizeof(mock_tap_press_record));
    clear_weak_mods();
}

// Reset achordion state for testing
void reset_achordion_state(void) {
    achordion_state = STATE_RELEASED;
    tap_hold_keycode = KC_NO;
    hold_timer = 0;
    pressed_another_key_before_release = false;
    memset(&tap_hold_record, 0, sizeof(tap_hold_record));
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Cases
// ─────────────────────────────────────────────────────────────────────────────

// Test Case 1: process_record_achordion correctly registers a tap when 
// a tap-hold key is pressed and released quickly
void test_quick_tap_registration(void) {
    printf("\n=== Test Case 1: Quick Tap Registration ===\n");
    
    reset_mocks();
    reset_achordion_state();
    
    // Create a mod-tap key (Ctrl+A) on left hand
    uint16_t keycode = MT(MOD_LCTL, KC_A);
    
    // Press the key
    keyrecord_t press_record = create_tap_hold_record(keycode, true, 0, 2, 100);
    bool result1 = process_record_achordion(keycode, &press_record);
    
    // Should be intercepted (return false) and enter unsettled state
    TEST_ASSERT(!result1, "Tap-hold key press should be intercepted");
    TEST_ASSERT(achordion_state == STATE_UNSETTLED, "Should enter unsettled state");
    TEST_ASSERT(tap_hold_keycode == keycode, "Should store the tap-hold keycode");
    
    // Release the key quickly (before timeout)
    keyrecord_t release_record = create_tap_hold_record(keycode, false, 0, 2, 150);
    bool result2 = process_record_achordion(keycode, &release_record);
    
    // Should be intercepted and state should reset
    TEST_ASSERT(!result2, "Tap-hold key release should be intercepted");
    TEST_ASSERT(achordion_state == STATE_RELEASED, "Should return to released state");
    TEST_ASSERT(tap_hold_keycode == KC_NO, "Should clear stored keycode");
}

// Test Case 2: process_record_achordion correctly registers a hold when 
// a tap-hold key is held beyond its timeout
void test_timeout_hold_registration(void) {
    printf("\n=== Test Case 2: Timeout Hold Registration ===\n");
    
    reset_mocks();
    reset_achordion_state();
    
    // Create a mod-tap key (Ctrl+A)
    uint16_t keycode = MT(MOD_LCTL, KC_A);
    
    // Press the key
    keyrecord_t press_record = create_tap_hold_record(keycode, true, 0, 2, 100);
    set_mock_timer(100);
    
    bool result1 = process_record_achordion(keycode, &press_record);
    TEST_ASSERT(!result1, "Tap-hold key press should be intercepted");
    TEST_ASSERT(achordion_state == STATE_UNSETTLED, "Should enter unsettled state");
    
    // Simulate time passing beyond timeout (default is 1000ms)
    set_mock_timer(1200); // 1200ms > 100ms + 1000ms timeout
    
    // Call housekeeping task to trigger timeout
    housekeeping_task_achordion();
    
    TEST_ASSERT(achordion_state == STATE_RELEASED, "Should settle as hold and return to released state");
    TEST_ASSERT(mock_process_record_called, "Should have called process_record for hold action");
    TEST_ASSERT(tap_hold_keycode == KC_NO, "Should clear stored keycode after settling");
}

// Test Case 3: achordion_opposite_hands correctly identifies keys pressed on different hands
void test_opposite_hands_detection(void) {
    printf("\n=== Test Case 3: Opposite Hands Detection ===\n");
    
    // Left hand key (row 0, assuming split keyboard with 6 rows per side)
    keyrecord_t left_record = create_keyrecord(KC_A, true, 0, 2, 100);
    
    // Right hand key (row 6, assuming 6 rows per side for split)
    keyrecord_t right_record = create_keyrecord(KC_J, true, 0, 8, 100);
    
    // Same hand keys
    keyrecord_t left_record2 = create_keyrecord(KC_S, true, 1, 2, 100);
    
    // Test opposite hands
    bool opposite1 = achordion_opposite_hands(&left_record, &right_record);
    TEST_ASSERT(opposite1, "Should detect opposite hands (left row 2 vs right row 8)");
    
    bool opposite2 = achordion_opposite_hands(&right_record, &left_record);
    TEST_ASSERT(opposite2, "Should detect opposite hands (right row 8 vs left row 2)");
    
    // Test same hand
    bool same_hand = achordion_opposite_hands(&left_record, &left_record2);
    TEST_ASSERT(!same_hand, "Should detect same hand (both left side)");
    
    // Test same key
    bool same_key = achordion_opposite_hands(&left_record, &left_record);
    TEST_ASSERT(!same_key, "Should detect same key");
}

// Test Case 4: process_record_achordion correctly settles a tap-hold key as a hold 
// when a chording condition is met (opposite hands)
void test_chording_condition_hold(void) {
    printf("\n=== Test Case 4: Chording Condition - Hold ===\n");
    
    reset_mocks();
    reset_achordion_state();
    
    // Create a mod-tap key (Ctrl+A) on left hand
    uint16_t tap_hold_keycode_val = MT(MOD_LCTL, KC_A);
    
    // Press the tap-hold key (left hand, row 2)
    keyrecord_t tap_hold_press = create_tap_hold_record(tap_hold_keycode_val, true, 0, 2, 100);
    bool result1 = process_record_achordion(tap_hold_keycode_val, &tap_hold_press);
    
    TEST_ASSERT(!result1, "Tap-hold key press should be intercepted");
    TEST_ASSERT(achordion_state == STATE_UNSETTLED, "Should enter unsettled state");
    
    // Press another key on opposite hand (right hand, row 8)
    uint16_t other_keycode = KC_J;
    keyrecord_t other_press = create_keyrecord(other_keycode, true, 0, 8, 150);
    bool result2 = process_record_achordion(other_keycode, &other_press);
    
    // Should settle as hold due to opposite hands condition
    TEST_ASSERT(!result2, "Other key press should be intercepted during settlement");
    TEST_ASSERT(achordion_state == STATE_RELEASED, "Should settle and return to released state");
    TEST_ASSERT(mock_process_record_called, "Should have processed the hold action");
    TEST_ASSERT(tap_hold_keycode == KC_NO, "Should clear stored keycode after settling");
}

// Test Case 5: process_record_achordion correctly settles a tap-hold key as a tap 
// when a chording condition is not met (same hand)
void test_chording_condition_tap(void) {
    printf("\n=== Test Case 5: Chording Condition - Tap ===\n");
    
    reset_mocks();
    reset_achordion_state();
    
    // Create a mod-tap key (Ctrl+A) on left hand
    uint16_t tap_hold_keycode_val = MT(MOD_LCTL, KC_A);
    
    // Press the tap-hold key (left hand, row 2)
    keyrecord_t tap_hold_press = create_tap_hold_record(tap_hold_keycode_val, true, 0, 2, 100);
    bool result1 = process_record_achordion(tap_hold_keycode_val, &tap_hold_press);
    
    TEST_ASSERT(!result1, "Tap-hold key press should be intercepted");
    TEST_ASSERT(achordion_state == STATE_UNSETTLED, "Should enter unsettled state");
    
    // Press another key on same hand (left hand, row 3)
    uint16_t other_keycode = KC_S;
    keyrecord_t other_press = create_keyrecord(other_keycode, true, 1, 3, 150);
    bool result2 = process_record_achordion(other_keycode, &other_press);
    
    // Should settle as tap due to same hand condition
    TEST_ASSERT(!result2, "Other key press should be intercepted during settlement");
    TEST_ASSERT(achordion_state == STATE_RELEASED, "Should settle and return to released state");
    TEST_ASSERT(mock_process_record_called, "Should have processed the tap action");
    TEST_ASSERT(tap_hold_keycode == KC_NO, "Should clear stored keycode after settling");
    
    // Verify that the tap action was processed correctly
    TEST_ASSERT(mock_tap_press_captured, "Should have captured tap press event");
    TEST_ASSERT(mock_tap_press_record.tap.count == 1, "Should have set tap count to 1");
    TEST_ASSERT(mock_tap_press_record.tap.interrupted == true, "Should have marked tap as interrupted");
}

// Additional test: Non-tap-hold keys should pass through
void test_non_tap_hold_passthrough(void) {
    printf("\n=== Additional Test: Non-Tap-Hold Passthrough ===\n");
    
    reset_mocks();
    reset_achordion_state();
    
    // Regular key press
    uint16_t keycode = KC_A;
    keyrecord_t press_record = create_keyrecord(keycode, true, 0, 2, 100);
    
    bool result = process_record_achordion(keycode, &press_record);
    
    TEST_ASSERT(result, "Non-tap-hold keys should pass through (return true)");
    TEST_ASSERT(achordion_state == STATE_RELEASED, "State should remain released");
    TEST_ASSERT(tap_hold_keycode == KC_NO, "Should not store regular keycodes");
}

// Additional test: Layer tap keys behavior
void test_layer_tap_behavior(void) {
    printf("\n=== Additional Test: Layer Tap Behavior ===\n");
    
    reset_mocks();
    reset_achordion_state();
    
    // Layer tap key (Layer 1 + Tab)
    uint16_t keycode = LT(1, KC_TAB);
    
    // Press the layer tap key (left hand, row 4)
    keyrecord_t press_record = create_tap_hold_record(keycode, true, 2, 4, 100);
    bool result1 = process_record_achordion(keycode, &press_record);
    
    TEST_ASSERT(!result1, "Layer tap key press should be intercepted");
    TEST_ASSERT(achordion_state == STATE_UNSETTLED, "Should enter unsettled state");
    
    // Press key on opposite hand (right hand, row 8)
    uint16_t other_keycode = KC_J;
    keyrecord_t other_press = create_keyrecord(other_keycode, true, 0, 8, 150);
    bool result2 = process_record_achordion(other_keycode, &other_press);
    
    TEST_ASSERT(!result2, "Should handle layer tap with chording");
    TEST_ASSERT(achordion_state == STATE_RELEASED, "Should settle layer tap");
}

// Test Case 6: achordion_chord() allows holds only when keys are pressed on opposite hands
void test_achordion_chord_opposite_hands_allows_hold(void) {
    printf("\n=== Test Case 6: achordion_chord() - Opposite Hands Allow Hold ===\n");
    
    uint16_t tap_hold_kc = MT(MOD_LCTL, KC_A);
    keyrecord_t tap_hold_rec = create_keyrecord(KC_A, true, 0, 2, 100);
    
    uint16_t other_kc = KC_J;
    keyrecord_t other_rec = create_keyrecord(KC_J, true, 0, 8, 150);
    
    bool result = achordion_chord(tap_hold_kc, &tap_hold_rec, other_kc, &other_rec);
    
    TEST_ASSERT(result, "achordion_chord() should return true (allow hold) for opposite hands");
}

// Test Case 7: achordion_chord() forces taps when keys are pressed on same hand
void test_achordion_chord_same_hand_forces_tap(void) {
    printf("\n=== Test Case 7: achordion_chord() - Same Hand Forces Tap ===\n");
    
    uint16_t tap_hold_kc = MT(MOD_LCTL, KC_A);
    keyrecord_t tap_hold_rec = create_keyrecord(KC_A, true, 0, 2, 100);
    
    uint16_t other_kc = KC_S;
    keyrecord_t other_rec = create_keyrecord(KC_S, true, 1, 3, 150);
    
    bool result = achordion_chord(tap_hold_kc, &tap_hold_rec, other_kc, &other_rec);
    
    TEST_ASSERT(!result, "achordion_chord() should return false (force tap) for same hand");
}

// Test Case 8: caps_word_press_user() continues Caps Word and applies shift for alphabetical keys (A-Z)
void test_caps_word_alphabetical_keys_with_shift(void) {
    printf("\n=== Test Case 8: caps_word_press_user() - Alphabetical Keys (A-Z) ===\n");
    
    reset_mocks();
    
    // Test lowercase letter 'a'
    bool result_a = caps_word_press_user(KC_A);
    TEST_ASSERT(result_a, "caps_word_press_user(KC_A) should return true (continue Caps Word)");
    TEST_ASSERT(get_weak_mods() != 0, "Should apply shift for KC_A");
    
    clear_weak_mods();
    
    // Test lowercase letter 'z'
    bool result_z = caps_word_press_user(KC_Z);
    TEST_ASSERT(result_z, "caps_word_press_user(KC_Z) should return true (continue Caps Word)");
    TEST_ASSERT(get_weak_mods() != 0, "Should apply shift for KC_Z");
    
    clear_weak_mods();
    
    // Test lowercase letter 's'
    bool result_s = caps_word_press_user(KC_S);
    TEST_ASSERT(result_s, "caps_word_press_user(KC_S) should return true (continue Caps Word)");
    TEST_ASSERT(get_weak_mods() != 0, "Should apply shift for KC_S");
}

// Test Case 9: caps_word_press_user() continues Caps Word without shift for numerical keys (0-9)
void test_caps_word_numerical_keys_without_shift(void) {
    printf("\n=== Test Case 9: caps_word_press_user() - Numerical Keys (0-9) ===\n");
    
    reset_mocks();
    
    // Test digit '1'
    bool result_1 = caps_word_press_user(KC_1);
    TEST_ASSERT(result_1, "caps_word_press_user(KC_1) should return true (continue Caps Word)");
    TEST_ASSERT(get_weak_mods() == 0, "Should NOT apply shift for KC_1");
    
    // Test digit '0'
    bool result_0 = caps_word_press_user(KC_0);
    TEST_ASSERT(result_0, "caps_word_press_user(KC_0) should return true (continue Caps Word)");
    TEST_ASSERT(get_weak_mods() == 0, "Should NOT apply shift for KC_0");
}

// Test Case 10: caps_word_press_user() continues Caps Word for special allowed keys
void test_caps_word_special_allowed_keys(void) {
    printf("\n=== Test Case 10: caps_word_press_user() - Special Allowed Keys ===\n");
    
    reset_mocks();
    
    // Test minus (for snake_case) - should apply shift
    bool result_mins = caps_word_press_user(KC_MINS);
    TEST_ASSERT(result_mins, "caps_word_press_user(KC_MINS) should return true (continue Caps Word)");
    TEST_ASSERT(get_weak_mods() != 0, "Should apply shift for KC_MINS");
    
    clear_weak_mods();
    
    // Test underscore (already shifted) - should NOT apply shift
    bool result_unds = caps_word_press_user(KC_UNDS);
    TEST_ASSERT(result_unds, "caps_word_press_user(KC_UNDS) should return true (continue Caps Word)");
    TEST_ASSERT(get_weak_mods() == 0, "Should NOT apply shift for KC_UNDS");
    
    // Test backspace - should NOT apply shift
    bool result_bspc = caps_word_press_user(KC_BSPC);
    TEST_ASSERT(result_bspc, "caps_word_press_user(KC_BSPC) should return true (continue Caps Word)");
    TEST_ASSERT(get_weak_mods() == 0, "Should NOT apply shift for KC_BSPC");
    
    // Test delete - should NOT apply shift
    bool result_del = caps_word_press_user(KC_DEL);
    TEST_ASSERT(result_del, "caps_word_press_user(KC_DEL) should return true (continue Caps Word)");
    TEST_ASSERT(get_weak_mods() == 0, "Should NOT apply shift for KC_DEL");
}

// Test Case 11: caps_word_press_user() terminates Caps Word for non-specified keys
void test_caps_word_terminates_for_other_keys(void) {
    printf("\n=== Test Case 11: caps_word_press_user() - Terminates for Non-Specified Keys ===\n");
    
    reset_mocks();
    
    // Test space - should terminate
    bool result_spc = caps_word_press_user(KC_SPC);
    TEST_ASSERT(!result_spc, "caps_word_press_user(KC_SPC) should return false (terminate Caps Word)");
    TEST_ASSERT(get_weak_mods() == 0, "Should NOT apply shift for KC_SPC");
    
    // Test tab - should terminate
    bool result_tab = caps_word_press_user(KC_TAB);
    TEST_ASSERT(!result_tab, "caps_word_press_user(KC_TAB) should return false (terminate Caps Word)");
    TEST_ASSERT(get_weak_mods() == 0, "Should NOT apply shift for KC_TAB");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Runner
// ─────────────────────────────────────────────────────────────────────────────

void run_all_tests(void) {
    printf("=== Achordion Unit Tests ===\n");
    printf("Testing Achordion implementation for QMK Voyager keymap\n");
    printf("Split keyboard configuration: %d rows, %d cols\n\n", MATRIX_ROWS, MATRIX_COLS);
    
    // Run all test cases
    test_quick_tap_registration();
    test_timeout_hold_registration();
    test_opposite_hands_detection();
    test_chording_condition_hold();
    test_chording_condition_tap();
    test_non_tap_hold_passthrough();
    test_layer_tap_behavior();
    test_achordion_chord_opposite_hands_allows_hold();
    test_achordion_chord_same_hand_forces_tap();
    test_caps_word_alphabetical_keys_with_shift();
    test_caps_word_numerical_keys_without_shift();
    test_caps_word_special_allowed_keys();
    test_caps_word_terminates_for_other_keys();
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", test_count);
    printf("Passed: %d\n", test_passed);
    printf("Failed: %d\n", test_failed);
    printf("Success rate: %.1f%%\n", test_count > 0 ? (float)test_passed / test_count * 100 : 0);
    
    if (test_failed == 0) {
        printf("🎉 All tests passed!\n");
    } else {
        printf("❌ Some tests failed. Please review implementation.\n");
    }
}

// Main function for standalone testing
int main(void) {
    run_all_tests();
    return test_failed == 0 ? 0 : 1;
}