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
#include "objects/seasonal/pumpkin.h"
#include "objects/seasonal/ghost.h"
#include "framebuffer.h"

// Halloween animation state
ghost_t ghosts[NUM_GHOSTS];
bool ghost_initialized = false;
bool ghost_background_saved = false;
uint32_t ghost_animation_timer = 0;

// Pumpkin instances
static pumpkin_t pumpkins[NUM_PUMPKINS];
static bool pumpkins_initialized = false;

// Forward declarations
extern uint8_t current_month;
extern uint8_t current_day;

// Halloween event check
bool is_halloween_event(void) {
    return (current_month == 10 && current_day >= 28) ||
           (current_month == 11 && current_day <= 3);
}

// Draw Halloween elements
void draw_halloween_elements(void) {
    // Initialize pumpkins on first draw
    if (!pumpkins_initialized) {
        pumpkin_init(&pumpkins[0], 25, 145, 8);
        pumpkin_init(&pumpkins[1], 55, 143, 10);
        pumpkin_init(&pumpkins[2], 90, 144, 9);
        pumpkins_initialized = true;
    }

    // Draw all pumpkins
    for (uint8_t i = 0; i < NUM_PUMPKINS; i++) {
        pumpkin_draw(&pumpkins[i]);
    }
}

// Initialize ghosts
void init_ghosts(void) {
    if (ghost_initialized) return;

    ghost_init(&ghosts[0], 20, 90, 1, 0);
    ghost_init(&ghosts[1], 60, 50, -1, 40);
    ghost_init(&ghosts[2], 100, 70, 1, 80);

    ghost_initialized = true;
}

// Check if pixel is in ghost
bool is_pixel_in_ghost(int16_t px, int16_t py, uint8_t ghost_idx) {
    if (ghost_idx >= NUM_GHOSTS) return false;
    return ghost_contains_point(&ghosts[ghost_idx], px, py);
}

// Redraw ghosts in region
void redraw_ghosts_in_region(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if (!ghost_initialized) return;

    for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
        int16_t ghost_x1, ghost_y1, ghost_x2, ghost_y2;
        ghost_get_bounds(&ghosts[i], &ghost_x1, &ghost_y1, &ghost_x2, &ghost_y2);

        // Check if ghost overlaps with the region
        if (ghost_x2 >= x1 && ghost_x1 <= x2 && ghost_y2 >= y1 && ghost_y1 <= y2) {
            ghost_draw(&ghosts[i]);
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
    pumpkins_initialized = false;
}
