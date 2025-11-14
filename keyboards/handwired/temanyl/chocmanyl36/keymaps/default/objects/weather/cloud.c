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

#include "cloud.h"
#include "../../display/framebuffer.h"

// Cloud color configuration (HSV)
// Light clouds (winter/snow)
#define LIGHT_CLOUD_HUE 0       // Hue doesn't matter for white/grey
#define LIGHT_CLOUD_SAT 0       // 0=white/grey
#define LIGHT_CLOUD_VAL_MAIN 160  // Main body brightness
#define LIGHT_CLOUD_VAL_TOP  150  // Top bump slightly darker

// Light rain clouds (light gray)
#define LIGHT_RAIN_CLOUD_HUE 0       // Hue doesn't matter for grey
#define LIGHT_RAIN_CLOUD_SAT 0       // 0=grey
#define LIGHT_RAIN_CLOUD_VAL_MAIN 150  // Light gray (brighter)
#define LIGHT_RAIN_CLOUD_VAL_TOP  140  // Top bump slightly darker

// Medium rain clouds (medium gray)
#define MEDIUM_RAIN_CLOUD_HUE 0       // Hue doesn't matter for grey
#define MEDIUM_RAIN_CLOUD_SAT 0       // 0=grey
#define MEDIUM_RAIN_CLOUD_VAL_MAIN 110  // Medium gray
#define MEDIUM_RAIN_CLOUD_VAL_TOP  100  // Top bump slightly darker

// Heavy rain clouds (dark gray)
#define HEAVY_RAIN_CLOUD_HUE 0       // Hue doesn't matter for grey
#define HEAVY_RAIN_CLOUD_SAT 0       // 0=grey
#define HEAVY_RAIN_CLOUD_VAL_MAIN 70   // Dark gray (darker)
#define HEAVY_RAIN_CLOUD_VAL_TOP  60   // Top bump slightly darker

// Initialize a cloud
void cloud_init(cloud_t* cloud, int16_t x, int16_t y, int8_t vx) {
    cloud->x = x;
    cloud->y = y;
    cloud->vx = vx;
}

// Draw a cloud at its current position
void cloud_draw(const cloud_t* cloud, cloud_type_t type) {
    int16_t x = cloud->x;
    int16_t y = cloud->y;

    // Don't draw clouds that are completely off-screen
    if (x < -30 || x > 165) {
        return;
    }

    // Choose colors based on cloud type
    uint8_t hue, sat, val_main, val_top;
    switch (type) {
        case CLOUD_TYPE_DARK_LIGHT:
            hue = LIGHT_RAIN_CLOUD_HUE;
            sat = LIGHT_RAIN_CLOUD_SAT;
            val_main = LIGHT_RAIN_CLOUD_VAL_MAIN;
            val_top = LIGHT_RAIN_CLOUD_VAL_TOP;
            break;
        case CLOUD_TYPE_DARK_MEDIUM:
            hue = MEDIUM_RAIN_CLOUD_HUE;
            sat = MEDIUM_RAIN_CLOUD_SAT;
            val_main = MEDIUM_RAIN_CLOUD_VAL_MAIN;
            val_top = MEDIUM_RAIN_CLOUD_VAL_TOP;
            break;
        case CLOUD_TYPE_DARK_HEAVY:
            hue = HEAVY_RAIN_CLOUD_HUE;
            sat = HEAVY_RAIN_CLOUD_SAT;
            val_main = HEAVY_RAIN_CLOUD_VAL_MAIN;
            val_top = HEAVY_RAIN_CLOUD_VAL_TOP;
            break;
        case CLOUD_TYPE_LIGHT:
        default:
            hue = LIGHT_CLOUD_HUE;
            sat = LIGHT_CLOUD_SAT;
            val_main = LIGHT_CLOUD_VAL_MAIN;
            val_top = LIGHT_CLOUD_VAL_TOP;
            break;
    }

    // Cloud shape using circles
    // Bounds: x-16 to x+18, y-11 to y+10 (conservative estimate)
    fb_circle_hsv(x, y, 9, hue, sat, val_main, true);          // Main body (larger)
    fb_circle_hsv(x + 10, y + 2, 7, hue, sat, val_main, true); // Right bump (larger)
    fb_circle_hsv(x - 8, y + 2, 7, hue, sat, val_main, true);  // Left bump (larger)
    fb_circle_hsv(x + 5, y - 4, 6, hue, sat, val_top, true);   // Top bump (larger)
}

// Get the cloud's bounding box
void cloud_get_bounds(const cloud_t* cloud, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2) {
    // Conservative bounds to cover all circles
    // Main body: x±9, y±9
    // Right bump: (x+10)±7, (y+2)±7 = x+3 to x+17, y-5 to y+9
    // Left bump: (x-8)±7, (y+2)±7 = x-15 to x-1, y-5 to y+9
    // Top bump: (x+5)±6, (y-4)±6 = x-1 to x+11, y-10 to y+2
    *x1 = cloud->x - 16;
    *y1 = cloud->y - 11;
    *x2 = cloud->x + 18;
    *y2 = cloud->y + 10;
}

// Check if a point is inside the cloud's bounds (approximate)
bool cloud_contains_point(const cloud_t* cloud, int16_t px, int16_t py) {
    int16_t x1, y1, x2, y2;
    cloud_get_bounds(cloud, &x1, &y1, &x2, &y2);
    return (px >= x1 && px <= x2 && py >= y1 && py <= y2);
}
