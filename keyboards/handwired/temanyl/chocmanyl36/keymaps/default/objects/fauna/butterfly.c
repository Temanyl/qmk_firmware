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

// Butterfly animation states (exposed for per-object animation)
butterfly_state_t butterflies[NUM_SPRING_BUTTERFLIES];

// Initial butterfly configuration (base_x, base_y, hue, flutter_phase_x, flutter_phase_y, amp_x, amp_y)
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

// Animation parameters
#define BUTTERFLY_FLUTTER_FREQ_X 0.004f          // Horizontal flutter frequency
#define BUTTERFLY_FLUTTER_FREQ_Y 0.006f          // Vertical flutter frequency (faster for figure-8)
#define WING_FLAP_INTERVAL_BUTTERFLY 120         // ms between wing animation frames
#define BUTTERFLY_WANDER_MIN_INTERVAL 4000       // Min time between wanders (4 sec)
#define BUTTERFLY_WANDER_MAX_INTERVAL 10000      // Max time between wanders (10 sec)
#define BUTTERFLY_WANDER_DURATION 3000           // How long a wander lasts (3 sec)
#define BUTTERFLY_WANDER_DISTANCE 15.0f          // Max distance to wander from base

// Initialize butterfly animations
void butterflies_init(void) {
    uint32_t now = timer_read32();
    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        butterflies[i].base_x = butterfly_config[i].base_x;
        butterflies[i].base_y = butterfly_config[i].base_y;
        butterflies[i].x = butterflies[i].base_x;
        butterflies[i].y = butterflies[i].base_y;
        butterflies[i].hue = butterfly_config[i].hue;
        butterflies[i].flutter_phase_x = butterfly_config[i].flutter_phase_x;
        butterflies[i].flutter_phase_y = butterfly_config[i].flutter_phase_y;
        butterflies[i].amplitude_x = butterfly_config[i].amplitude_x;
        butterflies[i].amplitude_y = butterfly_config[i].amplitude_y;
        butterflies[i].wander_offset_x = 0.0f;
        butterflies[i].wander_offset_y = 0.0f;
        butterflies[i].is_wandering = false;
        // Stagger initial wander timers so they don't all wander at once
        butterflies[i].wander_timer = now + (i * 1000);
        butterflies[i].wing_frame = 0;
        butterflies[i].last_update = now;
    }
}

// Reset butterfly animations (same as init)
void butterflies_reset(void) {
    butterflies_init();
}

// Pseudo-random number generator using timer and index
static float pseudo_random(uint32_t seed, uint8_t index) {
    uint32_t hash = seed * 1103515245 + index * 12345;
    return (float)(hash % 1000) / 1000.0f;
}

// Update butterfly animations
void butterflies_update(void) {
    uint32_t now = timer_read32();

    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        uint32_t elapsed = now - butterflies[i].last_update;

        // Check wander state transitions
        if (!butterflies[i].is_wandering) {
            // Not wandering - check if it's time to start a new wander
            if (now - butterflies[i].wander_timer >= BUTTERFLY_WANDER_MIN_INTERVAL) {
                // Use pseudo-random to decide if we should wander now
                float rand_val = pseudo_random(now, i);
                uint32_t wander_interval = BUTTERFLY_WANDER_MIN_INTERVAL +
                    (uint32_t)(rand_val * (BUTTERFLY_WANDER_MAX_INTERVAL - BUTTERFLY_WANDER_MIN_INTERVAL));

                if (now - butterflies[i].wander_timer >= wander_interval) {
                    // Start wandering!
                    butterflies[i].is_wandering = true;
                    butterflies[i].wander_timer = now;

                    // Pick a random wander target (relative to base position)
                    float rand_x = pseudo_random(now + i, i * 2);
                    float rand_y = pseudo_random(now + i * 2, i * 3);

                    // Convert to -1.0 to 1.0 range, then scale by wander distance
                    butterflies[i].wander_offset_x = (rand_x * 2.0f - 1.0f) * BUTTERFLY_WANDER_DISTANCE;
                    butterflies[i].wander_offset_y = (rand_y * 2.0f - 1.0f) * BUTTERFLY_WANDER_DISTANCE;
                }
            }
        } else {
            // Currently wandering - check if wander duration is over
            if (now - butterflies[i].wander_timer >= BUTTERFLY_WANDER_DURATION) {
                butterflies[i].is_wandering = false;
                butterflies[i].wander_timer = now;
                // Gradually return to base (offsets will decay below)
            }
        }

        // Gradually apply or remove wander offset
        if (butterflies[i].is_wandering) {
            // Already at wander offset (set when entering wander mode)
            // Keep it steady
        } else {
            // Not wandering - gradually return to base position
            if (fabsf(butterflies[i].wander_offset_x) > 0.1f || fabsf(butterflies[i].wander_offset_y) > 0.1f) {
                butterflies[i].wander_offset_x *= 0.95f;  // Decay by 5% each frame
                butterflies[i].wander_offset_y *= 0.95f;
            } else {
                butterflies[i].wander_offset_x = 0.0f;
                butterflies[i].wander_offset_y = 0.0f;
            }
        }

        // Update flutter phases
        butterflies[i].flutter_phase_x += BUTTERFLY_FLUTTER_FREQ_X * elapsed;
        butterflies[i].flutter_phase_y += BUTTERFLY_FLUTTER_FREQ_Y * elapsed;

        // Calculate position: base + wander_offset + flutter pattern
        float flutter_x = butterflies[i].amplitude_x * sinf(butterflies[i].flutter_phase_x);
        float flutter_y = butterflies[i].amplitude_y * sinf(butterflies[i].flutter_phase_y);

        butterflies[i].x = butterflies[i].base_x + butterflies[i].wander_offset_x + flutter_x;
        butterflies[i].y = butterflies[i].base_y + butterflies[i].wander_offset_y + flutter_y;

        // Update wing animation frame
        if (elapsed >= WING_FLAP_INTERVAL_BUTTERFLY) {
            butterflies[i].wing_frame = (butterflies[i].wing_frame + 1) % 4;  // 4-frame animation
        }

        // Always update timer (not just on wing flap)
        butterflies[i].last_update = now;
    }
}

// Draw a single butterfly with current animation frame
static void draw_butterfly(int16_t x, int16_t y, uint8_t hue, uint8_t wing_frame) {
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

// Draw a single butterfly by index
void butterfly_draw_single(uint8_t index) {
    if (index >= NUM_SPRING_BUTTERFLIES) return;
    draw_butterfly((int16_t)butterflies[index].x, (int16_t)butterflies[index].y,
                  butterflies[index].hue, butterflies[index].wing_frame);
}

// Draw all spring butterflies
void butterflies_draw_all(void) {
    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        butterfly_draw_single(i);
    }
}
