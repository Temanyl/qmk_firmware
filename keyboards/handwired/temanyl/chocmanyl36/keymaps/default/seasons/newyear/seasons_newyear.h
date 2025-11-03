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

// New Year's Eve event (Dec 31)
#define NUM_ROCKETS 5  // 5 colorful rockets including yellow
#define ROCKET_ANIMATION_SPEED 120  // Update every 120ms (~8fps)
#define NUM_EXPLOSION_PARTICLES 6  // Reduced for better performance

// Rocket animation states
typedef enum {
    ROCKET_INACTIVE,
    ROCKET_LAUNCHING,
    ROCKET_ASCENDING,
    ROCKET_EXPLODING,
    ROCKET_FADING
} rocket_state_t;

// Explosion particle structure (optimized - use int16_t instead of float where possible)
typedef struct {
    int16_t x;          // Current position (scaled by 16 for sub-pixel precision)
    int16_t y;
    int16_t vx;         // X velocity (scaled by 16)
    int16_t vy;         // Y velocity (scaled by 16)
    uint8_t brightness;
} explosion_particle_t;

// Rocket structure
typedef struct {
    int16_t x;                                        // Current x position
    int16_t y;                                        // Current y position
    int16_t launch_x;                                 // Launch position
    int16_t target_y;                                 // Explosion height
    uint8_t hue;                                      // Color
    rocket_state_t state;                             // Current state
    uint32_t state_timer;                             // Time in current state
    explosion_particle_t particles[NUM_EXPLOSION_PARTICLES];  // Explosion particles
} rocket_t;

// External state
extern bool rockets_initialized;
extern bool newyear_scene_drawn;  // Track if static scene has been drawn
extern uint32_t rocket_animation_timer;
extern rocket_t rockets[NUM_ROCKETS];

// New Year's Eve functions
bool is_new_years_eve(void);
void draw_newyear_elements(void);
void init_rockets(void);
void update_rocket_animation(void);
void draw_rocket(rocket_t *rocket);
void reset_newyear_animations(void);
