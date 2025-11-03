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

// Summer bee configuration
#define NUM_SUMMER_BEES 5
bee_t bees[NUM_SUMMER_BEES];

// Sunflower positions (matching sunflower.c configuration)
// Sunflower data: {x, stem_height}, flower head is at (x+1, ground_y - stem_height - 3)
// With ground_y = 150
static const struct {
    uint16_t center_x;
    uint16_t center_y;
    float orbit_radius;
    float orbit_phase;
} bee_config[NUM_SUMMER_BEES] = {
    {23, 134, 8.0f, 0.0f},      // Bee 0: orbits sunflower 0
    {53, 132, 9.0f, 1.3f},      // Bee 1: orbits sunflower 1
    {79, 133, 8.5f, 2.6f},      // Bee 2: orbits sunflower 2
    {103, 135, 7.5f, 3.9f},     // Bee 3: orbits sunflower 3
    {123, 133, 8.0f, 5.2f}      // Bee 4: orbits sunflower 4
};

// Summer firefly configuration
#define NUM_SUMMER_FIREFLIES 12
firefly_t fireflies[NUM_SUMMER_FIREFLIES];

static const struct {
    uint16_t base_x;
    uint16_t base_y;
    float drift_phase_x;
    float drift_phase_y;
} firefly_config[NUM_SUMMER_FIREFLIES] = {
    {30,  120, 0.0f, 0.0f},
    {60,  110, 1.5f, 0.8f},
    {90,  125, 3.0f, 1.6f},
    {120, 115, 4.5f, 2.4f},
    {40,  105, 2.0f, 3.2f},
    {70,  130, 3.5f, 4.0f},
    {100, 108, 5.0f, 4.8f},
    {130, 122, 0.7f, 5.6f},
    {50,  118, 1.0f, 2.5f},
    {80,  135, 2.5f, 1.2f},
    {110, 112, 4.0f, 3.8f},
    {140, 128, 0.3f, 4.5f}
};

// Forward declarations
extern painter_device_t display;
extern uint8_t current_month;
extern uint8_t current_hour;
extern bool smoke_initialized;
extern bool smoke_background_saved;

// Initialize summer animations
void init_summer_animations(void) {
    if (summer_initialized) return;

    // Initialize bees
    for (uint8_t i = 0; i < NUM_SUMMER_BEES; i++) {
        bee_init(&bees[i],
                 bee_config[i].center_x,
                 bee_config[i].center_y,
                 bee_config[i].orbit_radius,
                 bee_config[i].orbit_phase);
    }

    // Initialize fireflies
    for (uint8_t i = 0; i < NUM_SUMMER_FIREFLIES; i++) {
        firefly_init(&fireflies[i],
                     firefly_config[i].base_x,
                     firefly_config[i].base_y,
                     firefly_config[i].drift_phase_x,
                     firefly_config[i].drift_phase_y,
                     i * 200);  // Blink offset
    }

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
    for (uint8_t i = 0; i < NUM_SUMMER_BEES; i++) {
        bee_update(&bees[i]);
    }

    if (is_evening) {
        for (uint8_t i = 0; i < NUM_SUMMER_FIREFLIES; i++) {
            firefly_update(&fireflies[i]);
        }
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
            bee_draw(&bees[i]);

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
                firefly_draw(&fireflies[i]);  // Only draws if lit

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
