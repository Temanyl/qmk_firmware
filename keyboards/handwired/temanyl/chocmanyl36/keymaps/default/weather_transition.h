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

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Weather states
typedef enum {
    WEATHER_SUNNY = 0,
    WEATHER_RAIN_LIGHT = 1,
    WEATHER_RAIN_MEDIUM = 2,
    WEATHER_RAIN_HEAVY = 3,
    WEATHER_SNOW_LIGHT = 4,
    WEATHER_SNOW_MEDIUM = 5,
    WEATHER_SNOW_HEAVY = 6,
    WEATHER_CLOUDY = 7,      // Partly cloudy (2 white clouds, no precipitation)
    WEATHER_OVERCAST = 8     // Overcast (5 white clouds, no precipitation)
} weather_state_t;

// Legacy aliases for backward compatibility
#define WEATHER_RAIN WEATHER_RAIN_MEDIUM
#define WEATHER_SNOW WEATHER_SNOW_MEDIUM

// Weather transition system
typedef struct {
    weather_state_t current_weather;
    weather_state_t target_weather;
    bool transition_active;
    uint8_t transition_progress;  // 0-255, like day/night transition
    uint32_t transition_timer;
    uint32_t transition_duration;  // Duration in milliseconds
} weather_transition_t;

// Snow accumulation tracking (0-255)
typedef struct {
    uint8_t ground_coverage;
    uint8_t tree_coverage;
    uint8_t cabin_coverage;
} snow_accumulation_t;

// Global weather transition state
extern weather_transition_t weather_transition;
extern snow_accumulation_t snow_accumulation;

// Weather transition functions
void weather_transition_init(uint8_t month);
void weather_transition_set_target(weather_state_t target);
bool weather_transition_update(void);  // Returns true if transition just completed
uint8_t weather_transition_get_progress(void);
weather_state_t weather_transition_get_current(void);
bool weather_transition_is_active(void);

// Weather check helpers
static inline bool weather_is_raining(weather_state_t weather) {
    return weather >= WEATHER_RAIN_LIGHT && weather <= WEATHER_RAIN_HEAVY;
}

static inline bool weather_is_snowing(weather_state_t weather) {
    return weather >= WEATHER_SNOW_LIGHT && weather <= WEATHER_SNOW_HEAVY;
}

static inline bool weather_is_cloudy(weather_state_t weather) {
    return weather == WEATHER_CLOUDY || weather == WEATHER_OVERCAST;
}

static inline bool weather_has_clouds(weather_state_t weather) {
    return weather_is_raining(weather) || weather_is_snowing(weather) || weather_is_cloudy(weather);
}

static inline uint8_t weather_get_rain_intensity(weather_state_t weather) {
    if (weather == WEATHER_RAIN_LIGHT) return 1;
    if (weather == WEATHER_RAIN_MEDIUM) return 2;
    if (weather == WEATHER_RAIN_HEAVY) return 3;
    return 0;
}

static inline uint8_t weather_get_snow_intensity(weather_state_t weather) {
    if (weather == WEATHER_SNOW_LIGHT) return 1;
    if (weather == WEATHER_SNOW_MEDIUM) return 2;
    if (weather == WEATHER_SNOW_HEAVY) return 3;
    return 0;
}

// Snow accumulation functions
void snow_accumulation_reset(void);
void snow_accumulation_update(uint8_t progress);
uint8_t snow_accumulation_get_ground(void);
uint8_t snow_accumulation_get_tree(void);
uint8_t snow_accumulation_get_cabin(void);
