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

// Tree structure
typedef struct {
    uint16_t base_x;
    uint16_t base_y;
    uint8_t season;  // 0=Winter, 1=Spring, 2=Summer, 3=Fall
    uint8_t hue;     // Color hue (layer-specific)
    uint8_t sat;     // Color saturation
    uint8_t val;     // Color value
} tree_t;

// Initialize tree
void tree_init(tree_t *tree, uint16_t base_x, uint16_t base_y, uint8_t season, uint8_t hue, uint8_t sat, uint8_t val);

// Draw tree with seasonal variations
void tree_draw(const tree_t *tree);
