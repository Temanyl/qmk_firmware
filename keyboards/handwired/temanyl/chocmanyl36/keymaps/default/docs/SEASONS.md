# Adding New Seasons and Seasonal Events

This guide explains how to add new seasons or seasonal events to the chocmanyl36 display system.

## Table of Contents

1. [Quick Overview](#quick-overview)
2. [Adding a New Season](#adding-a-new-season)
3. [Adding a New Seasonal Event](#adding-a-new-seasonal-event)
4. [Modifying Existing Seasons](#modifying-existing-seasons)
5. [Testing Your Season](#testing-your-season)

---

## Quick Overview

### Architecture

```
scenes/scenes.c (orchestrator)
    ↓ calls
seasons/<season_name>/seasons_<season_name>.c (event handler)
    ↓ uses
objects/*/ (drawable elements)
```

### When to Create a Season vs Event

**Create a new Season when:**
- You want a different base environment for 3+ months
- Example: A "rainy season" that lasts April-June

**Create a new Event when:**
- You want special decorations/animations for a specific date range
- Example: Valentine's Day (Feb 14), Easter (specific dates), etc.
- Events override the base season during their active period

---

## Adding a New Season

Let's create a "Monsoon" season as an example (June 15 - August 15).

### Step 1: Create Season Directory

```bash
cd keyboards/handwired/temanyl/chocmanyl36/keymaps/default/seasons/
mkdir monsoon
```

### Step 2: Create Header File

Create `seasons/monsoon/seasons_monsoon.h`:

```c
/*
Copyright 2022 Joe Scotto

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Monsoon season (June 15 - August 15)

// Animation constants
#define NUM_HEAVY_RAIN 80          // More rain than fall season
#define HEAVY_RAIN_ANIMATION_SPEED 30  // Faster animation (30ms = 33fps)

// External state variables
extern bool monsoon_initialized;
extern bool monsoon_background_saved;
extern uint32_t monsoon_animation_timer;

// Public functions
bool is_monsoon_season(void);
void draw_monsoon_scene(void);
void init_monsoon_rain(void);
void animate_monsoon_rain(void);
void reset_monsoon_animations(void);
```

### Step 3: Create Implementation File

Create `seasons/monsoon/seasons_monsoon.c`:

```c
/*
Copyright 2022 Joe Scotto

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
*/

#include QMK_KEYBOARD_H
#include "seasons_monsoon.h"
#include "../../display/display.h"
#include "../../display/framebuffer.h"
#include "../../objects/weather/raindrop.h"
#include "../../objects/weather/cloud.h"

// State variables
bool monsoon_initialized = false;
bool monsoon_background_saved = false;
uint32_t monsoon_animation_timer = 0;

static raindrop_t monsoon_rain[NUM_HEAVY_RAIN];
static cloud_t monsoon_clouds[3];

// External date/time from display.c
extern uint8_t current_month;
extern uint8_t current_day;

// Check if it's monsoon season
bool is_monsoon_season(void) {
    if (current_month == 6 && current_day >= 15) return true;
    if (current_month == 7) return true;
    if (current_month == 8 && current_day <= 15) return true;
    return false;
}

// Initialize monsoon rain
void init_monsoon_rain(void) {
    if (!monsoon_initialized) {
        // Create heavy rain particles
        for (int i = 0; i < NUM_HEAVY_RAIN; i++) {
            int16_t x = (i * 37) % 240;  // Spread across screen
            int16_t y = (i * 19) % 155;  // Stagger vertically
            raindrop_init(&monsoon_rain[i], x, y, 5);  // Speed: 5 pixels/frame
        }

        // Create dark clouds
        cloud_init(&monsoon_clouds[0], 40, 20, 40, 30);   // Large cloud
        cloud_init(&monsoon_clouds[1], 120, 15, 35, 25);  // Medium cloud
        cloud_init(&monsoon_clouds[2], 200, 25, 30, 20);  // Small cloud

        monsoon_initialized = true;
    }
}

// Animate monsoon rain
void animate_monsoon_rain(void) {
    // Throttle animation
    if (timer_elapsed32(monsoon_animation_timer) < HEAVY_RAIN_ANIMATION_SPEED) {
        return;
    }
    monsoon_animation_timer = timer_read32();

    // Save background on first frame
    if (!monsoon_background_saved) {
        fb_save_background();
        monsoon_background_saved = true;
    }

    // Animate each raindrop
    for (int i = 0; i < NUM_HEAVY_RAIN; i++) {
        // Get old bounds
        int16_t x1, y1, x2, y2;
        raindrop_get_bounds(&monsoon_rain[i], &x1, &y1, &x2, &y2);

        // Restore background
        fb_restore_from_background(x1, y1, x2, y2);

        // Update position
        raindrop_update(&monsoon_rain[i]);

        // Reset if off screen
        if (monsoon_rain[i].y > 155) {
            monsoon_rain[i].y = -5;
            monsoon_rain[i].x = (monsoon_rain[i].x + 17) % 240;
        }

        // Draw at new position
        raindrop_draw(&monsoon_rain[i]);

        // Get new bounds and flush
        raindrop_get_bounds(&monsoon_rain[i], &x1, &y1, &x2, &y2);
        fb_flush_region(x1, y1, x2, y2);
    }
}

// Draw monsoon scene (static elements)
void draw_monsoon_scene(void) {
    // Draw dark clouds
    for (int i = 0; i < 3; i++) {
        cloud_draw(&monsoon_clouds[i]);
    }

    // Initialize rain animation
    init_monsoon_rain();
}

// Reset monsoon animations
void reset_monsoon_animations(void) {
    monsoon_initialized = false;
    monsoon_background_saved = false;
}
```

### Step 4: Integrate into scenes.c

Edit `scenes/scenes.c`:

**Add include:**
```c
#include "../seasons/monsoon/seasons_monsoon.h"
```

**Update `reset_scene_animations()`:**
```c
void reset_scene_animations(void) {
    // ... existing resets ...
    reset_monsoon_animations();
}
```

**Update `draw_seasonal_animation()`:**
```c
void draw_seasonal_animation(void) {
    // ... existing scene setup ...

    // Check for monsoon season (overrides summer)
    if (is_monsoon_season()) {
        draw_monsoon_scene();
    } else {
        // ... normal seasonal rendering ...
    }

    // ... rest of function ...
}
```

### Step 5: Call Animation in keymap.c

Edit `keymap.c` in the `housekeeping_task_user()` function:

```c
void housekeeping_task_user(void) {
    // ... existing code ...

    // Trigger monsoon animation
    if (is_monsoon_season()) {
        animate_monsoon_rain();
    }

    // ... rest of function ...
}
```

### Step 6: Update Build Configuration

Edit `rules.mk`:

```makefile
# Add monsoon source files
SRC += seasons/monsoon/seasons_monsoon.c
```

---

## Adding a New Seasonal Event

Let's create a "Valentine's Day" event as an example (February 14).

### Step 1: Create Event Directory

```bash
cd keyboards/handwired/temanyl/chocmanyl36/keymaps/default/seasons/
mkdir valentine
```

### Step 2: Create Header File

Create `seasons/valentine/seasons_valentine.h`:

```c
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Valentine's Day event (Feb 14)

#define NUM_HEARTS 12
#define HEART_ANIMATION_SPEED 120  // Slow floating (120ms = 8.3fps)

// External state
extern bool valentine_initialized;
extern bool valentine_background_saved;
extern uint32_t valentine_animation_timer;

// Public functions
bool is_valentine_day(void);
void draw_valentine_elements(void);
void init_valentine_hearts(void);
void animate_valentine_hearts(void);
void reset_valentine_animations(void);
```

### Step 3: Create Implementation File

Create `seasons/valentine/seasons_valentine.c`:

```c
#include QMK_KEYBOARD_H
#include "seasons_valentine.h"
#include "../../display/display.h"
#include "../../display/framebuffer.h"

// State variables
bool valentine_initialized = false;
bool valentine_background_saved = false;
uint32_t valentine_animation_timer = 0;

// Heart structure
typedef struct {
    int16_t x;
    int16_t y;
    uint8_t phase;  // For floating motion
    uint8_t size;   // Heart size (small, medium, large)
} heart_t;

static heart_t hearts[NUM_HEARTS];

// External date
extern uint8_t current_month;
extern uint8_t current_day;

// Check if Valentine's Day
bool is_valentine_day(void) {
    return (current_month == 2 && current_day == 14);
}

// Draw a single heart
static void draw_heart(int16_t cx, int16_t cy, uint8_t size, uint8_t hue) {
    // Simple heart shape using circles and triangle
    // Left circle
    fb_circle_hsv(cx - size/2, cy, size/2, hue, 255, 255);
    // Right circle
    fb_circle_hsv(cx + size/2, cy, size/2, hue, 255, 255);
    // Bottom triangle (approximated with rectangles)
    for (int i = 0; i < size; i++) {
        int w = size - (i * size / (size + 2));
        fb_rect_hsv(cx - w/2, cy + i, w, 1, hue, 255, 255);
    }
}

// Initialize hearts
void init_valentine_hearts(void) {
    if (!valentine_initialized) {
        for (int i = 0; i < NUM_HEARTS; i++) {
            hearts[i].x = (i * 53 + 20) % 220;
            hearts[i].y = 30 + (i * 17) % 100;
            hearts[i].phase = (i * 21) % 160;
            hearts[i].size = 6 + (i % 3) * 3;  // 6, 9, or 12 pixels
        }
        valentine_initialized = true;
    }
}

// Animate floating hearts
void animate_valentine_hearts(void) {
    if (timer_elapsed32(valentine_animation_timer) < HEART_ANIMATION_SPEED) {
        return;
    }
    valentine_animation_timer = timer_read32();

    if (!valentine_background_saved) {
        fb_save_background();
        valentine_background_saved = true;
    }

    for (int i = 0; i < NUM_HEARTS; i++) {
        // Restore background (approximate bounds)
        int16_t s = hearts[i].size + 2;
        fb_restore_from_background(
            hearts[i].x - s, hearts[i].y - s,
            hearts[i].x + s, hearts[i].y + s
        );

        // Update floating motion (simple sine wave approximation)
        hearts[i].phase = (hearts[i].phase + 2) % 160;
        int8_t offset = (hearts[i].phase < 80) ?
            (hearts[i].phase / 10) : ((160 - hearts[i].phase) / 10);
        hearts[i].y += (offset > 4) ? -1 : 1;

        // Wrap vertically
        if (hearts[i].y < 20) hearts[i].y = 140;
        if (hearts[i].y > 140) hearts[i].y = 20;

        // Draw heart (pink hue = 0 or 330 degrees, use hue 0 for red-pink)
        draw_heart(hearts[i].x, hearts[i].y, hearts[i].size, 0);

        // Flush region
        int16_t s2 = hearts[i].size + 2;
        fb_flush_region(
            hearts[i].x - s2, hearts[i].y - s2,
            hearts[i].x + s2, hearts[i].y + s2
        );
    }
}

// Draw Valentine's elements (static)
void draw_valentine_elements(void) {
    init_valentine_hearts();
    // Could add static decorations here
}

// Reset Valentine's animations
void reset_valentine_animations(void) {
    valentine_initialized = false;
    valentine_background_saved = false;
}
```

### Step 4: Integrate into scenes.c

Edit `scenes/scenes.c`:

**Add include:**
```c
#include "../seasons/valentine/seasons_valentine.h"
```

**Update `reset_scene_animations()`:**
```c
void reset_scene_animations(void) {
    // ... existing resets ...
    reset_valentine_animations();
}
```

**Update `draw_seasonal_animation()`:**
```c
void draw_seasonal_animation(void) {
    // ... existing scene setup ...

    // Special events (checked after base season)
    if (is_valentine_day()) {
        draw_valentine_elements();
    } else if (is_halloween_event()) {
        draw_halloween_elements();
    } else if (is_christmas_season()) {
        draw_christmas_scene();
    }

    // ... rest of function ...
}
```

### Step 5: Call Animation in keymap.c

Edit `keymap.c`:

```c
void housekeeping_task_user(void) {
    // ... existing code ...

    // Trigger Valentine's animation
    if (is_valentine_day()) {
        animate_valentine_hearts();
    }

    // ... rest of function ...
}
```

### Step 6: Update Build Configuration

Edit `rules.mk`:

```makefile
# Add valentine source files
SRC += seasons/valentine/seasons_valentine.c
```

---

## Modifying Existing Seasons

### Example: Add a New Object to Winter

**Step 1:** Create or use existing object (e.g., `objects/fauna/penguin.h/c`)

**Step 2:** Edit `seasons/winter/seasons_winter.c`:

```c
#include "../../objects/fauna/penguin.h"

// Add state
static penguin_t penguin;

void draw_winter_scene(void) {
    // ... existing winter scene ...

    // Add penguin
    penguin_init(&penguin, 180, 120);
    penguin_draw(&penguin);
}
```

### Example: Change Event Dates

Edit the event detection function in the season file:

```c
// Old: Oct 28 - Nov 3
bool is_halloween_event(void) {
    if (current_month == 10 && current_day >= 28) return true;
    if (current_month == 11 && current_day <= 3) return true;
    return false;
}

// New: Oct 25 - Nov 5
bool is_halloween_event(void) {
    if (current_month == 10 && current_day >= 25) return true;
    if (current_month == 11 && current_day <= 5) return true;
    return false;
}
```

---

## Testing Your Season

### Step 1: Use Hard-Coded Date

Edit `display/display.c`:

```c
#define HARDCODE_DATE_TIME

#define HARDCODED_MONTH     2       // February
#define HARDCODED_DAY       14      // Valentine's Day
#define HARDCODED_YEAR      2025
#define HARDCODED_HOUR      14
#define HARDCODED_MINUTE    30
```

### Step 2: Build and Flash

```bash
make handwired/temanyl/chocmanyl36:default:flash
```

### Step 3: Verify Display

Check that:
- Static elements appear correctly
- Animations are smooth (no flickering)
- Frame rate is acceptable
- Colors match your design
- Elements don't overlap unexpectedly

### Step 4: Test Edge Cases

**Date transitions:**
```c
// Test day before event starts
#define HARDCODED_DAY 13

// Test day event starts
#define HARDCODED_DAY 14

// Test day event ends (if multi-day)
#define HARDCODED_DAY 15
```

**Time of day:**
```c
// Test at different hours to see celestial positioning
#define HARDCODED_HOUR 6   // Sunrise
#define HARDCODED_HOUR 12  // Noon
#define HARDCODED_HOUR 18  // Sunset
#define HARDCODED_HOUR 22  // Night
```

### Step 5: Performance Testing

Monitor for:
- **Smooth animation**: No stuttering or lag
- **Low CPU usage**: Other keyboard functions still responsive
- **Memory usage**: Firmware still fits in flash

If performance issues:
- Reduce number of animated objects
- Increase animation speed (lower FPS)
- Simplify drawing routines
- Use region flushing more aggressively

### Step 6: Reset and Test with Real Time

```c
// Comment out in display/display.c
// #define HARDCODE_DATE_TIME
```

Run companion script:
```bash
python3 keyboard_monitor.py
```

Verify that your season activates on the correct real-world date.

---

## Best Practices

### Performance

1. **Limit animated objects**: 20-30 moving elements max
2. **Use appropriate FPS**: 10-20 fps is smooth enough
3. **Optimize drawing**: Use simple shapes, avoid complex calculations
4. **Region-based flushing**: Only update changed areas

### Code Organization

1. **One season per directory**: Keep related files together
2. **Consistent naming**: `seasons_<name>.h/c`
3. **Clear state management**: Use static variables, expose only what's needed
4. **Document date ranges**: Comment when your event is active

### Visual Design

1. **Match existing style**: Keep consistent with other seasons
2. **Test at different times**: Ensure your elements work with sun/moon
3. **Color harmony**: Use HSV color space for easy palette creation
4. **Layering**: Static elements first, then animations

### Testing

1. **Always test with hard-coded dates first**
2. **Test date boundaries** (day before/after event)
3. **Test time of day variations**
4. **Remember to disable hard-coded dates before merging!**

---

## Common Pitfalls

### Animation Not Showing

**Symptom:** Static scene renders, but animation doesn't appear

**Fixes:**
- Check that animation function is called in `housekeeping_task_user()`
- Verify `is_<season>_event()` returns true
- Ensure animation timer is initialized
- Check that objects are being initialized

### Flickering Display

**Symptom:** Display flickers or tears

**Fixes:**
- Use `fb_save_background()` / `fb_restore_from_background()`
- Call `fb_flush_region()` not `fb_flush()` for animations
- Ensure you restore background before drawing new position

### Wrong Colors

**Symptom:** Colors don't match what you expect

**Fixes:**
- HSV values: Hue (0-255), Sat (0-255), Val (0-255)
- Hue wheel: 0=Red, 85=Green, 170=Blue, 21=Orange, 43=Yellow, 128=Cyan
- Test with simple solid colors first

### Memory Issues

**Symptom:** Firmware won't compile or keyboard crashes

**Fixes:**
- Reduce number of static variables
- Use smaller data types (uint8_t instead of uint16_t where possible)
- Reduce number of animated objects
- Check firmware size with `make handwired/temanyl/chocmanyl36:default`

### Date Detection Not Working

**Symptom:** Event doesn't activate on correct date

**Fixes:**
- Verify companion script is sending date/time
- Check `current_month` and `current_day` values
- Remember: months are 1-12, days are 1-31
- Test with hard-coded dates first

---

## Examples from Existing Code

### Simple Static Event (Pumpkins)

See: `seasons/halloween/seasons_halloween.c` - `draw_pumpkin_positions()`

Shows how to draw static decorations at fixed positions.

### Simple Animation (Ghosts)

See: `seasons/halloween/seasons_halloween.c` - `animate_ghosts()`

Shows timer-based animation with background restoration.

### Progressive Reveal (Christmas Advent)

See: `seasons/christmas/seasons_christmas.c` - `get_christmas_items_to_show()`

Shows how to reveal items progressively based on date.

### Moving Animation (Santa)

See: `seasons/christmas/seasons_christmas.c` - `update_santa_animation()`

Shows continuous horizontal movement with wrapping.

---

## Checklist for New Season/Event

- [ ] Created season directory in `seasons/`
- [ ] Created `seasons_<name>.h` with public interface
- [ ] Created `seasons_<name>.c` with implementation
- [ ] Added date detection function (`is_<name>_event()`)
- [ ] Added drawing function (`draw_<name>_scene()`)
- [ ] Added animation function (if animated)
- [ ] Added reset function (`reset_<name>_animations()`)
- [ ] Included header in `scenes/scenes.c`
- [ ] Called drawing function in `draw_seasonal_animation()`
- [ ] Called reset function in `reset_scene_animations()`
- [ ] Called animation function in `keymap.c` housekeeping (if animated)
- [ ] Added source file to `rules.mk`
- [ ] Tested with hard-coded dates
- [ ] Tested date boundaries
- [ ] Tested with real-time companion
- [ ] Disabled hard-coded dates before commit
- [ ] Documented in comments

---

## Need Help?

- Check existing seasons for reference implementations
- See `README.md` for architecture overview
- See `ANIMATIONS.md` for animation patterns
- See `OBJECTS.md` for creating new objects
