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

// Animation parameters
#define BEE_ORBIT_FREQ 0.002f           // Circular orbit speed (slower than buzz)
#define BEE_BUZZ_FREQ_X 0.025f          // Fast horizontal buzz frequency
#define BEE_BUZZ_FREQ_Y 0.030f          // Fast vertical buzz frequency
#define BEE_BUZZ_AMPLITUDE 1.5f         // Small buzz amplitude
#define WING_FLAP_INTERVAL_BEE 60       // Very fast wing flap (60ms)

// Initialize a single bee instance
void bee_init(bee_t* bee, float center_x, float center_y, float orbit_radius, float orbit_phase) {
    bee->center_x = center_x;
    bee->center_y = center_y;
    bee->orbit_radius = orbit_radius;
    bee->orbit_phase = orbit_phase;
    bee->buzz_phase_x = 0.0f;
    bee->buzz_phase_y = 0.0f;
    bee->x = center_x;
    bee->y = center_y;
    bee->wing_frame = 0;
    bee->last_update = timer_read32();
}

// Update a single bee's animation state
void bee_update(bee_t* bee) {
    uint32_t now = timer_read32();
    uint32_t elapsed = now - bee->last_update;

    // Update orbit phase (circular motion around flower)
    bee->orbit_phase += BEE_ORBIT_FREQ * elapsed;

    // Update buzz phases (fast vibration)
    bee->buzz_phase_x += BEE_BUZZ_FREQ_X * elapsed;
    bee->buzz_phase_y += BEE_BUZZ_FREQ_Y * elapsed;

    // Calculate position: center + orbit + buzz
    float orbit_x = bee->orbit_radius * cosf(bee->orbit_phase);
    float orbit_y = bee->orbit_radius * sinf(bee->orbit_phase);

    float buzz_x = BEE_BUZZ_AMPLITUDE * sinf(bee->buzz_phase_x);
    float buzz_y = BEE_BUZZ_AMPLITUDE * sinf(bee->buzz_phase_y);

    bee->x = bee->center_x + orbit_x + buzz_x;
    bee->y = bee->center_y + orbit_y + buzz_y;

    // Update wing animation frame (fast flapping)
    if (elapsed >= WING_FLAP_INTERVAL_BEE) {
        bee->wing_frame = (bee->wing_frame + 1) % 2;  // 2-frame animation
    }

    // Always update timer
    bee->last_update = now;
}

// Draw a single bee
void bee_draw(const bee_t* bee) {
    int16_t x = (int16_t)bee->x;
    int16_t y = (int16_t)bee->y;
    uint8_t wing_frame = bee->wing_frame;

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

// Get bee's bounding box
void bee_get_bounds(const bee_t* bee, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    int16_t x = (int16_t)bee->x;
    int16_t y = (int16_t)bee->y;

    *x1 = x - (BEE_WIDTH / 2);
    *y1 = y - (BEE_HEIGHT / 2);
    *x2 = x + (BEE_WIDTH / 2);
    *y2 = y + (BEE_HEIGHT / 2);
}

// Check if a point is inside the bee's bounds
bool bee_contains_point(const bee_t* bee, int16_t px, int16_t py) {
    int16_t x1, y1, x2, y2;
    bee_get_bounds(bee, &x1, &y1, &x2, &y2);
    return (px >= x1 && px <= x2 && py >= y1 && py <= y2);
}
