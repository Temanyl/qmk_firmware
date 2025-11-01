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

// Spring animation timing
#define SPRING_ANIMATION_SPEED 50  // Update every 50ms for smooth animation

// External state
extern bool spring_initialized;
extern bool spring_background_saved;
extern uint32_t spring_animation_timer;

// Spring functions
void init_spring_animations(void);
void animate_spring(void);
void draw_spring_scene_elements(void);
void reset_spring_animations(void);
