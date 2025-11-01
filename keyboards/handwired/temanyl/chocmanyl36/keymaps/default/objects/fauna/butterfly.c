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

#include "butterfly.h"
#include "framebuffer.h"

// Butterfly data (x, y, hue)
static const struct { uint16_t x; uint16_t y; uint8_t hue; } butterflies[NUM_SPRING_BUTTERFLIES] = {
    {20, 115, 234}, {45, 125, 170}, {65, 120, 42}, {85, 130, 200}, {105, 118, 10},
    {125, 135, 234}, {35, 128, 85}, {75, 122, 42}, {95, 133, 170}
};

// Draw all spring butterflies
void butterflies_draw_all(void) {
    for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
        // Left wing
        fb_circle_hsv(butterflies[i].x - 2, butterflies[i].y, 2, butterflies[i].hue, 255, 200, true);
        // Right wing
        fb_circle_hsv(butterflies[i].x + 2, butterflies[i].y, 2, butterflies[i].hue, 255, 200, true);
    }
}
