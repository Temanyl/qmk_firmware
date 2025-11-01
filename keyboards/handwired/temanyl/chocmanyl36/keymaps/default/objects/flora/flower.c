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

#include "flower.h"
#include "framebuffer.h"

// Flower data
static const flower_t flowers[NUM_SPRING_FLOWERS] = {
    {15, 234, 3, 5}, {28, 0, 4, 6}, {42, 42, 3, 5}, {58, 170, 5, 7}, {72, 200, 3, 5},
    {88, 10, 4, 6}, {102, 85, 3, 5}, {118, 234, 5, 7}, {25, 42, 4, 6}, {50, 200, 3, 5},
    {80, 0, 5, 7}, {95, 170, 3, 5}, {110, 234, 4, 6}, {35, 10, 5, 7}, {65, 42, 3, 5}
};

// Draw all spring flowers
void flowers_draw_all(uint16_t ground_y) {
    for (uint8_t i = 0; i < NUM_SPRING_FLOWERS; i++) {
        // Flower stem (green)
        fb_rect_hsv(flowers[i].x, ground_y - flowers[i].stem_height,
                    flowers[i].x + 1, ground_y, 85, 200, 150, true);
        // Flower petals (colorful circles with varying sizes)
        fb_circle_hsv(flowers[i].x, ground_y - flowers[i].stem_height - 2,
                      flowers[i].size, flowers[i].hue, 255, 220, true);
        // Flower center (yellow, size varies with flower)
        fb_circle_hsv(flowers[i].x, ground_y - flowers[i].stem_height - 2,
                      flowers[i].size / 3, 42, 255, 255, true);
    }
}
