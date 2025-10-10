// test_achordion.c — Unit tests for Achordion implementation
// Test framework designed for QMK environment

// Define testing flag before includes
#define ACHORDION_TESTING

#include "quantum.h"
#include "achordion.h"
#include "achordion_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

// Mock variables for testing
static uint16_t mock_timer = 0;
static bool mock_process_record_called = false;
static keyrecord_t mock_processed_record;
static uint16_t mock_processed_keycode;

// Mock timer functions
uint16_t timer_read(void) {
    return mock_timer;
}

bool timer_expired(uint16_t current, uint16_t target) {
    return current >= target;
}

// Mock process_record function to capture calls
void process_record(keyrecord_t* record) {
    mock_process_record_called = true;
    mock_processed_record = *record;
    // For testing, we also need to capture the keycode
    // In real QMK, this would be handled differently
}

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
    memset(&mock_processed_record, 0, sizeof(mock_processed_record));
    mock_processed_keycode = KC_NO;
}

// Reset achordion state for testing
void reset_achordion_state(void) {
    reset_achordion_state_for_testing();
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
    mock_timer = 100;
    
    bool result1 = process_record_achordion(keycode, &press_record);
    TEST_ASSERT(!result1, "Tap-hold key press should be intercepted");
    TEST_ASSERT(achordion_state == STATE_UNSETTLED, "Should enter unsettled state");
    
    // Simulate time passing beyond timeout (default is 1000ms)
    mock_timer = 1200; // 1200ms > 100ms + 1000ms timeout
    
    // Call housekeeping task to trigger timeout
    housekeeping_task_achordion();
    
    TEST_ASSERT(achordion_state == STATE_RELEASED, "Should settle as hold and return to released state");
    TEST_ASSERT(mock_process_record_called, "Should have called process_record for hold action");
    TEST_ASSERT(tap_hold_keycode == KC_NO, "Should clear stored keycode after settling");
}

// Test Case 3: achordion_opposite_hands correctly identifies keys pressed on different hands
void test_opposite_hands_detection(void) {
    printf("\n=== Test Case 3: Opposite Hands Detection ===\n");
    
    // Left hand key (column 0, assuming split keyboard)
    keyrecord_t left_record = create_keyrecord(KC_A, true, 0, 2, 100);
    
    // Right hand key (column 6, assuming 6-column split)
    keyrecord_t right_record = create_keyrecord(KC_J, true, 6, 2, 100);
    
    // Same hand keys
    keyrecord_t left_record2 = create_keyrecord(KC_S, true, 1, 2, 100);
    
    // Test opposite hands
    bool opposite1 = achordion_opposite_hands(&left_record, &right_record);
    TEST_ASSERT(opposite1, "Should detect opposite hands (left col 0 vs right col 6)");
    
    bool opposite2 = achordion_opposite_hands(&right_record, &left_record);
    TEST_ASSERT(opposite2, "Should detect opposite hands (right col 6 vs left col 0)");
    
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
    
    // Press the tap-hold key (left hand)
    keyrecord_t tap_hold_press = create_tap_hold_record(tap_hold_keycode_val, true, 0, 2, 100);
    bool result1 = process_record_achordion(tap_hold_keycode_val, &tap_hold_press);
    
    TEST_ASSERT(!result1, "Tap-hold key press should be intercepted");
    TEST_ASSERT(achordion_state == STATE_UNSETTLED, "Should enter unsettled state");
    
    // Press another key on opposite hand (right hand)
    uint16_t other_keycode = KC_J;
    keyrecord_t other_press = create_keyrecord(other_keycode, true, 6, 2, 150);
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
    
    // Press the tap-hold key (left hand)
    keyrecord_t tap_hold_press = create_tap_hold_record(tap_hold_keycode_val, true, 0, 2, 100);
    bool result1 = process_record_achordion(tap_hold_keycode_val, &tap_hold_press);
    
    TEST_ASSERT(!result1, "Tap-hold key press should be intercepted");
    TEST_ASSERT(achordion_state == STATE_UNSETTLED, "Should enter unsettled state");
    
    // Press another key on same hand (left hand)
    uint16_t other_keycode = KC_S;
    keyrecord_t other_press = create_keyrecord(other_keycode, true, 1, 2, 150);
    bool result2 = process_record_achordion(other_keycode, &other_press);
    
    // Should settle as tap due to same hand condition
    TEST_ASSERT(!result2, "Other key press should be intercepted during settlement");
    TEST_ASSERT(achordion_state == STATE_RELEASED, "Should settle and return to released state");
    TEST_ASSERT(mock_process_record_called, "Should have processed the tap action");
    TEST_ASSERT(tap_hold_keycode == KC_NO, "Should clear stored keycode after settling");
    
    // Verify that the tap action was processed correctly
    TEST_ASSERT(mock_processed_record.tap.count == 1, "Should have set tap count to 1");
    TEST_ASSERT(mock_processed_record.tap.interrupted == true, "Should have marked tap as interrupted");
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
    
    // Press the layer tap key
    keyrecord_t press_record = create_tap_hold_record(keycode, true, 2, 4, 100);
    bool result1 = process_record_achordion(keycode, &press_record);
    
    TEST_ASSERT(!result1, "Layer tap key press should be intercepted");
    TEST_ASSERT(achordion_state == STATE_UNSETTLED, "Should enter unsettled state");
    
    // Press key on opposite hand
    uint16_t other_keycode = KC_J;
    keyrecord_t other_press = create_keyrecord(other_keycode, true, 6, 2, 150);
    bool result2 = process_record_achordion(other_keycode, &other_press);
    
    TEST_ASSERT(!result2, "Should handle layer tap with chording");
    TEST_ASSERT(achordion_state == STATE_RELEASED, "Should settle layer tap");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Runner
// ─────────────────────────────────────────────────────────────────────────────

void run_all_tests(void) {
    printf("=== Achordion Unit Tests ===\n");
    printf("Testing Achordion implementation for QMK Voyager keymap\n\n");
    
    // Run all test cases
    test_quick_tap_registration();
    test_timeout_hold_registration();
    test_opposite_hands_detection();
    test_chording_condition_hold();
    test_chording_condition_tap();
    test_non_tap_hold_passthrough();
    test_layer_tap_behavior();
    
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

// QMK integration - call this from your keymap if running tests in QMK environment
void achordion_run_tests(void) {
    run_all_tests();
}