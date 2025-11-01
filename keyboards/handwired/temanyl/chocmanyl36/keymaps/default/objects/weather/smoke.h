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

// Smoke particle structure
typedef struct {
    int16_t x;
    int16_t y;
    uint8_t size;       // Smoke puff size
    uint8_t brightness; // Brightness (0 = inactive)
    uint8_t age;        // Age of particle (0-255)
    int8_t  drift;      // Horizontal drift speed
} smoke_particle_t;

// Smoke particle functions
void smoke_init(smoke_particle_t* smoke, int16_t x, int16_t y, uint8_t size, uint8_t brightness, int8_t drift);
void smoke_draw(const smoke_particle_t* smoke);
bool smoke_contains_point(const smoke_particle_t* smoke, int16_t px, int16_t py);
void smoke_get_bounds(const smoke_particle_t* smoke, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);
