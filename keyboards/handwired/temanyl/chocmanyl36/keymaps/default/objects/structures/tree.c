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

#include "tree.h"
#include "../../display/framebuffer.h"

// Initialize tree
void tree_init(tree_t *tree, uint16_t base_x, uint16_t base_y, uint8_t season, uint8_t hue, uint8_t sat, uint8_t val) {
    tree->base_x = base_x;
    tree->base_y = base_y;
    tree->season = season;
    tree->hue = hue;
    tree->sat = sat;
    tree->val = val;
}

// Draw tree with seasonal variations
void tree_draw(const tree_t *tree) {
    uint16_t base_x = tree->base_x;
    uint16_t base_y = tree->base_y;
    uint8_t season = tree->season;

    // Tree structure: trunk + canopy
    // Trunk (brown)
    uint8_t trunk_width = 6;
    uint8_t trunk_height = (season == 1) ? 28 : 22; // Spring trees are taller
    fb_rect_hsv(base_x - trunk_width/2, base_y - trunk_height,
            base_x + trunk_width/2, base_y, 20, 200, 100, true);

    // Canopy changes by season
    if (season == 0) { // Winter - bare branches with more detail
        // Draw main upward-reaching branches
        // Left upward branch
        fb_rect_hsv(base_x - 8, base_y - trunk_height - 10, base_x - 6, base_y - trunk_height - 2, 20, 150, 80, true);
        fb_rect_hsv(base_x - 12, base_y - trunk_height - 8, base_x - 8, base_y - trunk_height - 6, 20, 150, 80, true);
        // Right upward branch
        fb_rect_hsv(base_x + 6, base_y - trunk_height - 10, base_x + 8, base_y - trunk_height - 2, 20, 150, 80, true);
        fb_rect_hsv(base_x + 8, base_y - trunk_height - 8, base_x + 12, base_y - trunk_height - 6, 20, 150, 80, true);

        // Middle upward branches (from mid-trunk)
        fb_rect_hsv(base_x - 6, base_y - trunk_height - 6, base_x - 4, base_y - trunk_height + 2, 20, 150, 80, true);
        fb_rect_hsv(base_x + 4, base_y - trunk_height - 6, base_x + 6, base_y - trunk_height + 2, 20, 150, 80, true);

        // Outward angled branches (lower)
        fb_rect_hsv(base_x - 10, base_y - trunk_height + 4, base_x - 8, base_y - trunk_height + 8, 20, 150, 80, true);
        fb_rect_hsv(base_x + 8, base_y - trunk_height + 4, base_x + 10, base_y - trunk_height + 8, 20, 150, 80, true);

        // Smaller upward twigs
        fb_rect_hsv(base_x - 10, base_y - trunk_height - 12, base_x - 9, base_y - trunk_height - 9, 20, 120, 70, true);
        fb_rect_hsv(base_x + 9, base_y - trunk_height - 12, base_x + 10, base_y - trunk_height - 9, 20, 120, 70, true);
        fb_rect_hsv(base_x - 3, base_y - trunk_height - 13, base_x - 2, base_y - trunk_height - 10, 20, 120, 70, true);
        fb_rect_hsv(base_x + 2, base_y - trunk_height - 13, base_x + 3, base_y - trunk_height - 10, 20, 120, 70, true);

        // Side twigs extending from main branches
        fb_rect_hsv(base_x - 14, base_y - trunk_height - 6, base_x - 12, base_y - trunk_height - 4, 20, 120, 70, true);
        fb_rect_hsv(base_x + 12, base_y - trunk_height - 6, base_x + 14, base_y - trunk_height - 4, 20, 120, 70, true);

        // Add snow accumulation on branches (thicker and more coverage)
        // Snow on main upward branches (thicker patches)
        fb_rect_hsv(base_x - 9, base_y - trunk_height - 11, base_x - 5, base_y - trunk_height - 9, 170, 40, 255, true);
        fb_rect_hsv(base_x + 5, base_y - trunk_height - 11, base_x + 9, base_y - trunk_height - 9, 170, 40, 255, true);

        // Snow on horizontal/angled branch sections (larger)
        fb_rect_hsv(base_x - 13, base_y - trunk_height - 9, base_x - 7, base_y - trunk_height - 7, 170, 40, 255, true);
        fb_rect_hsv(base_x + 7, base_y - trunk_height - 9, base_x + 13, base_y - trunk_height - 7, 170, 40, 255, true);

        // Snow on middle branches (thicker)
        fb_rect_hsv(base_x - 7, base_y - trunk_height - 7, base_x - 3, base_y - trunk_height - 5, 170, 40, 255, true);
        fb_rect_hsv(base_x + 3, base_y - trunk_height - 7, base_x + 7, base_y - trunk_height - 5, 170, 40, 255, true);

        // Additional snow on mid-trunk branches
        fb_rect_hsv(base_x - 6, base_y - trunk_height - 3, base_x - 3, base_y - trunk_height - 1, 170, 40, 255, true);
        fb_rect_hsv(base_x + 3, base_y - trunk_height - 3, base_x + 6, base_y - trunk_height - 1, 170, 40, 255, true);

        // Snow on lower outward branches (larger)
        fb_rect_hsv(base_x - 11, base_y - trunk_height + 3, base_x - 7, base_y - trunk_height + 5, 170, 40, 255, true);
        fb_rect_hsv(base_x + 7, base_y - trunk_height + 3, base_x + 11, base_y - trunk_height + 5, 170, 40, 255, true);

        // Additional snow lower down
        fb_rect_hsv(base_x - 9, base_y - trunk_height + 6, base_x - 7, base_y - trunk_height + 8, 170, 40, 255, true);
        fb_rect_hsv(base_x + 7, base_y - trunk_height + 6, base_x + 9, base_y - trunk_height + 8, 170, 40, 255, true);

        // Snow patches on twigs (larger and brighter)
        fb_rect_hsv(base_x - 11, base_y - trunk_height - 13, base_x - 8, base_y - trunk_height - 11, 0, 0, 255, true);
        fb_rect_hsv(base_x + 8, base_y - trunk_height - 13, base_x + 11, base_y - trunk_height - 11, 0, 0, 255, true);
        fb_rect_hsv(base_x - 4, base_y - trunk_height - 14, base_x - 1, base_y - trunk_height - 12, 0, 0, 255, true);
        fb_rect_hsv(base_x + 1, base_y - trunk_height - 14, base_x + 4, base_y - trunk_height - 12, 0, 0, 255, true);

        // Side twig snow
        fb_rect_hsv(base_x - 15, base_y - trunk_height - 7, base_x - 11, base_y - trunk_height - 5, 170, 40, 255, true);
        fb_rect_hsv(base_x + 11, base_y - trunk_height - 7, base_x + 15, base_y - trunk_height - 5, 170, 40, 255, true);
    } else if (season == 1) { // Spring - green leaves with pink blossoms
        // Tree shape with green base
        fb_circle_hsv(base_x, base_y - trunk_height - 7, 15, 85, 220, 200, true); // Light green
        // Add leaf and blossom dots (smaller, mostly pink blossoms)
        for (uint8_t i = 0; i < 9; i++) {
            int8_t offset_x = (i % 3 - 1) * 7;
            int8_t offset_y = (i / 3 - 1) * 7;
            // Make 8 dots pink blossoms, only dot 4 (center) is green leaf
            if (i != 4) {
                fb_circle_hsv(base_x + offset_x, base_y - trunk_height - 7 + offset_y, 2, 234, 255, 220, true); // Pink blossom (smaller)
            } else {
                fb_circle_hsv(base_x + offset_x, base_y - trunk_height - 7 + offset_y, 2, 85, 255, 180, true); // Green leaf (smaller)
            }
        }
    } else if (season == 2) { // Summer - cherry tree with cherries
        // Dense green canopy
        fb_circle_hsv(base_x, base_y - trunk_height - 7, 16, 85, 255, 200, true);       // Center
        fb_circle_hsv(base_x - 9, base_y - trunk_height - 4, 11, 85, 255, 180, true);  // Left
        fb_circle_hsv(base_x + 9, base_y - trunk_height - 4, 11, 85, 255, 180, true);  // Right

        // Add red cherries scattered throughout the entire canopy
        // Cherry positions relative to canopy center at (base_x, base_y - trunk_height - 7)
        // Canopy extends from y=-16 (top) to y=+14 (bottom on sides)
        int8_t cherry_offsets[][2] = {
            // Top area (y: -16 to -9)
            {-4, -14}, {2, -13}, {-9, -11}, {6, -12}, {-1, -10},
            // Middle area (y: -8 to -1)
            {-12, -5}, {-6, -3}, {0, -4}, {8, -2}, {13, -6},
            // Lower area (y: 0 to +12)
            {-14, 3}, {-8, 8}, {-2, 10}, {4, 9}, {10, 6}, {15, 4}
        };

        for (uint8_t i = 0; i < 16; i++) {
            // Draw cherries (bright red, small circles)
            fb_circle_hsv(base_x + cherry_offsets[i][0],
                     base_y - trunk_height - 7 + cherry_offsets[i][1],
                     2, 0, 255, 220, true);
        }
    } else { // Fall - orange/red/yellow leaves
        // Tree shape with autumn colors
        fb_circle_hsv(base_x, base_y - trunk_height - 7, 15, 20, 255, 200, true);      // Orange
        fb_circle_hsv(base_x - 8, base_y - trunk_height - 4, 10, 10, 255, 220, true);  // Red-orange
        fb_circle_hsv(base_x + 8, base_y - trunk_height - 4, 10, 30, 255, 200, true);  // Yellow-orange
    }
}
