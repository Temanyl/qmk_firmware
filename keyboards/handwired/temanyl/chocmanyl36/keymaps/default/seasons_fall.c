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
#include "seasons_halloween.h"
#include "seasons_winter.h"
#include "framebuffer.h"
#include "display.h"

// Rain animation state
bool rain_initialized = false;
bool rain_background_saved = false;
raindrop_t raindrops[NUM_RAINDROPS];
uint32_t rain_animation_timer = 0;

// Forward declarations
extern painter_device_t display;
extern uint8_t current_month;

// Animate raindrops
void animate_raindrops(void) {
    if (!rain_initialized || !rain_background_saved) {
        return;
    }

    // Get current season
    uint8_t season = (current_month == 12 || current_month <= 2) ? 0 :
                     (current_month >= 3 && current_month <= 5) ? 1 :
                     (current_month >= 6 && current_month <= 8) ? 2 : 3;

    // Only animate during fall (season 3)
    if (season != 3) {
        return;
    }

    // Animate each raindrop
    for (uint8_t i = 0; i < NUM_RAINDROPS; i++) {
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
            fb_rect_hsv(raindrops[i].x, raindrops[i].y,
                       raindrops[i].x + RAINDROP_WIDTH - 1,
                       raindrops[i].y + RAINDROP_HEIGHT - 1,
                       170, 150, 200, true);

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

// Draw fall-specific scene elements
void draw_fall_scene_elements(void) {
    // Fall clouds - animated (will be drawn after background is saved)
    if (!cloud_initialized) {
        init_clouds();
    }

    // Draw fallen leaves on the ground (just above ground line at y=150)
    struct { uint16_t x; uint8_t hue; } fallen_leaves[] = {
        {18, 10}, {35, 0}, {52, 25}, {68, 15}, {82, 8}, {95, 20}, {108, 5}, {122, 30},
        {25, 12}, {45, 18}, {62, 22}, {78, 28}, {92, 15}, {105, 10}, {118, 25}
    };
    for (uint8_t i = 0; i < 15; i++) {
        // Small leaves on ground (small circles just above ground line)
        fb_circle_hsv(fallen_leaves[i].x, 146, 2, fallen_leaves[i].hue, 255, 220, true);
    }

    // Initialize rain if not already done (but don't draw yet - draw after background is saved)
    if (!rain_initialized) {
        // Initialize raindrop positions
        uint16_t rain_positions[][2] = {
            {91, 86}, {25, 128}, {108, 61}, {62, 101}, {45, 74}, {119, 139}, {31, 52}, {76, 118}, {100, 93}, {53, 67},
            {17, 131}, {85, 79}, {69, 105}, {122, 49}, {38, 123}, {96, 84}, {58, 58}, {20, 143}, {106, 71}, {72, 113},
            {41, 96}, {115, 54}, {29, 136}, {83, 88}, {50, 109}, {124, 63}, {64, 121}, {18, 76}, {98, 99}, {56, 56},
            {36, 140}, {88, 82}, {67, 115}, {110, 69}, {42, 127}, {78, 91}, {26, 59}, {102, 103}, {60, 77}, {21, 133},
            {94, 94}, {48, 66}, {116, 51}, {33, 119}, {81, 87}, {52, 106}, {120, 73}, {39, 137}, {75, 98}, {104, 62}
        };
        for (uint8_t i = 0; i < NUM_RAINDROPS; i++) {
            raindrops[i].x = rain_positions[i][0];
            raindrops[i].y = rain_positions[i][1];
        }
        rain_initialized = true;
    }

    // NOTE: Raindrops are NOT drawn here - they're drawn after background is saved
    // to prevent them from being baked into the background image
}
