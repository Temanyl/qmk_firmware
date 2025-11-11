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
#include "../../display/display.h"
#include "../../display/framebuffer.h"
#include "../../objects/weather/cloud.h"
#include "../../objects/effects/snowflake.h"
#include "../../objects/effects/snow_drift.h"
#include "../../objects/seasonal/snowman.h"

// Cloud animation state
cloud_t clouds[NUM_CLOUDS];
bool cloud_initialized = false;
bool cloud_background_saved = false;
uint32_t cloud_animation_timer = 0;

// Snowflake animation state
snowflake_t snowflakes[NUM_SNOWFLAKES];
bool snowflake_initialized = false;
bool snowflake_background_saved = false;
uint32_t snowflake_animation_timer = 0;

// Snowman state
snowman_t snowmen[NUM_SNOWMEN];
bool snowman_initialized = false;

// Forward declarations
extern uint8_t current_month;
extern painter_device_t display;

// Initialize clouds
void init_clouds(void) {
    if (cloud_initialized) return;

    // Initialize 5 clouds spread across a wide area
    // All clouds move at same speed to prevent overlap artifacts
    // Spacing: ~55 pixels apart for continuous flow (not all visible at once)

    cloud_init(&clouds[0], 10, 35, -1);
    cloud_init(&clouds[1], 65, 28, -1);
    cloud_init(&clouds[2], 120, 42, -1);
    cloud_init(&clouds[3], 175, 32, -1);
    cloud_init(&clouds[4], 230, 38, -1);

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
    uint8_t num_active_clouds = is_fall ? 5 : 4;  // 5 clouds in fall, 4 in winter

    // Update cloud positions
    for (uint8_t i = 0; i < num_active_clouds; i++) {
        // Move cloud left
        clouds[i].x += clouds[i].vx;

        // Respawn cloud when it's fully off the left edge of the display
        // Cloud left edge is at x-16, so when x < -16, the cloud is completely gone
        if (clouds[i].x < -16) {
            // Respawn on the right side, maintaining spacing
            // Since all clouds move at same speed, find the rightmost cloud
            int16_t rightmost_x = -100;
            for (uint8_t j = 0; j < num_active_clouds; j++) {
                if (j != i && clouds[j].x > rightmost_x) {
                    rightmost_x = clouds[j].x;
                }
            }
            // Place this cloud 55 pixels to the right of the rightmost cloud
            clouds[i].x = rightmost_x + 55;
            // Vary y position slightly (between 25-45)
            clouds[i].y = 25 + ((i * 7) % 20);
        }
    }

    // NOTE: Drawing is handled by caller to ensure consistent z-ordering
}

// Animate snowflakes
void animate_snowflakes(void) {
    if (!snowflake_initialized || !snowflake_background_saved) {
        return;
    }

    // Animation logic controlled by weather state (checked in display.c)
    // No season check needed here anymore

    // Animate each snowflake
    for (uint8_t i = 0; i < NUM_SNOWFLAKES; i++) {
        // Restore old snowflake position from background
        int16_t bounds_x1, bounds_y1, bounds_x2, bounds_y2;
        snowflake_get_bounds(&snowflakes[i], &bounds_x1, &bounds_y1, &bounds_x2, &bounds_y2);
        fb_restore_from_background(bounds_x1, bounds_y1, bounds_x2, bounds_y2);

        // Flush the old snowflake region to erase it from display
        fb_flush_region(display, bounds_x1, bounds_y1, bounds_x2, bounds_y2);

        // Move snowflake down by 1 pixel (slower than rain)
        snowflakes[i].y += 1;

        // Add gentle horizontal drift (alternates left/right based on snowflake index)
        // Some snowflakes drift left, some right, some don't drift
        if (i % 3 == 0 && snowflakes[i].y % 4 == 0) {
            snowflakes[i].x += 1;  // Drift right
        } else if (i % 3 == 1 && snowflakes[i].y % 4 == 0) {
            snowflakes[i].x -= 1;  // Drift left
        }
        // i % 3 == 2: no horizontal drift (straight down)

        // Reset snowflake to top if it goes below ground (y=150)
        if (snowflakes[i].y >= 150) {
            // Reset to random position at top (below clouds, y=45-55)
            snowflakes[i].y = 45 + (i * 7) % 10;
            // Reset to pseudo-random x position based on snowflake index
            snowflakes[i].x = 5 + ((i * 11 + (i / 5) * 13) % 125);
            // Clamp to screen bounds
            if (snowflakes[i].x < 0) snowflakes[i].x = 0;
            if (snowflakes[i].x > 130) snowflakes[i].x = 130;
        }

        // Draw snowflake at new position
        if (snowflakes[i].y >= 0 && snowflakes[i].y < 150) {
            snowflake_draw(&snowflakes[i]);

            // Get new bounds for flushing
            snowflake_get_bounds(&snowflakes[i], &bounds_x1, &bounds_y1, &bounds_x2, &bounds_y2);

            // Flush the new snowflake region to draw it on display
            fb_flush_region(display, bounds_x1, bounds_y1, bounds_x2, bounds_y2);
        }
    }
}

// Reset winter animations
void reset_winter_animations(void) {
    cloud_initialized = false;
    cloud_background_saved = false;
    snowflake_initialized = false;
    snowflake_background_saved = false;
    snowman_initialized = false;
}

// Draw SNOW WEATHER effects (weather-based, not seasonal)
void draw_snow_weather_elements(void) {
    // Snow clouds - animated (will be drawn after background is saved)
    if (!cloud_initialized) {
        init_clouds();
    }

    // Initialize snowflakes if not already done (but don't draw yet - draw after background is saved)
    if (!snowflake_initialized) {
        // Initialize snowflake positions
        uint16_t snow_positions[][2] = {
            {15, 50}, {40, 70}, {65, 90}, {85, 60}, {110, 80}, {25, 100}, {55, 120}, {95, 110}, {120, 65}, {10, 45},
            {32, 85}, {48, 105}, {72, 55}, {90, 75}, {105, 95}, {125, 115}, {18, 130}, {35, 62}, {62, 88}, {78, 108},
            {98, 72}, {22, 95}, {47, 68}, {73, 122}, {103, 58}, {118, 87}, {28, 114}, {58, 77}, {88, 102}, {113, 71},
            {8, 125}, {38, 83}, {68, 96}, {93, 64}, {123, 106}, {13, 79}, {43, 118}, {77, 81}, {100, 91}, {128, 99}
        };
        for (uint8_t i = 0; i < NUM_SNOWFLAKES; i++) {
            snowflakes[i].x = snow_positions[i][0];
            snowflakes[i].y = snow_positions[i][1];
        }
        snowflake_initialized = true;
    }

    // Initialize and draw snowman (weather-based: you need snow to build a snowman)
    if (!snowman_initialized) {
        // Place snowman to the left of the first tree, on the ground
        snowman_init(&snowmen[0], 15, 150, 6);
        snowman_initialized = true;
    }

    // Draw snowman (static, part of background)
    snowman_draw(&snowmen[0]);

    // Snow drifts on ground (weather-based: appears with snow)
    uint16_t ground_y = 150;
    snow_drifts_draw(ground_y);

    // NOTE: Snowflakes are NOT drawn here - they're drawn after background is saved
    // to prevent them from being baked into the background image
}

// Draw winter-specific scene elements (SEASONAL - only decorations, no weather)
void draw_winter_scene_elements(void) {
    // Winter season: bare trees (no leaves)
    // Trees are already drawn with season=0 parameter in scenes.c
    // No additional seasonal decorations needed for winter
}
