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
#include "objects/weather/cloud.h"

// Cloud animation
#define NUM_CLOUDS 5  // 5 clouds total
#define CLOUD_ANIMATION_SPEED 100  // Update every 100ms for smooth movement

// External state
extern cloud_t clouds[NUM_CLOUDS];
extern bool cloud_initialized;
extern bool cloud_background_saved;
extern uint32_t cloud_animation_timer;

// Winter functions
void init_clouds(void);
void draw_cloud(int16_t x, int16_t y);
void animate_clouds(void);
void reset_winter_animations(void);
void draw_winter_scene_elements(void);
