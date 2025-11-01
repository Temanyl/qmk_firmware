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

#include "ghost.h"
#include "framebuffer.h"

// Ghost color configuration (HSV)
#define GHOST_HUE 0        // 0=red, 42=yellow, 85=green, 170=blue, 234=pink
#define GHOST_SAT 0        // 0=white, 255=fully saturated color
#define GHOST_VAL 240      // 0=black, 255=maximum brightness

// Initialize a ghost object with position and initial velocity
void ghost_init(ghost_t* ghost, int16_t x, int16_t y, int8_t vx, uint8_t phase) {
    ghost->x = x;
    ghost->y = y;
    ghost->vx = vx;
    ghost->vy = 0;
    ghost->phase = phase;
}

// Draw the ghost at its current position
void ghost_draw(const ghost_t* ghost) {
    int16_t x = ghost->x;
    int16_t y = ghost->y;

    // Bounds check - don't draw if completely off-screen
    if (x < -15 || x > 150 || y < -20 || y > 172) {
        return;
    }

    // Head (colored circle)
    fb_circle_hsv(x, y, 7, GHOST_HUE, GHOST_SAT, GHOST_VAL, true);

    // Body (colored rectangle)
    fb_rect_hsv(x - 7, y, x + 7, y + 12, GHOST_HUE, GHOST_SAT, GHOST_VAL, true);

    // Bottom wavy edge (four rounded sections)
    fb_rect_hsv(x - 7, y + 10, x - 4, y + 13, GHOST_HUE, GHOST_SAT, GHOST_VAL, true);
    fb_rect_hsv(x - 3, y + 10, x + 0, y + 12, GHOST_HUE, GHOST_SAT, GHOST_VAL, true);
    fb_rect_hsv(x + 1, y + 10, x + 4, y + 13, GHOST_HUE, GHOST_SAT, GHOST_VAL, true);
    fb_rect_hsv(x + 5, y + 10, x + 7, y + 12, GHOST_HUE, GHOST_SAT, GHOST_VAL, true);

    // Eyes (black rectangles)
    fb_rect_hsv(x - 3, y - 2, x - 1, y, 0, 0, 0, true);
    fb_rect_hsv(x + 1, y - 2, x + 3, y, 0, 0, 0, true);

    // Mouth (black circle outline)
    fb_circle_hsv(x, y + 3, 2, 0, 0, 0, false);
}

// Check if a point is inside the ghost's bounds
bool ghost_contains_point(const ghost_t* ghost, int16_t px, int16_t py) {
    int16_t gx = ghost->x;
    int16_t gy = ghost->y;

    // Ghost bounds: x ± 7, y from -7 to +13
    return (px >= gx - 7 && px <= gx + 7 && py >= gy - 7 && py <= gy + 13);
}

// Get the ghost's bounding box
void ghost_get_bounds(const ghost_t* ghost, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    *x1 = ghost->x - 7;
    *y1 = ghost->y - 7;
    *x2 = ghost->x + 7;
    *y2 = ghost->y + 13;
}
