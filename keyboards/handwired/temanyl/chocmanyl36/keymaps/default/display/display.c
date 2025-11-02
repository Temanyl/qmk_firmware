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
#include <qp.h>
#include "display.h"
#include "../scenes/scenes.h"
#include "framebuffer.h"
#include "draw_logo.h"
#include "../graphics/helvetica20.qff.c"
#include "../objects/weather/cloud.h"
#include "../objects/weather/smoke.h"
#include "../seasons/winter/seasons_winter.h"
#include "../seasons/spring/seasons_spring.h"
#include "../seasons/summer/seasons_summer.h"
#include "../seasons/fall/seasons_fall.h"
#include "../seasons/halloween/seasons_halloween.h"
#include "../seasons/christmas/seasons_christmas.h"
#include "../seasons/newyear/seasons_newyear.h"
#include "../objects/seasonal/ghost.h"
#include "../objects/celestial/sun.h"
#include "../objects/celestial/moon.h"
#include "../objects/celestial/stars.h"
#include "../objects/structures/tree.h"
#include "../objects/structures/cabin.h"

// Layer enum (from keymap.c)
enum layer_names {
    _MAC_COLEMAK_DH,
    _MAC_CODE,
    _MAC_NAV,
    _MAC_NUM
};

// Display device and font
painter_device_t display;
painter_font_handle_t media_font = NULL;

// Display state tracking
uint8_t current_display_layer = 255;
uint8_t backlight_brightness = 102;
uint32_t last_uptime_update = 0;

// Volume indicator state
uint8_t current_volume = 0;

// Date/time state (configuration now in display.h)
#ifdef HARDCODE_DATE_TIME
uint8_t current_hour = HARDCODED_HOUR;
uint8_t current_minute = HARDCODED_MINUTE;
uint8_t current_day = HARDCODED_DAY;
uint8_t current_month = HARDCODED_MONTH;
uint16_t current_year = HARDCODED_YEAR;
bool time_received = true;  // Mark as received so display renders immediately
uint8_t last_hour = HARDCODED_HOUR;    // Initialize to match hardcoded values to prevent false change detection
uint8_t last_day = HARDCODED_DAY;
#else
uint8_t current_hour = 0;
uint8_t current_minute = 0;
uint8_t current_day = 1;
uint8_t current_month = 1;
uint16_t current_year = 2025;
bool time_received = false;
uint8_t last_hour = 255;
uint8_t last_day = 255;
#endif

// Brightness indicator state
uint8_t last_brightness_value = 102;
uint32_t brightness_display_timer = 0;
bool brightness_display_active = false;

// Media text state
char current_media[64] = "";
bool media_active = false;
uint8_t scroll_position = 0;
uint32_t scroll_timer = 0;
uint8_t text_length = 0;
bool needs_scroll = false;

// Forward declaration
void set_layer_background(uint8_t layer);

// Draw a single 7-segment digit
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

    // Draw segments using QP (lower region y >= 155)
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

// Draw volume bar at bottom of display
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

// Draw brightness indicator overlay
void draw_brightness_indicator(void) {
    // Brightness indicator appears as an overlay at the top of the screen
    // Box dimensions: 100x40 at top center
    uint16_t box_x = 17;   // (135 - 100) / 2
    uint16_t box_y = 10;   // Top of screen
    uint16_t box_w = 100;
    uint16_t box_h = 40;

    // Draw dark grey background box with lighter border
    fb_rect_hsv(box_x, box_y, box_x + box_w, box_y + box_h, 0, 0, 40, true);
    fb_rect_hsv(box_x, box_y, box_x + box_w, box_y + box_h, 0, 0, 150, false);

    // Draw "BRI" text using simple rectangles at top of box
    uint16_t text_y = box_y + 6;
    uint16_t text_x = box_x + 8;

    // B (white text)
    fb_rect_hsv(text_x, text_y, text_x + 1, text_y + 9, 0, 0, 255, true);
    fb_rect_hsv(text_x, text_y, text_x + 6, text_y + 1, 0, 0, 255, true);
    fb_rect_hsv(text_x, text_y + 4, text_x + 5, text_y + 5, 0, 0, 255, true);
    fb_rect_hsv(text_x, text_y + 9, text_x + 6, text_y + 10, 0, 0, 255, true);
    fb_rect_hsv(text_x + 5, text_y + 1, text_x + 7, text_y + 4, 0, 0, 255, true);
    fb_rect_hsv(text_x + 5, text_y + 5, text_x + 7, text_y + 9, 0, 0, 255, true);

    // R (white text)
    text_x += 10;
    fb_rect_hsv(text_x, text_y, text_x + 1, text_y + 9, 0, 0, 255, true);
    fb_rect_hsv(text_x, text_y, text_x + 6, text_y + 1, 0, 0, 255, true);
    fb_rect_hsv(text_x, text_y + 4, text_x + 5, text_y + 5, 0, 0, 255, true);
    fb_rect_hsv(text_x + 5, text_y + 1, text_x + 7, text_y + 4, 0, 0, 255, true);
    fb_rect_hsv(text_x + 4, text_y + 5, text_x + 7, text_y + 9, 0, 0, 255, true);

    // I (white text)
    text_x += 10;
    fb_rect_hsv(text_x, text_y, text_x + 5, text_y + 1, 0, 0, 255, true);
    fb_rect_hsv(text_x + 2, text_y + 1, text_x + 3, text_y + 9, 0, 0, 255, true);
    fb_rect_hsv(text_x, text_y + 9, text_x + 5, text_y + 10, 0, 0, 255, true);

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
    fb_rect_hsv(digit_x + 2, digit_y + 2, digit_x + 4, digit_y + 4, hue, sat, val, true);
    fb_rect_hsv(digit_x + 4, digit_y + 5, digit_x + 6, digit_y + 7, hue, sat, val, true);
    fb_rect_hsv(digit_x + 5, digit_y + 8, digit_x + 7, digit_y + 10, hue, sat, val, true);
    fb_rect_hsv(digit_x + 7, digit_y + 12, digit_x + 9, digit_y + 14, hue, sat, val, true);
    fb_rect_hsv(digit_x + 2, digit_y + 16, digit_x + 4, digit_y + 18, hue, sat, val, true);

    fb_flush(display);
}

// Draw media text with scrolling
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

// Set backlight brightness
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

// Set layer background and redraw dynamic elements
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
        // Reset animation states to allow background to be re-saved
        reset_scene_animations();

        // Clear entire screen
        fb_rect_hsv(0, 0, 134, 239, 0, 0, 0, true);

        // Draw the logo in teal (always the same color)
        draw_amboss_logo(7, 10, 128, 255, 255);

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

    // Flush both framebuffer (upper) and QP (lower)
    fb_flush(display);   // Flush framebuffer scenic area
    qp_flush(display);   // Flush QP info area
}

// Update display based on current layer state
void update_display_for_layer(void) {
    set_layer_background(get_highest_layer(layer_state));
}

// Framebuffer quick test (diagnostic pattern)
void fb_quick_test(void) {
    // Clear screen to black
    fb_clear(FB_COLOR_BLACK);

    // Draw three colored rectangles at top
    fb_rect_hsv(5, 5, 35, 35, 0, 255, 255, true);        // Red (hue 0)
    fb_rect_hsv(45, 5, 75, 35, 85, 255, 255, true);      // Green (hue 85)
    fb_rect_hsv(85, 5, 115, 35, 170, 255, 255, true);    // Blue (hue 170)

    // Draw three circles below squares
    fb_circle_hsv(20, 60, 12, 128, 255, 255, true);      // Teal (hue 128)
    fb_circle_hsv(60, 60, 12, 43, 255, 255, true);       // Yellow (hue 43)
    fb_circle_hsv(100, 60, 12, 213, 255, 255, true);     // Magenta (hue 213)

    // Draw a white crosshair in center
    int16_t cx = FB_WIDTH / 2;
    int16_t cy = FB_HEIGHT / 2;
    fb_line(cx - 20, cy, cx + 20, cy, FB_COLOR_WHITE);   // Horizontal line
    fb_line(cx, cy - 20, cx, cy + 20, FB_COLOR_WHITE);   // Vertical line

    // Draw border around screen
    fb_rect(0, 0, FB_WIDTH - 1, FB_HEIGHT - 1, FB_COLOR_WHITE, false);

    // Draw some diagonal lines in bottom corner
    fb_line(0, 220, 20, 239, FB_COLOR_YELLOW);
    fb_line(20, 220, 40, 239, FB_COLOR_CYAN);
    fb_line(40, 220, 60, 239, FB_COLOR_MAGENTA);

    // Flush everything to display
    fb_flush(display);

    // Wait 2 seconds to see the test pattern
    wait_ms(2000);
}

// Initialize the ST7789 display
void init_display(void) {
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

    // Initialize framebuffer system
    fb_init();

#if FRAMEBUFFER_TEST
    // Run framebuffer quick test (shows test pattern for 2 seconds)
    fb_quick_test();
#endif

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
    fb_rect_hsv(0, 0, 134, 239, 0, 0, 0, true);
    wait_ms(50);

    // Draw the Amboss logo at the top in teal using line-by-line rendering
    // Logo is 120x120, centered horizontally (135-120)/2 = 7.5, positioned at top (y=10)
    draw_amboss_logo(7, 10, 128, 255, 255);  // Teal color

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
    fb_flush(display);   // Flush framebuffer scenic area
    qp_flush(display);   // Flush QP info area
}

// Display housekeeping task - handles all display animations and updates
void display_housekeeping_task(void) {
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
        // Reset background flags to force re-saving with updated scene
        // (sun/moon positions change, so background buffer must be updated)
        spring_background_saved = false;
        rain_background_saved = false;
        ghost_background_saved = false;
        smoke_background_saved = false;
        cloud_background_saved = false;
        snowflake_background_saved = false;

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

    // Get current season for animation handling
    uint8_t season = get_season(current_month);

    // Handle spring animations (birds and butterflies)
    // Note: animate_spring() handles its own region-based flushing
    if (spring_initialized && spring_background_saved) {
        if (season == 1) { // Spring season
            if (current_time - spring_animation_timer >= SPRING_ANIMATION_SPEED) {
                spring_animation_timer = current_time;
                animate_spring();
                // No needs_flush = true here - spring animation flushes its own regions
            }
        }
    }

    // Handle summer animations (bees and fireflies)
    // Note: animate_summer() handles its own region-based flushing
    if (summer_initialized && summer_background_saved) {
        if (season == 2) { // Summer season
            if (current_time - summer_animation_timer >= SUMMER_ANIMATION_SPEED) {
                summer_animation_timer = current_time;
                animate_summer();
                // No needs_flush = true here - summer animation flushes its own regions
            }
        }
    }

    // Handle rain animation (during fall season)
    // Note: animate_raindrops() handles its own region-based flushing
    if (rain_initialized && rain_background_saved) {
        if (season == 3) { // Fall season
            if (current_time - rain_animation_timer >= RAIN_ANIMATION_SPEED) {
                rain_animation_timer = current_time;
                animate_raindrops();
                // No needs_flush = true here - raindrops flush their own regions
            }
        }
    }

    // Handle snowflake animation (during winter season, but not New Year's Eve)
    // Note: animate_snowflakes() handles its own region-based flushing
    if (snowflake_initialized && snowflake_background_saved && !is_new_years_eve()) {
        uint8_t season = get_season(current_month);
        if (season == 0) { // Winter season
            if (current_time - snowflake_animation_timer >= SNOWFLAKE_ANIMATION_SPEED) {
                snowflake_animation_timer = current_time;
                animate_snowflakes();
                // No needs_flush = true here - snowflakes flush their own regions
            }
        }
    }

    // Region-based animation with smart overlap detection
    // Store old positions before updating
    cloud_t old_clouds[NUM_CLOUDS];
    ghost_t old_ghosts[NUM_GHOSTS];
    bool clouds_updated = false;
    bool ghosts_updated = false;
    uint8_t num_active_clouds = (season == 3) ? 5 : 3;

    // Update cloud positions if timer elapsed (but not on New Year's Eve)
    if (cloud_initialized && cloud_background_saved && !is_new_years_eve()) {
        if (current_time - cloud_animation_timer >= CLOUD_ANIMATION_SPEED) {
            // Store old positions
            for (uint8_t i = 0; i < num_active_clouds; i++) {
                old_clouds[i] = clouds[i];
            }
            cloud_animation_timer = current_time;
            animate_clouds();  // Updates positions only
            clouds_updated = true;
        }
    }

    // Update ghost positions if timer elapsed (during Halloween event)
    bool ghosts_active = ghost_initialized && ghost_background_saved && is_halloween_event();
    if (ghosts_active) {
        if (current_time - ghost_animation_timer >= GHOST_ANIMATION_SPEED) {
            // Store old positions
            for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
                old_ghosts[i] = ghosts[i];
            }
            ghost_animation_timer = current_time;
            animate_ghosts();  // Updates positions only
            ghosts_updated = true;
        }
    }

    // Smart region-based rendering with z-ordering
    if (clouds_updated || ghosts_updated) {
        // Track dirty bounds for efficient flushing
        int16_t dirty_x1 = 134, dirty_y1 = 121;
        int16_t dirty_x2 = 0, dirty_y2 = 12;

        // Helper to expand dirty region
        #define EXPAND_DIRTY(x1, y1, x2, y2) do { \
            if ((x1) < dirty_x1) dirty_x1 = (x1); \
            if ((y1) < dirty_y1) dirty_y1 = (y1); \
            if ((x2) > dirty_x2) dirty_x2 = (x2); \
            if ((y2) > dirty_y2) dirty_y2 = (y2); \
        } while(0)

        // Helper to check rectangle overlap
        #define RECTS_OVERLAP(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2) \
            (!((ax2) < (bx1) || (ax1) > (bx2) || (ay2) < (by1) || (ay1) > (by2)))

        // Track which objects need redrawing
        bool redraw_clouds[NUM_CLOUDS] = {false};
        bool redraw_ghosts[NUM_GHOSTS] = {false};

        // Process cloud updates
        if (clouds_updated) {
            for (uint8_t i = 0; i < num_active_clouds; i++) {
                // Cloud bounds: x-16 to x+18, y-11 to y+10 (conservative)
                int16_t old_x1 = old_clouds[i].x - 16;
                int16_t old_y1 = old_clouds[i].y - 11;
                int16_t old_x2 = old_clouds[i].x + 18;
                int16_t old_y2 = old_clouds[i].y + 10;

                // Restore old position
                fb_restore_from_background(old_x1, old_y1, old_x2, old_y2);
                EXPAND_DIRTY(old_x1, old_y1, old_x2, old_y2);

                // Mark this cloud for redraw
                redraw_clouds[i] = true;

                // Check if any ghosts overlap old cloud position
                if (ghosts_active) {
                    for (uint8_t j = 0; j < NUM_GHOSTS; j++) {
                        int16_t gx1 = ghosts[j].x - 7;
                        int16_t gy1 = ghosts[j].y - 7;
                        int16_t gx2 = ghosts[j].x + 7;
                        int16_t gy2 = ghosts[j].y + 13;
                        if (RECTS_OVERLAP(old_x1, old_y1, old_x2, old_y2, gx1, gy1, gx2, gy2)) {
                            redraw_ghosts[j] = true;
                        }
                    }
                }
            }
        }

        // Process ghost updates
        if (ghosts_updated) {
            for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
                // Ghost bounds: x-7 to x+7, y-7 to y+13
                int16_t old_x1 = old_ghosts[i].x - 7;
                int16_t old_y1 = old_ghosts[i].y - 7;
                int16_t old_x2 = old_ghosts[i].x + 7;
                int16_t old_y2 = old_ghosts[i].y + 13;

                // Restore old position
                fb_restore_from_background(old_x1, old_y1, old_x2, old_y2);
                EXPAND_DIRTY(old_x1, old_y1, old_x2, old_y2);

                // Mark this ghost for redraw
                redraw_ghosts[i] = true;

                // Check if any clouds overlap old ghost position
                if (cloud_initialized && cloud_background_saved) {
                    for (uint8_t j = 0; j < num_active_clouds; j++) {
                        int16_t cx1 = clouds[j].x - 16;
                        int16_t cy1 = clouds[j].y - 11;
                        int16_t cx2 = clouds[j].x + 18;
                        int16_t cy2 = clouds[j].y + 10;
                        if (RECTS_OVERLAP(old_x1, old_y1, old_x2, old_y2, cx1, cy1, cx2, cy2)) {
                            redraw_clouds[j] = true;
                        }
                    }
                }
            }
        }

        // Redraw affected objects in z-order (clouds first, then ghosts)
        // Draw clouds (background layer)
        if (cloud_initialized && cloud_background_saved) {
            for (uint8_t i = 0; i < num_active_clouds; i++) {
                if (redraw_clouds[i]) {
                    int16_t x = clouds[i].x;
                    int16_t y = clouds[i].y;
                    if (x >= -30 && x <= 165) {
                        // Expand dirty region for new position
                        EXPAND_DIRTY(x - 16, y - 11, x + 18, y + 10);

                        if (season == 3) {
                            // Fall: darker rain clouds
                            cloud_draw(&clouds[i], CLOUD_TYPE_DARK);
                        } else {
                            // Winter: lighter clouds
                            cloud_draw(&clouds[i], CLOUD_TYPE_LIGHT);
                        }
                    }
                }
            }
        }

        // Draw ghosts (foreground layer)
        if (ghosts_active) {
            for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
                if (redraw_ghosts[i]) {
                    // Expand dirty region for new position
                    EXPAND_DIRTY(ghosts[i].x - 7, ghosts[i].y - 7, ghosts[i].x + 7, ghosts[i].y + 13);
                    ghost_draw(&ghosts[i]);
                }
            }
        }

        // Flush only the dirty region (much smaller than full area)
        if (dirty_x2 >= dirty_x1 && dirty_y2 >= dirty_y1) {
            fb_flush_region(display, dirty_x1, dirty_y1, dirty_x2, dirty_y2);
        }

        #undef EXPAND_DIRTY
        #undef RECTS_OVERLAP
    }

    // Handle smoke animation (all seasons except summer)
    // Note: animate_smoke() handles its own region-based flushing
    if (smoke_initialized && smoke_background_saved) {
        uint8_t season = get_season(current_month);
        if (season != 2) { // Not summer
            if (current_time - smoke_animation_timer >= SMOKE_ANIMATION_SPEED) {
                smoke_animation_timer = current_time;
                animate_smoke();
                // No needs_flush = true here - smoke flushes its own regions
            }
        }
    }

    // Handle Santa sleigh animation (on Christmas Day Dec 25 and after, but not New Year's Eve)
    if (is_christmas_season() && current_day >= 25 && current_day < 31) {
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

    // Handle rocket animation (on New Year's Eve Dec 31)
    if (is_new_years_eve()) {
        if (current_time - rocket_animation_timer >= ROCKET_ANIMATION_SPEED) {
            rocket_animation_timer = current_time;
            update_rocket_animation();
            // OPTIMIZATION: Clear and redraw sky with static elements
            // Clear sky including rocket starting positions (y=0 to y=149)
            // Rockets start at y=148, ground is at y=150, so clear up to y=149
            fb_rect_hsv(0, 0, 134, 149, 170, 255, 30, true);
            // Redraw logo (expensive but necessary - 120x120 at 7,10)
            draw_amboss_logo(7, 10, 128, 255, 255);

            // Redraw static scene elements
            bool is_night = (current_hour >= 20 || current_hour < 6);

            // Get sun/moon position
            uint16_t celestial_x, celestial_y;
            get_celestial_position(current_hour, &celestial_x, &celestial_y);

            // Draw sun or moon
            if (is_night) {
                moon_t moon;
                moon_init(&moon, celestial_x, celestial_y, current_day, current_hour);
                moon_draw(&moon);
                // Draw stars
                stars_draw();
            } else {
                sun_t sun;
                sun_init(&sun, celestial_x, celestial_y, current_hour);
                sun_draw(&sun);
            }

            // Draw ground line
            uint16_t ground_y = 150;
            fb_rect_hsv(0, ground_y, 134, ground_y + 1, 85, 180, 100, true);

            // Draw trees with layer color
            uint8_t layer = get_highest_layer(layer_state);
            uint8_t tree_hue, tree_sat, tree_val;
            get_layer_color(layer, &tree_hue, &tree_sat, &tree_val);

            // Use wrapper function from scenes.c for tree drawing
            tree_t tree1, tree2;
            tree_init(&tree1, 30, ground_y, 0, tree_hue, tree_sat, tree_val);  // season 0 = winter
            tree_draw(&tree1);
            tree_init(&tree2, 67, ground_y, 0, tree_hue, tree_sat, tree_val);
            tree_draw(&tree2);

            // Draw cabin
            cabin_t cabin;
            cabin_init(&cabin, 105, ground_y, 0);  // season 0 = winter
            cabin_draw(&cabin);

            // Redraw smoke particles (if active) to prevent flickering
            if (smoke_initialized) {
                for (uint8_t i = 0; i < NUM_SMOKE_PARTICLES; i++) {
                    if (smoke_particles[i].brightness > 0) {
                        smoke_draw(&smoke_particles[i]);
                    }
                }
            }

            // Draw rockets
            draw_newyear_elements();
            needs_flush = true;
        }
    } else {
        // Reset rocket state when not New Year's Eve
        if (rockets_initialized) {
            reset_newyear_animations();
        }
    }

    // Single flush at the end to batch all updates
    if (needs_flush) {
        fb_flush(display);
    }
}
