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

#include "snow_drift.h"
#include "../../display/framebuffer.h"

// Snow drift data (x position, height)
static const struct { uint16_t x; uint8_t height; } snow_drifts[] = {
    {0, 2}, {20, 4}, {45, 3}, {70, 5}, {95, 3}, {115, 4}
};

// Draw snow on ground with drifts
void snow_drifts_draw(uint16_t ground_y) {
    // Base snow layer
    fb_rect_hsv(0, ground_y - 2, 134, ground_y, 0, 0, 240, true);

    // Snow drifts
    for (uint8_t i = 0; i < 6; i++) {
        fb_rect_hsv(snow_drifts[i].x, ground_y - snow_drifts[i].height,
                    snow_drifts[i].x + 20, ground_y, 170, 40, 255, true);
    }
}
