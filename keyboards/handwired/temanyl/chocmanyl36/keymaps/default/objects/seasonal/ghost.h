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

// Ghost dimensions
#define GHOST_WIDTH 15   // Total width (±7 from center)
#define GHOST_HEIGHT 21  // Total height (from -7 to +13)

// Ghost object represents a floating, animated ghost
typedef struct {
    int16_t x;        // Center X position
    int16_t y;        // Center Y position
    int8_t  vx;       // Horizontal velocity (negative for left, positive for right)
    int8_t  vy;       // Vertical velocity (for floating effect, not currently used)
    uint8_t phase;    // Animation phase for floating motion (0-159)
} ghost_t;

// Initialize a ghost object with position and initial velocity
void ghost_init(ghost_t* ghost, int16_t x, int16_t y, int8_t vx, uint8_t phase);

// Draw the ghost at its current position
void ghost_draw(const ghost_t* ghost);

// Check if a point is inside the ghost's bounds
bool ghost_contains_point(const ghost_t* ghost, int16_t px, int16_t py);

// Get the ghost's bounding box (for region-based rendering)
void ghost_get_bounds(const ghost_t* ghost, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);
