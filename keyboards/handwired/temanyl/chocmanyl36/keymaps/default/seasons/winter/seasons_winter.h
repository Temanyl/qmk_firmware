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
#include "objects/effects/snowflake.h"
#include "objects/seasonal/snowman.h"

// Cloud animation
#define NUM_CLOUDS 5  // 5 clouds total
#define CLOUD_ANIMATION_SPEED 200  // Update every 200ms (~5fps)

// Snowflake animation
#define NUM_SNOWFLAKES 40  // Number of animated snowflakes
#define SNOWFLAKE_ANIMATION_SPEED 120  // Update every 120ms (~8fps)

// Snowman
#define NUM_SNOWMEN 1  // Number of snowmen in the scene

// External state
extern cloud_t clouds[NUM_CLOUDS];
extern bool cloud_initialized;
extern bool cloud_background_saved;
extern uint32_t cloud_animation_timer;

extern snowflake_t snowflakes[NUM_SNOWFLAKES];
extern bool snowflake_initialized;
extern bool snowflake_background_saved;
extern uint32_t snowflake_animation_timer;

extern snowman_t snowmen[NUM_SNOWMEN];
extern bool snowman_initialized;

// Winter functions
void init_clouds(void);
void draw_cloud(int16_t x, int16_t y);
void animate_clouds(void);
void animate_snowflakes(void);
void reset_winter_animations(void);
void draw_snow_weather_elements(void);   // Weather-based snow (no snowman/drifts)
void draw_winter_scene_elements(void);   // Seasonal winter (snow + snowman + drifts)
