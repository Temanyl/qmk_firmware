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

#include "bunny.h"
#include "../../display/framebuffer.h"
#include "timer.h"
#include <math.h>
#include <stdlib.h>

// Animation parameters
#define HOP_DURATION 800         // ms for complete hop
#define HOP_HEIGHT 20.0f         // Maximum hop height
#define HOP_MIN_INTERVAL 2000    // Minimum time between hops
#define HOP_MAX_INTERVAL 5000    // Maximum time between hops
#define EAR_WIGGLE_INTERVAL 100  // ms between ear animation frames

// Bunny colors (HSV)
#define BUNNY_HUE 30             // Light brown/tan
#define BUNNY_SAT 80
#define BUNNY_VAL 200
#define BUNNY_PINK_HUE 240       // Pink for nose/ears

// Initialize a single bunny instance
void bunny_init(bunny_t* bunny, float x, float base_y, float velocity_x, uint32_t last_hop_offset) {
    uint32_t now = timer_read32();

    bunny->x = x;
    bunny->base_y = base_y;
    bunny->y = base_y;
    bunny->velocity_x = velocity_x;
    bunny->velocity_y = 0.0f;
    bunny->hop_phase = 0.0f;
    bunny->is_hopping = false;
    bunny->animation_frame = 0;
    bunny->last_hop = now - last_hop_offset;
    bunny->last_update = now;
}

// Update a single bunny's animation state
void bunny_update(bunny_t* bunny) {
    uint32_t now = timer_read32();
    uint32_t elapsed = now - bunny->last_update;

    // Update horizontal position (continuous slow movement)
    bunny->x += bunny->velocity_x;

    // Wrap around screen
    if (bunny->x > FB_WIDTH + BUNNY_WIDTH) {
        bunny->x = -BUNNY_WIDTH;
    }

    // Update ear wiggle animation
    if (elapsed >= EAR_WIGGLE_INTERVAL) {
        bunny->animation_frame = (bunny->animation_frame + 1) % 4;
    }

    // Check if bunny should start hopping
    uint32_t time_since_hop = now - bunny->last_hop;
    if (!bunny->is_hopping && time_since_hop > HOP_MIN_INTERVAL) {
        // Random chance to hop (or forced after max interval)
        if (time_since_hop > HOP_MAX_INTERVAL || (rand() % 100) < 5) {
            bunny->is_hopping = true;
            bunny->hop_phase = 0.0f;
            bunny->last_hop = now;
        }
    }

    // Update hop animation
    if (bunny->is_hopping) {
        bunny->hop_phase += (float)elapsed / HOP_DURATION;

        if (bunny->hop_phase >= 1.0f) {
            // Hop complete
            bunny->is_hopping = false;
            bunny->hop_phase = 0.0f;
            bunny->y = bunny->base_y;
        } else {
            // Parabolic hop motion: y = 4h * phase * (1 - phase)
            float normalized_phase = bunny->hop_phase;
            bunny->y = bunny->base_y - (4.0f * HOP_HEIGHT * normalized_phase * (1.0f - normalized_phase));
        }
    }

    bunny->last_update = now;
}

// Draw a single bunny
void bunny_draw(const bunny_t* bunny) {
    int16_t x = (int16_t)bunny->x;
    int16_t y = (int16_t)bunny->y;
    uint8_t animation_frame = bunny->animation_frame;
    bool is_hopping = bunny->is_hopping;

    // Calculate ear wiggle offset
    int8_t left_ear_offset = (animation_frame < 2) ? 0 : -1;
    int8_t right_ear_offset = (animation_frame % 2 == 0) ? 0 : -1;

    // Ears (long ears characteristic of bunny)
    // Left ear
    fb_rect_hsv(x + 1, y + left_ear_offset, x + 2, y + 4 + left_ear_offset,
                BUNNY_HUE, BUNNY_SAT, BUNNY_VAL, true);
    fb_set_pixel_hsv(x + 2, y + 1 + left_ear_offset, BUNNY_PINK_HUE, 180, 150);

    // Right ear
    fb_rect_hsv(x + 6, y + right_ear_offset, x + 7, y + 4 + right_ear_offset,
                BUNNY_HUE, BUNNY_SAT, BUNNY_VAL, true);
    fb_set_pixel_hsv(x + 6, y + 1 + right_ear_offset, BUNNY_PINK_HUE, 180, 150);

    // Head (round)
    fb_circle_hsv(x + 4, y + 6, 3, BUNNY_HUE, BUNNY_SAT, BUNNY_VAL, true);

    // Eyes (small black dots)
    fb_set_pixel_hsv(x + 3, y + 5, 0, 0, 0);  // Left eye
    fb_set_pixel_hsv(x + 5, y + 5, 0, 0, 0);  // Right eye

    // Nose (pink)
    fb_set_pixel_hsv(x + 4, y + 7, BUNNY_PINK_HUE, 200, 180);

    // Body (oval)
    fb_ellipse_hsv(x + 4, y + 10, 3, 2, BUNNY_HUE, BUNNY_SAT, BUNNY_VAL, true);

    // Feet (only visible when not hopping high)
    if (is_hopping) {
        // Tucked feet during hop (smaller)
        fb_rect_hsv(x + 2, y + 11, x + 3, y + 11, BUNNY_HUE, BUNNY_SAT, BUNNY_VAL - 20, true);
        fb_rect_hsv(x + 5, y + 11, x + 6, y + 11, BUNNY_HUE, BUNNY_SAT, BUNNY_VAL - 20, true);
    } else {
        // Extended feet on ground
        fb_rect_hsv(x + 2, y + 11, x + 3, y + 12, BUNNY_HUE, BUNNY_SAT, BUNNY_VAL - 20, true);
        fb_rect_hsv(x + 5, y + 11, x + 6, y + 12, BUNNY_HUE, BUNNY_SAT, BUNNY_VAL - 20, true);
    }

    // Fluffy tail (white puff)
    fb_circle_hsv(x + 8, y + 10, 2, 0, 0, 255, true);
}

// Get bunny's bounding box
void bunny_get_bounds(const bunny_t* bunny, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    int16_t x = (int16_t)bunny->x;
    int16_t y = (int16_t)bunny->y;

    *x1 = x;
    *y1 = y;
    *x2 = x + BUNNY_WIDTH;
    *y2 = y + BUNNY_HEIGHT;
}

// Check if a point is inside the bunny's bounds
bool bunny_contains_point(const bunny_t* bunny, int16_t px, int16_t py) {
    int16_t x1, y1, x2, y2;
    bunny_get_bounds(bunny, &x1, &y1, &x2, &y2);
    return (px >= x1 && px <= x2 && py >= y1 && py <= y2);
}
