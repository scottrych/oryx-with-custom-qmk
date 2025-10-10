# Achordion Unit Tests

This directory contains comprehensive unit tests for the Achordion implementation in your QMK Voyager keymap. The tests verify that the tap-hold behavior, chording logic, and timing mechanisms work as expected.

## Overview

Achordion is a QMK feature that improves the reliability of home row mods by implementing intelligent tap-hold decision making based on which hand presses subsequent keys. These tests ensure that your implementation correctly:

1. Registers taps when keys are pressed and released quickly
2. Registers holds when keys are held beyond their timeout
3. Detects opposite hand key presses correctly
4. Settles tap-hold keys as holds when chording conditions are met (opposite hands)
5. Settles tap-hold keys as taps when chording conditions are not met (same hand)

## Test Files

### Core Files
- `test_achordion_standalone.c` - **Standalone test suite** (recommended)
- `test_achordion.c` - Modular test suite that uses actual achordion.c
- `achordion_test.h` - Test helper header for exposing internal state
- `run_tests.sh` - Fish shell script to compile and run tests
- `Makefile.test` - Alternative build system for tests

### Documentation
- `TESTING.md` - This file

## Running the Tests

### Quick Start (Recommended)

The easiest way to run the tests is using the standalone test file:

```fish
# Make sure you're in the mEaYP directory
cd /Volumes/HomeX/Users/scottrych/Documents/GitHub/oryx-with-custom-qmk/mEaYP

# Run the tests using the fish script
./run_tests.sh
```

### Alternative Methods

#### Manual Compilation
```fish
# Compile and run manually
gcc -std=c99 -Wall -Wextra -g -o test_achordion_standalone test_achordion_standalone.c
./test_achordion_standalone
rm test_achordion_standalone
```

#### Using Makefile
```fish
# Using the provided Makefile
make -f Makefile.test test
make -f Makefile.test clean
```

## Test Cases Explained

### Test Case 1: Quick Tap Registration
**Purpose**: Verify that tap-hold keys register as taps when pressed and released quickly.

**Test Steps**:
1. Press a mod-tap key (Ctrl+A)
2. Release it quickly (before timeout)
3. Verify the key is intercepted and state management works correctly

**Expected Behavior**:
- Key press should be intercepted (return false)
- State should change to `STATE_UNSETTLED`
- Key release should be intercepted
- State should return to `STATE_RELEASED`

### Test Case 2: Timeout Hold Registration
**Purpose**: Verify that tap-hold keys register as holds when held beyond their timeout.

**Test Steps**:
1. Press a mod-tap key (Ctrl+A)
2. Wait beyond the timeout period (1000ms)
3. Trigger the housekeeping task
4. Verify the hold action is processed

**Expected Behavior**:
- Key should enter unsettled state
- After timeout, `process_record` should be called for hold action
- State should return to released

### Test Case 3: Opposite Hands Detection
**Purpose**: Verify the `achordion_opposite_hands` function correctly identifies keys on different hands.

**Test Steps**:
1. Test keys from left hand (row < 6) vs right hand (row >= 6)
2. Test keys from same hand
3. Test same key against itself

**Expected Behavior**:
- Left hand vs right hand should return `true`
- Same hand keys should return `false`
- Same key should return `false`

### Test Case 4: Chording Condition - Hold
**Purpose**: Verify that tap-hold keys settle as holds when pressed with opposite hand keys.

**Test Steps**:
1. Press mod-tap key on left hand
2. Press regular key on right hand while mod-tap is unsettled
3. Verify hold action is processed

**Expected Behavior**:
- Chord condition should be met (opposite hands)
- Tap-hold key should settle as hold
- Both keys should be processed

### Test Case 5: Chording Condition - Tap
**Purpose**: Verify that tap-hold keys settle as taps when pressed with same hand keys.

**Test Steps**:
1. Press mod-tap key on left hand
2. Press regular key on same (left) hand while mod-tap is unsettled
3. Verify tap action is processed

**Expected Behavior**:
- Chord condition should not be met (same hand)
- Tap-hold key should settle as tap
- Tap should be marked as interrupted
- Both keys should be processed

### Additional Tests

#### Non-Tap-Hold Passthrough
Verifies that regular (non-tap-hold) keys pass through Achordion without interference.

#### Layer Tap Behavior
Verifies that layer tap keys (LT macros) work correctly with Achordion's chording logic.

## Understanding the Output

When you run the tests, you'll see output like this:

```
=== Achordion Unit Tests ===
Testing Achordion implementation for QMK Voyager keymap
Split keyboard configuration: 12 rows, 7 cols

=== Test Case 1: Quick Tap Registration ===
✓ Test 1: Tap-hold key press should be intercepted
✓ Test 2: Should enter unsettled state
✓ Test 3: Should store the tap-hold keycode
✓ Test 4: Tap-hold key release should be intercepted
✓ Test 5: Should return to released state
✓ Test 6: Should clear stored keycode

[... more test output ...]

=== Test Summary ===
Total tests: 23
Passed: 23
Failed: 0
Success rate: 100.0%
🎉 All tests passed!
```

## Voyager-Specific Configuration

The tests are configured for the ZSA Voyager split keyboard:
- **Matrix**: 12 rows × 7 columns
- **Split Layout**: 6 rows per side
- **Hand Detection**: Based on row number (left: rows 0-5, right: rows 6-11)

## Troubleshooting

### Compilation Issues
- Ensure you have GCC installed
- Check that you're in the correct directory
- Verify file permissions on the shell script

### Test Failures
If tests fail, check:
1. **Achordion Implementation**: Compare with the working version in `achordion.c`
2. **Keymap Configuration**: Ensure your home row mods are configured correctly
3. **Matrix Configuration**: Verify `MATRIX_ROWS` and `MATRIX_COLS` match your keyboard

### Hand Detection Issues
If opposite hands detection fails:
- Check the `on_left_hand()` function logic
- Verify the split keyboard configuration
- Ensure row/column assignments match your physical layout

## Integration with Your Workflow

### Pre-Commit Testing
Add the test runner to your development workflow:

```fish
# Before committing changes to achordion.c or keymap.c
./run_tests.sh
```

### Continuous Validation
Run tests whenever you:
- Modify the Achordion implementation
- Change home row mod configuration
- Update tapping terms or timing
- Modify the keymap layout

## Advanced Usage

### Custom Test Cases
You can add your own test cases to `test_achordion_standalone.c`:

```c
void test_my_custom_behavior(void) {
    printf("\n=== My Custom Test ===\n");
    
    reset_mocks();
    reset_achordion_state();
    
    // Your test logic here
    
    TEST_ASSERT(condition, "Description of expected behavior");
}
```

Then add it to the `run_all_tests()` function.

### Debugging Failed Tests
Enable verbose output by modifying the TEST_ASSERT macro to include more debugging information:

```c
#define TEST_ASSERT(condition, message) do { \
    test_count++; \
    if (condition) { \
        test_passed++; \
        printf("✓ Test %d: %s\n", test_count, message); \
    } else { \
        test_failed++; \
        printf("✗ Test %d: %s\n", test_count, message); \
        printf("  Debug: condition evaluated to false at line %d\n", __LINE__); \
    } \
} while(0)
```

## Contributing

When modifying the Achordion implementation:
1. Run the existing tests to ensure no regressions
2. Add new tests for any new functionality
3. Update this documentation if needed
4. Ensure all tests pass before committing changes

---

**Note**: These tests use a simplified QMK environment with mocked functions. They test the core Achordion logic but don't test integration with the full QMK system. For complete validation, also test your keymap on the actual hardware.