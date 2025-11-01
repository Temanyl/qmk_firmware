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

// Snowman object represents a cute winter snowman
typedef struct {
    int16_t x;      // Center X position
    int16_t y;      // Base Y position (bottom of snowman)
    uint8_t size;   // Base size of the snowman
} snowman_t;

// Initialize a snowman object with position and size
void snowman_init(snowman_t* snowman, int16_t x, int16_t y, uint8_t size);

// Draw the snowman at its current position
void snowman_draw(const snowman_t* snowman);
