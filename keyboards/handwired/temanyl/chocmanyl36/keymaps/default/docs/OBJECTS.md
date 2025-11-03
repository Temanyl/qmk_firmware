# Creating Drawable Objects Guide

This guide explains how to create reusable drawable objects for the chocmanyl36 display system.

## Table of Contents

1. [Object System Overview](#object-system-overview)
2. [Creating a Static Object](#creating-a-static-object)
3. [Creating an Animated Object](#creating-an-animated-object)
4. [Object Categories](#object-categories)
5. [Best Practices](#best-practices)
6. [Examples](#examples)

---

## Object System Overview

### What is an Object?

An object is a self-contained, reusable visual element with its own:
- **State** (position, color, animation phase)
- **Behavior** (movement, animation logic)
- **Rendering** (how it draws itself)

### Unified Object Interface

**All objects follow a standard interface pattern:**

```c
// Standard methods (where applicable):
<type>_init(<type>_t* obj, ...params)           // Initialize single instance
<type>_draw(const <type>_t* obj, ...)           // Draw single instance
<type>_update(<type>_t* obj)                    // Update animation (animated objects)
<type>_get_bounds(const <type>_t* obj, ...)     // Get bounding box
<type>_contains_point(const <type>_t* obj, ...) // Point containment test
```

**Key Principles:**
- Objects operate on **single instances**, not arrays
- Array management happens in **scene managers**, not in object code
- All objects use **singular naming** (bird_draw, not birds_draw_all)

### Why Use Objects?

**Modularity:**
- Write once, use in multiple seasons/events
- Example: `cloud.c` used in winter, fall, and monsoon

**Maintainability:**
- All code for one element in one place
- Easy to modify without affecting other code
- Consistent interface across all object types

**Testability:**
- Can be tested independently
- Clear interface with standard method names
- No hidden dependencies on global arrays

**Object-Agnostic Scene Management:**
- Scene managers can treat all objects uniformly
- Same update/draw loop pattern for all objects
- Easy to add new object types

### Object Architecture

```
objects/
├── celestial/         # Sky elements (sun, moon, stars)
├── weather/          # Weather effects (rain, snow, smoke)
├── flora/            # Plant life (flowers, grass, trees)
├── fauna/            # Animals (birds, butterflies, bees)
├── structures/       # Buildings (cabin, tree, ground)
├── seasonal/         # Holiday-specific (ghost, pumpkin)
└── effects/          # Visual effects (sparkles, trails)
```

Each object has:
- `.h` file (interface/header)
- `.c` file (implementation)

---

## Creating a Static Object

Let's create a `flower` object that can be drawn at any position with any color.

### Step 1: Create Header File

Create `objects/flora/flower.h`:

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

// Flower dimensions
#define FLOWER_WIDTH 8
#define FLOWER_HEIGHT 12

// Flower object
typedef struct {
    int16_t x;          // Center position
    int16_t y;          // Center position
    uint8_t petal_hue;  // Color of petals
    uint8_t center_hue; // Color of center
    uint8_t stem_hue;   // Color of stem
} flower_t;

// Initialize a flower object
void flower_init(flower_t* flower, int16_t x, int16_t y, uint8_t petal_hue);

// Draw the flower at its position
void flower_draw(const flower_t* flower);

// Get bounding box (for region-based updates)
void flower_get_bounds(const flower_t* flower, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);
```

### Step 2: Create Implementation File

Create `objects/flora/flower.c`:

```c
/*
Copyright 2022 Joe Scotto

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
*/

#include "flower.h"
#include "../../display/framebuffer.h"

void flower_init(flower_t* flower, int16_t x, int16_t y, uint8_t petal_hue) {
    flower->x = x;
    flower->y = y;
    flower->petal_hue = petal_hue;
    flower->center_hue = 43;  // Yellow center
    flower->stem_hue = 85;    // Green stem
}

void flower_draw(const flower_t* flower) {
    // Draw stem (vertical line)
    fb_rect_hsv(
        flower->x - 1,       // x: centered, 2 pixels wide
        flower->y + 2,       // y: below flower head
        2,                   // width: 2 pixels
        6,                   // height: 6 pixels down
        flower->stem_hue,    // green
        255,                 // full saturation
        200                  // slightly dark
    );

    // Draw petals (5 circles around center)
    uint8_t petal_radius = 3;
    int8_t petal_offset = 3;

    // Top petal
    fb_circle_hsv(flower->x, flower->y - petal_offset, petal_radius,
                  flower->petal_hue, 255, 255);

    // Right petal
    fb_circle_hsv(flower->x + petal_offset, flower->y, petal_radius,
                  flower->petal_hue, 255, 255);

    // Bottom petal
    fb_circle_hsv(flower->x, flower->y + petal_offset, petal_radius,
                  flower->petal_hue, 255, 255);

    // Left petal
    fb_circle_hsv(flower->x - petal_offset, flower->y, petal_radius,
                  flower->petal_hue, 255, 255);

    // Draw center (yellow circle on top of petals)
    fb_circle_hsv(flower->x, flower->y, 2,
                  flower->center_hue, 255, 255);
}

void flower_get_bounds(const flower_t* flower, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    // Bounds include entire flower (petals + stem)
    *x1 = flower->x - 6;  // Petal extends 3 + radius 3 = 6
    *y1 = flower->y - 6;  // Top petal
    *x2 = flower->x + 6;  // Right petal
    *y2 = flower->y + 8;  // Stem extends 6 + 2 = 8 down
}
```

### Step 3: Add to Build System

Edit `rules.mk`:

```makefile
# Add flower source
SRC += objects/flora/flower.c
```

### Step 4: Use in Season

In your season file (e.g., `seasons/spring/seasons_spring.c`):

```c
#include "../../objects/flora/flower.h"

void draw_spring_scene(void) {
    // ... other spring elements ...

    // Draw some flowers
    flower_t flower1, flower2, flower3;

    flower_init(&flower1, 40, 130, 0);    // Red flower
    flower_init(&flower2, 80, 135, 170);  // Blue flower
    flower_init(&flower3, 120, 128, 300); // Pink flower

    flower_draw(&flower1);
    flower_draw(&flower2);
    flower_draw(&flower3);
}
```

---

## Creating an Animated Object

Let's create an animated `butterfly` object.

### Step 1: Create Header File

Create `objects/fauna/butterfly.h`:

```c
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Butterfly dimensions
#define BUTTERFLY_WIDTH 8
#define BUTTERFLY_HEIGHT 6

// Butterfly object
typedef struct {
    int16_t x;              // Position
    int16_t y;
    int8_t  vx;             // Velocity (horizontal)
    int8_t  vy;             // Velocity (vertical)
    uint8_t wing_phase;     // Wing flapping animation (0-255)
    uint8_t body_hue;       // Body color
    uint8_t wing_hue;       // Wing color
} butterfly_t;

// Initialize a butterfly
void butterfly_init(butterfly_t* butterfly, int16_t x, int16_t y, int8_t vx, int8_t vy, uint8_t wing_hue);

// Draw the butterfly
void butterfly_draw(const butterfly_t* butterfly);

// Update butterfly position and animation
void butterfly_update(butterfly_t* butterfly);

// Check if a point is inside the butterfly
bool butterfly_contains_point(const butterfly_t* butterfly, int16_t px, int16_t py);

// Get bounding box
void butterfly_get_bounds(const butterfly_t* butterfly, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);
```

### Step 2: Create Implementation File

Create `objects/fauna/butterfly.c`:

```c
#include "butterfly.h"
#include "../../display/framebuffer.h"

void butterfly_init(butterfly_t* butterfly, int16_t x, int16_t y, int8_t vx, int8_t vy, uint8_t wing_hue) {
    butterfly->x = x;
    butterfly->y = y;
    butterfly->vx = vx;
    butterfly->vy = vy;
    butterfly->wing_phase = 0;
    butterfly->body_hue = 0;    // Black body
    butterfly->wing_hue = wing_hue;
}

void butterfly_draw(const butterfly_t* butterfly) {
    // Wing flapping: phase 0-127 = open, 128-255 = closed
    bool wings_open = (butterfly->wing_phase < 128);
    uint8_t wing_spread = wings_open ? 3 : 1;

    // Draw left wing
    fb_circle_hsv(
        butterfly->x - wing_spread,   // Left of body
        butterfly->y,
        2,                            // Wing radius
        butterfly->wing_hue,
        255, 255
    );

    // Draw right wing
    fb_circle_hsv(
        butterfly->x + wing_spread,   // Right of body
        butterfly->y,
        2,                            // Wing radius
        butterfly->wing_hue,
        255, 255
    );

    // Draw body (small rectangle on top)
    fb_rect_hsv(
        butterfly->x - 1,
        butterfly->y - 1,
        2, 3,                         // 2x3 pixel body
        butterfly->body_hue,
        0, 100                        // Dark gray
    );
}

void butterfly_update(butterfly_t* butterfly) {
    // Update position
    butterfly->x += butterfly->vx;
    butterfly->y += butterfly->vy;

    // Wrap around screen horizontally
    if (butterfly->x < -10) butterfly->x = 250;
    if (butterfly->x > 250) butterfly->x = -10;

    // Bounce vertically (stay in upper portion of screen)
    if (butterfly->y < 20 || butterfly->y > 100) {
        butterfly->vy = -butterfly->vy;
    }

    // Update wing flapping animation
    butterfly->wing_phase += 8;  // Increment phase (wraps at 256)

    // Add slight randomness to flight (every 16 frames)
    if ((butterfly->wing_phase & 0x0F) == 0) {
        // Occasionally change direction slightly
        if (butterfly->wing_phase & 0x10) {
            butterfly->vy += (butterfly->vy > 0) ? -1 : 1;
        }
    }

    // Clamp velocity
    if (butterfly->vy > 2) butterfly->vy = 2;
    if (butterfly->vy < -2) butterfly->vy = -2;
}

bool butterfly_contains_point(const butterfly_t* butterfly, int16_t px, int16_t py) {
    // Simple rectangular bounds check
    return (px >= butterfly->x - 4 && px <= butterfly->x + 4 &&
            py >= butterfly->y - 3 && py <= butterfly->y + 3);
}

void butterfly_get_bounds(const butterfly_t* butterfly, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    // Bounds when wings are fully open
    *x1 = butterfly->x - 5;  // Left wing extended
    *y1 = butterfly->y - 3;
    *x2 = butterfly->x + 5;  // Right wing extended
    *y2 = butterfly->y + 3;
}
```

### Step 3: Add to Build System

Edit `rules.mk`:

```makefile
SRC += objects/fauna/butterfly.c
```

### Step 4: Use in Animated Season

**Key Point:** Arrays are managed by the scene manager, not the object.

In `seasons/spring/seasons_spring.h`:

```c
// Array size - exposed to other modules
#define NUM_SPRING_BUTTERFLIES 5
```

In `seasons/spring/seasons_spring.c`:

```c
#include "../../objects/fauna/butterfly.h"

// Animation state (scene manager owns the array)
#define NUM_SPRING_BUTTERFLIES 5
butterfly_t butterflies[NUM_SPRING_BUTTERFLIES];

static bool butterflies_initialized = false;
static bool butterflies_background_saved = false;
static uint32_t butterflies_animation_timer = 0;

// Configuration data
static const struct {
    int16_t x, y;
    int8_t vx, vy;
    uint8_t wing_hue;
} butterfly_config[NUM_SPRING_BUTTERFLIES] = {
    {40, 60, 1, 1, 300},    // Pink
    {100, 50, -1, 1, 43},   // Yellow
    {160, 70, 1, -1, 170},  // Blue
    {200, 55, -1, -1, 85},  // Green
    {80, 80, 1, 1, 0}       // Red
};

void init_spring_butterflies(void) {
    if (!butterflies_initialized) {
        // Initialize each butterfly using the unified interface
        for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
            butterfly_init(&butterflies[i],
                          butterfly_config[i].x,
                          butterfly_config[i].y,
                          butterfly_config[i].vx,
                          butterfly_config[i].vy,
                          butterfly_config[i].wing_hue);
        }
        butterflies_initialized = true;
    }
}

void animate_spring_butterflies(void) {
    // Throttle animation
    if (timer_elapsed32(butterflies_animation_timer) < 60) {
        return;
    }
    butterflies_animation_timer = timer_read32();

    // Save background on first frame
    if (!butterflies_background_saved) {
        fb_save_background();
        butterflies_background_saved = true;
    }

    // Update and draw each butterfly using the unified interface
    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        // Get old bounds
        int16_t x1, y1, x2, y2;
        butterfly_get_bounds(&butterflies[i], &x1, &y1, &x2, &y2);

        // Restore background
        fb_restore_from_background(x1, y1, x2, y2);

        // Update position and animation (single instance method)
        butterfly_update(&butterflies[i]);

        // Draw at new position (single instance method)
        butterfly_draw(&butterflies[i]);

        // Get new bounds and flush
        butterfly_get_bounds(&butterflies[i], &x1, &y1, &x2, &y2);
        fb_flush_region(x1, y1, x2, y2);
    }
}

void draw_spring_scene(void) {
    // ... other spring elements ...

    // Initialize butterflies
    init_spring_butterflies();
}

void reset_spring_animations(void) {
    butterflies_initialized = false;
    butterflies_background_saved = false;
}
```

**Note the pattern:**
- Scene manager declares array: `butterfly_t butterflies[NUM_SPRING_BUTTERFLIES]`
- Scene manager owns configuration data
- Scene manager loops calling single-instance methods: `butterfly_init(&butterflies[i], ...)`
- Object code has NO knowledge of arrays

---

## Object Categories

### Celestial Objects

**Location:** `objects/celestial/`

**Characteristics:**
- Usually static or slow-moving
- Position based on time of day
- Large, prominent elements

**Examples:**
- `sun.h/c` - Sun with calculated position based on hour
- `moon.h/c` - Moon with phase calculation
- `stars.h/c` - Background stars (optional twinkling)

**Template Structure:**
```c
typedef struct {
    int16_t x, y;
    uint8_t hour;        // For position calculation
    uint8_t brightness;  // For day/night transitions
} celestial_t;

void celestial_init(celestial_t* obj, uint8_t hour);
void celestial_draw(const celestial_t* obj);
void celestial_update_position(celestial_t* obj, uint8_t hour);
```

---

### Weather Objects

**Location:** `objects/weather/`

**Characteristics:**
- Many small particles
- Continuous spawning/despawning
- Simple shapes for performance

**Examples:**
- `raindrop.h/c` - Falling rain particles
- `snowflake.h/c` - Falling snow particles
- `cloud.h/c` - Drifting clouds
- `smoke.h/c` - Rising smoke particles

**Template Structure:**
```c
typedef struct {
    int16_t x, y;
    int8_t vx, vy;       // Velocity
    uint8_t lifetime;    // Frames until death
    bool active;         // Active/inactive flag
} weather_particle_t;

void particle_init(weather_particle_t* p, int16_t x, int16_t y);
void particle_draw(const weather_particle_t* p);
void particle_update(weather_particle_t* p);
bool particle_is_alive(const weather_particle_t* p);
```

---

### Flora Objects

**Location:** `objects/flora/`

**Characteristics:**
- Usually static (don't move)
- Can have gentle swaying animation
- Seasonal color variations

**Examples:**
- `flower.h/c` - Various flower types
- `grass.h/c` - Grass tufts
- `leaf.h/c` - Falling leaves

**Template Structure:**
```c
typedef struct {
    int16_t x, y;
    uint8_t type;        // Variety of plant
    uint8_t hue;         // Color (seasonal)
    uint8_t sway_phase;  // For gentle animation (optional)
} plant_t;

void plant_init(plant_t* p, int16_t x, int16_t y, uint8_t type, uint8_t hue);
void plant_draw(const plant_t* p);
void plant_update_sway(plant_t* p);  // Optional animation
```

---

### Fauna Objects

**Location:** `objects/fauna/`

**Characteristics:**
- Animated movement
- More complex behavior
- May have multiple animation states

**Examples:**
- `bird.h/c` - Flying birds with wing flapping
- `butterfly.h/c` - Fluttering butterflies
- `bee.h/c` - Buzzing bees
- `firefly.h/c` - Glowing fireflies

**Template Structure:**
```c
typedef struct {
    int16_t x, y;
    int8_t vx, vy;
    uint8_t animation_phase;    // For wing flapping, etc.
    uint8_t behavior_state;     // For complex behaviors
} creature_t;

void creature_init(creature_t* c, int16_t x, int16_t y);
void creature_draw(const creature_t* c);
void creature_update(creature_t* c);
void creature_get_bounds(const creature_t* c, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);
```

---

### Structure Objects

**Location:** `objects/structures/`

**Characteristics:**
- Large, complex, static
- May have seasonal variations
- Usually one or few instances

**Examples:**
- `tree.h/c` - Trees with seasonal foliage
- `cabin.h/c` - Cabin with chimney
- `ground.h/c` - Ground/terrain

**Template Structure:**
```c
typedef struct {
    int16_t x, y;          // Base position
    uint8_t season;        // Which season's appearance
    uint8_t foliage_hue;   // Seasonal color
    uint8_t structure_hue; // Base structure color
} structure_t;

void structure_init(structure_t* s, int16_t x, int16_t y, uint8_t season, ...);
void structure_draw(const structure_t* s);
void structure_update_season(structure_t* s, uint8_t season);
```

---

### Seasonal Objects

**Location:** `objects/seasonal/`

**Characteristics:**
- Event-specific
- Often unique behaviors
- Only appear during certain dates

**Examples:**
- `ghost.h/c` - Halloween ghost
- `pumpkin.h/c` - Halloween jack-o'-lantern
- `snowman.h/c` - Winter snowman

**Template Structure:**
```c
typedef struct {
    int16_t x, y;
    // Event-specific fields
    uint8_t special_animation;
} seasonal_object_t;

void seasonal_init(seasonal_object_t* obj, int16_t x, int16_t y);
void seasonal_draw(const seasonal_object_t* obj);
void seasonal_update(seasonal_object_t* obj);
```

---

## Best Practices

### 1. Encapsulation

**Good:** All object state in struct, all behavior in functions
```c
// Good: Self-contained
typedef struct {
    int16_t x, y;
    uint8_t hue;
} flower_t;

void flower_init(flower_t* f, int16_t x, int16_t y, uint8_t hue);
void flower_draw(const flower_t* f);
```

**Bad:** External state, tightly coupled
```c
// Bad: Scattered state
int16_t flower_x;      // Global variables
int16_t flower_y;
uint8_t flower_hue;

void draw_flower(void);  // Depends on globals
```

### 2. Const Correctness

Use `const` for functions that don't modify the object:

```c
// Draw doesn't modify object, use const
void object_draw(const object_t* obj);

// Update modifies object, no const
void object_update(object_t* obj);
```

### 3. Small, Focused Objects

**Good:** One object, one purpose
```c
// Good: Raindrop does one thing
typedef struct {
    int16_t x, y;
    int8_t speed;
} raindrop_t;
```

**Bad:** Object with too many responsibilities
```c
// Bad: Weather does everything
typedef struct {
    raindrop_t rain[100];
    snowflake_t snow[100];
    cloud_t clouds[5];
    // ... too much in one object
} weather_t;
```

### 4. Efficient Bounds Calculation

Always provide a `get_bounds()` function for animated objects:

```c
void object_get_bounds(const object_t* obj, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    // Add 1-pixel margin for safety
    *x1 = obj->x - obj->width/2 - 1;
    *y1 = obj->y - obj->height/2 - 1;
    *x2 = obj->x + obj->width/2 + 1;
    *y2 = obj->y + obj->height/2 + 1;

    // Clamp to screen bounds
    if (*x1 < 0) *x1 = 0;
    if (*y1 < 0) *y1 = 0;
    if (*x2 > 239) *x2 = 239;
    if (*y2 > 154) *y2 = 154;
}
```

### 5. Use Standard Drawing Functions

Always use framebuffer functions for drawing:

```c
// Available framebuffer functions:
fb_rect_hsv(x, y, width, height, hue, sat, val);
fb_circle_hsv(cx, cy, radius, hue, sat, val);
fb_line_hsv(x1, y1, x2, y2, hue, sat, val);
fb_pixel_hsv(x, y, hue, sat, val);
```

### 6. Memory Efficiency

Use appropriate data types:

```c
// Efficient: 7 bytes per object
typedef struct {
    int16_t x, y;      // 4 bytes (screen is 240x155)
    int8_t vx, vy;     // 2 bytes (velocity < 127)
    uint8_t phase;     // 1 byte
} object_t;

// Wasteful: 20 bytes per object
typedef struct {
    int32_t x, y;      // 8 bytes (overkill)
    int32_t vx, vy;    // 8 bytes (overkill)
    uint32_t phase;    // 4 bytes (overkill)
} wasteful_object_t;
```

---

## Examples

### Example 1: Simple Static Decoration

**Pumpkin** (Halloween decoration):

See: `objects/seasonal/pumpkin.c`

- Static position
- Simple shape (circle + triangle)
- No animation
- Used multiple times at different positions

### Example 2: Simple Particle

**Raindrop** (Weather effect):

See: `objects/weather/raindrop.c`

- Vertical movement only
- Resets at bottom
- Very simple drawing (2x4 pixel rectangle)
- Used in arrays of 50+

### Example 3: Complex Animation

**Ghost** (Halloween creature):

See: `objects/seasonal/ghost.c`

- Horizontal + vertical sine wave motion
- Screen wrapping
- Multiple animation phases
- Background restoration required

### Example 4: Stateful Object

**Tree** (Structure with seasonal variations):

See: `objects/structures/tree.c`

- Changes appearance based on season
- Large, complex drawing
- Static but visually detailed
- Single or few instances

---

## Object Checklist

When creating a new object:

- [ ] Created header file in appropriate category directory
- [ ] Created implementation file
- [ ] Defined object struct with minimal necessary state
- [ ] Implemented `init()` function
- [ ] Implemented `draw()` function (uses `const`)
- [ ] Implemented `update()` function (if animated)
- [ ] Implemented `get_bounds()` function (if animated)
- [ ] Implemented `contains_point()` function (if needed for hit detection)
- [ ] Used appropriate data types (int16_t for position, int8_t for velocity)
- [ ] Used framebuffer drawing functions (fb_rect_hsv, fb_circle_hsv)
- [ ] Added copyright header
- [ ] Added to rules.mk build configuration
- [ ] Tested drawing at various positions
- [ ] Tested animation (if applicable)
- [ ] Tested bounds calculation (if applicable)
- [ ] Verified no memory leaks or buffer overruns
- [ ] Documented any special requirements in comments

---

## Common Patterns

### Pattern: Object Pool

For weather particles that spawn/despawn:

```c
#define MAX_PARTICLES 50

typedef struct {
    int16_t x, y;
    bool active;
} particle_t;

static particle_t pool[MAX_PARTICLES];

void particle_emit(int16_t x, int16_t y) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!pool[i].active) {
            pool[i].x = x;
            pool[i].y = y;
            pool[i].active = true;
            return;
        }
    }
}

void particle_update_all(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (pool[i].active) {
            particle_update(&pool[i]);
            if (particle_should_die(&pool[i])) {
                pool[i].active = false;
            }
        }
    }
}
```

### Pattern: Seasonal Variation

Objects that change appearance by season:

```c
void tree_draw(const tree_t* tree) {
    // Draw trunk (same for all seasons)
    fb_rect_hsv(tree->x - 2, tree->y, 4, 20, 21, 200, 100);

    // Draw foliage (varies by season)
    uint8_t foliage_hue, foliage_sat, foliage_val;

    switch(tree->season) {
        case WINTER:
            foliage_hue = 0; foliage_sat = 0; foliage_val = 255;  // White
            break;
        case SPRING:
            foliage_hue = 85; foliage_sat = 255; foliage_val = 200;  // Light green
            break;
        case SUMMER:
            foliage_hue = 85; foliage_sat = 255; foliage_val = 255;  // Bright green
            break;
        case FALL:
            foliage_hue = 21; foliage_sat = 255; foliage_val = 255;  // Orange
            break;
    }

    fb_circle_hsv(tree->x, tree->y - 15, 15, foliage_hue, foliage_sat, foliage_val);
}
```

---

## Resources

- See `README.md` for architecture overview
- See `SEASONS.md` for using objects in seasons
- See `ANIMATIONS.md` for animating objects
- Study existing objects in `objects/` subdirectories
- Check `display/framebuffer.h` for available drawing functions
