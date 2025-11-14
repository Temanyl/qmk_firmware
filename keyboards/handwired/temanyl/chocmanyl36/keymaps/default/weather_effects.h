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
#include "objects/weather/raindrop.h"
#include "objects/effects/snowflake.h"
#include "objects/seasonal/snowman.h"

// Weather effect counts
#define NUM_CLOUDS 5        // Max 5 clouds
#define NUM_RAINDROPS 50    // Max 50 raindrops
#define NUM_SNOWFLAKES 40   // Max 40 snowflakes
#define NUM_SNOWMEN 1       // Max 1 snowman

// Animation speeds
#define CLOUD_ANIMATION_SPEED 200       // Update every 200ms (~5fps)
#define RAIN_ANIMATION_SPEED 100        // Update every 100ms (~10fps)
#define SNOWFLAKE_ANIMATION_SPEED 120   // Update every 120ms (~8fps)

// External state - clouds
extern cloud_t clouds[NUM_CLOUDS];
extern bool cloud_initialized;
extern bool cloud_background_saved;
extern uint32_t cloud_animation_timer;

// External state - rain
extern raindrop_t raindrops[NUM_RAINDROPS];
extern bool rain_initialized;
extern bool rain_background_saved;
extern uint32_t rain_animation_timer;

// External state - snow
extern snowflake_t snowflakes[NUM_SNOWFLAKES];
extern bool snowflake_initialized;
extern bool snowflake_background_saved;
extern uint32_t snowflake_animation_timer;

// External state - snowman
extern snowman_t snowmen[NUM_SNOWMEN];
extern bool snowman_initialized;

// Cloud system functions
void weather_clouds_init(void);
void weather_clouds_animate(void);
void weather_clouds_reset(void);

// Rain system functions
void weather_rain_init(void);
void weather_rain_animate(void);
void weather_rain_reset(void);

// Snow system functions
void weather_snow_init(void);
void weather_snow_animate(void);
void weather_snow_draw_ground_effects(void);
void weather_snow_reset(void);

// Query functions
uint8_t weather_get_active_cloud_count(void);

// Unified reset functions
void weather_effects_reset_all(void);
