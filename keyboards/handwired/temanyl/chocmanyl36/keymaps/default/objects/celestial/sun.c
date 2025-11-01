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

#include "sun.h"
#include "framebuffer.h"

// Initialize sun
void sun_init(sun_t *sun, int16_t x, int16_t y, uint8_t hour) {
    sun->x = x;
    sun->y = y;
    sun->hour = hour;
}

// Draw sun with rays
void sun_draw(const sun_t *sun) {
    // Determine sun color based on time of day
    uint8_t sun_hue, sun_sat;
    if (sun->hour < 8 || sun->hour > 17) {
        // Dawn/dusk - orange/red sun
        sun_hue = 10;
        sun_sat = 255;
    } else {
        // Midday - bright yellow sun
        sun_hue = 42;
        sun_sat = 255;
    }

    // Draw sun circle
    fb_circle_hsv(sun->x, sun->y, 9, sun_hue, sun_sat, 255, true);

    // Add sun rays (8 rays around sun)
    for (uint8_t i = 0; i < 8; i++) {
        int16_t ray_x = 0, ray_y = 0;

        if (i == 0) { ray_x = 12; ray_y = 0; }
        else if (i == 1) { ray_x = 9; ray_y = -9; }
        else if (i == 2) { ray_x = 0; ray_y = -12; }
        else if (i == 3) { ray_x = -9; ray_y = -9; }
        else if (i == 4) { ray_x = -12; ray_y = 0; }
        else if (i == 5) { ray_x = -9; ray_y = 9; }
        else if (i == 6) { ray_x = 0; ray_y = 12; }
        else if (i == 7) { ray_x = 9; ray_y = 9; }

        fb_rect_hsv(sun->x + ray_x - 1, sun->y + ray_y - 1,
                sun->x + ray_x + 1, sun->y + ray_y + 1, sun_hue, sun_sat, 200, true);
    }
}
