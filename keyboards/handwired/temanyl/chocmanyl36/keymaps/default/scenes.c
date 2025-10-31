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
#include <stdio.h>
#include <string.h>
#include "scenes.h"
#include "display.h"
#include "framebuffer.h"

// Rain animation state
bool rain_initialized = false;
bool rain_background_saved = false;
raindrop_t raindrops[NUM_RAINDROPS];
uint32_t rain_animation_timer = 0;

// Halloween animation state
ghost_t ghosts[NUM_GHOSTS];
bool ghost_initialized = false;
bool ghost_background_saved = false;
uint32_t ghost_animation_timer = 0;

// Christmas animation state
bool santa_initialized = false;
uint32_t santa_animation_timer = 0;
int16_t santa_x = -60;

// Advent calendar items with positions (day 1-24)
static const christmas_item_t advent_items[NUM_CHRISTMAS_ITEMS] = {
    // Days 1-8: Ground level decorations
    {XMAS_PRESENT_RED, 20, 145},        // Day 1
    {XMAS_PRESENT_GREEN, 40, 143},      // Day 2
    {XMAS_CANDY_CANE, 60, 140},         // Day 3
    {XMAS_PRESENT_BLUE, 80, 144},       // Day 4
    {XMAS_STOCKING, 100, 138},          // Day 5
    {XMAS_GINGERBREAD, 115, 142},       // Day 6
    {XMAS_SLEIGH_BELL, 12, 141},        // Day 7
    {XMAS_SNOWMAN_SMALL, 125, 140},     // Day 8

    // Days 9-16: Mid-level decorations
    {XMAS_ORNAMENT_RED, 25, 110},       // Day 9
    {XMAS_ORNAMENT_GOLD, 50, 105},      // Day 10
    {XMAS_ORNAMENT_BLUE, 75, 108},      // Day 11
    {XMAS_BELL, 95, 112},               // Day 12
    {XMAS_WREATH, 110, 100},            // Day 13
    {XMAS_TREE_SMALL, 15, 115},         // Day 14
    {XMAS_HOLLY, 120, 115},             // Day 15
    {XMAS_CANDLE, 42, 125},             // Day 16

    // Days 17-24: Upper decorations
    {XMAS_STAR_SMALL, 30, 70},          // Day 17
    {XMAS_SNOWFLAKE, 65, 75},           // Day 18
    {XMAS_ANGEL, 90, 65},               // Day 19
    {XMAS_STAR_SMALL, 115, 80},         // Day 20
    {XMAS_MISTLETOE, 48, 85},           // Day 21
    {XMAS_LIGHTS, 10, 95},              // Day 22
    {XMAS_NORTH_STAR, 67, 30},          // Day 23 - North Star high in sky
    {XMAS_HEART, 100, 55}               // Day 24
};

// Reset all scene animation states
void reset_scene_animations(void) {
    rain_initialized = false;
    rain_background_saved = false;
    ghost_initialized = false;
    ghost_background_saved = false;
}

// Get season based on month
uint8_t get_season(uint8_t month) {
    if (month == 12 || month <= 2) return 0; // Winter
    else if (month >= 3 && month <= 5) return 1; // Spring
    else if (month >= 6 && month <= 8) return 2; // Summer
    else return 3; // Fall
}

// Forward declaration from display.h
extern void get_layer_color(uint8_t layer, uint8_t *hue, uint8_t *sat, uint8_t *val);

// Draw a tree with seasonal variations
void draw_tree(uint16_t base_x, uint16_t base_y, uint8_t season, uint8_t hue, uint8_t sat, uint8_t val) {
    // Tree structure: trunk + canopy
    // Trunk (brown)
    uint8_t trunk_width = 6;
    uint8_t trunk_height = (season == 1) ? 28 : 22; // Spring trees are taller
    fb_rect_hsv(base_x - trunk_width/2, base_y - trunk_height,
            base_x + trunk_width/2, base_y, 20, 200, 100, true);

    // Canopy changes by season
    if (season == 0) { // Winter - bare branches with more detail
        // Draw main upward-reaching branches
        // Left upward branch
        fb_rect_hsv(base_x - 8, base_y - trunk_height - 10, base_x - 6, base_y - trunk_height - 2, 20, 150, 80, true);
        fb_rect_hsv(base_x - 12, base_y - trunk_height - 8, base_x - 8, base_y - trunk_height - 6, 20, 150, 80, true);
        // Right upward branch
        fb_rect_hsv(base_x + 6, base_y - trunk_height - 10, base_x + 8, base_y - trunk_height - 2, 20, 150, 80, true);
        fb_rect_hsv(base_x + 8, base_y - trunk_height - 8, base_x + 12, base_y - trunk_height - 6, 20, 150, 80, true);

        // Middle upward branches (from mid-trunk)
        fb_rect_hsv(base_x - 6, base_y - trunk_height - 6, base_x - 4, base_y - trunk_height + 2, 20, 150, 80, true);
        fb_rect_hsv(base_x + 4, base_y - trunk_height - 6, base_x + 6, base_y - trunk_height + 2, 20, 150, 80, true);

        // Outward angled branches (lower)
        fb_rect_hsv(base_x - 10, base_y - trunk_height + 4, base_x - 8, base_y - trunk_height + 8, 20, 150, 80, true);
        fb_rect_hsv(base_x + 8, base_y - trunk_height + 4, base_x + 10, base_y - trunk_height + 8, 20, 150, 80, true);

        // Smaller upward twigs
        fb_rect_hsv(base_x - 10, base_y - trunk_height - 12, base_x - 9, base_y - trunk_height - 9, 20, 120, 70, true);
        fb_rect_hsv(base_x + 9, base_y - trunk_height - 12, base_x + 10, base_y - trunk_height - 9, 20, 120, 70, true);
        fb_rect_hsv(base_x - 3, base_y - trunk_height - 13, base_x - 2, base_y - trunk_height - 10, 20, 120, 70, true);
        fb_rect_hsv(base_x + 2, base_y - trunk_height - 13, base_x + 3, base_y - trunk_height - 10, 20, 120, 70, true);

        // Side twigs extending from main branches
        fb_rect_hsv(base_x - 14, base_y - trunk_height - 6, base_x - 12, base_y - trunk_height - 4, 20, 120, 70, true);
        fb_rect_hsv(base_x + 12, base_y - trunk_height - 6, base_x + 14, base_y - trunk_height - 4, 20, 120, 70, true);

        // Add snow accumulation on branches (thicker and more coverage)
        // Snow on main upward branches (thicker patches)
        fb_rect_hsv(base_x - 9, base_y - trunk_height - 11, base_x - 5, base_y - trunk_height - 9, 170, 40, 255, true);
        fb_rect_hsv(base_x + 5, base_y - trunk_height - 11, base_x + 9, base_y - trunk_height - 9, 170, 40, 255, true);

        // Snow on horizontal/angled branch sections (larger)
        fb_rect_hsv(base_x - 13, base_y - trunk_height - 9, base_x - 7, base_y - trunk_height - 7, 170, 40, 255, true);
        fb_rect_hsv(base_x + 7, base_y - trunk_height - 9, base_x + 13, base_y - trunk_height - 7, 170, 40, 255, true);

        // Snow on middle branches (thicker)
        fb_rect_hsv(base_x - 7, base_y - trunk_height - 7, base_x - 3, base_y - trunk_height - 5, 170, 40, 255, true);
        fb_rect_hsv(base_x + 3, base_y - trunk_height - 7, base_x + 7, base_y - trunk_height - 5, 170, 40, 255, true);

        // Additional snow on mid-trunk branches
        fb_rect_hsv(base_x - 6, base_y - trunk_height - 3, base_x - 3, base_y - trunk_height - 1, 170, 40, 255, true);
        fb_rect_hsv(base_x + 3, base_y - trunk_height - 3, base_x + 6, base_y - trunk_height - 1, 170, 40, 255, true);

        // Snow on lower outward branches (larger)
        fb_rect_hsv(base_x - 11, base_y - trunk_height + 3, base_x - 7, base_y - trunk_height + 5, 170, 40, 255, true);
        fb_rect_hsv(base_x + 7, base_y - trunk_height + 3, base_x + 11, base_y - trunk_height + 5, 170, 40, 255, true);

        // Additional snow lower down
        fb_rect_hsv(base_x - 9, base_y - trunk_height + 6, base_x - 7, base_y - trunk_height + 8, 170, 40, 255, true);
        fb_rect_hsv(base_x + 7, base_y - trunk_height + 6, base_x + 9, base_y - trunk_height + 8, 170, 40, 255, true);

        // Snow patches on twigs (larger and brighter)
        fb_rect_hsv(base_x - 11, base_y - trunk_height - 13, base_x - 8, base_y - trunk_height - 11, 0, 0, 255, true);
        fb_rect_hsv(base_x + 8, base_y - trunk_height - 13, base_x + 11, base_y - trunk_height - 11, 0, 0, 255, true);
        fb_rect_hsv(base_x - 4, base_y - trunk_height - 14, base_x - 1, base_y - trunk_height - 12, 0, 0, 255, true);
        fb_rect_hsv(base_x + 1, base_y - trunk_height - 14, base_x + 4, base_y - trunk_height - 12, 0, 0, 255, true);

        // Side twig snow
        fb_rect_hsv(base_x - 15, base_y - trunk_height - 7, base_x - 11, base_y - trunk_height - 5, 170, 40, 255, true);
        fb_rect_hsv(base_x + 11, base_y - trunk_height - 7, base_x + 15, base_y - trunk_height - 5, 170, 40, 255, true);
    } else if (season == 1) { // Spring - green leaves with pink blossoms
        // Tree shape with green base
        fb_circle_hsv(base_x, base_y - trunk_height - 7, 15, 85, 220, 200, true); // Light green
        // Add leaf and blossom dots (smaller, mostly pink blossoms)
        for (uint8_t i = 0; i < 9; i++) {
            int8_t offset_x = (i % 3 - 1) * 7;
            int8_t offset_y = (i / 3 - 1) * 7;
            // Make 8 dots pink blossoms, only dot 4 (center) is green leaf
            if (i != 4) {
                fb_circle_hsv(base_x + offset_x, base_y - trunk_height - 7 + offset_y, 2, 234, 255, 220, true); // Pink blossom (smaller)
            } else {
                fb_circle_hsv(base_x + offset_x, base_y - trunk_height - 7 + offset_y, 2, 85, 255, 180, true); // Green leaf (smaller)
            }
        }
    } else if (season == 2) { // Summer - cherry tree with cherries
        // Dense green canopy
        fb_circle_hsv(base_x, base_y - trunk_height - 7, 16, 85, 255, 200, true);       // Center
        fb_circle_hsv(base_x - 9, base_y - trunk_height - 4, 11, 85, 255, 180, true);  // Left
        fb_circle_hsv(base_x + 9, base_y - trunk_height - 4, 11, 85, 255, 180, true);  // Right

        // Add red cherries scattered throughout the entire canopy
        // Cherry positions relative to canopy center at (base_x, base_y - trunk_height - 7)
        // Canopy extends from y=-16 (top) to y=+14 (bottom on sides)
        int8_t cherry_offsets[][2] = {
            // Top area (y: -16 to -9)
            {-4, -14}, {2, -13}, {-9, -11}, {6, -12}, {-1, -10},
            // Middle area (y: -8 to -1)
            {-12, -5}, {-6, -3}, {0, -4}, {8, -2}, {13, -6},
            // Lower area (y: 0 to +12)
            {-14, 3}, {-8, 8}, {-2, 10}, {4, 9}, {10, 6}, {15, 4}
        };

        for (uint8_t i = 0; i < 16; i++) {
            // Draw cherries (bright red, small circles)
            fb_circle_hsv(base_x + cherry_offsets[i][0],
                     base_y - trunk_height - 7 + cherry_offsets[i][1],
                     2, 0, 255, 220, true);
        }
    } else { // Fall - orange/red/yellow leaves
        // Tree shape with autumn colors
        fb_circle_hsv(base_x, base_y - trunk_height - 7, 15, 20, 255, 200, true);      // Orange
        fb_circle_hsv(base_x - 8, base_y - trunk_height - 4, 10, 10, 255, 220, true);  // Red-orange
        fb_circle_hsv(base_x + 8, base_y - trunk_height - 4, 10, 30, 255, 200, true);  // Yellow-orange
    }
}

// Draw a cabin with seasonal variations
void draw_cabin(uint16_t base_x, uint16_t base_y, uint8_t season) {
    // Cabin dimensions
    uint8_t cabin_width = 24;
    uint8_t cabin_height = 18;
    uint8_t roof_height = 10;

    // Main cabin body (brown wood)
    fb_rect_hsv(base_x - cabin_width/2, base_y - cabin_height,
            base_x + cabin_width/2, base_y, 20, 200, 120, true);

    // Roof (darker brown/grey triangular roof using rectangles)
    // Left side of roof
    for (uint8_t i = 0; i < roof_height; i++) {
        uint8_t roof_y = base_y - cabin_height - i;
        uint8_t roof_left = base_x - (cabin_width/2 + roof_height - i);
        uint8_t roof_right = base_x - (cabin_width/2 - i);
        fb_rect_hsv(roof_left, roof_y, roof_right, roof_y + 1, 15, 180, 80, true);
    }
    // Right side of roof
    for (uint8_t i = 0; i < roof_height; i++) {
        uint8_t roof_y = base_y - cabin_height - i;
        uint8_t roof_left = base_x + (cabin_width/2 - i);
        uint8_t roof_right = base_x + (cabin_width/2 + roof_height - i);
        fb_rect_hsv(roof_left, roof_y, roof_right, roof_y + 1, 15, 180, 80, true);
    }
    // Fill the peak gap with a center line
    fb_rect_hsv(base_x - 7, base_y - cabin_height - roof_height,
            base_x + 7, base_y - cabin_height, 15, 180, 80, true);

    // Door (darker brown)
    uint8_t door_width = 7;
    uint8_t door_height = 10;
    fb_rect_hsv(base_x - door_width/2, base_y - door_height,
            base_x + door_width/2, base_y, 15, 220, 60, true);

    // Window (light yellow - lit window)
    uint8_t window_size = 6;
    fb_rect_hsv(base_x + 5, base_y - cabin_height + 5,
            base_x + 5 + window_size, base_y - cabin_height + 5 + window_size, 42, 150, 255, true);

    // Window frame cross (dark brown)
    fb_rect_hsv(base_x + 7, base_y - cabin_height + 5,
            base_x + 8, base_y - cabin_height + 5 + window_size, 20, 200, 80, true);
    fb_rect_hsv(base_x + 5, base_y - cabin_height + 8,
            base_x + 5 + window_size, base_y - cabin_height + 9, 20, 200, 80, true);

    // Chimney on roof (brick red/brown)
    uint8_t chimney_width = 4;
    uint8_t chimney_height = 8;
    fb_rect_hsv(base_x + 5, base_y - cabin_height - roof_height - chimney_height + 2,
            base_x + 5 + chimney_width, base_y - cabin_height - roof_height + 3, 10, 200, 100, true);

    // Smoke from chimney (light grey puffs) - only if not summer
    if (season != 2) {
        fb_circle_hsv(base_x + 6, base_y - cabin_height - roof_height - chimney_height - 2, 2, 0, 0, 180, true);
        fb_circle_hsv(base_x + 7, base_y - cabin_height - roof_height - chimney_height - 5, 2, 0, 0, 160, true);
        fb_circle_hsv(base_x + 8, base_y - cabin_height - roof_height - chimney_height - 8, 2, 0, 0, 140, true);
    }

    // Add snow on roof in winter
    if (season == 0) {
        // Snow on left side of roof
        for (uint8_t i = 0; i < roof_height; i++) {
            uint8_t roof_y = base_y - cabin_height - i;
            uint8_t roof_left = base_x - (cabin_width/2 + roof_height - i);
            uint8_t roof_right = base_x - (cabin_width/2 - i);
            fb_rect_hsv(roof_left, roof_y - 2, roof_right, roof_y - 1, 170, 40, 255, true);
        }
        // Snow on right side of roof
        for (uint8_t i = 0; i < roof_height; i++) {
            uint8_t roof_y = base_y - cabin_height - i;
            uint8_t roof_left = base_x + (cabin_width/2 - i);
            uint8_t roof_right = base_x + (cabin_width/2 + roof_height - i);
            fb_rect_hsv(roof_left, roof_y - 2, roof_right, roof_y - 1, 170, 40, 255, true);
        }
    }
}

// Get celestial object (sun/moon) position based on time
void get_celestial_position(uint8_t hour, uint16_t *x, uint16_t *y) {
    // Sun/moon moves across sky throughout the day
    // Hour 0-23: position from left to right
    // Peak at noon (y lowest), near horizon at dawn/dusk (y highest)

    // X position: moves from left to right across the sky
    // Map hour 0-23 to x position
    if (hour >= 6 && hour <= 19) {
        // Daytime: sun moves from left to right
        // hour 6 -> x=20, hour 12 -> x=67, hour 19 -> x=114
        *x = 20 + ((hour - 6) * 94) / 13;

        // Y position: arc across sky (lowest at noon)
        // hour 6 -> y=40, hour 12 -> y=20, hour 19 -> y=40
        int16_t time_from_noon = hour - 12;
        *y = 20 + (time_from_noon * time_from_noon) / 2;
    } else {
        // Nighttime: moon moves from left to right (same direction as sun)
        // Night: hour 20-23 (evening) and 0-5 (early morning)
        // Map to continuous night progression: 20->0, 21->1, 22->2, 23->3, 0->4, 1->5, ..., 5->9
        uint8_t night_hour;
        if (hour >= 20) {
            night_hour = hour - 20;  // 20->0, 21->1, 22->2, 23->3
        } else {
            night_hour = hour + 4;    // 0->4, 1->5, 2->6, 3->7, 4->8, 5->9
        }
        // night_hour 0-9: position from left to right (like sun)
        // Starts at x=20 (hour 20) and ends at x=114 (hour 6)
        *x = 20 + (night_hour * 94) / 9;

        // Y position: arc across sky (lowest at midnight, which is night_hour=4)
        int16_t time_from_midnight = night_hour - 4;
        *y = 25 + (time_from_midnight * time_from_midnight) / 2;
    }

    // Clamp values
    if (*x < 15) *x = 15;
    if (*x > 120) *x = 120;
    if (*y < 15) *y = 15;
    if (*y > 50) *y = 50;
}

// Main seasonal animation drawing function
void draw_seasonal_animation(void) {
    // Animation area: entire upper portion from top to date area
    // Logo area: y=10 to y=130 (120x120 logo)
    // Animation extends from y=0 to y=152 (above date which starts at y=155)

    // Determine season based on month
    uint8_t season = get_season(current_month);

    // Determine time of day based on hour (0-23)
    bool is_night = (current_hour >= 20 || current_hour < 6);

    // Get sun/moon position based on time
    uint16_t celestial_x, celestial_y;
    get_celestial_position(current_hour, &celestial_x, &celestial_y);

    // === SKY AND CELESTIAL OBJECTS ===

    // Draw sun or moon with appropriate coloring based on time
    if (is_night) {
        // Draw moon with phase based on day of month (waxing/waning cycle)
        uint8_t moon_day;
        if (current_hour < 6) {
            // Early morning: this is the end of yesterday's night
            moon_day = (current_day > 1) ? (current_day - 1) : 31;
        } else {
            // Evening (hours 20-23): this is the start of today's night
            moon_day = current_day;
        }
        uint8_t moon_phase = (moon_day * 29) / 31; // Map day 1-31 to phase 0-29

        // Draw full moon circle first (pale yellow/white base)
        fb_circle_hsv(celestial_x, celestial_y, 8, 42, 100, 255, true);

        // Add shadow to create moon phase effect
        if (moon_phase < 14) {
            // Waxing moon (new -> full): shadow on left side, shrinking
            if (moon_phase < 7) {
                // New moon to first quarter: shadow covers most/half of left side
                int8_t shadow_offset = -8 + (moon_phase * 2); // -8 to 6
                uint8_t shadow_radius = 8 - (moon_phase / 2); // 8 to 4
                fb_circle_hsv(celestial_x + shadow_offset, celestial_y, shadow_radius, 0, 0, 20, true);
            } else {
                // First quarter to full: small shadow on left, disappearing
                int8_t shadow_offset = 6 - ((moon_phase - 7) * 2); // 6 to -8
                uint8_t shadow_radius = 5 - ((moon_phase - 7) / 2); // 4 to 0
                if (shadow_radius > 0) {
                    fb_circle_hsv(celestial_x + shadow_offset, celestial_y, shadow_radius, 0, 0, 20, true);
                }
            }
        } else if (moon_phase > 14) {
            // Waning moon (full -> new): shadow on right side, growing
            uint8_t waning_phase = moon_phase - 15; // 0 to 14
            if (waning_phase < 7) {
                // Full to last quarter: small shadow on right, growing
                int8_t shadow_offset = -6 + (waning_phase * 2); // -6 to 8
                uint8_t shadow_radius = (waning_phase / 2); // 0 to 3
                if (shadow_radius > 0) {
                    fb_circle_hsv(celestial_x + shadow_offset, celestial_y, shadow_radius, 0, 0, 20, true);
                }
            } else {
                // Last quarter to new: shadow covers half/most of right side
                int8_t shadow_offset = 8 - ((waning_phase - 7) * 2); // 8 to -6
                uint8_t shadow_radius = 5 + ((waning_phase - 7) / 2); // 4 to 7
                fb_circle_hsv(celestial_x + shadow_offset, celestial_y, shadow_radius, 0, 0, 20, true);
            }
        }

        // Add stars scattered across the night sky
        uint16_t star_positions[][2] = {
            {20, 15}, {50, 25}, {90, 18}, {110, 30},
            {65, 12}, {100, 22}, {80, 30},
            {120, 15}, {10, 25}, {28, 20},
            {85, 8}, {70, 25}, {60, 15}
        };
        for (uint8_t i = 0; i < 13; i++) {
            fb_rect_hsv(star_positions[i][0], star_positions[i][1],
                    star_positions[i][0] + 2, star_positions[i][1] + 2, 42, 50, 255, true);
        }
    } else {
        // Draw sun with color based on time of day
        uint8_t sun_hue, sun_sat;
        if (current_hour < 8 || current_hour > 17) {
            // Dawn/dusk - orange/red sun
            sun_hue = 10;
            sun_sat = 255;
        } else {
            // Midday - bright yellow sun
            sun_hue = 42;
            sun_sat = 255;
        }

        // Draw sun with rays
        fb_circle_hsv(celestial_x, celestial_y, 9, sun_hue, sun_sat, 255, true);

        // Add sun rays (8 rays around sun)
        for (uint8_t i = 0; i < 8; i++) {
            int16_t ray_x = 0, ray_y = 0;

            if (i == 0) { ray_x = 12; ray_y = 0; }
            else if (i == 1) { ray_x = 9; ray_y = -9; }
            else if (i == 2) { ray_x = 0; ray_y = -12; }
            else if (i == 3) { ray_x = -9; ray_y = -9; }
            else if (i == 4) { ray_x = -12; ray_y = 0; }
            else if (i == 5) { ray_x = -9; ray_y = 9; }
            else if (i == 6) { ray_x = 0; ray_y = 12; }
            else if (i == 7) { ray_x = 9; ray_y = 9; }

            fb_rect_hsv(celestial_x + ray_x - 1, celestial_y + ray_y - 1,
                    celestial_x + ray_x + 1, celestial_y + ray_y + 1, sun_hue, sun_sat, 200, true);
        }
    }

    // === GROUND AND TREES ===

    // Draw ground line
    uint16_t ground_y = 150;
    fb_rect_hsv(0, ground_y, 134, ground_y + 1, 85, 180, 100, true);

    // Draw trees at different positions
    uint8_t layer = get_highest_layer(layer_state);
    uint8_t tree_hue, tree_sat, tree_val;
    get_layer_color(layer, &tree_hue, &tree_sat, &tree_val);

    draw_tree(30, ground_y, season, tree_hue, tree_sat, tree_val);
    draw_tree(67, ground_y, season, tree_hue, tree_sat, tree_val);
    draw_cabin(105, ground_y, season);

    // === SEASONAL WEATHER EFFECTS ===

    if (season == 0) { // Winter
        // Winter clouds, snowflakes, and snow on ground
        uint16_t cloud_positions[][2] = {{25, 35}, {85, 40}, {110, 30}};
        for (uint8_t i = 0; i < 3; i++) {
            fb_circle_hsv(cloud_positions[i][0], cloud_positions[i][1], 8, 0, 0, 160, true);
            fb_circle_hsv(cloud_positions[i][0] + 9, cloud_positions[i][1] + 2, 6, 0, 0, 160, true);
            fb_circle_hsv(cloud_positions[i][0] - 7, cloud_positions[i][1] + 2, 6, 0, 0, 160, true);
            fb_circle_hsv(cloud_positions[i][0] + 4, cloud_positions[i][1] - 3, 5, 0, 0, 150, true);
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
        fb_rect_hsv(0, ground_y - 2, 134, ground_y, 0, 0, 240, true);
        struct { uint16_t x; uint8_t height; } snow_drifts[] = {
            {0, 2}, {20, 4}, {45, 3}, {70, 5}, {95, 3}, {115, 4}
        };
        for (uint8_t i = 0; i < 6; i++) {
            fb_rect_hsv(snow_drifts[i].x, ground_y - snow_drifts[i].height, snow_drifts[i].x + 20, ground_y, 170, 40, 255, true);
        }
    } else if (season == 1) { // Spring - butterflies, flowers, and birds
        // Draw birds in the sky (larger V shapes) - 6 birds
        uint16_t bird_positions[][2] = {
            {25, 50}, {60, 40}, {90, 70}, {110, 45}, {40, 75}, {150, 65}
        };
        for (uint8_t i = 0; i < 6; i++) {
            // Left wing (larger)
            fb_rect_hsv(bird_positions[i][0] - 5, bird_positions[i][1], bird_positions[i][0] - 1, bird_positions[i][1] - 3, 0, 0, 100, true);
            // Right wing (larger)
            fb_rect_hsv(bird_positions[i][0] + 1, bird_positions[i][1] - 3, bird_positions[i][0] + 5, bird_positions[i][1], 0, 0, 100, true);
        }

        // More butterflies, lower to the ground
        struct { uint16_t x; uint16_t y; uint8_t hue; } butterflies[] = {
            {20, 115, 234}, {45, 125, 170}, {65, 120, 42}, {85, 130, 200}, {105, 118, 10},
            {125, 135, 234}, {35, 128, 85}, {75, 122, 42}, {95, 133, 170}
        };
        for (uint8_t i = 0; i < 9; i++) {
            fb_circle_hsv(butterflies[i].x - 2, butterflies[i].y, 2, butterflies[i].hue, 255, 200, true);
            fb_circle_hsv(butterflies[i].x + 2, butterflies[i].y, 2, butterflies[i].hue, 255, 200, true);
        }

        // Draw flowers on the ground (various colors and sizes)
        struct { uint16_t x; uint8_t hue; uint8_t size; uint8_t stem_height; } flowers[] = {
            {15, 234, 3, 5}, {28, 0, 4, 6}, {42, 42, 3, 5}, {58, 170, 5, 7}, {72, 200, 3, 5},
            {88, 10, 4, 6}, {102, 85, 3, 5}, {118, 234, 5, 7}, {25, 42, 4, 6}, {50, 200, 3, 5},
            {80, 0, 5, 7}, {95, 170, 3, 5}, {110, 234, 4, 6}, {35, 10, 5, 7}, {65, 42, 3, 5}
        };
        for (uint8_t i = 0; i < 15; i++) {
            // Flower stem (green)
            fb_rect_hsv(flowers[i].x, ground_y - flowers[i].stem_height, flowers[i].x + 1, ground_y, 85, 200, 150, true);
            // Flower petals (colorful circles with varying sizes)
            fb_circle_hsv(flowers[i].x, ground_y - flowers[i].stem_height - 2, flowers[i].size, flowers[i].hue, 255, 220, true);
            // Flower center (yellow, size varies with flower)
            fb_circle_hsv(flowers[i].x, ground_y - flowers[i].stem_height - 2, flowers[i].size / 3, 42, 255, 255, true);
        }
    } else if (season == 2) { // Summer - airplane and sunflowers
        // Draw airplane in top left (more realistic side view)
        uint16_t plane_x = 15;
        uint16_t plane_y = 25;

        // Main fuselage (body) - tapered at nose
        fb_rect_hsv(plane_x + 3, plane_y + 1, plane_x + 25, plane_y + 4, 0, 0, 180, true);

        // Nose (pointed front)
        fb_rect_hsv(plane_x + 1, plane_y + 2, plane_x + 3, plane_y + 3, 0, 0, 180, true);

        // Cockpit windows (series of light windows)
        fb_rect_hsv(plane_x + 20, plane_y + 2, plane_x + 22, plane_y + 3, 170, 80, 240, true);
        fb_rect_hsv(plane_x + 17, plane_y + 2, plane_x + 19, plane_y + 3, 170, 80, 240, true);

        // Main wings (swept back)
        fb_rect_hsv(plane_x + 10, plane_y - 3, plane_x + 18, plane_y + 1, 0, 0, 180, true);
        fb_rect_hsv(plane_x + 10, plane_y + 4, plane_x + 18, plane_y + 8, 0, 0, 180, true);

        // Tail section
        // Vertical stabilizer (tail fin)
        fb_rect_hsv(plane_x + 3, plane_y - 3, plane_x + 6, plane_y + 1, 0, 0, 180, true);
        // Horizontal stabilizer
        fb_rect_hsv(plane_x + 3, plane_y + 4, plane_x + 8, plane_y + 6, 0, 0, 180, true);

        // Engine under wing (optional detail)
        fb_circle_hsv(plane_x + 13, plane_y + 7, 2, 0, 0, 160, true);

        // Draw sunflowers on the ground (tall with large yellow heads)
        struct { uint16_t x; uint8_t stem_height; } sunflowers[] = {
            {22, 13}, {52, 15}, {78, 14}, {102, 12}, {122, 14}
        };
        for (uint8_t i = 0; i < 5; i++) {
            // Sunflower stem (green)
            fb_rect_hsv(sunflowers[i].x, ground_y - sunflowers[i].stem_height, sunflowers[i].x + 2, ground_y, 85, 200, 150, true);

            // Large sunflower head (bright yellow)
            fb_circle_hsv(sunflowers[i].x + 1, ground_y - sunflowers[i].stem_height - 3, 5, 42, 255, 255, true);

            // Dark center (brown)
            fb_circle_hsv(sunflowers[i].x + 1, ground_y - sunflowers[i].stem_height - 3, 2, 20, 200, 100, true);
        }
    } else { // Fall - rain and clouds
        // Draw rain clouds (darker gray clouds)
        uint16_t cloud_positions[][2] = {
            {25, 30}, {70, 40}, {120, 35}
        };
        for (uint8_t i = 0; i < 3; i++) {
            // Main cloud body (dark gray)
            fb_circle_hsv(cloud_positions[i][0], cloud_positions[i][1], 9, 0, 0, 120, true);
            fb_circle_hsv(cloud_positions[i][0] + 10, cloud_positions[i][1] + 2, 7, 0, 0, 120, true);
            fb_circle_hsv(cloud_positions[i][0] - 8, cloud_positions[i][1] + 2, 7, 0, 0, 120, true);
            fb_circle_hsv(cloud_positions[i][0] + 5, cloud_positions[i][1] - 4, 6, 0, 0, 110, true);
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
    }

    // === HALLOWEEN/CHRISTMAS OVERLAYS ===
    if (is_halloween_event()) {
        draw_halloween_elements();
    }

    if (is_christmas_season()) {
        draw_christmas_scene();
    }

    // === SAVE BACKGROUND FOR ANIMATIONS ===
    bool need_background = (season == 3) || is_halloween_event();
    if (need_background && !rain_background_saved && !ghost_background_saved) {
        fb_save_to_background();
        rain_background_saved = true;
        ghost_background_saved = true;
    }

    // === DRAW RAINDROPS (if fall season) ===
    if (season == 3) {
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

        // Draw raindrops
        for (uint8_t i = 0; i < NUM_RAINDROPS; i++) {
            if (raindrops[i].y >= 0 && raindrops[i].y < 150) {
                fb_rect_hsv(raindrops[i].x, raindrops[i].y,
                           raindrops[i].x + RAINDROP_WIDTH - 1,
                           raindrops[i].y + RAINDROP_HEIGHT - 1,
                           170, 150, 200, true);
            }
        }
    } else {
        if (rain_initialized) {
            rain_initialized = false;
            rain_background_saved = false;
        }
    }

    // === DRAW GHOSTS (if Halloween) ===
    if (is_halloween_event()) {
        if (!ghost_initialized) {
            init_ghosts();
        }

        for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
            draw_ghost(ghosts[i].x, ghosts[i].y);
        }
    } else {
        if (ghost_initialized) {
            ghost_initialized = false;
            ghost_background_saved = false;
        }
    }
}

// Halloween event check
bool is_halloween_event(void) {
    return (current_month == 10 && current_day >= 28) ||
           (current_month == 11 && current_day <= 3);
}

// Draw pumpkin
void draw_pumpkin(int16_t x, int16_t y, uint8_t size) {
    if (x < -size || x > 135 + size || y < -size || y > 152 + size) return;

    fb_circle_hsv(x, y, size, 20, 255, 255, true);
    fb_circle_hsv(x, y + size/3, size - 2, 16, 255, 220, true);

    uint8_t eye_offset = size / 3;
    uint8_t eye_size = size / 4;

    fb_rect_hsv(x - eye_offset - eye_size, y - eye_offset,
            x - eye_offset + eye_size, y - eye_offset + eye_size, 0, 0, 0, true);
    fb_rect_hsv(x + eye_offset - eye_size, y - eye_offset,
            x + eye_offset + eye_size, y - eye_offset + eye_size, 0, 0, 0, true);
    fb_rect_hsv(x - eye_size/2, y, x + eye_size/2, y + eye_size, 0, 0, 0, true);
    fb_rect_hsv(x - size/2, y + size/3, x + size/2, y + size/2, 0, 0, 0, true);

    for (int8_t i = -size/3; i < size/3; i += size/4) {
        fb_rect_hsv(x + i, y + size/3, x + i + size/6, y + size/2 - 1, 20, 255, 255, true);
    }

    fb_rect_hsv(x - 2, y - size - 3, x + 2, y - size + 1, 85, 200, 100, true);
}

// Draw ghost
void draw_ghost(int16_t x, int16_t y) {
    if (x < -15 || x > 150 || y < -20 || y > 172) return;

    fb_circle_hsv(x, y, 7, 0, 0, 240, true);
    fb_rect_hsv(x - 7, y, x + 7, y + 12, 0, 0, 240, true);
    fb_rect_hsv(x - 7, y + 10, x - 4, y + 13, 0, 0, 240, true);
    fb_rect_hsv(x - 3, y + 10, x + 0, y + 12, 0, 0, 240, true);
    fb_rect_hsv(x + 1, y + 10, x + 4, y + 13, 0, 0, 240, true);
    fb_rect_hsv(x + 5, y + 10, x + 7, y + 12, 0, 0, 240, true);
    fb_rect_hsv(x - 3, y - 2, x - 1, y, 0, 0, 0, true);
    fb_rect_hsv(x + 1, y - 2, x + 3, y, 0, 0, 0, true);
    fb_circle_hsv(x, y + 3, 2, 0, 0, 0, false);
}

// Draw Halloween elements
void draw_halloween_elements(void) {
    draw_pumpkin(25, 145, 8);
    draw_pumpkin(55, 143, 10);
    draw_pumpkin(90, 144, 9);
}

// Initialize ghosts
void init_ghosts(void) {
    if (ghost_initialized) return;

    ghosts[0].x = 20; ghosts[0].y = 90; ghosts[0].vx = 1; ghosts[0].vy = 0; ghosts[0].phase = 0;
    ghosts[1].x = 60; ghosts[1].y = 50; ghosts[1].vx = -1; ghosts[1].vy = 0; ghosts[1].phase = 40;
    ghosts[2].x = 100; ghosts[2].y = 70; ghosts[2].vx = 1; ghosts[2].vy = 0; ghosts[2].phase = 80;

    ghost_initialized = true;
}

// Check if pixel is in ghost
bool is_pixel_in_ghost(int16_t px, int16_t py, uint8_t ghost_idx) {
    if (ghost_idx >= NUM_GHOSTS) return false;
    int16_t gx = ghosts[ghost_idx].x;
    int16_t gy = ghosts[ghost_idx].y;
    return (px >= gx - 7 && px <= gx + 7 && py >= gy - 7 && py <= gy + 13);
}

// Redraw ghosts in region
void redraw_ghosts_in_region(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if (!ghost_initialized) return;

    for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
        int16_t gx = ghosts[i].x;
        int16_t gy = ghosts[i].y;
        int16_t ghost_x1 = gx - 7, ghost_y1 = gy - 7;
        int16_t ghost_x2 = gx + 7, ghost_y2 = gy + 13;

        if (ghost_x2 >= x1 && ghost_x1 <= x2 && ghost_y2 >= y1 && ghost_y1 <= y2) {
            draw_ghost(gx, gy);
        }
    }
}

// Animate ghosts
void animate_ghosts(void) {
    if (!ghost_initialized || !ghost_background_saved) {
        return;
    }

    // Only animate during Halloween event
    if (!is_halloween_event()) {
        return;
    }

    // Animate each ghost
    for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
        // Store old position
        int16_t old_x = ghosts[i].x;
        int16_t old_y = ghosts[i].y;

        // Calculate bounding box for old position
        int16_t old_x1 = old_x - 8;  // Ghost extends 7 pixels left, plus 1 margin
        int16_t old_y1 = old_y - 8;  // Ghost extends 7 pixels up, plus 1 margin
        int16_t old_x2 = old_x + 8;  // Ghost extends 7 pixels right, plus 1 margin
        int16_t old_y2 = old_y + 14; // Ghost extends to y+13, plus 1 margin

        // Restore old ghost position from background
        // BUT: need to check if other ghosts or raindrops overlap this area
        for (int16_t py = old_y1; py <= old_y2; py++) {
            for (int16_t px = old_x1; px <= old_x2; px++) {
                if (px < 0 || px >= FB_WIDTH || py < 0 || py >= FB_SPLIT_Y) continue;

                // Check if this pixel is covered by another ghost
                bool covered_by_other_ghost = false;
                for (uint8_t j = 0; j < NUM_GHOSTS; j++) {
                    if (j != i && is_pixel_in_ghost(px, py, j)) {
                        covered_by_other_ghost = true;
                        break;
                    }
                }

                if (!covered_by_other_ghost) {
                    // Restore from background
                    uint16_t bg_color = fb_background.pixels[py][px];
                    fb.pixels[py][px] = bg_color;
                }
            }
        }

        // Flush the old ghost region to erase it from display
        fb_flush_region(display, old_x1, old_y1, old_x2, old_y2);

        // Update ghost position with floating motion
        // Horizontal movement
        ghosts[i].x += ghosts[i].vx;

        // Floating motion (sine wave approximation)
        ghosts[i].phase = (ghosts[i].phase + 1) % 160;  // Full cycle every 160 frames

        // Different base heights and oscillation amplitudes for each ghost
        int16_t base_y, amplitude;
        switch(i) {
            case 0:
                base_y = 90;
                amplitude = 8;
                break;
            case 1:
                base_y = 50;
                amplitude = 6;
                break;
            case 2:
            default:
                base_y = 70;
                amplitude = 10;
                break;
        }

        // Smooth sine wave approximation using piecewise linear interpolation
        int16_t offset;
        uint8_t quarter = ghosts[i].phase / 40;
        uint8_t phase_in_quarter = ghosts[i].phase % 40;

        switch(quarter) {
            case 0: // Rising: 0 to +amplitude
                offset = (amplitude * phase_in_quarter) / 40;
                break;
            case 1: // Peak to middle: +amplitude to 0
                offset = amplitude - (amplitude * phase_in_quarter) / 40;
                break;
            case 2: // Falling: 0 to -amplitude
                offset = -(amplitude * phase_in_quarter) / 40;
                break;
            case 3: // Trough to middle: -amplitude to 0
            default:
                offset = -amplitude + (amplitude * phase_in_quarter) / 40;
                break;
        }

        ghosts[i].y = base_y + offset;

        // Bounce off screen edges
        if (ghosts[i].x <= 8 || ghosts[i].x >= FB_WIDTH - 8) {
            ghosts[i].vx = -ghosts[i].vx;
        }

        // Draw ghost at new position
        draw_ghost(ghosts[i].x, ghosts[i].y);

        // Calculate bounding box for new position
        int16_t new_x1 = ghosts[i].x - 8;
        int16_t new_y1 = ghosts[i].y - 8;
        int16_t new_x2 = ghosts[i].x + 8;
        int16_t new_y2 = ghosts[i].y + 14;

        // Flush the new ghost region to draw it on display
        fb_flush_region(display, new_x1, new_y1, new_x2, new_y2);
    }
}

// Animate raindrops
void animate_raindrops(void) {
    if (!rain_initialized || !rain_background_saved) {
        return;
    }

    // Get current season
    uint8_t season = get_season(current_month);

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

// Christmas season check
bool is_christmas_season(void) {
    return current_month == 12;
}

// Get number of Christmas items to show
uint8_t get_christmas_items_to_show(void) {
    if (!is_christmas_season()) return 0;
    if (current_day >= 25) return NUM_CHRISTMAS_ITEMS;
    if (current_day >= 1 && current_day <= 24) return current_day;
    return 0;
}

// Draw Christmas item
void draw_christmas_item(christmas_item_type_t type, int16_t x, int16_t y) {
    switch (type) {
        case XMAS_PRESENT_RED:
        case XMAS_PRESENT_GREEN:
        case XMAS_PRESENT_BLUE: {
            uint8_t hue = (type == XMAS_PRESENT_RED) ? 0 : (type == XMAS_PRESENT_GREEN) ? 85 : 170;
            fb_rect_hsv(x - 4, y - 4, x + 4, y + 4, hue, 255, 200, true);
            fb_rect_hsv(x - 4, y - 1, x + 4, y + 1, 42, 200, 255, true);
            fb_rect_hsv(x - 1, y - 4, x + 1, y + 4, 42, 200, 255, true);
            fb_rect_hsv(x - 2, y - 6, x + 2, y - 4, 42, 200, 255, true);
            break;
        }
        case XMAS_CANDY_CANE: {
            fb_rect_hsv(x, y - 8, x + 2, y, 0, 255, 255, true);
            fb_rect_hsv(x, y - 11, x + 5, y - 9, 0, 255, 255, true);
            fb_rect_hsv(x, y - 6, x + 2, y - 4, 0, 0, 255, true);
            fb_rect_hsv(x, y - 2, x + 2, y, 0, 0, 255, true);
            fb_rect_hsv(x + 3, y - 11, x + 5, y - 10, 0, 0, 255, true);
            break;
        }
        case XMAS_STOCKING: {
            fb_rect_hsv(x - 3, y - 8, x + 2, y - 2, 0, 255, 220, true);
            fb_rect_hsv(x - 2, y - 2, x + 4, y, 0, 255, 220, true);
            fb_rect_hsv(x - 3, y - 9, x + 2, y - 8, 0, 0, 255, true);
            break;
        }
        case XMAS_ORNAMENT_RED:
        case XMAS_ORNAMENT_GOLD:
        case XMAS_ORNAMENT_BLUE: {
            uint8_t hue = (type == XMAS_ORNAMENT_RED) ? 0 : (type == XMAS_ORNAMENT_GOLD) ? 42 : 170;
            fb_circle_hsv(x, y, 4, hue, 255, 255, true);
            fb_rect_hsv(x - 1, y - 5, x + 1, y - 4, 0, 0, 180, true);
            break;
        }
        case XMAS_BELL: {
            fb_rect_hsv(x - 3, y - 2, x + 3, y + 2, 42, 255, 255, true);
            fb_rect_hsv(x - 4, y - 3, x + 4, y - 2, 42, 255, 255, true);
            fb_circle_hsv(x, y + 3, 1, 42, 255, 200, true);
            break;
        }
        case XMAS_HOLLY: {
            fb_rect_hsv(x - 4, y - 1, x + 4, y + 1, 85, 255, 180, true);
            fb_circle_hsv(x - 3, y - 2, 1, 0, 255, 255, true);
            fb_circle_hsv(x + 3, y - 2, 1, 0, 255, 255, true);
            break;
        }
        case XMAS_STAR_SMALL: {
            fb_rect_hsv(x - 1, y - 3, x + 1, y + 3, 42, 255, 255, true);
            fb_rect_hsv(x - 3, y - 1, x + 3, y + 1, 42, 255, 255, true);
            fb_rect_hsv(x - 2, y - 2, x + 2, y + 2, 42, 255, 255, true);
            break;
        }
        case XMAS_SNOWFLAKE: {
            fb_rect_hsv(x, y - 4, x, y + 4, 170, 100, 255, true);
            fb_rect_hsv(x - 4, y, x + 4, y, 170, 100, 255, true);
            fb_rect_hsv(x - 3, y - 3, x + 3, y + 3, 170, 100, 255, true);
            fb_rect_hsv(x - 3, y + 3, x + 3, y - 3, 170, 100, 255, true);
            break;
        }
        case XMAS_CANDLE: {
            fb_rect_hsv(x - 2, y - 8, x + 2, y, 0, 255, 200, true);
            fb_rect_hsv(x - 1, y - 11, x + 1, y - 8, 42, 255, 255, true);
            break;
        }
        case XMAS_TREE_SMALL: {
            fb_rect_hsv(x - 1, y - 2, x + 1, y, 20, 200, 120, true);
            fb_circle_hsv(x, y - 5, 4, 85, 255, 180, true);
            fb_circle_hsv(x - 2, y - 4, 1, 0, 255, 255, true);
            fb_circle_hsv(x + 2, y - 6, 1, 42, 255, 255, true);
            break;
        }
        case XMAS_GINGERBREAD: {
            fb_circle_hsv(x, y - 6, 2, 20, 200, 150, true);
            fb_rect_hsv(x - 2, y - 4, x + 2, y + 2, 20, 200, 150, true);
            fb_rect_hsv(x - 4, y - 2, x - 2, y, 20, 200, 150, true);
            fb_rect_hsv(x + 2, y - 2, x + 4, y, 20, 200, 150, true);
            fb_rect_hsv(x - 2, y + 2, x, y + 4, 20, 200, 150, true);
            fb_rect_hsv(x, y + 2, x + 2, y + 4, 20, 200, 150, true);
            break;
        }
        case XMAS_WREATH: {
            fb_circle_hsv(x, y, 5, 85, 255, 180, false);
            fb_circle_hsv(x, y, 4, 85, 255, 180, false);
            fb_rect_hsv(x - 2, y + 5, x + 2, y + 7, 0, 255, 255, true);
            break;
        }
        case XMAS_ANGEL: {
            fb_circle_hsv(x, y - 5, 2, 42, 100, 255, true);
            fb_circle_hsv(x, y - 2, 2, 0, 0, 240, true);
            fb_rect_hsv(x - 3, y, x + 3, y + 4, 0, 0, 240, true);
            fb_rect_hsv(x - 5, y + 1, x - 3, y + 3, 0, 0, 220, true);
            fb_rect_hsv(x + 3, y + 1, x + 5, y + 3, 0, 0, 220, true);
            break;
        }
        case XMAS_REINDEER_SMALL: {
            fb_circle_hsv(x, y, 2, 20, 200, 150, true);
            fb_circle_hsv(x + 2, y - 2, 1, 20, 200, 150, true);
            fb_rect_hsv(x + 1, y - 4, x + 2, y - 3, 20, 200, 120, true);
            fb_rect_hsv(x + 3, y - 1, x + 3, y, 0, 255, 255, true);
            break;
        }
        case XMAS_SNOWMAN_SMALL: {
            fb_circle_hsv(x, y - 5, 2, 0, 0, 240, true);
            fb_circle_hsv(x, y - 1, 3, 0, 0, 240, true);
            fb_rect_hsv(x - 1, y - 5, x + 1, y - 5, 0, 0, 0, true);
            fb_rect_hsv(x - 3, y - 6, x + 3, y - 6, 20, 200, 100, true);
            break;
        }
        case XMAS_LIGHTS: {
            fb_rect_hsv(x, y, x + 15, y, 0, 0, 100, true);
            fb_circle_hsv(x + 2, y + 1, 1, 0, 255, 255, true);
            fb_circle_hsv(x + 6, y + 1, 1, 85, 255, 255, true);
            fb_circle_hsv(x + 10, y + 1, 1, 170, 255, 255, true);
            fb_circle_hsv(x + 14, y + 1, 1, 42, 255, 255, true);
            break;
        }
        case XMAS_MISTLETOE: {
            fb_circle_hsv(x, y, 3, 85, 200, 150, true);
            fb_circle_hsv(x - 2, y - 1, 1, 0, 0, 255, true);
            fb_circle_hsv(x + 2, y - 1, 1, 0, 0, 255, true);
            break;
        }
        case XMAS_NORTH_STAR: {
            fb_rect_hsv(x - 1, y - 5, x + 1, y + 5, 42, 255, 255, true);
            fb_rect_hsv(x - 5, y - 1, x + 5, y + 1, 42, 255, 255, true);
            fb_rect_hsv(x - 3, y - 3, x + 3, y + 3, 42, 255, 255, true);
            fb_rect_hsv(x - 3, y + 3, x + 3, y - 3, 42, 255, 255, true);
            fb_circle_hsv(x, y, 6, 42, 150, 200, false);
            break;
        }
        case XMAS_SLEIGH_BELL: {
            fb_circle_hsv(x, y, 2, 42, 255, 255, true);
            fb_rect_hsv(x - 1, y - 3, x + 1, y - 2, 42, 200, 200, true);
            break;
        }
        case XMAS_HEART: {
            fb_circle_hsv(x - 2, y - 2, 2, 0, 255, 255, true);
            fb_circle_hsv(x + 2, y - 2, 2, 0, 255, 255, true);
            fb_rect_hsv(x - 3, y - 1, x + 3, y + 2, 0, 255, 255, true);
            break;
        }
    }
}

// Draw Christmas advent items
void draw_christmas_advent_items(void) {
    uint8_t items_to_show = get_christmas_items_to_show();
    for (uint8_t i = 0; i < items_to_show; i++) {
        draw_christmas_item(advent_items[i].type, advent_items[i].x, advent_items[i].y);
    }
}

// Draw Santa sleigh
void draw_santa_sleigh(int16_t x, int16_t y) {
    if (x < -60 || x > 195) return;

    // Leading reindeer
    fb_circle_hsv(x + 40, y, 3, 20, 200, 150, true);
    fb_circle_hsv(x + 43, y - 2, 2, 20, 200, 150, true);
    fb_rect_hsv(x + 42, y - 5, x + 43, y - 3, 20, 180, 120, true);
    fb_rect_hsv(x + 44, y - 5, x + 45, y - 3, 20, 180, 120, true);
    fb_circle_hsv(x + 45, y - 2, 1, 0, 255, 255, true);
    fb_rect_hsv(x + 38, y + 2, x + 39, y + 4, 20, 200, 130, true);
    fb_rect_hsv(x + 42, y + 2, x + 43, y + 4, 20, 200, 130, true);

    // Second reindeer
    fb_circle_hsv(x + 25, y + 1, 3, 20, 200, 150, true);
    fb_circle_hsv(x + 28, y - 1, 2, 20, 200, 150, true);
    fb_rect_hsv(x + 27, y - 4, x + 28, y - 2, 20, 180, 120, true);
    fb_rect_hsv(x + 29, y - 4, x + 30, y - 2, 20, 180, 120, true);

    // Reins
    fb_rect_hsv(x + 20, y + 2, x + 40, y + 2, 20, 180, 100, true);

    // Sleigh
    fb_rect_hsv(x + 5, y + 2, x + 20, y + 8, 0, 255, 220, true);
    fb_rect_hsv(x + 5, y + 8, x + 20, y + 10, 42, 200, 200, true);
    fb_rect_hsv(x + 8, y - 2, x + 17, y + 2, 0, 200, 180, true);

    // Santa
    fb_circle_hsv(x + 12, y - 2, 2, 20, 150, 255, true);
    fb_rect_hsv(x + 10, y, x + 14, y + 4, 0, 255, 220, true);
    fb_rect_hsv(x + 10, y - 1, x + 14, y, 0, 0, 255, true);
    fb_circle_hsv(x + 12, y - 4, 2, 0, 255, 255, true);
    fb_rect_hsv(x + 11, y - 5, x + 13, y - 4, 0, 0, 255, true);
}

// Update Santa animation
void update_santa_animation(void) {
    if (!santa_initialized) {
        santa_x = -60;
        santa_initialized = true;
    }

    santa_x += 2;
    if (santa_x > 195) {
        santa_x = -60;
    }
}

// Draw Christmas scene
void draw_christmas_scene(void) {
    if (!is_christmas_season()) return;

    if (is_new_years_eve()) {
        draw_fireworks_scene();
        return;
    }

    draw_christmas_advent_items();

    if (current_day >= 25 && current_day < 31) {
        draw_santa_sleigh(santa_x, 40);
    }
}

// New Year's Eve check
bool is_new_years_eve(void) {
    return current_month == 12 && current_day == 31;
}

// Draw static firework
void draw_static_firework(int16_t x, int16_t y, uint8_t hue, uint8_t size) {
    // Draw center bright spot
    fb_circle_hsv(x, y, size / 2, hue, 255, 255, true);

    // Draw burst particles in 8 directions
    for (uint8_t angle = 0; angle < 8; angle++) {
        int8_t dx = 0, dy = 0;
        switch (angle) {
            case 0: dx = size; dy = 0; break;
            case 1: dx = size; dy = -size; break;
            case 2: dx = 0; dy = -size; break;
            case 3: dx = -size; dy = -size; break;
            case 4: dx = -size; dy = 0; break;
            case 5: dx = -size; dy = size; break;
            case 6: dx = 0; dy = size; break;
            case 7: dx = size; dy = size; break;
        }

        int16_t px = x + dx;
        int16_t py = y + dy;

        // Draw particle
        if (px >= 0 && px < 135 && py >= 0 && py < 152) {
            fb_circle_hsv(px, py, 2, hue, 255, 220, true);
        }

        // Draw trail between center and particle
        int16_t mid_x = x + (dx / 2);
        int16_t mid_y = y + (dy / 2);
        if (mid_x >= 0 && mid_x < 135 && mid_y >= 0 && mid_y < 152) {
            fb_circle_hsv(mid_x, mid_y, 1, hue, 200, 180, true);
        }
    }
}

// Draw fireworks scene
void draw_fireworks_scene(void) {
    // Draw 6 colorful static firework bursts
    const uint8_t colors[] = {0, 42, 85, 128, 170, 200};
    const struct {int16_t x; int16_t y; uint8_t size;} positions[NUM_FIREWORKS] = {
        {30, 50, 12},
        {70, 35, 14},
        {110, 55, 11},
        {25, 90, 13},
        {95, 80, 15},
        {60, 105, 12}
    };

    for (uint8_t i = 0; i < NUM_FIREWORKS; i++) {
        draw_static_firework(positions[i].x, positions[i].y, colors[i], positions[i].size);
    }

    // Draw "HNY" text at bottom
    uint16_t text_x = 45;
    uint16_t text_y = 130;

    // H
    fb_rect_hsv(text_x, text_y, text_x + 2, text_y + 12, 42, 255, 255, true);
    fb_rect_hsv(text_x + 8, text_y, text_x + 10, text_y + 12, 42, 255, 255, true);
    fb_rect_hsv(text_x, text_y + 5, text_x + 10, text_y + 7, 42, 255, 255, true);

    // N
    text_x += 15;
    fb_rect_hsv(text_x, text_y, text_x + 2, text_y + 12, 42, 255, 255, true);
    fb_rect_hsv(text_x + 8, text_y, text_x + 10, text_y + 12, 42, 255, 255, true);
    fb_rect_hsv(text_x + 2, text_y + 4, text_x + 8, text_y + 8, 42, 255, 255, true);

    // Y
    text_x += 15;
    fb_rect_hsv(text_x, text_y, text_x + 2, text_y + 6, 42, 255, 255, true);
    fb_rect_hsv(text_x + 8, text_y, text_x + 10, text_y + 6, 42, 255, 255, true);
    fb_rect_hsv(text_x + 2, text_y + 6, text_x + 8, text_y + 8, 42, 255, 255, true);
    fb_rect_hsv(text_x + 4, text_y + 8, text_x + 6, text_y + 12, 42, 255, 255, true);
}
