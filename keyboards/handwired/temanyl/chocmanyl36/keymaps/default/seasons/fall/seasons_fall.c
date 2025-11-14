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
#include "seasons_fall.h"
#include "../../weather_effects.h"
#include "../../objects/flora/fallen_leaf.h"

// Draw RAIN WEATHER effects (weather-based, not seasonal)
// This is a thin wrapper that delegates to the unified weather effects module
void draw_rain_weather_elements(void) {
    // Initialize clouds (shared between rain and snow)
    weather_clouds_init();

    // Initialize rain
    weather_rain_init();
}

// Draw fall-specific scene elements (SEASONAL - only decorations, no weather)
void draw_fall_scene_elements(void) {
    // Draw fallen leaves on the ground (seasonal decoration)
    fallen_leaves_draw_all();
}
