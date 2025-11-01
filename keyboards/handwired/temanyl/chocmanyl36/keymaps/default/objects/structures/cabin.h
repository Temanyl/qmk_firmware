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

// Cabin structure
typedef struct {
    uint16_t base_x;
    uint16_t base_y;
    uint8_t season;  // 0=Winter, 1=Spring, 2=Summer, 3=Fall
} cabin_t;

// Initialize cabin
void cabin_init(cabin_t *cabin, uint16_t base_x, uint16_t base_y, uint8_t season);

// Draw cabin with seasonal variations
void cabin_draw(const cabin_t *cabin);
