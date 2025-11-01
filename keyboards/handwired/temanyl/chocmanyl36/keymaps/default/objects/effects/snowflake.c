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

#include "snowflake.h"
#include "framebuffer.h"

// Snowflake positions (x, y)
static const uint16_t snow_positions[NUM_WINTER_SNOWFLAKES][2] = {
    {15, 50}, {40, 70}, {65, 90}, {85, 60}, {110, 80}, {25, 100}, {55, 120}, {95, 110}, {120, 65}, {10, 45}, {32, 85},
    {48, 105}, {72, 55}, {90, 75}, {105, 95}, {125, 115}, {18, 130}, {35, 62}, {62, 88}, {78, 108}, {98, 72}
};

// Draw all winter snowflakes
void snowflakes_draw_all(void) {
    for (uint8_t i = 0; i < NUM_WINTER_SNOWFLAKES; i++) {
        // Center of snowflake
        fb_rect_hsv(snow_positions[i][0], snow_positions[i][1],
                    snow_positions[i][0] + 2, snow_positions[i][1] + 2, 170, 80, 255, true);
        // Horizontal line
        fb_rect_hsv(snow_positions[i][0] - 2, snow_positions[i][1] + 1,
                    snow_positions[i][0] + 4, snow_positions[i][1] + 1, 170, 80, 255, true);
        // Vertical line
        fb_rect_hsv(snow_positions[i][0] + 1, snow_positions[i][1] - 2,
                    snow_positions[i][0] + 1, snow_positions[i][1] + 4, 170, 80, 255, true);
    }
}
