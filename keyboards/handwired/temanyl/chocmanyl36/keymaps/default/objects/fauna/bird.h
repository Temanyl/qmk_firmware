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

#define BIRD_WIDTH 12   // Width for region-based updates
#define BIRD_HEIGHT 7   // Height for region-based updates

// Bird animation state (follows unified object interface)
typedef struct {
    float x;                // Current X position (floating point for smooth movement)
    float y;                // Current Y position (with vertical bobbing)
    float base_y;           // Base Y position for bobbing
    float velocity_x;       // Horizontal speed
    float bob_phase;        // Phase offset for vertical bobbing
    uint8_t wing_frame;     // Wing animation frame (0-3)
    uint32_t last_update;   // Last update time for animation
} bird_t;

// Initialize a single bird instance
void bird_init(bird_t* bird, float x, float base_y, float velocity_x, float bob_phase);

// Update a single bird's animation state
void bird_update(bird_t* bird);

// Draw a single bird
void bird_draw(const bird_t* bird);

// Get bird's bounding box
void bird_get_bounds(const bird_t* bird, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);

// Check if a point is inside the bird's bounds
bool bird_contains_point(const bird_t* bird, int16_t px, int16_t py);
