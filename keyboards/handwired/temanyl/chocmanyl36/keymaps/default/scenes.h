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

// Cloud animation
#define NUM_CLOUDS 5  // 5 clouds total (3 for winter, 5 for fall)
#define CLOUD_ANIMATION_SPEED 100  // Update every 100ms for smooth movement

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  vx;  // Horizontal velocity (negative for right-to-left)
} cloud_t;

extern cloud_t clouds[NUM_CLOUDS];
extern bool cloud_initialized;
extern bool cloud_background_saved;
extern uint32_t cloud_animation_timer;

// Rain animation
#define NUM_RAINDROPS 50
#define RAINDROP_WIDTH 2
#define RAINDROP_HEIGHT 4
#define RAIN_ANIMATION_SPEED 50  // Update every 50ms (20fps)

typedef struct {
    int16_t x;
    int16_t y;
} raindrop_t;

extern bool rain_initialized;
extern bool rain_background_saved;
extern raindrop_t raindrops[NUM_RAINDROPS];
extern uint32_t rain_animation_timer;

// Halloween event (Oct 28 - Nov 3)
#define NUM_PUMPKINS 4
#define NUM_GHOSTS 3
#define GHOST_WIDTH 16
#define GHOST_HEIGHT 16
#define GHOST_ANIMATION_SPEED 80  // Update every 80ms for smooth floating motion

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  vx;  // Horizontal velocity
    int8_t  vy;  // Vertical velocity (for floating effect)
    uint8_t phase; // Animation phase for floating
} ghost_t;

extern ghost_t ghosts[NUM_GHOSTS];
extern bool ghost_initialized;
extern bool ghost_background_saved;
extern uint32_t ghost_animation_timer;

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

extern bool santa_initialized;
extern uint32_t santa_animation_timer;
extern int16_t santa_x;

// New Year's Eve fireworks (Dec 31)
#define NUM_FIREWORKS 6

// Scene drawing functions
void draw_tree(uint16_t base_x, uint16_t base_y, uint8_t season, uint8_t hue, uint8_t sat, uint8_t val);
void draw_cabin(uint16_t base_x, uint16_t base_y, uint8_t season);
void get_celestial_position(uint8_t hour, uint16_t *x, uint16_t *y);
void draw_seasonal_animation(void);
void reset_scene_animations(void);

// Cloud animation
void init_clouds(void);
void draw_cloud(int16_t x, int16_t y);
void animate_clouds(void);

// Rain animation
void animate_raindrops(void);

// Halloween functions
bool is_halloween_event(void);
void draw_pumpkin(int16_t x, int16_t y, uint8_t size);
void draw_ghost(int16_t x, int16_t y);
void draw_halloween_elements(void);
void init_ghosts(void);
void animate_ghosts(void);
bool is_pixel_in_ghost(int16_t px, int16_t py, uint8_t ghost_idx);
void redraw_ghosts_in_region(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

// Christmas functions
bool is_christmas_season(void);
uint8_t get_christmas_items_to_show(void);
void draw_christmas_item(christmas_item_type_t type, int16_t x, int16_t y);
void draw_christmas_advent_items(void);
void draw_santa_sleigh(int16_t x, int16_t y);
void update_santa_animation(void);
void draw_christmas_scene(void);

// New Year's Eve functions
bool is_new_years_eve(void);
void draw_static_firework(int16_t x, int16_t y, uint8_t hue, uint8_t size);
void draw_fireworks_scene(void);

// Utility functions
uint8_t get_season(uint8_t month);
