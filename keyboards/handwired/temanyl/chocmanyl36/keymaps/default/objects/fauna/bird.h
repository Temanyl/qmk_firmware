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

#define NUM_SPRING_BIRDS 6
#define BIRD_WIDTH 12   // Wingspan: 5 left + 1 body + 5 right + margin
#define BIRD_HEIGHT 7   // Vertical extent including wing animations

// Bird animation state
typedef struct {
    float x;                // Current X position (floating point for smooth movement)
    float y;                // Current Y position (with vertical bobbing)
    float base_y;           // Base Y position for bobbing
    float velocity_x;       // Horizontal speed
    float bob_phase;        // Phase offset for vertical bobbing
    uint8_t wing_frame;     // Wing animation frame (0-3)
    uint32_t last_update;   // Last update time for animation
} bird_state_t;

// External access to bird array
extern bird_state_t birds[NUM_SPRING_BIRDS];

// Initialize bird animations
void birds_init(void);

// Update bird animations (call from housekeeping)
void birds_update(void);

// Draw a single bird by index
void bird_draw_single(uint8_t index);

// Draw all spring birds in the sky
void birds_draw_all(void);

// Reset bird animations
void birds_reset(void);
