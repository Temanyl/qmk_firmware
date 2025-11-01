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

// Bird animation states (exposed for per-object animation)
bird_state_t birds[NUM_SPRING_BIRDS];

// Initial bird configuration (base_y, velocity_x, bob_phase)
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

// Animation parameters
#define BIRD_BOB_AMPLITUDE 3.0f      // Vertical bobbing range
#define BIRD_BOB_FREQUENCY 0.003f    // Bobbing speed
#define WING_FLAP_INTERVAL 150       // ms between wing animation frames
#define BIRD_WRAP_MARGIN 15          // Pixels to wrap before edge

// Initialize bird animations
void birds_init(void) {
    for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
        birds[i].x = (i * 25.0f) + 15.0f;  // Spread birds across sky
        birds[i].base_y = bird_config[i].base_y;
        birds[i].y = birds[i].base_y;
        birds[i].velocity_x = bird_config[i].velocity_x;
        birds[i].bob_phase = bird_config[i].bob_phase;
        birds[i].wing_frame = 0;
        birds[i].last_update = timer_read32();
    }
}

// Reset bird animations (same as init)
void birds_reset(void) {
    birds_init();
}

// Update bird animations
void birds_update(void) {
    uint32_t now = timer_read32();

    for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
        uint32_t elapsed = now - birds[i].last_update;

        // Update horizontal position
        birds[i].x += birds[i].velocity_x;

        // Wrap around screen with margin
        if (birds[i].x > FB_WIDTH + BIRD_WRAP_MARGIN) {
            birds[i].x = -BIRD_WRAP_MARGIN;
        }

        // Update vertical bobbing using sine wave
        birds[i].bob_phase += BIRD_BOB_FREQUENCY * elapsed;
        birds[i].y = birds[i].base_y + (BIRD_BOB_AMPLITUDE * sinf(birds[i].bob_phase));

        // Update wing animation frame
        if (elapsed >= WING_FLAP_INTERVAL) {
            birds[i].wing_frame = (birds[i].wing_frame + 1) % 4;  // 4-frame animation
        }

        // Always update timer (not just on wing flap)
        birds[i].last_update = now;
    }
}

// Draw a single bird with current animation frame
static void draw_bird(int16_t x, int16_t y, uint8_t wing_frame) {
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

// Draw a single bird by index
void bird_draw_single(uint8_t index) {
    if (index >= NUM_SPRING_BIRDS) return;
    draw_bird((int16_t)birds[index].x, (int16_t)birds[index].y, birds[index].wing_frame);
}

// Draw all spring birds
void birds_draw_all(void) {
    for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
        bird_draw_single(i);
    }
}
