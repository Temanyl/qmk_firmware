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

// Bunny animation states
bunny_state_t bunnies[NUM_EASTER_BUNNIES];

// Animation parameters
#define HOP_DURATION 800         // ms for complete hop
#define HOP_HEIGHT 20.0f         // Maximum hop height
#define HOP_MIN_INTERVAL 2000    // Minimum time between hops
#define HOP_MAX_INTERVAL 5000    // Maximum time between hops
#define BUNNY_GROUND_Y 138       // Ground level (150 - bunny_height)
#define EAR_WIGGLE_INTERVAL 100  // ms between ear animation frames

// Bunny colors (HSV)
#define BUNNY_HUE 30             // Light brown/tan
#define BUNNY_SAT 80
#define BUNNY_VAL 200
#define BUNNY_PINK_HUE 240       // Pink for nose/ears

// Initialize bunny animations
void bunnies_init(void) {
    uint32_t now = timer_read32();
    for (uint8_t i = 0; i < NUM_EASTER_BUNNIES; i++) {
        bunnies[i].x = 20.0f + (i * 40.0f);
        bunnies[i].base_y = BUNNY_GROUND_Y;
        bunnies[i].y = BUNNY_GROUND_Y;
        bunnies[i].velocity_x = 0.6f;
        bunnies[i].velocity_y = 0.0f;
        bunnies[i].hop_phase = 0.0f;
        bunnies[i].is_hopping = false;
        bunnies[i].animation_frame = 0;
        bunnies[i].last_hop = now;
        bunnies[i].last_update = now;
    }
}

// Reset bunny animations
void bunnies_reset(void) {
    bunnies_init();
}

// Start a hop for a bunny
static void start_hop(uint8_t index) {
    bunnies[index].is_hopping = true;
    bunnies[index].hop_phase = 0.0f;
    bunnies[index].last_hop = timer_read32();
}

// Update bunny animations
void bunnies_update(void) {
    uint32_t now = timer_read32();

    for (uint8_t i = 0; i < NUM_EASTER_BUNNIES; i++) {
        uint32_t elapsed = now - bunnies[i].last_update;

        // Update horizontal position (continuous slow movement)
        bunnies[i].x += bunnies[i].velocity_x;

        // Wrap around screen
        if (bunnies[i].x > FB_WIDTH + BUNNY_WIDTH) {
            bunnies[i].x = -BUNNY_WIDTH;
        }

        // Update ear wiggle animation
        if (elapsed >= EAR_WIGGLE_INTERVAL) {
            bunnies[i].animation_frame = (bunnies[i].animation_frame + 1) % 4;
        }

        // Check if bunny should start hopping
        uint32_t time_since_hop = now - bunnies[i].last_hop;
        if (!bunnies[i].is_hopping && time_since_hop > HOP_MIN_INTERVAL) {
            // Random chance to hop (or forced after max interval)
            if (time_since_hop > HOP_MAX_INTERVAL || (rand() % 100) < 5) {
                start_hop(i);
            }
        }

        // Update hop animation
        if (bunnies[i].is_hopping) {
            bunnies[i].hop_phase += (float)elapsed / HOP_DURATION;

            if (bunnies[i].hop_phase >= 1.0f) {
                // Hop complete
                bunnies[i].is_hopping = false;
                bunnies[i].hop_phase = 0.0f;
                bunnies[i].y = bunnies[i].base_y;
            } else {
                // Parabolic hop motion: y = 4h * phase * (1 - phase)
                float normalized_phase = bunnies[i].hop_phase;
                bunnies[i].y = bunnies[i].base_y - (4.0f * HOP_HEIGHT * normalized_phase * (1.0f - normalized_phase));
            }
        }

        bunnies[i].last_update = now;
    }
}

// Draw a bunny sprite
static void draw_bunny(int16_t x, int16_t y, uint8_t animation_frame, bool is_hopping) {
    // Bunny is drawn from top to bottom
    // Structure: ears, head, body, feet

    // Calculate ear wiggle offset
    int8_t left_ear_offset = (animation_frame < 2) ? 0 : -1;
    int8_t right_ear_offset = (animation_frame % 2 == 0) ? 0 : -1;

    // Ears (long ears characteristic of bunny)
    // Left ear
    fb_rect_hsv(x + 1, y + left_ear_offset, x + 2, y + 4 + left_ear_offset,
                BUNNY_HUE, BUNNY_SAT, BUNNY_VAL, true);
    fb_set_pixel_hsv(x + 2, y + 1 + left_ear_offset, BUNNY_PINK_HUE, 180, 150); // Pink inner ear

    // Right ear
    fb_rect_hsv(x + 6, y + right_ear_offset, x + 7, y + 4 + right_ear_offset,
                BUNNY_HUE, BUNNY_SAT, BUNNY_VAL, true);
    fb_set_pixel_hsv(x + 6, y + 1 + right_ear_offset, BUNNY_PINK_HUE, 180, 150); // Pink inner ear

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

// Draw a single bunny by index
void bunny_draw_single(uint8_t index) {
    if (index >= NUM_EASTER_BUNNIES) return;
    draw_bunny((int16_t)bunnies[index].x, (int16_t)bunnies[index].y,
               bunnies[index].animation_frame, bunnies[index].is_hopping);
}

// Draw all Easter bunnies
void bunnies_draw_all(void) {
    for (uint8_t i = 0; i < NUM_EASTER_BUNNIES; i++) {
        bunny_draw_single(i);
    }
}
