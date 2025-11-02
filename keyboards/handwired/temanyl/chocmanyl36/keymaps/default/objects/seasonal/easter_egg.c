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

#include "easter_egg.h"
#include "../../display/framebuffer.h"
#include <stdlib.h>

// Easter egg states
easter_egg_t easter_eggs[NUM_EASTER_EGGS];

// Predefined egg configurations (x, y, base_hue, accent_hue, pattern)
static const struct {
    int16_t x;
    int16_t y;
    uint8_t base_hue;
    uint8_t accent_hue;
    egg_pattern_t pattern;
} egg_config[NUM_EASTER_EGGS] = {
    {25, 140, 0, 213, EGG_PATTERN_STRIPES},      // Red with magenta stripes
    {50, 140, 85, 170, EGG_PATTERN_DOTS},        // Green with blue dots
    {75, 140, 170, 42, EGG_PATTERN_ZIGZAG},      // Blue with yellow zigzag
    {100, 140, 42, 0, EGG_PATTERN_SOLID},        // Yellow with red border
    {125, 140, 213, 128, EGG_PATTERN_SWIRL}      // Magenta with cyan swirl
};

// Initialize Easter eggs
void easter_eggs_init(void) {
    for (uint8_t i = 0; i < NUM_EASTER_EGGS; i++) {
        easter_eggs[i].x = egg_config[i].x;
        easter_eggs[i].y = egg_config[i].y;
        easter_eggs[i].base_hue = egg_config[i].base_hue;
        easter_eggs[i].accent_hue = egg_config[i].accent_hue;
        easter_eggs[i].pattern = egg_config[i].pattern;
        easter_eggs[i].hidden = false;
    }
}

// Reset Easter eggs
void easter_eggs_reset(void) {
    easter_eggs_init();
}

// Draw egg with stripes pattern
static void draw_egg_stripes(int16_t x, int16_t y, uint8_t base_hue, uint8_t accent_hue) {
    // Egg outline (oval shape)
    fb_ellipse_hsv(x + 4, y + 5, 3, 4, base_hue, 255, 200, true);

    // Horizontal stripes
    fb_line_hsv(x + 2, y + 3, x + 6, y + 3, accent_hue, 255, 220);
    fb_line_hsv(x + 1, y + 5, x + 7, y + 5, accent_hue, 255, 220);
    fb_line_hsv(x + 2, y + 7, x + 6, y + 7, accent_hue, 255, 220);
}

// Draw egg with dots pattern
static void draw_egg_dots(int16_t x, int16_t y, uint8_t base_hue, uint8_t accent_hue) {
    // Egg base
    fb_ellipse_hsv(x + 4, y + 5, 3, 4, base_hue, 255, 200, true);

    // Polka dots
    fb_circle_hsv(x + 3, y + 3, 1, accent_hue, 255, 240, true);
    fb_circle_hsv(x + 5, y + 4, 1, accent_hue, 255, 240, true);
    fb_circle_hsv(x + 3, y + 6, 1, accent_hue, 255, 240, true);
    fb_circle_hsv(x + 5, y + 7, 1, accent_hue, 255, 240, true);
}

// Draw egg with zigzag pattern
static void draw_egg_zigzag(int16_t x, int16_t y, uint8_t base_hue, uint8_t accent_hue) {
    // Egg base
    fb_ellipse_hsv(x + 4, y + 5, 3, 4, base_hue, 255, 200, true);

    // Zigzag pattern (vertical)
    for (int16_t row = 2; row <= 8; row++) {
        int16_t offset = (row % 2 == 0) ? 0 : 1;
        fb_set_pixel_hsv(x + 3 + offset, y + row, accent_hue, 255, 240);
        fb_set_pixel_hsv(x + 5 + offset, y + row, accent_hue, 255, 240);
    }
}

// Draw solid egg with border
static void draw_egg_solid(int16_t x, int16_t y, uint8_t base_hue, uint8_t accent_hue) {
    // Egg base
    fb_ellipse_hsv(x + 4, y + 5, 3, 4, base_hue, 255, 200, true);

    // Border outline
    fb_ellipse_hsv(x + 4, y + 5, 3, 4, accent_hue, 255, 220, false);

    // Add a decorative band in the middle
    fb_line_hsv(x + 2, y + 5, x + 6, y + 5, accent_hue, 255, 220);
}

// Draw egg with swirl pattern
static void draw_egg_swirl(int16_t x, int16_t y, uint8_t base_hue, uint8_t accent_hue) {
    // Egg base
    fb_ellipse_hsv(x + 4, y + 5, 3, 4, base_hue, 255, 200, true);

    // Swirl pattern (curved line)
    fb_set_pixel_hsv(x + 4, y + 2, accent_hue, 255, 240);
    fb_set_pixel_hsv(x + 5, y + 3, accent_hue, 255, 240);
    fb_set_pixel_hsv(x + 5, y + 4, accent_hue, 255, 240);
    fb_set_pixel_hsv(x + 4, y + 5, accent_hue, 255, 240);
    fb_set_pixel_hsv(x + 3, y + 6, accent_hue, 255, 240);
    fb_set_pixel_hsv(x + 3, y + 7, accent_hue, 255, 240);
    fb_set_pixel_hsv(x + 4, y + 8, accent_hue, 255, 240);
}

// Draw a single Easter egg
static void draw_easter_egg(int16_t x, int16_t y, uint8_t base_hue, uint8_t accent_hue, egg_pattern_t pattern) {
    switch (pattern) {
        case EGG_PATTERN_STRIPES:
            draw_egg_stripes(x, y, base_hue, accent_hue);
            break;
        case EGG_PATTERN_DOTS:
            draw_egg_dots(x, y, base_hue, accent_hue);
            break;
        case EGG_PATTERN_ZIGZAG:
            draw_egg_zigzag(x, y, base_hue, accent_hue);
            break;
        case EGG_PATTERN_SOLID:
            draw_egg_solid(x, y, base_hue, accent_hue);
            break;
        case EGG_PATTERN_SWIRL:
            draw_egg_swirl(x, y, base_hue, accent_hue);
            break;
    }

    // Add highlight for shine effect
    fb_set_pixel_hsv(x + 5, y + 3, 0, 0, 255);
}

// Draw a single Easter egg by index
void easter_egg_draw_single(uint8_t index) {
    if (index >= NUM_EASTER_EGGS) return;
    if (easter_eggs[index].hidden) return;

    draw_easter_egg(easter_eggs[index].x, easter_eggs[index].y,
                   easter_eggs[index].base_hue, easter_eggs[index].accent_hue,
                   easter_eggs[index].pattern);
}

// Draw all Easter eggs
void easter_eggs_draw_all(void) {
    for (uint8_t i = 0; i < NUM_EASTER_EGGS; i++) {
        easter_egg_draw_single(i);
    }
}
