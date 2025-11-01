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
#include "seasons_halloween.h"
#include "framebuffer.h"

// Halloween animation state
ghost_t ghosts[NUM_GHOSTS];
bool ghost_initialized = false;
bool ghost_background_saved = false;
uint32_t ghost_animation_timer = 0;

// Forward declarations
extern uint8_t current_month;
extern uint8_t current_day;

// Halloween event check
bool is_halloween_event(void) {
    return (current_month == 10 && current_day >= 28) ||
           (current_month == 11 && current_day <= 3);
}

// Draw pumpkin
void draw_pumpkin(int16_t x, int16_t y, uint8_t size) {
    if (x < -size || x > 135 + size || y < -size || y > 152 + size) return;

    fb_circle_hsv(x, y, size, 20, 255, 255, true);
    fb_circle_hsv(x, y + size/3, size - 2, 16, 255, 220, true);

    uint8_t eye_offset = size / 3;
    uint8_t eye_size = size / 4;

    fb_rect_hsv(x - eye_offset - eye_size, y - eye_offset,
            x - eye_offset + eye_size, y - eye_offset + eye_size, 0, 0, 0, true);
    fb_rect_hsv(x + eye_offset - eye_size, y - eye_offset,
            x + eye_offset + eye_size, y - eye_offset + eye_size, 0, 0, 0, true);
    fb_rect_hsv(x - eye_size/2, y, x + eye_size/2, y + eye_size, 0, 0, 0, true);
    fb_rect_hsv(x - size/2, y + size/3, x + size/2, y + size/2, 0, 0, 0, true);

    for (int8_t i = -size/3; i < size/3; i += size/4) {
        fb_rect_hsv(x + i, y + size/3, x + i + size/6, y + size/2 - 1, 20, 255, 255, true);
    }

    fb_rect_hsv(x - 2, y - size - 3, x + 2, y - size + 1, 85, 200, 100, true);
}

// Draw ghost
void draw_ghost(int16_t x, int16_t y) {
    if (x < -15 || x > 150 || y < -20 || y > 172) return;

    fb_circle_hsv(x, y, 7, 0, 0, 240, true);
    fb_rect_hsv(x - 7, y, x + 7, y + 12, 0, 0, 240, true);
    fb_rect_hsv(x - 7, y + 10, x - 4, y + 13, 0, 0, 240, true);
    fb_rect_hsv(x - 3, y + 10, x + 0, y + 12, 0, 0, 240, true);
    fb_rect_hsv(x + 1, y + 10, x + 4, y + 13, 0, 0, 240, true);
    fb_rect_hsv(x + 5, y + 10, x + 7, y + 12, 0, 0, 240, true);
    fb_rect_hsv(x - 3, y - 2, x - 1, y, 0, 0, 0, true);
    fb_rect_hsv(x + 1, y - 2, x + 3, y, 0, 0, 0, true);
    fb_circle_hsv(x, y + 3, 2, 0, 0, 0, false);
}

// Draw Halloween elements
void draw_halloween_elements(void) {
    draw_pumpkin(25, 145, 8);
    draw_pumpkin(55, 143, 10);
    draw_pumpkin(90, 144, 9);
}

// Initialize ghosts
void init_ghosts(void) {
    if (ghost_initialized) return;

    ghosts[0].x = 20; ghosts[0].y = 90; ghosts[0].vx = 1; ghosts[0].vy = 0; ghosts[0].phase = 0;
    ghosts[1].x = 60; ghosts[1].y = 50; ghosts[1].vx = -1; ghosts[1].vy = 0; ghosts[1].phase = 40;
    ghosts[2].x = 100; ghosts[2].y = 70; ghosts[2].vx = 1; ghosts[2].vy = 0; ghosts[2].phase = 80;

    ghost_initialized = true;
}

// Check if pixel is in ghost
bool is_pixel_in_ghost(int16_t px, int16_t py, uint8_t ghost_idx) {
    if (ghost_idx >= NUM_GHOSTS) return false;
    int16_t gx = ghosts[ghost_idx].x;
    int16_t gy = ghosts[ghost_idx].y;
    return (px >= gx - 7 && px <= gx + 7 && py >= gy - 7 && py <= gy + 13);
}

// Redraw ghosts in region
void redraw_ghosts_in_region(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if (!ghost_initialized) return;

    for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
        int16_t gx = ghosts[i].x;
        int16_t gy = ghosts[i].y;
        int16_t ghost_x1 = gx - 7, ghost_y1 = gy - 7;
        int16_t ghost_x2 = gx + 7, ghost_y2 = gy + 13;

        if (ghost_x2 >= x1 && ghost_x1 <= x2 && ghost_y2 >= y1 && ghost_y1 <= y2) {
            draw_ghost(gx, gy);
        }
    }
}

// Animate ghosts
void animate_ghosts(void) {
    if (!ghost_initialized || !ghost_background_saved) {
        return;
    }

    // Only animate during Halloween event
    if (!is_halloween_event()) {
        return;
    }

    // Update all ghost positions with floating motion
    for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
        // Horizontal movement
        ghosts[i].x += ghosts[i].vx;

        // Floating motion (sine wave approximation)
        ghosts[i].phase = (ghosts[i].phase + 1) % 160;  // Full cycle every 160 frames

        // Different base heights and oscillation amplitudes for each ghost
        int16_t base_y, amplitude;
        switch(i) {
            case 0:
                base_y = 90;
                amplitude = 8;
                break;
            case 1:
                base_y = 50;
                amplitude = 6;
                break;
            case 2:
            default:
                base_y = 70;
                amplitude = 10;
                break;
        }

        // Smooth sine wave approximation using piecewise linear interpolation
        int16_t offset;
        uint8_t quarter = ghosts[i].phase / 40;
        uint8_t phase_in_quarter = ghosts[i].phase % 40;

        switch(quarter) {
            case 0: // Rising: 0 to +amplitude
                offset = (amplitude * phase_in_quarter) / 40;
                break;
            case 1: // Peak to middle: +amplitude to 0
                offset = amplitude - (amplitude * phase_in_quarter) / 40;
                break;
            case 2: // Falling: 0 to -amplitude
                offset = -(amplitude * phase_in_quarter) / 40;
                break;
            case 3: // Trough to middle: -amplitude to 0
            default:
                offset = -amplitude + (amplitude * phase_in_quarter) / 40;
                break;
        }

        ghosts[i].y = base_y + offset;

        // Bounce off screen edges
        if (ghosts[i].x <= 8 || ghosts[i].x >= FB_WIDTH - 8) {
            ghosts[i].vx = -ghosts[i].vx;
        }
    }

    // NOTE: Drawing is handled by caller to ensure consistent z-ordering
}

// Reset Halloween animations
void reset_halloween_animations(void) {
    ghost_initialized = false;
    ghost_background_saved = false;
}
