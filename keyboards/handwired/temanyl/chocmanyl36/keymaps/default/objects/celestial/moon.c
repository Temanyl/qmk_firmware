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

#include "moon.h"
#include "../../display/framebuffer.h"

// Initialize moon
void moon_init(moon_t *moon, int16_t x, int16_t y, uint8_t day, uint8_t hour) {
    moon->x = x;
    moon->y = y;
    moon->day = day;
    moon->hour = hour;
}

// Draw moon with phase
void moon_draw(const moon_t *moon) {
    // Calculate moon day for phase
    uint8_t moon_day;
    if (moon->hour < 6) {
        // Early morning: this is the end of yesterday's night
        moon_day = (moon->day > 1) ? (moon->day - 1) : 31;
    } else {
        // Evening (hours 20-23): this is the start of today's night
        moon_day = moon->day;
    }
    uint8_t moon_phase = (moon_day * 29) / 31; // Map day 1-31 to phase 0-29

    // Draw full moon circle first (pale yellow/white base)
    fb_circle_hsv(moon->x, moon->y, 8, 42, 100, 255, true);

    // Add shadow to create moon phase effect
    if (moon_phase < 14) {
        // Waxing moon (new -> full): shadow on left side, shrinking
        if (moon_phase < 7) {
            // New moon to first quarter: shadow covers most/half of left side
            int8_t shadow_offset = -8 + (moon_phase * 2); // -8 to 6
            uint8_t shadow_radius = 8 - (moon_phase / 2); // 8 to 4
            fb_circle_hsv(moon->x + shadow_offset, moon->y, shadow_radius, 0, 0, 20, true);
        } else {
            // First quarter to full: small shadow on left, disappearing
            int8_t shadow_offset = 6 - ((moon_phase - 7) * 2); // 6 to -8
            uint8_t shadow_radius = 5 - ((moon_phase - 7) / 2); // 4 to 0
            if (shadow_radius > 0) {
                fb_circle_hsv(moon->x + shadow_offset, moon->y, shadow_radius, 0, 0, 20, true);
            }
        }
    } else if (moon_phase > 14) {
        // Waning moon (full -> new): shadow on right side, growing
        uint8_t waning_phase = moon_phase - 15; // 0 to 14
        if (waning_phase < 7) {
            // Full to last quarter: small shadow on right, growing
            int8_t shadow_offset = -6 + (waning_phase * 2); // -6 to 8
            uint8_t shadow_radius = (waning_phase / 2); // 0 to 3
            if (shadow_radius > 0) {
                fb_circle_hsv(moon->x + shadow_offset, moon->y, shadow_radius, 0, 0, 20, true);
            }
        } else {
            // Last quarter to new: shadow covers half/most of right side
            int8_t shadow_offset = 8 - ((waning_phase - 7) * 2); // 8 to -6
            uint8_t shadow_radius = 5 + ((waning_phase - 7) / 2); // 4 to 7
            fb_circle_hsv(moon->x + shadow_offset, moon->y, shadow_radius, 0, 0, 20, true);
        }
    }
}
