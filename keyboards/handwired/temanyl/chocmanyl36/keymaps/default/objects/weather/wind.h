// Copyright 2025 BBT (@temanyl)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Wind intensity levels
typedef enum {
    WIND_NONE = 0,    // No wind - smoke goes straight up
    WIND_LIGHT = 1,   // Light breeze - slight drift
    WIND_MEDIUM = 2,  // Medium wind - moderate drift
    WIND_HIGH = 3     // High wind - strong drift
} wind_intensity_t;

// Wind direction (horizontal only)
typedef enum {
    WIND_LEFT = 0,    // Wind blowing left (from right to left)
    WIND_RIGHT = 1    // Wind blowing right (from left to right)
} wind_direction_t;

// Wind state structure
typedef struct {
    wind_intensity_t intensity;
    wind_direction_t direction;
} wind_state_t;

// Wind system initialization and control
void wind_init(void);
void wind_set_intensity(wind_intensity_t intensity);
void wind_set_direction(wind_direction_t direction);
void wind_set_state(wind_intensity_t intensity, wind_direction_t direction);

// Wind state queries
wind_intensity_t wind_get_intensity(void);
wind_direction_t wind_get_direction(void);
wind_state_t wind_get_state(void);

// Wind effect calculations
// Returns horizontal velocity for clouds (negative = left, positive = right)
int8_t wind_get_cloud_velocity(void);

// Returns horizontal drift for raindrops (pixels per update)
int8_t wind_get_rain_drift(void);

// Returns smoke drift speed (0 = straight up, higher = more drift)
int8_t wind_get_smoke_drift(void);

// Returns smoke horizontal direction (-1 = left, 0 = none, 1 = right)
int8_t wind_get_smoke_direction(void);
