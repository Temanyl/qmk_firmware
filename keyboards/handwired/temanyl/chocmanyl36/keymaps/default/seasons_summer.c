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

#include QMK_KEYBOARD_H
#include "seasons_summer.h"
#include "framebuffer.h"

// Reset summer animations
void reset_summer_animations(void) {
    // No animations to reset for summer (static elements only)
}

// Draw summer-specific scene elements
void draw_summer_scene_elements(void) {
    uint16_t ground_y = 150;

    // Draw airplane in top left (more realistic side view)
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

    // Draw sunflowers on the ground (tall with large yellow heads)
    struct { uint16_t x; uint8_t stem_height; } sunflowers[] = {
        {22, 13}, {52, 15}, {78, 14}, {102, 12}, {122, 14}
    };
    for (uint8_t i = 0; i < 5; i++) {
        // Sunflower stem (green)
        fb_rect_hsv(sunflowers[i].x, ground_y - sunflowers[i].stem_height, sunflowers[i].x + 2, ground_y, 85, 200, 150, true);

        // Large sunflower head (bright yellow)
        fb_circle_hsv(sunflowers[i].x + 1, ground_y - sunflowers[i].stem_height - 3, 5, 42, 255, 255, true);

        // Dark center (brown)
        fb_circle_hsv(sunflowers[i].x + 1, ground_y - sunflowers[i].stem_height - 3, 2, 20, 200, 100, true);
    }
}
