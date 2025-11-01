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
#include "framebuffer.h"

// Reset spring animations
void reset_spring_animations(void) {
    // No animations to reset for spring (static elements only)
}

// Draw spring-specific scene elements
void draw_spring_scene_elements(void) {
    uint16_t ground_y = 150;

    // Draw birds in the sky (larger V shapes) - 6 birds
    uint16_t bird_positions[][2] = {
        {25, 50}, {60, 40}, {90, 70}, {110, 45}, {40, 75}, {150, 65}
    };
    for (uint8_t i = 0; i < 6; i++) {
        // Left wing (larger)
        fb_rect_hsv(bird_positions[i][0] - 5, bird_positions[i][1], bird_positions[i][0] - 1, bird_positions[i][1] - 3, 0, 0, 100, true);
        // Right wing (larger)
        fb_rect_hsv(bird_positions[i][0] + 1, bird_positions[i][1] - 3, bird_positions[i][0] + 5, bird_positions[i][1], 0, 0, 100, true);
    }

    // More butterflies, lower to the ground
    struct { uint16_t x; uint16_t y; uint8_t hue; } butterflies[] = {
        {20, 115, 234}, {45, 125, 170}, {65, 120, 42}, {85, 130, 200}, {105, 118, 10},
        {125, 135, 234}, {35, 128, 85}, {75, 122, 42}, {95, 133, 170}
    };
    for (uint8_t i = 0; i < 9; i++) {
        fb_circle_hsv(butterflies[i].x - 2, butterflies[i].y, 2, butterflies[i].hue, 255, 200, true);
        fb_circle_hsv(butterflies[i].x + 2, butterflies[i].y, 2, butterflies[i].hue, 255, 200, true);
    }

    // Draw flowers on the ground (various colors and sizes)
    struct { uint16_t x; uint8_t hue; uint8_t size; uint8_t stem_height; } flowers[] = {
        {15, 234, 3, 5}, {28, 0, 4, 6}, {42, 42, 3, 5}, {58, 170, 5, 7}, {72, 200, 3, 5},
        {88, 10, 4, 6}, {102, 85, 3, 5}, {118, 234, 5, 7}, {25, 42, 4, 6}, {50, 200, 3, 5},
        {80, 0, 5, 7}, {95, 170, 3, 5}, {110, 234, 4, 6}, {35, 10, 5, 7}, {65, 42, 3, 5}
    };
    for (uint8_t i = 0; i < 15; i++) {
        // Flower stem (green)
        fb_rect_hsv(flowers[i].x, ground_y - flowers[i].stem_height, flowers[i].x + 1, ground_y, 85, 200, 150, true);
        // Flower petals (colorful circles with varying sizes)
        fb_circle_hsv(flowers[i].x, ground_y - flowers[i].stem_height - 2, flowers[i].size, flowers[i].hue, 255, 220, true);
        // Flower center (yellow, size varies with flower)
        fb_circle_hsv(flowers[i].x, ground_y - flowers[i].stem_height - 2, flowers[i].size / 3, 42, 255, 255, true);
    }
}
