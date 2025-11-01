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

#include "snowman.h"
#include "../../display/framebuffer.h"

// Initialize a snowman object with position and size
void snowman_init(snowman_t* snowman, int16_t x, int16_t y, uint8_t size) {
    snowman->x = x;
    snowman->y = y;
    snowman->size = size;
}

// Draw the snowman at its current position
void snowman_draw(const snowman_t* snowman) {
    int16_t x = snowman->x;
    int16_t y = snowman->y;
    uint8_t size = snowman->size;

    // Bounds check - don't draw if completely off-screen
    if (x < -(size * 2) || x > 135 + (size * 2) || y < -(size * 3) || y > 152 + size) {
        return;
    }

    // Calculate snowball sizes (bottom, middle, head)
    uint8_t bottom_radius = size;
    uint8_t middle_radius = size * 3 / 4;
    uint8_t head_radius = size / 2;

    // Calculate positions for each snowball
    int16_t bottom_y = y - bottom_radius;
    int16_t middle_y = bottom_y - bottom_radius - middle_radius + 2;
    int16_t head_y = middle_y - middle_radius - head_radius + 2;

    // Draw bottom snowball (white)
    fb_circle_hsv(x, bottom_y, bottom_radius, 0, 0, 255, true);

    // Draw middle snowball (white)
    fb_circle_hsv(x, middle_y, middle_radius, 0, 0, 255, true);

    // Draw head snowball (white)
    fb_circle_hsv(x, head_y, head_radius, 0, 0, 255, true);

    // Calculate feature sizes proportional to head
    uint8_t eye_size = head_radius / 4;
    uint8_t eye_offset = head_radius / 3;
    uint8_t button_size = middle_radius / 5;

    // Draw eyes (black dots)
    fb_circle_hsv(x - eye_offset, head_y - eye_size, eye_size, 0, 0, 0, true);
    fb_circle_hsv(x + eye_offset, head_y - eye_size, eye_size, 0, 0, 0, true);

    // Draw carrot nose (orange triangle, simplified as rectangle)
    uint8_t nose_width = head_radius / 2;
    uint8_t nose_height = head_radius / 4;
    fb_rect_hsv(x, head_y, x + nose_width, head_y + nose_height, 20, 255, 255, true);

    // Draw smile (black dots in an arc)
    uint8_t mouth_y = head_y + head_radius / 2;
    for (int8_t i = -2; i <= 2; i++) {
        int16_t mouth_x = x + i * (head_radius / 4);
        int16_t mouth_dot_y = mouth_y + (i * i) / 3; // Parabola for smile
        fb_circle_hsv(mouth_x, mouth_dot_y, 1, 0, 0, 0, true);
    }

    // Draw buttons on middle body (black dots)
    fb_circle_hsv(x, middle_y - middle_radius / 3, button_size, 0, 0, 0, true);
    fb_circle_hsv(x, middle_y, button_size, 0, 0, 0, true);
    fb_circle_hsv(x, middle_y + middle_radius / 3, button_size, 0, 0, 0, true);

    // Draw stick arms (brown lines)
    // Left arm
    int16_t arm_start_y = middle_y - middle_radius / 4;
    int16_t left_arm_end_x = x - middle_radius - size / 2;
    int16_t left_arm_end_y = arm_start_y - size / 3;
    fb_color_t brown_color = fb_rgb888_to_rgb565(139, 69, 19);
    fb_line(x - middle_radius, arm_start_y, left_arm_end_x, left_arm_end_y, brown_color); // Brown

    // Right arm
    int16_t right_arm_end_x = x + middle_radius + size / 2;
    int16_t right_arm_end_y = arm_start_y - size / 3;
    fb_line(x + middle_radius, arm_start_y, right_arm_end_x, right_arm_end_y, brown_color); // Brown

    // Draw top hat (black rectangle and rim)
    int16_t hat_width = head_radius;
    int16_t hat_height = head_radius;
    int16_t hat_top_y = head_y - head_radius - hat_height;
    int16_t rim_width = hat_width + head_radius / 2;

    // Hat brim (wider)
    fb_rect_hsv(x - rim_width / 2, head_y - head_radius - 2,
                x + rim_width / 2, head_y - head_radius + 2, 0, 0, 0, true);

    // Hat top (taller rectangle)
    fb_rect_hsv(x - hat_width / 2, hat_top_y,
                x + hat_width / 2, head_y - head_radius - 2, 0, 0, 0, true);
}
