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
#include "seasons_newyear.h"
#include "../../display/framebuffer.h"

// New Year animation state
bool rockets_initialized = false;
bool newyear_scene_drawn = false;  // Track if static scene has been drawn once
uint32_t rocket_animation_timer = 0;
rocket_t rockets[NUM_ROCKETS];

// Forward declarations
extern uint8_t current_month;
extern uint8_t current_day;

// New Year's Eve check
bool is_new_years_eve(void) {
    return current_month == 12 && current_day == 31;
}

// Pre-calculated sine/cosine values for 6 directions (scaled by 16 for fixed-point)
// Angles: 0°, 60°, 120°, 180°, 240°, 300° (hexagonal pattern)
static const int16_t cos_table[6] = {16, 8, -8, -16, -8, 8};
static const int16_t sin_table[6] = {0, -14, -14, 0, 14, 14};

// Initialize rockets with staggered launch times
void init_rockets(void) {
    if (rockets_initialized) {
        return;
    }

    // Rocket colors (vibrant New Year colors)
    const uint8_t rocket_colors[NUM_ROCKETS] = {0, 42, 85, 170, 200};  // Red, Yellow, Green, Blue, Pink

    // Base rocket launch positions (spread across display)
    const int16_t base_launch_positions[NUM_ROCKETS] = {25, 45, 67, 90, 110};

    // Base rocket target heights (where they explode)
    const int16_t base_target_heights[NUM_ROCKETS] = {45, 55, 50, 60, 52};

    // Use timer as pseudo-random seed for variation
    uint32_t seed = timer_read32();

    for (uint8_t i = 0; i < NUM_ROCKETS; i++) {
        // Add randomization to launch position (±12 pixels from base)
        int16_t random_offset_x = ((seed * (i + 7) * 13) % 25) - 12;  // -12 to +12
        int16_t launch_x = base_launch_positions[i] + random_offset_x;

        // Clamp to valid range (5 to 130)
        if (launch_x < 5) launch_x = 5;
        if (launch_x > 130) launch_x = 130;

        // Add randomization to target height (±15 pixels from base)
        int16_t random_offset_y = ((seed * (i + 3) * 17) % 31) - 15;  // -15 to +15
        int16_t target_y = base_target_heights[i] + random_offset_y;

        // Clamp to valid range (30 to 80)
        if (target_y < 30) target_y = 30;
        if (target_y > 80) target_y = 80;

        rockets[i].x = launch_x;
        rockets[i].y = 148;  // Start just above ground (ground is at y=150)
        rockets[i].launch_x = launch_x;
        rockets[i].target_y = target_y;
        rockets[i].hue = rocket_colors[i];
        rockets[i].state = ROCKET_INACTIVE;
        rockets[i].state_timer = i * 400;  // Stagger launches (400ms apart for better coverage)

        // Initialize explosion particles
        for (uint8_t j = 0; j < NUM_EXPLOSION_PARTICLES; j++) {
            rockets[i].particles[j].brightness = 0;
        }
    }

    rockets_initialized = true;
}

// Update rocket animation state machine (OPTIMIZED)
void update_rocket_animation(void) {
    if (!rockets_initialized) {
        init_rockets();
    }

    uint32_t current_time = timer_read32();

    for (uint8_t i = 0; i < NUM_ROCKETS; i++) {
        rocket_t *r = &rockets[i];
        uint32_t elapsed = current_time - r->state_timer;

        switch (r->state) {
            case ROCKET_INACTIVE:
                // Wait for launch time (cycle every 2.0 seconds + stagger offset for continuous coverage)
                if (elapsed >= 2000) {
                    r->state = ROCKET_LAUNCHING;
                    r->state_timer = current_time;
                    r->y = 148;  // Reset to just above ground (ground is at y=150)
                    r->x = r->launch_x;
                }
                break;

            case ROCKET_LAUNCHING:
                // Brief launch phase (100ms)
                if (elapsed >= 100) {
                    r->state = ROCKET_ASCENDING;
                    r->state_timer = current_time;
                }
                break;

            case ROCKET_ASCENDING:
                // Move upward (3 pixels per update)
                r->y -= 3;

                // Check if reached target height
                if (r->y <= r->target_y) {
                    r->y = r->target_y;
                    r->state = ROCKET_EXPLODING;
                    r->state_timer = current_time;

                    // Initialize explosion particles (OPTIMIZED: pre-calculated angles)
                    for (uint8_t j = 0; j < NUM_EXPLOSION_PARTICLES; j++) {
                        // Base speed: 20 (scaled by 16) with some variation
                        int16_t speed = 20 + ((j & 1) ? 8 : 0);  // Alternate between 20 and 28

                        r->particles[j].x = r->x << 4;  // Scale position by 16
                        r->particles[j].y = r->y << 4;
                        r->particles[j].vx = (cos_table[j] * speed) >> 4;  // Use pre-calculated values
                        r->particles[j].vy = (sin_table[j] * speed) >> 4;
                        r->particles[j].brightness = 255;
                    }
                }
                break;

            case ROCKET_EXPLODING:
                // Expand explosion for 300ms
                // Update explosion particles (OPTIMIZED: simple physics)
                for (uint8_t j = 0; j < NUM_EXPLOSION_PARTICLES; j++) {
                    r->particles[j].x += r->particles[j].vx;
                    r->particles[j].y += r->particles[j].vy;
                    r->particles[j].vy += 1;  // Gravity (scaled by 16, so this is 1/16 pixel/frame)
                }

                if (elapsed >= 300) {
                    r->state = ROCKET_FADING;
                    r->state_timer = current_time;
                }
                break;

            case ROCKET_FADING:
                // Fade out over 700ms (reduced from 800ms)
                for (uint8_t j = 0; j < NUM_EXPLOSION_PARTICLES; j++) {
                    // Slow down movement (multiply by 0.8 = multiply by 13/16)
                    r->particles[j].x += (r->particles[j].vx * 13) >> 4;
                    r->particles[j].y += r->particles[j].vy;
                    r->particles[j].vy += 1;  // Gravity

                    // Fade brightness faster
                    if (r->particles[j].brightness > 10) {
                        r->particles[j].brightness -= 10;
                    } else {
                        r->particles[j].brightness = 0;
                    }
                }

                if (elapsed >= 700) {
                    r->state = ROCKET_INACTIVE;
                    r->state_timer = current_time;

                    // Re-randomize position and height for next launch
                    // Base positions for each rocket
                    const int16_t base_positions[NUM_ROCKETS] = {25, 45, 67, 90, 110};
                    const int16_t base_heights[NUM_ROCKETS] = {45, 55, 50, 60, 52};

                    // Use current time as new seed for variation
                    uint32_t seed = current_time;

                    // Add randomization to launch position (±12 pixels)
                    int16_t random_x = ((seed * (i + 11) * 19) % 25) - 12;
                    int16_t new_launch_x = base_positions[i] + random_x;
                    if (new_launch_x < 5) new_launch_x = 5;
                    if (new_launch_x > 130) new_launch_x = 130;

                    // Add randomization to target height (±15 pixels)
                    int16_t random_y = ((seed * (i + 5) * 23) % 31) - 15;
                    int16_t new_target_y = base_heights[i] + random_y;
                    if (new_target_y < 30) new_target_y = 30;
                    if (new_target_y > 80) new_target_y = 80;

                    // Update for next cycle
                    r->launch_x = new_launch_x;
                    r->target_y = new_target_y;
                }
                break;
        }
    }
}

// Draw a single rocket (OPTIMIZED)
void draw_rocket(rocket_t *rocket) {
    switch (rocket->state) {
        case ROCKET_INACTIVE:
            // Don't draw
            break;

        case ROCKET_LAUNCHING:
        case ROCKET_ASCENDING: {
            // Draw rocket body (bright circle) - NO TRAIL for better performance
            fb_circle_hsv(rocket->x, rocket->y, 2, rocket->hue, 255, 255, true);
            break;
        }

        case ROCKET_EXPLODING:
        case ROCKET_FADING:
            // Draw explosion particles (OPTIMIZED: no trailing effect, direct drawing)
            for (uint8_t i = 0; i < NUM_EXPLOSION_PARTICLES; i++) {
                if (rocket->particles[i].brightness > 0) {
                    // Convert fixed-point back to pixels
                    int16_t px = rocket->particles[i].x >> 4;
                    int16_t py = rocket->particles[i].y >> 4;

                    // Bounds check (optimized: single check)
                    if (px >= 0 && px < 135 && py >= 0 && py < 152) {
                        // Draw particle (no trail for better performance)
                        fb_circle_hsv(px, py, 2, rocket->hue, 255, rocket->particles[i].brightness, true);
                    }
                }
            }
            break;
    }
}

// Draw New Year's Eve elements
void draw_newyear_elements(void) {
    // Initialize if needed
    if (!rockets_initialized) {
        init_rockets();
    }

    // Draw all active rockets (no clearing needed - they're drawn as overlays)
    for (uint8_t i = 0; i < NUM_ROCKETS; i++) {
        draw_rocket(&rockets[i]);
    }
}

// Reset New Year animations
void reset_newyear_animations(void) {
    rockets_initialized = false;
    newyear_scene_drawn = false;
}
