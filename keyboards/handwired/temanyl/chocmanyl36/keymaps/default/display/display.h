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

#pragma once

#include <qp.h>
#include <stdint.h>
#include <stdbool.h>

// Display device and font
extern painter_device_t display;
extern painter_font_handle_t media_font;

// Display state tracking
extern uint8_t current_display_layer;
extern uint8_t backlight_brightness;
extern uint32_t last_uptime_update;

// Volume indicator state
extern uint8_t current_volume;

// Date/time state
extern uint8_t current_hour;
extern uint8_t current_minute;
extern uint8_t current_day;
extern uint8_t current_month;
extern uint16_t current_year;
extern bool time_received;
extern uint8_t last_hour;
extern uint8_t last_day;

// Brightness indicator state
extern uint8_t last_brightness_value;
extern uint32_t brightness_display_timer;
extern bool brightness_display_active;
#define BRIGHTNESS_DISPLAY_TIMEOUT 3000

// Media text state
extern char current_media[64];
extern bool media_active;
extern uint8_t scroll_position;
extern uint32_t scroll_timer;
extern uint8_t text_length;
extern bool needs_scroll;
#define SCROLL_SPEED 300
#define SCROLL_PAUSE_START 500
#define MAX_DISPLAY_CHARS 13

// Display drawing functions
void draw_digit(uint16_t x, uint16_t y, uint8_t digit, uint8_t hue, uint8_t sat, uint8_t val);
void get_layer_color(uint8_t layer, uint8_t *hue, uint8_t *sat, uint8_t *val);
void draw_date_time(void);
void draw_volume_bar(uint8_t hue, uint8_t sat, uint8_t val);
void draw_brightness_indicator(void);
void draw_media_text(void);
void update_display_for_layer(void);
void set_backlight_brightness(uint8_t brightness);

// Display initialization
void init_display(void);
void fb_quick_test(void);
