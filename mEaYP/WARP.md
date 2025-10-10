# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Overview

This is a custom ZSA Voyager keymap (ID: `mEaYP`) that combines the convenience of Oryx's graphical layout editing with advanced QMK firmware features. The project uses a hybrid workflow where layout changes are made in Oryx and automatically synced to the repository, while custom QMK code is written directly in `keymap.c`, `config.h`, and `rules.mk`.

## Common Development Commands

### Building Firmware
The primary build method is through GitHub Actions:
```fish
# Trigger build via GitHub Actions interface:
# 1. Go to Actions tab -> "Fetch and build layout"
# 2. Click "Run workflow" 
# 3. Use layout_id: "mEaYP" and geometry: "voyager"
# 4. Download artifacts from completed workflow
```

### Local Development with Docker
```fish
# Build QMK Docker image
docker build -t qmk .

# Build the layout locally (from repository root)
docker run -v ./qmk_firmware:/root --rm qmk /bin/sh -c "qmk setup zsa/qmk_firmware -b firmware24 -y && make zsa/voyager:mEaYP"

# Find built firmware file
find ./qmk_firmware -maxdepth 1 -type f -regex ".*voyager.*\\.\\(bin\\|hex\\)$"
```

### QMK CLI Commands
```fish
# Setup QMK environment
qmk setup zsa/qmk_firmware

# Compile keymap
qmk compile -km mEaYP -kb zsa/voyager

# Flash firmware
qmk flash -km mEaYP -kb zsa/voyager
```

## Architecture

### Dual-Branch Workflow
- **`oryx` branch**: Contains pure Oryx exports, automatically updated when layout changes are made in the Oryx interface
- **`main` branch**: Merges Oryx changes with custom QMK modifications, used for builds

### Repository Structure
```
mEaYP/                    # Layout directory
├── keymap.c             # Main keymap logic, layers, custom functions
├── config.h             # QMK configuration constants
├── rules.mk             # Build configuration and feature enables
├── keymap.json          # Oryx module configuration
└── WARP.md             # This file

.github/workflows/
└── fetch-and-build-layout.yml  # GitHub Actions build automation
```

### Build Process
1. GitHub Action fetches latest Oryx changes to `oryx` branch
2. `oryx` branch is merged into `main` branch (custom code preserved)
3. QMK firmware submodule is updated to match layout's firmware version
4. Docker builds the firmware incorporating both Oryx layout and custom features
5. Compiled firmware is uploaded as workflow artifact

## Key QMK Features Implemented

### Home Row Mods (Layer 0)
- Left hand: `A`=Ctrl, `S`=Alt, `R`=Cmd, `T`=Shift
- Right hand: `N`=Shift, `E`=Cmd, `I`=Alt, `O`=Ctrl
- Custom tapping terms per key defined in `get_tapping_term()`

### Layer System
- **Layer 0**: Base layer (modified Workman layout)
- **Layer 1**: Numbers and function keys
- **Layer 2**: Navigation and media controls  
- **Layer 3**: System shortcuts and macros
- **Layer 4**: Currently empty/available

### Custom Macros (`ST_MACRO_0` through `ST_MACRO_5`)
- `:ls` - List files shortcut
- `dtim` - Date/time command
- `:apup` - Package update command
- `:lt` + Enter - Terminal command
- `:yup` - System update command
- `ddus` - Disk usage command

### RGB Matrix Lighting
- Per-layer LED colors defined in `ledmap[][]` array
- Custom HSV to RGB conversion with brightness control
- Layer-specific lighting patterns in `set_layer_color()`

### Advanced Features
- **Chordal Hold**: Enabled via `CHORDAL_HOLD` define and layout matrix
- **Caps Word**: Toggle on base layer for temporary caps
- **Permissive Hold**: Reduces accidental mod triggers
- **Custom Dual Function**: `DUAL_FUNC_0` key acts as `(` on tap, Shift on hold
- **Per-Key Tapping Terms**: Customized timing for `GRAVE`, `SCLN`, `SLASH`

## Technical Configuration

### Timing Settings (config.h)
- `TAPPING_TERM`: 250ms (base timing for mod-tap keys)
- `FLOW_TAP_TERM`: 100ms 
- `DEBOUNCE`: 5ms
- `AUTO_SHIFT_TIMEOUT`: 200ms

### Enabled Features (rules.mk)
- `ORYX_ENABLE`: Integration with Oryx workflow
- `CAPS_WORD_ENABLE`: Temporary caps lock functionality
- `REPEAT_KEY_ENABLE`: QMK repeat key feature
- `AUTO_SHIFT_ENABLE`: Shift characters by holding keys
- `RGB_MATRIX_CUSTOM_KB`: Custom LED patterns

### Important Functions
- `get_tapping_term()`: Per-key timing customization
- `process_record_user()`: Custom keycode handling and macros
- `rgb_matrix_indicators_user()`: Layer-based LED control
- `set_layer_color()`: Applies LED patterns for each layer

## File Locations
- Layout files: `/mEaYP/` directory
- Build workflow: `/.github/workflows/fetch-and-build-layout.yml`
- QMK firmware: `/qmk_firmware/` (submodule)
- Artifacts: Download from GitHub Actions workflow runs