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

#define NUM_EASTER_BUNNIES 1
#define BUNNY_WIDTH 10
#define BUNNY_HEIGHT 12

// Bunny animation state
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
} bunny_state_t;

// External access to bunny array
extern bunny_state_t bunnies[NUM_EASTER_BUNNIES];

// Initialize bunny animations
void bunnies_init(void);

// Update bunny animations (call from housekeeping)
void bunnies_update(void);

// Draw a single bunny by index
void bunny_draw_single(uint8_t index);

// Draw all Easter bunnies
void bunnies_draw_all(void);

// Reset bunny animations
void bunnies_reset(void);
