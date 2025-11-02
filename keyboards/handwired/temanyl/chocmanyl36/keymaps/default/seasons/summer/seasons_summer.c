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
#include "seasons_summer.h"
#include "../../display/framebuffer.h"
#include "../../objects/effects/airplane.h"
#include "../../objects/flora/sunflower.h"
#include "../../objects/fauna/bee.h"
#include "../../objects/fauna/firefly.h"
#include "../../scenes/scenes.h"

// Summer animation state
bool summer_initialized = false;
bool summer_background_saved = false;
uint32_t summer_animation_timer = 0;

// Forward declarations
extern painter_device_t display;
extern uint8_t current_month;
extern uint8_t current_hour;
extern bool smoke_initialized;
extern bool smoke_background_saved;

// Initialize summer animations
void init_summer_animations(void) {
    if (summer_initialized) return;

    // Initialize animated creatures
    bees_init();
    fireflies_init();
    summer_initialized = true;
}

// Animate summer creatures (per-object region-based updates)
void animate_summer(void) {
    if (!summer_initialized || !summer_background_saved) {
        return;
    }

    // Get current season
    uint8_t season = (current_month == 12 || current_month <= 2) ? 0 :
                     (current_month >= 3 && current_month <= 5) ? 1 :
                     (current_month >= 6 && current_month <= 8) ? 2 : 3;

    // Only animate during summer (season 2)
    if (season != 2) {
        return;
    }

    // Check if it's evening/night for fireflies (18:00 - 6:00)
    bool is_evening = (current_hour >= 18 || current_hour < 6);

    // Store old positions before updating
    int16_t old_bee_x[NUM_SUMMER_BEES];
    int16_t old_bee_y[NUM_SUMMER_BEES];
    int16_t old_firefly_x[NUM_SUMMER_FIREFLIES];
    int16_t old_firefly_y[NUM_SUMMER_FIREFLIES];

    for (uint8_t i = 0; i < NUM_SUMMER_BEES; i++) {
        old_bee_x[i] = (int16_t)bees[i].x;
        old_bee_y[i] = (int16_t)bees[i].y;
    }

    for (uint8_t i = 0; i < NUM_SUMMER_FIREFLIES; i++) {
        old_firefly_x[i] = (int16_t)fireflies[i].x;
        old_firefly_y[i] = (int16_t)fireflies[i].y;
    }

    // Update all positions
    bees_update();
    if (is_evening) {
        fireflies_update();
    }

    // Animate each bee individually
    for (uint8_t i = 0; i < NUM_SUMMER_BEES; i++) {
        // Calculate old region bounds
        int16_t old_x1 = old_bee_x[i] - (BEE_WIDTH / 2);
        int16_t old_y1 = old_bee_y[i] - (BEE_HEIGHT / 2);
        int16_t old_x2 = old_bee_x[i] + (BEE_WIDTH / 2);
        int16_t old_y2 = old_bee_y[i] + (BEE_HEIGHT / 2);

        // Restore old bee position from background
        fb_restore_from_background(old_x1, old_y1, old_x2, old_y2);

        // If smoke particles overlap old position, redraw them
        if (smoke_initialized && smoke_background_saved) {
            redraw_smoke_in_region(old_x1, old_y1, old_x2, old_y2);
        }

        // Flush old region
        fb_flush_region(display, old_x1, old_y1, old_x2, old_y2);

        // Draw bee at new position
        int16_t new_x = (int16_t)bees[i].x;
        int16_t new_y = (int16_t)bees[i].y;
        if (new_x >= 0 && new_x < FB_WIDTH && new_y >= 0 && new_y < 150) {
            bee_draw_single(i);

            // Redraw smoke if it overlaps new position
            if (smoke_initialized && smoke_background_saved) {
                int16_t new_x1 = new_x - (BEE_WIDTH / 2);
                int16_t new_y1 = new_y - (BEE_HEIGHT / 2);
                int16_t new_x2 = new_x + (BEE_WIDTH / 2);
                int16_t new_y2 = new_y + (BEE_HEIGHT / 2);
                redraw_smoke_in_region(new_x1, new_y1, new_x2, new_y2);
            }

            // Flush new region
            int16_t new_x1 = new_x - (BEE_WIDTH / 2);
            int16_t new_y1 = new_y - (BEE_HEIGHT / 2);
            int16_t new_x2 = new_x + (BEE_WIDTH / 2);
            int16_t new_y2 = new_y + (BEE_HEIGHT / 2);
            fb_flush_region(display, new_x1, new_y1, new_x2, new_y2);
        }
    }

    // Animate fireflies (only in evening/night)
    if (is_evening) {
        for (uint8_t i = 0; i < NUM_SUMMER_FIREFLIES; i++) {
            // Calculate old region bounds
            int16_t old_x1 = old_firefly_x[i] - (FIREFLY_WIDTH / 2);
            int16_t old_y1 = old_firefly_y[i] - (FIREFLY_HEIGHT / 2);
            int16_t old_x2 = old_firefly_x[i] + (FIREFLY_WIDTH / 2);
            int16_t old_y2 = old_firefly_y[i] + (FIREFLY_HEIGHT / 2);

            // Always restore background (whether it was lit or not)
            fb_restore_from_background(old_x1, old_y1, old_x2, old_y2);

            // If smoke particles overlap old position, redraw them
            if (smoke_initialized && smoke_background_saved) {
                redraw_smoke_in_region(old_x1, old_y1, old_x2, old_y2);
            }

            // Flush old region
            fb_flush_region(display, old_x1, old_y1, old_x2, old_y2);

            // Draw firefly at new position (only if lit)
            int16_t new_x = (int16_t)fireflies[i].x;
            int16_t new_y = (int16_t)fireflies[i].y;
            if (new_x >= 0 && new_x < FB_WIDTH && new_y >= 0 && new_y < 150) {
                firefly_draw_single(i);  // Only draws if lit

                // Redraw smoke if it overlaps new position
                if (smoke_initialized && smoke_background_saved) {
                    int16_t new_x1 = new_x - (FIREFLY_WIDTH / 2);
                    int16_t new_y1 = new_y - (FIREFLY_HEIGHT / 2);
                    int16_t new_x2 = new_x + (FIREFLY_WIDTH / 2);
                    int16_t new_y2 = new_y + (FIREFLY_HEIGHT / 2);
                    redraw_smoke_in_region(new_x1, new_y1, new_x2, new_y2);
                }

                // Flush new region
                int16_t new_x1 = new_x - (FIREFLY_WIDTH / 2);
                int16_t new_y1 = new_y - (FIREFLY_HEIGHT / 2);
                int16_t new_x2 = new_x + (FIREFLY_WIDTH / 2);
                int16_t new_y2 = new_y + (FIREFLY_HEIGHT / 2);
                fb_flush_region(display, new_x1, new_y1, new_x2, new_y2);
            }
        }
    }
}

// Reset summer animations
void reset_summer_animations(void) {
    // Reset all animated elements to initial state
    summer_initialized = false;
    summer_background_saved = false;
    bees_reset();
    fireflies_reset();
}

// Draw summer-specific scene elements
void draw_summer_scene_elements(void) {
    uint16_t ground_y = 150;

    // Initialize animations if not done
    if (!summer_initialized) {
        init_summer_animations();
    }

    // Draw airplane in top left
    airplane_draw();

    // Draw sunflowers on the ground (static)
    sunflowers_draw_all(ground_y);

    // NOTE: Bees and fireflies are NOT drawn here to avoid baking them into
    // the background. They are drawn only by the region-based animation system
    // in animate_summer() after the background is saved.
}
