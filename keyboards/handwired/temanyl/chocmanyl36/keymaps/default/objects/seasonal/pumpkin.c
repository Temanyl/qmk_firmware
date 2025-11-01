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

#include "pumpkin.h"
#include "../../display/framebuffer.h"

// Initialize a pumpkin object with position and size
void pumpkin_init(pumpkin_t* pumpkin, int16_t x, int16_t y, uint8_t size) {
    pumpkin->x = x;
    pumpkin->y = y;
    pumpkin->size = size;
}

// Draw the pumpkin at its current position
void pumpkin_draw(const pumpkin_t* pumpkin) {
    int16_t x = pumpkin->x;
    int16_t y = pumpkin->y;
    uint8_t size = pumpkin->size;

    // Bounds check - don't draw if completely off-screen
    if (x < -size || x > 135 + size || y < -size || y > 152 + size) {
        return;
    }

    // Main pumpkin body (orange circles)
    fb_circle_hsv(x, y, size, 20, 255, 255, true);
    fb_circle_hsv(x, y + size/3, size - 2, 16, 255, 220, true);

    // Calculate proportional sizes for facial features
    uint8_t eye_offset = size / 3;
    uint8_t eye_size = size / 4;

    // Eyes (triangular, rendered as rectangles)
    fb_rect_hsv(x - eye_offset - eye_size, y - eye_offset,
                x - eye_offset + eye_size, y - eye_offset + eye_size, 0, 0, 0, true);
    fb_rect_hsv(x + eye_offset - eye_size, y - eye_offset,
                x + eye_offset + eye_size, y - eye_offset + eye_size, 0, 0, 0, true);

    // Nose (triangular)
    fb_rect_hsv(x - eye_size/2, y, x + eye_size/2, y + eye_size, 0, 0, 0, true);

    // Mouth (jagged grin)
    fb_rect_hsv(x - size/2, y + size/3, x + size/2, y + size/2, 0, 0, 0, true);

    // Teeth (vertical lines in the mouth)
    for (int8_t i = -size/3; i < size/3; i += size/4) {
        fb_rect_hsv(x + i, y + size/3, x + i + size/6, y + size/2 - 1, 20, 255, 255, true);
    }

    // Stem (green rectangle on top)
    fb_rect_hsv(x - 2, y - size - 3, x + 2, y - size + 1, 85, 200, 100, true);
}
