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

#include "firefly.h"
#include "../../display/framebuffer.h"
#include "timer.h"
#include <math.h>

// Firefly animation states (exposed for per-object animation)
firefly_state_t fireflies[NUM_SUMMER_FIREFLIES];

// Initial firefly configuration (base_x, base_y, drift_phase_x, drift_phase_y)
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

// Animation parameters
#define FIREFLY_DRIFT_FREQ_X 0.0015f        // Horizontal drift speed (gentle, slow)
#define FIREFLY_DRIFT_FREQ_Y 0.002f         // Vertical drift speed (gentle, slow)
#define FIREFLY_DRIFT_AMPLITUDE_X 8.0f      // Horizontal drift range (pixels)
#define FIREFLY_DRIFT_AMPLITUDE_Y 6.0f      // Vertical drift range (pixels)
#define FIREFLY_BLINK_ON_MIN 1200           // Min time lit (ms)
#define FIREFLY_BLINK_ON_MAX 2500           // Max time lit (ms)
#define FIREFLY_BLINK_OFF_MIN 800           // Min time dark (ms)
#define FIREFLY_BLINK_OFF_MAX 3000          // Max time dark (ms)
#define FIREFLY_BRIGHTNESS_MAX 255          // Maximum brightness
#define FIREFLY_FADE_SPEED 8                // Brightness change per update (higher = faster fade)

// Pseudo-random number generator using timer and index
static uint32_t pseudo_random(uint32_t seed, uint8_t index) {
    uint32_t hash = seed * 1103515245 + index * 12345;
    return hash;
}

// Initialize firefly animations
void fireflies_init(void) {
    uint32_t now = timer_read32();
    for (uint8_t i = 0; i < NUM_SUMMER_FIREFLIES; i++) {
        fireflies[i].base_x = firefly_config[i].base_x;
        fireflies[i].base_y = firefly_config[i].base_y;
        fireflies[i].x = fireflies[i].base_x;
        fireflies[i].y = fireflies[i].base_y;
        fireflies[i].drift_phase_x = firefly_config[i].drift_phase_x;
        fireflies[i].drift_phase_y = firefly_config[i].drift_phase_y;

        // Initialize with random on/off state
        uint32_t rand_val = pseudo_random(now, i);
        fireflies[i].is_lit = (rand_val % 2) == 0;

        // Set initial blink duration
        if (fireflies[i].is_lit) {
            fireflies[i].next_blink_duration = FIREFLY_BLINK_ON_MIN +
                (pseudo_random(now + i, i) % (FIREFLY_BLINK_ON_MAX - FIREFLY_BLINK_ON_MIN));
        } else {
            fireflies[i].next_blink_duration = FIREFLY_BLINK_OFF_MIN +
                (pseudo_random(now + i, i) % (FIREFLY_BLINK_OFF_MAX - FIREFLY_BLINK_OFF_MIN));
        }

        fireflies[i].blink_timer = now;
        fireflies[i].brightness = fireflies[i].is_lit ? FIREFLY_BRIGHTNESS_MAX : 0;
        fireflies[i].last_update = now;
    }
}

// Reset firefly animations (same as init)
void fireflies_reset(void) {
    fireflies_init();
}

// Update firefly animations
void fireflies_update(void) {
    uint32_t now = timer_read32();

    for (uint8_t i = 0; i < NUM_SUMMER_FIREFLIES; i++) {
        uint32_t elapsed = now - fireflies[i].last_update;

        // Update drift phases (slow floating movement)
        fireflies[i].drift_phase_x += FIREFLY_DRIFT_FREQ_X * elapsed;
        fireflies[i].drift_phase_y += FIREFLY_DRIFT_FREQ_Y * elapsed;

        // Calculate position: base + drift
        float drift_x = FIREFLY_DRIFT_AMPLITUDE_X * sinf(fireflies[i].drift_phase_x);
        float drift_y = FIREFLY_DRIFT_AMPLITUDE_Y * sinf(fireflies[i].drift_phase_y);

        fireflies[i].x = fireflies[i].base_x + drift_x;
        fireflies[i].y = fireflies[i].base_y + drift_y;

        // Check if it's time to change blink state
        if (now - fireflies[i].blink_timer >= fireflies[i].next_blink_duration) {
            // Toggle lit state
            fireflies[i].is_lit = !fireflies[i].is_lit;
            fireflies[i].blink_timer = now;

            // Set next duration
            if (fireflies[i].is_lit) {
                fireflies[i].next_blink_duration = FIREFLY_BLINK_ON_MIN +
                    (pseudo_random(now + i, i * 2) % (FIREFLY_BLINK_ON_MAX - FIREFLY_BLINK_ON_MIN));
            } else {
                fireflies[i].next_blink_duration = FIREFLY_BLINK_OFF_MIN +
                    (pseudo_random(now + i, i * 3) % (FIREFLY_BLINK_OFF_MAX - FIREFLY_BLINK_OFF_MIN));
            }
        }

        // Update brightness with smooth fade in/out
        uint8_t target_brightness = fireflies[i].is_lit ? FIREFLY_BRIGHTNESS_MAX : 0;

        if (fireflies[i].brightness < target_brightness) {
            // Fade in
            int16_t new_brightness = (int16_t)fireflies[i].brightness + FIREFLY_FADE_SPEED;
            if (new_brightness > target_brightness) {
                fireflies[i].brightness = target_brightness;
            } else {
                fireflies[i].brightness = (uint8_t)new_brightness;
            }
        } else if (fireflies[i].brightness > target_brightness) {
            // Fade out
            int16_t new_brightness = (int16_t)fireflies[i].brightness - FIREFLY_FADE_SPEED;
            if (new_brightness < target_brightness) {
                fireflies[i].brightness = target_brightness;
            } else {
                fireflies[i].brightness = (uint8_t)new_brightness;
            }
        }

        fireflies[i].last_update = now;
    }
}

// Draw a single firefly with glow effect
static void draw_firefly(int16_t x, int16_t y, uint8_t brightness) {
    if (brightness == 0) return;  // Don't draw if not lit

    // Firefly glow (greenish-yellow)
    uint8_t hue = 60;  // Greenish-yellow

    // Center (brightest)
    fb_set_pixel_hsv(x, y, hue, 200, brightness);

    // Glow around center (dimmer)
    uint8_t glow_brightness = brightness * 3 / 4;
    fb_set_pixel_hsv(x - 1, y, hue, 200, glow_brightness);
    fb_set_pixel_hsv(x + 1, y, hue, 200, glow_brightness);
    fb_set_pixel_hsv(x, y - 1, hue, 200, glow_brightness);
    fb_set_pixel_hsv(x, y + 1, hue, 200, glow_brightness);

    // Outer glow (even dimmer)
    uint8_t outer_glow = brightness / 2;
    fb_set_pixel_hsv(x - 1, y - 1, hue, 200, outer_glow);
    fb_set_pixel_hsv(x + 1, y - 1, hue, 200, outer_glow);
    fb_set_pixel_hsv(x - 1, y + 1, hue, 200, outer_glow);
    fb_set_pixel_hsv(x + 1, y + 1, hue, 200, outer_glow);
}

// Draw a single firefly by index
void firefly_draw_single(uint8_t index) {
    if (index >= NUM_SUMMER_FIREFLIES) return;

    // Always draw (even at low brightness for fade effect)
    // Only skip if completely dark
    if (fireflies[index].brightness > 0) {
        draw_firefly((int16_t)fireflies[index].x, (int16_t)fireflies[index].y,
                     fireflies[index].brightness);
    }
}

// Draw all summer fireflies (only those that are lit)
void fireflies_draw_all(void) {
    for (uint8_t i = 0; i < NUM_SUMMER_FIREFLIES; i++) {
        firefly_draw_single(i);
    }
}
