// Copyright 2025 @temanyl
// SPDX-License-Identifier: GPL-2.0-or-later

#include "astronomical.h"

/**
 * @brief Lookup table for sunrise/sunset times in Northern Germany (53.5°N)
 *
 * Data points for the 1st and 15th of each month (24 entries)
 * Each entry: {sunrise_minutes_from_midnight, sunset_minutes_from_midnight}
 *
 * Based on Hamburg, Germany astronomical data:
 * - Latitude: 53.5°N
 * - Longitude: 10°E (UTC+1, CET)
 */
typedef struct {
    uint16_t sunrise_minutes;  // Minutes from midnight (e.g., 6:00 = 360)
    uint16_t sunset_minutes;   // Minutes from midnight (e.g., 18:00 = 1080)
} day_times_t;

static const day_times_t ASTRONOMICAL_TABLE[] = {
    // January
    {508, 1000},  // Jan 1:  ~8:28 sunrise, ~16:40 sunset (7h 12m daylight)
    {500, 1025},  // Jan 15: ~8:20 sunrise, ~17:05 sunset (7h 45m daylight)

    // February
    {480, 1060},  // Feb 1:  ~8:00 sunrise, ~17:40 sunset (8h 40m daylight)
    {445, 1095},  // Feb 15: ~7:25 sunrise, ~18:15 sunset (9h 50m daylight)

    // March
    {400, 1125},  // Mar 1:  ~6:40 sunrise, ~18:45 sunset (11h 05m daylight)
    {345, 1160},  // Mar 15: ~5:45 sunrise, ~19:20 sunset (12h 35m daylight)

    // April
    {290, 1200},  // Apr 1:  ~4:50 sunrise, ~20:00 sunset (14h 10m daylight) [DST starts]
    {245, 1235},  // Apr 15: ~4:05 sunrise, ~20:35 sunset (15h 30m daylight)

    // May
    {210, 1270},  // May 1:  ~3:30 sunrise, ~21:10 sunset (16h 40m daylight)
    {185, 1300},  // May 15: ~3:05 sunrise, ~21:40 sunset (17h 35m daylight)

    // June
    {175, 1315},  // Jun 1:  ~2:55 sunrise, ~21:55 sunset (18h 00m daylight)
    {175, 1320},  // Jun 15: ~2:55 sunrise, ~22:00 sunset (18h 05m daylight) [longest day]

    // July
    {180, 1315},  // Jul 1:  ~3:00 sunrise, ~21:55 sunset (17h 55m daylight)
    {200, 1295},  // Jul 15: ~3:20 sunrise, ~21:35 sunset (17h 15m daylight)

    // August
    {230, 1260},  // Aug 1:  ~3:50 sunrise, ~21:00 sunset (16h 10m daylight)
    {265, 1215},  // Aug 15: ~4:25 sunrise, ~20:15 sunset (14h 50m daylight)

    // September
    {300, 1165},  // Sep 1:  ~5:00 sunrise, ~19:25 sunset (13h 25m daylight)
    {340, 1110},  // Sep 15: ~5:40 sunrise, ~18:30 sunset (11h 50m daylight)

    // October
    {375, 1060},  // Oct 1:  ~6:15 sunrise, ~17:40 sunset (10h 25m daylight) [DST ends]
    {415, 1010},  // Oct 15: ~6:55 sunrise, ~16:50 sunset (8h 55m daylight)

    // November
    {455, 970},   // Nov 1:  ~7:35 sunrise, ~16:10 sunset (7h 35m daylight)
    {490, 950},   // Nov 15: ~8:10 sunrise, ~15:50 sunset (6h 40m daylight)

    // December
    {510, 945},   // Dec 1:  ~8:30 sunrise, ~15:45 sunset (6h 15m daylight)
    {515, 950},   // Dec 15: ~8:35 sunrise, ~15:50 sunset (6h 15m daylight) [shortest day]
};

// Days in each month (non-leap year)
static const uint8_t DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Cumulative days to start of each month
static const uint16_t DAYS_TO_MONTH[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

uint16_t astronomical_get_day_of_year(uint8_t month, uint8_t day) {
    if (month < 1 || month > 12) return 1;
    if (day < 1) day = 1;
    if (day > DAYS_IN_MONTH[month - 1]) day = DAYS_IN_MONTH[month - 1];

    return DAYS_TO_MONTH[month - 1] + day;
}

/**
 * @brief Linear interpolation helper
 *
 * @param x Current value
 * @param x0 Start value
 * @param x1 End value
 * @param y0 Start output
 * @param y1 End output
 * @return Interpolated output
 */
static uint16_t interpolate(uint16_t x, uint16_t x0, uint16_t x1, uint16_t y0, uint16_t y1) {
    if (x <= x0) return y0;
    if (x >= x1) return y1;

    // Linear interpolation: y = y0 + (y1-y0) * (x-x0) / (x1-x0)
    int32_t numerator = (int32_t)(y1 - y0) * (int32_t)(x - x0);
    int32_t denominator = (int32_t)(x1 - x0);

    return y0 + (uint16_t)(numerator / denominator);
}

void astronomical_calculate_times(uint8_t month, uint8_t day, astronomical_times_t *times) {
    if (!times) return;

    // Get day of year (1-365)
    uint16_t day_of_year = astronomical_get_day_of_year(month, day);

    // Find surrounding table entries (entries are 15 days apart, starting Jan 1)
    // Table indices: 0=Jan1, 1=Jan15, 2=Feb1, 3=Feb15, etc.

    uint16_t table_index = 0;
    uint16_t days_to_index = 1;  // Day of year for current table index

    // Find the table index just before or at our date
    for (uint8_t i = 0; i < 24; i++) {
        uint16_t month_idx = i / 2;  // Which month (0=Jan, 1=Feb, etc.)
        uint16_t is_mid_month = i % 2;  // 0 = start of month, 1 = mid month

        uint16_t day_in_month = is_mid_month ? 15 : 1;
        uint16_t test_day_of_year = DAYS_TO_MONTH[month_idx] + day_in_month;

        if (test_day_of_year <= day_of_year) {
            table_index = i;
            days_to_index = test_day_of_year;
        } else {
            break;
        }
    }

    // Get next table index (wrap around at end of year)
    uint16_t next_index = (table_index + 1) % 24;

    // Calculate day of year for next index
    uint16_t next_month_idx = next_index / 2;
    uint16_t next_is_mid_month = next_index % 2;
    uint16_t next_day_in_month = next_is_mid_month ? 15 : 1;
    uint16_t days_to_next_index = DAYS_TO_MONTH[next_month_idx] + next_day_in_month;

    // Handle year wrap
    if (days_to_next_index < days_to_index) {
        days_to_next_index += 365;
    }

    // Interpolate between table entries
    uint16_t sunrise_minutes = interpolate(
        day_of_year,
        days_to_index,
        days_to_next_index,
        ASTRONOMICAL_TABLE[table_index].sunrise_minutes,
        ASTRONOMICAL_TABLE[next_index].sunrise_minutes
    );

    uint16_t sunset_minutes = interpolate(
        day_of_year,
        days_to_index,
        days_to_next_index,
        ASTRONOMICAL_TABLE[table_index].sunset_minutes,
        ASTRONOMICAL_TABLE[next_index].sunset_minutes
    );

    // Convert to hours and minutes
    times->sunrise_hour = sunrise_minutes / 60;
    times->sunrise_minute = sunrise_minutes % 60;
    times->sunset_hour = sunset_minutes / 60;
    times->sunset_minute = sunset_minutes % 60;

    // Calculate solar noon (midpoint between sunrise and sunset)
    uint16_t solar_noon_minutes = (sunrise_minutes + sunset_minutes) / 2;
    times->solar_noon_hour = solar_noon_minutes / 60;
    times->solar_noon_minute = solar_noon_minutes % 60;

    // Calculate daylight duration
    times->daylight_minutes = sunset_minutes - sunrise_minutes;
}

bool astronomical_is_daytime(uint8_t hour, uint8_t minute, const astronomical_times_t *times) {
    if (!times) return false;

    uint16_t current_minutes = (uint16_t)hour * 60 + minute;
    uint16_t sunrise_minutes = (uint16_t)times->sunrise_hour * 60 + times->sunrise_minute;
    uint16_t sunset_minutes = (uint16_t)times->sunset_hour * 60 + times->sunset_minute;

    return (current_minutes >= sunrise_minutes && current_minutes < sunset_minutes);
}

uint8_t astronomical_get_cycle_progress(uint8_t hour, uint8_t minute, const astronomical_times_t *times) {
    if (!times) return 0;

    uint16_t current_minutes = (uint16_t)hour * 60 + minute;
    uint16_t sunrise_minutes = (uint16_t)times->sunrise_hour * 60 + times->sunrise_minute;
    uint16_t sunset_minutes = (uint16_t)times->sunset_hour * 60 + times->sunset_minute;

    if (astronomical_is_daytime(hour, minute, times)) {
        // Daytime: 0 at sunrise, 255 at sunset
        uint16_t daylight_duration = sunset_minutes - sunrise_minutes;
        uint16_t minutes_since_sunrise = current_minutes - sunrise_minutes;

        if (daylight_duration == 0) return 128;  // Prevent division by zero

        // Scale to 0-255
        uint32_t progress = ((uint32_t)minutes_since_sunrise * 255) / daylight_duration;
        return (uint8_t)(progress > 255 ? 255 : progress);
    } else {
        // Nighttime: 0 at sunset, 255 at next sunrise
        uint16_t night_duration = (24 * 60) - (sunset_minutes - sunrise_minutes);
        uint16_t minutes_since_sunset;

        if (current_minutes >= sunset_minutes) {
            // Evening/night (sunset to midnight)
            minutes_since_sunset = current_minutes - sunset_minutes;
        } else {
            // Early morning (midnight to sunrise)
            minutes_since_sunset = (24 * 60 - sunset_minutes) + current_minutes;
        }

        if (night_duration == 0) return 128;  // Prevent division by zero

        // Scale to 0-255
        uint32_t progress = ((uint32_t)minutes_since_sunset * 255) / night_duration;
        return (uint8_t)(progress > 255 ? 255 : progress);
    }
}
