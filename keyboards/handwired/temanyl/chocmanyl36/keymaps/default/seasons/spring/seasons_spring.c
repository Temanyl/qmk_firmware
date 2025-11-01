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
#include "seasons_spring.h"
#include "../../display/framebuffer.h"
#include "../../objects/fauna/bird.h"
#include "../../objects/fauna/butterfly.h"
#include "../../objects/flora/flower.h"
#include "../../scenes/scenes.h"

// Spring animation state
bool spring_initialized = false;
bool spring_background_saved = false;
uint32_t spring_animation_timer = 0;

// Forward declarations
extern painter_device_t display;
extern uint8_t current_month;
extern bool smoke_initialized;
extern bool smoke_background_saved;

// Initialize spring animations
void init_spring_animations(void) {
    if (spring_initialized) return;

    // Initialize animated creatures
    birds_init();
    butterflies_init();
    spring_initialized = true;
}

// Animate spring creatures (per-object region-based updates)
void animate_spring(void) {
    if (!spring_initialized || !spring_background_saved) {
        return;
    }

    // Get current season
    uint8_t season = (current_month == 12 || current_month <= 2) ? 0 :
                     (current_month >= 3 && current_month <= 5) ? 1 :
                     (current_month >= 6 && current_month <= 8) ? 2 : 3;

    // Only animate during spring (season 1)
    if (season != 1) {
        return;
    }

    // Store old positions before updating
    int16_t old_bird_x[NUM_SPRING_BIRDS];
    int16_t old_bird_y[NUM_SPRING_BIRDS];
    int16_t old_butterfly_x[NUM_SPRING_BUTTERFLIES];
    int16_t old_butterfly_y[NUM_SPRING_BUTTERFLIES];

    for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
        old_bird_x[i] = (int16_t)birds[i].x;
        old_bird_y[i] = (int16_t)birds[i].y;
    }

    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        old_butterfly_x[i] = (int16_t)butterflies[i].x;
        old_butterfly_y[i] = (int16_t)butterflies[i].y;
    }

    // Update all positions
    birds_update();
    butterflies_update();

    // Animate each bird individually (like raindrops)
    for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
        // Calculate old region bounds
        int16_t old_x1 = old_bird_x[i] - (BIRD_WIDTH / 2);
        int16_t old_y1 = old_bird_y[i] - 4;  // Above center for wing spread
        int16_t old_x2 = old_bird_x[i] + (BIRD_WIDTH / 2);
        int16_t old_y2 = old_bird_y[i] + 3;  // Below center for wing spread

        // Restore old bird position from background
        fb_restore_from_background(old_x1, old_y1, old_x2, old_y2);

        // If smoke particles overlap old position, redraw them
        if (smoke_initialized && smoke_background_saved) {
            redraw_smoke_in_region(old_x1, old_y1, old_x2, old_y2);
        }

        // Flush old region
        fb_flush_region(display, old_x1, old_y1, old_x2, old_y2);

        // Draw bird at new position
        int16_t new_x = (int16_t)birds[i].x;
        int16_t new_y = (int16_t)birds[i].y;
        if (new_x >= 0 && new_x < FB_WIDTH && new_y >= 0 && new_y < 150) {
            bird_draw_single(i);

            // Redraw smoke if it overlaps new position
            if (smoke_initialized && smoke_background_saved) {
                int16_t new_x1 = new_x - (BIRD_WIDTH / 2);
                int16_t new_y1 = new_y - 4;
                int16_t new_x2 = new_x + (BIRD_WIDTH / 2);
                int16_t new_y2 = new_y + 3;
                redraw_smoke_in_region(new_x1, new_y1, new_x2, new_y2);
            }

            // Flush new region
            int16_t new_x1 = new_x - (BIRD_WIDTH / 2);
            int16_t new_y1 = new_y - 4;
            int16_t new_x2 = new_x + (BIRD_WIDTH / 2);
            int16_t new_y2 = new_y + 3;
            fb_flush_region(display, new_x1, new_y1, new_x2, new_y2);
        }
    }

    // Animate each butterfly individually
    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        // Calculate old region bounds
        int16_t old_x1 = old_butterfly_x[i] - (BUTTERFLY_WIDTH / 2);
        int16_t old_y1 = old_butterfly_y[i] - (BUTTERFLY_HEIGHT / 2);
        int16_t old_x2 = old_butterfly_x[i] + (BUTTERFLY_WIDTH / 2);
        int16_t old_y2 = old_butterfly_y[i] + (BUTTERFLY_HEIGHT / 2);

        // Restore old butterfly position from background
        fb_restore_from_background(old_x1, old_y1, old_x2, old_y2);

        // If smoke particles overlap old position, redraw them
        if (smoke_initialized && smoke_background_saved) {
            redraw_smoke_in_region(old_x1, old_y1, old_x2, old_y2);
        }

        // Flush old region
        fb_flush_region(display, old_x1, old_y1, old_x2, old_y2);

        // Draw butterfly at new position
        int16_t new_x = (int16_t)butterflies[i].x;
        int16_t new_y = (int16_t)butterflies[i].y;
        if (new_x >= 0 && new_x < FB_WIDTH && new_y >= 0 && new_y < 150) {
            butterfly_draw_single(i);

            // Redraw smoke if it overlaps new position
            if (smoke_initialized && smoke_background_saved) {
                int16_t new_x1 = new_x - (BUTTERFLY_WIDTH / 2);
                int16_t new_y1 = new_y - (BUTTERFLY_HEIGHT / 2);
                int16_t new_x2 = new_x + (BUTTERFLY_WIDTH / 2);
                int16_t new_y2 = new_y + (BUTTERFLY_HEIGHT / 2);
                redraw_smoke_in_region(new_x1, new_y1, new_x2, new_y2);
            }

            // Flush new region
            int16_t new_x1 = new_x - (BUTTERFLY_WIDTH / 2);
            int16_t new_y1 = new_y - (BUTTERFLY_HEIGHT / 2);
            int16_t new_x2 = new_x + (BUTTERFLY_WIDTH / 2);
            int16_t new_y2 = new_y + (BUTTERFLY_HEIGHT / 2);
            fb_flush_region(display, new_x1, new_y1, new_x2, new_y2);
        }
    }
}

// Reset spring animations
void reset_spring_animations(void) {
    // Reset all animated elements to initial state
    spring_initialized = false;
    spring_background_saved = false;
    birds_reset();
    butterflies_reset();
}

// Draw spring-specific scene elements
void draw_spring_scene_elements(void) {
    uint16_t ground_y = 150;

    // Initialize animations if not done
    if (!spring_initialized) {
        init_spring_animations();
    }

    // NOTE: Birds and butterflies are NOT drawn here to avoid baking them into
    // the background. They are drawn only by the region-based animation system
    // in animate_spring() after the background is saved.

    // Draw flowers on the ground (static)
    flowers_draw_all(ground_y);
}
