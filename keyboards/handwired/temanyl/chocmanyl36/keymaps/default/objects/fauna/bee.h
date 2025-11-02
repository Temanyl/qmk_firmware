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

#define NUM_SUMMER_BEES 5
#define BEE_WIDTH 10   // Width for region-based updates
#define BEE_HEIGHT 10  // Height for region-based updates

// Bee animation state
typedef struct {
    float x;                // Current X position
    float y;                // Current Y position
    float center_x;         // Center X position (sunflower head)
    float center_y;         // Center Y position (sunflower head)
    float orbit_phase;      // Phase for circular orbit
    float orbit_radius;     // Distance from center
    float buzz_phase_x;     // Phase for horizontal buzz
    float buzz_phase_y;     // Phase for vertical buzz
    uint8_t wing_frame;     // Wing animation frame (0-1, simple flap)
    uint32_t last_update;   // Last update time
} bee_state_t;

// External access to bee array
extern bee_state_t bees[NUM_SUMMER_BEES];

// Initialize bee animations
void bees_init(void);

// Update bee animations (call from housekeeping)
void bees_update(void);

// Draw a single bee by index
void bee_draw_single(uint8_t index);

// Draw all summer bees
void bees_draw_all(void);

// Reset bee animations
void bees_reset(void);
