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

#include <stdint.h>
#include <stdbool.h>

// Christmas advent calendar (Dec 1-31)
#define NUM_CHRISTMAS_ITEMS 24
#define SANTA_ANIMATION_SPEED 200  // Update every 200ms (~5fps)

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

// External state
extern bool santa_initialized;
extern uint32_t santa_animation_timer;
extern int16_t santa_x;

// Christmas functions
bool is_christmas_season(void);
uint8_t get_christmas_items_to_show(void);
void draw_christmas_item(christmas_item_type_t type, int16_t x, int16_t y);
void draw_christmas_advent_items(void);
void draw_santa_sleigh(int16_t x, int16_t y);
void update_santa_animation(void);
void draw_christmas_scene(void);
void reset_christmas_animations(void);

