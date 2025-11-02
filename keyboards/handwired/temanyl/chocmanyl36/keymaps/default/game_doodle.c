// Copyright 2024
// SPDX-License-Identifier: GPL-2.0-or-later

#include "game_doodle.h"
#include "display/framebuffer.h"
#include "action.h"
#include "action_layer.h"
#include <stdlib.h>
#include <string.h>
#include "timer.h"
#include "raw_hid.h"

// HID message types (0x10-0x13 to avoid conflict with display HID messages)
#define MSG_SCORE_SUBMIT  0x10
#define MSG_ENTER_NAME    0x11
#define MSG_SHOW_SCORES   0x12
#define MSG_NAME_SUBMIT   0x13

// Global game state
game_state_t g_game;
input_state_t g_input;

// Helper function to generate random number in range
static int16_t random_range(int16_t min, int16_t max) {
    return min + (rand() % (max - min + 1));
}

// Initialize a platform at a specific y position
static void spawn_platform(uint8_t index, int16_t y) {
    g_game.platforms[index].x = random_range(0, GAME_WIDTH - PLATFORM_WIDTH);
    g_game.platforms[index].y = y;
    g_game.platforms[index].width = PLATFORM_WIDTH;
    g_game.platforms[index].active = true;
    g_game.platforms[index].scored = false;
}

// Send score to computer via Raw HID
static void send_score_to_computer(uint16_t score) {
    uint8_t data[32] = {0};
    data[0] = MSG_SCORE_SUBMIT;
    data[1] = (score >> 8) & 0xFF;  // High byte
    data[2] = score & 0xFF;          // Low byte
    raw_hid_send(data, sizeof(data));
}

// Send name and score to computer via Raw HID
static void send_name_to_computer(const char *name, uint16_t score) {
    uint8_t data[32] = {0};
    data[0] = MSG_NAME_SUBMIT;
    data[1] = name[0];
    data[2] = name[1];
    data[3] = name[2];
    data[4] = (score >> 8) & 0xFF;
    data[5] = score & 0xFF;
    raw_hid_send(data, sizeof(data));
}

// Initialize the game
void game_init(void) {
    // Initialize player in the middle of screen, more visible
    g_game.player.x = GAME_WIDTH / 2;
    g_game.player.y = GAME_HEIGHT - 50;  // Start higher for better visibility
    g_game.player.vx = 0;
    g_game.player.vy = 0;
    g_game.player.on_platform = false;

    // Initialize camera
    g_game.camera_y = 0;
    g_game.score = 0;
    g_game.active = true;
    g_game.game_over = false;
    g_game.last_update = timer_read32();

    // Initialize high score system
    g_game.mode = GAME_PLAYING;
    g_game.highscore_count = 0;
    g_game.player_rank = 255;
    g_game.waiting_for_hid_response = false;
    g_game.hid_wait_start = 0;
    g_game.offline_mode = false;
    memset(&g_game.name_entry, 0, sizeof(g_game.name_entry));
    memset(g_game.highscores, 0, sizeof(g_game.highscores));

    // Initialize platforms
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        g_game.platforms[i].active = false;
    }

    // Spawn initial platforms - ensure starting platform is centered under player
    // Starting platform: manually position it centered under the player
    g_game.platforms[0].x = (GAME_WIDTH / 2) - (PLATFORM_WIDTH / 2);  // Center under player
    g_game.platforms[0].y = GAME_HEIGHT - 30;  // Below player
    g_game.platforms[0].width = PLATFORM_WIDTH;
    g_game.platforms[0].active = true;
    g_game.platforms[0].scored = false;

    // Other platforms can be random
    spawn_platform(1, GAME_HEIGHT - 60);  // Platform at player level
    spawn_platform(2, GAME_HEIGHT - 90);  // Platform above
    spawn_platform(3, GAME_HEIGHT - 120); // Platform higher up

    // Initialize input
    g_input.left = false;
    g_input.right = false;
    g_input.up = false;
    g_input.down = false;
}

// Set input state
void game_set_input(bool left, bool right, bool up, bool down) {
    g_input.left = left;
    g_input.right = right;
    g_input.up = up;
    g_input.down = down;
}

// Check collision between player and platform
// Note: px, py are player position in WORLD SPACE
// platform->y is also in WORLD SPACE
static bool check_collision(int16_t px, int16_t py, platform_t *platform) {
    // Check horizontal overlap
    bool horizontal_overlap = (px + PLAYER_SIZE > platform->x) &&
                              (px < platform->x + platform->width);

    if (!horizontal_overlap) return false;

    // Check if player is falling down (vy >= 0)
    if (g_game.player.vy < 0) return false;

    // All in world space coordinates
    int16_t player_bottom = py + PLAYER_SIZE;
    int16_t platform_top = platform->y;
    int16_t platform_bottom = platform->y + PLATFORM_HEIGHT;

    // Check if player is landing on platform (within a reasonable range)
    // Player bottom should be at or just below platform top
    if (player_bottom >= platform_top && player_bottom <= platform_bottom + 4) {
        return true;
    }

    return false;
}

// Update game physics and logic
void game_update(void) {
    if (!g_game.active) return;

    // Handle different game modes
    if (g_game.mode == GAME_NAME_ENTRY || g_game.mode == GAME_SCORE_DISPLAY) {
        return;  // No physics updates in these modes
    }

    if (g_game.game_over) {
        // Transition to high score flow
        if (!g_game.waiting_for_hid_response) {
            g_game.waiting_for_hid_response = true;
            g_game.hid_wait_start = timer_read32();
            send_score_to_computer(g_game.score);
        } else {
            // Check for timeout (2 seconds) - if computer doesn't respond, go offline
            uint32_t elapsed = timer_elapsed32(g_game.hid_wait_start);
            if (elapsed > 2000) {  // 2 second timeout
                // Computer not responding, enter offline mode
                g_game.offline_mode = true;
                g_game.waiting_for_hid_response = false;

                // Go directly to name entry
                g_game.mode = GAME_NAME_ENTRY;
                g_game.name_entry.char_index = 0;
                g_game.name_entry.letter_index = 0;
                g_game.name_entry.name[0] = 'A';
                g_game.name_entry.name[1] = 'A';
                g_game.name_entry.name[2] = 'A';
            }
        }
        return;
    }

    uint32_t now = timer_read32();
    if (now - g_game.last_update < 10) return;  // ~100 FPS for smoother fullscreen rendering
    g_game.last_update = now;

    // Handle horizontal input
    if (g_input.left) {
        g_game.player.vx = -MOVE_SPEED;
    } else if (g_input.right) {
        g_game.player.vx = MOVE_SPEED;
    } else {
        g_game.player.vx = 0;
    }

    // Apply gravity
    g_game.player.vy += GRAVITY;
    if (g_game.player.vy > 15) g_game.player.vy = 15;  // Terminal velocity

    // Update player position
    g_game.player.x += g_game.player.vx;
    g_game.player.y += g_game.player.vy;

    // Wrap around horizontally
    if (g_game.player.x < -PLAYER_SIZE) {
        g_game.player.x = GAME_WIDTH;
    } else if (g_game.player.x > GAME_WIDTH) {
        g_game.player.x = -PLAYER_SIZE;
    }

    // Remove platforms that are off-screen (before collision detection)
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        if (!g_game.platforms[i].active) continue;
        int16_t platform_screen_y = g_game.platforms[i].y - g_game.camera_y;

        // Remove platforms that scrolled off the bottom or top
        if (platform_screen_y > GAME_HEIGHT + 20 || platform_screen_y < -PLATFORM_HEIGHT - 20) {
            g_game.platforms[i].active = false;
        }
    }

    // Check platform collisions
    g_game.player.on_platform = false;
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        if (!g_game.platforms[i].active) continue;

        // Only check collision if platform is visible on screen
        int16_t platform_screen_y = g_game.platforms[i].y - g_game.camera_y;
        if (platform_screen_y < -PLATFORM_HEIGHT || platform_screen_y > GAME_HEIGHT) {
            continue; // Skip collision check for off-screen platforms
        }

        if (check_collision(g_game.player.x, g_game.player.y, &g_game.platforms[i])) {
            // Position player on top of platform (in world space)
            g_game.player.y = g_game.platforms[i].y - PLAYER_SIZE;
            // Make player jump
            g_game.player.vy = JUMP_VELOCITY;
            g_game.player.on_platform = true;

            // Award point if this platform hasn't been scored yet
            if (!g_game.platforms[i].scored) {
                g_game.platforms[i].scored = true;
                g_game.score++;
            }
            break;
        }
    }

    // Update camera to follow player when jumping up
    // Keep player in bottom quarter (3/4 down the screen) to make falling easier
    int16_t player_screen_y = g_game.player.y - g_game.camera_y;
    int16_t target_y = (GAME_HEIGHT * 3) / 4;  // Bottom quarter position
    if (player_screen_y < target_y && g_game.player.vy < 0) {
        int16_t scroll = target_y - player_screen_y;
        g_game.camera_y -= scroll;
    }

    // Spawn new platforms as we scroll up
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        // Spawn new platforms at the top
        if (!g_game.platforms[i].active) {
            // Find the highest platform
            int16_t highest_y = GAME_HEIGHT;
            for (uint8_t j = 0; j < MAX_PLATFORMS; j++) {
                if (g_game.platforms[j].active && g_game.platforms[j].y < highest_y) {
                    highest_y = g_game.platforms[j].y;
                }
            }

            // Spawn new platform above the highest one
            if (highest_y - g_game.camera_y > PLATFORM_MAX_GAP) {
                int16_t new_y = highest_y - random_range(PLATFORM_MIN_GAP, PLATFORM_MAX_GAP);
                spawn_platform(i, new_y);
            }
        }
    }

    // Check game over (fell off bottom)
    if (player_screen_y > GAME_HEIGHT + 20) {
        g_game.game_over = true;
    }
}

// Draw the player
static void draw_player(int16_t screen_x, int16_t screen_y) {
    // Draw a simple character (green square with eyes)
    uint8_t h = 120, s = 255, v = 255;  // Green color

    // Body
    fb_rect_hsv(screen_x, screen_y, screen_x + PLAYER_SIZE, screen_y + PLAYER_SIZE, h, s, v, true);

    // Eyes (two white pixels)
    fb_set_pixel_hsv(screen_x + 1, screen_y + 2, 0, 0, 255);
    fb_set_pixel_hsv(screen_x + PLAYER_SIZE - 2, screen_y + 2, 0, 0, 255);
}

// Draw a platform
static void draw_platform(platform_t *platform) {
    int16_t screen_y = platform->y - g_game.camera_y;

    // Only draw if on screen
    if (screen_y < -PLATFORM_HEIGHT || screen_y > GAME_HEIGHT) return;

    // Draw platform (brown color)
    uint8_t h = 30, s = 200, v = 200;
    fb_rect_hsv(platform->x, screen_y,
            platform->x + platform->width, screen_y + PLATFORM_HEIGHT,
            h, s, v, true);
}

// Draw a digit (0-9) at position (x, y) using a simple 3x5 pixel font with scaling
static void draw_digit(int16_t x, int16_t y, uint8_t digit, uint8_t h, uint8_t s, uint8_t v, uint8_t scale) {
    if (digit > 9) return;

    // Simple 3x5 pixel font patterns (1 = pixel on)
    const uint8_t font[10][5] = {
        {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
        {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
        {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
        {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
        {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
        {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
        {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
        {0b111, 0b001, 0b001, 0b001, 0b001}, // 7
        {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
        {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
    };

    // Draw the digit pixel by pixel with scaling
    for (uint8_t row = 0; row < 5; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            if (font[digit][row] & (1 << (2 - col))) {
                // Draw a scaled pixel as a filled rectangle
                for (uint8_t sy = 0; sy < scale; sy++) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        fb_set_pixel_hsv(x + col * scale + sx, y + row * scale + sy, h, s, v);
                    }
                }
            }
        }
    }
}

// Draw the score in the top right corner
static void draw_score(void) {
    uint16_t score = g_game.score;
    uint8_t h = 0, s = 255, v = 255;  // Red color (hue=0, full saturation)
    uint8_t scale = 5;  // 5x scale makes digits 15x25 pixels

    // Calculate number of digits
    uint16_t temp = score;
    uint8_t num_digits = 0;
    if (score == 0) {        num_digits = 1;
    } else {
        while (temp > 0) {
            num_digits++;
            temp /= 10;
        }
    }

    // Draw each digit from right to left
    // Each digit is (3 * scale) pixels wide
    int16_t digit_width = 3 * scale;
    int16_t spacing = 2;  // Gap between digits
    // Position from the left to avoid right edge clipping
    int16_t x = 10;  // 10px left margin
    for (uint8_t i = 0; i < num_digits; i++) {
        uint8_t digit = score % 10;
        draw_digit(x + (i * (digit_width + spacing)), 10, digit, h, s, v, scale);  // 10px top margin
        score /= 10;
    }
}

// Draw a character at a position with scaling
static void draw_char(int16_t x, int16_t y, char c, uint8_t h, uint8_t s, uint8_t v, uint8_t scale) {
    // Simple 3x5 pixel font for letters A-Z
    const uint8_t font[26][5] = {
        {0b111, 0b101, 0b111, 0b101, 0b101}, // A
        {0b110, 0b101, 0b110, 0b101, 0b110}, // B
        {0b111, 0b100, 0b100, 0b100, 0b111}, // C
        {0b110, 0b101, 0b101, 0b101, 0b110}, // D
        {0b111, 0b100, 0b111, 0b100, 0b111}, // E
        {0b111, 0b100, 0b111, 0b100, 0b100}, // F
        {0b111, 0b100, 0b101, 0b101, 0b111}, // G
        {0b101, 0b101, 0b111, 0b101, 0b101}, // H
        {0b111, 0b010, 0b010, 0b010, 0b111}, // I
        {0b111, 0b001, 0b001, 0b101, 0b111}, // J
        {0b101, 0b110, 0b100, 0b110, 0b101}, // K
        {0b100, 0b100, 0b100, 0b100, 0b111}, // L
        {0b101, 0b111, 0b111, 0b101, 0b101}, // M
        {0b111, 0b101, 0b101, 0b101, 0b101}, // N
        {0b111, 0b101, 0b101, 0b101, 0b111}, // O
        {0b111, 0b101, 0b111, 0b100, 0b100}, // P
        {0b111, 0b101, 0b101, 0b111, 0b011}, // Q
        {0b111, 0b101, 0b110, 0b101, 0b101}, // R
        {0b111, 0b100, 0b111, 0b001, 0b111}, // S
        {0b111, 0b010, 0b010, 0b010, 0b010}, // T
        {0b101, 0b101, 0b101, 0b101, 0b111}, // U
        {0b101, 0b101, 0b101, 0b101, 0b010}, // V
        {0b101, 0b101, 0b111, 0b111, 0b101}, // W
        {0b101, 0b101, 0b010, 0b101, 0b101}, // X
        {0b101, 0b101, 0b111, 0b010, 0b010}, // Y
        {0b111, 0b001, 0b010, 0b100, 0b111}, // Z
    };

    if (c < 'A' || c > 'Z') return;
    uint8_t index = c - 'A';

    for (uint8_t row = 0; row < 5; row++) {
        for (uint8_t col = 0; col < 3; col++) {
            if (font[index][row] & (1 << (2 - col))) {
                for (uint8_t sy = 0; sy < scale; sy++) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        fb_set_pixel_hsv(x + col * scale + sx, y + row * scale + sy, h, s, v);
                    }
                }
            }
        }
    }
}

// Render name entry screen (arcade-style)
static void render_name_entry(void) {
    // Title: "NEW HIGH SCORE!"
    const char *title = "NEW HIGH SCORE!";
    int16_t title_y = 20;
    int16_t char_x = 10;

    for (uint8_t i = 0; title[i] != '\0'; i++) {
        if (title[i] >= 'A' && title[i] <= 'Z') {
            draw_char(char_x, title_y, title[i], 60, 255, 255, 2);  // Yellow
            char_x += 8;  // 3*2 + 2 spacing
        } else if (title[i] == ' ') {
            char_x += 6;
        } else if (title[i] == '!') {
            // Simple exclamation mark
            fb_rect_hsv(char_x, title_y, char_x + 2, title_y + 6, 60, 255, 255, true);
            fb_set_pixel_hsv(char_x, title_y + 8, 60, 255, 255);
            char_x += 5;
        }
    }

    // Show score
    int16_t score_y = 45;
    char_x = 10;
    const char *score_label = "SCORE: ";
    for (uint8_t i = 0; score_label[i] != '\0'; i++) {
        if (score_label[i] >= 'A' && score_label[i] <= 'Z') {
            draw_char(char_x, score_y, score_label[i], 0, 0, 255, 2);  // White
            char_x += 8;
        } else if (score_label[i] == ' ' || score_label[i] == ':') {
            char_x += 6;
        }
    }

    // Draw score digits
    uint16_t score = g_game.score;
    uint8_t num_digits = 0;
    uint16_t temp = score;
    if (score == 0) {
        num_digits = 1;
    } else {
        while (temp > 0) {
            num_digits++;
            temp /= 10;
        }
    }

    for (uint8_t i = 0; i < num_digits; i++) {
        draw_digit(char_x + (i * 10), score_y, score % 10, 0, 0, 255, 2);
        score /= 10;
    }

    // "ENTER NAME:" label
    int16_t name_label_y = 80;
    char_x = 15;
    const char *name_label = "ENTER NAME:";
    for (uint8_t i = 0; name_label[i] != '\0'; i++) {
        if (name_label[i] >= 'A' && name_label[i] <= 'Z') {
            draw_char(char_x, name_label_y, name_label[i], 0, 0, 255, 2);
            char_x += 8;
        } else if (name_label[i] == ' ' || name_label[i] == ':') {
            char_x += 6;
        }
    }

    // Draw the three character slots
    int16_t slot_y = 110;
    int16_t slot_spacing = 8;  // Reduced from 35 to bring letters closer
    int16_t slot_width = 20;   // Character box width (scale 4 = 12px + padding)
    int16_t slot_start_x = (GAME_WIDTH - (3 * slot_width + 2 * slot_spacing)) / 2;

    for (uint8_t i = 0; i < 3; i++) {
        int16_t slot_x = slot_start_x + i * (slot_width + slot_spacing);

        // Highlight current character with bright color
        bool is_current = (i == g_game.name_entry.char_index);
        uint8_t h = is_current ? 120 : 0;    // Green if current, red otherwise
        uint8_t s = is_current ? 255 : 0;     // Full saturation if current
        uint8_t v = is_current ? 255 : 200;   // Bright if current

        // Draw character box (tighter bounds)
        fb_rect_hsv(slot_x - 2, slot_y - 2, slot_x + slot_width + 2, slot_y + 26, h, s, v, false);

        // Draw the character (centered in smaller box)
        char letter = 'A' + (i == g_game.name_entry.char_index ? g_game.name_entry.letter_index :
                            (g_game.name_entry.name[i] ? g_game.name_entry.name[i] - 'A' : 0));
        draw_char(slot_x + 4, slot_y + 3, letter, h, s, v, 4);
    }

    // Instructions
    int16_t inst_y = 170;
    char_x = 5;
    const char *inst = "UP/DOWN: LETTER";
    for (uint8_t i = 0; inst[i] != '\0'; i++) {
        if (inst[i] >= 'A' && inst[i] <= 'Z') {
            draw_char(char_x, inst_y, inst[i], 0, 0, 200, 1);
            char_x += 4;
        } else if (inst[i] == ' ' || inst[i] == '/' || inst[i] == ':') {
            char_x += 3;
        }
    }

    inst_y += 15;
    char_x = 5;
    const char *inst2 = "LEFT/RIGHT: CHAR";
    for (uint8_t i = 0; inst2[i] != '\0'; i++) {
        if (inst2[i] >= 'A' && inst2[i] <= 'Z') {
            draw_char(char_x, inst_y, inst2[i], 0, 0, 200, 1);
            char_x += 4;
        } else if (inst2[i] == ' ' || inst2[i] == '/' || inst2[i] == ':') {
            char_x += 3;
        }
    }

    inst_y += 15;
    char_x = 5;
    const char *inst3 = "SHIFT: SUBMIT";
    for (uint8_t i = 0; inst3[i] != '\0'; i++) {
        if (inst3[i] >= 'A' && inst3[i] <= 'Z') {
            draw_char(char_x, inst_y, inst3[i], 0, 0, 200, 1);
            char_x += 4;
        } else if (inst3[i] == ' ' || inst3[i] == ':') {
            char_x += 3;
        }
    }

    // Show offline mode indicator if applicable
    if (g_game.offline_mode) {
        inst_y += 20;
        char_x = 20;
        const char *offline = "OFFLINE MODE";
        for (uint8_t i = 0; offline[i] != '\0'; i++) {
            if (offline[i] >= 'A' && offline[i] <= 'Z') {
                draw_char(char_x, inst_y, offline[i], 0, 255, 128, 1);  // Red-ish
                char_x += 4;
            } else if (offline[i] == ' ') {
                char_x += 3;
            }
        }
    }
}

// Render high score display screen
static void render_score_display(void) {
    // Check if offline mode (no scores to display)
    if (g_game.offline_mode && g_game.highscore_count == 0) {
        // Offline mode - show thank you message
        int16_t title_y = 40;
        int16_t char_x = 25;
        const char *title = "THANK YOU";
        for (uint8_t i = 0; title[i] != '\0'; i++) {
            if (title[i] >= 'A' && title[i] <= 'Z') {
                draw_char(char_x, title_y, title[i], 60, 255, 255, 3);  // Yellow, larger
                char_x += 11;
            } else if (title[i] == ' ') {
                char_x += 8;
            }
        }

        // Show entered name (tighter spacing)
        int16_t name_y = 80;
        char_x = 50;  // More centered
        for (uint8_t i = 0; i < 3; i++) {
            draw_char(char_x, name_y, g_game.name_entry.name[i], 120, 255, 255, 4);  // Green, large
            char_x += 14;  // Reduced from 18 to bring letters closer
        }

        // Show score
        int16_t score_y = 130;
        const char *score_label = "SCORE:";
        char_x = 30;
        for (uint8_t i = 0; score_label[i] != '\0'; i++) {
            if (score_label[i] >= 'A' && score_label[i] <= 'Z') {
                draw_char(char_x, score_y, score_label[i], 0, 0, 255, 2);
                char_x += 8;
            } else if (score_label[i] == ':') {
                char_x += 5;
            }
        }

        // Draw score
        uint16_t score = g_game.score;
        uint8_t num_digits = 0;
        uint16_t temp = score;
        if (score == 0) {
            num_digits = 1;
        } else {
            while (temp > 0) {
                num_digits++;
                temp /= 10;
            }
        }

        for (uint8_t i = 0; i < num_digits; i++) {
            draw_digit(char_x + (i * 10), score_y, score % 10, 0, 0, 255, 2);
            score /= 10;
        }

        // Offline note
        int16_t note_y = 170;
        const char *note = "OFFLINE MODE";
        char_x = 15;
        for (uint8_t i = 0; note[i] != '\0'; i++) {
            if (note[i] >= 'A' && note[i] <= 'Z') {
                draw_char(char_x, note_y, note[i], 0, 255, 128, 1);  // Red-ish
                char_x += 4;
            } else if (note[i] == ' ') {
                char_x += 3;
            }
        }

        note_y += 12;
        const char *note2 = "START PYTHON SCRIPT";
        char_x = 8;
        for (uint8_t i = 0; note2[i] != '\0'; i++) {
            if (note2[i] >= 'A' && note2[i] <= 'Z') {
                draw_char(char_x, note_y, note2[i], 0, 255, 128, 1);
                char_x += 4;
            } else if (note2[i] == ' ') {
                char_x += 3;
            }
        }

        note_y += 12;
        const char *note3 = "TO SAVE SCORES";
        char_x = 15;
        for (uint8_t i = 0; note3[i] != '\0'; i++) {
            if (note3[i] >= 'A' && note3[i] <= 'Z') {
                draw_char(char_x, note_y, note3[i], 0, 255, 128, 1);
                char_x += 4;
            } else if (note3[i] == ' ') {
                char_x += 3;
            }
        }

        // Instructions
        int16_t inst_y = 220;
        int16_t inst_x = 15;
        const char *inst = "SHIFT: RESTART";
        for (uint8_t i = 0; inst[i] != '\0'; i++) {
            if (inst[i] >= 'A' && inst[i] <= 'Z') {
                draw_char(inst_x, inst_y, inst[i], 0, 0, 200, 1);
                inst_x += 4;
            } else if (inst[i] == ' ' || inst[i] == ':') {
                inst_x += 3;
            }
        }
        return;
    }

    // Online mode - show actual high scores
    // Title
    int16_t title_y = 15;
    int16_t char_x = 15;
    const char *title = "HIGH SCORES";
    for (uint8_t i = 0; title[i] != '\0'; i++) {
        if (title[i] >= 'A' && title[i] <= 'Z') {
            draw_char(char_x, title_y, title[i], 60, 255, 255, 2);  // Yellow
            char_x += 8;
        } else if (title[i] == ' ') {
            char_x += 6;
        }
    }

    // Display each high score
    int16_t entry_y = 45;
    int16_t entry_spacing = 18;

    for (uint8_t i = 0; i < g_game.highscore_count && i < 10; i++) {
        int16_t y = entry_y + i * entry_spacing;

        // Rank number
        draw_digit(8, y, (i + 1) / 10, 0, 0, 255, 2);
        draw_digit(16, y, (i + 1) % 10, 0, 0, 255, 2);

        // Period
        fb_set_pixel_hsv(24, y + 8, 0, 0, 255);

        // Name (3 characters)
        char_x = 35;
        for (uint8_t j = 0; j < 3; j++) {
            char c = g_game.highscores[i].name[j];
            if (c >= 'A' && c <= 'Z') {
                draw_char(char_x, y, c, 120, 255, 255, 2);  // Green
            }
            char_x += 10;
        }

        // Score
        char_x = 75;
        uint16_t score = g_game.highscores[i].score;
        uint8_t digits[5];
        uint8_t num_digits = 0;

        if (score == 0) {
            digits[0] = 0;
            num_digits = 1;
        } else {
            while (score > 0 && num_digits < 5) {
                digits[num_digits++] = score % 10;
                score /= 10;
            }
        }

        for (int8_t j = num_digits - 1; j >= 0; j--) {
            draw_digit(char_x, y, digits[j], 0, 0, 255, 2);
            char_x += 10;
        }
    }

    // Instructions
    int16_t inst_y = 220;
    int16_t inst_x = 15;
    const char *inst = "SHIFT: RESTART";
    for (uint8_t i = 0; inst[i] != '\0'; i++) {
        if (inst[i] >= 'A' && inst[i] <= 'Z') {
            draw_char(inst_x, inst_y, inst[i], 0, 0, 200, 1);
            inst_x += 4;
        } else if (inst[i] == ' ' || inst[i] == ':') {
            inst_x += 3;
        }
    }
}

// Render the game
void game_render(painter_device_t device) {
    if (!g_game.active) return;

    // Clear framebuffer
    fb_color_t bg_color;

    if (g_game.mode == GAME_NAME_ENTRY) {
        // Name entry screen - dark background
        bg_color = fb_hsv_to_rgb565(0, 0, 30);
        fb_clear(bg_color);
        render_name_entry();
    } else if (g_game.mode == GAME_SCORE_DISPLAY) {
        // Score display screen - dark background
        bg_color = fb_hsv_to_rgb565(0, 0, 30);
        fb_clear(bg_color);
        render_score_display();
    } else {
        // Normal gameplay - sky blue
        bg_color = fb_hsv_to_rgb565(150, 180, 255);
        fb_clear(bg_color);

        // Draw platforms
        for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
            if (g_game.platforms[i].active) {
                draw_platform(&g_game.platforms[i]);
            }
        }

        // Draw player
        int16_t player_screen_y = g_game.player.y - g_game.camera_y;
        draw_player(g_game.player.x, player_screen_y);

        // Draw score in top left corner
        draw_score();

        // Draw game over text if needed
        if (g_game.game_over) {
            // Draw "GAME OVER" background in center
            fb_rect_hsv(GAME_WIDTH/2 - 30, GAME_HEIGHT/2 - 10,
                    GAME_WIDTH/2 + 30, GAME_HEIGHT/2 + 10,
                    0, 255, 255, true);
        }
    }

    // Flush entire framebuffer to display (fullscreen, bypasses FB_SPLIT_Y)
    fb_flush_fullscreen(device);
}

// Cleanup game resources
void game_cleanup(void) {
    g_game.active = false;
    g_game.game_over = false;
}

// Check if game is active
bool game_is_active(void) {
    return g_game.active;
}

// Handle keypresses for the game
bool game_process_record(uint16_t keycode, keyrecord_t *record, uint8_t *current_display_layer) {
    if (!g_game.active) return true;

    // Handle name entry mode
    if (g_game.mode == GAME_NAME_ENTRY && record->event.pressed) {
        switch (keycode) {
            case KC_UP:
                // Cycle letter up (A-Z)
                g_game.name_entry.letter_index = (g_game.name_entry.letter_index + 1) % 26;
                g_game.name_entry.name[g_game.name_entry.char_index] = 'A' + g_game.name_entry.letter_index;
                return false;

            case KC_DOWN:
                // Cycle letter down (A-Z)
                if (g_game.name_entry.letter_index == 0) {
                    g_game.name_entry.letter_index = 25;
                } else {
                    g_game.name_entry.letter_index--;
                }
                g_game.name_entry.name[g_game.name_entry.char_index] = 'A' + g_game.name_entry.letter_index;
                return false;

            case KC_RIGHT:
                // Move to next character
                g_game.name_entry.char_index = (g_game.name_entry.char_index + 1) % 3;
                g_game.name_entry.letter_index = g_game.name_entry.name[g_game.name_entry.char_index] - 'A';
                return false;

            case KC_LEFT:
                // Move to previous character
                if (g_game.name_entry.char_index == 0) {
                    g_game.name_entry.char_index = 2;
                } else {
                    g_game.name_entry.char_index--;
                }
                g_game.name_entry.letter_index = g_game.name_entry.name[g_game.name_entry.char_index] - 'A';
                return false;

            case KC_LSFT:
            case KC_RSFT:
                // Submit name
                if (!g_game.offline_mode) {
                    // Online mode: send to computer and wait for score list
                    send_name_to_computer(g_game.name_entry.name, g_game.score);
                    // Will transition to SCORE_DISPLAY when computer responds
                } else {
                    // Offline mode: just show a simple restart screen
                    g_game.mode = GAME_SCORE_DISPLAY;
                    g_game.highscore_count = 0;  // No scores to display
                }
                return false;

            default:
                return false;  // Ignore other keys
        }
    }

    // Handle score display mode
    if (g_game.mode == GAME_SCORE_DISPLAY && record->event.pressed) {
        if (keycode == KC_LSFT || keycode == KC_RSFT) {
            // Restart game
            game_init();
            return false;
        }
        return false;  // Ignore other keys
    }

    // Handle normal gameplay
    // Handle shift keys to exit game
    if (keycode == KC_LSFT || keycode == KC_RSFT) {
        if (record->event.pressed) {
            layer_clear();
            game_cleanup();
            // Invalidate display cache to force full redraw
            if (current_display_layer != NULL) {
                *current_display_layer = 255;
            }
            return false;
        }
    }

    // Handle arrow key input
    bool pressed = record->event.pressed;
    switch (keycode) {
        case KC_LEFT:
            game_set_input(pressed, g_input.right, g_input.up, g_input.down);
            return false;
        case KC_RIGHT:
            game_set_input(g_input.left, pressed, g_input.up, g_input.down);
            return false;
        case KC_UP:
            game_set_input(g_input.left, g_input.right, pressed, g_input.down);
            return false;
        case KC_DOWN:
            game_set_input(g_input.left, g_input.right, g_input.up, pressed);
            return false;
        default:
            // Let other keys be processed normally
            return true;
    }
}

// Handle Raw HID data from computer (high score responses)
void game_hid_receive(uint8_t *data, uint8_t length) {
    if (!g_game.active || length < 1) return;

    uint8_t msg_type = data[0];

    if (msg_type == MSG_ENTER_NAME) {
        // Score made top 10, enter name
        if (length >= 2) {
            g_game.player_rank = data[1];
        }

        // Initialize name entry
        g_game.mode = GAME_NAME_ENTRY;
        g_game.name_entry.char_index = 0;
        g_game.name_entry.letter_index = 0;
        g_game.name_entry.name[0] = 'A';
        g_game.name_entry.name[1] = 'A';
        g_game.name_entry.name[2] = 'A';
        g_game.waiting_for_hid_response = false;
        g_game.offline_mode = false;  // Computer responded, we're online

    } else if (msg_type == MSG_SHOW_SCORES) {
        // Receive high score list
        g_game.mode = GAME_SCORE_DISPLAY;
        g_game.highscore_count = 0;

        // Parse high scores (each entry: 3 chars + 2 bytes score = 5 bytes)
        uint8_t offset = 1;
        for (uint8_t i = 0; i < 10 && offset + 5 <= length; i++) {
            // Read name (3 chars)
            g_game.highscores[i].name[0] = data[offset++];
            g_game.highscores[i].name[1] = data[offset++];
            g_game.highscores[i].name[2] = data[offset++];
            g_game.highscores[i].name[3] = '\0';

            // Read score (2 bytes, big-endian)
            g_game.highscores[i].score = (data[offset] << 8) | data[offset + 1];
            offset += 2;

            g_game.highscore_count++;
        }

        g_game.waiting_for_hid_response = false;
        g_game.offline_mode = false;  // Computer responded, we're online
    }
}

// Handle game update and rendering in housekeeping loop
bool game_housekeeping(painter_device_t display) {
    if (!g_game.active) return false;

    game_update();
    game_render(display);
    return true;  // Game handled the update
}
