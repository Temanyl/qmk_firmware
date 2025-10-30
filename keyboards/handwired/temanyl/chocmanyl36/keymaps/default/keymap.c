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
static uint8_t last_hour = 255;             // Track last hour for sun/moon position updates
static uint8_t last_day = 255;              // Track last day for moon phase updates

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

// Halloween event (Oct 28 - Nov 3) - static decorations
#define NUM_PUMPKINS 4
#define NUM_GHOSTS 2

// Christmas advent calendar (Dec 1-31)
#define NUM_CHRISTMAS_ITEMS 24
#define SANTA_ANIMATION_SPEED 100  // Update every 100ms for smooth Santa flight

// Christmas item types
typedef enum {
    XMAS_PRESENT_RED, XMAS_PRESENT_GREEN, XMAS_PRESENT_BLUE,
    XMAS_CANDY_CANE, XMAS_STOCKING, XMAS_ORNAMENT_RED,
    XMAS_ORNAMENT_GOLD, XMAS_ORNAMENT_BLUE, XMAS_BELL,
    XMAS_HOLLY, XMAS_STAR_SMALL, XMAS_SNOWFLAKE,
    XMAS_CANDLE, XMAS_TREE_SMALL, XMAS_GINGERBREAD,
    XMAS_WREATH, XMAS_ANGEL, XMAS_REINDEER_SMALL,
    XMAS_SNOWMAN_SMALL, XMAS_LIGHTS, XMAS_MISTLETOE,
    XMAS_NORTH_STAR, XMAS_SLEIGH_BELL, XMAS_HEART
} christmas_item_type_t;

typedef struct {
    christmas_item_type_t type;
    int16_t x;
    int16_t y;
} christmas_item_t;

static bool santa_initialized = false;
static uint32_t santa_animation_timer = 0;
static int16_t santa_x = -60;  // Start offscreen left

// New Year's Eve fireworks (Dec 31) - static display
#define NUM_FIREWORKS 6

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
void draw_cabin(uint16_t base_x, uint16_t base_y, uint8_t season);
void get_celestial_position(uint8_t hour, uint16_t *x, uint16_t *y);
void update_rain_animation(void);
void get_background_pixel_color(uint16_t x, uint16_t y, uint8_t *hue, uint8_t *sat, uint8_t *val);
void capture_scene_to_framebuffer(void);

// Halloween event functions
bool is_halloween_event(void);
void draw_pumpkin(int16_t x, int16_t y, uint8_t size);
void draw_ghost(int16_t x, int16_t y);
void draw_halloween_elements(void);

// Christmas advent calendar functions
bool is_christmas_season(void);
bool is_new_years_eve(void);
uint8_t get_christmas_items_to_show(void);
void draw_christmas_item(christmas_item_type_t type, int16_t x, int16_t y);
void draw_christmas_advent_items(void);
void draw_santa_sleigh(int16_t x, int16_t y);
void update_santa_animation(void);
void draw_christmas_scene(void);

// New Year's Eve fireworks functions
void draw_static_firework(int16_t x, int16_t y, uint8_t hue, uint8_t size);
void draw_fireworks_scene(void);

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
    // Date starts at y=155 and is 20px tall, time starts at y=180 and is 20px tall
    qp_rect(display, 0, 155, 134, 206, 0, 0, 0, true);

    // Date area: y=155 to y=175 (20px tall digits)
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

// Helper function to draw a simple tree (adjusted size 6x22, spring: 6x28)
void draw_tree(uint16_t base_x, uint16_t base_y, uint8_t season, uint8_t hue, uint8_t sat, uint8_t val) {
    // Tree structure: trunk + canopy
    // Trunk (brown)
    uint8_t trunk_width = 6;
    uint8_t trunk_height = (season == 1) ? 28 : 22; // Spring trees are taller
    qp_rect(display, base_x - trunk_width/2, base_y - trunk_height,
            base_x + trunk_width/2, base_y, 20, 200, 100, true);

    // Canopy changes by season
    if (season == 0) { // Winter - bare branches with more detail
        // Draw main upward-reaching branches
        // Left upward branch
        qp_rect(display, base_x - 8, base_y - trunk_height - 10, base_x - 6, base_y - trunk_height - 2, 20, 150, 80, true);
        qp_rect(display, base_x - 12, base_y - trunk_height - 8, base_x - 8, base_y - trunk_height - 6, 20, 150, 80, true);
        // Right upward branch
        qp_rect(display, base_x + 6, base_y - trunk_height - 10, base_x + 8, base_y - trunk_height - 2, 20, 150, 80, true);
        qp_rect(display, base_x + 8, base_y - trunk_height - 8, base_x + 12, base_y - trunk_height - 6, 20, 150, 80, true);

        // Middle upward branches (from mid-trunk)
        qp_rect(display, base_x - 6, base_y - trunk_height - 6, base_x - 4, base_y - trunk_height + 2, 20, 150, 80, true);
        qp_rect(display, base_x + 4, base_y - trunk_height - 6, base_x + 6, base_y - trunk_height + 2, 20, 150, 80, true);

        // Outward angled branches (lower)
        qp_rect(display, base_x - 10, base_y - trunk_height + 4, base_x - 8, base_y - trunk_height + 8, 20, 150, 80, true);
        qp_rect(display, base_x + 8, base_y - trunk_height + 4, base_x + 10, base_y - trunk_height + 8, 20, 150, 80, true);

        // Smaller upward twigs
        qp_rect(display, base_x - 10, base_y - trunk_height - 12, base_x - 9, base_y - trunk_height - 9, 20, 120, 70, true);
        qp_rect(display, base_x + 9, base_y - trunk_height - 12, base_x + 10, base_y - trunk_height - 9, 20, 120, 70, true);
        qp_rect(display, base_x - 3, base_y - trunk_height - 13, base_x - 2, base_y - trunk_height - 10, 20, 120, 70, true);
        qp_rect(display, base_x + 2, base_y - trunk_height - 13, base_x + 3, base_y - trunk_height - 10, 20, 120, 70, true);

        // Side twigs extending from main branches
        qp_rect(display, base_x - 14, base_y - trunk_height - 6, base_x - 12, base_y - trunk_height - 4, 20, 120, 70, true);
        qp_rect(display, base_x + 12, base_y - trunk_height - 6, base_x + 14, base_y - trunk_height - 4, 20, 120, 70, true);

        // Add snow accumulation on branches (thicker and more coverage)
        // Snow on main upward branches (thicker patches)
        qp_rect(display, base_x - 9, base_y - trunk_height - 11, base_x - 5, base_y - trunk_height - 9, 170, 40, 255, true);
        qp_rect(display, base_x + 5, base_y - trunk_height - 11, base_x + 9, base_y - trunk_height - 9, 170, 40, 255, true);

        // Snow on horizontal/angled branch sections (larger)
        qp_rect(display, base_x - 13, base_y - trunk_height - 9, base_x - 7, base_y - trunk_height - 7, 170, 40, 255, true);
        qp_rect(display, base_x + 7, base_y - trunk_height - 9, base_x + 13, base_y - trunk_height - 7, 170, 40, 255, true);

        // Snow on middle branches (thicker)
        qp_rect(display, base_x - 7, base_y - trunk_height - 7, base_x - 3, base_y - trunk_height - 5, 170, 40, 255, true);
        qp_rect(display, base_x + 3, base_y - trunk_height - 7, base_x + 7, base_y - trunk_height - 5, 170, 40, 255, true);

        // Additional snow on mid-trunk branches
        qp_rect(display, base_x - 6, base_y - trunk_height - 3, base_x - 3, base_y - trunk_height - 1, 170, 40, 255, true);
        qp_rect(display, base_x + 3, base_y - trunk_height - 3, base_x + 6, base_y - trunk_height - 1, 170, 40, 255, true);

        // Snow on lower outward branches (larger)
        qp_rect(display, base_x - 11, base_y - trunk_height + 3, base_x - 7, base_y - trunk_height + 5, 170, 40, 255, true);
        qp_rect(display, base_x + 7, base_y - trunk_height + 3, base_x + 11, base_y - trunk_height + 5, 170, 40, 255, true);

        // Additional snow lower down
        qp_rect(display, base_x - 9, base_y - trunk_height + 6, base_x - 7, base_y - trunk_height + 8, 170, 40, 255, true);
        qp_rect(display, base_x + 7, base_y - trunk_height + 6, base_x + 9, base_y - trunk_height + 8, 170, 40, 255, true);

        // Snow patches on twigs (larger and brighter)
        qp_rect(display, base_x - 11, base_y - trunk_height - 13, base_x - 8, base_y - trunk_height - 11, 0, 0, 255, true);
        qp_rect(display, base_x + 8, base_y - trunk_height - 13, base_x + 11, base_y - trunk_height - 11, 0, 0, 255, true);
        qp_rect(display, base_x - 4, base_y - trunk_height - 14, base_x - 1, base_y - trunk_height - 12, 0, 0, 255, true);
        qp_rect(display, base_x + 1, base_y - trunk_height - 14, base_x + 4, base_y - trunk_height - 12, 0, 0, 255, true);

        // Side twig snow
        qp_rect(display, base_x - 15, base_y - trunk_height - 7, base_x - 11, base_y - trunk_height - 5, 170, 40, 255, true);
        qp_rect(display, base_x + 11, base_y - trunk_height - 7, base_x + 15, base_y - trunk_height - 5, 170, 40, 255, true);
    } else if (season == 1) { // Spring - green leaves with pink blossoms
        // Tree shape with green base
        qp_circle(display, base_x, base_y - trunk_height - 7, 15, 85, 220, 200, true); // Light green
        // Add leaf and blossom dots (smaller, mostly pink blossoms)
        for (uint8_t i = 0; i < 9; i++) {
            int8_t offset_x = (i % 3 - 1) * 7;
            int8_t offset_y = (i / 3 - 1) * 7;
            // Make 8 dots pink blossoms, only dot 4 (center) is green leaf
            if (i != 4) {
                qp_circle(display, base_x + offset_x, base_y - trunk_height - 7 + offset_y, 2, 234, 255, 220, true); // Pink blossom (smaller)
            } else {
                qp_circle(display, base_x + offset_x, base_y - trunk_height - 7 + offset_y, 2, 85, 255, 180, true); // Green leaf (smaller)
            }
        }
    } else if (season == 2) { // Summer - cherry tree with cherries
        // Dense green canopy
        qp_circle(display, base_x, base_y - trunk_height - 7, 16, 85, 255, 200, true);       // Center
        qp_circle(display, base_x - 9, base_y - trunk_height - 4, 11, 85, 255, 180, true);  // Left
        qp_circle(display, base_x + 9, base_y - trunk_height - 4, 11, 85, 255, 180, true);  // Right

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
            qp_circle(display, base_x + cherry_offsets[i][0],
                     base_y - trunk_height - 7 + cherry_offsets[i][1],
                     2, 0, 255, 220, true);
        }
    } else { // Fall - orange/red/yellow leaves
        // Tree shape with autumn colors
        qp_circle(display, base_x, base_y - trunk_height - 7, 15, 20, 255, 200, true);      // Orange
        qp_circle(display, base_x - 8, base_y - trunk_height - 4, 10, 10, 255, 220, true);  // Red-orange
        qp_circle(display, base_x + 8, base_y - trunk_height - 4, 10, 30, 255, 200, true);  // Yellow-orange
    }
}

// Helper function to draw a small wooden cabin
void draw_cabin(uint16_t base_x, uint16_t base_y, uint8_t season) {
    // Cabin dimensions
    uint8_t cabin_width = 24;
    uint8_t cabin_height = 18;
    uint8_t roof_height = 10;

    // Main cabin body (brown wood)
    qp_rect(display, base_x - cabin_width/2, base_y - cabin_height,
            base_x + cabin_width/2, base_y, 20, 200, 120, true);

    // Roof (darker brown/grey triangular roof using rectangles)
    // Left side of roof
    for (uint8_t i = 0; i < roof_height; i++) {
        uint8_t roof_y = base_y - cabin_height - i;
        uint8_t roof_left = base_x - (cabin_width/2 + roof_height - i);
        uint8_t roof_right = base_x - (cabin_width/2 - i);
        qp_rect(display, roof_left, roof_y, roof_right, roof_y + 1, 15, 180, 80, true);
    }
    // Right side of roof
    for (uint8_t i = 0; i < roof_height; i++) {
        uint8_t roof_y = base_y - cabin_height - i;
        uint8_t roof_left = base_x + (cabin_width/2 - i);
        uint8_t roof_right = base_x + (cabin_width/2 + roof_height - i);
        qp_rect(display, roof_left, roof_y, roof_right, roof_y + 1, 15, 180, 80, true);
    }
    // Fill the peak gap with a center line
    qp_rect(display, base_x - 7, base_y - cabin_height - roof_height,
            base_x + 7, base_y - cabin_height, 15, 180, 80, true);

    // Door (darker brown)
    uint8_t door_width = 7;
    uint8_t door_height = 10;
    qp_rect(display, base_x - door_width/2, base_y - door_height,
            base_x + door_width/2, base_y, 15, 220, 60, true);

    // Window (light yellow - lit window)
    uint8_t window_size = 6;
    qp_rect(display, base_x + 5, base_y - cabin_height + 5,
            base_x + 5 + window_size, base_y - cabin_height + 5 + window_size, 42, 150, 255, true);

    // Window frame cross (dark brown)
    qp_rect(display, base_x + 7, base_y - cabin_height + 5,
            base_x + 8, base_y - cabin_height + 5 + window_size, 20, 200, 80, true);
    qp_rect(display, base_x + 5, base_y - cabin_height + 8,
            base_x + 5 + window_size, base_y - cabin_height + 9, 20, 200, 80, true);

    // Chimney on roof (brick red/brown)
    uint8_t chimney_width = 4;
    uint8_t chimney_height = 8;
    qp_rect(display, base_x + 5, base_y - cabin_height - roof_height - chimney_height + 2,
            base_x + 5 + chimney_width, base_y - cabin_height - roof_height + 3, 10, 200, 100, true);

    // Smoke from chimney (light grey puffs) - only if not summer
    if (season != 2) {
        qp_circle(display, base_x + 6, base_y - cabin_height - roof_height - chimney_height - 2, 2, 0, 0, 180, true);
        qp_circle(display, base_x + 7, base_y - cabin_height - roof_height - chimney_height - 5, 2, 0, 0, 160, true);
        qp_circle(display, base_x + 8, base_y - cabin_height - roof_height - chimney_height - 8, 2, 0, 0, 140, true);
    }

    // Add snow on roof in winter
    if (season == 0) {
        // Snow on left side of roof
        for (uint8_t i = 0; i < roof_height; i++) {
            uint8_t roof_y = base_y - cabin_height - i;
            uint8_t roof_left = base_x - (cabin_width/2 + roof_height - i);
            uint8_t roof_right = base_x - (cabin_width/2 - i);
            qp_rect(display, roof_left, roof_y - 2, roof_right, roof_y - 1, 170, 40, 255, true);
        }
        // Snow on right side of roof
        for (uint8_t i = 0; i < roof_height; i++) {
            uint8_t roof_y = base_y - cabin_height - i;
            uint8_t roof_left = base_x + (cabin_width/2 - i);
            uint8_t roof_right = base_x + (cabin_width/2 + roof_height - i);
            qp_rect(display, roof_left, roof_y - 2, roof_right, roof_y - 1, 170, 40, 255, true);
        }
    }
}

// Calculate sun/moon position based on time of day
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
        // Draw moon with phase based on day of month (waxing/waning cycle)
        // Moon cycle: ~29.5 days, using day of month as approximation
        // Phase 0 (new moon) -> 7 (first quarter) -> 14-15 (full) -> 22 (last quarter) -> 29 (new)
        // Night ownership: hours 20-23 start a night, hours 0-5 continue that night
        // Hours 0-5 of day N: End of night that started on day N-1 → use day N-1
        // Hours 20-23 of day N: Start of night N → use day N
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
        qp_circle(display, celestial_x, celestial_y, 8, 42, 100, 255, true);

        // Add shadow to create moon phase effect
        if (moon_phase < 14) {
            // Waxing moon (new -> full): shadow on left side, shrinking
            // Phase 0 = fully shadowed, phase 14 = no shadow (full moon)
            if (moon_phase < 7) {
                // New moon to first quarter: shadow covers most/half of left side
                int8_t shadow_offset = -8 + (moon_phase * 2); // -8 to 6
                uint8_t shadow_radius = 8 - (moon_phase / 2); // 8 to 4
                qp_circle(display, celestial_x + shadow_offset, celestial_y, shadow_radius, 0, 0, 20, true);
            } else {
                // First quarter to full: small shadow on left, disappearing
                int8_t shadow_offset = 6 - ((moon_phase - 7) * 2); // 6 to -8
                uint8_t shadow_radius = 5 - ((moon_phase - 7) / 2); // 4 to 0
                if (shadow_radius > 0) {
                    qp_circle(display, celestial_x + shadow_offset, celestial_y, shadow_radius, 0, 0, 20, true);
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
                    qp_circle(display, celestial_x + shadow_offset, celestial_y, shadow_radius, 0, 0, 20, true);
                }
            } else {
                // Last quarter to new: shadow covers half/most of right side
                int8_t shadow_offset = 8 - ((waning_phase - 7) * 2); // 8 to -6
                uint8_t shadow_radius = 5 + ((waning_phase - 7) / 2); // 4 to 7
                qp_circle(display, celestial_x + shadow_offset, celestial_y, shadow_radius, 0, 0, 20, true);
            }
        }
        // Phase 14-15 is full moon - no shadow applied

        // Add stars scattered across the night sky
        uint16_t star_positions[][2] = {
            {20, 15}, {50, 25}, {90, 18}, {110, 30},            // Row 1
            {65, 12}, {100, 22}, {80, 30},                      // Row 2
            {120, 15}, {10, 25}, {28, 20},                      // Row 3
            {85, 8}, {70, 25}, {60, 15}                         // Row 4
        };
        for (uint8_t i = 0; i < 13; i++) {
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
    draw_cabin(105, ground_y, season); // Cabin on the right

    // === SEASONAL WEATHER EFFECTS ===

    if (season == 0) { // Winter - snow falling with clouds
        // Draw winter clouds (light gray, fluffy)
        uint16_t cloud_positions[][2] = {
            {25, 35}, {85, 40}, {110, 30}
        };
        for (uint8_t i = 0; i < 3; i++) {
            // Main cloud body (light gray)
            qp_circle(display, cloud_positions[i][0], cloud_positions[i][1], 8, 0, 0, 160, true);
            qp_circle(display, cloud_positions[i][0] + 9, cloud_positions[i][1] + 2, 6, 0, 0, 160, true);
            qp_circle(display, cloud_positions[i][0] - 7, cloud_positions[i][1] + 2, 6, 0, 0, 160, true);
            qp_circle(display, cloud_positions[i][0] + 4, cloud_positions[i][1] - 3, 5, 0, 0, 150, true);
        }

        // Draw snowflakes
        uint16_t snow_positions[][2] = {
            {15, 50}, {40, 70}, {65, 90}, {85, 60}, {110, 80}, {25, 100}, {55, 120}, {95, 110}, {120, 65}, {10, 45}, {32, 85},
            {48, 105}, {72, 55}, {90, 75}, {105, 95}, {125, 115}, {18, 130}, {35, 62}, {62, 88}, {78, 108}, {98, 72}
        };
        for (uint8_t i = 0; i < 21; i++) {
            // Simple snowflake with cross pattern
            qp_rect(display, snow_positions[i][0], snow_positions[i][1], snow_positions[i][0] + 2, snow_positions[i][1] + 2, 170, 80, 255, true);
            qp_rect(display, snow_positions[i][0] - 2, snow_positions[i][1] + 1, snow_positions[i][0] + 4, snow_positions[i][1] + 1, 170, 80, 255, true);
            qp_rect(display, snow_positions[i][0] + 1, snow_positions[i][1] - 2, snow_positions[i][0] + 1, snow_positions[i][1] + 4, 170, 80, 255, true);
        }

        // Draw snow on the ground
        // Create uneven snow drifts using overlapping shapes
        qp_rect(display, 0, ground_y - 2, 134, ground_y, 0, 0, 240, true); // Base snow layer

        // Add snow drifts with varying heights for natural look (extending upward from ground)
        struct { uint16_t x; uint8_t height; } snow_drifts[] = {
            {0, 2}, {20, 4}, {45, 3}, {70, 5}, {95, 3}, {115, 4}
        };
        for (uint8_t i = 0; i < 6; i++) {
            qp_rect(display, snow_drifts[i].x, ground_y - snow_drifts[i].height, snow_drifts[i].x + 20, ground_y, 170, 40, 255, true);
        }
    } else if (season == 1) { // Spring - butterflies, flowers, and birds
        // Draw birds in the sky (larger V shapes) - 7 birds
        uint16_t bird_positions[][2] = {
            {25, 50}, {60, 40}, {90, 70}, {110, 45}, {40, 75}, {150, 65}
        };
        for (uint8_t i = 0; i < 6; i++) {
            // Left wing (larger)
            qp_rect(display, bird_positions[i][0] - 5, bird_positions[i][1], bird_positions[i][0] - 1, bird_positions[i][1] - 3, 0, 0, 100, true);
            // Right wing (larger)
            qp_rect(display, bird_positions[i][0] + 1, bird_positions[i][1] - 3, bird_positions[i][0] + 5, bird_positions[i][1], 0, 0, 100, true);
        }

        // More butterflies, lower to the ground
        struct { uint16_t x; uint16_t y; uint8_t hue; } butterflies[] = {
            {20, 115, 234}, {45, 125, 170}, {65, 120, 42}, {85, 130, 200}, {105, 118, 10},
            {125, 135, 234}, {35, 128, 85}, {75, 122, 42}, {95, 133, 170}
            // Pink, cyan, yellow, magenta, red, pink, green, yellow, cyan
        };
        for (uint8_t i = 0; i < 9; i++) {
            qp_circle(display, butterflies[i].x - 2, butterflies[i].y, 2, butterflies[i].hue, 255, 200, true);
            qp_circle(display, butterflies[i].x + 2, butterflies[i].y, 2, butterflies[i].hue, 255, 200, true);
        }

        // Draw flowers on the ground (various colors and sizes)
        struct { uint16_t x; uint8_t hue; uint8_t size; uint8_t stem_height; } flowers[] = {
            {15, 234, 3, 5}, {28, 0, 4, 6}, {42, 42, 3, 5}, {58, 170, 5, 7}, {72, 200, 3, 5},
            {88, 10, 4, 6}, {102, 85, 3, 5}, {118, 234, 5, 7}, {25, 42, 4, 6}, {50, 200, 3, 5},
            {80, 0, 5, 7}, {95, 170, 3, 5}, {110, 234, 4, 6}, {35, 10, 5, 7}, {65, 42, 3, 5}
            // Various spring colors and sizes
        };
        for (uint8_t i = 0; i < 15; i++) {
            // Flower stem (green)
            qp_rect(display, flowers[i].x, ground_y - flowers[i].stem_height, flowers[i].x + 1, ground_y, 85, 200, 150, true);
            // Flower petals (colorful circles with varying sizes)
            qp_circle(display, flowers[i].x, ground_y - flowers[i].stem_height - 2, flowers[i].size, flowers[i].hue, 255, 220, true);
            // Flower center (yellow, size varies with flower)
            qp_circle(display, flowers[i].x, ground_y - flowers[i].stem_height - 2, flowers[i].size / 3, 42, 255, 255, true);
        }
    } else if (season == 2) { // Summer - sunflowers and airplane
        // Draw airplane in top left (more realistic side view)
        uint16_t plane_x = 15;
        uint16_t plane_y = 25;

        // Main fuselage (body) - tapered at nose
        qp_rect(display, plane_x + 3, plane_y + 1, plane_x + 25, plane_y + 4, 0, 0, 180, true);

        // Nose (pointed front)
        qp_rect(display, plane_x + 1, plane_y + 2, plane_x + 3, plane_y + 3, 0, 0, 180, true);

        // Cockpit windows (series of light windows)
        qp_rect(display, plane_x + 20, plane_y + 2, plane_x + 22, plane_y + 3, 170, 80, 240, true);
        qp_rect(display, plane_x + 17, plane_y + 2, plane_x + 19, plane_y + 3, 170, 80, 240, true);

        // Main wings (swept back)
        qp_rect(display, plane_x + 10, plane_y - 3, plane_x + 18, plane_y + 1, 0, 0, 180, true);
        qp_rect(display, plane_x + 10, plane_y + 4, plane_x + 18, plane_y + 8, 0, 0, 180, true);

        // Tail section
        // Vertical stabilizer (tail fin)
        qp_rect(display, plane_x + 3, plane_y - 3, plane_x + 6, plane_y + 1, 0, 0, 180, true);
        // Horizontal stabilizer
        qp_rect(display, plane_x + 3, plane_y + 4, plane_x + 8, plane_y + 6, 0, 0, 180, true);

        // Engine under wing (optional detail)
        qp_circle(display, plane_x + 13, plane_y + 7, 2, 0, 0, 160, true);

        // Draw sunflowers on the ground (tall with large yellow heads)
        struct { uint16_t x; uint8_t stem_height; } sunflowers[] = {
            {22, 13}, {52, 15}, {78, 14}, {102, 12}, {122, 14} // Tall stems
        };
        for (uint8_t i = 0; i < 5; i++) {
            // Sunflower stem (green)
            qp_rect(display, sunflowers[i].x, ground_y - sunflowers[i].stem_height, sunflowers[i].x + 2, ground_y, 85, 200, 150, true);

            // Large sunflower head (bright yellow)
            qp_circle(display, sunflowers[i].x + 1, ground_y - sunflowers[i].stem_height - 3, 5, 42, 255, 255, true);

            // Dark center (brown)
            qp_circle(display, sunflowers[i].x + 1, ground_y - sunflowers[i].stem_height - 3, 2, 20, 200, 100, true);
        }
    } else { // Fall - rain and clouds
        // Draw rain clouds (darker gray clouds)
        uint16_t cloud_positions[][2] = {
            {25, 30}, {70, 40}, {120, 35}
        };
        for (uint8_t i = 0; i < 3; i++) {
            // Main cloud body (dark gray)
            qp_circle(display, cloud_positions[i][0], cloud_positions[i][1], 9, 0, 0, 120, true);
            qp_circle(display, cloud_positions[i][0] + 10, cloud_positions[i][1] + 2, 7, 0, 0, 120, true);
            qp_circle(display, cloud_positions[i][0] - 8, cloud_positions[i][1] + 2, 7, 0, 0, 120, true);
            qp_circle(display, cloud_positions[i][0] + 5, cloud_positions[i][1] - 4, 6, 0, 0, 110, true);
        }

        // Draw rain drops scattered throughout the scene
        // Random distribution from clouds to near ground (50 drops)
        uint16_t rain_positions[][2] = {
            {91, 86}, {25, 128}, {108, 61}, {62, 101}, {45, 74}, {119, 139}, {31, 52}, {76, 118}, {100, 93}, {53, 67},
            {17, 131}, {85, 79}, {69, 105}, {122, 49}, {38, 123}, {96, 84}, {58, 58}, {20, 143}, {106, 71}, {72, 113},
            {41, 96}, {115, 54}, {29, 136}, {83, 88}, {50, 109}, {124, 63}, {64, 121}, {18, 76}, {98, 99}, {56, 56},
            {36, 140}, {88, 82}, {67, 115}, {110, 69}, {42, 127}, {78, 91}, {26, 59}, {102, 103}, {60, 77}, {21, 133},
            {94, 94}, {48, 66}, {116, 51}, {33, 119}, {81, 87}, {52, 106}, {120, 73}, {39, 137}, {75, 98}, {104, 62}
        };
        for (uint8_t i = 0; i < NUM_RAINDROPS; i++) {
            // Use static Y positions directly
            uint16_t y_pos = rain_positions[i][1];

            // Draw raindrops (2 pixels wide, 4 pixels tall)
            uint8_t drop_height = 4;
            if (y_pos < 150) {
                qp_rect(display, rain_positions[i][0], y_pos, rain_positions[i][0] + 1, y_pos + drop_height, 170, 150, 200, true);
            }
        }
        rain_initialized = true;

        // Draw fallen leaves on the ground (just above ground line at y=150)
        struct { uint16_t x; uint8_t hue; } fallen_leaves[] = {
            {18, 10}, {35, 0}, {52, 25}, {68, 15}, {82, 8}, {95, 20}, {108, 5}, {122, 30},
            {25, 12}, {45, 18}, {62, 22}, {78, 28}, {92, 15}, {105, 10}, {118, 25}
            // Orange, red, yellow shades
        };
        for (uint8_t i = 0; i < 15; i++) {
            // Small leaves on ground (small circles just above ground line)
            qp_circle(display, fallen_leaves[i].x, 146, 2, fallen_leaves[i].hue, 255, 220, true);
        }
    }

    // === HALLOWEEN EVENT OVERLAY ===
    // Draw Halloween elements on top of seasonal scene during Halloween period
    if (is_halloween_event()) {
        draw_halloween_elements();
    }

    // === CHRISTMAS ADVENT CALENDAR OVERLAY ===
    // Draw Christmas decorations and Santa during December
    if (is_christmas_season()) {
        draw_christmas_scene();
    }

    // Note: No qp_flush() here - let the caller decide when to flush
}

// === HALLOWEEN EVENT FUNCTIONS ===

// Check if current date is within Halloween event period (Oct 28 - Nov 3)
bool is_halloween_event(void) {
    // Halloween event: 3 days before (Oct 28-30), Halloween (Oct 31), 3 days after (Nov 1-3)
    return (current_month == 10 && current_day >= 28) ||
           (current_month == 11 && current_day <= 3);
}

// Draw a jack-o-lantern pumpkin
void draw_pumpkin(int16_t x, int16_t y, uint8_t size) {
    if (x < -size || x > 135 + size || y < -size || y > 152 + size) return;

    // Pumpkin body (orange circle)
    qp_circle(display, x, y, size, 20, 255, 255, true);  // Bright orange

    // Darker orange shading on bottom
    qp_circle(display, x, y + size/3, size - 2, 16, 255, 220, true);

    // Draw jack-o-lantern face (scale with size)
    uint8_t eye_offset = size / 3;
    uint8_t eye_size = size / 4;

    // Left eye (triangle)
    qp_rect(display, x - eye_offset - eye_size, y - eye_offset,
            x - eye_offset + eye_size, y - eye_offset + eye_size, 0, 0, 0, true);

    // Right eye (triangle)
    qp_rect(display, x + eye_offset - eye_size, y - eye_offset,
            x + eye_offset + eye_size, y - eye_offset + eye_size, 0, 0, 0, true);

    // Nose (small triangle)
    qp_rect(display, x - eye_size/2, y,
            x + eye_size/2, y + eye_size, 0, 0, 0, true);

    // Mouth (jagged grin)
    qp_rect(display, x - size/2, y + size/3,
            x + size/2, y + size/2, 0, 0, 0, true);

    // Teeth (make gaps in the mouth)
    for (int8_t i = -size/3; i < size/3; i += size/4) {
        qp_rect(display, x + i, y + size/3,
                x + i + size/6, y + size/2 - 1, 20, 255, 255, true);
    }

    // Stem (brown/green)
    qp_rect(display, x - 2, y - size - 3, x + 2, y - size + 1, 85, 200, 100, true);
}

// Draw a ghost (static)
void draw_ghost(int16_t x, int16_t y) {
    if (x < -15 || x > 150 || y < -20 || y > 172) return;

    // Ghost body (white rounded shape)
    // Head (circle)
    qp_circle(display, x, y, 7, 0, 0, 240, true);  // Light grey-white

    // Body (rounded rectangle)
    qp_rect(display, x - 7, y, x + 7, y + 12, 0, 0, 240, true);

    // Wavy bottom edge (static)
    qp_rect(display, x - 7, y + 10, x - 4, y + 13, 0, 0, 240, true);
    qp_rect(display, x - 3, y + 10, x + 0, y + 12, 0, 0, 240, true);
    qp_rect(display, x + 1, y + 10, x + 4, y + 13, 0, 0, 240, true);
    qp_rect(display, x + 5, y + 10, x + 7, y + 12, 0, 0, 240, true);

    // Eyes (black dots)
    qp_rect(display, x - 3, y - 2, x - 1, y, 0, 0, 0, true);  // Left eye
    qp_rect(display, x + 1, y - 2, x + 3, y, 0, 0, 0, true);  // Right eye

    // Mouth (small O shape)
    qp_circle(display, x, y + 3, 2, 0, 0, 0, false);
}

// Draw all Halloween elements (static decorations)
void draw_halloween_elements(void) {
    // Draw 2 ghosts in the sky at fixed positions
    draw_ghost(20, 90);   // Left ghost
    draw_ghost(40, 50);   // middle ghost
    draw_ghost(95, 60);   // Right ghost

    // Draw 4 pumpkins on the ground (y=150 is ground level)
    draw_pumpkin(25, 145, 8);   // Left pumpkin (small)
    draw_pumpkin(55, 143, 10);  // Left-center pumpkin (medium)
    draw_pumpkin(90, 144, 9);  // Right pumpkin (medium-small)
}

// === CHRISTMAS ADVENT CALENDAR FUNCTIONS ===

// Check if we're in December (Christmas season)
bool is_christmas_season(void) {
    return current_month == 12;
}

// Get number of Christmas items to show based on current day
uint8_t get_christmas_items_to_show(void) {
    if (!is_christmas_season()) return 0;
    if (current_day >= 25) return NUM_CHRISTMAS_ITEMS;  // Show all items from Dec 25 onwards
    if (current_day >= 1 && current_day <= 24) return current_day;  // Progressive reveal
    return 0;
}

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

// Draw a specific Christmas item
void draw_christmas_item(christmas_item_type_t type, int16_t x, int16_t y) {
    switch (type) {
        case XMAS_PRESENT_RED:
        case XMAS_PRESENT_GREEN:
        case XMAS_PRESENT_BLUE: {
            // Gift box with bow
            uint8_t hue = (type == XMAS_PRESENT_RED) ? 0 : (type == XMAS_PRESENT_GREEN) ? 85 : 170;
            qp_rect(display, x - 4, y - 4, x + 4, y + 4, hue, 255, 200, true);  // Box
            qp_rect(display, x - 4, y - 1, x + 4, y + 1, 42, 200, 255, true);   // Ribbon horizontal
            qp_rect(display, x - 1, y - 4, x + 1, y + 4, 42, 200, 255, true);   // Ribbon vertical
            qp_rect(display, x - 2, y - 6, x + 2, y - 4, 42, 200, 255, true);   // Bow
            break;
        }
        case XMAS_CANDY_CANE: {
            // Red and white striped candy cane
            qp_rect(display, x, y - 8, x + 2, y, 0, 255, 255, true);            // Stick (red)
            qp_rect(display, x, y - 11, x + 5, y - 9, 0, 255, 255, true);       // Hook (red)
            qp_rect(display, x, y - 6, x + 2, y - 4, 0, 0, 255, true);          // White stripe 1
            qp_rect(display, x, y - 2, x + 2, y, 0, 0, 255, true);              // White stripe 2
            qp_rect(display, x + 3, y - 11, x + 5, y - 10, 0, 0, 255, true);    // White stripe on hook
            break;
        }
        case XMAS_STOCKING: {
            // Christmas stocking
            qp_rect(display, x - 3, y - 8, x + 2, y - 2, 0, 255, 220, true);    // Stocking body (red)
            qp_rect(display, x - 2, y - 2, x + 4, y, 0, 255, 220, true);        // Foot
            qp_rect(display, x - 3, y - 9, x + 2, y - 8, 0, 0, 255, true);      // White trim
            break;
        }
        case XMAS_ORNAMENT_RED:
        case XMAS_ORNAMENT_GOLD:
        case XMAS_ORNAMENT_BLUE: {
            // Christmas ornament ball
            uint8_t hue = (type == XMAS_ORNAMENT_RED) ? 0 : (type == XMAS_ORNAMENT_GOLD) ? 42 : 170;
            qp_circle(display, x, y, 4, hue, 255, 255, true);                   // Ball
            qp_rect(display, x - 1, y - 5, x + 1, y - 4, 0, 0, 180, true);      // Hanger
            break;
        }
        case XMAS_BELL: {
            // Golden bell
            qp_rect(display, x - 3, y - 2, x + 3, y + 2, 42, 255, 255, true);   // Bell body
            qp_rect(display, x - 4, y - 3, x + 4, y - 2, 42, 255, 255, true);   // Bell top
            qp_circle(display, x, y + 3, 1, 42, 255, 200, true);                // Clapper
            break;
        }
        case XMAS_HOLLY: {
            // Holly leaves with berries
            qp_rect(display, x - 4, y - 1, x + 4, y + 1, 85, 255, 180, true);   // Leaves
            qp_circle(display, x - 3, y - 2, 1, 0, 255, 255, true);             // Berry 1
            qp_circle(display, x + 3, y - 2, 1, 0, 255, 255, true);             // Berry 2
            break;
        }
        case XMAS_STAR_SMALL: {
            // 5-pointed star (gold)
            qp_rect(display, x - 1, y - 3, x + 1, y + 3, 42, 255, 255, true);   // Vertical
            qp_rect(display, x - 3, y - 1, x + 3, y + 1, 42, 255, 255, true);   // Horizontal
            qp_rect(display, x - 2, y - 2, x + 2, y + 2, 42, 255, 255, true);   // Diagonal cross
            break;
        }
        case XMAS_SNOWFLAKE: {
            // Snowflake
            qp_rect(display, x, y - 4, x, y + 4, 170, 100, 255, true);          // Vertical
            qp_rect(display, x - 4, y, x + 4, y, 170, 100, 255, true);          // Horizontal
            qp_rect(display, x - 3, y - 3, x + 3, y + 3, 170, 100, 255, true);  // Diagonal
            qp_rect(display, x - 3, y + 3, x + 3, y - 3, 170, 100, 255, true);  // Diagonal
            break;
        }
        case XMAS_CANDLE: {
            // Candle with flame
            qp_rect(display, x - 2, y - 8, x + 2, y, 0, 255, 200, true);        // Candle (red)
            qp_rect(display, x - 1, y - 11, x + 1, y - 8, 42, 255, 255, true);  // Flame
            break;
        }
        case XMAS_TREE_SMALL: {
            // Small decorated tree
            qp_rect(display, x - 1, y - 2, x + 1, y, 20, 200, 120, true);       // Trunk
            qp_circle(display, x, y - 5, 4, 85, 255, 180, true);                // Foliage
            qp_circle(display, x - 2, y - 4, 1, 0, 255, 255, true);             // Red ornament
            qp_circle(display, x + 2, y - 6, 1, 42, 255, 255, true);            // Gold ornament
            break;
        }
        case XMAS_GINGERBREAD: {
            // Gingerbread man
            qp_circle(display, x, y - 6, 2, 20, 200, 150, true);                // Head
            qp_rect(display, x - 2, y - 4, x + 2, y + 2, 20, 200, 150, true);   // Body
            qp_rect(display, x - 4, y - 2, x - 2, y, 20, 200, 150, true);       // Left arm
            qp_rect(display, x + 2, y - 2, x + 4, y, 20, 200, 150, true);       // Right arm
            qp_rect(display, x - 2, y + 2, x, y + 4, 20, 200, 150, true);       // Left leg
            qp_rect(display, x, y + 2, x + 2, y + 4, 20, 200, 150, true);       // Right leg
            break;
        }
        case XMAS_WREATH: {
            // Christmas wreath
            qp_circle(display, x, y, 5, 85, 255, 180, false);                   // Green circle
            qp_circle(display, x, y, 4, 85, 255, 180, false);                   // Double outline
            qp_rect(display, x - 2, y + 5, x + 2, y + 7, 0, 255, 255, true);    // Red bow
            break;
        }
        case XMAS_ANGEL: {
            // Angel
            qp_circle(display, x, y - 5, 2, 42, 100, 255, true);                // Halo (gold)
            qp_circle(display, x, y - 2, 2, 0, 0, 240, true);                   // Head (white)
            qp_rect(display, x - 3, y, x + 3, y + 4, 0, 0, 240, true);          // Body (white)
            qp_rect(display, x - 5, y + 1, x - 3, y + 3, 0, 0, 220, true);      // Left wing
            qp_rect(display, x + 3, y + 1, x + 5, y + 3, 0, 0, 220, true);      // Right wing
            break;
        }
        case XMAS_REINDEER_SMALL: {
            // Small reindeer
            qp_circle(display, x, y, 2, 20, 200, 150, true);                    // Body (brown)
            qp_circle(display, x + 2, y - 2, 1, 20, 200, 150, true);            // Head
            qp_rect(display, x + 1, y - 4, x + 2, y - 3, 20, 200, 120, true);   // Antler
            qp_rect(display, x + 3, y - 1, x + 3, y, 0, 255, 255, true);        // Red nose
            break;
        }
        case XMAS_SNOWMAN_SMALL: {
            // Small snowman
            qp_circle(display, x, y - 5, 2, 0, 0, 240, true);                   // Head
            qp_circle(display, x, y - 1, 3, 0, 0, 240, true);                   // Body
            qp_rect(display, x - 1, y - 5, x + 1, y - 5, 0, 0, 0, true);        // Eyes
            qp_rect(display, x - 3, y - 6, x + 3, y - 6, 20, 200, 100, true);   // Hat
            break;
        }
        case XMAS_LIGHTS: {
            // String of Christmas lights
            qp_rect(display, x, y, x + 15, y, 0, 0, 100, true);                 // String
            qp_circle(display, x + 2, y + 1, 1, 0, 255, 255, true);             // Red bulb
            qp_circle(display, x + 6, y + 1, 1, 85, 255, 255, true);            // Green bulb
            qp_circle(display, x + 10, y + 1, 1, 170, 255, 255, true);          // Blue bulb
            qp_circle(display, x + 14, y + 1, 1, 42, 255, 255, true);           // Yellow bulb
            break;
        }
        case XMAS_MISTLETOE: {
            // Mistletoe
            qp_circle(display, x, y, 3, 85, 200, 150, true);                    // Green leaves
            qp_circle(display, x - 2, y - 1, 1, 0, 0, 255, true);               // White berry
            qp_circle(display, x + 2, y - 1, 1, 0, 0, 255, true);               // White berry
            break;
        }
        case XMAS_NORTH_STAR: {
            // Large North Star (bright)
            qp_rect(display, x - 1, y - 5, x + 1, y + 5, 42, 255, 255, true);   // Vertical
            qp_rect(display, x - 5, y - 1, x + 5, y + 1, 42, 255, 255, true);   // Horizontal
            qp_rect(display, x - 3, y - 3, x + 3, y + 3, 42, 255, 255, true);   // Diagonal 1
            qp_rect(display, x - 3, y + 3, x + 3, y - 3, 42, 255, 255, true);   // Diagonal 2
            // Add glow
            qp_circle(display, x, y, 6, 42, 150, 200, false);                   // Glow ring
            break;
        }
        case XMAS_SLEIGH_BELL: {
            // Small sleigh bell (gold)
            qp_circle(display, x, y, 2, 42, 255, 255, true);                    // Bell
            qp_rect(display, x - 1, y - 3, x + 1, y - 2, 42, 200, 200, true);   // Top
            break;
        }
        case XMAS_HEART: {
            // Heart ornament (red)
            qp_circle(display, x - 2, y - 2, 2, 0, 255, 255, true);             // Left circle
            qp_circle(display, x + 2, y - 2, 2, 0, 255, 255, true);             // Right circle
            qp_rect(display, x - 3, y - 1, x + 3, y + 2, 0, 255, 255, true);    // Bottom triangle
            break;
        }
    }
}

// Draw advent calendar items based on current day
void draw_christmas_advent_items(void) {
    uint8_t items_to_show = get_christmas_items_to_show();
    for (uint8_t i = 0; i < items_to_show; i++) {
        draw_christmas_item(advent_items[i].type, advent_items[i].x, advent_items[i].y);
    }
}

// Draw Santa's sleigh with reindeer
void draw_santa_sleigh(int16_t x, int16_t y) {
    if (x < -60 || x > 195) return;  // Off screen

    // Reindeer (simplified - 2 reindeer)
    // Leading reindeer
    qp_circle(display, x + 40, y, 3, 20, 200, 150, true);                       // Body
    qp_circle(display, x + 43, y - 2, 2, 20, 200, 150, true);                   // Head
    qp_rect(display, x + 42, y - 5, x + 43, y - 3, 20, 180, 120, true);         // Antler left
    qp_rect(display, x + 44, y - 5, x + 45, y - 3, 20, 180, 120, true);         // Antler right
    qp_circle(display, x + 45, y - 2, 1, 0, 255, 255, true);                    // Red nose (Rudolph!)
    qp_rect(display, x + 38, y + 2, x + 39, y + 4, 20, 200, 130, true);         // Legs
    qp_rect(display, x + 42, y + 2, x + 43, y + 4, 20, 200, 130, true);

    // Second reindeer
    qp_circle(display, x + 25, y + 1, 3, 20, 200, 150, true);                   // Body
    qp_circle(display, x + 28, y - 1, 2, 20, 200, 150, true);                   // Head
    qp_rect(display, x + 27, y - 4, x + 28, y - 2, 20, 180, 120, true);         // Antler
    qp_rect(display, x + 29, y - 4, x + 30, y - 2, 20, 180, 120, true);

    // Reins (connecting reindeer to sleigh)
    qp_rect(display, x + 20, y + 2, x + 40, y + 2, 20, 180, 100, true);         // Rein line

    // Sleigh
    qp_rect(display, x + 5, y + 2, x + 20, y + 8, 0, 255, 220, true);           // Sleigh body (red)
    qp_rect(display, x + 5, y + 8, x + 20, y + 10, 42, 200, 200, true);         // Sleigh runners (gold)
    qp_rect(display, x + 8, y - 2, x + 17, y + 2, 0, 200, 180, true);           // Gift sack

    // Santa
    qp_circle(display, x + 12, y - 2, 2, 20, 150, 255, true);                   // Head (peach)
    qp_rect(display, x + 10, y, x + 14, y + 4, 0, 255, 220, true);              // Body (red suit)
    qp_rect(display, x + 10, y - 1, x + 14, y, 0, 0, 255, true);                // White trim
    qp_circle(display, x + 12, y - 4, 2, 0, 255, 255, true);                    // Hat
    qp_rect(display, x + 11, y - 5, x + 13, y - 4, 0, 0, 255, true);            // Hat pom-pom
}

// Update Santa's sleigh position
void update_santa_animation(void) {
    if (!santa_initialized) {
        santa_x = -60;  // Start from left
        santa_initialized = true;
    }

    // Move Santa across screen from left to right
    santa_x += 2;

    // Reset when off screen
    if (santa_x > 195) {
        santa_x = -60;
    }
}

// Main Christmas scene drawing function
void draw_christmas_scene(void) {
    if (!is_christmas_season()) return;

    // On New Year's Eve (Dec 31), show fireworks instead of Christmas items
    if (is_new_years_eve()) {
        draw_fireworks_scene();
        return;
    }

    // Draw advent calendar items (progressive reveal Dec 1-24, all shown Dec 25+)
    draw_christmas_advent_items();

    // On Christmas Day (Dec 25-30), show Santa flying
    if (current_day >= 25 && current_day < 31) {
        draw_santa_sleigh(santa_x, 40);  // Santa flies at y=40 in the sky
    }
}

// === NEW YEAR'S EVE FIREWORKS FUNCTIONS ===

// Check if it's New Year's Eve
bool is_new_years_eve(void) {
    return current_month == 12 && current_day == 31;
}

// Draw a single static firework burst
void draw_static_firework(int16_t x, int16_t y, uint8_t hue, uint8_t size) {
    // Draw center bright spot
    qp_circle(display, x, y, size / 2, hue, 255, 255, true);

    // Draw burst particles in 8 directions
    for (uint8_t angle = 0; angle < 8; angle++) {
        int8_t dx = 0, dy = 0;
        switch (angle) {
            case 0: dx = size; dy = 0; break;           // Right
            case 1: dx = size; dy = -size; break;       // Up-right
            case 2: dx = 0; dy = -size; break;          // Up
            case 3: dx = -size; dy = -size; break;      // Up-left
            case 4: dx = -size; dy = 0; break;          // Left
            case 5: dx = -size; dy = size; break;       // Down-left
            case 6: dx = 0; dy = size; break;           // Down
            case 7: dx = size; dy = size; break;        // Down-right
        }

        int16_t px = x + dx;
        int16_t py = y + dy;

        // Draw particle
        if (px >= 0 && px < 135 && py >= 0 && py < 152) {
            qp_circle(display, px, py, 2, hue, 255, 220, true);
        }

        // Draw trail between center and particle
        int16_t mid_x = x + (dx / 2);
        int16_t mid_y = y + (dy / 2);
        if (mid_x >= 0 && mid_x < 135 && mid_y >= 0 && mid_y < 152) {
            qp_circle(display, mid_x, mid_y, 1, hue, 200, 180, true);
        }
    }
}

// Draw fireworks scene
void draw_fireworks_scene(void) {
    // Draw 6 colorful static firework bursts at various positions
    // Red, Yellow, Green, Cyan, Blue, Magenta
    const uint8_t colors[] = {0, 42, 85, 128, 170, 200};
    const struct {int16_t x; int16_t y; uint8_t size;} positions[NUM_FIREWORKS] = {
        {30, 50, 12},   // Left top - Red
        {70, 35, 14},   // Center top - Yellow
        {110, 55, 11},  // Right top - Green
        {25, 90, 13},   // Left mid - Cyan
        {95, 80, 15},   // Right mid - Blue
        {60, 105, 12}   // Center lower - Magenta
    };

    for (uint8_t i = 0; i < NUM_FIREWORKS; i++) {
        draw_static_firework(positions[i].x, positions[i].y, colors[i], positions[i].size);
    }

    // Draw "HNY" (Happy New Year) text at bottom in large block letters
    uint16_t text_x = 45;  // Centered
    uint16_t text_y = 130;

    // H
    qp_rect(display, text_x, text_y, text_x + 2, text_y + 12, 42, 255, 255, true);
    qp_rect(display, text_x + 8, text_y, text_x + 10, text_y + 12, 42, 255, 255, true);
    qp_rect(display, text_x, text_y + 5, text_x + 10, text_y + 7, 42, 255, 255, true);

    // N
    text_x += 15;
    qp_rect(display, text_x, text_y, text_x + 2, text_y + 12, 42, 255, 255, true);
    qp_rect(display, text_x + 8, text_y, text_x + 10, text_y + 12, 42, 255, 255, true);
    qp_rect(display, text_x + 2, text_y + 4, text_x + 8, text_y + 8, 42, 255, 255, true);

    // Y
    text_x += 15;
    qp_rect(display, text_x, text_y, text_x + 2, text_y + 6, 42, 255, 255, true);
    qp_rect(display, text_x + 8, text_y, text_x + 10, text_y + 6, 42, 255, 255, true);
    qp_rect(display, text_x + 2, text_y + 6, text_x + 8, text_y + 8, 42, 255, 255, true);
    qp_rect(display, text_x + 4, text_y + 8, text_x + 6, text_y + 12, 42, 255, 255, true);
}

// Function to draw logo with color based on layer
void set_layer_background(uint8_t layer) {
    // Check if this is a forced full redraw (current_display_layer was set to 255)
    bool force_full_redraw = (current_display_layer == 255);

    // Only update if the layer actually changed or forced redraw
    if (!force_full_redraw && layer == current_display_layer) {
        return;
    }
    current_display_layer = layer;

    // Get layer color for dynamic elements (date/time, media, volume)
    uint8_t hue, sat, val;
    get_layer_color(layer, &hue, &sat, &val);

    // If full redraw is forced, clear screen and redraw everything
    if (force_full_redraw) {
        // Clear entire screen
        qp_rect(display, 0, 0, 134, 239, 0, 0, 0, true);

        // Draw the logo in teal (always the same color)
        draw_amboss_logo(display, 7, 10, 128, 255, 255);

        // Draw seasonal animation between logo and date
        draw_seasonal_animation();
    }

    // Always redraw dynamic elements (they change color with layer)
    // Each of these functions clears its own area before drawing

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

                // Update tracking variables for hour/day change detection
                last_hour = current_hour;
                last_day = current_day;

                // Force full redraw of scene (season and sun/moon position depend on time)
                current_display_layer = 255;  // Invalidate layer cache
                update_display_for_layer();
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

    // Check if hour or day changed (for seasonal animation updates)
    bool hour_changed = (current_hour != last_hour);
    bool day_changed = (current_day != last_day);

    // Update date/time display once per minute (or when time is received)
    if (time_received && (current_time - last_uptime_update >= 60000)) {
        last_uptime_update = current_time;
        // Increment minute (host will send updated time periodically)
        current_minute++;
        if (current_minute >= 60) {
            current_minute = 0;
            current_hour++;
            hour_changed = true;
            if (current_hour >= 24) {
                current_hour = 0;
                current_day++;
                day_changed = true;

                // Handle day rollover with proper month boundaries
                uint8_t days_in_month = 31; // Default
                if (current_month == 2) {
                    // February: check for leap year
                    bool is_leap = (current_year % 4 == 0 && current_year % 100 != 0) || (current_year % 400 == 0);
                    days_in_month = is_leap ? 29 : 28;
                } else if (current_month == 4 || current_month == 6 || current_month == 9 || current_month == 11) {
                    days_in_month = 30; // April, June, September, November
                }

                if (current_day > days_in_month) {
                    current_day = 1;
                    current_month++;
                    if (current_month > 12) {
                        current_month = 1;
                        current_year++;
                    }
                    // Force full redraw when month changes (season may change)
                    current_display_layer = 255;
                    update_display_for_layer();
                    needs_flush = false; // Already flushed by update_display_for_layer
                }
            }
        }
        if (needs_flush) {
            draw_date_time();
        }
        needs_flush = true;
    }

    // Redraw seasonal animation when hour or day changes (sun/moon position, moon phase)
    if (hour_changed || day_changed) {
        draw_seasonal_animation();
        last_hour = current_hour;
        last_day = current_day;
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

    // Handle Santa sleigh animation (on Christmas Day Dec 25 and after)
    if (is_christmas_season() && current_day >= 25) {
        if (current_time - santa_animation_timer >= SANTA_ANIMATION_SPEED) {
            santa_animation_timer = current_time;
            update_santa_animation();
            // Redraw seasonal animation to show updated Santa position
            draw_seasonal_animation();
            needs_flush = true;
        }
    } else {
        // Reset Santa state when not Christmas Day
        if (santa_initialized) {
            santa_initialized = false;
        }
    }

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
