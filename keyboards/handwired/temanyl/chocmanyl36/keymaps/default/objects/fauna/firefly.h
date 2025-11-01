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

#define NUM_SUMMER_FIREFLIES 12
#define FIREFLY_WIDTH 8   // Width for region-based updates (includes glow)
#define FIREFLY_HEIGHT 8  // Height for region-based updates (includes glow)

// Firefly animation state
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
} firefly_state_t;

// External access to firefly array
extern firefly_state_t fireflies[NUM_SUMMER_FIREFLIES];

// Initialize firefly animations
void fireflies_init(void);

// Update firefly animations (call from housekeeping)
void fireflies_update(void);

// Draw a single firefly by index (only if lit)
void firefly_draw_single(uint8_t index);

// Draw all summer fireflies (only those that are lit)
void fireflies_draw_all(void);

// Reset firefly animations
void fireflies_reset(void);
