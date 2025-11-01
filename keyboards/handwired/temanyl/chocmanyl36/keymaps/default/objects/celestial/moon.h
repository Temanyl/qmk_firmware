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

// Moon structure
typedef struct {
    int16_t x;
    int16_t y;
    uint8_t day;   // Current day (1-31) for phase calculation
    uint8_t hour;  // Current hour (0-23) for determining moon day
} moon_t;

// Initialize moon at position with current day and hour
void moon_init(moon_t *moon, int16_t x, int16_t y, uint8_t day, uint8_t hour);

// Draw moon with phase based on day of month
void moon_draw(const moon_t *moon);
