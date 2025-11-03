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

#define FIREFLY_WIDTH 8   // Width for region-based updates (includes glow)
#define FIREFLY_HEIGHT 8  // Height for region-based updates (includes glow)

// Firefly animation state (follows unified object interface)
typedef struct {
    float x;                // Current X position
    float y;                // Current Y position
    float base_x;           // Base X position
    float base_y;           // Base Y position
    float drift_phase_x;    // Phase for horizontal drift
    float drift_phase_y;    // Phase for vertical drift
    bool is_lit;            // Whether firefly is currently glowing
    uint32_t blink_timer;   // Timer for next blink state change
    uint32_t next_blink_duration;  // How long until next state change
    uint8_t brightness;     // Current brightness (for fade in/out)
    uint32_t last_update;   // Last update time
} firefly_t;

// Initialize a single firefly instance
void firefly_init(firefly_t* firefly, float base_x, float base_y, float drift_phase_x, float drift_phase_y, uint32_t blink_offset);

// Update a single firefly's animation state
void firefly_update(firefly_t* firefly);

// Draw a single firefly (only if lit)
void firefly_draw(const firefly_t* firefly);

// Get firefly's bounding box
void firefly_get_bounds(const firefly_t* firefly, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);

// Check if a point is inside the firefly's bounds
bool firefly_contains_point(const firefly_t* firefly, int16_t px, int16_t py);
