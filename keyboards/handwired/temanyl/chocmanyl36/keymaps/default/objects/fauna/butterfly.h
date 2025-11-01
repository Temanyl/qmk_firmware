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

#define NUM_SPRING_BUTTERFLIES 8
#define BUTTERFLY_WIDTH 12  // Wing radius 3 on each side + body + margin + flutter range
#define BUTTERFLY_HEIGHT 12 // Vertical extent including wing animations + flutter range

// Butterfly animation state
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
} butterfly_state_t;

// External access to butterfly array
extern butterfly_state_t butterflies[NUM_SPRING_BUTTERFLIES];

// Initialize butterfly animations
void butterflies_init(void);

// Update butterfly animations (call from housekeeping)
void butterflies_update(void);

// Draw a single butterfly by index
void butterfly_draw_single(uint8_t index);

// Draw all spring butterflies
void butterflies_draw_all(void);

// Reset butterfly animations
void butterflies_reset(void);
