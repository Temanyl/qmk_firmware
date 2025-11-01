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

#include QMK_KEYBOARD_H
#include "seasons_summer.h"
#include "framebuffer.h"
#include "objects/effects/airplane.h"
#include "objects/flora/sunflower.h"

// Reset summer animations
void reset_summer_animations(void) {
    // No animations to reset for summer (static elements only)
}

// Draw summer-specific scene elements
void draw_summer_scene_elements(void) {
    uint16_t ground_y = 150;

    // Draw airplane in top left
    airplane_draw();

    // Draw sunflowers on the ground
    sunflowers_draw_all(ground_y);
}
