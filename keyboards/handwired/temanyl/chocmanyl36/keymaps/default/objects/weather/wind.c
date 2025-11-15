// Copyright 2025 BBT (@temanyl)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "wind.h"

// Global wind state
static wind_state_t current_wind = {
    .intensity = WIND_NONE,
    .direction = WIND_RIGHT
};

// Initialize wind system
void wind_init(void) {
    current_wind.intensity = WIND_NONE;
    current_wind.direction = WIND_RIGHT;
}

// Set wind intensity
void wind_set_intensity(wind_intensity_t intensity) {
    if (intensity <= WIND_HIGH) {
        current_wind.intensity = intensity;
    }
}

// Set wind direction
void wind_set_direction(wind_direction_t direction) {
    if (direction <= WIND_RIGHT) {
        current_wind.direction = direction;
    }
}

// Set both intensity and direction
void wind_set_state(wind_intensity_t intensity, wind_direction_t direction) {
    wind_set_intensity(intensity);
    wind_set_direction(direction);
}

// Get current wind intensity
wind_intensity_t wind_get_intensity(void) {
    return current_wind.intensity;
}

// Get current wind direction
wind_direction_t wind_get_direction(void) {
    return current_wind.direction;
}

// Get full wind state
wind_state_t wind_get_state(void) {
    return current_wind;
}

// Calculate cloud velocity based on wind
// Negative = left movement, Positive = right movement
int8_t wind_get_cloud_velocity(void) {
    // Base velocity when no wind
    if (current_wind.intensity == WIND_NONE) {
        return -1;  // Slow drift left
    }

    // Calculate velocity based on intensity and direction
    int8_t base_speed;
    switch (current_wind.intensity) {
        case WIND_LIGHT:
            base_speed = 2;
            break;
        case WIND_MEDIUM:
            base_speed = 3;
            break;
        case WIND_HIGH:
            base_speed = 4;
            break;
        default:
            base_speed = 1;
            break;
    }

    // Apply direction
    return (current_wind.direction == WIND_LEFT) ? -base_speed : base_speed;
}

// Calculate raindrop horizontal drift
// Negative = drift left, Positive = drift right, 0 = straight down
int8_t wind_get_rain_drift(void) {
    if (current_wind.intensity == WIND_NONE) {
        return 0;  // No drift - straight down
    }

    // Calculate drift based on intensity
    int8_t drift;
    switch (current_wind.intensity) {
        case WIND_LIGHT:
            drift = 1;  // 1 pixel per update
            break;
        case WIND_MEDIUM:
            drift = 2;  // 2 pixels per update
            break;
        case WIND_HIGH:
            drift = 3;  // 3 pixels per update
            break;
        default:
            drift = 0;
            break;
    }

    // Apply direction
    return (current_wind.direction == WIND_LEFT) ? -drift : drift;
}

// Calculate smoke drift speed (used for timing)
// 0 = straight up, 1 = slow drift, 2 = medium drift, 3 = fast drift
int8_t wind_get_smoke_drift(void) {
    switch (current_wind.intensity) {
        case WIND_NONE:
            return 0;  // No drift - straight up
        case WIND_LIGHT:
            return 1;  // Slow drift (every 48 age-ticks)
        case WIND_MEDIUM:
            return 2;  // Medium drift (every 24 age-ticks)
        case WIND_HIGH:
            return 3;  // Fast drift (every 12 age-ticks)
        default:
            return 0;
    }
}

// Get smoke horizontal direction
// -1 = left, 0 = none, 1 = right
int8_t wind_get_smoke_direction(void) {
    if (current_wind.intensity == WIND_NONE) {
        return 0;  // No direction - straight up
    }

    return (current_wind.direction == WIND_LEFT) ? -1 : 1;
}
