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

// Cloud structure
typedef struct {
    int16_t x;
    int16_t y;
    int8_t  vx;  // Horizontal velocity (negative for right-to-left)
} cloud_t;

// Cloud type for different appearances
typedef enum {
    CLOUD_TYPE_LIGHT,       // Light winter/snow clouds
    CLOUD_TYPE_DARK_LIGHT,  // Light rain clouds (light gray)
    CLOUD_TYPE_DARK_MEDIUM, // Medium rain clouds (medium gray)
    CLOUD_TYPE_DARK_HEAVY   // Heavy rain clouds (dark gray)
} cloud_type_t;

// Cloud functions
void cloud_init(cloud_t* cloud, int16_t x, int16_t y, int8_t vx);
void cloud_draw(const cloud_t* cloud, cloud_type_t type);
void cloud_get_bounds(const cloud_t* cloud, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);
bool cloud_contains_point(const cloud_t* cloud, int16_t px, int16_t py);
