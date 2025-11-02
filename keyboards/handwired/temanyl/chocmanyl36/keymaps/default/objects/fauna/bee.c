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

#include "bee.h"
#include "../../display/framebuffer.h"
#include "timer.h"
#include <math.h>

// Bee animation states (exposed for per-object animation)
bee_state_t bees[NUM_SUMMER_BEES];

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

// Animation parameters
#define BEE_ORBIT_FREQ 0.002f           // Circular orbit speed (slower than buzz)
#define BEE_BUZZ_FREQ_X 0.025f          // Fast horizontal buzz frequency
#define BEE_BUZZ_FREQ_Y 0.030f          // Fast vertical buzz frequency
#define BEE_BUZZ_AMPLITUDE 1.5f         // Small buzz amplitude
#define WING_FLAP_INTERVAL_BEE 60       // Very fast wing flap (60ms)

// Initialize bee animations
void bees_init(void) {
    uint32_t now = timer_read32();
    for (uint8_t i = 0; i < NUM_SUMMER_BEES; i++) {
        bees[i].center_x = bee_config[i].center_x;
        bees[i].center_y = bee_config[i].center_y;
        bees[i].orbit_radius = bee_config[i].orbit_radius;
        bees[i].orbit_phase = bee_config[i].orbit_phase;
        bees[i].buzz_phase_x = 0.0f;
        bees[i].buzz_phase_y = 0.0f;
        bees[i].x = bees[i].center_x;
        bees[i].y = bees[i].center_y;
        bees[i].wing_frame = 0;
        bees[i].last_update = now;
    }
}

// Reset bee animations (same as init)
void bees_reset(void) {
    bees_init();
}

// Update bee animations
void bees_update(void) {
    uint32_t now = timer_read32();

    for (uint8_t i = 0; i < NUM_SUMMER_BEES; i++) {
        uint32_t elapsed = now - bees[i].last_update;

        // Update orbit phase (circular motion around flower)
        bees[i].orbit_phase += BEE_ORBIT_FREQ * elapsed;

        // Update buzz phases (fast vibration)
        bees[i].buzz_phase_x += BEE_BUZZ_FREQ_X * elapsed;
        bees[i].buzz_phase_y += BEE_BUZZ_FREQ_Y * elapsed;

        // Calculate position: center + orbit + buzz
        float orbit_x = bees[i].orbit_radius * cosf(bees[i].orbit_phase);
        float orbit_y = bees[i].orbit_radius * sinf(bees[i].orbit_phase);

        float buzz_x = BEE_BUZZ_AMPLITUDE * sinf(bees[i].buzz_phase_x);
        float buzz_y = BEE_BUZZ_AMPLITUDE * sinf(bees[i].buzz_phase_y);

        bees[i].x = bees[i].center_x + orbit_x + buzz_x;
        bees[i].y = bees[i].center_y + orbit_y + buzz_y;

        // Update wing animation frame (fast flapping)
        if (elapsed >= WING_FLAP_INTERVAL_BEE) {
            bees[i].wing_frame = (bees[i].wing_frame + 1) % 2;  // 2-frame animation
        }

        // Always update timer
        bees[i].last_update = now;
    }
}

// Draw a single bee with current animation frame
static void draw_bee(int16_t x, int16_t y, uint8_t wing_frame) {
    // Bee body (yellow with black stripes)
    // Draw rounded body (3 pixels tall)
    fb_set_pixel_hsv(x, y - 1, 42, 255, 200);  // Yellow top
    fb_set_pixel_hsv(x, y, 0, 0, 0);           // Black middle stripe
    fb_set_pixel_hsv(x, y + 1, 42, 255, 200);  // Yellow bottom

    // Wings (white/translucent, position varies with frame)
    if (wing_frame == 0) {
        // Wings up
        fb_set_pixel_hsv(x - 2, y - 1, 0, 0, 180);  // Left wing
        fb_set_pixel_hsv(x + 2, y - 1, 0, 0, 180);  // Right wing
        fb_set_pixel_hsv(x - 1, y - 2, 0, 0, 150);  // Left wing tip
        fb_set_pixel_hsv(x + 1, y - 2, 0, 0, 150);  // Right wing tip
    } else {
        // Wings down/forward
        fb_set_pixel_hsv(x - 2, y, 0, 0, 150);      // Left wing
        fb_set_pixel_hsv(x + 2, y, 0, 0, 150);      // Right wing
    }

    // Head (small black dot)
    fb_set_pixel_hsv(x, y - 2, 0, 0, 50);
}

// Draw a single bee by index
void bee_draw_single(uint8_t index) {
    if (index >= NUM_SUMMER_BEES) return;
    draw_bee((int16_t)bees[index].x, (int16_t)bees[index].y, bees[index].wing_frame);
}

// Draw all summer bees
void bees_draw_all(void) {
    for (uint8_t i = 0; i < NUM_SUMMER_BEES; i++) {
        bee_draw_single(i);
    }
}
