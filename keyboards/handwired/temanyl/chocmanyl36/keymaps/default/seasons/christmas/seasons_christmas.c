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
#include <math.h>
#include "seasons_christmas.h"
#include "../newyear/seasons_newyear.h"
#include "../../display/framebuffer.h"

// Christmas animation state
bool santa_initialized = false;
uint32_t santa_animation_timer = 0;
int16_t santa_x = -60;

// Forward declarations
extern uint8_t current_month;
extern uint8_t current_day;

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

    // Skip drawing on New Year's Eve (handled by newyear season)
    if (is_new_years_eve()) {
        return;
    }

    draw_christmas_advent_items();

    if (current_day >= 25 && current_day < 31) {
        draw_santa_sleigh(santa_x, 40);
    }
}

// Reset Christmas animations
void reset_christmas_animations(void) {
    santa_initialized = false;
}
