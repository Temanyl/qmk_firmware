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

#include QMK_KEYBOARD_H
#include "seasons_easter.h"
#include "../../objects/seasonal/easter_egg.h"
#include "../../objects/fauna/bunny.h"
#include "../../display/framebuffer.h"

// Easter animation state
bool easter_initialized = false;
bool easter_background_saved = false;
uint32_t easter_animation_timer = 0;

// Easter bunny configuration
#define NUM_EASTER_BUNNIES 1
#define BUNNY_GROUND_Y 138  // Ground level (150 - bunny_height)
bunny_t bunnies[NUM_EASTER_BUNNIES];

// Forward declarations
extern painter_device_t display;
extern uint8_t current_month;
extern uint8_t current_day;

// Easter event check (March 15 - April 30)
bool is_easter_event(void) {
    return (current_month == 3 && current_day >= 15) ||
           (current_month == 4);
}

// Draw Easter elements
void draw_easter_elements(void) {
    // Draw Easter eggs scattered on the ground
    easter_eggs_draw_all();

    // Note: Spring flowers are already drawn by the spring scene rendering
    // Easter eggs are the main decorative overlay for the Easter event
}

// Initialize Easter animations
void init_easter_animations(void) {
    if (easter_initialized) return;

    // Initialize Easter eggs (static)
    easter_eggs_init();

    // Initialize hopping bunny
    for (uint8_t i = 0; i < NUM_EASTER_BUNNIES; i++) {
        bunny_init(&bunnies[i],
                   20.0f + (i * 40.0f),  // Starting x position
                   BUNNY_GROUND_Y,        // Base y (ground level)
                   0.6f,                  // Horizontal velocity
                   0);                    // Last hop offset
    }

    easter_initialized = true;
}

// Animate Easter bunny (region-based updates)
void animate_easter(void) {
    if (!easter_initialized || !easter_background_saved) {
        return;
    }

    if (!is_easter_event()) {
        return;
    }

    // Store old bunny positions before updating
    int16_t old_bunny_x[NUM_EASTER_BUNNIES];
    int16_t old_bunny_y[NUM_EASTER_BUNNIES];

    for (uint8_t i = 0; i < NUM_EASTER_BUNNIES; i++) {
        old_bunny_x[i] = (int16_t)bunnies[i].x;
        old_bunny_y[i] = (int16_t)bunnies[i].y;
    }

    // Update bunny positions
    for (uint8_t i = 0; i < NUM_EASTER_BUNNIES; i++) {
        bunny_update(&bunnies[i]);
    }

    // Restore background and redraw each bunny
    for (uint8_t i = 0; i < NUM_EASTER_BUNNIES; i++) {
        // Calculate bounding box for old position
        int16_t old_x1 = old_bunny_x[i] - 2;
        int16_t old_y1 = old_bunny_y[i] - 2;
        int16_t old_x2 = old_bunny_x[i] + BUNNY_WIDTH + 2;
        int16_t old_y2 = old_bunny_y[i] + BUNNY_HEIGHT + 2;

        // Restore background at old position
        fb_restore_from_background(old_x1, old_y1, old_x2, old_y2);

        // Draw bunny at new position
        bunny_draw(&bunnies[i]);

        // Calculate region to flush (union of old and new positions)
        int16_t new_x1 = (int16_t)bunnies[i].x - 2;
        int16_t new_y1 = (int16_t)bunnies[i].y - 2;
        int16_t new_x2 = (int16_t)bunnies[i].x + BUNNY_WIDTH + 2;
        int16_t new_y2 = (int16_t)bunnies[i].y + BUNNY_HEIGHT + 2;

        // Compute union of old and new regions
        int16_t flush_x1 = (old_x1 < new_x1) ? old_x1 : new_x1;
        int16_t flush_y1 = (old_y1 < new_y1) ? old_y1 : new_y1;
        int16_t flush_x2 = (old_x2 > new_x2) ? old_x2 : new_x2;
        int16_t flush_y2 = (old_y2 > new_y2) ? old_y2 : new_y2;

        // Clamp to display bounds
        if (flush_x1 < 0) flush_x1 = 0;
        if (flush_y1 < 0) flush_y1 = 0;
        if (flush_x2 >= FB_WIDTH) flush_x2 = FB_WIDTH - 1;
        if (flush_y2 >= FB_HEIGHT) flush_y2 = FB_HEIGHT - 1;

        // Flush the dirty region
        if (flush_x1 <= flush_x2 && flush_y1 <= flush_y2) {
            fb_flush_region(display, flush_x1, flush_y1, flush_x2, flush_y2);
        }
    }
}

// Reset Easter animations
void reset_easter_animations(void) {
    easter_initialized = false;
    easter_background_saved = false;
}
