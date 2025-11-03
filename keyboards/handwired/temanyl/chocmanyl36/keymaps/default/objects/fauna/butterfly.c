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

#include "butterfly.h"
#include "../../display/framebuffer.h"
#include "timer.h"
#include <math.h>

// Animation parameters
#define BUTTERFLY_FLUTTER_FREQ_X 0.004f          // Horizontal flutter frequency
#define BUTTERFLY_FLUTTER_FREQ_Y 0.006f          // Vertical flutter frequency (faster for figure-8)
#define WING_FLAP_INTERVAL_BUTTERFLY 120         // ms between wing animation frames
#define BUTTERFLY_WANDER_MIN_INTERVAL 4000       // Min time between wanders (4 sec)
#define BUTTERFLY_WANDER_MAX_INTERVAL 10000      // Max time between wanders (10 sec)
#define BUTTERFLY_WANDER_DURATION 3000           // How long a wander lasts (3 sec)
#define BUTTERFLY_WANDER_DISTANCE 15.0f          // Max distance to wander from base

// Pseudo-random number generator using timer and seed
static float pseudo_random(uint32_t seed, uint8_t index) {
    uint32_t hash = seed * 1103515245 + index * 12345;
    return (float)(hash % 1000) / 1000.0f;
}

// Initialize a single butterfly instance
void butterfly_init(butterfly_t* butterfly, float base_x, float base_y, uint8_t hue,
                    float flutter_phase_x, float flutter_phase_y, float amplitude_x, float amplitude_y,
                    uint32_t wander_timer_offset) {
    uint32_t now = timer_read32();

    butterfly->base_x = base_x;
    butterfly->base_y = base_y;
    butterfly->x = base_x;
    butterfly->y = base_y;
    butterfly->hue = hue;
    butterfly->flutter_phase_x = flutter_phase_x;
    butterfly->flutter_phase_y = flutter_phase_y;
    butterfly->amplitude_x = amplitude_x;
    butterfly->amplitude_y = amplitude_y;
    butterfly->wander_offset_x = 0.0f;
    butterfly->wander_offset_y = 0.0f;
    butterfly->is_wandering = false;
    butterfly->wander_timer = now + wander_timer_offset;
    butterfly->wing_frame = 0;
    butterfly->last_update = now;
}

// Update a single butterfly's animation state
void butterfly_update(butterfly_t* butterfly) {
    uint32_t now = timer_read32();
    uint32_t elapsed = now - butterfly->last_update;

    // Check wander state transitions
    if (!butterfly->is_wandering) {
        // Not wandering - check if it's time to start a new wander
        if (now - butterfly->wander_timer >= BUTTERFLY_WANDER_MIN_INTERVAL) {
            // Use pseudo-random to decide if we should wander now
            float rand_val = pseudo_random(now, (uint8_t)(butterfly->base_x + butterfly->base_y));
            uint32_t wander_interval = BUTTERFLY_WANDER_MIN_INTERVAL +
                (uint32_t)(rand_val * (BUTTERFLY_WANDER_MAX_INTERVAL - BUTTERFLY_WANDER_MIN_INTERVAL));

            if (now - butterfly->wander_timer >= wander_interval) {
                // Start wandering!
                butterfly->is_wandering = true;
                butterfly->wander_timer = now;

                // Pick a random wander target (relative to base position)
                float rand_x = pseudo_random(now + (uint32_t)butterfly->base_x, (uint8_t)(butterfly->base_x * 2));
                float rand_y = pseudo_random(now + (uint32_t)butterfly->base_y * 2, (uint8_t)(butterfly->base_y * 3));

                // Convert to -1.0 to 1.0 range, then scale by wander distance
                butterfly->wander_offset_x = (rand_x * 2.0f - 1.0f) * BUTTERFLY_WANDER_DISTANCE;
                butterfly->wander_offset_y = (rand_y * 2.0f - 1.0f) * BUTTERFLY_WANDER_DISTANCE;
            }
        }
    } else {
        // Currently wandering - check if wander duration is over
        if (now - butterfly->wander_timer >= BUTTERFLY_WANDER_DURATION) {
            butterfly->is_wandering = false;
            butterfly->wander_timer = now;
            // Gradually return to base (offsets will decay below)
        }
    }

    // Gradually apply or remove wander offset
    if (!butterfly->is_wandering) {
        // Not wandering - gradually return to base position
        if (fabsf(butterfly->wander_offset_x) > 0.1f || fabsf(butterfly->wander_offset_y) > 0.1f) {
            butterfly->wander_offset_x *= 0.95f;  // Decay by 5% each frame
            butterfly->wander_offset_y *= 0.95f;
        } else {
            butterfly->wander_offset_x = 0.0f;
            butterfly->wander_offset_y = 0.0f;
        }
    }

    // Update flutter phases
    butterfly->flutter_phase_x += BUTTERFLY_FLUTTER_FREQ_X * elapsed;
    butterfly->flutter_phase_y += BUTTERFLY_FLUTTER_FREQ_Y * elapsed;

    // Calculate position: base + wander_offset + flutter pattern
    float flutter_x = butterfly->amplitude_x * sinf(butterfly->flutter_phase_x);
    float flutter_y = butterfly->amplitude_y * sinf(butterfly->flutter_phase_y);

    butterfly->x = butterfly->base_x + butterfly->wander_offset_x + flutter_x;
    butterfly->y = butterfly->base_y + butterfly->wander_offset_y + flutter_y;

    // Update wing animation frame
    if (elapsed >= WING_FLAP_INTERVAL_BUTTERFLY) {
        butterfly->wing_frame = (butterfly->wing_frame + 1) % 4;  // 4-frame animation
    }

    // Always update timer
    butterfly->last_update = now;
}

// Draw a single butterfly
void butterfly_draw(const butterfly_t* butterfly) {
    int16_t x = (int16_t)butterfly->x;
    int16_t y = (int16_t)butterfly->y;
    uint8_t hue = butterfly->hue;
    uint8_t wing_frame = butterfly->wing_frame;

    // Wing positions vary based on animation frame
    // Frame 0: Wings fully open
    // Frame 1: Wings partially open
    // Frame 2: Wings partially closed
    // Frame 3: Wings closing (back to partially open)

    uint8_t wing_radius = 2;
    int8_t wing_offset_x = 0;
    int8_t wing_offset_y = 0;
    uint8_t wing_brightness = 200;

    switch (wing_frame) {
        case 0:  // Fully open
            wing_offset_x = 3;
            wing_offset_y = 0;
            wing_radius = 2;
            wing_brightness = 220;
            break;
        case 1:  // Partially open
            wing_offset_x = 2;
            wing_offset_y = 1;
            wing_radius = 2;
            wing_brightness = 200;
            break;
        case 2:  // Partially closed
            wing_offset_x = 2;
            wing_offset_y = 1;
            wing_radius = 1;
            wing_brightness = 180;
            break;
        case 3:  // Opening (same as partially open)
            wing_offset_x = 2;
            wing_offset_y = 1;
            wing_radius = 2;
            wing_brightness = 200;
            break;
    }

    // Draw butterfly body (thin vertical line)
    fb_set_pixel_hsv(x, y, hue, 180, 120);
    fb_set_pixel_hsv(x, y - 1, hue, 180, 100);
    fb_set_pixel_hsv(x, y + 1, hue, 180, 100);

    // Draw left wings (upper and lower)
    fb_circle_hsv(x - wing_offset_x, y - wing_offset_y, wing_radius, hue, 255, wing_brightness, true);
    if (wing_radius > 1) {
        fb_circle_hsv(x - wing_offset_x, y + wing_offset_y + 1, wing_radius - 1, hue, 255, wing_brightness - 30, true);
    }

    // Draw right wings (upper and lower)
    fb_circle_hsv(x + wing_offset_x, y - wing_offset_y, wing_radius, hue, 255, wing_brightness, true);
    if (wing_radius > 1) {
        fb_circle_hsv(x + wing_offset_x, y + wing_offset_y + 1, wing_radius - 1, hue, 255, wing_brightness - 30, true);
    }

    // Add wing details (small highlights)
    if (wing_frame == 0 || wing_frame == 1) {
        fb_set_pixel_hsv(x - wing_offset_x, y - wing_offset_y - 1, hue, 200, 255);
        fb_set_pixel_hsv(x + wing_offset_x, y - wing_offset_y - 1, hue, 200, 255);
    }
}

// Get butterfly's bounding box
void butterfly_get_bounds(const butterfly_t* butterfly, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    int16_t x = (int16_t)butterfly->x;
    int16_t y = (int16_t)butterfly->y;

    *x1 = x - (BUTTERFLY_WIDTH / 2);
    *y1 = y - (BUTTERFLY_HEIGHT / 2);
    *x2 = x + (BUTTERFLY_WIDTH / 2);
    *y2 = y + (BUTTERFLY_HEIGHT / 2);
}

// Check if a point is inside the butterfly's bounds
bool butterfly_contains_point(const butterfly_t* butterfly, int16_t px, int16_t py) {
    int16_t x1, y1, x2, y2;
    butterfly_get_bounds(butterfly, &x1, &y1, &x2, &y2);
    return (px >= x1 && px <= x2 && py >= y1 && py <= y2);
}
