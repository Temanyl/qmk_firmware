# Creating Animations Guide

This guide explains how to create smooth, efficient animations for the chocmanyl36 display system.

## Table of Contents

1. [Animation Fundamentals](#animation-fundamentals)
2. [Simple Animation Example](#simple-animation-example)
3. [Advanced Animation Patterns](#advanced-animation-patterns)
4. [Optimization Techniques](#optimization-techniques)
5. [Troubleshooting](#troubleshooting)

---

## Animation Fundamentals

### How Animations Work

The chocmanyl36 uses a **background buffering** system to achieve smooth animations:

1. **Draw static scene** to framebuffer
2. **Save scene** to background buffer
3. **Each frame:**
   - Restore background where object was
   - Update object position
   - Draw object at new position
   - Flush only changed region

This approach is much faster than redrawing the entire scene each frame.

### Key Concepts

**Timer-Based Updates:**
- Use QMK's `timer_read32()` and `timer_elapsed32()` functions
- Throttle to desired frame rate (10-20 fps is smooth)
- Independent timers for different animations

**Region-Based Flushing:**
- Only update pixels that changed
- Use `fb_flush_region(x1, y1, x2, y2)` not `fb_flush()`
- Reduces SPI transfer time significantly

**Background Restoration:**
- Save static scene once with `fb_save_background()`
- Restore regions with `fb_restore_from_background(x1, y1, x2, y2)`
- Avoids redrawing static elements

---

## Simple Animation Example

Let's create a bouncing ball animation.

### Step 1: Define Object Structure

In your object header (e.g., `objects/effects/ball.h`):

```c
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define BALL_RADIUS 5

typedef struct {
    int16_t x;        // Current position
    int16_t y;
    int8_t  vx;       // Velocity
    int8_t  vy;
    uint8_t hue;      // Color
} ball_t;

void ball_init(ball_t* ball, int16_t x, int16_t y, int8_t vx, int8_t vy, uint8_t hue);
void ball_draw(const ball_t* ball);
void ball_update(ball_t* ball);
void ball_get_bounds(const ball_t* ball, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);
```

### Step 2: Implement Object Functions

In `objects/effects/ball.c`:

```c
#include "ball.h"
#include "../../display/framebuffer.h"

void ball_init(ball_t* ball, int16_t x, int16_t y, int8_t vx, int8_t vy, uint8_t hue) {
    ball->x = x;
    ball->y = y;
    ball->vx = vx;
    ball->vy = vy;
    ball->hue = hue;
}

void ball_draw(const ball_t* ball) {
    fb_circle_hsv(ball->x, ball->y, BALL_RADIUS, ball->hue, 255, 255);
}

void ball_update(ball_t* ball) {
    // Update position
    ball->x += ball->vx;
    ball->y += ball->vy;

    // Bounce off walls (assuming 240x155 framebuffer)
    if (ball->x - BALL_RADIUS < 0 || ball->x + BALL_RADIUS > 239) {
        ball->vx = -ball->vx;
        ball->x += ball->vx;  // Move away from wall
    }

    if (ball->y - BALL_RADIUS < 0 || ball->y + BALL_RADIUS > 154) {
        ball->vy = -ball->vy;
        ball->y += ball->vy;
    }
}

void ball_get_bounds(const ball_t* ball, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    *x1 = ball->x - BALL_RADIUS - 1;
    *y1 = ball->y - BALL_RADIUS - 1;
    *x2 = ball->x + BALL_RADIUS + 1;
    *y2 = ball->y + BALL_RADIUS + 1;
}
```

### Step 3: Add Animation to Season

In your season file (e.g., `seasons/spring/seasons_spring.c`):

```c
#include "../../objects/effects/ball.h"

// Animation constants
#define NUM_BALLS 3
#define BALL_ANIMATION_SPEED 50  // 50ms = 20fps

// State variables
static bool balls_initialized = false;
static bool balls_background_saved = false;
static uint32_t balls_animation_timer = 0;
static ball_t balls[NUM_BALLS];

// Initialize balls
void init_spring_balls(void) {
    if (!balls_initialized) {
        ball_init(&balls[0], 40, 40, 2, 3, 85);   // Green
        ball_init(&balls[1], 120, 60, -3, 2, 128); // Cyan
        ball_init(&balls[2], 200, 80, 2, -2, 43);  // Yellow

        balls_initialized = true;
    }
}

// Animate balls
void animate_spring_balls(void) {
    // Throttle to desired frame rate
    if (timer_elapsed32(balls_animation_timer) < BALL_ANIMATION_SPEED) {
        return;
    }
    balls_animation_timer = timer_read32();

    // Save background on first frame
    if (!balls_background_saved) {
        fb_save_background();
        balls_background_saved = true;
    }

    // Update each ball
    for (int i = 0; i < NUM_BALLS; i++) {
        // Get old bounds
        int16_t x1, y1, x2, y2;
        ball_get_bounds(&balls[i], &x1, &y1, &x2, &y2);

        // Restore background at old position
        fb_restore_from_background(x1, y1, x2, y2);

        // Update position
        ball_update(&balls[i]);

        // Draw at new position
        ball_draw(&balls[i]);

        // Get new bounds
        ball_get_bounds(&balls[i], &x1, &y1, &x2, &y2);

        // Flush changed region
        fb_flush_region(x1, y1, x2, y2);
    }
}

// Reset animation
void reset_spring_animations(void) {
    balls_initialized = false;
    balls_background_saved = false;
}

// Call from draw function
void draw_spring_scene(void) {
    // ... draw static spring elements ...

    // Initialize balls
    init_spring_balls();
}
```

### Step 4: Call from Housekeeping

In `keymap.c`:

```c
void housekeeping_task_user(void) {
    // ... other code ...

    // Trigger spring ball animation
    if (get_season(current_month) == SPRING) {
        animate_spring_balls();
    }

    // ... other code ...
}
```

---

## Advanced Animation Patterns

### Pattern 1: Sine Wave Motion

For smooth floating/oscillating motion:

```c
// In object update function
void ghost_update(ghost_t* ghost) {
    // Horizontal movement
    ghost->x += ghost->vx;

    // Vertical sine wave (approximate)
    // phase goes from 0-159, sine approximation gives -8 to +8
    ghost->phase = (ghost->phase + 2) % 160;

    int8_t sine_approx;
    if (ghost->phase < 40) {
        sine_approx = (ghost->phase * 8) / 40;  // 0 to 8
    } else if (ghost->phase < 80) {
        sine_approx = 8 - ((ghost->phase - 40) * 16) / 40;  // 8 to -8
    } else if (ghost->phase < 120) {
        sine_approx = -8 + ((ghost->phase - 80) * 16) / 40;  // -8 to 8
    } else {
        sine_approx = 8 - ((ghost->phase - 120) * 8) / 40;  // 8 to 0
    }

    ghost->y = ghost->base_y + sine_approx;

    // Wrap horizontally
    if (ghost->x < -10) ghost->x = 250;
    if (ghost->x > 250) ghost->x = -10;
}
```

### Pattern 2: Particle Systems

For many small particles (rain, snow, sparks):

```c
#define NUM_PARTICLES 50

typedef struct {
    int16_t x, y;
    int8_t vx, vy;
    uint8_t lifetime;  // Frames until particle dies
    bool active;
} particle_t;

static particle_t particles[NUM_PARTICLES];

void emit_particle(int16_t x, int16_t y, int8_t vx, int8_t vy) {
    // Find inactive particle
    for (int i = 0; i < NUM_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = vx;
            particles[i].vy = vy;
            particles[i].lifetime = 100;  // 100 frames
            particles[i].active = true;
            break;
        }
    }
}

void update_particles(void) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        if (!particles[i].active) continue;

        // Restore background
        fb_restore_from_background(
            particles[i].x - 1, particles[i].y - 1,
            particles[i].x + 1, particles[i].y + 1
        );

        // Update
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].lifetime--;

        // Deactivate if dead
        if (particles[i].lifetime == 0 || particles[i].y > 155) {
            particles[i].active = false;
            continue;
        }

        // Draw
        fb_rect_hsv(particles[i].x, particles[i].y, 2, 2, 170, 255, 255);

        // Flush
        fb_flush_region(
            particles[i].x - 1, particles[i].y - 1,
            particles[i].x + 1, particles[i].y + 1
        );
    }
}
```

### Pattern 3: Sprite Animation (Frame-Based)

For character animations with multiple frames:

```c
typedef struct {
    int16_t x, y;
    uint8_t current_frame;
    uint8_t num_frames;
    uint32_t frame_timer;
    uint16_t frame_duration;  // ms per frame
} animated_sprite_t;

void draw_sprite_frame(const animated_sprite_t* sprite) {
    // Draw different graphics based on current_frame
    switch(sprite->current_frame) {
        case 0:
            // Draw frame 0
            fb_rect_hsv(sprite->x, sprite->y, 10, 15, 0, 255, 255);
            break;
        case 1:
            // Draw frame 1
            fb_rect_hsv(sprite->x, sprite->y + 2, 10, 13, 0, 255, 255);
            break;
        case 2:
            // Draw frame 2
            fb_rect_hsv(sprite->x, sprite->y, 10, 15, 0, 255, 255);
            break;
        case 3:
            // Draw frame 3
            fb_rect_hsv(sprite->x, sprite->y - 2, 10, 17, 0, 255, 255);
            break;
    }
}

void update_sprite_animation(animated_sprite_t* sprite) {
    if (timer_elapsed32(sprite->frame_timer) >= sprite->frame_duration) {
        sprite->frame_timer = timer_read32();

        // Advance to next frame
        sprite->current_frame = (sprite->current_frame + 1) % sprite->num_frames;

        // Redraw with new frame
        // (restoration and flushing handled elsewhere)
        draw_sprite_frame(sprite);
    }
}
```

### Pattern 4: Path Following

For objects that follow a predefined path:

```c
typedef struct {
    int16_t x, y;
} point_t;

typedef struct {
    point_t* path;       // Array of points
    uint8_t num_points;
    uint8_t current_point;
    int16_t x, y;        // Current position
    uint8_t speed;       // Pixels per frame
} path_follower_t;

void path_follower_update(path_follower_t* obj) {
    // Get target point
    point_t target = obj->path[obj->current_point];

    // Calculate direction
    int16_t dx = target.x - obj->x;
    int16_t dy = target.y - obj->y;

    // Calculate distance
    int16_t dist_sq = dx*dx + dy*dy;

    if (dist_sq < obj->speed * obj->speed) {
        // Reached waypoint, move to next
        obj->current_point = (obj->current_point + 1) % obj->num_points;
    } else {
        // Move towards target
        // Normalize and scale by speed (simplified)
        if (dx > 0) obj->x += (dx > obj->speed) ? obj->speed : dx;
        else if (dx < 0) obj->x += (dx < -obj->speed) ? -obj->speed : dx;

        if (dy > 0) obj->y += (dy > obj->speed) ? obj->speed : dy;
        else if (dy < 0) obj->y += (dy < -obj->speed) ? -obj->speed : dy;
    }
}

// Usage example:
point_t santa_path[] = {
    {-20, 40},    // Start off-screen left
    {120, 35},    // Middle of screen, slight arc
    {260, 40}     // End off-screen right
};

path_follower_t santa;
santa.path = santa_path;
santa.num_points = 3;
santa.current_point = 0;
santa.x = santa_path[0].x;
santa.y = santa_path[0].y;
santa.speed = 3;
```

### Pattern 5: Time-Based Emission

For spawning objects at intervals (smoke puffs, fireworks):

```c
static uint32_t spawn_timer = 0;
static uint16_t spawn_interval = 800;  // 800ms between spawns

void update_smoke_emission(void) {
    if (timer_elapsed32(spawn_timer) >= spawn_interval) {
        spawn_timer = timer_read32();

        // Emit new smoke particle
        emit_smoke_particle(chimney_x, chimney_y);

        // Randomize next interval (optional)
        spawn_interval = 700 + (timer_read32() % 300);  // 700-1000ms
    }

    // Update existing particles
    update_smoke_particles();
}

void emit_smoke_particle(int16_t x, int16_t y) {
    // Find inactive particle slot
    for (int i = 0; i < NUM_SMOKE; i++) {
        if (!smoke[i].active) {
            smoke_particle_init(&smoke[i], x, y);
            smoke[i].active = true;
            break;
        }
    }
}
```

---

## Optimization Techniques

### 1. Minimize Region Size

Only flush the smallest rectangle that contains changes:

```c
// Bad: flush entire object bounds every time
void animate_bad(void) {
    for (int i = 0; i < num_objects; i++) {
        object_get_bounds(&objects[i], &x1, &y1, &x2, &y2);
        fb_restore_from_background(x1, y1, x2, y2);
        object_update(&objects[i]);
        object_draw(&objects[i]);
        fb_flush_region(x1, y1, x2, y2);  // Always full bounds
    }
}

// Good: flush union of old and new positions
void animate_good(void) {
    for (int i = 0; i < num_objects; i++) {
        int16_t old_x1, old_y1, old_x2, old_y2;
        object_get_bounds(&objects[i], &old_x1, &old_y1, &old_x2, &old_y2);

        fb_restore_from_background(old_x1, old_y1, old_x2, old_y2);
        object_update(&objects[i]);
        object_draw(&objects[i]);

        int16_t new_x1, new_y1, new_x2, new_y2;
        object_get_bounds(&objects[i], &new_x1, &new_y1, &new_x2, &new_y2);

        // Flush union of old and new bounds
        int16_t x1 = (old_x1 < new_x1) ? old_x1 : new_x1;
        int16_t y1 = (old_y1 < new_y1) ? old_y1 : new_y1;
        int16_t x2 = (old_x2 > new_x2) ? old_x2 : new_x2;
        int16_t y2 = (old_y2 > new_y2) ? old_y2 : new_y2;

        fb_flush_region(x1, y1, x2, y2);
    }
}
```

### 2. Batch Overlapping Regions

If multiple objects move in the same area, flush once:

```c
void animate_batched(void) {
    int16_t dirty_x1 = 240, dirty_y1 = 155;
    int16_t dirty_x2 = 0, dirty_y2 = 0;
    bool any_dirty = false;

    for (int i = 0; i < num_objects; i++) {
        int16_t x1, y1, x2, y2;
        object_get_bounds(&objects[i], &x1, &y1, &x2, &y2);

        fb_restore_from_background(x1, y1, x2, y2);
        object_update(&objects[i]);
        object_draw(&objects[i]);

        // Expand dirty region
        if (x1 < dirty_x1) dirty_x1 = x1;
        if (y1 < dirty_y1) dirty_y1 = y1;
        if (x2 > dirty_x2) dirty_x2 = x2;
        if (y2 > dirty_y2) dirty_y2 = y2;
        any_dirty = true;
    }

    // Single flush for all objects
    if (any_dirty) {
        fb_flush_region(dirty_x1, dirty_y1, dirty_x2, dirty_y2);
    }
}
```

### 3. Skip Unchanged Frames

If object didn't move, don't redraw:

```c
typedef struct {
    int16_t x, y;
    int16_t last_x, last_y;  // Track previous position
    // ... other fields
} smart_object_t;

void smart_object_update(smart_object_t* obj) {
    obj->last_x = obj->x;
    obj->last_y = obj->y;

    // Update position
    obj->x += obj->vx;
    obj->y += obj->vy;
}

bool smart_object_moved(const smart_object_t* obj) {
    return (obj->x != obj->last_x || obj->y != obj->last_y);
}

// In animation loop:
for (int i = 0; i < num_objects; i++) {
    smart_object_update(&objects[i]);

    if (smart_object_moved(&objects[i])) {
        // Only redraw if position changed
        // ... restore, draw, flush ...
    }
}
```

### 4. Reduce Draw Complexity

Simple shapes are faster than complex ones:

```c
// Slow: Complex ghost with many pixels
void draw_ghost_complex(ghost_t* ghost) {
    // 50+ drawing calls for detailed ghost
    for (int i = 0; i < 50; i++) {
        fb_rect_hsv(/* ... */);
    }
}

// Fast: Simple ghost with fewer shapes
void draw_ghost_simple(ghost_t* ghost) {
    // Just 3-4 shapes
    fb_circle_hsv(ghost->x, ghost->y, 7, 0, 0, 255);  // White body
    fb_rect_hsv(ghost->x - 7, ghost->y + 3, 14, 10, 0, 0, 255);  // Bottom
    fb_circle_hsv(ghost->x - 3, ghost->y - 2, 2, 0, 0, 0);  // Eye
    fb_circle_hsv(ghost->x + 3, ghost->y - 2, 2, 0, 0, 0);  // Eye
}
```

### 5. Adjust Frame Rate Dynamically

Lower FPS when keyboard is busy:

```c
#define ANIMATION_SPEED_NORMAL 50   // 20fps
#define ANIMATION_SPEED_SLOW 100    // 10fps

uint16_t current_animation_speed = ANIMATION_SPEED_NORMAL;

void housekeeping_task_user(void) {
    // If typing detected, slow down animations
    static uint32_t last_activity = 0;
    if (last_matrix_activity_time() > last_activity) {
        current_animation_speed = ANIMATION_SPEED_SLOW;
        last_activity = last_matrix_activity_time();
    } else if (timer_elapsed32(last_activity) > 5000) {
        // No typing for 5 seconds, resume normal speed
        current_animation_speed = ANIMATION_SPEED_NORMAL;
    }

    // Use dynamic speed
    if (timer_elapsed32(animation_timer) < current_animation_speed) {
        return;
    }
    // ... animate ...
}
```

---

## Troubleshooting

### Problem: Animation is Stuttering

**Symptoms:**
- Uneven motion
- Occasional freezes
- Jerky movement

**Solutions:**

1. **Check timer throttling:**
```c
// Make sure you're using timer_elapsed32 correctly
if (timer_elapsed32(animation_timer) < ANIMATION_SPEED) {
    return;  // Don't update yet
}
animation_timer = timer_read32();  // Reset timer
```

2. **Reduce complexity:**
```c
// Too many objects?
#define NUM_PARTICLES 80  // Try reducing to 40-50
```

3. **Simplify drawing:**
```c
// Replace complex shapes with simpler ones
// Use circles/rects instead of custom pixel-by-pixel drawing
```

### Problem: Display Flickering

**Symptoms:**
- Brief flashes
- Visible "tearing"
- Elements appear/disappear randomly

**Solutions:**

1. **Use background restoration:**
```c
// Always restore before drawing new position
fb_restore_from_background(old_x, old_y, old_x + w, old_y + h);
object_draw(&obj);
```

2. **Flush after drawing, not before:**
```c
// Wrong order:
fb_flush_region(x, y, x + w, y + h);
object_draw(&obj);  // Won't show up!

// Correct order:
object_draw(&obj);
fb_flush_region(x, y, x + w, y + h);
```

3. **Save background after drawing static scene:**
```c
void draw_seasonal_animation(void) {
    // Draw all static elements
    draw_sky();
    draw_trees();
    draw_cabin();

    // THEN save background
    fb_save_background();  // Must be after static elements

    // THEN init animations
    init_rain();
}
```

### Problem: Objects Disappearing

**Symptoms:**
- Objects vanish after a while
- Parts of objects missing
- Trails left behind

**Solutions:**

1. **Check bounds before restoring:**
```c
// Ensure bounds are correct and within screen
void object_get_bounds(const object_t* obj, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    *x1 = obj->x - obj->width / 2;
    *y1 = obj->y - obj->height / 2;
    *x2 = obj->x + obj->width / 2;
    *y2 = obj->y + obj->height / 2;

    // Clamp to screen bounds
    if (*x1 < 0) *x1 = 0;
    if (*y1 < 0) *y1 = 0;
    if (*x2 > 239) *x2 = 239;
    if (*y2 > 154) *y2 = 154;
}
```

2. **Restore full bounds:**
```c
// Make bounds slightly larger to catch any edge pixels
void object_get_bounds(const object_t* obj, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    *x1 = obj->x - obj->radius - 1;  // +1 pixel margin
    *y1 = obj->y - obj->radius - 1;
    *x2 = obj->x + obj->radius + 1;
    *y2 = obj->y + obj->radius + 1;
}
```

### Problem: Animation Too Fast/Slow

**Symptoms:**
- Objects move too quickly or too slowly
- Doesn't match expected speed

**Solutions:**

1. **Adjust animation speed (frame rate):**
```c
// Slower (lower FPS)
#define ANIMATION_SPEED 100  // 10 fps instead of 20fps

// Faster (higher FPS)
#define ANIMATION_SPEED 33   // 30 fps instead of 20fps
```

2. **Adjust velocity:**
```c
// Slower movement
object->vx = 1;  // Instead of 2 or 3

// Faster movement
object->vx = 5;  // Instead of 2 or 3
```

3. **Combine both:**
```c
// Smooth slow motion: Low velocity + high FPS
#define ANIMATION_SPEED 33   // 30 fps
object->vx = 1;  // 1 pixel per frame = 30 pixels/second

// Fast motion: High velocity + medium FPS
#define ANIMATION_SPEED 50   // 20 fps
object->vx = 4;  // 4 pixels per frame = 80 pixels/second
```

### Problem: Memory Errors

**Symptoms:**
- Firmware won't compile
- Keyboard crashes
- Random behavior

**Solutions:**

1. **Reduce static allocations:**
```c
// Too much:
#define NUM_PARTICLES 200  // 200 * sizeof(particle_t) may be too much

// Better:
#define NUM_PARTICLES 50   // Reduce to what you actually need
```

2. **Use smaller data types:**
```c
// Wasteful:
typedef struct {
    int32_t x, y;      // 8 bytes
    int32_t vx, vy;    // 8 bytes
    uint32_t phase;    // 4 bytes
} object_t;  // 20 bytes total

// Efficient:
typedef struct {
    int16_t x, y;      // 4 bytes (screen is only 240x155)
    int8_t  vx, vy;    // 2 bytes (velocity rarely needs >127)
    uint8_t phase;     // 1 byte (0-255 is enough for phase)
} object_t;  // 7 bytes total (65% savings!)
```

3. **Check firmware size:**
```bash
# After build, check output:
make handwired/temanyl/chocmanyl36:default

# Look for:
# Creating load file for flashing: .build/handwired_temanyl_chocmanyl36_default.uf2
# Size of binary: XXXXX bytes

# If close to RP2040 limits, reduce features
```

---

## Animation Checklist

Before committing your animation:

- [ ] Smooth motion (no stuttering)
- [ ] No flickering or tearing
- [ ] Frame rate is 10-20 fps (appropriate for visual smoothness)
- [ ] Uses background restoration
- [ ] Flushes only changed regions
- [ ] Objects don't leave trails
- [ ] Objects don't disappear
- [ ] Bounds checking prevents off-screen access
- [ ] Timer throttling implemented correctly
- [ ] Reset function clears all state
- [ ] Works with layer changes (doesn't break)
- [ ] Tested with hard-coded dates
- [ ] Memory usage is reasonable
- [ ] No compiler warnings

---

## Performance Guidelines

### Target Frame Rates

| Animation Type | Target FPS | Timer Speed (ms) | Use Case |
|----------------|-----------|------------------|----------|
| Slow drift | 8-10 | 100-125 | Santa, clouds |
| Smooth motion | 12-15 | 67-83 | Ghosts, butterflies |
| Fast action | 20 | 50 | Rain, snow, bouncing |
| Very fast | 30 | 33 | Games, reactive effects |

### Object Limits

| Element Type | Max Count | Notes |
|--------------|-----------|-------|
| Large objects (>15px) | 5-10 | Trees, buildings, characters |
| Medium objects (5-15px) | 10-20 | Ghosts, birds, balls |
| Small particles (< 5px) | 50-100 | Rain, snow, stars |
| Tiny particles (1-2px) | 100-200 | Sparks, dust, effects |

---

## Example Animations to Study

**Simple linear motion:**
- Rain (seasons/fall/) - Vertical fall with reset
- Santa (seasons/christmas/) - Horizontal flight

**Sine wave floating:**
- Ghosts (seasons/halloween/) - Smooth oscillation

**Particle system:**
- Smoke (objects/weather/smoke.c) - Time-based emission
- Snowflakes (seasons/winter/) - Many small particles

**Bouncing physics:**
- Doodle Jump (game_doodle.c) - Gravity and platform collision

**Path following:**
- Santa sleigh - Predefined arc across screen

---

## Resources

- See `README.md` for architecture overview
- See `SEASONS.md` for integrating animations into seasons
- See `OBJECTS.md` for creating animated objects
- Study existing animations in `seasons/` directories
- Check framebuffer functions in `display/framebuffer.h`
