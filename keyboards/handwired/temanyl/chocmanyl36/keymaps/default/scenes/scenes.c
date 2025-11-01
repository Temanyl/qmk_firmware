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

// Include season-specific modules
#include "../seasons/winter/seasons_winter.h"
#include "../seasons/spring/seasons_spring.h"
#include "../seasons/summer/seasons_summer.h"
#include "../seasons/fall/seasons_fall.h"
#include "../seasons/halloween/seasons_halloween.h"
#include "../seasons/christmas/seasons_christmas.h"

// Include drawable objects
#include "../objects/weather/smoke.h"
#include "../objects/weather/raindrop.h"
#include "../objects/celestial/sun.h"
#include "../objects/celestial/moon.h"
#include "../objects/celestial/stars.h"
#include "../objects/structures/tree.h"
#include "../objects/structures/cabin.h"

// Smoke animation state (shared across seasons)
bool smoke_initialized = false;
bool smoke_background_saved = false;
smoke_particle_t smoke_particles[NUM_SMOKE_PARTICLES];
uint32_t smoke_animation_timer = 0;
uint32_t smoke_spawn_timer = 0;

// Forward declarations from display.c
extern uint8_t current_hour;
extern uint8_t current_day;
extern uint8_t current_month;
extern painter_device_t display;

// Reset all scene animation states
void reset_scene_animations(void) {
    reset_winter_animations();
    reset_spring_animations();
    reset_summer_animations();
    reset_fall_animations();
    reset_halloween_animations();
    reset_christmas_animations();
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

// Get celestial object (sun/moon) position based on time
void get_celestial_position(uint8_t hour, uint16_t *x, uint16_t *y) {
    // Sun/moon moves across sky throughout the day
    // Hour 0-23: position from left to right
    // Peak at noon (y lowest), near horizon at dawn/dusk (y highest)

    // X position: moves from left to right across the sky
    // Map hour 0-23 to x position
    if (hour >= 6 && hour <= 19) {
        // Daytime: sun moves from left to right
        // hour 6 -> x=20, hour 12 -> x=67, hour 19 -> x=114
        *x = 20 + ((hour - 6) * 94) / 13;

        // Y position: arc across sky (lowest at noon)
        // hour 6 -> y=40, hour 12 -> y=20, hour 19 -> y=40
        int16_t time_from_noon = hour - 12;
        *y = 20 + (time_from_noon * time_from_noon) / 2;
    } else {
        // Nighttime: moon moves from left to right (same direction as sun)
        // Night: hour 20-23 (evening) and 0-5 (early morning)
        // Map to continuous night progression: 20->0, 21->1, 22->2, 23->3, 0->4, 1->5, ..., 5->9
        uint8_t night_hour;
        if (hour >= 20) {
            night_hour = hour - 20;  // 20->0, 21->1, 22->2, 23->3
        } else {
            night_hour = hour + 4;    // 0->4, 1->5, 2->6, 3->7, 4->8, 5->9
        }
        // night_hour 0-9: position from left to right (like sun)
        // Starts at x=20 (hour 20) and ends at x=114 (hour 6)
        *x = 20 + (night_hour * 94) / 9;

        // Y position: arc across sky (lowest at midnight, which is night_hour=4)
        int16_t time_from_midnight = night_hour - 4;
        *y = 25 + (time_from_midnight * time_from_midnight) / 2;
    }

    // Clamp values
    if (*x < 15) *x = 15;
    if (*x > 120) *x = 120;
    if (*y < 15) *y = 15;
    if (*y > 50) *y = 50;
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

    // Determine time of day based on hour (0-23)
    bool is_night = (current_hour >= 20 || current_hour < 6);

    // Get sun/moon position based on time
    uint16_t celestial_x, celestial_y;
    get_celestial_position(current_hour, &celestial_x, &celestial_y);

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

    // === HALLOWEEN/CHRISTMAS OVERLAYS ===
    if (is_halloween_event()) {
        draw_halloween_elements();
    }

    if (is_christmas_season()) {
        draw_christmas_scene();
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

    // === SAVE BACKGROUND FOR ANIMATIONS ===
    // Check if we need to save background for any active animations
    bool need_background = false;

    // Fall season needs background for rain
    if (season == 3 && rain_initialized && !rain_background_saved) {
        need_background = true;
    }

    // Halloween needs background for ghosts
    if (is_halloween_event() && ghost_initialized && !ghost_background_saved) {
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
        if (season == 3 && rain_initialized) {
            rain_background_saved = true;
        }
        if (is_halloween_event() && ghost_initialized) {
            ghost_background_saved = true;
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

    // === DRAW CLOUDS (winter and fall) ===
    if ((season == 0 || season == 3) && cloud_initialized) {
        // Draw clouds
        for (uint8_t i = 0; i < NUM_CLOUDS; i++) {
            draw_cloud(clouds[i].x, clouds[i].y);
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
}
