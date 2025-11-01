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

// Rain animation
#define NUM_RAINDROPS 50
#define RAINDROP_WIDTH 2
#define RAINDROP_HEIGHT 4
#define RAIN_ANIMATION_SPEED 50  // Update every 50ms (20fps)

typedef struct {
    int16_t x;
    int16_t y;
} raindrop_t;

// External state
extern bool rain_initialized;
extern bool rain_background_saved;
extern raindrop_t raindrops[NUM_RAINDROPS];
extern uint32_t rain_animation_timer;

// Fall functions
void animate_raindrops(void);
void reset_fall_animations(void);
void draw_fall_scene_elements(void);
