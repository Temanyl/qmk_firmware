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
#include "seasons_spring.h"
#include "../../display/framebuffer.h"
#include "../../objects/fauna/bird.h"
#include "../../objects/fauna/butterfly.h"
#include "../../objects/flora/flower.h"

// Reset spring animations
void reset_spring_animations(void) {
    // No animations to reset for spring (static elements only)
}

// Draw spring-specific scene elements
void draw_spring_scene_elements(void) {
    uint16_t ground_y = 150;

    // Draw birds in the sky
    birds_draw_all();

    // Draw butterflies
    butterflies_draw_all();

    // Draw flowers on the ground
    flowers_draw_all(ground_y);
}
