/*
Copyright 2022 Joe Scotto

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "firefly.h"
#include "../../display/framebuffer.h"
#include "timer.h"
#include <math.h>

// Animation parameters
#define FIREFLY_DRIFT_FREQ_X 0.0015f        // Horizontal drift speed (gentle, slow)
#define FIREFLY_DRIFT_FREQ_Y 0.002f         // Vertical drift speed (gentle, slow)
#define FIREFLY_DRIFT_AMPLITUDE_X 8.0f      // Horizontal drift range (pixels)
#define FIREFLY_DRIFT_AMPLITUDE_Y 6.0f      // Vertical drift range (pixels)
#define FIREFLY_BLINK_ON_MIN 1200           // Min time lit (ms)
#define FIREFLY_BLINK_ON_MAX 2500           // Max time lit (ms)
#define FIREFLY_BLINK_OFF_MIN 800           // Min time dark (ms)
#define FIREFLY_BLINK_OFF_MAX 3000          // Max time dark (ms)
#define FIREFLY_BRIGHTNESS_MAX 255          // Maximum brightness
#define FIREFLY_FADE_SPEED 8                // Brightness change per update

// Pseudo-random number generator using timer and seed
static uint32_t pseudo_random(uint32_t seed, uint8_t index) {
    uint32_t hash = seed * 1103515245 + index * 12345;
    return hash;
}

// Initialize a single firefly instance
void firefly_init(firefly_t* firefly, float base_x, float base_y, float drift_phase_x, float drift_phase_y, uint32_t blink_offset) {
    uint32_t now = timer_read32();

    firefly->base_x = base_x;
    firefly->base_y = base_y;
    firefly->x = base_x;
    firefly->y = base_y;
    firefly->drift_phase_x = drift_phase_x;
    firefly->drift_phase_y = drift_phase_y;

    // Initialize with random on/off state based on blink_offset
    uint32_t rand_val = pseudo_random(now + blink_offset, (uint8_t)base_x);
    firefly->is_lit = (rand_val % 2) == 0;

    // Set initial blink duration
    if (firefly->is_lit) {
        firefly->next_blink_duration = FIREFLY_BLINK_ON_MIN +
            (pseudo_random(now + blink_offset, (uint8_t)base_y) % (FIREFLY_BLINK_ON_MAX - FIREFLY_BLINK_ON_MIN));
    } else {
        firefly->next_blink_duration = FIREFLY_BLINK_OFF_MIN +
            (pseudo_random(now + blink_offset, (uint8_t)base_y) % (FIREFLY_BLINK_OFF_MAX - FIREFLY_BLINK_OFF_MIN));
    }

    firefly->blink_timer = now;
    firefly->brightness = firefly->is_lit ? FIREFLY_BRIGHTNESS_MAX : 0;
    firefly->last_update = now;
}

// Update a single firefly's animation state
void firefly_update(firefly_t* firefly) {
    uint32_t now = timer_read32();
    uint32_t elapsed = now - firefly->last_update;

    // Update drift phases (slow floating movement)
    firefly->drift_phase_x += FIREFLY_DRIFT_FREQ_X * elapsed;
    firefly->drift_phase_y += FIREFLY_DRIFT_FREQ_Y * elapsed;

    // Calculate position: base + drift
    float drift_x = FIREFLY_DRIFT_AMPLITUDE_X * sinf(firefly->drift_phase_x);
    float drift_y = FIREFLY_DRIFT_AMPLITUDE_Y * sinf(firefly->drift_phase_y);

    firefly->x = firefly->base_x + drift_x;
    firefly->y = firefly->base_y + drift_y;

    // Check if it's time to change blink state
    if (now - firefly->blink_timer >= firefly->next_blink_duration) {
        // Toggle lit state
        firefly->is_lit = !firefly->is_lit;
        firefly->blink_timer = now;

        // Set next duration
        uint8_t seed = (uint8_t)(firefly->base_x + firefly->base_y);
        if (firefly->is_lit) {
            firefly->next_blink_duration = FIREFLY_BLINK_ON_MIN +
                (pseudo_random(now, seed * 2) % (FIREFLY_BLINK_ON_MAX - FIREFLY_BLINK_ON_MIN));
        } else {
            firefly->next_blink_duration = FIREFLY_BLINK_OFF_MIN +
                (pseudo_random(now, seed * 3) % (FIREFLY_BLINK_OFF_MAX - FIREFLY_BLINK_OFF_MIN));
        }
    }

    // Update brightness with smooth fade in/out
    uint8_t target_brightness = firefly->is_lit ? FIREFLY_BRIGHTNESS_MAX : 0;

    if (firefly->brightness < target_brightness) {
        // Fade in
        int16_t new_brightness = (int16_t)firefly->brightness + FIREFLY_FADE_SPEED;
        if (new_brightness > target_brightness) {
            firefly->brightness = target_brightness;
        } else {
            firefly->brightness = (uint8_t)new_brightness;
        }
    } else if (firefly->brightness > target_brightness) {
        // Fade out
        int16_t new_brightness = (int16_t)firefly->brightness - FIREFLY_FADE_SPEED;
        if (new_brightness < target_brightness) {
            firefly->brightness = target_brightness;
        } else {
            firefly->brightness = (uint8_t)new_brightness;
        }
    }

    firefly->last_update = now;
}

// Draw a single firefly (only if lit)
void firefly_draw(const firefly_t* firefly) {
    if (firefly->brightness == 0) return;  // Don't draw if not lit

    int16_t x = (int16_t)firefly->x;
    int16_t y = (int16_t)firefly->y;
    uint8_t brightness = firefly->brightness;

    // Firefly glow (greenish-yellow)
    uint8_t hue = 60;  // Greenish-yellow

    // Center (brightest)
    fb_set_pixel_hsv(x, y, hue, 200, brightness);

    // Glow around center (dimmer)
    uint8_t glow_brightness = brightness * 3 / 4;
    fb_set_pixel_hsv(x - 1, y, hue, 200, glow_brightness);
    fb_set_pixel_hsv(x + 1, y, hue, 200, glow_brightness);
    fb_set_pixel_hsv(x, y - 1, hue, 200, glow_brightness);
    fb_set_pixel_hsv(x, y + 1, hue, 200, glow_brightness);

    // Outer glow (even dimmer)
    uint8_t outer_glow = brightness / 2;
    fb_set_pixel_hsv(x - 1, y - 1, hue, 200, outer_glow);
    fb_set_pixel_hsv(x + 1, y - 1, hue, 200, outer_glow);
    fb_set_pixel_hsv(x - 1, y + 1, hue, 200, outer_glow);
    fb_set_pixel_hsv(x + 1, y + 1, hue, 200, outer_glow);
}

// Get firefly's bounding box
void firefly_get_bounds(const firefly_t* firefly, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    int16_t x = (int16_t)firefly->x;
    int16_t y = (int16_t)firefly->y;

    *x1 = x - (FIREFLY_WIDTH / 2);
    *y1 = y - (FIREFLY_HEIGHT / 2);
    *x2 = x + (FIREFLY_WIDTH / 2);
    *y2 = y + (FIREFLY_HEIGHT / 2);
}

// Check if a point is inside the firefly's bounds
bool firefly_contains_point(const firefly_t* firefly, int16_t px, int16_t py) {
    int16_t x1, y1, x2, y2;
    firefly_get_bounds(firefly, &x1, &y1, &x2, &y2);
    return (px >= x1 && px <= x2 && py >= y1 && py <= y2);
}
