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
#include "seasons_fall.h"
#include "../halloween/seasons_halloween.h"
#include "../winter/seasons_winter.h"
#include "../../display/framebuffer.h"
#include "../../display/display.h"
#include "../../objects/weather/raindrop.h"
#include "../../objects/flora/fallen_leaf.h"
#include "../../weather_transition.h"

// Rain animation state
bool rain_initialized = false;
bool rain_background_saved = false;
raindrop_t raindrops[NUM_RAINDROPS];
uint32_t rain_animation_timer = 0;

// Forward declarations
extern painter_device_t display;
extern uint8_t current_month;

// Get active raindrop count based on rain intensity
static uint8_t get_active_raindrop_count(void) {
    extern weather_transition_t weather_transition;
    uint8_t intensity = weather_get_rain_intensity(weather_transition.current_weather);

    if (intensity == 1) return NUM_RAINDROPS / 3;      // Light rain: ~17 drops
    if (intensity == 2) return (NUM_RAINDROPS * 2) / 3; // Medium rain: ~33 drops
    if (intensity == 3) return NUM_RAINDROPS;           // Heavy rain: 50 drops
    return NUM_RAINDROPS / 3;  // Default to light
}

// Animate raindrops
void animate_raindrops(void) {
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
        if (is_halloween_event() && ghost_initialized) {
            redraw_ghosts_in_region(old_x, old_y,
                                   old_x + RAINDROP_WIDTH - 1,
                                   old_y + RAINDROP_HEIGHT - 1);
        }

        // Flush the old raindrop region to erase it from display
        fb_flush_region(display, old_x, old_y,
                       old_x + RAINDROP_WIDTH - 1,
                       old_y + RAINDROP_HEIGHT - 1);

        // Move raindrop down by 3 pixels
        raindrops[i].y += 3;

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

// Reset fall animations
void reset_fall_animations(void) {
    rain_initialized = false;
    rain_background_saved = false;
}

// Draw RAIN WEATHER effects (weather-based, not seasonal)
void draw_rain_weather_elements(void) {
    // Rain clouds - animated (will be drawn after background is saved)
    if (!cloud_initialized) {
        init_clouds();
    }

    // Initialize rain if not already done (but don't draw yet - draw after background is saved)
    if (!rain_initialized) {
        // Get current rain intensity to determine how many drops to initialize
        extern weather_transition_t weather_transition;
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

    // NOTE: Raindrops are NOT drawn here - they're drawn after background is saved
    // to prevent them from being baked into the background image
}

// Draw fall-specific scene elements (SEASONAL - only decorations, no weather)
void draw_fall_scene_elements(void) {
    // Draw fallen leaves on the ground (seasonal decoration)
    fallen_leaves_draw_all();
}
