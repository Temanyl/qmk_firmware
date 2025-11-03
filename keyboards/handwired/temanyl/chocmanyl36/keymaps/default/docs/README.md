# Chocmanyl36 Default Keymap Documentation

This directory contains the default keymap for the Chocmanyl36 keyboard with ST7789 LCD display, featuring seasonal animations, interactive games, and a modular object-based rendering system.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Architecture Overview](#architecture-overview)
3. [File Structure](#file-structure)
4. [Display System](#display-system)
5. [Season System](#season-system)
6. [Objects System](#objects-system)
7. [Animation Workflow](#animation-workflow)
8. [Games](#games)
9. [Testing](#testing)

---

## Quick Start

### Building and Flashing

```bash
# From repository root
make handwired/temanyl/chocmanyl36:default:flash
```

### Running the Companion Script

The keyboard requires a Python companion script for full functionality:

```bash
# From repository root
python3 keyboard_monitor.py
```

This provides:
- Volume monitoring and display
- Media playback information
- Date/time synchronization for seasonal animations
- High score persistence for games

---

## Architecture Overview

The firmware uses a modular, object-oriented architecture:

```
keymap.c (keyboard logic)
    ↓
display/ (UI and hardware)
    ↓
scenes/ (seasonal orchestration)
    ↓
seasons/ (season-specific events)
    ↓
objects/ (reusable drawable elements)
```

### Design Principles

1. **Modularity**: Each visual element is a self-contained object with `.h` and `.c` files
2. **Reusability**: Objects can be used across multiple seasons/events
3. **Separation of Concerns**: Display hardware, scene logic, and objects are independent
4. **Animation Framework**: Shared timer-based animation system with background buffering

---

## File Structure

### Core Keyboard Files

**`keymap.c`** - Keyboard behavior and QMK integration
- Layer definitions (Colemak-DH, Code, Nav, Num, Arrow)
- Custom keycodes (brightness control, game modes)
- Tap dance definitions
- Raw HID communication (volume, media, date/time, high scores)
- Housekeeping task for periodic updates and animations
- Game mode switching and input handling

**`config.h`** - Keyboard configuration
- Tap dance settings
- Tapping term customization
- Hardware pin definitions

**`rules.mk`** - Build configuration
```makefile
TAP_DANCE_ENABLE = yes
QUANTUM_PAINTER_ENABLE = yes
QUANTUM_PAINTER_DRIVERS += st7789_spi
RAW_ENABLE = yes  # For HID communication

# Include all source files
SRC += display/display.c display/framebuffer.c
SRC += scenes/scenes.c
SRC += seasons/**/*.c
SRC += objects/**/*.c
SRC += game_manager.c game_doodle.c game_tetris.c
```

---

### Display System

Located in `display/` directory:

**`display.h` / `display.c`** - Display hardware and UI elements
- ST7789 SPI initialization (240x135 pixels, 53x40 offset)
- PWM backlight control (GP4)
- 7-segment clock rendering
- Volume bar visualization
- Scrolling media text
- Brightness overlay
- Layer color management

**`framebuffer.h` / `framebuffer.c`** - Custom framebuffer system
- HSV framebuffer (240x155 pixels) for scenes
- RGB565 conversion for ST7789
- Region-based flushing for efficiency
- Background save/restore for animations
- Direct drawing primitives (rect, circle, line)

**Display Layout:**
```
┌─────────────────────────────┐
│   Framebuffer Region        │ y=0-154
│   (Seasonal scenes)         │
│                             │
│   - Sky/celestial           │
│   - Environment (trees)     │
│   - Animated elements       │
│   - Seasonal effects        │
├─────────────────────────────┤ y=155
│   Quantum Painter Region    │ y=155-199
│                             │
│   HH:MM MM/DD (clock)       │
│   [Now Playing Text]        │
│   [=====] (volume bar)      │
└─────────────────────────────┘
```

---

### Season System

Located in `seasons/` directory. Each season has its own subdirectory with `.h` and `.c` files.

#### Season Definitions

**Winter** (`seasons/winter/`)
- **Months**: December, January, February
- **Features**: Snow-covered environment, snowflakes, chimney smoke
- **Colors**: White/blue palette

**Spring** (`seasons/spring/`)
- **Months**: March, April, May
- **Features**: Green foliage, flowers, butterflies
- **Colors**: Green/pastel palette

**Summer** (`seasons/summer/`)
- **Months**: June, July, August
- **Features**: Bright sun, sun rays, bees, fireflies
- **Colors**: Yellow/bright palette

**Fall** (`seasons/fall/`)
- **Months**: September, October, November
- **Features**: Orange/red foliage, rain animation, birds migrating
- **Colors**: Orange/brown palette

#### Special Events

**Halloween** (`seasons/halloween/`)
- **Dates**: October 28 - November 3
- **Features**: Pumpkins, animated floating ghosts (3 ghosts with sine-wave motion)
- **Animation**: 80ms update rate (12.5 fps)
- **Overrides**: Fall season during this period

**Christmas** (`seasons/christmas/`)
- **Dates**: December 1-31
- **Features**:
  - Advent calendar (1-24 items appear progressively)
  - 24 different decorations (presents, ornaments, candy canes, etc.)
  - Animated Santa sleigh (December 25+)
- **Animation**: 100ms update rate (10 fps)

**New Year's Eve** (`seasons/christmas/`)
- **Date**: December 31
- **Features**: Static fireworks display, "HNY" text
- **Note**: Part of Christmas module

#### Season Module Structure

Each season module follows this pattern:

**`seasons_<name>.h`** - Public interface
```c
// Event detection
bool is_<season>_event(void);

// Drawing
void draw_<season>_elements(void);
void draw_<season>_scene(void);

// Animation
void init_<season>_animation(void);
void animate_<season>_elements(void);
void reset_<season>_animations(void);

// State variables (extern)
extern bool <season>_initialized;
extern uint32_t <season>_animation_timer;
```

**`seasons_<name>.c`** - Implementation
- State management
- Animation logic
- Object composition
- Event detection based on date/time

---

### Objects System

Located in `objects/` directory, organized by category.

#### Object Categories

**`celestial/`** - Sky elements
- `sun.h/c` - Sun with hourly position calculation
- `moon.h/c` - Moon with phase calculation
- `stars.h/c` - Background stars

**`weather/`** - Weather effects
- `cloud.h/c` - Cloud rendering
- `raindrop.h/c` - Animated rain particles
- `snowflake.h/c` - Snowflake particles
- `smoke.h/c` - Chimney smoke particles

**`flora/`** - Plant life
- `flower.h/c` - Spring flowers
- Various plant objects

**`fauna/`** - Animals and creatures
- `bird.h/c` - Flying birds
- `butterfly.h/c` - Animated butterflies
- `bee.h/c` - Buzzing bees
- `firefly.h/c` - Glowing fireflies

**`structures/`** - Buildings and static elements
- `tree.h/c` - Trees with seasonal variations
- `cabin.h/c` - Cabin with chimney
- `ground.h/c` - Ground with seasonal colors

**`seasonal/`** - Holiday-specific objects
- `ghost.h/c` - Floating ghost (Halloween)
- `pumpkin.h/c` - Jack-o'-lantern (Halloween)
- `snowman.h/c` - Snowman (Winter)

**`effects/`** - Visual effects
- Various effect objects

#### Object Structure Pattern

**All objects follow the unified interface pattern** with standardized method names and single-instance operations. Objects operate on individual instances; scene managers own and manage arrays.

**Key Principles:**
- Objects operate on **single instances**, not arrays
- Array management happens in **scene managers**, not in object code
- All objects use **singular naming** (e.g., `bird_draw()`, not `birds_draw_all()`)
- Consistent method signatures across all object types

**Standard Methods** (where applicable):
```c
<type>_init(<type>_t* obj, ...params)           // Initialize single instance
<type>_draw(const <type>_t* obj, ...)           // Draw single instance
<type>_update(<type>_t* obj)                    // Update animation (animated objects)
<type>_get_bounds(const <type>_t* obj, ...)     // Get bounding box
<type>_contains_point(const <type>_t* obj, ...) // Point containment test
```

**Header file (`object.h`):**
```c
#pragma once
#include <stdint.h>
#include <stdbool.h>

// Constants
#define OBJECT_WIDTH 20
#define OBJECT_HEIGHT 20

// Type definition
typedef struct {
    int16_t x;        // Position
    int16_t y;
    int8_t  vx;       // Velocity (if animated)
    int8_t  vy;
    uint8_t phase;    // Animation phase
    // ... other state
} object_t;

// Functions
void object_init(object_t* obj, int16_t x, int16_t y, ...);
void object_draw(const object_t* obj);
void object_update(object_t* obj);  // For animated objects
bool object_contains_point(const object_t* obj, int16_t px, int16_t py);
void object_get_bounds(const object_t* obj, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);
```

**Implementation file (`object.c`):**
```c
#include "object.h"
#include "../../display/framebuffer.h"

void object_init(object_t* obj, int16_t x, int16_t y, ...) {
    obj->x = x;
    obj->y = y;
    // ... initialize other fields
}

void object_draw(const object_t* obj) {
    // Use framebuffer drawing primitives
    fb_rect_hsv(obj->x, obj->y, width, height, hue, sat, val);
    fb_circle_hsv(obj->x, obj->y, radius, hue, sat, val);
    // ... drawing logic
}

void object_update(object_t* obj) {
    // Update position/state for animation
    obj->x += obj->vx;
    obj->y += obj->vy;
    // ... animation logic
}
```

**See:** `objects/object_interface.h` for comprehensive documentation and examples.

#### Using Objects in Scenes

**Scene managers own arrays and loop over single-instance methods.** This follows the unified interface pattern.

Example from `seasons_halloween.c`:

```c
#include "../../objects/seasonal/ghost.h"
#include "../../objects/seasonal/pumpkin.h"

// Scene manager owns the array
#define NUM_GHOSTS 3
static ghost_t ghosts[NUM_GHOSTS];

void init_ghosts(void) {
    // Loop over array, calling single-instance init
    ghost_init(&ghosts[0], 40, 60, 1, 0);
    ghost_init(&ghosts[1], 120, 70, -1, 53);
    ghost_init(&ghosts[2], 200, 50, 1, 106);
}

void animate_ghosts(void) {
    // Loop over array, calling single-instance methods
    for (int i = 0; i < NUM_GHOSTS; i++) {
        // Restore background before moving
        int16_t x1, y1, x2, y2;
        ghost_get_bounds(&ghosts[i], &x1, &y1, &x2, &y2);
        fb_restore_from_background(x1, y1, x2, y2);

        // Update position (single-instance method)
        ghost_update(&ghosts[i]);

        // Draw at new position (single-instance method)
        ghost_draw(&ghosts[i]);

        // Flush changed region
        fb_flush_region(x1, y1, x2, y2);
    }
}
```

**Note:** Objects no longer have plural methods like `ghosts_draw_all()`. Scene managers loop over arrays calling singular methods like `ghost_draw(&ghost)`.

---

### Scene Rendering

Located in `scenes/` directory.

**`scenes.h` / `scenes.c`** - Scene orchestration

**Main Function**: `draw_seasonal_animation()`
```c
void draw_seasonal_animation(void) {
    // 1. Draw base environment
    uint8_t season = get_season(current_month);
    draw_sky(season);
    draw_celestial(current_hour);  // Sun or moon

    // 2. Draw landscape
    draw_trees(season);
    draw_cabin(season);
    draw_ground(season);

    // 3. Draw seasonal effects
    switch(season) {
        case WINTER: draw_winter_scene(); break;
        case SPRING: draw_spring_scene(); break;
        case SUMMER: draw_summer_scene(); break;
        case FALL:   draw_fall_scene(); break;
    }

    // 4. Check for special events (override seasonal)
    if (is_halloween_event()) {
        draw_halloween_elements();
    } else if (is_christmas_season()) {
        draw_christmas_scene();
    }
    if (is_new_years_eve()) {
        draw_fireworks_scene();
    }

    // 5. Flush framebuffer
    fb_flush();
}
```

**Reset Function**: `reset_scene_animations()`
```c
void reset_scene_animations(void) {
    // Called when layer changes or scene needs refresh
    reset_winter_animations();
    reset_spring_animations();
    reset_summer_animations();
    reset_fall_animations();
    reset_halloween_animations();
    reset_christmas_animations();
    smoke_initialized = false;
    smoke_background_saved = false;
}
```

---

## Animation Workflow

### Animation System Architecture

All animations use QMK's timer system (`timer_read32()`) and follow this pattern:

1. **Initialization**: Create object instances
2. **Background Save**: Save static scene to background buffer
3. **Update Loop**: Restore → Update → Draw → Flush region
4. **Reset**: Clear state when scene changes

### Animation Timer Pattern

```c
// State variables
static bool animation_initialized = false;
static bool background_saved = false;
static uint32_t animation_timer = 0;
static object_t objects[NUM_OBJECTS];

void init_animation(void) {
    if (!animation_initialized) {
        // Initialize objects
        for (int i = 0; i < NUM_OBJECTS; i++) {
            object_init(&objects[i], x, y, ...);
        }
        animation_initialized = true;
    }
}

void animate_objects(void) {
    // Throttle to desired frame rate
    if (timer_elapsed32(animation_timer) < ANIMATION_SPEED) {
        return;  // Not time to update yet
    }
    animation_timer = timer_read32();

    // Save background on first frame
    if (!background_saved) {
        fb_save_background();
        background_saved = true;
    }

    // Update each object
    for (int i = 0; i < NUM_OBJECTS; i++) {
        // Get old bounds
        int16_t x1, y1, x2, y2;
        object_get_bounds(&objects[i], &x1, &y1, &x2, &y2);

        // Restore background
        fb_restore_from_background(x1, y1, x2, y2);

        // Update position
        object_update(&objects[i]);

        // Draw at new position
        object_draw(&objects[i]);

        // Flush changed region only
        fb_flush_region(x1, y1, x2, y2);
    }
}

void reset_animation(void) {
    animation_initialized = false;
    background_saved = false;
}
```

### Animation Frame Rates

Different animations use different frame rates for performance:

| Animation | Speed (ms) | FPS | Notes |
|-----------|-----------|-----|-------|
| Rain | 50 | 20 | Fall season |
| Ghosts | 80 | 12.5 | Halloween event |
| Santa | 100 | 10 | Christmas Dec 25+ |
| Smoke | 100 | 10 | Winter chimney |
| Snowflakes | 50 | 20 | Winter weather |

### Calling Animations from Housekeeping

In `keymap.c`, the `housekeeping_task_user()` function calls animations:

```c
void housekeeping_task_user(void) {
    // Update layer-based display
    update_display_for_layer();

    // Trigger seasonal animations based on date
    if (get_season(current_month) == FALL) {
        animate_raindrops();
    }

    if (is_halloween_event()) {
        animate_ghosts();
    }

    if (is_christmas_season() && current_day >= 25) {
        update_santa_animation();
    }

    if (get_season(current_month) == WINTER) {
        animate_smoke();
    }

    // ... other periodic tasks
}
```

---

## Games

The keyboard includes two built-in games accessible via the Arrow layer.

### Game System

**`game_manager.h/c`** - Game selection and mode switching
- Game selection menu
- Input routing to active game
- Score tracking

**`game_doodle.h/c`** - Doodle Jump implementation
- Platform jumping mechanics
- Score tracking
- High score system with arcade-style name entry
- See `HIGHSCORE_README.md` for details

**`game_tetris.h/c`** - Tetris implementation
- Classic block-falling gameplay
- Score tracking
- High score system

### Game Mode Activation

1. Switch to Arrow layer (game layer)
2. Use arrow keys to select game
3. Press Shift to start
4. Arrow keys control gameplay
5. Shift to exit

### High Score System

See `HIGHSCORE_README.md` for complete documentation.

**Quick summary:**
- Top 10 scores saved via Python companion script
- Arcade-style 3-letter name entry
- Offline mode for testing (scores not saved)
- Persistent storage in `highscores.json`

---

## Testing

### Hard-Coded Date Testing

For testing seasonal displays without changing system time, use hard-coded dates.

**Enable in `display/display.c`:**

```c
#define HARDCODE_DATE_TIME  // Uncomment this line

// Set test date
#define HARDCODED_MONTH     10      // October
#define HARDCODED_DAY       28      // 28th
#define HARDCODED_YEAR      2025
#define HARDCODED_HOUR      18      // 6 PM
#define HARDCODED_MINUTE    30
```

**Test scenarios:**

| Event | Month | Day | Hour | What to Test |
|-------|-------|-----|------|--------------|
| Halloween | 10 | 28 | 18 | Pumpkins + floating ghosts |
| Christmas Advent | 12 | 15 | 10 | 15 advent items |
| Christmas Day | 12 | 25 | 14 | Santa flying across screen |
| New Year's Eve | 12 | 31 | 23 | Fireworks + "HNY" text |
| Spring | 4 | 15 | 14 | Flowers, butterflies |
| Summer Noon | 7 | 20 | 12 | Sun at zenith, bright colors |
| Fall Rain | 10 | 10 | 16 | Rain animation |
| Winter Night | 1 | 15 | 20 | Moon, stars, snow, smoke |

**Important:** Always comment out `#define HARDCODE_DATE_TIME` before merging!

### Workflow for Test Branches

```bash
# 1. Create test branch for specific feature
git checkout -b test-halloween-ghosts

# 2. Enable hard-coded date in display/display.c
# Set date to Oct 28, 6 PM

# 3. Build and test
make handwired/temanyl/chocmanyl36:default:flash

# 4. Iterate on ghost animation...

# 5. Before merging: disable hard-coded date
# Comment out: // #define HARDCODE_DATE_TIME

# 6. Test with real time from companion script
python3 keyboard_monitor.py

# 7. Commit and merge
git add .
git commit -m "Add smooth floating motion to Halloween ghosts"
git checkout master
git merge test-halloween-ghosts
```

---

## Adding New Content

See separate documentation files:
- `SEASONS.md` - How to add new seasons and seasonal events
- `ANIMATIONS.md` - How to create new animations
- `OBJECTS.md` - How to create new drawable objects

---

## Performance Considerations

### Memory Usage

- **Framebuffer**: ~72 KB (240×155 × 3 bytes HSV)
- **Background buffer**: ~72 KB (for animation restoration)
- **Total display memory**: ~144 KB of RP2040's 264 KB SRAM

### Optimization Strategies

1. **Region-based flushing**: Only update changed pixels
2. **Background buffering**: Avoid redrawing static elements
3. **Change detection**: Only redraw when values actually change
4. **Throttled animations**: Different frame rates for different effects
5. **Batched updates**: Single flush at end of housekeeping

### Best Practices

- Use `fb_flush_region()` instead of `fb_flush()` for animations
- Save background once, restore repeatedly
- Minimize SPI transfers (each pixel is 2 bytes over SPI)
- Use HSV color space for easy color manipulation
- Keep animation speeds reasonable (10-20 fps is smooth enough)

---

## Companion Script

### keyboard_monitor.py

Located in repository root. Provides:

**Volume Monitoring:**
- Reads system volume (macOS: AppleScript, Linux: ALSA)
- Sends volume updates to keyboard (HID message 0x01)

**Media Playback:**
- Reads current media info (macOS: Music.app, Spotify)
- Sends now-playing text to keyboard (HID message 0x02)

**Date/Time Sync:**
- Sends current date/time every second (HID message 0x03)
- Required for seasonal animations to work

**High Score Management:**
- Receives scores from games (HID message 0x10)
- Saves to `highscores.json`
- Sends leaderboard back to keyboard (HID message 0x12)

### Running the Companion

```bash
# Install dependencies
pip3 install hid

# Run (must stay running for full functionality)
python3 keyboard_monitor.py
```

**Note:** Keyboard works without companion, but:
- No seasonal animations (no date/time)
- No volume/media display
- No high score persistence

---

## License

Copyright 2022 Joe Scotto

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version.

---

## Additional Documentation

- `HIGHSCORE_README.md` - Game high score system
- `SEASONS.md` - Adding new seasons (to be created)
- `ANIMATIONS.md` - Creating new animations (to be created)
- `OBJECTS.md` - Creating new objects (to be created)
