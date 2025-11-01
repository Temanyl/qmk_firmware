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

#include "smoke.h"
#include "../../display/framebuffer.h"

// Smoke color configuration (HSV)
#define SMOKE_HUE 0        // 0=red smoke, 0=grey/white smoke (depends on saturation)
#define SMOKE_SAT 0        // 0=white/grey smoke, 255=colored smoke
// Brightness is per-particle and varies (fades as smoke rises)

// Initialize a smoke particle
void smoke_init(smoke_particle_t* smoke, int16_t x, int16_t y, uint8_t size, uint8_t brightness, int8_t drift) {
    smoke->x = x;
    smoke->y = y;
    smoke->size = size;
    smoke->brightness = brightness;
    smoke->age = 0;
    smoke->drift = drift;
}

// Draw the smoke particle at its current position
void smoke_draw(const smoke_particle_t* smoke) {
    // Only draw if active
    if (smoke->brightness == 0) {
        return;
    }

    // Bounds check
    if (smoke->x < -10 || smoke->x > 145 || smoke->y < -10 || smoke->y > 160) {
        return;
    }

    // Draw smoke puff as a soft circle
    fb_circle_hsv(smoke->x, smoke->y, smoke->size, SMOKE_HUE, SMOKE_SAT, smoke->brightness, true);
}

// Check if a point is inside the smoke particle's bounds
bool smoke_contains_point(const smoke_particle_t* smoke, int16_t px, int16_t py) {
    if (smoke->brightness == 0) return false;

    int16_t dx = px - smoke->x;
    int16_t dy = py - smoke->y;
    int16_t radius = smoke->size;

    // Circular bounds check
    return (dx * dx + dy * dy <= radius * radius);
}

// Get the smoke particle's bounding box
void smoke_get_bounds(const smoke_particle_t* smoke, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    *x1 = smoke->x - smoke->size;
    *y1 = smoke->y - smoke->size;
    *x2 = smoke->x + smoke->size;
    *y2 = smoke->y + smoke->size;
}
