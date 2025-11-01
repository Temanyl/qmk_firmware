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

// Pumpkin object represents a jack-o'-lantern
typedef struct {
    int16_t x;      // Center X position
    int16_t y;      // Center Y position
    uint8_t size;   // Radius of the pumpkin
} pumpkin_t;

// Initialize a pumpkin object with position and size
void pumpkin_init(pumpkin_t* pumpkin, int16_t x, int16_t y, uint8_t size);

// Draw the pumpkin at its current position
void pumpkin_draw(const pumpkin_t* pumpkin);
