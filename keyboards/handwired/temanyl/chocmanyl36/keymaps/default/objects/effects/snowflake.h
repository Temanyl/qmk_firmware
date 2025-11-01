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

// Snowflake dimensions (cross pattern with center)
#define SNOWFLAKE_SIZE 6  // Total size of cross pattern

// Snowflake structure
typedef struct {
    int16_t x;
    int16_t y;
} snowflake_t;

// Snowflake functions
void snowflake_init(snowflake_t* flake, int16_t x, int16_t y);
void snowflake_draw(const snowflake_t* flake);
void snowflake_get_bounds(const snowflake_t* flake, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);
bool snowflake_contains_point(const snowflake_t* flake, int16_t px, int16_t py);
