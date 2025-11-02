// Copyright 2024
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "quantum/painter/qp.h"

// Available games
typedef enum {
    GAME_NONE = 0,
    GAME_DOODLE = 1,
    GAME_TETRIS = 2,
    GAME_SELECTION = 3  // Special state for game selection screen
} game_type_t;

// Game manager state
typedef struct {
    game_type_t current_game;    // Which game is running
    game_type_t selected_game;   // Which game is highlighted in selection
    bool active;                 // Game manager is active
} game_manager_state_t;

// Function declarations

/**
 * Initialize game manager (shows game selection screen)
 */
void game_manager_init(void);

/**
 * Update game manager state (delegates to active game or handles selection)
 */
void game_manager_update(void);

/**
 * Render game manager (delegates to active game or shows selection screen)
 */
void game_manager_render(painter_device_t device);

/**
 * Cleanup game manager and any active game
 */
void game_manager_cleanup(void);

/**
 * Check if game manager is active
 */
bool game_manager_is_active(void);

/**
 * Handle keypresses for game manager
 * @param keycode The keycode being processed
 * @param record The keyrecord
 * @param current_display_layer Pointer to display layer cache
 * @return false if game handled the key, true to continue normal processing
 */
bool game_manager_process_record(uint16_t keycode, keyrecord_t *record, uint8_t *current_display_layer);

/**
 * Handle game manager update and rendering in housekeeping loop
 * @param display The display device
 * @return true if game manager handled the update, false otherwise
 */
bool game_manager_housekeeping(painter_device_t display);

/**
 * Handle Raw HID data from computer (high score responses)
 * Routes to appropriate game
 * @param data The received data buffer (32 bytes)
 * @param length The length of the data
 */
void game_manager_hid_receive(uint8_t *data, uint8_t length);

// Global state
extern game_manager_state_t g_game_manager;
