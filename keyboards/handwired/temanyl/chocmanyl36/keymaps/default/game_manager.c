// Copyright 2024
// SPDX-License-Identifier: GPL-2.0-or-later

#include "game_manager.h"
#include "game_doodle.h"
#include "game_tetris.h"
#include "display/framebuffer.h"
#include "display/display.h"
#include <string.h>

// Global game manager state
game_manager_state_t g_game_manager = {0};

// Forward declarations
static void draw_selection_screen(void);
static void draw_char(int16_t x, int16_t y, char c, uint8_t scale, uint8_t hue);
static void start_selected_game(void);

// Draw a character (A-Z, 0-9, and some symbols)
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

    // Numbers
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

    const uint8_t *glyph = NULL;

    if (c >= 'A' && c <= 'Z') {
        glyph = font[c - 'A'];
    } else if (c >= '0' && c <= '9') {
        glyph = digits[c - '0'];
    } else {
        return;  // Unsupported character
    }

    for (uint8_t row = 0; row < 5; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            if (glyph[row] & (1 << (2 - col))) {
                for (uint8_t sy = 0; sy < scale; sy++) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        fb_set_pixel_hsv(x + col * scale + sx, y + row * scale + sy, hue, 255, 255);
                    }
                }
            }
        }
    }
}

// Draw game selection screen
static void draw_selection_screen(void) {
    // Dark blue background
    fb_rect_hsv(0, 0, 134, 239, 170, 200, 30, true);

    // Title
    const char *title = "SELECT GAME";
    int16_t x = 15;
    for (uint8_t i = 0; title[i] != '\0'; i++) {
        if (title[i] >= 'A' && title[i] <= 'Z') {
            draw_char(x, 20, title[i], 2, 42);  // Yellow
            x += 8;
        } else if (title[i] == ' ') {
            x += 8;
        }
    }

    // Game 1: Doodle Jump (left side)
    int16_t g1_x = 20;
    int16_t g1_y = 80;
    uint8_t g1_hue = (g_game_manager.selected_game == GAME_DOODLE) ? 85 : 255;  // Green if selected, white otherwise

    // Draw box
    for (int16_t i = 0; i < 50; i++) {
        for (int16_t j = 0; j < 60; j++) {
            fb_set_pixel_hsv(g1_x + i, g1_y + j, g1_hue, (g_game_manager.selected_game == GAME_DOODLE) ? 255 : 0, 100);
        }
    }

    // Draw border
    for (int16_t i = 0; i < 50; i++) {
        fb_set_pixel_hsv(g1_x + i, g1_y, g1_hue, 255, 255);
        fb_set_pixel_hsv(g1_x + i, g1_y + 59, g1_hue, 255, 255);
    }
    for (int16_t j = 0; j < 60; j++) {
        fb_set_pixel_hsv(g1_x, g1_y + j, g1_hue, 255, 255);
        fb_set_pixel_hsv(g1_x + 49, g1_y + j, g1_hue, 255, 255);
    }

    // "1" in the box
    draw_char(g1_x + 20, g1_y + 25, '1', 3, g1_hue);

    // "DOODLE" label below
    const char *doodle = "DOODLE";
    x = g1_x - 5;
    for (uint8_t i = 0; doodle[i] != '\0'; i++) {
        if (doodle[i] >= 'A' && doodle[i] <= 'Z') {
            draw_char(x, g1_y + 65, doodle[i], 1, g1_hue);
            x += 4;
        }
    }

    // Game 2: Tetris (right side)
    int16_t g2_x = 75;
    int16_t g2_y = 80;
    uint8_t g2_hue = (g_game_manager.selected_game == GAME_TETRIS) ? 85 : 255;  // Green if selected

    // Draw box
    for (int16_t i = 0; i < 50; i++) {
        for (int16_t j = 0; j < 60; j++) {
            fb_set_pixel_hsv(g2_x + i, g2_y + j, g2_hue, (g_game_manager.selected_game == GAME_TETRIS) ? 255 : 0, 100);
        }
    }

    // Draw border
    for (int16_t i = 0; i < 50; i++) {
        fb_set_pixel_hsv(g2_x + i, g2_y, g2_hue, 255, 255);
        fb_set_pixel_hsv(g2_x + i, g2_y + 59, g2_hue, 255, 255);
    }
    for (int16_t j = 0; j < 60; j++) {
        fb_set_pixel_hsv(g2_x, g2_y + j, g2_hue, 255, 255);
        fb_set_pixel_hsv(g2_x + 49, g2_y + j, g2_hue, 255, 255);
    }

    // "2" in the box
    draw_char(g2_x + 20, g2_y + 25, '2', 3, g2_hue);

    // "TETRIS" label below
    const char *tetris = "TETRIS";
    x = g2_x + 3;
    for (uint8_t i = 0; tetris[i] != '\0'; i++) {
        if (tetris[i] >= 'A' && tetris[i] <= 'Z') {
            draw_char(x, g2_y + 65, tetris[i], 1, g2_hue);
            x += 4;
        }
    }

    // Instructions
    const char *inst1 = "LT RT SELECT";
    const char *inst2 = "UP START";
    const char *inst3 = "SHIFT EXIT";

    x = 15;
    for (uint8_t i = 0; inst1[i] != '\0'; i++) {
        if (inst1[i] >= 'A' && inst1[i] <= 'Z') {
            draw_char(x, 170, inst1[i], 1, 128);  // Cyan
            x += 4;
        } else if (inst1[i] == ' ') {
            x += 4;
        }
    }

    x = 25;
    for (uint8_t i = 0; inst2[i] != '\0'; i++) {
        if (inst2[i] >= 'A' && inst2[i] <= 'Z') {
            draw_char(x, 185, inst2[i], 1, 128);
            x += 4;
        } else if (inst2[i] == ' ') {
            x += 4;
        }
    }

    x = 20;
    for (uint8_t i = 0; inst3[i] != '\0'; i++) {
        if (inst3[i] >= 'A' && inst3[i] <= 'Z') {
            draw_char(x, 200, inst3[i], 1, 128);
            x += 4;
        } else if (inst3[i] == ' ') {
            x += 4;
        }
    }
}

// Start the selected game
static void start_selected_game(void) {
    g_game_manager.current_game = g_game_manager.selected_game;

    if (g_game_manager.current_game == GAME_DOODLE) {
        game_init();  // Initialize Doodle Jump
    } else if (g_game_manager.current_game == GAME_TETRIS) {
        tetris_init();  // Initialize Tetris
    }
}

// Initialize game manager
void game_manager_init(void) {
    memset(&g_game_manager, 0, sizeof(game_manager_state_t));
    g_game_manager.active = true;
    g_game_manager.current_game = GAME_SELECTION;
    g_game_manager.selected_game = GAME_DOODLE;  // Default selection
}

// Update game manager
void game_manager_update(void) {
    if (!g_game_manager.active) return;

    // Delegate to active game
    if (g_game_manager.current_game == GAME_DOODLE) {
        game_update();
    } else if (g_game_manager.current_game == GAME_TETRIS) {
        tetris_update();
    }
    // Selection screen has no update logic
}

// Render game manager
void game_manager_render(painter_device_t device) {
    if (!g_game_manager.active) return;

    if (g_game_manager.current_game == GAME_SELECTION) {
        draw_selection_screen();
        fb_flush_fullscreen(device);
    } else if (g_game_manager.current_game == GAME_DOODLE) {
        game_render(device);
    } else if (g_game_manager.current_game == GAME_TETRIS) {
        tetris_render(device);
    }
}

// Cleanup game manager
void game_manager_cleanup(void) {
    // Cleanup any active game
    if (g_game_manager.current_game == GAME_DOODLE) {
        game_cleanup();
    } else if (g_game_manager.current_game == GAME_TETRIS) {
        tetris_cleanup();
    }

    g_game_manager.active = false;
    g_game_manager.current_game = GAME_NONE;
}

// Check if active
bool game_manager_is_active(void) {
    return g_game_manager.active;
}

// Process key input
bool game_manager_process_record(uint16_t keycode, keyrecord_t *record, uint8_t *current_display_layer) {
    if (!g_game_manager.active) return true;

    // Handle selection screen
    if (g_game_manager.current_game == GAME_SELECTION) {
        if (record->event.pressed) {
            if (keycode == KC_LEFT) {
                g_game_manager.selected_game = GAME_DOODLE;
                return false;
            } else if (keycode == KC_RGHT) {
                g_game_manager.selected_game = GAME_TETRIS;
                return false;
            } else if (keycode == KC_UP) {
                // Start selected game
                start_selected_game();
                return false;
            } else if (keycode == KC_LSFT || keycode == KC_RSFT) {
                // Exit to main menu
                game_manager_cleanup();
                layer_clear();
                *current_display_layer = 255;
                return false;
            }
        }
        return true;  // Don't handle other keys in selection
    }

    // Delegate to active game
    if (g_game_manager.current_game == GAME_DOODLE) {
        bool handled = game_process_record(keycode, record, current_display_layer);

        // Check if game was exited
        if (!game_is_active()) {
            // Return to selection screen
            g_game_manager.current_game = GAME_SELECTION;
            g_game_manager.selected_game = GAME_DOODLE;
        }

        return handled;
    } else if (g_game_manager.current_game == GAME_TETRIS) {
        bool handled = tetris_process_record(keycode, record, current_display_layer);

        // Check if game was exited
        if (!tetris_is_active()) {
            // Return to selection screen
            g_game_manager.current_game = GAME_SELECTION;
            g_game_manager.selected_game = GAME_TETRIS;
        }

        return handled;
    }

    return true;
}

// Housekeeping task
bool game_manager_housekeeping(painter_device_t display) {
    if (!g_game_manager.active) return false;

    game_manager_update();
    game_manager_render(display);
    return true;
}

// Handle HID receive
void game_manager_hid_receive(uint8_t *data, uint8_t length) {
    if (length < 1) return;

    uint8_t command = data[0];

    // Route to appropriate game based on command ID
    // Doodle Jump: 0x10-0x13
    // Tetris: 0x14-0x17
    if (command >= 0x10 && command <= 0x13) {
        game_hid_receive(data, length);
    } else if (command >= 0x14 && command <= 0x17) {
        tetris_hid_receive(data, length);
    }
}
