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

#define NUM_EASTER_EGGS 5
#define EASTER_EGG_WIDTH 8
#define EASTER_EGG_HEIGHT 10

// Easter egg pattern types
typedef enum {
    EGG_PATTERN_STRIPES,     // Horizontal stripes
    EGG_PATTERN_DOTS,        // Polka dots
    EGG_PATTERN_ZIGZAG,      // Zigzag pattern
    EGG_PATTERN_SOLID,       // Solid color with border
    EGG_PATTERN_SWIRL        // Swirl pattern
} egg_pattern_t;

// Easter egg state
typedef struct {
    int16_t x;              // X position
    int16_t y;              // Y position (on ground)
    uint8_t base_hue;       // Primary color hue
    uint8_t accent_hue;     // Secondary color hue
    egg_pattern_t pattern;  // Pattern type
    bool hidden;            // Is the egg hidden (for future egg hunt feature)
} easter_egg_t;

// External access to egg array
extern easter_egg_t easter_eggs[NUM_EASTER_EGGS];

// Initialize Easter eggs
void easter_eggs_init(void);

// Draw a single Easter egg by index
void easter_egg_draw_single(uint8_t index);

// Draw all Easter eggs
void easter_eggs_draw_all(void);

// Reset Easter eggs
void easter_eggs_reset(void);
