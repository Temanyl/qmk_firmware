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

#define NUM_SPRING_FLOWERS 15

// Spring flower structure
typedef struct {
    uint16_t x;
    uint8_t hue;
    uint8_t size;
    uint8_t stem_height;
} flower_t;

// Draw all spring flowers on ground
void flowers_draw_all(uint16_t ground_y);
