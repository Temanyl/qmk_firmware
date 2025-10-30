# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is a QMK Firmware repository containing custom keyboard configurations, primarily focused on handwired keyboards under the `temanyl` namespace. QMK is a keyboard firmware framework based on tmk_keyboard for AVR and ARM controllers.

## Build Commands

### Compile firmware for a specific keyboard and keymap:
```bash
make <keyboard>:<keymap>
# Example: make handwired/temanyl/chocmanyl36:default
```

### Compile and flash firmware:
```bash
make <keyboard>:<keymap>:flash
# Example: make handwired/temanyl/chocmanyl36:default:flash
```

### Using QMK CLI (recommended):
```bash
# Compile
qmk compile -kb <keyboard> -km <keymap>
# Example: qmk compile -kb handwired/temanyl/chocmanyl36 -km default

# Flash
qmk flash -kb <keyboard> -km <keymap>
# Example: qmk flash -kb handwired/temanyl/chocmanyl36 -km default
```

### List available keyboards:
```bash
qmk list-keyboards
# or
make list-keyboards
```

### List keymaps for a specific keyboard:
```bash
make <keyboard>:list-keymaps
```

### Clean build artifacts:
```bash
make clean        # Delete .build/ directory
make distclean    # Delete .build/ plus all .bin, .hex, and .uf2 files
```

### Initialize/update git submodules:
```bash
make git-submodule
```

## Repository Structure

### Core Directories
- `keyboards/` - Keyboard-specific configurations and code
  - `keyboards/handwired/temanyl/` - Custom handwired keyboards (chocmanyl36, splitmanyl, duckymanyl, fulcrum34)
  - Each keyboard has subdirectories: `keymaps/` (keymap definitions), `info.json` (hardware config), `rules.mk` (build rules)
- `quantum/` - QMK core firmware library providing keyboard functionality (action handling, layers, tap dance, etc.)
- `tmk_core/` - Low-level keyboard firmware core
- `builddefs/` - Build system makefiles
  - `build_keyboard.mk` - Main keyboard build logic
  - `common_features.mk` - Feature compilation rules
  - `mcu_selection.mk` - MCU-specific configurations
- `drivers/` - Hardware driver implementations
- `layouts/` - Shared layout definitions across keyboards
- `lib/` - Git submodules for external dependencies (ChibiOS, LUFA, pico-sdk, etc.)

### Keyboard Configuration Files

Each keyboard requires:
1. `info.json` - Hardware configuration (matrix pins, USB IDs, layouts, processor, bootloader)
2. `rules.mk` - Build configuration and feature flags
3. `keymaps/<name>/keymap.c` - Keymap layer definitions
4. Optional: `config.h`, `halconf.h`, `mcuconf.h` for advanced hardware configuration

## Keyboard Architecture

### Custom Temanyl Keyboards

Located in `keyboards/handwired/temanyl/`, these are custom RP2040-based keyboards with unique features:

- **chocmanyl36**: 36-key ortho with ST7789 SPI display (240x135), uses QP (Quantum Painter) for display rendering
- **splitmanyl**: Split keyboard configuration
- **duckymanyl**: Custom layout variant
- **fulcrum34**: 34-key layout

All use RP2040 processor with rp2040 bootloader (UF2 format).

### Keymap Structure

Keymaps are defined in `keymaps/<name>/keymap.c` with:
- Layer definitions using `LAYOUT_*` macros matching the keyboard's layout name in `info.json`
- Layer switching via `LT()`, `TO()`, `MO()`, tap dances
- Custom key behaviors in `process_record_user()`
- Tap dance definitions for multi-function keys
- Per-key tapping terms via `get_tapping_term()`

### Build System Flow

1. Top-level `Makefile` parses `<keyboard>:<keymap>:<target>` format
2. Loads `keyboards/<keyboard>/rules.mk` for feature flags
3. Includes `builddefs/build_keyboard.mk` for compilation
4. Feature flags in `rules.mk` control which quantum/ modules are compiled
5. Outputs `.uf2` files for RP2040 boards (bootloader format)

## Common Development Patterns

### Adding a new keyboard
1. Create directory under `keyboards/handwired/<name>/`
2. Define `info.json` with matrix configuration, USB IDs, processor, bootloader
3. Create `rules.mk` with bootloader and feature settings
4. Add `keymaps/default/keymap.c` with layer definitions
5. Build with `make handwired/<name>:default`

### Modifying keymaps
- Edit `keyboards/<keyboard>/keymaps/<name>/keymap.c`
- Define layers in `keymaps[][]` array using keyboard's `LAYOUT_*` macro
- Add custom logic in `process_record_user()` for key overrides
- Use `get_tapping_term()` for per-key timing customization

### Working with displays (QP - Quantum Painter)
- Initialize in `keyboard_post_init_kb()` or `keyboard_post_init_user()`
- Use `qp_*` functions for drawing (qp_rect, qp_circle, qp_line, etc.)
- Set viewport offsets for displays with non-standard GRAM alignment
- Example in `keyboards/handwired/temanyl/chocmanyl36/keymaps/default/keymap.c`

### Tap Dance
- Define enum for tap dance IDs
- Implement state machine functions (*_finished, *_reset)
- Register in `tap_dance_actions[]` array
- Use `TD()` macro in keymap definitions

## Important Notes

- QMK CLI (`qmk` command) must be functional - checked by Makefile
- Git submodules (lib/) must be initialized for compilation
- `.uf2` files in root are compiled firmware ready to flash to RP2040 boards
- The build system automatically handles submodule initialization on first build
- Feature flags in `rules.mk` significantly affect firmware size and functionality
