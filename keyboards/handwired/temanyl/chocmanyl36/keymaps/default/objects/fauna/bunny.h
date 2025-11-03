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

#define BUNNY_WIDTH 10
#define BUNNY_HEIGHT 12

// Bunny animation state (follows unified object interface)
typedef struct {
    float x;                // Current X position
    float y;                // Current Y position (changes with hop)
    float base_y;           // Ground position
    float velocity_x;       // Horizontal speed
    float velocity_y;       // Vertical velocity (for hopping)
    float hop_phase;        // Phase in hop cycle (0.0 to 1.0)
    bool is_hopping;        // Is currently in hop
    uint8_t animation_frame; // Ear wiggle animation frame
    uint32_t last_hop;      // Last hop time
    uint32_t last_update;   // Last update time
} bunny_t;

// Initialize a single bunny instance
void bunny_init(bunny_t* bunny, float x, float base_y, float velocity_x, uint32_t last_hop_offset);

// Update a single bunny's animation state
void bunny_update(bunny_t* bunny);

// Draw a single bunny
void bunny_draw(const bunny_t* bunny);

// Get bunny's bounding box
void bunny_get_bounds(const bunny_t* bunny, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);

// Check if a point is inside the bunny's bounds
bool bunny_contains_point(const bunny_t* bunny, int16_t px, int16_t py);
