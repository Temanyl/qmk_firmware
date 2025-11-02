// Copyright 2024
// SPDX-License-Identifier: GPL-2.0-or-later

#include "game_tetris.h"
#include "display/framebuffer.h"
#include "raw_hid.h"
#include <stdlib.h>
#include <string.h>
#include "timer.h"

// Global game state
tetris_state_t g_tetris = {0};
tetris_input_t g_tetris_input = {0};

// Tetromino shape definitions
// Format: [rotation][row][col] where 1 = block present, 0 = empty
// All pieces use SRS (Super Rotation System) standard orientations
const tetris_piece_def_t tetris_pieces[TETRIS_NUM_PIECES] = {
    // I piece (Cyan) - hue 128 (cyan in HSV)
    [TETRIS_PIECE_I] = {
        .blocks = {
            // Rotation 0 (horizontal)
            {{0, 0, 0, 0},
             {1, 1, 1, 1},
             {0, 0, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 1 (vertical)
            {{0, 0, 1, 0},
             {0, 0, 1, 0},
             {0, 0, 1, 0},
             {0, 0, 1, 0}},
            // Rotation 2 (horizontal)
            {{0, 0, 0, 0},
             {0, 0, 0, 0},
             {1, 1, 1, 1},
             {0, 0, 0, 0}},
            // Rotation 3 (vertical)
            {{0, 1, 0, 0},
             {0, 1, 0, 0},
             {0, 1, 0, 0},
             {0, 1, 0, 0}}
        },
        .hue = 128  // Cyan
    },

    // O piece (Yellow) - hue 42 (yellow in HSV)
    [TETRIS_PIECE_O] = {
        .blocks = {
            // All rotations the same (square)
            {{0, 1, 1, 0},
             {0, 1, 1, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}},
            {{0, 1, 1, 0},
             {0, 1, 1, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}},
            {{0, 1, 1, 0},
             {0, 1, 1, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}},
            {{0, 1, 1, 0},
             {0, 1, 1, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}}
        },
        .hue = 42  // Yellow
    },

    // T piece (Purple) - hue 192 (purple/magenta in HSV)
    [TETRIS_PIECE_T] = {
        .blocks = {
            // Rotation 0 (T pointing up)
            {{0, 1, 0, 0},
             {1, 1, 1, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 1 (T pointing right)
            {{0, 1, 0, 0},
             {0, 1, 1, 0},
             {0, 1, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 2 (T pointing down)
            {{0, 0, 0, 0},
             {1, 1, 1, 0},
             {0, 1, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 3 (T pointing left)
            {{0, 1, 0, 0},
             {1, 1, 0, 0},
             {0, 1, 0, 0},
             {0, 0, 0, 0}}
        },
        .hue = 192  // Purple
    },

    // S piece (Green) - hue 85 (green in HSV)
    [TETRIS_PIECE_S] = {
        .blocks = {
            // Rotation 0
            {{0, 1, 1, 0},
             {1, 1, 0, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 1
            {{0, 1, 0, 0},
             {0, 1, 1, 0},
             {0, 0, 1, 0},
             {0, 0, 0, 0}},
            // Rotation 2
            {{0, 0, 0, 0},
             {0, 1, 1, 0},
             {1, 1, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 3
            {{1, 0, 0, 0},
             {1, 1, 0, 0},
             {0, 1, 0, 0},
             {0, 0, 0, 0}}
        },
        .hue = 85  // Green
    },

    // Z piece (Red) - hue 0 (red in HSV)
    [TETRIS_PIECE_Z] = {
        .blocks = {
            // Rotation 0
            {{1, 1, 0, 0},
             {0, 1, 1, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 1
            {{0, 0, 1, 0},
             {0, 1, 1, 0},
             {0, 1, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 2
            {{0, 0, 0, 0},
             {1, 1, 0, 0},
             {0, 1, 1, 0},
             {0, 0, 0, 0}},
            // Rotation 3
            {{0, 1, 0, 0},
             {1, 1, 0, 0},
             {1, 0, 0, 0},
             {0, 0, 0, 0}}
        },
        .hue = 0  // Red
    },

    // J piece (Blue) - hue 170 (blue in HSV)
    [TETRIS_PIECE_J] = {
        .blocks = {
            // Rotation 0
            {{1, 0, 0, 0},
             {1, 1, 1, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 1
            {{0, 1, 1, 0},
             {0, 1, 0, 0},
             {0, 1, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 2
            {{0, 0, 0, 0},
             {1, 1, 1, 0},
             {0, 0, 1, 0},
             {0, 0, 0, 0}},
            // Rotation 3
            {{0, 1, 0, 0},
             {0, 1, 0, 0},
             {1, 1, 0, 0},
             {0, 0, 0, 0}}
        },
        .hue = 170  // Blue
    },

    // L piece (Orange) - hue 21 (orange in HSV)
    [TETRIS_PIECE_L] = {
        .blocks = {
            // Rotation 0
            {{0, 0, 1, 0},
             {1, 1, 1, 0},
             {0, 0, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 1
            {{0, 1, 0, 0},
             {0, 1, 0, 0},
             {0, 1, 1, 0},
             {0, 0, 0, 0}},
            // Rotation 2
            {{0, 0, 0, 0},
             {1, 1, 1, 0},
             {1, 0, 0, 0},
             {0, 0, 0, 0}},
            // Rotation 3
            {{1, 1, 0, 0},
             {0, 1, 0, 0},
             {0, 1, 0, 0},
             {0, 0, 0, 0}}
        },
        .hue = 21  // Orange
    }
};

// Forward declarations
static void spawn_piece(tetris_piece_type_t type);
static void spawn_next_piece(void);
static bool check_collision(int8_t x, int8_t y, uint8_t rotation);
static void lock_piece(void);
static void check_lines(void);
static void clear_lines(void);
static void move_piece(int8_t dx, int8_t dy);
static bool try_rotate(void);
static void draw_board(void);
static void draw_piece(tetris_piece_t *piece, bool ghost);
static void draw_score_ui(void);
static void draw_digit(int16_t x, int16_t y, uint8_t digit, uint8_t scale, uint8_t hue);
static void draw_char(int16_t x, int16_t y, char c, uint8_t scale, uint8_t hue);
static void draw_name_entry(void);
static void draw_score_display(void);
static uint16_t get_drop_delay(void);
static void submit_score_to_hid(void);
static void submit_name_to_hid(void);

// Initialize game
void tetris_init(void) {
    memset(&g_tetris, 0, sizeof(tetris_state_t));
    memset(&g_tetris_input, 0, sizeof(tetris_input_t));

    g_tetris.active = true;
    g_tetris.mode = TETRIS_PLAYING;
    g_tetris.level = 1;
    g_tetris.player_rank = 255;  // Not in top 10

    // Seed random with timer
    srand(timer_read32());

    // Spawn first piece
    g_tetris.next_piece = rand() % TETRIS_NUM_PIECES;
    spawn_next_piece();
}

// Spawn a specific piece type
static void spawn_piece(tetris_piece_type_t type) {
    g_tetris.current_piece.type = type;
    g_tetris.current_piece.x = TETRIS_BOARD_WIDTH / 2 - 2;  // Center
    g_tetris.current_piece.y = 0;
    g_tetris.current_piece.rotation = 0;
    g_tetris.piece_locked = false;
    g_tetris.lock_timer = 0;

    // Check if piece can spawn (game over if not)
    if (check_collision(g_tetris.current_piece.x, g_tetris.current_piece.y, 0)) {
        g_tetris.game_over = true;
        g_tetris.mode = TETRIS_PLAYING;  // Will transition to name entry after score submit
        submit_score_to_hid();
    }
}

// Spawn next piece and generate new next piece
static void spawn_next_piece(void) {
    spawn_piece(g_tetris.next_piece);
    g_tetris.next_piece = rand() % TETRIS_NUM_PIECES;
}

// Check if piece collides with board or boundaries
static bool check_collision(int8_t x, int8_t y, uint8_t rotation) {
    const tetris_piece_def_t *piece_def = &tetris_pieces[g_tetris.current_piece.type];

    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            if (piece_def->blocks[rotation][row][col]) {
                int8_t board_x = x + col;
                int8_t board_y = y + row;

                // Check boundaries
                if (board_x < 0 || board_x >= TETRIS_BOARD_WIDTH ||
                    board_y >= TETRIS_BOARD_HEIGHT) {
                    return true;  // Collision
                }

                // Check board (allow negative y for spawning)
                if (board_y >= 0 && g_tetris.board[board_y][board_x] != 0) {
                    return true;  // Collision with placed block
                }
            }
        }
    }

    return false;  // No collision
}

// Lock piece to board
static void lock_piece(void) {
    const tetris_piece_def_t *piece_def = &tetris_pieces[g_tetris.current_piece.type];

    // Copy piece to board
    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            if (piece_def->blocks[g_tetris.current_piece.rotation][row][col]) {
                int8_t board_x = g_tetris.current_piece.x + col;
                int8_t board_y = g_tetris.current_piece.y + row;

                if (board_y >= 0 && board_y < TETRIS_BOARD_HEIGHT &&
                    board_x >= 0 && board_x < TETRIS_BOARD_WIDTH) {
                    // Store piece type + 1 (0 is empty)
                    g_tetris.board[board_y][board_x] = g_tetris.current_piece.type + 1;
                }
            }
        }
    }

    // Check for completed lines
    check_lines();
}

// Check for completed lines
static void check_lines(void) {
    g_tetris.num_lines_to_clear = 0;

    for (int8_t row = TETRIS_BOARD_HEIGHT - 1; row >= 0; row--) {
        bool line_full = true;
        for (uint8_t col = 0; col < TETRIS_BOARD_WIDTH; col++) {
            if (g_tetris.board[row][col] == 0) {
                line_full = false;
                break;
            }
        }

        if (line_full) {
            g_tetris.lines_to_clear[g_tetris.num_lines_to_clear++] = row;
        }
    }

    if (g_tetris.num_lines_to_clear > 0) {
        // Start line clear animation
        g_tetris.mode = TETRIS_LINE_CLEAR_ANIM;
        g_tetris.anim_timer = timer_read32();
    } else {
        // No lines, spawn next piece
        spawn_next_piece();
    }
}

// Clear completed lines and update score
static void clear_lines(void) {
    // Update statistics
    g_tetris.lines_cleared += g_tetris.num_lines_to_clear;

    // Calculate score
    uint16_t line_score = 0;
    switch (g_tetris.num_lines_to_clear) {
        case 1: line_score = TETRIS_SCORE_SINGLE; break;
        case 2: line_score = TETRIS_SCORE_DOUBLE; break;
        case 3: line_score = TETRIS_SCORE_TRIPLE; break;
        case 4: line_score = TETRIS_SCORE_TETRIS; break;
    }
    g_tetris.score += line_score * g_tetris.level;

    // Update level (every 10 lines)
    g_tetris.level = (g_tetris.lines_cleared / 10) + 1;

    // Remove lines from board (from bottom to top)
    for (uint8_t i = 0; i < g_tetris.num_lines_to_clear; i++) {
        uint8_t row = g_tetris.lines_to_clear[i];

        // Move all rows above down by one
        for (int8_t r = row; r > 0; r--) {
            for (uint8_t c = 0; c < TETRIS_BOARD_WIDTH; c++) {
                g_tetris.board[r][c] = g_tetris.board[r - 1][c];
            }
        }

        // Clear top row
        for (uint8_t c = 0; c < TETRIS_BOARD_WIDTH; c++) {
            g_tetris.board[0][c] = 0;
        }
    }

    // Spawn next piece
    g_tetris.num_lines_to_clear = 0;
    g_tetris.mode = TETRIS_PLAYING;
    spawn_next_piece();
}

// Move piece by delta
static void move_piece(int8_t dx, int8_t dy) {
    int8_t new_x = g_tetris.current_piece.x + dx;
    int8_t new_y = g_tetris.current_piece.y + dy;

    if (!check_collision(new_x, new_y, g_tetris.current_piece.rotation)) {
        g_tetris.current_piece.x = new_x;
        g_tetris.current_piece.y = new_y;

        // Award soft drop points
        if (dy > 0) {
            g_tetris.score += TETRIS_SCORE_SOFT_DROP * dy;
        }

        // Reset lock timer if piece moved down
        if (dy > 0) {
            g_tetris.lock_timer = 0;
        }
    } else if (dy > 0) {
        // Piece hit bottom, start lock timer
        if (!g_tetris.piece_locked) {
            g_tetris.piece_locked = true;
            g_tetris.lock_timer = timer_read32();
        }
    }
}

// Try to rotate piece
static bool try_rotate(void) {
    uint8_t new_rotation = (g_tetris.current_piece.rotation + 1) % 4;

    // Try basic rotation
    if (!check_collision(g_tetris.current_piece.x, g_tetris.current_piece.y, new_rotation)) {
        g_tetris.current_piece.rotation = new_rotation;
        return true;
    }

    // Try wall kicks (simple: try left, right, up)
    int8_t kicks[][2] = {{-1, 0}, {1, 0}, {0, -1}, {-1, -1}, {1, -1}};
    for (uint8_t i = 0; i < 5; i++) {
        int8_t kick_x = g_tetris.current_piece.x + kicks[i][0];
        int8_t kick_y = g_tetris.current_piece.y + kicks[i][1];

        if (!check_collision(kick_x, kick_y, new_rotation)) {
            g_tetris.current_piece.x = kick_x;
            g_tetris.current_piece.y = kick_y;
            g_tetris.current_piece.rotation = new_rotation;
            return true;
        }
    }

    return false;  // Rotation failed
}

// Get drop delay based on level
static uint16_t get_drop_delay(void) {
    // Speed increases with level (slower progression)
    uint16_t delay = TETRIS_INITIAL_DROP_DELAY - (g_tetris.level - 1) * 50;
    if (delay < 250) delay = 250;  // Minimum delay (still playable at high levels)
    return delay;
}

// Update game state
void tetris_update(void) {
    if (!g_tetris.active) return;

    uint32_t now = timer_read32();

    // Handle different modes
    switch (g_tetris.mode) {
        case TETRIS_PLAYING:
            if (g_tetris.game_over) {
                // Wait for HID response or timeout
                if (g_tetris.waiting_for_hid_response) {
                    if (timer_elapsed32(g_tetris.hid_wait_start) > 2000) {
                        // Timeout - go to offline name entry
                        g_tetris.offline_mode = true;
                        g_tetris.waiting_for_hid_response = false;
                        g_tetris.mode = TETRIS_NAME_ENTRY;
                        memset(&g_tetris.name_entry, 0, sizeof(tetris_name_entry_state_t));
                    }
                }
                return;
            }

            // Handle automatic dropping
            uint16_t drop_delay = g_tetris_input.down ? TETRIS_SOFT_DROP_DELAY : get_drop_delay();
            if (timer_elapsed32(g_tetris.last_drop) > drop_delay) {
                move_piece(0, 1);  // Move down
                g_tetris.last_drop = now;
            }

            // Handle lock delay
            if (g_tetris.piece_locked && timer_elapsed32(g_tetris.lock_timer) > TETRIS_LOCK_DELAY) {
                lock_piece();
                g_tetris.piece_locked = false;
            }

            // Handle horizontal movement with edge detection and auto-repeat
            // Left movement
            if (g_tetris_input.left && !g_tetris_input.prev_left) {
                // Just pressed - move immediately
                move_piece(-1, 0);
                g_tetris_input.left_press_time = now;
            } else if (g_tetris_input.left && g_tetris_input.prev_left) {
                // Held - auto-repeat after initial delay
                uint32_t held_time = timer_elapsed32(g_tetris_input.left_press_time);
                if (held_time > TETRIS_MOVE_INITIAL_DELAY) {
                    if (timer_elapsed32(g_tetris.last_move) > TETRIS_MOVE_REPEAT_DELAY) {
                        move_piece(-1, 0);
                        g_tetris.last_move = now;
                    }
                }
            }

            // Right movement
            if (g_tetris_input.right && !g_tetris_input.prev_right) {
                // Just pressed - move immediately
                move_piece(1, 0);
                g_tetris_input.right_press_time = now;
            } else if (g_tetris_input.right && g_tetris_input.prev_right) {
                // Held - auto-repeat after initial delay
                uint32_t held_time = timer_elapsed32(g_tetris_input.right_press_time);
                if (held_time > TETRIS_MOVE_INITIAL_DELAY) {
                    if (timer_elapsed32(g_tetris.last_move) > TETRIS_MOVE_REPEAT_DELAY) {
                        move_piece(1, 0);
                        g_tetris.last_move = now;
                    }
                }
            }

            // Handle rotation - immediate on press
            if (g_tetris_input.rotate && !g_tetris_input.prev_rotate) {
                try_rotate();
                g_tetris.last_rotate = now;
            }

            // Update previous input state
            g_tetris_input.prev_left = g_tetris_input.left;
            g_tetris_input.prev_right = g_tetris_input.right;
            g_tetris_input.prev_down = g_tetris_input.down;
            g_tetris_input.prev_rotate = g_tetris_input.rotate;

            break;

        case TETRIS_LINE_CLEAR_ANIM:
            // Wait for animation to finish
            if (timer_elapsed32(g_tetris.anim_timer) > TETRIS_LINE_CLEAR_DELAY) {
                clear_lines();
            }
            break;

        case TETRIS_NAME_ENTRY:
        case TETRIS_SCORE_DISPLAY:
            // Handled by input
            break;
    }
}

// Draw a single digit
static void draw_digit(int16_t x, int16_t y, uint8_t digit, uint8_t scale, uint8_t hue) {
    // Simple 3x5 pixel font for digits 0-9
    const uint8_t digits[10][5] = {
        {0b111, 0b101, 0b101, 0b101, 0b111},  // 0
        {0b010, 0b110, 0b010, 0b010, 0b111},  // 1
        {0b111, 0b001, 0b111, 0b100, 0b111},  // 2
        {0b111, 0b001, 0b111, 0b001, 0b111},  // 3
        {0b101, 0b101, 0b111, 0b001, 0b001},  // 4
        {0b111, 0b100, 0b111, 0b001, 0b111},  // 5
        {0b111, 0b100, 0b111, 0b101, 0b111},  // 6
        {0b111, 0b001, 0b001, 0b001, 0b001},  // 7
        {0b111, 0b101, 0b111, 0b101, 0b111},  // 8
        {0b111, 0b101, 0b111, 0b001, 0b111}   // 9
    };

    if (digit > 9) return;

    for (uint8_t row = 0; row < 5; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            if (digits[digit][row] & (1 << (2 - col))) {
                for (uint8_t sy = 0; sy < scale; sy++) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        fb_set_pixel_hsv(x + col * scale + sx, y + row * scale + sy, hue, 255, 255);
                    }
                }
            }
        }
    }
}

// Draw a character (A-Z)
static void draw_char(int16_t x, int16_t y, char c, uint8_t scale, uint8_t hue) {
    // Simple 3x5 font for A-Z
    const uint8_t font[26][5] = {
        {0b010, 0b101, 0b111, 0b101, 0b101},  // A
        {0b110, 0b101, 0b110, 0b101, 0b110},  // B
        {0b011, 0b100, 0b100, 0b100, 0b011},  // C
        {0b110, 0b101, 0b101, 0b101, 0b110},  // D
        {0b111, 0b100, 0b110, 0b100, 0b111},  // E
        {0b111, 0b100, 0b110, 0b100, 0b100},  // F
        {0b011, 0b100, 0b101, 0b101, 0b011},  // G
        {0b101, 0b101, 0b111, 0b101, 0b101},  // H
        {0b111, 0b010, 0b010, 0b010, 0b111},  // I
        {0b111, 0b001, 0b001, 0b101, 0b010},  // J
        {0b101, 0b110, 0b100, 0b110, 0b101},  // K
        {0b100, 0b100, 0b100, 0b100, 0b111},  // L
        {0b101, 0b111, 0b111, 0b101, 0b101},  // M
        {0b101, 0b111, 0b111, 0b111, 0b101},  // N
        {0b010, 0b101, 0b101, 0b101, 0b010},  // O
        {0b110, 0b101, 0b110, 0b100, 0b100},  // P
        {0b010, 0b101, 0b101, 0b111, 0b011},  // Q
        {0b110, 0b101, 0b110, 0b101, 0b101},  // R
        {0b011, 0b100, 0b010, 0b001, 0b110},  // S
        {0b111, 0b010, 0b010, 0b010, 0b010},  // T
        {0b101, 0b101, 0b101, 0b101, 0b111},  // U
        {0b101, 0b101, 0b101, 0b101, 0b010},  // V
        {0b101, 0b101, 0b111, 0b111, 0b101},  // W
        {0b101, 0b101, 0b010, 0b101, 0b101},  // X
        {0b101, 0b101, 0b010, 0b010, 0b010},  // Y
        {0b111, 0b001, 0b010, 0b100, 0b111}   // Z
    };

    if (c < 'A' || c > 'Z') return;

    uint8_t index = c - 'A';
    for (uint8_t row = 0; row < 5; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            if (font[index][row] & (1 << (2 - col))) {
                for (uint8_t sy = 0; sy < scale; sy++) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        fb_set_pixel_hsv(x + col * scale + sx, y + row * scale + sy, hue, 255, 255);
                    }
                }
            }
        }
    }
}

// Draw the game board
static void draw_board(void) {
    // Draw each cell
    for (uint8_t row = 0; row < TETRIS_BOARD_HEIGHT; row++) {
        for (uint8_t col = 0; col < TETRIS_BOARD_WIDTH; col++) {
            uint8_t cell = g_tetris.board[row][col];

            int16_t px = TETRIS_BOARD_X_OFFSET + col * TETRIS_CELL_SIZE;
            int16_t py = TETRIS_BOARD_Y_OFFSET + row * TETRIS_CELL_SIZE;

            if (cell == 0) {
                // Empty cell - dark background
                for (uint8_t y = 0; y < TETRIS_CELL_SIZE; y++) {
                    for (uint8_t x = 0; x < TETRIS_CELL_SIZE; x++) {
                        fb_set_pixel_hsv(px + x, py + y, 0, 0, 10);
                    }
                }
            } else {
                // Filled cell - use piece color
                uint8_t hue = tetris_pieces[cell - 1].hue;

                // Fill cell
                for (uint8_t y = 1; y < TETRIS_CELL_SIZE - 1; y++) {
                    for (uint8_t x = 1; x < TETRIS_CELL_SIZE - 1; x++) {
                        fb_set_pixel_hsv(px + x, py + y, hue, 255, 200);
                    }
                }

                // Draw border
                for (uint8_t i = 0; i < TETRIS_CELL_SIZE; i++) {
                    fb_set_pixel_hsv(px + i, py, hue, 255, 255);  // Top
                    fb_set_pixel_hsv(px + i, py + TETRIS_CELL_SIZE - 1, hue, 100, 100);  // Bottom
                    fb_set_pixel_hsv(px, py + i, hue, 255, 255);  // Left
                    fb_set_pixel_hsv(px + TETRIS_CELL_SIZE - 1, py + i, hue, 100, 100);  // Right
                }
            }
        }
    }

    // Highlight lines being cleared
    if (g_tetris.mode == TETRIS_LINE_CLEAR_ANIM) {
        for (uint8_t i = 0; i < g_tetris.num_lines_to_clear; i++) {
            uint8_t row = g_tetris.lines_to_clear[i];
            int16_t py = TETRIS_BOARD_Y_OFFSET + row * TETRIS_CELL_SIZE;

            for (uint8_t col = 0; col < TETRIS_BOARD_WIDTH; col++) {
                int16_t px = TETRIS_BOARD_X_OFFSET + col * TETRIS_CELL_SIZE;

                // Flash white
                for (uint8_t y = 0; y < TETRIS_CELL_SIZE; y++) {
                    for (uint8_t x = 0; x < TETRIS_CELL_SIZE; x++) {
                        fb_set_pixel_hsv(px + x, py + y, 0, 0, 255);
                    }
                }
            }
        }
    }
}

// Draw a tetromino piece
static void draw_piece(tetris_piece_t *piece, bool ghost) {
    if (piece->type >= TETRIS_NUM_PIECES) return;

    const tetris_piece_def_t *piece_def = &tetris_pieces[piece->type];

    for (uint8_t row = 0; row < 4; row++) {
        for (uint8_t col = 0; col < 4; col++) {
            if (piece_def->blocks[piece->rotation][row][col]) {
                int8_t board_x = piece->x + col;
                int8_t board_y = piece->y + row;

                if (board_x >= 0 && board_x < TETRIS_BOARD_WIDTH &&
                    board_y >= 0 && board_y < TETRIS_BOARD_HEIGHT) {

                    int16_t px = TETRIS_BOARD_X_OFFSET + board_x * TETRIS_CELL_SIZE;
                    int16_t py = TETRIS_BOARD_Y_OFFSET + board_y * TETRIS_CELL_SIZE;

                    if (ghost) {
                        // Ghost piece - outline only
                        for (uint8_t i = 0; i < TETRIS_CELL_SIZE; i++) {
                            fb_set_pixel_hsv(px + i, py, piece_def->hue, 255, 100);  // Top
                            fb_set_pixel_hsv(px + i, py + TETRIS_CELL_SIZE - 1, piece_def->hue, 255, 100);  // Bottom
                            fb_set_pixel_hsv(px, py + i, piece_def->hue, 255, 100);  // Left
                            fb_set_pixel_hsv(px + TETRIS_CELL_SIZE - 1, py + i, piece_def->hue, 255, 100);  // Right
                        }
                    } else {
                        // Active piece - filled
                        for (uint8_t y = 1; y < TETRIS_CELL_SIZE - 1; y++) {
                            for (uint8_t x = 1; x < TETRIS_CELL_SIZE - 1; x++) {
                                fb_set_pixel_hsv(px + x, py + y, piece_def->hue, 255, 220);
                            }
                        }

                        // Border
                        for (uint8_t i = 0; i < TETRIS_CELL_SIZE; i++) {
                            fb_set_pixel_hsv(px + i, py, piece_def->hue, 255, 255);
                            fb_set_pixel_hsv(px + i, py + TETRIS_CELL_SIZE - 1, piece_def->hue, 100, 100);
                            fb_set_pixel_hsv(px, py + i, piece_def->hue, 255, 255);
                            fb_set_pixel_hsv(px + TETRIS_CELL_SIZE - 1, py + i, piece_def->hue, 100, 100);
                        }
                    }
                }
            }
        }
    }
}

// Draw score and UI
static void draw_score_ui(void) {
    // Draw score at top (centered)
    char score_str[6];
    snprintf(score_str, sizeof(score_str), "%u", g_tetris.score);

    int16_t x = 5;
    for (uint8_t i = 0; score_str[i] != '\0' && i < 5; i++) {
        if (score_str[i] >= '0' && score_str[i] <= '9') {
            draw_digit(x, 2, score_str[i] - '0', 2, 0);  // Red score
            x += 8;
        }
    }

    // Draw level
    draw_char(125, 2, 'L', 1, 42);  // Yellow "L"
    draw_digit(128, 7, g_tetris.level / 10, 1, 42);
    draw_digit(131, 7, g_tetris.level % 10, 1, 42);

    // Game over text
    if (g_tetris.game_over && g_tetris.mode == TETRIS_PLAYING) {
        const char *text = "GAME OVER";
        int16_t tx = 20;
        int16_t ty = 100;
        for (uint8_t i = 0; text[i] != '\0'; i++) {
            if (text[i] >= 'A' && text[i] <= 'Z') {
                draw_char(tx, ty, text[i], 2, 0);  // Red
                tx += 8;
            } else if (text[i] == ' ') {
                tx += 8;
            }
        }
    }
}

// Draw name entry screen
static void draw_name_entry(void) {
    // Dark background
    fb_rect_hsv(0, 0, TETRIS_DISPLAY_WIDTH - 1, TETRIS_DISPLAY_HEIGHT - 1, 0, 0, 20, true);

    // Title
    const char *title = g_tetris.offline_mode ? "OFFLINE MODE" : "HIGH SCORE!";
    int16_t x = 10;
    for (uint8_t i = 0; title[i] != '\0'; i++) {
        if (title[i] >= 'A' && title[i] <= 'Z') {
            draw_char(x, 10, title[i], 2, 42);  // Yellow
            x += 8;
        } else if (title[i] == ' ') {
            x += 8;
        }
    }

    // Score
    x = 20;
    const char *score_label = "SCORE";
    for (uint8_t i = 0; score_label[i] != '\0'; i++) {
        draw_char(x, 30, score_label[i], 2, 255);  // White
        x += 8;
    }

    // Score value
    char score_str[6];
    snprintf(score_str, sizeof(score_str), "%u", g_tetris.score);
    x = 35;
    for (uint8_t i = 0; score_str[i] != '\0'; i++) {
        if (score_str[i] >= '0' && score_str[i] <= '9') {
            draw_digit(x, 50, score_str[i] - '0', 3, 0);  // Red
            x += 12;
        }
    }

    // Name entry
    const char *name_label = "NAME";
    x = 35;
    for (uint8_t i = 0; name_label[i] != '\0'; i++) {
        draw_char(x, 80, name_label[i], 2, 255);  // White
        x += 8;
    }

    // Name input boxes
    for (uint8_t i = 0; i < 3; i++) {
        int16_t bx = 25 + i * 30;
        int16_t by = 100;

        // Character
        char c = g_tetris.name_entry.name[i];
        if (c == 0) c = 'A';  // Default to A

        // Highlight current character in green, others in red
        uint8_t hue = (i == g_tetris.name_entry.char_index) ? 85 : 0;

        draw_char(bx + 10, by + 5, c, 3, hue);

        // Box border
        for (int16_t j = 0; j < 28; j++) {
            fb_set_pixel_hsv(bx + j, by, hue, 255, 200);
            fb_set_pixel_hsv(bx + j, by + 24, hue, 255, 200);
        }
        for (int16_t j = 0; j < 25; j++) {
            fb_set_pixel_hsv(bx, by + j, hue, 255, 200);
            fb_set_pixel_hsv(bx + 27, by + j, hue, 255, 200);
        }
    }

    // Instructions
    const char *inst1 = "UP DN LETTER";
    const char *inst2 = "LT RT MOVE";
    const char *inst3 = "SHIFT SUBMIT";

    x = 5;
    for (uint8_t i = 0; inst1[i] != '\0'; i++) {
        if (inst1[i] >= 'A' && inst1[i] <= 'Z') {
            draw_char(x, 150, inst1[i], 1, 128);
            x += 4;
        } else if (inst1[i] == ' ') {
            x += 4;
        }
    }

    x = 5;
    for (uint8_t i = 0; inst2[i] != '\0'; i++) {
        if (inst2[i] >= 'A' && inst2[i] <= 'Z') {
            draw_char(x, 160, inst2[i], 1, 128);
            x += 4;
        } else if (inst2[i] == ' ') {
            x += 4;
        }
    }

    x = 5;
    for (uint8_t i = 0; inst3[i] != '\0'; i++) {
        if (inst3[i] >= 'A' && inst3[i] <= 'Z') {
            draw_char(x, 170, inst3[i], 1, 128);
            x += 4;
        } else if (inst3[i] == ' ') {
            x += 4;
        }
    }
}

// Draw score display screen
static void draw_score_display(void) {
    fb_rect_hsv(0, 0, TETRIS_DISPLAY_WIDTH - 1, TETRIS_DISPLAY_HEIGHT - 1, 0, 0, 20, true);

    if (g_tetris.offline_mode) {
        // Offline mode - just thank you
        const char *thank = "THANK YOU";
        int16_t x = 15;
        for (uint8_t i = 0; thank[i] != '\0'; i++) {
            if (thank[i] >= 'A' && thank[i] <= 'Z') {
                draw_char(x, 50, thank[i], 2, 42);
                x += 8;
            } else if (thank[i] == ' ') {
                x += 8;
            }
        }

        // Show name
        x = 45;
        for (uint8_t i = 0; i < 3; i++) {
            char c = g_tetris.name_entry.name[i];
            if (c == 0) c = 'A';
            draw_char(x, 80, c, 3, 85);  // Green
            x += 12;
        }

        // Offline indicator
        const char *offline = "OFFLINE MODE";
        x = 10;
        for (uint8_t i = 0; offline[i] != '\0'; i++) {
            if (offline[i] >= 'A' && offline[i] <= 'Z') {
                draw_char(x, 120, offline[i], 1, 0);  // Red
                x += 4;
            } else if (offline[i] == ' ') {
                x += 4;
            }
        }

        const char *inst = "SHIFT RESTART";
        x = 10;
        for (uint8_t i = 0; inst[i] != '\0'; i++) {
            if (inst[i] >= 'A' && inst[i] <= 'Z') {
                draw_char(x, 200, inst[i], 1, 128);
                x += 4;
            } else if (inst[i] == ' ') {
                x += 4;
            }
        }
    } else {
        // Online mode - show high scores
        const char *title = "HIGH SCORES";
        int16_t x = 10;
        for (uint8_t i = 0; title[i] != '\0'; i++) {
            if (title[i] >= 'A' && title[i] <= 'Z') {
                draw_char(x, 5, title[i], 2, 42);  // Yellow
                x += 8;
            } else if (title[i] == ' ') {
                x += 8;
            }
        }

        // Show scores
        for (uint8_t i = 0; i < g_tetris.highscore_count && i < 10; i++) {
            int16_t y = 30 + i * 20;

            // Rank
            draw_digit(5, y, (i + 1) / 10, 2, 255);
            draw_digit(12, y, (i + 1) % 10, 2, 255);

            // Name (green if player's score)
            uint8_t name_hue = (i == g_tetris.player_rank) ? 85 : 255;
            x = 25;
            for (uint8_t j = 0; j < 3; j++) {
                char c = g_tetris.highscores[i].name[j];
                if (c == 0) break;
                draw_char(x, y, c, 2, name_hue);
                x += 8;
            }

            // Score (white)
            char score_str[6];
            snprintf(score_str, sizeof(score_str), "%u", g_tetris.highscores[i].score);
            x = 60;
            for (uint8_t j = 0; score_str[j] != '\0'; j++) {
                if (score_str[j] >= '0' && score_str[j] <= '9') {
                    draw_digit(x, y, score_str[j] - '0', 2, 255);
                    x += 8;
                }
            }
        }

        // Instructions
        const char *inst = "SHIFT RESTART";
        x = 10;
        for (uint8_t i = 0; inst[i] != '\0'; i++) {
            if (inst[i] >= 'A' && inst[i] <= 'Z') {
                draw_char(x, 225, inst[i], 1, 128);
                x += 4;
            } else if (inst[i] == ' ') {
                x += 4;
            }
        }
    }
}

// Render game
void tetris_render(painter_device_t device) {
    if (!g_tetris.active) return;

    switch (g_tetris.mode) {
        case TETRIS_PLAYING:
        case TETRIS_LINE_CLEAR_ANIM:
            // Clear screen
            fb_rect_hsv(0, 0, TETRIS_DISPLAY_WIDTH - 1, TETRIS_DISPLAY_HEIGHT - 1, 0, 0, 0, true);  // Black background

            // Draw board
            draw_board();

            if (g_tetris.mode == TETRIS_PLAYING && !g_tetris.game_over) {
                // Draw ghost piece (where piece will land)
                tetris_piece_t ghost = g_tetris.current_piece;
                while (!check_collision(ghost.x, ghost.y + 1, ghost.rotation)) {
                    ghost.y++;
                }
                draw_piece(&ghost, true);

                // Draw current piece
                draw_piece(&g_tetris.current_piece, false);
            }

            // Draw UI
            draw_score_ui();
            break;

        case TETRIS_NAME_ENTRY:
            draw_name_entry();
            break;

        case TETRIS_SCORE_DISPLAY:
            draw_score_display();
            break;
    }

    // Flush framebuffer to display
    fb_flush_fullscreen(device);
}

// Set input state
void tetris_set_input(bool left, bool right, bool down, bool rotate) {
    g_tetris_input.left = left;
    g_tetris_input.right = right;
    g_tetris_input.down = down;
    g_tetris_input.rotate = rotate;
}

// Submit score via HID
static void submit_score_to_hid(void) {
    uint8_t data[32] = {0};
    data[0] = 0x14;  // MSG_TETRIS_SCORE_SUBMIT (0x14 for Tetris, 0x10 was Doodle Jump)
    data[1] = g_tetris.score & 0xFF;
    data[2] = (g_tetris.score >> 8) & 0xFF;
    raw_hid_send(data, sizeof(data));

    g_tetris.waiting_for_hid_response = true;
    g_tetris.hid_wait_start = timer_read32();
}

// Submit name via HID
static void submit_name_to_hid(void) {
    uint8_t data[32] = {0};
    data[0] = 0x17;  // MSG_TETRIS_NAME_SUBMIT (0x17)
    data[1] = g_tetris.name_entry.name[0];
    data[2] = g_tetris.name_entry.name[1];
    data[3] = g_tetris.name_entry.name[2];
    raw_hid_send(data, sizeof(data));

    g_tetris.waiting_for_hid_response = true;
    g_tetris.hid_wait_start = timer_read32();
}

// Process key input
bool tetris_process_record(uint16_t keycode, keyrecord_t *record, uint8_t *current_display_layer) {
    if (!g_tetris.active) return true;

    switch (g_tetris.mode) {
        case TETRIS_PLAYING:
        case TETRIS_LINE_CLEAR_ANIM:
            // Arrow keys for game control
            if (keycode == KC_LEFT) {
                tetris_set_input(record->event.pressed, g_tetris_input.right,
                               g_tetris_input.down, g_tetris_input.rotate);
                return false;
            }
            if (keycode == KC_RGHT) {
                tetris_set_input(g_tetris_input.left, record->event.pressed,
                               g_tetris_input.down, g_tetris_input.rotate);
                return false;
            }
            if (keycode == KC_DOWN) {
                tetris_set_input(g_tetris_input.left, g_tetris_input.right,
                               record->event.pressed, g_tetris_input.rotate);
                return false;
            }
            if (keycode == KC_UP) {
                tetris_set_input(g_tetris_input.left, g_tetris_input.right,
                               g_tetris_input.down, record->event.pressed);
                return false;
            }

            // Shift to exit game
            if ((keycode == KC_LSFT || keycode == KC_RSFT) && record->event.pressed) {
                tetris_cleanup();
                layer_clear();
                *current_display_layer = 255;  // Invalidate display
                return false;
            }
            break;

        case TETRIS_NAME_ENTRY:
            if (record->event.pressed) {
                if (keycode == KC_UP) {
                    // Next letter
                    g_tetris.name_entry.letter_index = (g_tetris.name_entry.letter_index + 1) % 26;
                    g_tetris.name_entry.name[g_tetris.name_entry.char_index] = 'A' + g_tetris.name_entry.letter_index;
                    return false;
                } else if (keycode == KC_DOWN) {
                    // Previous letter
                    g_tetris.name_entry.letter_index = (g_tetris.name_entry.letter_index + 25) % 26;
                    g_tetris.name_entry.name[g_tetris.name_entry.char_index] = 'A' + g_tetris.name_entry.letter_index;
                    return false;
                } else if (keycode == KC_LEFT) {
                    // Previous character
                    if (g_tetris.name_entry.char_index > 0) {
                        g_tetris.name_entry.char_index--;
                        g_tetris.name_entry.letter_index = g_tetris.name_entry.name[g_tetris.name_entry.char_index] - 'A';
                    }
                    return false;
                } else if (keycode == KC_RGHT) {
                    // Next character
                    if (g_tetris.name_entry.char_index < 2) {
                        g_tetris.name_entry.char_index++;
                        char c = g_tetris.name_entry.name[g_tetris.name_entry.char_index];
                        if (c == 0) c = 'A';
                        g_tetris.name_entry.letter_index = c - 'A';
                    }
                    return false;
                } else if (keycode == KC_LSFT || keycode == KC_RSFT) {
                    // Submit name
                    if (g_tetris.offline_mode) {
                        g_tetris.mode = TETRIS_SCORE_DISPLAY;
                    } else {
                        submit_name_to_hid();
                    }
                    return false;
                }
            }
            break;

        case TETRIS_SCORE_DISPLAY:
            if ((keycode == KC_LSFT || keycode == KC_RSFT) && record->event.pressed) {
                // Restart game
                tetris_init();
                return false;
            }
            break;
    }

    return true;
}

// Housekeeping task
bool tetris_housekeeping(painter_device_t display) {
    if (!g_tetris.active) return false;

    tetris_update();
    tetris_render(display);
    return true;
}

// Handle HID receive
void tetris_hid_receive(uint8_t *data, uint8_t length) {
    if (length < 2) return;

    uint8_t command = data[0];
    g_tetris.waiting_for_hid_response = false;

    switch (command) {
        case 0x15:  // MSG_TETRIS_ENTER_NAME - score made top 10
            g_tetris.mode = TETRIS_NAME_ENTRY;
            g_tetris.player_rank = data[1];  // Rank 0-9
            memset(&g_tetris.name_entry, 0, sizeof(tetris_name_entry_state_t));
            g_tetris.name_entry.name[0] = 'A';
            g_tetris.name_entry.name[1] = 'A';
            g_tetris.name_entry.name[2] = 'A';
            break;

        case 0x16:  // MSG_TETRIS_SHOW_SCORES - show high score table
            g_tetris.mode = TETRIS_SCORE_DISPLAY;

            // Parse high scores (format: count, then [name(3), score_lo, score_hi] * count)
            g_tetris.highscore_count = data[1];
            if (g_tetris.highscore_count > 10) g_tetris.highscore_count = 10;

            for (uint8_t i = 0; i < g_tetris.highscore_count; i++) {
                uint8_t offset = 2 + i * 5;
                g_tetris.highscores[i].name[0] = data[offset];
                g_tetris.highscores[i].name[1] = data[offset + 1];
                g_tetris.highscores[i].name[2] = data[offset + 2];
                g_tetris.highscores[i].name[3] = '\0';
                g_tetris.highscores[i].score = data[offset + 3] | (data[offset + 4] << 8);
            }
            break;
    }
}

// Cleanup
void tetris_cleanup(void) {
    g_tetris.active = false;
    memset(&g_tetris_input, 0, sizeof(tetris_input_t));
}

// Check if active
bool tetris_is_active(void) {
    return g_tetris.active;
}
