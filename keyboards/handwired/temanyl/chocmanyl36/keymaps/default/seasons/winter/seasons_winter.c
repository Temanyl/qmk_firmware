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
#include "seasons_winter.h"
#include "../../weather_effects.h"

// Draw SNOW WEATHER effects (weather-based, not seasonal)
// This is a thin wrapper that delegates to the unified weather effects module
void draw_snow_weather_elements(void) {
    // Initialize clouds (shared between rain and snow)
    weather_clouds_init();

    // Initialize snowflakes
    weather_snow_init();

    // Draw snow ground effects (drifts and snowman)
    weather_snow_draw_ground_effects();
}

// Draw winter-specific scene elements (SEASONAL - only decorations, no weather)
void draw_winter_scene_elements(void) {
    // Winter season: bare trees (no leaves)
    // Trees are already drawn with season=0 parameter in scenes.c
    // No additional seasonal decorations needed for winter
}
