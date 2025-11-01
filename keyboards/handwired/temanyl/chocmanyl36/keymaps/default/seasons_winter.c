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
#include "seasons_winter.h"
#include "display.h"
#include "framebuffer.h"

// Cloud animation state
cloud_t clouds[NUM_CLOUDS];
bool cloud_initialized = false;
bool cloud_background_saved = false;
uint32_t cloud_animation_timer = 0;

// Forward declarations
extern uint8_t current_month;

// Initialize clouds
void init_clouds(void) {
    if (cloud_initialized) return;

    // Initialize 5 clouds evenly spaced across the screen
    // All clouds move at same speed to prevent overlap artifacts
    // Spacing: ~50 pixels apart to cover the full width (135 pixels + margins)

    // Cloud 0
    clouds[0].x = 25;
    clouds[0].y = 35;
    clouds[0].vx = -1;

    // Cloud 1
    clouds[1].x = 70;
    clouds[1].y = 28;
    clouds[1].vx = -1;

    // Cloud 2
    clouds[2].x = 115;
    clouds[2].y = 42;
    clouds[2].vx = -1;

    // Cloud 3
    clouds[3].x = 160;
    clouds[3].y = 32;
    clouds[3].vx = -1;

    // Cloud 4
    clouds[4].x = 205;
    clouds[4].y = 38;
    clouds[4].vx = -1;

    cloud_initialized = true;
}

// Draw a single cloud
void draw_cloud(int16_t x, int16_t y) {
    // Don't draw clouds that are completely off-screen
    if (x < -30 || x > 165) return;

    // Cloud shape using circles (gray/white)
    // Bounds: x-13 to x+15, y-8 to y+8
    fb_circle_hsv(x, y, 8, 0, 0, 160, true);          // Main body
    fb_circle_hsv(x + 9, y + 2, 6, 0, 0, 160, true);  // Right bump
    fb_circle_hsv(x - 7, y + 2, 6, 0, 0, 160, true);  // Left bump
    fb_circle_hsv(x + 4, y - 3, 5, 0, 0, 150, true);  // Top bump
}

// Animate clouds (updates positions only, drawing done by caller)
void animate_clouds(void) {
    if (!cloud_initialized || !cloud_background_saved) {
        return;
    }

    // Only animate clouds in winter (season 0) and fall (season 3)
    uint8_t season = (current_month == 12 || current_month <= 2) ? 0 :
                     (current_month >= 3 && current_month <= 5) ? 1 :
                     (current_month >= 6 && current_month <= 8) ? 2 : 3;

    bool is_fall = (season == 3);
    bool is_winter = (season == 0);

    // Only animate clouds in winter and fall
    if (!is_winter && !is_fall) {
        return;
    }

    // Determine how many clouds to animate based on season
    uint8_t num_active_clouds = is_fall ? 5 : 3;  // 5 clouds in fall, 3 in winter

    // Update cloud positions
    for (uint8_t i = 0; i < num_active_clouds; i++) {
        // Move cloud left
        clouds[i].x += clouds[i].vx;

        // Check if cloud has moved completely off the left side
        if (clouds[i].x < -20) {
            // Respawn on the right side, maintaining spacing
            // Since all clouds move at same speed, find the rightmost cloud
            int16_t rightmost_x = -100;
            for (uint8_t j = 0; j < NUM_CLOUDS; j++) {
                if (j != i && clouds[j].x > rightmost_x) {
                    rightmost_x = clouds[j].x;
                }
            }
            // Place this cloud 45 pixels to the right of the rightmost cloud
            clouds[i].x = rightmost_x + 45;
            // Vary y position slightly (between 25-45)
            clouds[i].y = 25 + ((i * 7) % 20);
        }
    }

    // NOTE: Drawing is handled by caller to ensure consistent z-ordering
}

// Reset winter animations
void reset_winter_animations(void) {
    cloud_initialized = false;
    cloud_background_saved = false;
}

// Draw winter-specific scene elements
void draw_winter_scene_elements(void) {
    // Winter clouds - animated (will be drawn after background is saved)
    if (!cloud_initialized) {
        init_clouds();
    }

    // Snowflakes (21 total)
    uint16_t snow_positions[][2] = {
        {15, 50}, {40, 70}, {65, 90}, {85, 60}, {110, 80}, {25, 100}, {55, 120}, {95, 110}, {120, 65}, {10, 45}, {32, 85},
        {48, 105}, {72, 55}, {90, 75}, {105, 95}, {125, 115}, {18, 130}, {35, 62}, {62, 88}, {78, 108}, {98, 72}
    };
    for (uint8_t i = 0; i < 21; i++) {
        fb_rect_hsv(snow_positions[i][0], snow_positions[i][1], snow_positions[i][0] + 2, snow_positions[i][1] + 2, 170, 80, 255, true);
        fb_rect_hsv(snow_positions[i][0] - 2, snow_positions[i][1] + 1, snow_positions[i][0] + 4, snow_positions[i][1] + 1, 170, 80, 255, true);
        fb_rect_hsv(snow_positions[i][0] + 1, snow_positions[i][1] - 2, snow_positions[i][0] + 1, snow_positions[i][1] + 4, 170, 80, 255, true);
    }

    // Snow on ground
    uint16_t ground_y = 150;
    fb_rect_hsv(0, ground_y - 2, 134, ground_y, 0, 0, 240, true);
    struct { uint16_t x; uint8_t height; } snow_drifts[] = {
        {0, 2}, {20, 4}, {45, 3}, {70, 5}, {95, 3}, {115, 4}
    };
    for (uint8_t i = 0; i < 6; i++) {
        fb_rect_hsv(snow_drifts[i].x, ground_y - snow_drifts[i].height, snow_drifts[i].x + 20, ground_y, 170, 40, 255, true);
    }
}
