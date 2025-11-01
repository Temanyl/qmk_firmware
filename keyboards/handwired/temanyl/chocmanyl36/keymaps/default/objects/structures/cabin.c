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

#include "cabin.h"
#include "../../display/framebuffer.h"

// Initialize cabin
void cabin_init(cabin_t *cabin, uint16_t base_x, uint16_t base_y, uint8_t season) {
    cabin->base_x = base_x;
    cabin->base_y = base_y;
    cabin->season = season;
}

// Draw cabin with seasonal variations
void cabin_draw(const cabin_t *cabin) {
    uint16_t base_x = cabin->base_x;
    uint16_t base_y = cabin->base_y;
    uint8_t season = cabin->season;

    // Cabin dimensions
    uint8_t cabin_width = 24;
    uint8_t cabin_height = 18;
    uint8_t roof_height = 10;

    // Main cabin body (brown wood)
    fb_rect_hsv(base_x - cabin_width/2, base_y - cabin_height,
            base_x + cabin_width/2, base_y, 20, 200, 120, true);

    // Roof (darker brown/grey triangular roof using rectangles)
    // Left side of roof
    for (uint8_t i = 0; i < roof_height; i++) {
        uint8_t roof_y = base_y - cabin_height - i;
        uint8_t roof_left = base_x - (cabin_width/2 + roof_height - i);
        uint8_t roof_right = base_x - (cabin_width/2 - i);
        fb_rect_hsv(roof_left, roof_y, roof_right, roof_y + 1, 15, 180, 80, true);
    }
    // Right side of roof
    for (uint8_t i = 0; i < roof_height; i++) {
        uint8_t roof_y = base_y - cabin_height - i;
        uint8_t roof_left = base_x + (cabin_width/2 - i);
        uint8_t roof_right = base_x + (cabin_width/2 + roof_height - i);
        fb_rect_hsv(roof_left, roof_y, roof_right, roof_y + 1, 15, 180, 80, true);
    }
    // Fill the peak gap with a center line
    fb_rect_hsv(base_x - 7, base_y - cabin_height - roof_height,
            base_x + 7, base_y - cabin_height, 15, 180, 80, true);

    // Door (darker brown)
    uint8_t door_width = 7;
    uint8_t door_height = 10;
    fb_rect_hsv(base_x - door_width/2, base_y - door_height,
            base_x + door_width/2, base_y, 15, 220, 60, true);

    // Window (light yellow - lit window)
    uint8_t window_size = 6;
    fb_rect_hsv(base_x + 5, base_y - cabin_height + 5,
            base_x + 5 + window_size, base_y - cabin_height + 5 + window_size, 42, 150, 255, true);

    // Window frame cross (dark brown)
    fb_rect_hsv(base_x + 7, base_y - cabin_height + 5,
            base_x + 8, base_y - cabin_height + 5 + window_size, 20, 200, 80, true);
    fb_rect_hsv(base_x + 5, base_y - cabin_height + 8,
            base_x + 5 + window_size, base_y - cabin_height + 9, 20, 200, 80, true);

    // Chimney on roof (brick red/brown)
    uint8_t chimney_width = 4;
    uint8_t chimney_height = 8;
    fb_rect_hsv(base_x + 5, base_y - cabin_height - roof_height - chimney_height + 2,
            base_x + 5 + chimney_width, base_y - cabin_height - roof_height + 3, 10, 200, 100, true);

    // Note: Smoke is animated separately and not drawn here

    // Add snow on roof in winter
    if (season == 0) {
        // Snow on left side of roof
        for (uint8_t i = 0; i < roof_height; i++) {
            uint8_t roof_y = base_y - cabin_height - i;
            uint8_t roof_left = base_x - (cabin_width/2 + roof_height - i);
            uint8_t roof_right = base_x - (cabin_width/2 - i);
            fb_rect_hsv(roof_left, roof_y - 2, roof_right, roof_y - 1, 170, 40, 255, true);
        }
        // Snow on right side of roof
        for (uint8_t i = 0; i < roof_height; i++) {
            uint8_t roof_y = base_y - cabin_height - i;
            uint8_t roof_left = base_x + (cabin_width/2 - i);
            uint8_t roof_right = base_x + (cabin_width/2 + roof_height - i);
            fb_rect_hsv(roof_left, roof_y - 2, roof_right, roof_y - 1, 170, 40, 255, true);
        }
    }
}
