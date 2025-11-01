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

// Sun structure
typedef struct {
    int16_t x;
    int16_t y;
    uint8_t hour;  // Current hour (0-23) for color calculation
} sun_t;

// Initialize sun at position with current hour
void sun_init(sun_t *sun, int16_t x, int16_t y, uint8_t hour);

// Draw sun with rays (color varies by time of day)
void sun_draw(const sun_t *sun);
