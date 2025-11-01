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

#include "raindrop.h"
#include "framebuffer.h"

// Raindrop color configuration (HSV)
#define RAINDROP_HUE 170   // 170=cyan/blue
#define RAINDROP_SAT 150   // Moderate saturation for realistic water color
#define RAINDROP_VAL 200   // Fairly bright

// Initialize a raindrop
void raindrop_init(raindrop_t* drop, int16_t x, int16_t y) {
    drop->x = x;
    drop->y = y;
}

// Draw a raindrop at its current position
void raindrop_draw(const raindrop_t* drop) {
    int16_t x = drop->x;
    int16_t y = drop->y;

    // Bounds check - don't draw if completely off-screen
    if (x < -RAINDROP_WIDTH || x > 135 || y < -RAINDROP_HEIGHT || y > 152) {
        return;
    }

    // Only draw if within visible area
    if (y >= 0 && y < 150) {
        // Draw raindrop as a small vertical rectangle
        fb_rect_hsv(x, y,
                    x + RAINDROP_WIDTH - 1,
                    y + RAINDROP_HEIGHT - 1,
                    RAINDROP_HUE, RAINDROP_SAT, RAINDROP_VAL, true);
    }
}

// Get the raindrop's bounding box
void raindrop_get_bounds(const raindrop_t* drop, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    *x1 = drop->x;
    *y1 = drop->y;
    *x2 = drop->x + RAINDROP_WIDTH - 1;
    *y2 = drop->y + RAINDROP_HEIGHT - 1;
}

// Check if a point is inside the raindrop's bounds
bool raindrop_contains_point(const raindrop_t* drop, int16_t px, int16_t py) {
    return (px >= drop->x &&
            px < drop->x + RAINDROP_WIDTH &&
            py >= drop->y &&
            py < drop->y + RAINDROP_HEIGHT);
}
