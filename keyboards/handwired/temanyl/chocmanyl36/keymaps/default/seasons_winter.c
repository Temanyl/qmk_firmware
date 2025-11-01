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
#include "objects/weather/cloud.h"
#include "objects/effects/snowflake.h"
#include "objects/effects/snow_drift.h"

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

    cloud_init(&clouds[0], 25, 35, -1);
    cloud_init(&clouds[1], 70, 28, -1);
    cloud_init(&clouds[2], 115, 42, -1);
    cloud_init(&clouds[3], 160, 32, -1);
    cloud_init(&clouds[4], 205, 38, -1);

    cloud_initialized = true;
}

// Draw a single cloud (wrapper for compatibility)
void draw_cloud(int16_t x, int16_t y) {
    cloud_t temp_cloud;
    cloud_init(&temp_cloud, x, y, 0);
    cloud_draw(&temp_cloud, CLOUD_TYPE_LIGHT);
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

    // Snowflakes
    snowflakes_draw_all();

    // Snow on ground with drifts
    uint16_t ground_y = 150;
    snow_drifts_draw(ground_y);
}
