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

#include "stars.h"
#include "../../display/framebuffer.h"

// Star positions
static const uint16_t star_positions[NUM_STARS][2] = {
    {20, 15}, {50, 25}, {90, 18}, {110, 30},
    {65, 12}, {100, 22}, {80, 30},
    {120, 15}, {10, 25}, {28, 20},
    {85, 8}, {70, 25}, {60, 15}
};

// Draw all stars
void stars_draw(void) {
    for (uint8_t i = 0; i < NUM_STARS; i++) {
        fb_rect_hsv(star_positions[i][0], star_positions[i][1],
                star_positions[i][0] + 2, star_positions[i][1] + 2, 42, 50, 255, true);
    }
}
