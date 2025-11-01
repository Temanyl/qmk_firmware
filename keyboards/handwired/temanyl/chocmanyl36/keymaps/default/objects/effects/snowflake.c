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
#include "../../display/framebuffer.h"

// Snowflake color configuration (HSV)
#define SNOWFLAKE_HUE 170  // Cyan/blue-white
#define SNOWFLAKE_SAT 80   // Low saturation for white appearance
#define SNOWFLAKE_VAL 255  // Maximum brightness

// Initialize a snowflake
void snowflake_init(snowflake_t* flake, int16_t x, int16_t y) {
    flake->x = x;
    flake->y = y;
}

// Draw a snowflake at its current position (cross pattern)
void snowflake_draw(const snowflake_t* flake) {
    int16_t x = flake->x;
    int16_t y = flake->y;

    // Bounds check - don't draw if completely off-screen
    if (x < -SNOWFLAKE_SIZE || x > 135 || y < -SNOWFLAKE_SIZE || y > 152) {
        return;
    }

    // Only draw if within visible area
    if (y >= 0 && y < 150) {
        // Center of snowflake (2x2 square)
        fb_rect_hsv(x, y, x + 2, y + 2,
                    SNOWFLAKE_HUE, SNOWFLAKE_SAT, SNOWFLAKE_VAL, true);

        // Horizontal line (extends 2 pixels left and right from center)
        fb_rect_hsv(x - 2, y + 1, x + 4, y + 1,
                    SNOWFLAKE_HUE, SNOWFLAKE_SAT, SNOWFLAKE_VAL, true);

        // Vertical line (extends 2 pixels up and down from center)
        fb_rect_hsv(x + 1, y - 2, x + 1, y + 4,
                    SNOWFLAKE_HUE, SNOWFLAKE_SAT, SNOWFLAKE_VAL, true);
    }
}

// Get the snowflake's bounding box
void snowflake_get_bounds(const snowflake_t* flake, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    *x1 = flake->x - 2;
    *y1 = flake->y - 2;
    *x2 = flake->x + 4;
    *y2 = flake->y + 4;
}

// Check if a point is inside the snowflake's bounds
bool snowflake_contains_point(const snowflake_t* flake, int16_t px, int16_t py) {
    return (px >= flake->x - 2 &&
            px < flake->x + 5 &&
            py >= flake->y - 2 &&
            py < flake->y + 5);
}
