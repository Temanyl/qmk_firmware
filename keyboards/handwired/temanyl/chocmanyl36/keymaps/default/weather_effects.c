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
#include "weather_effects.h"
#include "weather_transition.h"
#include "display/display.h"
#include "display/framebuffer.h"
#include "objects/weather/cloud.h"
#include "objects/weather/raindrop.h"
#include "objects/weather/wind.h"
#include "objects/effects/snowflake.h"
#include "objects/effects/snow_drift.h"
#include "objects/seasonal/snowman.h"

// Forward declarations
extern uint8_t current_month;
extern painter_device_t display;
extern weather_transition_t weather_transition;

// =============================================================================
// CLOUD SYSTEM (shared by rain and snow)
// =============================================================================

cloud_t clouds[NUM_CLOUDS];
bool cloud_initialized = false;
bool cloud_background_saved = false;
uint32_t cloud_animation_timer = 0;
static uint8_t last_cloud_count = 0;  // Track last cloud count to detect changes

// Get number of active clouds based on weather intensity
uint8_t weather_get_active_cloud_count(void) {
    weather_state_t weather = weather_transition.current_weather;

    if (weather == WEATHER_RAIN_LIGHT || weather == WEATHER_SNOW_LIGHT) {
        return 3;  // Light rain/snow: 3 clouds
    } else if (weather == WEATHER_RAIN_MEDIUM || weather == WEATHER_SNOW_MEDIUM) {
        return 4;  // Medium rain/snow: 4 clouds
    } else if (weather == WEATHER_RAIN_HEAVY || weather == WEATHER_SNOW_HEAVY) {
        return 5;  // Heavy rain/snow: 5 clouds
    } else if (weather == WEATHER_CLOUDY) {
        return 2;  // Partly cloudy: 2 white clouds
    } else if (weather == WEATHER_OVERCAST) {
        return 5;  // Overcast: 5 white clouds (full coverage)
    }
    return 0;  // Default: no clouds (sunny)
}

// Initialize clouds
void weather_clouds_init(void) {
    // Get current weather to determine number of active clouds
    uint8_t num_active = weather_get_active_cloud_count();

    // Force re-initialization if cloud count changed (even if already initialized)
    if (cloud_initialized && num_active == last_cloud_count) {
        return;  // Already initialized with correct count, no need to reinit
    }

    // Update tracked cloud count
    last_cloud_count = num_active;

    // If cloud count changed, we need to reset background to re-save with new positions
    if (cloud_initialized) {
        cloud_background_saved = false;
    }

    // Calculate spacing based on number of active clouds
    int16_t spacing;
    if (num_active == 2) {
        spacing = 70;  // 2 clouds: very wide spacing (partly cloudy)
    } else if (num_active == 3) {
        spacing = 45;  // 3 clouds: wide spacing
    } else if (num_active == 4) {
        spacing = 34;  // 4 clouds: medium spacing
    } else {
        spacing = 26;  // 5 clouds: tight spacing (fits perfectly on 135px screen)
    }

    // Initialize active clouds distributed across screen width
    // Calculate starting position to center the cloud group
    int16_t total_width = (num_active - 1) * spacing;
    int16_t start_x = (135 - total_width) / 2;  // Center horizontally

    // Ensure clouds fit on screen
    // Cloud extends +18 pixels to the right from center, so rightmost cloud center must be <= 117
    if (start_x < 10) start_x = 10;
    if (start_x + total_width > 117) start_x = 117 - total_width;  // Changed from 125 to 117

    // Get wind velocity for cloud movement
    int8_t vx = wind_get_cloud_velocity();

    for (uint8_t i = 0; i < num_active; i++) {
        int16_t x = start_x + (i * spacing);
        int16_t y = 25 + ((i * 7) % 18);  // Vary height between 25-43
        cloud_init(&clouds[i], x, y, vx);
    }

    // Move inactive clouds off-screen
    for (uint8_t i = num_active; i < NUM_CLOUDS; i++) {
        cloud_init(&clouds[i], 200, 30, vx);  // Off-screen to the right
    }

    cloud_initialized = true;
}

// Animate clouds (updates positions only, drawing done by caller)
void weather_clouds_animate(void) {
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

    // Get number of active clouds based on weather intensity
    uint8_t num_active_clouds = weather_get_active_cloud_count();

    // Move inactive clouds off-screen to the right
    for (uint8_t i = num_active_clouds; i < NUM_CLOUDS; i++) {
        clouds[i].x = 200;  // Off-screen to the right
        clouds[i].y = 30;
    }

    // Get current wind velocity for clouds
    int8_t vx = wind_get_cloud_velocity();

    // Update cloud positions for active clouds
    for (uint8_t i = 0; i < num_active_clouds; i++) {
        // Update cloud velocity to match current wind
        clouds[i].vx = vx;

        // Move cloud
        clouds[i].x += clouds[i].vx;

        // Respawn cloud when it goes off-screen
        if (clouds[i].vx < 0) {
            // Moving left - respawn from right when fully off left edge
            // Cloud left edge is at x-16, so when x < -16, the cloud is completely gone
            if (clouds[i].x < -16) {
                clouds[i].x = 145;  // Just off the right edge (135 + 10 pixels)
                clouds[i].y = 25 + ((i * 7) % 20);  // Vary y position (between 25-45)
            }
        } else if (clouds[i].vx > 0) {
            // Moving right - respawn from left when fully off right edge
            // Cloud right edge is at x+18, so when x > 135+18, the cloud is completely gone
            if (clouds[i].x > 153) {
                clouds[i].x = -10;  // Just off the left edge
                clouds[i].y = 25 + ((i * 7) % 20);  // Vary y position (between 25-45)
            }
        }
        // If vx == 0, clouds don't move or respawn
    }

    // NOTE: Drawing is handled by caller to ensure consistent z-ordering
}

// Reset cloud animation state
void weather_clouds_reset(void) {
    cloud_initialized = false;
    cloud_background_saved = false;
}

// =============================================================================
// RAIN SYSTEM
// =============================================================================

raindrop_t raindrops[NUM_RAINDROPS];
bool rain_initialized = false;
bool rain_background_saved = false;
uint32_t rain_animation_timer = 0;

// Get active raindrop count based on rain intensity
static uint8_t get_active_raindrop_count(void) {
    uint8_t intensity = weather_get_rain_intensity(weather_transition.current_weather);

    if (intensity == 1) return NUM_RAINDROPS / 3;      // Light rain: ~17 drops
    if (intensity == 2) return (NUM_RAINDROPS * 2) / 3; // Medium rain: ~33 drops
    if (intensity == 3) return NUM_RAINDROPS;           // Heavy rain: 50 drops
    return NUM_RAINDROPS / 3;  // Default to light
}

// Initialize rain system
void weather_rain_init(void) {
    if (rain_initialized) return;

    // Get current rain intensity to determine how many drops to initialize
    uint8_t intensity = weather_get_rain_intensity(weather_transition.current_weather);
    uint8_t drops_to_init = (intensity == 1) ? NUM_RAINDROPS / 3 :      // Light: ~17
                            (intensity == 2) ? (NUM_RAINDROPS * 2) / 3 : // Medium: ~33
                            NUM_RAINDROPS;                               // Heavy: 50

    // Initialize raindrop positions (only for the drops we'll use)
    uint16_t rain_positions[][2] = {
        {91, 86}, {25, 128}, {108, 61}, {62, 101}, {45, 74}, {119, 139}, {31, 52}, {76, 118}, {100, 93}, {53, 67},
        {17, 131}, {85, 79}, {69, 105}, {122, 49}, {38, 123}, {96, 84}, {58, 58}, {20, 143}, {106, 71}, {72, 113},
        {41, 96}, {115, 54}, {29, 136}, {83, 88}, {50, 109}, {124, 63}, {64, 121}, {18, 76}, {98, 99}, {56, 56},
        {36, 140}, {88, 82}, {67, 115}, {110, 69}, {42, 127}, {78, 91}, {26, 59}, {102, 103}, {60, 77}, {21, 133},
        {94, 94}, {48, 66}, {116, 51}, {33, 119}, {81, 87}, {52, 106}, {120, 73}, {39, 137}, {75, 98}, {104, 62}
    };

    // Initialize only the drops we need based on intensity
    for (uint8_t i = 0; i < drops_to_init; i++) {
        raindrops[i].x = rain_positions[i][0];
        raindrops[i].y = rain_positions[i][1];
    }

    // Set remaining drops off-screen (below ground) so they don't appear
    for (uint8_t i = drops_to_init; i < NUM_RAINDROPS; i++) {
        raindrops[i].x = 0;
        raindrops[i].y = 200;  // Off screen below ground
    }

    rain_initialized = true;
}

// Animate raindrops
void weather_rain_animate(void) {
    if (!rain_initialized || !rain_background_saved) {
        return;
    }

    // Animation logic controlled by weather state (checked in display.c)
    // No season check needed here anymore

    // Get number of active raindrops based on intensity
    uint8_t active_drops = get_active_raindrop_count();

    // Animate each raindrop
    for (uint8_t i = 0; i < active_drops; i++) {
        // Store old position
        int16_t old_x = raindrops[i].x;
        int16_t old_y = raindrops[i].y;

        // Restore old raindrop position from background
        fb_restore_from_background(old_x, old_y,
                                   old_x + RAINDROP_WIDTH - 1,
                                   old_y + RAINDROP_HEIGHT - 1);

        // If ghosts are active and might be underneath, redraw them
        // NOTE: This is called from seasons_fall.c which handles ghost redrawing
        // We'll keep the interface but the caller handles the actual ghost logic

        // Flush the old raindrop region to erase it from display
        fb_flush_region(display, old_x, old_y,
                       old_x + RAINDROP_WIDTH - 1,
                       old_y + RAINDROP_HEIGHT - 1);

        // Move raindrop down by 3 pixels
        raindrops[i].y += 3;

        // Add horizontal drift based on wind
        int8_t drift = wind_get_rain_drift();
        raindrops[i].x += drift;

        // Wrap around screen edges instead of clamping
        if (raindrops[i].x < 0) {
            raindrops[i].x = FB_WIDTH - RAINDROP_WIDTH;  // Wrap from left to right
        } else if (raindrops[i].x > FB_WIDTH - RAINDROP_WIDTH) {
            raindrops[i].x = 0;  // Wrap from right to left
        }

        // Reset raindrop to top if it goes below ground (y=150)
        if (raindrops[i].y >= 150) {
            // Reset to random position at top (below clouds, y=45-55)
            raindrops[i].y = 45 + (i * 7) % 10;
            // Reset to pseudo-random x position based on raindrop index
            raindrops[i].x = 10 + ((i * 13 + (i / 5) * 7) % 115);
            // Clamp to screen bounds
            if (raindrops[i].x < 0) raindrops[i].x = 0;
            if (raindrops[i].x > FB_WIDTH - RAINDROP_WIDTH) raindrops[i].x = FB_WIDTH - RAINDROP_WIDTH;
        }

        // Draw raindrop at new position
        if (raindrops[i].y >= 0 && raindrops[i].y < 150) {
            raindrop_draw(&raindrops[i]);

            // Flush the new raindrop region to draw it on display
            fb_flush_region(display, raindrops[i].x, raindrops[i].y,
                           raindrops[i].x + RAINDROP_WIDTH - 1,
                           raindrops[i].y + RAINDROP_HEIGHT - 1);
        }
    }
}

// Reset rain animation state
void weather_rain_reset(void) {
    rain_initialized = false;
    rain_background_saved = false;
}

// =============================================================================
// SNOW SYSTEM
// =============================================================================

snowflake_t snowflakes[NUM_SNOWFLAKES];
bool snowflake_initialized = false;
bool snowflake_background_saved = false;
uint32_t snowflake_animation_timer = 0;

snowman_t snowmen[NUM_SNOWMEN];
bool snowman_initialized = false;

// Get number of active snowflakes based on snow intensity
static uint8_t get_active_snowflake_count(void) {
    uint8_t intensity = weather_get_snow_intensity(weather_transition.current_weather);

    if (intensity == 1) return NUM_SNOWFLAKES / 4;      // Light snow: ~10 flakes
    if (intensity == 2) return NUM_SNOWFLAKES / 2;      // Medium snow: ~20 flakes
    if (intensity == 3) return NUM_SNOWFLAKES;          // Heavy snow: 40 flakes
    return NUM_SNOWFLAKES / 2;  // Default to medium
}

// Initialize snow system
void weather_snow_init(void) {
    if (snowflake_initialized) return;

    // Get current snow intensity to determine how many flakes to initialize
    uint8_t flakes_to_init = get_active_snowflake_count();

    // Initialize snowflake positions (only for active flakes)
    uint16_t snow_positions[][2] = {
        {15, 50}, {40, 70}, {65, 90}, {85, 60}, {110, 80}, {25, 100}, {55, 120}, {95, 110}, {120, 65}, {10, 45},
        {32, 85}, {48, 105}, {72, 55}, {90, 75}, {105, 95}, {125, 115}, {18, 130}, {35, 62}, {62, 88}, {78, 108},
        {98, 72}, {22, 95}, {47, 68}, {73, 122}, {103, 58}, {118, 87}, {28, 114}, {58, 77}, {88, 102}, {113, 71},
        {8, 125}, {38, 83}, {68, 96}, {93, 64}, {123, 106}, {13, 79}, {43, 118}, {77, 81}, {100, 91}, {128, 99}
    };

    // Initialize only the flakes we need based on intensity
    for (uint8_t i = 0; i < flakes_to_init; i++) {
        snowflakes[i].x = snow_positions[i][0];
        snowflakes[i].y = snow_positions[i][1];
    }

    // Set remaining flakes off-screen (below ground) so they don't appear
    for (uint8_t i = flakes_to_init; i < NUM_SNOWFLAKES; i++) {
        snowflakes[i].x = 0;
        snowflakes[i].y = 200;  // Off screen below ground
    }

    snowflake_initialized = true;
}

// Animate snowflakes
void weather_snow_animate(void) {
    if (!snowflake_initialized || !snowflake_background_saved) {
        return;
    }

    // Animation logic controlled by weather state (checked in display.c)
    // No season check needed here anymore

    // Get number of active snowflakes based on intensity
    uint8_t active_flakes = get_active_snowflake_count();

    // Animate each active snowflake
    for (uint8_t i = 0; i < active_flakes; i++) {
        // Restore old snowflake position from background
        int16_t bounds_x1, bounds_y1, bounds_x2, bounds_y2;
        snowflake_get_bounds(&snowflakes[i], &bounds_x1, &bounds_y1, &bounds_x2, &bounds_y2);
        fb_restore_from_background(bounds_x1, bounds_y1, bounds_x2, bounds_y2);

        // Flush the old snowflake region to erase it from display
        fb_flush_region(display, bounds_x1, bounds_y1, bounds_x2, bounds_y2);

        // Move snowflake down by 1 pixel (slower than rain)
        snowflakes[i].y += 1;

        // Add horizontal drift based on wind
        int8_t drift = wind_get_rain_drift();
        snowflakes[i].x += drift;

        // Wrap around screen edges instead of clamping
        if (snowflakes[i].x < 0) {
            snowflakes[i].x = 130;  // Wrap from left to right
        } else if (snowflakes[i].x > 130) {
            snowflakes[i].x = 0;    // Wrap from right to left
        }

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

// Draw snow ground effects (drifts and snowman)
void weather_snow_draw_ground_effects(void) {
    // Get snow intensity to control accumulation
    uint8_t snow_intensity = weather_get_snow_intensity(weather_transition.current_weather);

    // Determine snow accumulation based on intensity
    uint8_t ground_snow;
    if (snow_intensity == 1) {
        // Light snow: no accumulation, just flakes falling
        ground_snow = 0;
    } else {
        // Medium and heavy snow: full accumulation
        ground_snow = 255;
    }

    // Snow drifts on ground fade in/out gradually based on accumulation
    uint16_t ground_y = 150;
    snow_drifts_draw(ground_y, ground_snow);

    // Snowman appears ONLY in heavy snow (intensity 3)
    if (snow_intensity == 3 && ground_snow > 204) {
        if (!snowman_initialized) {
            // Place snowman to the left of the first tree, on the ground
            snowman_init(&snowmen[0], 15, 150, 6);
            snowman_initialized = true;
        }
        // Draw snowman (static, part of background)
        snowman_draw(&snowmen[0]);
    } else {
        // Not heavy snow or not enough accumulation - no snowman
        snowman_initialized = false;
    }
}

// Reset snow animation state
void weather_snow_reset(void) {
    snowflake_initialized = false;
    snowflake_background_saved = false;
    snowman_initialized = false;
}

// =============================================================================
// UNIFIED RESET FUNCTIONS
// =============================================================================

void weather_effects_reset_all(void) {
    weather_clouds_reset();
    weather_rain_reset();
    weather_snow_reset();
}
