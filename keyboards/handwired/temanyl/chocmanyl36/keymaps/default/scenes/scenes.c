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
#include <stdio.h>
#include <string.h>
#include "scenes.h"
#include "../display/display.h"
#include "../display/framebuffer.h"
#include "../display/draw_logo.h"

// Include season-specific modules
#include "../seasons/winter/seasons_winter.h"
#include "../seasons/spring/seasons_spring.h"
#include "../seasons/summer/seasons_summer.h"
#include "../seasons/fall/seasons_fall.h"
#include "../seasons/halloween/seasons_halloween.h"
#include "../seasons/christmas/seasons_christmas.h"
#include "../seasons/easter/seasons_easter.h"

// Include drawable objects
#include "../objects/weather/smoke.h"
#include "../objects/weather/cloud.h"
#include "../objects/weather/raindrop.h"
#include "../objects/celestial/sun.h"
#include "../objects/celestial/moon.h"
#include "../objects/celestial/stars.h"
#include "../objects/celestial/astronomical.h"
#include "../objects/structures/tree.h"
#include "../objects/structures/cabin.h"
#include "../objects/fauna/bird.h"
#include "../objects/fauna/butterfly.h"
#include "../objects/fauna/bee.h"
#include "../objects/fauna/firefly.h"

// External object arrays from season-specific files
extern bird_t birds[];
extern butterfly_t butterflies[];
extern bee_t bees[];
extern firefly_t fireflies[];

// Array sizes from season files
#define NUM_SPRING_BIRDS 6
#define NUM_SPRING_BUTTERFLIES 8
#define NUM_SUMMER_BEES 5
#define NUM_SUMMER_FIREFLIES 12

// Smoke animation state (shared across seasons)
bool smoke_initialized = false;
bool smoke_background_saved = false;
smoke_particle_t smoke_particles[NUM_SMOKE_PARTICLES];
uint32_t smoke_animation_timer = 0;
uint32_t smoke_spawn_timer = 0;

// Forward declarations from display.c
extern uint8_t current_hour;
extern uint8_t current_minute;
extern uint8_t current_day;
extern uint8_t current_month;
extern painter_device_t display;

// Astronomical times cache
static astronomical_times_t cached_astro_times;
static uint8_t cached_astro_day = 0;
static uint8_t cached_astro_month = 0;

// Reset all scene animation states
void reset_scene_animations(void) {
    reset_winter_animations();
    reset_spring_animations();
    reset_summer_animations();
    reset_fall_animations();
    reset_halloween_animations();
    reset_christmas_animations();
    reset_easter_animations();
    smoke_initialized = false;
    smoke_background_saved = false;
}

// Get season based on month
uint8_t get_season(uint8_t month) {
    if (month == 12 || month <= 2) return 0; // Winter
    else if (month >= 3 && month <= 5) return 1; // Spring
    else if (month >= 6 && month <= 8) return 2; // Summer
    else return 3; // Fall
}

// Forward declaration from display.h
extern void get_layer_color(uint8_t layer, uint8_t *hue, uint8_t *sat, uint8_t *val);

// Draw a tree with seasonal variations (wrapper for compatibility)
void draw_tree(uint16_t base_x, uint16_t base_y, uint8_t season, uint8_t hue, uint8_t sat, uint8_t val) {
    tree_t tree;
    tree_init(&tree, base_x, base_y, season, hue, sat, val);
    tree_draw(&tree);
}

// Draw a cabin with seasonal variations (wrapper for compatibility)
void draw_cabin(uint16_t base_x, uint16_t base_y, uint8_t season) {
    cabin_t cabin;
    cabin_init(&cabin, base_x, base_y, season);
    cabin_draw(&cabin);

    // Initialize smoke animation if not summer
    if (season != 2) {
        if (!smoke_initialized) {
            init_smoke();
        }
    }
}

// Get current astronomical times (with caching)
static const astronomical_times_t* get_astronomical_times(void) {
    // Recalculate if date has changed
    if (cached_astro_day != current_day || cached_astro_month != current_month) {
        astronomical_calculate_times(current_month, current_day, &cached_astro_times);
        cached_astro_day = current_day;
        cached_astro_month = current_month;
    }
    return &cached_astro_times;
}

// Get celestial object (sun/moon) position based on time with realistic astronomical calculations
void get_celestial_position(uint8_t hour, uint16_t *x, uint16_t *y) {
    const astronomical_times_t *astro = get_astronomical_times();

    // Screen bounds for celestial motion
    const uint16_t x_min = 15;
    const uint16_t x_max = 120;
    const uint16_t y_peak = 15;     // Peak altitude (noon/midnight) - highest in sky
    const uint16_t y_horizon = 50;  // Horizon level (sunrise/sunset) - lowest visible

    bool is_daytime = astronomical_is_daytime(hour, current_minute, astro);

    if (is_daytime) {
        // DAYTIME: Sun moves from sunrise to sunset
        // Progress: 0 at sunrise, 128 at solar noon, 255 at sunset
        uint8_t day_progress = astronomical_get_cycle_progress(hour, current_minute, astro);

        // X position: linear movement from left to right across day
        *x = x_min + ((uint32_t)day_progress * (x_max - x_min)) / 255;

        // Y position: parabolic arc, lowest at solar noon
        // At sunrise/sunset: y = y_horizon
        // At solar noon: y = y_peak
        // Use parabolic curve: y = y_peak + (y_horizon - y_peak) * (2*progress/255 - 1)^2
        int16_t progress_centered = (int16_t)day_progress - 128;  // Range: -128 to +127
        // Square the centered progress to get parabola (lowest at center)
        uint32_t arc_factor = (uint32_t)progress_centered * (uint32_t)progress_centered;  // 0 to 16384
        // Scale to y range
        *y = y_peak + ((y_horizon - y_peak) * arc_factor) / 16384;

    } else {
        // NIGHTTIME: Moon moves from sunset to next sunrise
        // Progress: 0 at sunset, 128 at midnight, 255 at sunrise
        uint8_t night_progress = astronomical_get_cycle_progress(hour, current_minute, astro);

        // X position: linear movement from left to right across night
        *x = x_min + ((uint32_t)night_progress * (x_max - x_min)) / 255;

        // Y position: parabolic arc, lowest at midnight
        // At sunset/sunrise: y = y_horizon
        // At midnight: y = y_peak + 5 (moon is slightly lower than sun's peak)
        int16_t progress_centered = (int16_t)night_progress - 128;  // Range: -128 to +127
        uint32_t arc_factor = (uint32_t)progress_centered * (uint32_t)progress_centered;  // 0 to 16384
        // Moon peak is slightly lower than sun peak
        uint16_t moon_y_peak = y_peak + 5;
        *y = moon_y_peak + ((y_horizon - moon_y_peak) * arc_factor) / 16384;
    }

    // Final clamping for safety
    if (*x < x_min) *x = x_min;
    if (*x > x_max) *x = x_max;
    if (*y < y_peak) *y = y_peak;
    if (*y > y_horizon) *y = y_horizon;
}

// Initialize smoke particles
void init_smoke(void) {
    if (smoke_initialized) {
        return;
    }

    // Initialize all particles as inactive (brightness = 0)
    for (uint8_t i = 0; i < NUM_SMOKE_PARTICLES; i++) {
        smoke_init(&smoke_particles[i], 0, 0, 2, 0, 1);  // Inactive particle
    }

    smoke_initialized = true;
    smoke_spawn_timer = timer_read32();  // Start spawn timer
}

// Check if a pixel is part of smoke (for occlusion detection)
bool is_pixel_in_smoke(int16_t px, int16_t py) {
    if (!smoke_initialized) return false;

    for (uint8_t i = 0; i < NUM_SMOKE_PARTICLES; i++) {
        int16_t dx = px - smoke_particles[i].x;
        int16_t dy = py - smoke_particles[i].y;
        int16_t radius = smoke_particles[i].size;

        // Simple circular bounds check
        if (dx * dx + dy * dy <= radius * radius) {
            return true;
        }
    }
    return false;
}

// Redraw smoke particles in a specific region (for layering with other animations)
void redraw_smoke_in_region(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if (!smoke_initialized) return;

    for (uint8_t i = 0; i < NUM_SMOKE_PARTICLES; i++) {
        // Check if smoke particle overlaps with region
        int16_t smoke_left = smoke_particles[i].x - smoke_particles[i].size;
        int16_t smoke_right = smoke_particles[i].x + smoke_particles[i].size;
        int16_t smoke_top = smoke_particles[i].y - smoke_particles[i].size;
        int16_t smoke_bottom = smoke_particles[i].y + smoke_particles[i].size;

        if (smoke_right >= x1 && smoke_left <= x2 &&
            smoke_bottom >= y1 && smoke_top <= y2 &&
            smoke_particles[i].brightness > 0) {
            // Draw this smoke puff
            smoke_draw(&smoke_particles[i]);
        }
    }
}

// Animate smoke particles
void animate_smoke(void) {
    if (!smoke_initialized || !smoke_background_saved) {
        return;
    }

    // Chimney position
    int16_t cabin_base_x = 105;
    int16_t cabin_base_y = 150;
    uint8_t cabin_height = 18;
    uint8_t roof_height = 10;
    uint8_t chimney_width = 4;
    uint8_t chimney_height = 8;

    int16_t chimney_x = cabin_base_x + 5 + chimney_width / 2;
    int16_t chimney_top_y = cabin_base_y - cabin_height - roof_height - chimney_height + 2;

    uint32_t current_time = timer_read32();

    // TIME-BASED SPAWNING: Spawn new particle at randomized intervals (0.7-1.0 seconds)
    // Calculate random interval using current_time as seed for variation
    uint32_t random_interval = SMOKE_SPAWN_INTERVAL_MIN +
                               ((current_time * 13 + 7) % (SMOKE_SPAWN_INTERVAL_MAX - SMOKE_SPAWN_INTERVAL_MIN + 1));

    if (current_time - smoke_spawn_timer >= random_interval) {
        smoke_spawn_timer = current_time;

        // Find an inactive particle slot
        for (uint8_t i = 0; i < NUM_SMOKE_PARTICLES; i++) {
            if (smoke_particles[i].brightness == 0) {
                // Spawn new particle at chimney with slight position variation
                smoke_particles[i].x = chimney_x + ((current_time % 3) - 1);  // -1, 0, or +1
                smoke_particles[i].y = chimney_top_y;
                smoke_particles[i].size = 4;  // Start at size 4
                smoke_particles[i].brightness = 180;
                smoke_particles[i].age = 0;

                // Randomize drift speed: some drift slowly, some faster
                // Values: 0 = very slow, 1 = normal, 2 = faster
                // Using current_time and particle index for pseudo-random variation
                uint8_t drift_rand = (current_time + i * 17) % 10;
                if (drift_rand < 3) {
                    smoke_particles[i].drift = 0;  // 30% chance: very slow drift
                } else if (drift_rand < 7) {
                    smoke_particles[i].drift = 1;  // 40% chance: normal drift
                } else {
                    smoke_particles[i].drift = 2;  // 30% chance: faster drift
                }

                break;  // Only spawn one particle per interval
            }
        }
    }

    // Animate each ACTIVE smoke particle
    for (uint8_t i = 0; i < NUM_SMOKE_PARTICLES; i++) {
        // Skip inactive particles
        if (smoke_particles[i].brightness == 0) {
            continue;
        }

        // Store old position
        int16_t old_x = smoke_particles[i].x;
        int16_t old_y = smoke_particles[i].y;
        uint8_t old_size = smoke_particles[i].size;

        // Restore old smoke position from background
        fb_restore_from_background(old_x - old_size, old_y - old_size,
                                   old_x + old_size, old_y + old_size);

        // If ghosts are active and might overlap, redraw them
        if (is_halloween_event() && ghost_initialized) {
            redraw_ghosts_in_region(old_x - old_size, old_y - old_size,
                                   old_x + old_size, old_y + old_size);
        }

        // Flush the old smoke region to erase it
        fb_flush_region(display, old_x - old_size, old_y - old_size,
                       old_x + old_size, old_y + old_size);

        // Age the particle
        smoke_particles[i].age += 8;

        // Move particle upward
        smoke_particles[i].y -= 1;

        // Add horizontal drift based on particle's drift speed
        // drift=0: slow (every 48 ticks), drift=1: normal (every 24 ticks), drift=2: fast (every 12 ticks)
        uint8_t drift_frequency = (smoke_particles[i].drift == 0) ? 48 :
                                  (smoke_particles[i].drift == 1) ? 24 : 12;

        if (smoke_particles[i].age % drift_frequency == 0) {
            smoke_particles[i].x += 1;  // Always drift right by 1 pixel
        }

        // Shrink size as it rises (gets smaller as it gains height)
        // Shrink more slowly - every 64 age-ticks instead of 32
        if (smoke_particles[i].size > 2 && smoke_particles[i].age % 64 == 0) {
            smoke_particles[i].size--;
        }

        // Fade out as it rises
        if (smoke_particles[i].brightness > 10) {
            smoke_particles[i].brightness -= 2;
        } else {
            smoke_particles[i].brightness = 0;
        }

        // DELETION: Deactivate particle when it goes off screen or fades out
        // This is INDEPENDENT of spawning - spawning is time-based
        // Disappear around halfway up the screen (y=75, since ground is at y=150)
        if (smoke_particles[i].brightness == 0 ||
            smoke_particles[i].y < 75 ||
            smoke_particles[i].x > 135) {
            // Just deactivate - don't respawn here
            smoke_particles[i].brightness = 0;
            continue;  // Skip drawing
        }

        // Draw smoke at new position
        if (smoke_particles[i].y >= 0 && smoke_particles[i].y < 155) {
            smoke_draw(&smoke_particles[i]);

            // Flush the new smoke region
            fb_flush_region(display,
                           smoke_particles[i].x - smoke_particles[i].size,
                           smoke_particles[i].y - smoke_particles[i].size,
                           smoke_particles[i].x + smoke_particles[i].size,
                           smoke_particles[i].y + smoke_particles[i].size);
        }
    }
}

// Main seasonal animation drawing function
void draw_seasonal_animation(void) {
    // Animation area: entire upper portion from top to date area
    // Logo area: y=10 to y=130 (120x120 logo)
    // Animation extends from y=0 to y=152 (above date which starts at y=155)

    // Determine season based on month
    uint8_t season = get_season(current_month);

    // Get astronomical times for realistic day/night determination
    const astronomical_times_t *astro = get_astronomical_times();

    // Determine time of day using realistic sunrise/sunset times
    bool is_night = !astronomical_is_daytime(current_hour, current_minute, astro);

    // Get sun/moon position based on time (now uses realistic astronomical calculations)
    uint16_t celestial_x, celestial_y;
    get_celestial_position(current_hour, &celestial_x, &celestial_y);

    // === SKY BACKGROUND ===

    // Draw sky background based on time of day and weather conditions
    // Sky area: y=0 to y=152 (above date which starts at y=155)
    if (is_night) {
        // Night sky: dark blue (like New Year's Eve)
        // HSV: Hue=170 (blue), Saturation=200, Value=30 (dark)
        fb_rect_hsv(0, 0, 134, 152, 170, 200, 30, true);
    } else {
        // Daytime sky depends on weather conditions
        // Check if there are clouds (winter and fall have clouds)
        if (season == 0 || season == 3) {
            // Cloudy/rainy day: darker grayish sky
            // HSV: Hue=170 (blue-gray), Saturation=40 (low saturation for gray), Value=50 (darker)
            fb_rect_hsv(0, 0, 134, 152, 170, 40, 50, true);
        } else {
            // Clear day: light blue sky
            // HSV: Hue=170 (blue), Saturation=200, Value=180 (bright)
            fb_rect_hsv(0, 0, 134, 152, 170, 200, 180, true);
        }
    }

    // === LOGO ===

    // Draw the Amboss logo in teal (always the same color)
    // Logo is 120x120, centered horizontally at x=7, positioned at y=10
    draw_amboss_logo(7, 10, 128, 255, 255);

    // === SKY AND CELESTIAL OBJECTS ===

    // Draw sun or moon with appropriate coloring based on time
    if (is_night) {
        // Draw moon with phase
        moon_t moon;
        moon_init(&moon, celestial_x, celestial_y, current_day, current_hour);
        moon_draw(&moon);

        // Add stars scattered across the night sky
        stars_draw();
    } else {
        // Draw sun with color based on time of day
        sun_t sun;
        sun_init(&sun, celestial_x, celestial_y, current_hour);
        sun_draw(&sun);
    }

    // === GROUND AND TREES ===

    // Draw ground line
    uint16_t ground_y = 150;
    fb_rect_hsv(0, ground_y, 134, ground_y + 1, 85, 180, 100, true);

    // Draw trees at different positions
    uint8_t layer = get_highest_layer(layer_state);
    uint8_t tree_hue, tree_sat, tree_val;
    get_layer_color(layer, &tree_hue, &tree_sat, &tree_val);

    draw_tree(30, ground_y, season, tree_hue, tree_sat, tree_val);
    draw_tree(67, ground_y, season, tree_hue, tree_sat, tree_val);
    draw_cabin(105, ground_y, season);

    // === SEASONAL WEATHER EFFECTS ===

    if (season == 0) { // Winter
        draw_winter_scene_elements();
    } else if (season == 1) { // Spring
        draw_spring_scene_elements();
    } else if (season == 2) { // Summer
        draw_summer_scene_elements();
    } else { // Fall
        draw_fall_scene_elements();
    }

    // === HALLOWEEN/CHRISTMAS/EASTER OVERLAYS ===
    if (is_halloween_event()) {
        draw_halloween_elements();
    }

    if (is_christmas_season()) {
        draw_christmas_scene();
    }

    if (is_easter_event()) {
        draw_easter_elements();
    }

    // === INITIALIZE GHOSTS (if Halloween) ===
    // NOTE: This must happen BEFORE background save so the check can detect
    // that ghosts need background saved. Ghosts are drawn AFTER background save.
    if (is_halloween_event()) {
        if (!ghost_initialized) {
            init_ghosts();
        }
    } else {
        if (ghost_initialized) {
            ghost_initialized = false;
            ghost_background_saved = false;
        }
    }

    // === INITIALIZE EASTER BUNNY (if Easter) ===
    if (is_easter_event()) {
        if (!easter_initialized) {
            init_easter_animations();
        }
    } else {
        if (easter_initialized) {
            easter_initialized = false;
            easter_background_saved = false;
        }
    }

    // === SAVE BACKGROUND FOR ANIMATIONS ===
    // Check if we need to save background for any active animations
    bool need_background = false;

    // Spring season needs background for birds and butterflies
    if (season == 1 && spring_initialized && !spring_background_saved) {
        need_background = true;
    }

    // Summer season needs background for bees and fireflies
    if (season == 2 && summer_initialized && !summer_background_saved) {
        need_background = true;
    }

    // Fall season needs background for rain
    if (season == 3 && rain_initialized && !rain_background_saved) {
        need_background = true;
    }

    // Winter season needs background for snowflakes
    if (season == 0 && snowflake_initialized && !snowflake_background_saved) {
        need_background = true;
    }

    // Halloween needs background for ghosts
    if (is_halloween_event() && ghost_initialized && !ghost_background_saved) {
        need_background = true;
    }

    // Easter needs background for bunny
    if (is_easter_event() && easter_initialized && !easter_background_saved) {
        need_background = true;
    }

    // Smoke needs background (all seasons except summer)
    if (season != 2 && smoke_initialized && !smoke_background_saved) {
        need_background = true;
    }

    // Winter and fall need background for clouds
    if ((season == 0 || season == 3) && cloud_initialized && !cloud_background_saved) {
        need_background = true;
    }

    // Save background once if any animation needs it
    if (need_background) {
        fb_save_to_background();

        // Set the appropriate flags based on what's active
        if (season == 1 && spring_initialized) {
            spring_background_saved = true;
        }
        if (season == 2 && summer_initialized) {
            summer_background_saved = true;
        }
        if (season == 3 && rain_initialized) {
            rain_background_saved = true;
        }
        if (season == 0 && snowflake_initialized) {
            snowflake_background_saved = true;
        }
        if (is_halloween_event() && ghost_initialized) {
            ghost_background_saved = true;
        }
        if (is_easter_event() && easter_initialized) {
            easter_background_saved = true;
        }
        if (season != 2 && smoke_initialized) {
            smoke_background_saved = true;
        }
        if ((season == 0 || season == 3) && cloud_initialized) {
            cloud_background_saved = true;
        }
    }

    // === DRAW GHOSTS (if Halloween) ===
    // NOTE: Ghosts are NOT drawn here - they are drawn only by the region-based
    // animation system in keymap.c to avoid baking them into the background.
    // Ghost initialization happens earlier, before background save.

    // === DRAW SMOKE (all seasons except summer) ===
    if (season != 2 && smoke_initialized) {
        // Draw smoke particles
        for (uint8_t i = 0; i < NUM_SMOKE_PARTICLES; i++) {
            if (smoke_particles[i].brightness > 0) {
                smoke_draw(&smoke_particles[i]);
            }
        }
    } else if (season == 2) {
        // Summer - no smoke from chimney
        if (smoke_initialized) {
            smoke_initialized = false;
            smoke_background_saved = false;
        }
    }

    // === DRAW BIRDS AND BUTTERFLIES (spring only) ===
    // NOTE: Birds and butterflies are drawn AFTER background save to avoid
    // baking them into the background. They are drawn here once, then animated
    // by the region-based animation system in animate_spring().
    if (season == 1 && spring_initialized) {
        for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
            bird_draw(&birds[i]);
        }
        for (uint8_t i = 0; i < NUM_SPRING_BUTTERFLIES; i++) {
            butterfly_draw(&butterflies[i]);
        }
    } else if (season != 1) {
        // Not spring - clean up spring animation state
        if (spring_initialized) {
            spring_initialized = false;
            spring_background_saved = false;
        }
    }

    // === DRAW BEES AND FIREFLIES (summer only) ===
    // NOTE: Bees and fireflies are drawn AFTER background save to avoid
    // baking them into the background. They are drawn here once, then animated
    // by the region-based animation system in animate_summer().
    if (season == 2 && summer_initialized) {
        for (uint8_t i = 0; i < NUM_SUMMER_BEES; i++) {
            bee_draw(&bees[i]);
        }
        // Fireflies are drawn by animate_summer() based on time-of-day
        // Check if it's evening/night for fireflies (18:00 - 6:00)
        if (current_hour >= 18 || current_hour < 6) {
            for (uint8_t i = 0; i < NUM_SUMMER_FIREFLIES; i++) {
                firefly_draw(&fireflies[i]);
            }
        }
    } else if (season != 2) {
        // Not summer - clean up summer animation state
        if (summer_initialized) {
            summer_initialized = false;
            summer_background_saved = false;
        }
    }

    // === DRAW CLOUDS (winter and fall) ===
    if ((season == 0 || season == 3) && cloud_initialized) {
        // Draw clouds with appropriate type based on season
        cloud_type_t type = (season == 3) ? CLOUD_TYPE_DARK : CLOUD_TYPE_LIGHT;
        for (uint8_t i = 0; i < NUM_CLOUDS; i++) {
            cloud_draw(&clouds[i], type);
        }
    } else if (season != 0 && season != 3) {
        // Not winter or fall - clean up cloud state
        if (cloud_initialized) {
            cloud_initialized = false;
            cloud_background_saved = false;
        }
    }

    // === DRAW RAINDROPS (fall only) ===
    if (season == 3 && rain_initialized) {
        // Draw raindrops
        for (uint8_t i = 0; i < NUM_RAINDROPS; i++) {
            if (raindrops[i].y >= 0 && raindrops[i].y < 150) {
                raindrop_draw(&raindrops[i]);
            }
        }
    } else if (season != 3) {
        // Not fall - clean up rain state
        if (rain_initialized) {
            rain_initialized = false;
            rain_background_saved = false;
        }
    }

    // === DRAW SNOWFLAKES (winter only) ===
    if (season == 0 && snowflake_initialized) {
        // Draw snowflakes
        for (uint8_t i = 0; i < NUM_SNOWFLAKES; i++) {
            if (snowflakes[i].y >= 0 && snowflakes[i].y < 150) {
                snowflake_draw(&snowflakes[i]);
            }
        }
    } else if (season != 0) {
        // Not winter - clean up snowflake state
        if (snowflake_initialized) {
            snowflake_initialized = false;
            snowflake_background_saved = false;
        }
    }
}
