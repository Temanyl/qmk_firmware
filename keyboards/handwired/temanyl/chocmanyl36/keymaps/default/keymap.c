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
#include <stdlib.h>
#include <qp.h>
#include "draw_logo.h"
#include "graphics/helvetica20.qff.c"

// Display configuration
painter_device_t display;
static painter_font_handle_t media_font = NULL;
static uint8_t current_display_layer = 255; // Track currently displayed layer
static uint8_t backlight_brightness = 102;  // Current brightness level (40% default)
static uint32_t last_uptime_update = 0;     // Track last uptime display update

// Volume indicator state (permanent bar at bottom)
static uint8_t current_volume = 0;          // Current volume level (0-100)

// Date/time state (received from host)
static uint8_t current_hour = 0;            // Current hour (0-23)
static uint8_t current_minute = 0;          // Current minute (0-59)
static uint8_t current_day = 1;             // Current day (1-31)
static uint8_t current_month = 1;           // Current month (1-12)
static uint16_t current_year = 2025;        // Current year
static bool time_received = false;          // Whether we've received time from host

// Brightness indicator state (temporary overlay)
static uint8_t last_brightness_value = 102; // Track last brightness for change detection
static uint32_t brightness_display_timer = 0; // Timer for brightness display timeout
static bool brightness_display_active = false; // Whether brightness indicator is shown
#define BRIGHTNESS_DISPLAY_TIMEOUT 3000      // Show brightness for 3 seconds

// Media text state (scrolling text between uptime and volume bar)
static char current_media[64] = "";  // Current media text
static bool media_active = false;     // Whether media is playing
static uint8_t scroll_position = 0;  // Current scroll position in characters
static uint32_t scroll_timer = 0;    // Timer for scroll updates
static uint8_t text_length = 0;      // Length of current text in characters
static bool needs_scroll = false;    // Whether text is too long and needs scrolling
#define SCROLL_SPEED 300              // Tick every 500ms (0.5 seconds)
#define SCROLL_PAUSE_START 500      // Pause 2 seconds before first scroll
#define MAX_DISPLAY_CHARS 13         // Maximum characters that fit on display (~130px / 10px per char)

// Rain (static - no animation to avoid artifacts)
static bool rain_initialized = false; // Track if rain has been drawn
#define NUM_RAINDROPS 50

// Custom keycodes
enum custom_keycodes {
    DISP_UP = SAFE_RANGE,  // Display brightness up
    DISP_DN,               // Display brightness down
};

// Layer Names
enum layer_names {
    _MAC_COLEMAK_DH,
    _MAC_CODE,
    _MAC_NAV,
    _MAC_NUM
};

// Forward declarations
void draw_volume_bar(uint8_t hue, uint8_t sat, uint8_t val);
void draw_date_time(void);
void get_layer_color(uint8_t layer, uint8_t *hue, uint8_t *sat, uint8_t *val);
void draw_brightness_indicator(void);
void draw_media_text(void);
void draw_seasonal_animation(void);
void draw_tree(uint16_t base_x, uint16_t base_y, uint8_t season, uint8_t hue, uint8_t sat, uint8_t val);
void get_celestial_position(uint8_t hour, uint16_t *x, uint16_t *y);
void update_rain_animation(void);
void get_background_pixel_color(uint16_t x, uint16_t y, uint8_t *hue, uint8_t *sat, uint8_t *val);
void capture_scene_to_framebuffer(void);

// Helper function to draw a single digit using 7-segment style
void draw_digit(uint16_t x, uint16_t y, uint8_t digit, uint8_t hue, uint8_t sat, uint8_t val) {
    // Each segment is a rectangle
    // Segment positions (7-segment display):
    //  AAA
    // F   B
    //  GGG
    // E   C
    //  DDD

    bool seg_a = false, seg_b = false, seg_c = false, seg_d = false;
    bool seg_e = false, seg_f = false, seg_g = false;

    switch(digit) {
        case 0: seg_a = seg_b = seg_c = seg_d = seg_e = seg_f = true; break;
        case 1: seg_b = seg_c = true; break;
        case 2: seg_a = seg_b = seg_d = seg_e = seg_g = true; break;
        case 3: seg_a = seg_b = seg_c = seg_d = seg_g = true; break;
        case 4: seg_b = seg_c = seg_f = seg_g = true; break;
        case 5: seg_a = seg_c = seg_d = seg_f = seg_g = true; break;
        case 6: seg_a = seg_c = seg_d = seg_e = seg_f = seg_g = true; break;
        case 7: seg_a = seg_b = seg_c = true; break;
        case 8: seg_a = seg_b = seg_c = seg_d = seg_e = seg_f = seg_g = true; break;
        case 9: seg_a = seg_b = seg_c = seg_d = seg_f = seg_g = true; break;
    }

    // Draw segments (larger size, ~14x20 digit)
    if (seg_a) qp_rect(display, x + 2, y, x + 11, y + 2, hue, sat, val, true);          // top
    if (seg_b) qp_rect(display, x + 11, y + 2, x + 13, y + 9, hue, sat, val, true);     // top right
    if (seg_c) qp_rect(display, x + 11, y + 11, x + 13, y + 18, hue, sat, val, true);   // bottom right
    if (seg_d) qp_rect(display, x + 2, y + 18, x + 11, y + 20, hue, sat, val, true);    // bottom
    if (seg_e) qp_rect(display, x, y + 11, x + 2, y + 18, hue, sat, val, true);         // bottom left
    if (seg_f) qp_rect(display, x, y + 2, x + 2, y + 9, hue, sat, val, true);           // top left
    if (seg_g) qp_rect(display, x + 2, y + 9, x + 11, y + 11, hue, sat, val, true);     // middle
}

// Get color for a given layer
void get_layer_color(uint8_t layer, uint8_t *hue, uint8_t *sat, uint8_t *val) {
    switch (layer) {
        case _MAC_COLEMAK_DH:
            *hue = 128; *sat = 255; *val = 255;  // Teal
            break;
        case _MAC_NAV:
            *hue = 85; *sat = 255; *val = 255;   // Green
            break;
        case _MAC_CODE:
            *hue = 0; *sat = 255; *val = 255;    // Red
            break;
        case _MAC_NUM:
            *hue = 43; *sat = 255; *val = 255;   // Yellow
            break;
        default:
            *hue = 128; *sat = 255; *val = 255;  // Default to teal
            break;
    }
}

// Draw date and time at top of display
void draw_date_time(void) {
    // Get current layer color
    uint8_t layer = get_highest_layer(layer_state);
    uint8_t hue, sat, val;
    get_layer_color(layer, &hue, &sat, &val);

    // Clear date/time area above media text (black background)
    qp_rect(display, 0, 165, 134, 206, 0, 0, 0, true);

    // Date area: y=155 to y=171
    uint16_t date_y = 155;

    // Draw date in DD.MM.YYYY format centered
    // Each digit is ~14px wide, with dots: total ~115px wide
    uint16_t date_x = (135 - 115) / 2;

    // Draw day (2 digits)
    draw_digit(date_x, date_y, current_day / 10, hue, sat, val);
    draw_digit(date_x + 16, date_y, current_day % 10, hue, sat, val);

    // Draw first dot
    qp_rect(display, date_x + 31, date_y + 15, date_x + 34, date_y + 18, hue, sat, val, true);

    // Draw month (2 digits)
    draw_digit(date_x + 37, date_y, current_month / 10, hue, sat, val);
    draw_digit(date_x + 53, date_y, current_month % 10, hue, sat, val);

    // Draw second dot
    qp_rect(display, date_x + 68, date_y + 15, date_x + 71, date_y + 18, hue, sat, val, true);

    // Draw year (4 digits, but only last 2 for space)
    uint8_t year_last_two = current_year % 100;
    draw_digit(date_x + 74, date_y, year_last_two / 10, hue, sat, val);
    draw_digit(date_x + 90, date_y, year_last_two % 10, hue, sat, val);

    // Time area: y=190 to y=206
    uint16_t time_y = 180;

    // Draw time in HH:MM format centered
    // Each digit is ~14px wide, with colon: total ~70px wide
    uint16_t time_x = (135 - 70) / 2;

    // Draw hours (2 digits)
    draw_digit(time_x, time_y, current_hour / 10, hue, sat, val);
    draw_digit(time_x + 16, time_y, current_hour % 10, hue, sat, val);

    // Draw colon
    qp_rect(display, time_x + 32, time_y + 5, time_x + 35, time_y + 7, hue, sat, val, true);
    qp_rect(display, time_x + 32, time_y + 13, time_x + 35, time_y + 15, hue, sat, val, true);

    // Draw minutes (2 digits)
    draw_digit(time_x + 38, time_y, current_minute / 10, hue, sat, val);
    draw_digit(time_x + 54, time_y, current_minute % 10, hue, sat, val);

    // Note: No qp_flush() here - let the caller decide when to flush
}

// Helper function to draw a simple tree (adjusted size 6x22)
void draw_tree(uint16_t base_x, uint16_t base_y, uint8_t season, uint8_t hue, uint8_t sat, uint8_t val) {
    // Tree structure: trunk + canopy
    // Trunk (brown)
    uint8_t trunk_width = 6;
    uint8_t trunk_height = 22;
    qp_rect(display, base_x - trunk_width/2, base_y - trunk_height,
            base_x + trunk_width/2, base_y, 20, 200, 100, true);

    // Canopy changes by season
    if (season == 0) { // Winter - bare branches
        // Draw simple branch lines
        for (int8_t i = -3; i <= 3; i++) {
            qp_rect(display, base_x - 12 + i * 5, base_y - trunk_height - 4 - abs(i) * 2,
                    base_x - 9 + i * 5, base_y - trunk_height - 2 - abs(i) * 2, 20, 150, 80, true);
        }
    } else if (season == 1) { // Spring - pink blossoms
        // Tree shape with pink/white blossoms
        qp_circle(display, base_x, base_y - trunk_height - 7, 15, 234, 180, 255, true); // Pink
        // Add blossom dots
        for (uint8_t i = 0; i < 9; i++) {
            int8_t offset_x = (i % 3 - 1) * 7;
            int8_t offset_y = (i / 3 - 1) * 7;
            qp_circle(display, base_x + offset_x, base_y - trunk_height - 7 + offset_y, 3, 0, 0, 255, true); // White
        }
    } else if (season == 2) { // Summer - full green foliage
        // Dense green canopy
        qp_circle(display, base_x, base_y - trunk_height - 7, 16, 85, 255, 200, true);       // Center
        qp_circle(display, base_x - 9, base_y - trunk_height - 4, 11, 85, 255, 180, true);  // Left
        qp_circle(display, base_x + 9, base_y - trunk_height - 4, 11, 85, 255, 180, true);  // Right
    } else { // Fall - orange/red/yellow leaves
        // Tree shape with autumn colors
        qp_circle(display, base_x, base_y - trunk_height - 7, 15, 20, 255, 200, true);      // Orange
        qp_circle(display, base_x - 8, base_y - trunk_height - 4, 10, 10, 255, 220, true);  // Red-orange
        qp_circle(display, base_x + 8, base_y - trunk_height - 4, 10, 30, 255, 200, true);  // Yellow-orange
    }
}

// Calculate sun/moon position based on time of day
void get_celestial_position(uint8_t hour, uint16_t *x, uint16_t *y) {
    // Sun/moon moves across sky throughout the day
    // Hour 0-23: position from left to right
    // Peak at noon (y lowest), near horizon at dawn/dusk (y highest)

    // X position: moves from left (sunrise ~6am) to right (sunset ~18pm)
    // Map hour 0-23 to x position
    if (hour >= 6 && hour <= 18) {
        // Daytime: sun moves from left to right
        // hour 6 -> x=20, hour 12 -> x=67, hour 18 -> x=114
        *x = 20 + ((hour - 6) * 94) / 12;

        // Y position: arc across sky (lowest at noon)
        // hour 6 -> y=40, hour 12 -> y=20, hour 18 -> y=40
        int16_t time_from_noon = hour - 12;
        *y = 20 + (time_from_noon * time_from_noon) / 2;
    } else {
        // Nighttime: moon moves from right to left
        // hour 18-23 and 0-6
        uint8_t night_hour = (hour >= 18) ? (hour - 18) : (hour + 6);
        // night_hour 0-12: position from right to left
        *x = 114 - (night_hour * 94) / 12;

        // Y position: arc across sky (lowest at midnight)
        int16_t time_from_midnight = (hour >= 18) ? (hour - 24) : hour;
        *y = 25 + (time_from_midnight * time_from_midnight) / 3;
    }

    // Clamp values
    if (*x < 15) *x = 15;
    if (*x > 120) *x = 120;
    if (*y < 15) *y = 15;
    if (*y > 50) *y = 50;
}

// Draw seasonal animation based on time and month (overlays the entire upper screen including logo)
void draw_seasonal_animation(void) {
    // Animation area: entire upper portion from top to date area
    // Logo area: y=10 to y=130 (120x120 logo)
    // Animation extends from y=0 to y=152 (above date which starts at y=155)

    // Determine season based on month (1-12)
    // Winter: 12, 1, 2 | Spring: 3, 4, 5 | Summer: 6, 7, 8 | Fall: 9, 10, 11
    uint8_t season = 0; // 0=winter, 1=spring, 2=summer, 3=fall
    if (current_month == 12 || current_month <= 2) season = 0;
    else if (current_month >= 3 && current_month <= 5) season = 1;
    else if (current_month >= 6 && current_month <= 8) season = 2;
    else season = 3;

    // Determine time of day based on hour (0-23)
    bool is_night = (current_hour >= 20 || current_hour < 6);

    // Get sun/moon position based on time
    uint16_t celestial_x, celestial_y;
    get_celestial_position(current_hour, &celestial_x, &celestial_y);

    // === SKY AND CELESTIAL OBJECTS ===

    // Draw sun or moon with appropriate coloring based on time
    if (is_night) {
        // Draw moon (pale yellow/white)
        qp_circle(display, celestial_x, celestial_y, 8, 42, 100, 255, true);
        // Add some stars
        uint16_t star_positions[][2] = {{20, 15}, {50, 25}, {90, 18}, {110, 30}, {35, 40}};
        for (uint8_t i = 0; i < 5; i++) {
            qp_rect(display, star_positions[i][0], star_positions[i][1],
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
        qp_circle(display, celestial_x, celestial_y, 9, sun_hue, sun_sat, 255, true);

        // Add sun rays (8 rays around sun)
        for (uint8_t i = 0; i < 8; i++) {
            int16_t ray_x = 0, ray_y = 0;

            // Calculate ray direction (simplified - just 8 cardinal directions)
            if (i == 0) { ray_x = 12; ray_y = 0; }       // Right
            else if (i == 1) { ray_x = 9; ray_y = -9; }  // Up-right
            else if (i == 2) { ray_x = 0; ray_y = -12; } // Up
            else if (i == 3) { ray_x = -9; ray_y = -9; } // Up-left
            else if (i == 4) { ray_x = -12; ray_y = 0; } // Left
            else if (i == 5) { ray_x = -9; ray_y = 9; }  // Down-left
            else if (i == 6) { ray_x = 0; ray_y = 12; }  // Down
            else if (i == 7) { ray_x = 9; ray_y = 9; }   // Down-right

            qp_rect(display, celestial_x + ray_x - 1, celestial_y + ray_y - 1,
                    celestial_x + ray_x + 1, celestial_y + ray_y + 1, sun_hue, sun_sat, 200, true);
        }
    }

    // === GROUND AND TREES ===

    // Draw ground line
    uint16_t ground_y = 150;
    qp_rect(display, 0, ground_y, 134, ground_y + 1, 85, 180, 100, true); // Green-brown ground

    // Draw trees at different positions
    uint8_t layer = get_highest_layer(layer_state);
    uint8_t tree_hue, tree_sat, tree_val;
    get_layer_color(layer, &tree_hue, &tree_sat, &tree_val);

    draw_tree(30, ground_y, season, tree_hue, tree_sat, tree_val);
    draw_tree(67, ground_y, season, tree_hue, tree_sat, tree_val); // Center tree
    draw_tree(105, ground_y, season, tree_hue, tree_sat, tree_val);

    // === SEASONAL WEATHER EFFECTS ===

    if (season == 0) { // Winter - snow falling
        uint16_t snow_x[] = {15, 40, 65, 85, 110, 25, 55, 95, 120};
        uint16_t snow_y[] = {30, 50, 70, 40, 60, 80, 100, 90, 45};
        for (uint8_t i = 0; i < 9; i++) {
            // Simple snowflake
            qp_rect(display, snow_x[i], snow_y[i], snow_x[i] + 2, snow_y[i] + 2, 170, 80, 255, true);
            qp_rect(display, snow_x[i] - 2, snow_y[i] + 1, snow_x[i] + 4, snow_y[i] + 1, 170, 80, 255, true);
            qp_rect(display, snow_x[i] + 1, snow_y[i] - 2, snow_x[i] + 1, snow_y[i] + 4, 170, 80, 255, true);
        }
    } else if (season == 1) { // Spring - butterflies
        uint16_t butterfly_x[] = {25, 70, 100};
        uint16_t butterfly_y[] = {60, 80, 50};
        uint8_t butterfly_hues[] = {234, 170, 42}; // Pink, cyan, yellow
        for (uint8_t i = 0; i < 3; i++) {
            qp_circle(display, butterfly_x[i] - 2, butterfly_y[i], 2, butterfly_hues[i], 255, 200, true);
            qp_circle(display, butterfly_x[i] + 2, butterfly_y[i], 2, butterfly_hues[i], 255, 200, true);
        }
    } else if (season == 2) { // Summer - birds or clouds
        // Simple cloud shapes
        uint16_t cloud_x[] = {20, 90};
        for (uint8_t i = 0; i < 2; i++) {
            qp_circle(display, cloud_x[i], 35, 8, 0, 0, 180, true);
            qp_circle(display, cloud_x[i] + 10, 35, 6, 0, 0, 180, true);
            qp_circle(display, cloud_x[i] - 8, 35, 6, 0, 0, 180, true);
        }
    } else { // Fall - rain and clouds
        // Draw rain clouds (darker gray clouds)
        uint16_t cloud_x[] = {25, 70, 105};
        uint16_t cloud_y[] = {30, 40, 35};
        for (uint8_t i = 0; i < 3; i++) {
            // Main cloud body (dark gray)
            qp_circle(display, cloud_x[i], cloud_y[i], 9, 0, 0, 120, true);
            qp_circle(display, cloud_x[i] + 10, cloud_y[i] + 2, 7, 0, 0, 120, true);
            qp_circle(display, cloud_x[i] - 8, cloud_y[i] + 2, 7, 0, 0, 120, true);
            qp_circle(display, cloud_x[i] + 5, cloud_y[i] - 4, 6, 0, 0, 110, true);
        }

        // Draw rain drops scattered throughout the scene
        // Random distribution from clouds to near ground (50 drops)
        uint16_t rain_x[] = {91, 25, 108, 62, 45, 119, 31, 76, 100, 53, 17, 85, 69, 122, 38, 96, 58, 20, 106, 72,
                             41, 115, 29, 83, 50, 124, 64, 18, 98, 56, 36, 88, 67, 110, 42, 78, 26, 102, 60, 21,
                             94, 48, 116, 33, 81, 52, 120, 39, 75, 104};
        uint16_t rain_y_base[] = {86, 128, 61, 101, 74, 139, 52, 118, 93, 67, 131, 79, 105, 49, 123, 84, 58, 143, 71, 113,
                                  96, 54, 136, 88, 109, 63, 121, 76, 99, 56, 140, 82, 115, 69, 127, 91, 59, 103, 77, 133,
                                  94, 66, 51, 119, 87, 106, 73, 137, 98, 62};
        for (uint8_t i = 0; i < NUM_RAINDROPS; i++) {
            // Use static Y positions directly
            uint16_t y_pos = rain_y_base[i];

            // Draw raindrops (2 pixels wide, 4 pixels tall)
            uint8_t drop_height = 4;
            if (y_pos < 150) {
                qp_rect(display, rain_x[i], y_pos, rain_x[i] + 1, y_pos + drop_height, 170, 150, 200, true);
            }
        }
        rain_initialized = true;

        // Draw fallen leaves on the ground (just above ground line at y=150)
        uint16_t leaf_ground_x[] = {18, 35, 52, 68, 82, 95, 108, 122, 25, 45, 62, 78, 92, 105, 118};
        uint8_t leaf_ground_colors[] = {10, 0, 25, 15, 8, 20, 5, 30, 12, 18, 22, 28, 15, 10, 25}; // Orange, red, yellow shades
        for (uint8_t i = 0; i < 15; i++) {
            // Small leaves on ground (small circles just above ground line)
            qp_circle(display, leaf_ground_x[i], 146, 2, leaf_ground_colors[i], 255, 220, true);
        }
    }

    // Note: No qp_flush() here - let the caller decide when to flush
}

// Function to draw logo with color based on layer
void set_layer_background(uint8_t layer) {
    // Only update if the layer actually changed
    if (layer == current_display_layer) {
        return;
    }
    current_display_layer = layer;

    // Always draw black background
    qp_rect(display, 0, 0, 134, 239, 0, 0, 0, true);

    // Select logo color based on layer
    uint8_t hue, sat, val;
    get_layer_color(layer, &hue, &sat, &val);

    // Draw the logo at the top with the selected color
    draw_amboss_logo(display, 7, 10, hue, sat, val);

    // Draw seasonal animation between logo and date
    draw_seasonal_animation();

    // Redraw date/time above volume bar (will handle its own clearing)
    draw_date_time();

    // Redraw media text (if active) with new layer color
    draw_media_text();

    // Redraw volume bar with the same color at the bottom
    draw_volume_bar(hue, sat, val);

    qp_flush(display);
}

// Update display based on current layer state
void update_display_for_layer(void) {
    set_layer_background(get_highest_layer(layer_state));
}

// Draw volume bar at bottom of screen (permanent)
void draw_volume_bar(uint8_t hue, uint8_t sat, uint8_t val) {
    // Calculate bar width based on volume (max width 120 pixels, leaving margins)
    uint16_t bar_width = (current_volume * 120) / 100;

    // Clear bottom area with black background (starts after media text at y=230)
    qp_rect(display, 0, 231, 134, 239, 0, 0, 0, true);

    // Draw volume bar outline (thin light grey border)
    qp_rect(display, 5, 233, 127, 238, 0, 0, 150, false);

    // Draw filled volume bar using the logo color
    if (bar_width > 0) {
        qp_rect(display, 6, 234, 6 + bar_width, 237, hue, sat, val, true);
    }

    // Note: No qp_flush() here - let the caller decide when to flush
}

// Draw brightness indicator overlay (temporary, appears when brightness changes)
void draw_brightness_indicator(void) {
    // Brightness indicator appears as an overlay at the top of the screen
    // Box dimensions: 100x40 at top center
    uint16_t box_x = 17;   // (135 - 100) / 2
    uint16_t box_y = 10;   // Top of screen
    uint16_t box_w = 100;
    uint16_t box_h = 40;

    // Draw dark grey background box with lighter border
    qp_rect(display, box_x, box_y, box_x + box_w, box_y + box_h, 0, 0, 40, true);
    qp_rect(display, box_x, box_y, box_x + box_w, box_y + box_h, 0, 0, 150, false);

    // Draw "BRI" text using simple rectangles at top of box
    uint16_t text_y = box_y + 6;
    uint16_t text_x = box_x + 8;

    // B (white text)
    qp_rect(display, text_x, text_y, text_x + 1, text_y + 9, 0, 0, 255, true);
    qp_rect(display, text_x, text_y, text_x + 6, text_y + 1, 0, 0, 255, true);
    qp_rect(display, text_x, text_y + 4, text_x + 5, text_y + 5, 0, 0, 255, true);
    qp_rect(display, text_x, text_y + 9, text_x + 6, text_y + 10, 0, 0, 255, true);
    qp_rect(display, text_x + 5, text_y + 1, text_x + 7, text_y + 4, 0, 0, 255, true);
    qp_rect(display, text_x + 5, text_y + 5, text_x + 7, text_y + 9, 0, 0, 255, true);

    // R (white text)
    text_x += 10;
    qp_rect(display, text_x, text_y, text_x + 1, text_y + 9, 0, 0, 255, true);
    qp_rect(display, text_x, text_y, text_x + 6, text_y + 1, 0, 0, 255, true);
    qp_rect(display, text_x, text_y + 4, text_x + 5, text_y + 5, 0, 0, 255, true);
    qp_rect(display, text_x + 5, text_y + 1, text_x + 7, text_y + 4, 0, 0, 255, true);
    qp_rect(display, text_x + 4, text_y + 5, text_x + 7, text_y + 9, 0, 0, 255, true);

    // I (white text)
    text_x += 10;
    qp_rect(display, text_x, text_y, text_x + 5, text_y + 1, 0, 0, 255, true);
    qp_rect(display, text_x + 2, text_y + 1, text_x + 3, text_y + 9, 0, 0, 255, true);
    qp_rect(display, text_x, text_y + 9, text_x + 5, text_y + 10, 0, 0, 255, true);

    // Draw percentage using 7-segment digits
    uint16_t digit_x = box_x + 36;
    uint16_t digit_y = box_y + 18;

    // Get current layer color for digits
    uint8_t layer = get_highest_layer(layer_state);
    uint8_t hue, sat, val;
    get_layer_color(layer, &hue, &sat, &val);

    // Calculate brightness percentage (backlight_brightness is 0-255, convert to 0-100)
    uint8_t brightness_percent = (backlight_brightness * 100) / 255;

    // Draw brightness percentage (3 digits max: 0-100)
    uint8_t hundreds = brightness_percent / 100;
    uint8_t tens = (brightness_percent % 100) / 10;
    uint8_t ones = brightness_percent % 10;

    if (hundreds > 0) {
        draw_digit(digit_x, digit_y, hundreds, hue, sat, val);
        digit_x += 14;
    }

    if (hundreds > 0 || tens > 0) {
        draw_digit(digit_x, digit_y, tens, hue, sat, val);
        digit_x += 14;
    }

    draw_digit(digit_x, digit_y, ones, hue, sat, val);
    digit_x += 14;

    // Draw "%" symbol
    qp_rect(display, digit_x + 2, digit_y + 2, digit_x + 4, digit_y + 4, hue, sat, val, true);
    qp_rect(display, digit_x + 4, digit_y + 5, digit_x + 6, digit_y + 7, hue, sat, val, true);
    qp_rect(display, digit_x + 5, digit_y + 8, digit_x + 7, digit_y + 10, hue, sat, val, true);
    qp_rect(display, digit_x + 7, digit_y + 12, digit_x + 9, digit_y + 14, hue, sat, val, true);
    qp_rect(display, digit_x + 2, digit_y + 16, digit_x + 4, digit_y + 18, hue, sat, val, true);

    qp_flush(display);
}

// Draw media text using Quantum Painter's text rendering with scrolling
void draw_media_text(void) {
    // Media text area: between uptime timer and volume bar
    // Timer ends at y=206, media starts at y=207
    // Area: y=207-230 (23px height with 2px padding top, 1px bottom)
    uint16_t media_y = 207;
    uint16_t media_h = 23;

    // Clear media text area (black background)
    qp_rect(display, 0, media_y, 134, media_y + media_h - 1, 0, 0, 0, true);

    // Only draw text if font was loaded successfully
    if (media_font != NULL) {
        // Get current layer color
        uint8_t layer = get_highest_layer(layer_state);
        uint8_t hue, sat, val;
        get_layer_color(layer, &hue, &sat, &val);

        // Determine what text to show
        const char *display_text;
        if (media_active && current_media[0] != '\0') {
            display_text = current_media;
        } else {
            display_text = "No Media playing";
        }

        // Calculate text length if this is new text
        if (text_length == 0) {
            text_length = strlen(display_text);
            needs_scroll = (text_length > MAX_DISPLAY_CHARS);
            scroll_position = 0;
            scroll_timer = timer_read32();
        }

        // Prepare display buffer with scrolled text
        char display_buffer[MAX_DISPLAY_CHARS + 1];

        if (needs_scroll) {
            // Create a scrolling window into the text
            for (uint8_t i = 0; i < MAX_DISPLAY_CHARS; i++) {
                uint8_t source_pos = (scroll_position + i) % (text_length + 3); // +3 for spacing
                if (source_pos < text_length) {
                    display_buffer[i] = display_text[source_pos];
                } else {
                    display_buffer[i] = ' '; // Gap between repetitions
                }
            }
            display_buffer[MAX_DISPLAY_CHARS] = '\0';
        } else {
            // Text fits, no scrolling needed
            strncpy(display_buffer, display_text, MAX_DISPLAY_CHARS);
            display_buffer[MAX_DISPLAY_CHARS] = '\0';
        }

        // Draw the text (2px left margin, 2px top padding)
        qp_drawtext_recolor(display, 2, media_y + 2, media_font, display_buffer, hue, sat, val, 0, 0, 0);
    } else {
        // Font failed to load - draw error indicator (red rectangle)
        qp_rect(display, 2, media_y + 2, 20, media_y + 10, 0, 255, 255, true);
    }

    // Note: No qp_flush() here - let the caller decide when to flush
}

// Set backlight brightness via PWM
void set_backlight_brightness(uint8_t brightness) {
    backlight_brightness = brightness;
    // Update PWM duty cycle (channel A compare value)
    *(volatile uint32_t*)(0x40050028 + 0x0C) = brightness;

    // Show brightness indicator overlay and reset timer
    brightness_display_active = true;
    brightness_display_timer = timer_read32();
    last_brightness_value = brightness;

    // Draw the brightness indicator overlay
    draw_brightness_indicator();
}

// Initialize the ST7789 display
static void init_display(void) {
    // CRITICAL: Enable display power on GP22 (LILYGO board power enable)
    setPinOutput(GP22);
    writePinHigh(GP22);

    // Small delay to let power stabilize
    wait_ms(50);

    // Create display: 135x240 portrait mode (rotated 90°)
    // Using SPI mode 3 and slower divisor (16) for reliable communication
    display = qp_st7789_make_spi_device(135, 240, GP5, GP1, GP0, 16, 3);

    // LILYGO T-Display RP2040: Portrait mode with proper offsets
    qp_set_viewport_offsets(display, 53, 40);

    // Initialize with 180° rotation (controller mounted upside down)
    if (!qp_init(display, QP_ROTATION_180)) {
        return;  // Initialization failed
    }

    // Power on display
    if (!qp_power(display, true)) {
        return;  // Power on failed
    }

    // Wait for display to stabilize
    wait_ms(50);

    // Load font for media text (20px Helvetica)
    media_font = qp_load_font_mem(font_helvetica20);

    // Dim backlight on GP4 using PWM (50% brightness)
    // First unreset the PWM peripheral (RESETS_BASE=0x4000c000, bit 14 for PWM)
    *(volatile uint32_t*)(0x4000c000) &= ~(1 << 14);  // Clear PWM reset bit

    // Wait for PWM reset to complete
    while (!(*(volatile uint32_t*)(0x4000c008) & (1 << 14))) {
        wait_ms(1);
    }

    // Set GPIO4 to PWM function
    *(volatile uint32_t*)(0x40014024) = 4;

    /*
      - 26 = 10%
      - 51 = 20%
      - 77 = 30%
      - 102 = 40%
      - 128 = 50%
      - 191 = 75%
      - 255 = 100%
    */
    // Configure PWM slice 2 (GP4 = PWM2_A)
    *(volatile uint32_t*)(0x40050028 + 0x04) = 16 << 4;  // DIV: no division
    *(volatile uint32_t*)(0x40050028 + 0x10) = 255;      // TOP: wrap at 255
    *(volatile uint32_t*)(0x40050028 + 0x0C) = 102;      // CC: channel A = 128 (50%)
    *(volatile uint32_t*)(0x40050028 + 0x00) = 0x01;     // CSR: enable

    // Fill screen with black background (135x240 portrait)
    qp_rect(display, 0, 0, 134, 239, 0, 0, 0, true);
    wait_ms(50);

    // Draw the Amboss logo at the top in teal using line-by-line rendering
    // Logo is 120x120, centered horizontally (135-120)/2 = 7.5, positioned at top (y=10)
    draw_amboss_logo(display, 7, 10, 128, 255, 255);  // Teal color

    // Update brightness variable to match the PWM setting
    backlight_brightness = 102;
    last_brightness_value = 102;

    // Draw seasonal animation between logo and date
    draw_seasonal_animation();

    // Draw initial date/time above volume bar
    draw_date_time();

    // Draw initial media text (empty at startup)
    draw_media_text();

    // Draw initial volume bar with teal color (base layer) at bottom
    draw_volume_bar(128, 255, 255);

    // Force flush to ensure everything is drawn
    qp_flush(display);
}

void keyboard_post_init_kb(void) {
    // Initialize the display
    init_display();
}

// Raw HID receive callback - handles data from computer
void raw_hid_receive(uint8_t *data, uint8_t length) {
    // Protocol:
    // Byte 0: Command ID
    //   0x01 = Volume update (Byte 1: volume 0-100)
    //   0x02 = Media text update (Bytes 1-31: null-terminated string)
    //   0x03 = Date/Time update (Bytes 1-7: year_low, year_high, month, day, hour, minute, second)

    if (length < 2) return;  // Need at least 2 bytes

    uint8_t command = data[0];

    switch (command) {
        case 0x01:  // Volume update
            current_volume = data[1];
            // Clamp to 0-100 range
            if (current_volume > 100) {
                current_volume = 100;
            }

            // Update the permanent volume bar
            uint8_t layer = get_highest_layer(layer_state);
            uint8_t hue, sat, val;
            get_layer_color(layer, &hue, &sat, &val);
            draw_volume_bar(hue, sat, val);
            qp_flush(display);
            break;

        case 0x02:  // Media text update
            {
                // Copy media text (null-terminated string starting at byte 1)
                bool text_changed = false;
                if (data[1] == 0) {
                    // Empty string = no media playing
                    if (media_active) {
                        media_active = false;
                        current_media[0] = '\0';
                        text_changed = true;
                    }
                } else {
                    // Copy media text
                    char new_media[sizeof(current_media)];
                    strncpy(new_media, (char*)&data[1], sizeof(new_media) - 1);
                    new_media[sizeof(new_media) - 1] = '\0';

                    // Only update if text changed
                    if (strcmp(current_media, new_media) != 0) {
                        strncpy(current_media, new_media, sizeof(current_media) - 1);
                        current_media[sizeof(current_media) - 1] = '\0';
                        media_active = true;
                        text_changed = true;
                    }
                }

                // Only redraw if text actually changed
                if (text_changed) {
                    // Reset scroll state for new text
                    scroll_position = 0;
                    text_length = 0;
                    needs_scroll = false;
                    scroll_timer = timer_read32();
                    draw_media_text();
                    qp_flush(display);
                }
            }
            break;

        case 0x03:  // Date/Time update
            if (length >= 8) {
                // Extract date/time components
                current_year = data[1] | (data[2] << 8);  // 16-bit year
                current_month = data[3];
                current_day = data[4];
                current_hour = data[5];
                current_minute = data[6];
                // data[7] is seconds, but we don't display it

                // Validate ranges
                if (current_month < 1 || current_month > 12) current_month = 1;
                if (current_day < 1 || current_day > 31) current_day = 1;
                if (current_hour > 23) current_hour = 0;
                if (current_minute > 59) current_minute = 0;

                time_received = true;
                last_uptime_update = timer_read32();
                draw_date_time();
                qp_flush(display);
            }
            break;

        default:
            // Unknown command, ignore
            break;
    }
}


// Tap Dance declarations
enum {
    TD_Q_ESC_EMOJI_RESET,
    TD_ESC_WINDOWS_EMOJI,
    TD_LAYER_NAV_NUM,
    TD_LAYER_DEFAULT_SHIFT,
};

// Define a type for as many tap dance states as you need
typedef enum {
    TD_NONE,
    TD_UNKNOWN,
    TD_SINGLE_TAP,
    TD_SINGLE_HOLD,
    TD_DOUBLE_TAP,
    TD_OSL_CODE
} td_state_t;


typedef struct {
    bool is_press_action;
    td_state_t state;
} td_tap_t;


// Declare the functions to be used with your tap dance key(s)

// Function associated with all tap dances
td_state_t cur_dance(tap_dance_state_t *state);

// Functions associated with individual tap dances
void nav_num_finished(tap_dance_state_t *state, void *user_data);
void nav_num_reset(tap_dance_state_t *state, void *user_data);
void layer_default_shift_finished(tap_dance_state_t *state, void *user_data);
void layer_default_shift_reset(tap_dance_state_t *state, void *user_data);
void osl_code_finished(tap_dance_state_t *state, void *user_data);
void osl_code_reset(tap_dance_state_t *state, void *user_data);


// #############################################################

void td_q_esc_emoji_reset (tap_dance_state_t *state, void *user_data) {
    if (state->count == 1) {
        tap_code(KC_Q);
    } else if (state->count == 2) {
        tap_code(KC_ESC);
    } else if (state->count == 3) {
        tap_code16(C(G(KC_SPC)));
    } else if (state->count == 5) {
        reset_keyboard();
    }
}

 // Tap Dance definitions
tap_dance_action_t tap_dance_actions[] = {
    [TD_Q_ESC_EMOJI_RESET]   = ACTION_TAP_DANCE_FN(td_q_esc_emoji_reset),
    [TD_LAYER_NAV_NUM]       = ACTION_TAP_DANCE_FN_ADVANCED(NULL, nav_num_finished, nav_num_reset),
    [TD_LAYER_DEFAULT_SHIFT] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, layer_default_shift_finished, layer_default_shift_reset),
    [TD_OSL_CODE]            = ACTION_TAP_DANCE_FN_ADVANCED(NULL, osl_code_finished, osl_code_reset)
};
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case TD(TD_Q_ESC_EMOJI_RESET) :
        case TD(TD_ESC_WINDOWS_EMOJI) :
        case LGUI_T(KC_SPC) :
        case LT(1, KC_TAB) :
        case LT(2, KC_ENT) :
            return 200;
        case TD(TD_LAYER_DEFAULT_SHIFT):
            return 180;
        case LT(0,KC_SCLN) :
            return 155;
    default:
      return TAPPING_TERM;
  }
};

bool send_hold_code(uint16_t keycode, keyrecord_t *record) {
        if (!record->tap.count && record->event.pressed) {
            tap_code16(G(keycode)); // Intercept hold function to send Ctrl-X
            return false;
        }
        return true;
}

// Initialize variable holding the binary
// representation of active modifiers.
uint8_t mod_state;
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    mod_state = get_mods();
    switch (keycode) {
        case LT(0,KC_SCLN):
            if (!record->tap.count && record->event.pressed) {
                tap_code(KC_ENT);
                return false;
            }
            return true;
        case KC_BSPC:
        {
            // Initialize a boolean variable that keeps track
            // of the delete key status: registered or not?
            static bool delkey_registered;
            if (record->event.pressed) {
                // Detect the activation of either shift keys
                if (mod_state & MOD_MASK_SHIFT) {
                    // First temporarily canceling both shifts so that
                    // shift isn't applied to the KC_DEL keycode
                    del_mods(MOD_MASK_SHIFT);
                    register_code(KC_DEL);
                    // Update the boolean variable to reflect the status of KC_DEL
                    delkey_registered = true;
                    // Reapplying modifier state so that the held shift key(s)
                    // still work even after having tapped the Backspace/Delete key.
                    set_mods(mod_state);
                    return false;
                }
            } else { // on release of KC_BSPC
                // In case KC_DEL is still being sent even after the release of KC_BSPC
                if (delkey_registered) {
                    unregister_code(KC_DEL);
                    delkey_registered = false;
                    return false;
                }
            }
            // Let QMK process the KC_BSPC keycode as usual outside of shift
            return true;
        }
        case DISP_UP:
            if (record->event.pressed) {
                // Increase brightness by ~10% (25 steps)
                if (backlight_brightness < 230) {
                    set_backlight_brightness(backlight_brightness + 25);
                } else {
                    set_backlight_brightness(255); // Max brightness
                }
            }
            return false;
        case DISP_DN:
            if (record->event.pressed) {
                // Decrease brightness by ~10% (25 steps)
                if (backlight_brightness > 25) {
                    set_backlight_brightness(backlight_brightness - 25);
                } else {
                    set_backlight_brightness(1); // Min brightness (not off)
                }
            }
            return false;
    }
    return true;
}

// Periodically check and update display based on active layer
void housekeeping_task_user(void) {
    update_display_for_layer();

    uint32_t current_time = timer_read32();
    bool needs_flush = false;

    // Update date/time display once per minute (or when time is received)
    if (time_received && (current_time - last_uptime_update >= 60000)) {
        last_uptime_update = current_time;
        // Increment minute (host will send updated time periodically)
        current_minute++;
        if (current_minute >= 60) {
            current_minute = 0;
            current_hour++;
            if (current_hour >= 24) {
                current_hour = 0;
                // Day rollover - would need date logic, but host should update before this
            }
        }
        draw_date_time();
        needs_flush = true;
    }

    // Handle brightness display timeout
    if (brightness_display_active) {
        if (current_time - brightness_display_timer >= BRIGHTNESS_DISPLAY_TIMEOUT) {
            // Timeout expired, hide brightness indicator
            brightness_display_active = false;
            // Force a full redraw by invalidating the current layer
            current_display_layer = 255;
            update_display_for_layer();
            needs_flush = true;
        }
    }

    // Handle media text scrolling (character-based tick scrolling)
    if (needs_scroll && media_active) {
        uint32_t elapsed = current_time - scroll_timer;

        // Start scrolling after initial pause
        if (elapsed >= SCROLL_PAUSE_START) {
            // Calculate how many ticks should have occurred
            uint32_t scroll_ticks = (elapsed - SCROLL_PAUSE_START) / SCROLL_SPEED;
            uint8_t target_position = scroll_ticks % (text_length + 3); // +3 for spacing

            // Only redraw if scroll position actually changed
            if (target_position != scroll_position) {
                scroll_position = target_position;
                draw_media_text();
                needs_flush = true;
            }
        }
    }

    // Rain animation disabled - static rain looks better without artifacts

    // Single flush at the end to batch all updates
    if (needs_flush) {
        qp_flush(display);
    }
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_MAC_CODE] = LAYOUT_ortho_3x10_6(
        KC_UNDS, KC_LT,   KC_GT,   KC_LCBR, KC_RCBR,        KC_PIPE,  KC_AT,   KC_BSLS, KC_GRAVE, KC_ENT,
        KC_EXLM, KC_MINS, KC_EQL,  KC_LPRN, KC_RPRN,        KC_AMPR,  KC_QUOT, KC_DOWN, KC_DQUO, KC_NO,
        KC_CIRC, KC_PLUS, KC_ASTR, KC_LBRC, KC_RBRC,        KC_TILDE, KC_DLR,  KC_PERC, KC_HASH, RSFT_T(KC_BSLS),
                            KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,        KC_BSPC,  TO(_MAC_NAV), KC_NO
    ),
    [_MAC_NAV] = LAYOUT_ortho_3x10_6(
        KC_ESC,  MS_BTN1, MS_UP, MS_BTN2, KC_NO,          KC_VOLU, KC_PGUP, KC_UP,    KC_PGDN, KC_ENT,
        KC_NO,   KC_LCTL, KC_LALT, KC_LGUI, KC_MPLY,        KC_MUTE, KC_LEFT, KC_DOWN,  KC_RGHT, KC_NO,
        KC_NO,   MS_LEFT, MS_DOWN, MS_RGHT, KC_NO,          KC_VOLD, KC_NO,   KC_NO,    KC_NO,   KC_NO,
                          KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,          KC_BSPC, KC_NO, TO(_MAC_CODE)
    ),
    [_MAC_NUM] = LAYOUT_ortho_3x10_6(
         KC_F1,   KC_F2, KC_F3,   KC_F4,   KC_F5,          KC_DOT,   KC_7,   KC_8,  KC_9,   KC_ENT,
         KC_F6,   KC_F7, KC_F8,   KC_F9,   KC_F10,         KC_COMMA, KC_4,   KC_5,  KC_6,   DISP_UP,
         KC_F11,  KC_F12,KC_LCTL, KC_LALT, KC_LGUI,        KC_0,     KC_1,   KC_2,  KC_3,   DISP_DN,
                         KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,           KC_BSPC, TO(_MAC_NAV), KC_NO
    ),
    [_MAC_COLEMAK_DH] = LAYOUT_ortho_3x10_6(
         TD(TD_Q_ESC_EMOJI_RESET), KC_W,  KC_F,    KC_P,  KC_B,           KC_J,  KC_L,          KC_U,              KC_Y,           LT(0,KC_SCLN),
         KC_A, LCTL_T(KC_R), LALT_T(KC_S), LGUI_T(KC_T),  KC_G,           KC_M,  LGUI_T(KC_N),  LALT_T(KC_E),      LCTL_T(KC_I),   KC_O,
         KC_Z, KC_X,         KC_C,         KC_D,          KC_V,           KC_K,  KC_H,          KC_COMMA,          KC_DOT,         KC_SLSH,
                MEH_T(KC_TAB), KC_LSFT, KC_SPC,      KC_BSPC, TD(TD_LAYER_NAV_NUM), OSL(_MAC_CODE)
     )
};

// tap dances again
// Determine the current tap dance state
td_state_t cur_dance(tap_dance_state_t *state) {
    if (state->count == 1) {
        if (!state->pressed) return TD_SINGLE_TAP;
        else return TD_SINGLE_HOLD;
    } else if (state->count == 2) return TD_DOUBLE_TAP;
    else return TD_UNKNOWN;
}

// Initialize tap structure associated with example tap dance key
static td_tap_t ql_tap_state = {
    .is_press_action = true,
    .state = TD_NONE
};

// Functions that control what our tap dance key does
void nav_num_finished(tap_dance_state_t *state, void *user_data) {
    ql_tap_state.state = cur_dance(state);
    switch (ql_tap_state.state) {
        case TD_SINGLE_TAP:
           // Check to see if the layer is already set
           if (layer_state_is(_MAC_NAV)) {
               // If already set, then switch it off
               layer_off(_MAC_NAV);
           } else {
               // If not already set, then switch the layer on
               layer_on(_MAC_NAV);
           }
           break;
        case TD_SINGLE_HOLD:
            layer_on(_MAC_NUM);
            break;
        case TD_DOUBLE_TAP:
            // Check to see if the layer is already set
            if (layer_state_is(_MAC_NUM)) {
                // If already set, then switch it off
                layer_off(_MAC_NUM);
            } else {
                // If not already set, then switch the layer on
                layer_on(_MAC_NUM);
            }
            break;
        default:
            break;
    }
}

void nav_num_reset(tap_dance_state_t *state, void *user_data) {
    // If the key was held down and now is released then switch off the layer
    if (ql_tap_state.state != TD_DOUBLE_TAP) {
        layer_off(_MAC_NUM);
    }
    ql_tap_state.state = TD_NONE;
}

// Functions that control what our tap dance key does
void layer_default_shift_finished(tap_dance_state_t *state, void *user_data) {
    ql_tap_state.state = cur_dance(state);
    switch (ql_tap_state.state) {
        case TD_SINGLE_TAP:
           layer_clear();
           break;
        case TD_SINGLE_HOLD:
            register_code(KC_LSFT);
            break;
        default:
            break;
    }
}

void layer_default_shift_reset(tap_dance_state_t *state, void *user_data) {
    // If the key was held down and now is released then switch off the layer
    if (ql_tap_state.state == TD_SINGLE_HOLD) {
         unregister_code(KC_LSFT);
    }
    ql_tap_state.state = TD_NONE;
}

void osl_code_finished(tap_dance_state_t *state, void *user_data) {
    ql_tap_state.state = cur_dance(state);
    switch (ql_tap_state.state) {
        case TD_SINGLE_TAP:
            set_oneshot_layer(_MAC_CODE, ONESHOT_START);
            break;
        case TD_SINGLE_HOLD:
            layer_on(_MAC_CODE);
            break;
        default:
            break;
    }
}

void osl_code_reset(tap_dance_state_t *state, void *user_data) {
    ql_tap_state.state = cur_dance(state);
    // If the key was held down and now is released then switch off the layer
    if (ql_tap_state.state == TD_SINGLE_TAP) {
        clear_oneshot_layer_state(ONESHOT_PRESSED);
    } else {
        layer_clear();
    }
}

// OLED
//#ifdef OLED_ENABLE
// Draw to OLED
//bool oled_task_user() {
//
//    // Layer text
//    oled_set_cursor(0, 1);
//    switch (get_highest_layer(layer_state)) {
//        case _MAC_CODE :
//            oled_write_P(PSTR("MAC"), false);
//            oled_set_cursor(0, 2);
//            oled_write_P(PSTR("SYM"), false);
//            break;
//        case _MAC_NUM :
//            oled_write_P(PSTR("MAC"), false);
//            oled_set_cursor(0, 2);
//            oled_write_P(PSTR("NUM"), false);
//            break;
//        case _MAC_NAV :
//            oled_write_P(PSTR("MAC"), false);
//            oled_set_cursor(0, 2);
//            oled_write_P(PSTR("NAV"), false);
//            break;
//        case _MAC_COLEMAK_DH :
//            oled_write_P(PSTR("MAC"), false);
//            oled_set_cursor(0, 2);
//            oled_write_P(PSTR("COLE"), false);
//            break;
//    }
//
//    // Caps lock text
//    led_t led_state = host_keyboard_led_state();
//    oled_set_cursor(0, 3);
//    oled_write_P(led_state.caps_lock ? PSTR("CAPS") : PSTR(""), false);
//
//    return false;
//}
//#endif
