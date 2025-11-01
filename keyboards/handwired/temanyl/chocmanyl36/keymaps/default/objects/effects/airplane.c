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

#include "airplane.h"
#include "framebuffer.h"

// Draw airplane in top left
void airplane_draw(void) {
    uint16_t plane_x = 15;
    uint16_t plane_y = 25;

    // Main fuselage (body) - tapered at nose
    fb_rect_hsv(plane_x + 3, plane_y + 1, plane_x + 25, plane_y + 4, 0, 0, 180, true);

    // Nose (pointed front)
    fb_rect_hsv(plane_x + 1, plane_y + 2, plane_x + 3, plane_y + 3, 0, 0, 180, true);

    // Cockpit windows (series of light windows)
    fb_rect_hsv(plane_x + 20, plane_y + 2, plane_x + 22, plane_y + 3, 170, 80, 240, true);
    fb_rect_hsv(plane_x + 17, plane_y + 2, plane_x + 19, plane_y + 3, 170, 80, 240, true);

    // Main wings (swept back)
    fb_rect_hsv(plane_x + 10, plane_y - 3, plane_x + 18, plane_y + 1, 0, 0, 180, true);
    fb_rect_hsv(plane_x + 10, plane_y + 4, plane_x + 18, plane_y + 8, 0, 0, 180, true);

    // Tail section
    // Vertical stabilizer (tail fin)
    fb_rect_hsv(plane_x + 3, plane_y - 3, plane_x + 6, plane_y + 1, 0, 0, 180, true);
    // Horizontal stabilizer
    fb_rect_hsv(plane_x + 3, plane_y + 4, plane_x + 8, plane_y + 6, 0, 0, 180, true);

    // Engine under wing (optional detail)
    fb_circle_hsv(plane_x + 13, plane_y + 7, 2, 0, 0, 160, true);
}
