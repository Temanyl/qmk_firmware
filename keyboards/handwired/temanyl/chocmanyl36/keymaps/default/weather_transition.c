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

#include "weather_transition.h"
#include "timer.h"

// Global weather transition state
weather_transition_t weather_transition = {
    .current_weather = WEATHER_SUNNY,
    .target_weather = WEATHER_SUNNY,
    .transition_active = false,
    .transition_progress = 0,
    .transition_timer = 0,
    .transition_duration = 30000  // 30 seconds default
};

// Snow accumulation state
snow_accumulation_t snow_accumulation = {
    .ground_coverage = 0,
    .tree_coverage = 0,
    .cabin_coverage = 0
};

/**
 * Initialize weather transition system
 */
void weather_transition_init(void) {
    weather_transition.current_weather = WEATHER_SUNNY;
    weather_transition.target_weather = WEATHER_SUNNY;
    weather_transition.transition_active = false;
    weather_transition.transition_progress = 0;
    weather_transition.transition_timer = timer_read32();
    snow_accumulation_reset();
}

/**
 * Set target weather and start transition
 */
void weather_transition_set_target(weather_state_t target) {
    // If already at target weather, do nothing
    if (weather_transition.current_weather == target && !weather_transition.transition_active) {
        return;
    }

    // Set target weather
    weather_transition.target_weather = target;

    // Change current weather IMMEDIATELY for particle rendering
    weather_transition.current_weather = target;

    // Set snow accumulation IMMEDIATELY
    if (target == WEATHER_SNOW) {
        snow_accumulation.ground_coverage = 255;
        snow_accumulation.tree_coverage = 255;
        snow_accumulation.cabin_coverage = 255;
    } else {
        snow_accumulation.ground_coverage = 0;
        snow_accumulation.tree_coverage = 0;
        snow_accumulation.cabin_coverage = 0;
    }

    // No transition needed - everything is instant
    weather_transition.transition_active = false;
    weather_transition.transition_progress = 255;
}

/**
 * Update weather transition progress
 * Should be called regularly from main loop
 * Returns true if transition just completed (to trigger redraw)
 */
bool weather_transition_update(void) {
    // No gradual transitions - everything is instant
    return false;
}

/**
 * Get current transition progress (0-255)
 */
uint8_t weather_transition_get_progress(void) {
    return weather_transition.transition_progress;
}

/**
 * Get current weather state
 */
weather_state_t weather_transition_get_current(void) {
    return weather_transition.current_weather;
}

/**
 * Check if transition is active
 */
bool weather_transition_is_active(void) {
    return weather_transition.transition_active;
}

/**
 * Reset snow accumulation to zero
 */
void snow_accumulation_reset(void) {
    snow_accumulation.ground_coverage = 0;
    snow_accumulation.tree_coverage = 0;
    snow_accumulation.cabin_coverage = 0;
}

/**
 * Update snow accumulation based on transition progress
 * progress: 0-255 transition progress
 */
void snow_accumulation_update(uint8_t progress) {
    // Determine if we're transitioning to snow or away from snow
    // Since current_weather changes immediately, we check target_weather
    bool transitioning_to_snow = (weather_transition.target_weather == WEATHER_SNOW);
    bool transitioning_from_snow = (weather_transition.target_weather != WEATHER_SNOW &&
                                    snow_accumulation.ground_coverage > 0);

    if (transitioning_to_snow) {
        // Accumulating snow
        // Ground accumulates fastest, trees medium, cabin slowest
        snow_accumulation.ground_coverage = progress;

        // Trees start accumulating after ground reaches 30%
        if (progress > 76) {  // 76/255 ≈ 30%
            uint8_t tree_progress = ((uint32_t)(progress - 76) * 255) / (255 - 76);
            snow_accumulation.tree_coverage = tree_progress;
        } else {
            snow_accumulation.tree_coverage = 0;
        }

        // Cabin starts accumulating after ground reaches 50%
        if (progress > 127) {  // 127/255 ≈ 50%
            uint8_t cabin_progress = ((uint32_t)(progress - 127) * 255) / (255 - 127);
            snow_accumulation.cabin_coverage = cabin_progress;
        } else {
            snow_accumulation.cabin_coverage = 0;
        }

    } else if (transitioning_from_snow) {
        // Melting snow (reverse order: cabin melts first, then trees, then ground)
        uint8_t melt_progress = progress;  // 0 = full snow, 255 = no snow

        // Cabin melts first (0-30% of transition)
        if (melt_progress < 76) {  // 76/255 ≈ 30%
            uint8_t cabin_melt = (melt_progress * 255) / 76;
            snow_accumulation.cabin_coverage = 255 - cabin_melt;
        } else {
            snow_accumulation.cabin_coverage = 0;
        }

        // Trees melt next (30-70% of transition)
        if (melt_progress < 178) {  // 178/255 ≈ 70%
            if (melt_progress < 76) {
                snow_accumulation.tree_coverage = 255;
            } else {
                uint8_t tree_melt = ((melt_progress - 76) * 255) / (178 - 76);
                snow_accumulation.tree_coverage = 255 - tree_melt;
            }
        } else {
            snow_accumulation.tree_coverage = 0;
        }

        // Ground melts last (entire transition)
        snow_accumulation.ground_coverage = 255 - melt_progress;
    }
}

/**
 * Get snow accumulation levels (0-255)
 */
uint8_t snow_accumulation_get_ground(void) {
    return snow_accumulation.ground_coverage;
}

uint8_t snow_accumulation_get_tree(void) {
    return snow_accumulation.tree_coverage;
}

uint8_t snow_accumulation_get_cabin(void) {
    return snow_accumulation.cabin_coverage;
}
