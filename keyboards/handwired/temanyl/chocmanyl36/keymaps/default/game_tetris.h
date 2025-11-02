// Copyright 2024
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "quantum/painter/qp.h"

// Display dimensions (fullscreen: 135x240 portrait)
#define TETRIS_DISPLAY_WIDTH 135
#define TETRIS_DISPLAY_HEIGHT 240

// Game board constants
#define TETRIS_BOARD_WIDTH 10
#define TETRIS_BOARD_HEIGHT 20
#define TETRIS_CELL_SIZE 12  // 12x12 pixels per cell
#define TETRIS_BOARD_X_OFFSET 7   // Center board: (135 - 10*12) / 2 = 7.5
#define TETRIS_BOARD_Y_OFFSET 0

// Tetromino constants
#define TETRIS_NUM_PIECES 7
#define TETRIS_PIECE_SIZE 4  // Each piece fits in a 4x4 grid

// Game timing
#define TETRIS_INITIAL_DROP_DELAY 1200  // ms between automatic drops at level 1
#define TETRIS_SOFT_DROP_DELAY 100      // ms between drops when pressing down
#define TETRIS_LOCK_DELAY 800           // ms before piece locks after landing
#define TETRIS_LINE_CLEAR_DELAY 400     // ms to show line clear animation
#define TETRIS_MOVE_INITIAL_DELAY 200   // ms before auto-repeat starts for horizontal movement
#define TETRIS_MOVE_REPEAT_DELAY 60     // ms between auto-repeat moves
#define TETRIS_ROTATE_DELAY 200         // ms debounce for rotation

// Scoring
#define TETRIS_SCORE_SINGLE 100
#define TETRIS_SCORE_DOUBLE 300
#define TETRIS_SCORE_TRIPLE 500
#define TETRIS_SCORE_TETRIS 800
#define TETRIS_SCORE_SOFT_DROP 1  // Per cell dropped

// Tetromino types
typedef enum {
    TETRIS_PIECE_I = 0,  // Cyan
    TETRIS_PIECE_O = 1,  // Yellow
    TETRIS_PIECE_T = 2,  // Purple
    TETRIS_PIECE_S = 3,  // Green
    TETRIS_PIECE_Z = 4,  // Red
    TETRIS_PIECE_J = 5,  // Blue
    TETRIS_PIECE_L = 6,  // Orange
    TETRIS_PIECE_NONE = 7
} tetris_piece_type_t;

// Tetromino shape definition (4x4 grid, 4 rotation states)
typedef struct {
    uint8_t blocks[4][4][4];  // [rotation][row][col] - 1 if block present, 0 if empty
    uint8_t hue;              // HSV hue for color (0-255)
} tetris_piece_def_t;

// Active falling piece
typedef struct {
    tetris_piece_type_t type;
    int8_t x;          // Board position (0-9)
    int8_t y;          // Board position (0-19, can be negative during spawn)
    uint8_t rotation;  // 0-3
} tetris_piece_t;

// Game modes
typedef enum {
    TETRIS_PLAYING,
    TETRIS_LINE_CLEAR_ANIM,
    TETRIS_NAME_ENTRY,
    TETRIS_SCORE_DISPLAY
} tetris_mode_t;

// High score entry
typedef struct {
    char name[4];  // 3 chars + null terminator
    uint16_t score;
} tetris_highscore_entry_t;

// Name entry state (arcade-style)
typedef struct {
    char name[3];         // Current name being entered
    uint8_t char_index;   // Which character (0-2) we're editing
    uint8_t letter_index; // Which letter (0-25 for A-Z)
} tetris_name_entry_state_t;

// Tetris game state
typedef struct {
    // Board state (0 = empty, 1-7 = piece type for color)
    uint8_t board[TETRIS_BOARD_HEIGHT][TETRIS_BOARD_WIDTH];

    // Active piece
    tetris_piece_t current_piece;
    tetris_piece_type_t next_piece;  // Preview

    // Game state
    bool active;
    bool game_over;
    tetris_mode_t mode;

    // Timing
    uint32_t last_drop;       // Timer for automatic dropping
    uint32_t last_move;       // Timer for horizontal movement
    uint32_t last_rotate;     // Timer for rotation
    uint32_t lock_timer;      // Timer for lock delay
    uint32_t anim_timer;      // Timer for animations
    bool piece_locked;        // True when piece has landed

    // Line clearing animation
    uint8_t lines_to_clear[4];  // Row indices of lines to clear (max 4)
    uint8_t num_lines_to_clear;

    // Scoring
    uint16_t score;
    uint16_t lines_cleared;
    uint8_t level;

    // High score system
    tetris_name_entry_state_t name_entry;
    tetris_highscore_entry_t highscores[10];
    uint8_t highscore_count;
    uint8_t player_rank;  // 0-9 if in top 10, 255 otherwise
    bool waiting_for_hid_response;
    uint32_t hid_wait_start;
    bool offline_mode;
} tetris_state_t;

// Input state
typedef struct {
    bool left;
    bool right;
    bool down;
    bool rotate;
    // Previous state for edge detection
    bool prev_left;
    bool prev_right;
    bool prev_down;
    bool prev_rotate;
    // Timing for auto-repeat
    uint32_t left_press_time;
    uint32_t right_press_time;
} tetris_input_t;

// Function declarations
void tetris_init(void);
void tetris_update(void);
void tetris_render(painter_device_t device);
void tetris_set_input(bool left, bool right, bool down, bool rotate);
void tetris_cleanup(void);
bool tetris_is_active(void);

/**
 * Handle keypresses for Tetris
 * @param keycode The keycode being processed
 * @param record The keyrecord
 * @param current_display_layer Pointer to display layer cache (will be invalidated if game exits)
 * @return false if game handled the key, true to continue normal processing
 */
bool tetris_process_record(uint16_t keycode, keyrecord_t *record, uint8_t *current_display_layer);

/**
 * Handle game update and rendering in housekeeping loop
 * @param display The display device
 * @return true if game handled the update, false otherwise
 */
bool tetris_housekeeping(painter_device_t display);

/**
 * Handle Raw HID data from computer (high score responses)
 * @param data The received data buffer (32 bytes)
 * @param length The length of the data
 */
void tetris_hid_receive(uint8_t *data, uint8_t length);

// Global state
extern tetris_state_t g_tetris;
extern tetris_input_t g_tetris_input;

// Tetromino definitions (shape data)
extern const tetris_piece_def_t tetris_pieces[TETRIS_NUM_PIECES];
