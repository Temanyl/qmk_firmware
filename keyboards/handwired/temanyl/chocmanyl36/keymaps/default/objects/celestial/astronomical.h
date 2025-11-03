// Copyright 2025 @temanyl
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @file astronomical.h
 * @brief Astronomical calculations for realistic day/night cycles
 *
 * This module calculates sunrise and sunset times based on:
 * - Day of year (1-365/366)
 * - Month (1-12)
 * - Latitude of Northern Germany (~53.5°N)
 *
 * The calculations provide seasonal variation in day length:
 * - Summer solstice (June 21): ~17 hours of daylight
 * - Winter solstice (Dec 21): ~7.5 hours of daylight
 * - Equinoxes (Mar 20, Sep 23): ~12 hours of daylight
 */

/**
 * @brief Astronomical calculation result
 */
typedef struct {
    uint8_t sunrise_hour;      // Hour of sunrise (0-23)
    uint8_t sunrise_minute;    // Minute of sunrise (0-59)
    uint8_t sunset_hour;       // Hour of sunset (0-23)
    uint8_t sunset_minute;     // Minute of sunset (0-59)
    uint8_t solar_noon_hour;   // Hour of solar noon (0-23)
    uint8_t solar_noon_minute; // Minute of solar noon (0-59)
    uint16_t daylight_minutes; // Total daylight duration in minutes
} astronomical_times_t;

/**
 * @brief Calculate sunrise and sunset times for Northern Germany
 *
 * Uses a simplified astronomical algorithm suitable for embedded systems.
 * Latitude: 53.5°N (Hamburg, Germany)
 * Longitude: 10°E (for solar noon calculation)
 *
 * @param month Current month (1-12)
 * @param day Current day of month (1-31)
 * @param times Output structure to store calculated times
 */
void astronomical_calculate_times(uint8_t month, uint8_t day, astronomical_times_t *times);

/**
 * @brief Check if current time is during daytime
 *
 * @param hour Current hour (0-23)
 * @param minute Current minute (0-59)
 * @param times Astronomical times calculated for current date
 * @return true if it's daytime, false if nighttime
 */
bool astronomical_is_daytime(uint8_t hour, uint8_t minute, const astronomical_times_t *times);

/**
 * @brief Get progress through the day/night cycle (0-255)
 *
 * Returns a value representing how far through the day or night we are:
 * - During day: 0 at sunrise, 128 at solar noon, 255 at sunset
 * - During night: 0 at sunset, 128 at midnight, 255 at sunrise
 *
 * @param hour Current hour (0-23)
 * @param minute Current minute (0-59)
 * @param times Astronomical times calculated for current date
 * @return Progress value (0-255)
 */
uint8_t astronomical_get_cycle_progress(uint8_t hour, uint8_t minute, const astronomical_times_t *times);

/**
 * @brief Get day of year from month and day
 *
 * @param month Month (1-12)
 * @param day Day of month (1-31)
 * @return Day of year (1-365, ignoring leap years for simplicity)
 */
uint16_t astronomical_get_day_of_year(uint8_t month, uint8_t day);
