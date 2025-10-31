# Chocmanyl36 Default Keymap

This directory contains the default keymap and display code for the Chocmanyl36 keyboard with ST7789 LCD display.

## File Structure

### Core Keyboard Files

#### `keymap.c` (565 lines)
**Purpose**: Keyboard behavior, input handling, and QMK integration

**Handles**:
- Layer definitions (Colemak-DH, Code, Nav, Num layers)
- Custom keycodes (DISP_UP, DISP_DN for brightness control)
- Tap dance definitions and state machines
- Key processing logic (`process_record_user`)
- Tapping term customization (`get_tapping_term`)
- Raw HID receive callback for host communication (volume, media, date/time)
- Housekeeping task for periodic updates and animations
- Keyboard initialization (`keyboard_post_init_kb`)

**Key Functions**:
- `process_record_user()` - Handles custom keycodes and special key behaviors
- `housekeeping_task_user()` - Main event loop for display updates and animations
- `raw_hid_receive()` - Receives volume, media text, and date/time from host computer
- Tap dance functions for layer switching and multi-tap behaviors

---

### Display System Files

#### `display.h` (75 lines)
**Purpose**: Public interface for display functionality

**Declares**:
- External variables: `display`, `media_font`, brightness, volume, date/time state
- Display drawing functions
- Display initialization and management functions

**Key Declarations**:
- `init_display()` - Initialize display hardware and draw initial UI
- `update_display_for_layer()` - Update background color based on active layer
- `draw_date_time()` - Render 7-segment clock display
- `draw_volume_bar()` - Render volume indicator bar
- `draw_brightness_indicator()` - Show temporary brightness overlay
- `draw_media_text()` - Render scrolling media text
- `set_backlight_brightness()` - Control PWM backlight

#### `display.c` (549 lines)
**Purpose**: Display hardware initialization and UI element rendering

**Handles**:
- **Hardware Setup**:
  - ST7789 SPI display initialization (240x135 pixels with offsets)
  - PWM backlight control on GP4
  - Power pin management (GP22 for LILYGO board)
  - Font loading (Helvetica 20px)

- **UI Elements**:
  - 7-segment digit rendering for clock (`draw_digit`)
  - Date/time display (HH:MM MM/DD)
  - Volume bar (0-100% visualization)
  - Scrolling media text with automatic scroll detection
  - Brightness indicator overlay (temporary, 3-second timeout)

- **Layer Management**:
  - Layer-to-color mapping (Teal=Colemak, Green=Nav, Red=Code, Yellow=Num)
  - Background color transitions
  - Scene animation reset on layer changes

**State Variables**:
- `display` - Quantum Painter device handle
- `media_font` - Font handle for media text
- `current_display_layer` - Cached layer for change detection
- `backlight_brightness` - Current PWM brightness (0-255)
- `current_volume` - Volume level (0-100)
- `current_hour/minute/day/month/year` - Date/time from host
- `time_received` - Whether time has been received from host
- Media text state: `current_media`, `scroll_position`, `needs_scroll`
- Brightness overlay state: `brightness_display_active`, `brightness_display_timer`

---

### Scene Rendering Files

#### `scenes.h` (123 lines)
**Purpose**: Public interface for seasonal animations and scene rendering

**Declares**:
- Animation constants (raindrop count, ghost count, animation speeds)
- Type definitions for animated elements (raindrop_t, ghost_t, christmas_item_type_t)
- External variables for animation state
- Scene drawing and animation functions

**Key Declarations**:
- `draw_seasonal_animation()` - Main scene renderer (sun/moon, weather, events)
- `reset_scene_animations()` - Reset all animation states
- Halloween functions: `init_ghosts()`, `animate_ghosts()`, `draw_halloween_elements()`
- Christmas functions: `draw_christmas_scene()`, `update_santa_animation()`
- Utility functions: `get_season()`, `is_halloween_event()`, `is_christmas_season()`

#### `scenes.c` (1240 lines)
**Purpose**: Seasonal scene rendering and animation implementation

**Handles**:
- **Seasonal Scenes** (Spring, Summer, Fall, Winter):
  - Trees with seasonal foliage colors
  - Cabin with chimney smoke (winter only)
  - Ground with seasonal colors
  - Sun/moon with position based on hour (0-23)
  - Moon phases based on day of month
  - Seasonal weather effects (flowers, sun rays, raindrops, snowflakes)

- **Rain Animation** (Fall season):
  - 50 animated raindrops (2x4 pixels each)
  - Vertical movement with reset at ground level
  - Background restoration for smooth animation
  - Region-based framebuffer flushing for efficiency

- **Halloween Event** (Oct 28 - Nov 3):
  - 4 static pumpkins at fixed positions
  - 3 animated floating ghosts (16x16 pixels)
  - Sine wave approximation for smooth floating motion
  - Horizontal drifting with screen wrapping
  - Background restoration and region-based rendering

- **Christmas Season** (Dec 1-31):
  - Advent calendar system (1-24 items appear progressively)
  - 24 different Christmas decorations (presents, ornaments, candy canes, etc.)
  - Animated Santa sleigh (flies across screen on Dec 25+)
  - Santa with 2 reindeer (Rudolph with red nose)

- **New Year's Eve** (Dec 31):
  - 6 static fireworks at various positions
  - "HNY" text in large block letters
  - Colorful burst patterns (8 directional rays per firework)

**State Variables**:
- Rain: `raindrops[]`, `rain_initialized`, `rain_background_saved`, `rain_animation_timer`
- Ghosts: `ghosts[]`, `ghost_initialized`, `ghost_background_saved`, `ghost_animation_timer`
- Christmas: `santa_x`, `santa_initialized`, `santa_animation_timer`
- Advent items: `advent_items[]` array with 24 positioned decorations

**Key Functions**:
- `draw_seasonal_animation()` - Main orchestrator, draws complete scene based on date/time
- `animate_raindrops()` - Updates raindrop positions (called at 20fps during fall)
- `animate_ghosts()` - Updates ghost floating motion (called at 12.5fps during Halloween)
- `draw_christmas_item()` - Renders one of 24 Christmas decoration types
- `update_santa_animation()` - Moves Santa sleigh across screen
- `get_celestial_position()` - Calculates sun/moon position based on hour

---

### Supporting Files

#### `framebuffer.h` / `framebuffer.c`
**Purpose**: Custom framebuffer system for split rendering

**Provides**:
- Upper region (y=0-154): HSV framebuffer for scenes, manually flushed
- Lower region (y=155+): Direct Quantum Painter rendering for UI elements
- Background saving/restoration for animations
- Region-based flushing for efficiency
- HSV to RGB565 conversion for ST7789

**Key Functions**:
- `fb_init()` - Allocate framebuffer memory
- `fb_rect_hsv()` - Draw HSV rectangle to framebuffer
- `fb_circle_hsv()` - Draw HSV circle to framebuffer
- `fb_flush()` - Flush entire framebuffer to display
- `fb_flush_region()` - Flush specific region for efficiency
- `fb_save_background()` - Save current scene to background buffer
- `fb_restore_from_background()` - Restore specific region from background

#### `draw_logo.h`
**Purpose**: Amboss logo rendering

**Provides**:
- `draw_amboss_logo()` - Renders company logo in HSV color
- Used during display initialization in `display.c`

#### `rules.mk`
**Purpose**: Build configuration

**Contents**:
```makefile
TAP_DANCE_ENABLE = yes
QUANTUM_PAINTER_ENABLE = yes
QUANTUM_PAINTER_DRIVERS += st7789_spi

# Framebuffer support
SRC += framebuffer.c

# Display and scene rendering
SRC += display.c scenes.c
```

---

## Data Flow

### Initialization (Power On)
1. `keyboard_post_init_kb()` in `keymap.c` calls `init_display()` from `display.c`
2. `init_display()`:
   - Initializes ST7789 SPI display with proper offsets (53, 40)
   - Sets up PWM backlight on GP4
   - Loads Helvetica 20px font
   - Initializes framebuffer system
   - Draws Amboss logo
   - Calls `draw_seasonal_animation()` from `scenes.c` to draw initial scene
   - Draws initial UI elements (date/time, volume bar)

### Periodic Updates (Housekeeping)
`housekeeping_task_user()` in `keymap.c` runs continuously and:
1. Calls `update_display_for_layer()` to handle layer changes
2. Checks for time/date changes and redraws scene if needed
3. Handles brightness overlay timeout
4. Manages media text scrolling
5. Triggers rain animation (fall season, 20fps)
6. Triggers ghost animation (Halloween event, 12.5fps)
7. Triggers Santa animation (Dec 25+, 10fps)
8. Flushes framebuffer if any changes occurred

### Host Communication
`raw_hid_receive()` in `keymap.c` receives data from host:
- **Volume updates** (0x01): Updates volume bar via `draw_volume_bar()` from `display.c`
- **Media text** (0x02): Updates scrolling text via `draw_media_text()` from `display.c`
- **Date/Time** (0x03): Updates clock and triggers full scene redraw

### User Input
`process_record_user()` in `keymap.c` handles:
- **DISP_UP/DISP_DN**: Calls `set_backlight_brightness()` from `display.c`
- Brightness changes trigger overlay display via `draw_brightness_indicator()`
- Other custom key behaviors (backspace→delete with shift, etc.)

---

## Animation System

### Frame-Based Animations
All animations use the QMK timer system (`timer_read32()`) and region-based flushing:

1. **Rain** (Fall season):
   - Speed: 50ms per frame (20fps)
   - Updates: Vertical movement by 3 pixels
   - Optimization: Only flushes changed regions (2x4 pixel raindrops)

2. **Ghosts** (Halloween Oct 28 - Nov 3):
   - Speed: 80ms per frame (12.5fps)
   - Updates: Sine wave floating + horizontal drift
   - Optimization: Restores background before redrawing

3. **Santa** (Christmas Dec 25+):
   - Speed: 100ms per frame (10fps)
   - Updates: Horizontal movement across screen
   - Resets when off-screen to loop animation

### Background Buffering
Scenes use a two-buffer system:
- **Primary framebuffer**: Current visible scene
- **Background buffer**: Static scene without animations (saved via `fb_save_background()`)

Animations restore from background before drawing new position, eliminating the need to redraw the entire static scene each frame.

---

## Color System

### Layer Colors (HSV)
- **Colemak-DH** (default): Hue=128 (Teal)
- **Navigation**: Hue=85 (Green)
- **Code/Symbols**: Hue=0 (Red)
- **Numbers**: Hue=43 (Yellow)

All layer colors use Sat=255, Val=255 (fully saturated and bright).

### Seasonal Colors
- **Spring**: Green trees, light green ground, flowers
- **Summer**: Darker green trees, yellow sun, sun rays
- **Fall**: Orange/red trees, brown ground, rain
- **Winter**: White trees (snow), white ground, snowflakes

### Conversion
The framebuffer system converts HSV to RGB565 for the ST7789 display.

---

## Seasonal Event Schedule

| Event | Dates | Features |
|-------|-------|----------|
| Spring | Mar 1 - May 31 | Green trees, flowers |
| Summer | Jun 1 - Aug 31 | Sun rays, darker foliage |
| Fall | Sep 1 - Nov 30 | Orange trees, rain animation |
| Winter | Dec 1 - Feb 28/29 | Snow-covered trees, snowflakes, chimney smoke |
| Halloween | Oct 28 - Nov 3 | Pumpkins + animated floating ghosts |
| Christmas Advent | Dec 1-24 | Progressive decoration reveal (1 per day) |
| Christmas Day | Dec 25-31 | All decorations + animated Santa sleigh |
| New Year's Eve | Dec 31 | Fireworks display + "HNY" text |

---

## Display Layout

```
┌─────────────────────────────┐
│   Framebuffer Region        │ y=0
│   (240x155 pixels)          │
│                             │
│   Seasonal Scene:           │
│   - Sky gradient            │
│   - Sun/Moon (moves hourly) │
│   - Trees & Cabin           │
│   - Animated elements       │
│     (rain/ghosts/santa)     │
├─────────────────────────────┤ y=155 (transition line)
│   Quantum Painter Region    │
│   (240x20 pixels)           │
│                             │
│   Date/Time:   HH:MM MM/DD  │ y=157 (7-segment digits)
│   Media Text:  [Scrolling]  │ y=178 (uses font)
│   Volume Bar:  [========]   │ y=190 (horizontal bar)
│                             │
└─────────────────────────────┘ y=199 (bottom)
     (240 pixels wide)
```

**Note**: The framebuffer region (y < 155) requires manual flushing via `fb_flush()` or `fb_flush_region()`. The Quantum Painter region (y ≥ 155) auto-flushes on `qp_flush()` calls.

---

## Performance Considerations

### Efficient Updates
- **Region-based flushing**: Only update changed screen areas
- **Background buffering**: Avoid redrawing static elements
- **Change detection**: Only redraw when values actually change
- **Batched updates**: Single flush at end of housekeeping task

### Memory Usage
- Primary framebuffer: ~72KB (240×155 pixels × 3 bytes HSV)
- Background buffer: ~72KB (same size, for animation restoration)
- Display buffers managed by framebuffer system
- Font data: Embedded in binary (Helvetica 20px)

### Animation Throttling
Rain, ghosts, and Santa use independent timers at different framerates to balance visual smoothness with CPU usage.

---

## Future Enhancements

Potential areas for expansion:
- Additional seasonal events (Easter, Valentine's Day, etc.)
- More animation types (snow accumulation, wind effects)
- Dynamic weather based on real conditions (via host data)
- User-customizable event dates
- More interactive elements responding to keyboard activity
- Transition animations between scenes

---

## License

Copyright 2022 Joe Scotto

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version.
