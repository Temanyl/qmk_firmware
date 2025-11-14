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
void moon_init(moon_t *moon, int16_t x, int16_t y, uint16_t year, uint8_t month, uint8_t day, uint8_t hour) {
    moon->x = x;
    moon->y = y;
    moon->year = year;
    moon->month = month;
    moon->day = day;
    moon->hour = hour;
}

// Integer square root helper (for calculating ellipse extent)
static uint16_t isqrt(uint16_t n) {
    if (n == 0) return 0;
    uint16_t result = 0;
    uint16_t bit = 1 << 7;  // Start from highest bit for 16-bit

    while (bit > 0) {
        uint16_t temp = result + bit;
        if (temp * temp <= n) {
            result = temp;
        }
        bit >>= 1;
    }
    return result;
}

// Calculate Julian Day Number from Gregorian date
// Based on algorithm from "Astronomical Algorithms" by Jean Meeus
static int32_t calculate_julian_day(uint16_t year, uint8_t month, uint8_t day, uint8_t hour) {
    // Adjust for Julian calendar algorithm (treat Jan/Feb as months 13/14 of previous year)
    int32_t a = (14 - month) / 12;
    int32_t y = year + 4800 - a;
    int32_t m = month + 12 * a - 3;

    // Calculate JDN at midnight
    int32_t jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;

    // Add fractional day for the hour (simplified: hour/24)
    // We multiply by 100 to keep precision (will divide later)
    return jdn * 100 + (hour * 100) / 24;
}

// Calculate moon phase (0-29) based on astronomical lunar cycle
// Uses the synodic month period of 29.530588853 days
// Reference: New moon on January 6, 2000 at 18:14 UTC (JD 2451550.26)
static uint8_t calculate_moon_phase(uint16_t year, uint8_t month, uint8_t day, uint8_t hour) {
    // Calculate current Julian Day (scaled by 100 for precision)
    int32_t current_jd_scaled = calculate_julian_day(year, month, day, hour);

    // Reference new moon: Jan 6, 2000, 18:14 UTC
    // JD = 2451550.26, scaled by 100 = 245155026
    const int32_t reference_jd_scaled = 245155026;

    // Calculate days since reference new moon (scaled by 100)
    int32_t days_since_ref_scaled = current_jd_scaled - reference_jd_scaled;

    // Synodic month = 29.530588853 days
    // Scaled by 100: 2953.0588853, approximated as 2953
    const int32_t synodic_month_scaled = 2953;

    // Calculate position in lunar cycle
    // We want: (days_since_ref % synodic_month) / synodic_month * 29.53
    // This gives us the phase as a value from 0 to 29.53

    // First, get the remainder when dividing by synodic month
    int32_t phase_scaled = days_since_ref_scaled % synodic_month_scaled;

    // Handle negative remainders (for dates before reference)
    if (phase_scaled < 0) {
        phase_scaled += synodic_month_scaled;
    }

    // Convert to phase 0-29
    // phase_scaled is in range [0, 2953), representing days in the cycle
    // Map to [0, 29]: multiply by 29 and divide by synodic_month_scaled
    uint8_t moon_phase = (uint8_t)((phase_scaled * 29) / synodic_month_scaled);

    // Clamp to 0-29 range
    if (moon_phase > 29) {
        moon_phase = 29;
    }

    return moon_phase;
}

// Draw moon with phase using geometrically correct crescents
void moon_draw(const moon_t *moon) {
    // Calculate astronomically accurate moon phase
    uint8_t moon_phase = calculate_moon_phase(moon->year, moon->month, moon->day, moon->hour);

    const int16_t radius = 8;
    const int16_t cx = moon->x;
    const int16_t cy = moon->y;

    // Draw moon pixel by pixel with proper elliptical terminator
    // This creates smooth, mathematically correct crescents instead of circle intersections
    for (int16_t dy = -radius; dy <= radius; dy++) {
        for (int16_t dx = -radius; dx <= radius; dx++) {
            // Check if pixel is within moon circle
            if (dx * dx + dy * dy > radius * radius) {
                continue;
            }

            // Determine if this pixel should be illuminated based on phase
            bool is_lit;

            if (moon_phase <= 14) {
                // Waxing moon (new -> full): illumination grows from right to left
                // The terminator (shadow boundary) is an ellipse that moves left as phase increases
                // Phase 0: shadow covers everything (term_x at leftmost, all dx > term_x is false)
                // Phase 7: first quarter, shadow at center (term_x = 0)
                // Phase 14: full moon, no shadow (term_x at rightmost, all dx > term_x is true)

                // Calculate terminator x-position at this y-height
                // Formula: term_x = k * sqrt(radius² - dy²)
                // where k ranges from -1 (phase 0) to 0 (phase 7) to +1 (phase 14)
                int16_t k_num = (int16_t)moon_phase - 7;  // Range: -7 to +7
                int16_t k_den = 7;

                // Calculate maximum x extent at this y: sqrt(radius² - dy²)
                int16_t y_extent_sq = radius * radius - dy * dy;
                int16_t y_extent = isqrt(y_extent_sq);

                // Calculate terminator position: term_x = (k_num * y_extent) / k_den
                int16_t term_x = (k_num * y_extent) / k_den;

                // Pixel is illuminated if it's to the right of the terminator
                is_lit = (dx > term_x);

            } else {
                // Waning moon (full -> new): illumination shrinks from left to right
                // Phase 15: full moon (term_x at rightmost, all dx < term_x is true)
                // Phase 22: last quarter, shadow at center (term_x = 0)
                // Phase 29: new moon (term_x at leftmost, all dx < term_x is false)

                uint8_t waning_phase = moon_phase - 15;  // Range: 0 to 14

                // k ranges from +1 (phase 15) to 0 (phase 22) to -1 (phase 29)
                int16_t k_num = 7 - (int16_t)waning_phase;  // Range: +7 to -7
                int16_t k_den = 7;

                // Calculate maximum x extent at this y
                int16_t y_extent_sq = radius * radius - dy * dy;
                int16_t y_extent = isqrt(y_extent_sq);

                // Calculate terminator position
                int16_t term_x = (k_num * y_extent) / k_den;

                // Pixel is illuminated if it's to the left of the terminator
                is_lit = (dx < term_x);
            }

            // Draw the pixel with appropriate color
            if (is_lit) {
                // Illuminated moon surface: pale yellow
                fb_set_pixel_hsv(cx + dx, cy + dy, 42, 100, 255);
            } else {
                // Shadow: very dark
                fb_set_pixel_hsv(cx + dx, cy + dy, 0, 0, 20);
            }
        }
    }
}
