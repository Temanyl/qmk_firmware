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

// Smoke animation (shared across seasons)
#define NUM_SMOKE_PARTICLES 20  // Larger pool for time-based emission
#define SMOKE_ANIMATION_SPEED 100  // Update every 100ms for smooth rising smoke
#define SMOKE_SPAWN_INTERVAL_MIN 700   // Minimum spawn interval (0.7 seconds)
#define SMOKE_SPAWN_INTERVAL_MAX 1000  // Maximum spawn interval (1.0 second)

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t size;       // Smoke puff size (grows as it rises)
    uint8_t brightness; // Brightness (fades as it rises) - 0 means inactive
    uint8_t age;        // Age of particle (0-255)
    int8_t  drift;      // Horizontal drift direction
} smoke_particle_t;

extern smoke_particle_t smoke_particles[NUM_SMOKE_PARTICLES];
extern bool smoke_initialized;
extern bool smoke_background_saved;
extern uint32_t smoke_animation_timer;
extern uint32_t smoke_spawn_timer;

// Common scene drawing functions
void draw_tree(uint16_t base_x, uint16_t base_y, uint8_t season, uint8_t hue, uint8_t sat, uint8_t val);
void draw_cabin(uint16_t base_x, uint16_t base_y, uint8_t season);
void get_celestial_position(uint8_t hour, uint16_t *x, uint16_t *y);
void draw_seasonal_animation(void);
void reset_scene_animations(void);

// Smoke animation
void init_smoke(void);
void animate_smoke(void);
bool is_pixel_in_smoke(int16_t px, int16_t py);
void redraw_smoke_in_region(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

// Utility functions
uint8_t get_season(uint8_t month);
