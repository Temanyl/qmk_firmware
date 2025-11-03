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

#include "bird.h"
#include "../../display/framebuffer.h"
#include "timer.h"
#include <math.h>

// Animation parameters
#define BIRD_BOB_AMPLITUDE 3.0f      // Vertical bobbing range
#define BIRD_BOB_FREQUENCY 0.003f    // Bobbing speed
#define WING_FLAP_INTERVAL 150       // ms between wing animation frames
#define BIRD_WRAP_MARGIN 15          // Pixels to wrap before edge

// Initialize a single bird instance
void bird_init(bird_t* bird, float x, float base_y, float velocity_x, float bob_phase) {
    bird->x = x;
    bird->base_y = base_y;
    bird->y = base_y;
    bird->velocity_x = velocity_x;
    bird->bob_phase = bob_phase;
    bird->wing_frame = 0;
    bird->last_update = timer_read32();
}

// Update a single bird's animation state
void bird_update(bird_t* bird) {
    uint32_t now = timer_read32();
    uint32_t elapsed = now - bird->last_update;

    // Update horizontal position
    bird->x += bird->velocity_x;

    // Wrap around screen with margin
    if (bird->x > FB_WIDTH + BIRD_WRAP_MARGIN) {
        bird->x = -BIRD_WRAP_MARGIN;
    }

    // Update vertical bobbing using sine wave
    bird->bob_phase += BIRD_BOB_FREQUENCY * elapsed;
    bird->y = bird->base_y + (BIRD_BOB_AMPLITUDE * sinf(bird->bob_phase));

    // Update wing animation frame
    if (elapsed >= WING_FLAP_INTERVAL) {
        bird->wing_frame = (bird->wing_frame + 1) % 4;  // 4-frame animation
    }

    // Always update timer
    bird->last_update = now;
}

// Draw a single bird
void bird_draw(const bird_t* bird) {
    int16_t x = (int16_t)bird->x;
    int16_t y = (int16_t)bird->y;
    uint8_t wing_frame = bird->wing_frame;

    // Wing shapes vary based on animation frame
    // Frame 0: Wings up (V shape pointing up)
    // Frame 1: Wings mid-up
    // Frame 2: Wings down (V shape pointing down)
    // Frame 3: Wings mid-down (back to mid)

    int8_t wing_offset_y = 0;
    int8_t wing_spread = 5;

    switch (wing_frame) {
        case 0:  // Wings up
            wing_offset_y = -3;
            wing_spread = 5;
            break;
        case 1:  // Wings mid-up
            wing_offset_y = -2;
            wing_spread = 4;
            break;
        case 2:  // Wings down
            wing_offset_y = 0;
            wing_spread = 5;
            break;
        case 3:  // Wings mid-down
            wing_offset_y = -1;
            wing_spread = 4;
            break;
    }

    // Draw bird body (small center point)
    fb_set_pixel_hsv(x, y, 0, 0, 100);

    // Draw left wing
    for (int8_t i = 1; i <= wing_spread; i++) {
        int16_t wing_x = x - i;
        int16_t wing_y = y + wing_offset_y + (i / 2);
        fb_set_pixel_hsv(wing_x, wing_y, 0, 0, 100);
        if (i <= 2) {  // Make wings slightly thicker
            fb_set_pixel_hsv(wing_x, wing_y + 1, 0, 0, 80);
        }
    }

    // Draw right wing
    for (int8_t i = 1; i <= wing_spread; i++) {
        int16_t wing_x = x + i;
        int16_t wing_y = y + wing_offset_y + (i / 2);
        fb_set_pixel_hsv(wing_x, wing_y, 0, 0, 100);
        if (i <= 2) {  // Make wings slightly thicker
            fb_set_pixel_hsv(wing_x, wing_y + 1, 0, 0, 80);
        }
    }
}

// Get bird's bounding box
void bird_get_bounds(const bird_t* bird, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    int16_t x = (int16_t)bird->x;
    int16_t y = (int16_t)bird->y;

    *x1 = x - (BIRD_WIDTH / 2);
    *y1 = y - 4;  // Above center for wing spread
    *x2 = x + (BIRD_WIDTH / 2);
    *y2 = y + 3;  // Below center for wing spread
}

// Check if a point is inside the bird's bounds
bool bird_contains_point(const bird_t* bird, int16_t px, int16_t py) {
    int16_t x1, y1, x2, y2;
    bird_get_bounds(bird, &x1, &y1, &x2, &y2);
    return (px >= x1 && px <= x2 && py >= y1 && py <= y2);
}
