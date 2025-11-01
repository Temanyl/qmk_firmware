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

// Initial butterfly configuration (base_x, base_y, hue, flutter_phase_x, flutter_phase_y)
static const struct {
    uint16_t base_x;
    uint16_t base_y;
    uint8_t hue;
    float flutter_phase_x;
    float flutter_phase_y;
} butterfly_config[NUM_SPRING_BUTTERFLIES] = {
    {20,  115, 234, 0.0f,  0.0f},   // Purple butterfly
    {45,  125, 170, 1.2f,  0.5f},   // Blue butterfly
    {65,  120, 42,  2.4f,  1.0f},   // Yellow butterfly
    {85,  130, 200, 3.6f,  1.5f},   // Light blue butterfly
    {105, 118, 10,  0.8f,  2.0f},   // Orange butterfly
    {125, 135, 234, 2.0f,  2.5f},   // Purple butterfly
    {35,  128, 85,  1.5f,  0.8f},   // Green butterfly
    {75,  122, 42,  3.0f,  1.8f},   // Yellow butterfly
    {95,  133, 170, 0.5f,  2.2f}    // Blue butterfly
};

// Animation parameters
#define BUTTERFLY_FLUTTER_AMP_X 4.0f         // Horizontal flutter amplitude
#define BUTTERFLY_FLUTTER_AMP_Y 3.0f         // Vertical flutter amplitude
#define BUTTERFLY_FLUTTER_FREQ_X 0.004f      // Horizontal flutter frequency
#define BUTTERFLY_FLUTTER_FREQ_Y 0.006f      // Vertical flutter frequency (faster for figure-8)
#define WING_FLAP_INTERVAL_BUTTERFLY 120     // ms between wing animation frames

// Initialize butterfly animations
void butterflies_init(void) {
    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        butterflies[i].base_x = butterfly_config[i].base_x;
        butterflies[i].base_y = butterfly_config[i].base_y;
        butterflies[i].x = butterflies[i].base_x;
        butterflies[i].y = butterflies[i].base_y;
        butterflies[i].hue = butterfly_config[i].hue;
        butterflies[i].flutter_phase_x = butterfly_config[i].flutter_phase_x;
        butterflies[i].flutter_phase_y = butterfly_config[i].flutter_phase_y;
        butterflies[i].wing_frame = 0;
        butterflies[i].last_update = timer_read32();
    }
}

// Reset butterfly animations (same as init)
void butterflies_reset(void) {
    butterflies_init();
}

// Update butterfly animations
void butterflies_update(void) {
    uint32_t now = timer_read32();

    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        uint32_t elapsed = now - butterflies[i].last_update;

        // Update flutter phases
        butterflies[i].flutter_phase_x += BUTTERFLY_FLUTTER_FREQ_X * elapsed;
        butterflies[i].flutter_phase_y += BUTTERFLY_FLUTTER_FREQ_Y * elapsed;

        // Create figure-8 pattern with phase-shifted sine waves
        butterflies[i].x = butterflies[i].base_x +
                          (BUTTERFLY_FLUTTER_AMP_X * sinf(butterflies[i].flutter_phase_x));
        butterflies[i].y = butterflies[i].base_y +
                          (BUTTERFLY_FLUTTER_AMP_Y * sinf(butterflies[i].flutter_phase_y));

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
