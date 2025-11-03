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

// Spring bird configuration
#define NUM_SPRING_BIRDS 6
bird_t birds[NUM_SPRING_BIRDS];

static const struct {
    uint16_t base_y;
    float velocity_x;
    float bob_phase;
} bird_config[NUM_SPRING_BIRDS] = {
    {50, 0.25f, 0.0f},      // Bird 0: slow, starts at phase 0
    {40, 0.35f, 1.0f},      // Bird 1: medium-fast, offset phase
    {70, 0.20f, 2.5f},      // Bird 2: slow, different phase
    {45, 0.30f, 0.8f},      // Bird 3: medium, offset phase
    {75, 0.28f, 1.7f},      // Bird 4: medium, different phase
    {65, 0.22f, 3.2f}       // Bird 5: slow-medium, offset phase
};

// Spring butterfly configuration
#define NUM_SPRING_BUTTERFLIES 8
butterfly_t butterflies[NUM_SPRING_BUTTERFLIES];

static const struct {
    uint16_t base_x;
    uint16_t base_y;
    uint8_t hue;
    float flutter_phase_x;
    float flutter_phase_y;
    float amplitude_x;
    float amplitude_y;
} butterfly_config[NUM_SPRING_BUTTERFLIES] = {
    {20,  115, 234, 0.0f,  0.0f,  3.0f, 2.5f},   // Purple: small flutter
    {45,  125, 170, 1.2f,  0.5f,  5.0f, 4.0f},   // Blue: larger movements
    {65,  120, 42,  2.4f,  1.0f,  4.0f, 3.0f},   // Yellow: medium flutter
    {85,  130, 200, 3.6f,  1.5f,  6.0f, 3.5f},   // Light blue: wide horizontal
    {125, 135, 234, 2.0f,  2.5f,  4.5f, 2.0f},   // Purple: wide shallow
    {35,  128, 85,  1.5f,  0.8f,  5.5f, 3.5f},   // Green: large movements
    {75,  122, 42,  3.0f,  1.8f,  2.5f, 3.0f},   // Yellow: tight pattern
    {95,  133, 170, 0.5f,  2.2f,  4.0f, 5.0f}    // Blue: tall flutter
};

// Forward declarations
extern painter_device_t display;
extern uint8_t current_month;
extern bool smoke_initialized;
extern bool smoke_background_saved;

// Initialize spring animations
void init_spring_animations(void) {
    if (spring_initialized) return;

    // Initialize birds
    for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
        bird_init(&birds[i],
                  (i * 25.0f) + 15.0f,           // Spread birds across sky
                  bird_config[i].base_y,
                  bird_config[i].velocity_x,
                  bird_config[i].bob_phase);
    }

    // Initialize butterflies
    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        butterfly_init(&butterflies[i],
                       butterfly_config[i].base_x,
                       butterfly_config[i].base_y,
                       butterfly_config[i].hue,
                       butterfly_config[i].flutter_phase_x,
                       butterfly_config[i].flutter_phase_y,
                       butterfly_config[i].amplitude_x,
                       butterfly_config[i].amplitude_y,
                       i * 1000);  // Stagger initial wander timers
    }

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
    for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
        bird_update(&birds[i]);
    }

    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        butterfly_update(&butterflies[i]);
    }

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
            bird_draw(&birds[i]);

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
            butterfly_draw(&butterflies[i]);

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
