#define FLOW_TAP_TERM 100
#define CHORDAL_HOLD
#undef DEBOUNCE
#define DEBOUNCE 5

// Timing tuned for home-row mods
#ifndef PERMISSIVE_HOLD
#define PERMISSIVE_HOLD     // (Oryx toggle you already use)
#endif
#define QUICK_TAP_TERM 120    // Allow key repeat but prevent accidental double-taps

// If you use Retro Tapping elsewhere, leave it off for HRMs.

#define USB_SUSPEND_WAKEUP_DELAY 0
#define AUTO_SHIFT_TIMEOUT 200
#define NO_AUTO_SHIFT_TAB
#define NO_AUTO_SHIFT_ALPHA
#define SERIAL_NUMBER "mEaYP/x9LQY0"
#define LAYER_STATE_8BIT

#define TAPPING_TERM_PER_KEY
#define RGB_MATRIX_STARTUP_SPD 60
