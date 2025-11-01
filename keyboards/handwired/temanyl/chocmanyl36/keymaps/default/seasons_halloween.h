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

// Halloween event (Oct 28 - Nov 3)
#define NUM_PUMPKINS 4
#define NUM_GHOSTS 3
#define GHOST_WIDTH 16
#define GHOST_HEIGHT 16
#define GHOST_ANIMATION_SPEED 80  // Update every 80ms for smooth floating motion

typedef struct {
    int16_t x;
    int16_t y;
    int8_t  vx;  // Horizontal velocity
    int8_t  vy;  // Vertical velocity (for floating effect)
    uint8_t phase; // Animation phase for floating
} ghost_t;

// External state
extern ghost_t ghosts[NUM_GHOSTS];
extern bool ghost_initialized;
extern bool ghost_background_saved;
extern uint32_t ghost_animation_timer;

// Halloween functions
bool is_halloween_event(void);
void draw_pumpkin(int16_t x, int16_t y, uint8_t size);
void draw_ghost(int16_t x, int16_t y);
void draw_halloween_elements(void);
void init_ghosts(void);
void animate_ghosts(void);
bool is_pixel_in_ghost(int16_t px, int16_t py, uint8_t ghost_idx);
void redraw_ghosts_in_region(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void reset_halloween_animations(void);
