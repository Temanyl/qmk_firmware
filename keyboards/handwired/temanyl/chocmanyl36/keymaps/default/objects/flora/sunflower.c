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

#include "sunflower.h"
#include "framebuffer.h"

// Sunflower data (x position, stem height)
static const struct { uint16_t x; uint8_t stem_height; } sunflowers[NUM_SUMMER_SUNFLOWERS] = {
    {22, 13}, {52, 15}, {78, 14}, {102, 12}, {122, 14}
};

// Draw all summer sunflowers
void sunflowers_draw_all(uint16_t ground_y) {
    for (uint8_t i = 0; i < NUM_SUMMER_SUNFLOWERS; i++) {
        // Sunflower stem (green)
        fb_rect_hsv(sunflowers[i].x, ground_y - sunflowers[i].stem_height,
                    sunflowers[i].x + 2, ground_y, 85, 200, 150, true);

        // Large sunflower head (bright yellow)
        fb_circle_hsv(sunflowers[i].x + 1, ground_y - sunflowers[i].stem_height - 3,
                      5, 42, 255, 255, true);

        // Dark center (brown)
        fb_circle_hsv(sunflowers[i].x + 1, ground_y - sunflowers[i].stem_height - 3,
                      2, 20, 200, 100, true);
    }
}
