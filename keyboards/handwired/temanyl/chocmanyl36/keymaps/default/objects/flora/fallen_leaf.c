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

#include "fallen_leaf.h"
#include "framebuffer.h"

// Fallen leaf data (x position, hue color)
static const struct { uint16_t x; uint8_t hue; } fallen_leaves[NUM_FALL_LEAVES] = {
    {18, 10}, {35, 0}, {52, 25}, {68, 15}, {82, 8}, {95, 20}, {108, 5}, {122, 30},
    {25, 12}, {45, 18}, {62, 22}, {78, 28}, {92, 15}, {105, 10}, {118, 25}
};

// Draw all fallen leaves
void fallen_leaves_draw_all(void) {
    for (uint8_t i = 0; i < NUM_FALL_LEAVES; i++) {
        // Small leaves on ground (small circles just above ground line at y=146)
        fb_circle_hsv(fallen_leaves[i].x, 146, 2, fallen_leaves[i].hue, 255, 220, true);
    }
}
