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

#include "bird.h"
#include "../../display/framebuffer.h"

// Bird positions (x, y)
static const uint16_t bird_positions[NUM_SPRING_BIRDS][2] = {
    {25, 50}, {60, 40}, {90, 70}, {110, 45}, {40, 75}, {150, 65}
};

// Draw all spring birds
void birds_draw_all(void) {
    for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
        // Left wing (larger V shape)
        fb_rect_hsv(bird_positions[i][0] - 5, bird_positions[i][1],
                    bird_positions[i][0] - 1, bird_positions[i][1] - 3, 0, 0, 100, true);
        // Right wing (larger V shape)
        fb_rect_hsv(bird_positions[i][0] + 1, bird_positions[i][1] - 3,
                    bird_positions[i][0] + 5, bird_positions[i][1], 0, 0, 100, true);
    }
}
