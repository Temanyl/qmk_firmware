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

#define BUTTERFLY_WIDTH 12  // Width for region-based updates
#define BUTTERFLY_HEIGHT 12 // Height for region-based updates

// Butterfly animation state (follows unified object interface)
typedef struct {
    float x;                // Current X position
    float y;                // Current Y position
    float base_x;           // Base X position for flutter
    float base_y;           // Base Y position for flutter
    float flutter_phase_x;  // Phase for horizontal flutter
    float flutter_phase_y;  // Phase for vertical flutter
    float amplitude_x;      // Horizontal flutter amplitude (varies per butterfly)
    float amplitude_y;      // Vertical flutter amplitude (varies per butterfly)
    float wander_offset_x;  // Additional offset during wander mode
    float wander_offset_y;  // Additional offset during wander mode
    uint32_t wander_timer;  // Timer for wander state changes
    bool is_wandering;      // Whether currently in wander mode
    uint8_t hue;            // Wing color
    uint8_t wing_frame;     // Wing animation frame (0-3)
    uint32_t last_update;   // Last update time
} butterfly_t;

// Initialize a single butterfly instance
void butterfly_init(butterfly_t* butterfly, float base_x, float base_y, uint8_t hue,
                    float flutter_phase_x, float flutter_phase_y, float amplitude_x, float amplitude_y,
                    uint32_t wander_timer_offset);

// Update a single butterfly's animation state
void butterfly_update(butterfly_t* butterfly);

// Draw a single butterfly
void butterfly_draw(const butterfly_t* butterfly);

// Get butterfly's bounding box
void butterfly_get_bounds(const butterfly_t* butterfly, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);

// Check if a point is inside the butterfly's bounds
bool butterfly_contains_point(const butterfly_t* butterfly, int16_t px, int16_t py);
