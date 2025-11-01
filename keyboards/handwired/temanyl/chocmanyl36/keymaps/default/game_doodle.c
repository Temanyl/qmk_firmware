// Copyright 2024
// SPDX-License-Identifier: GPL-2.0-or-later

#include "game_doodle.h"
#include "display/framebuffer.h"
#include "action.h"
#include "action_layer.h"
#include <stdlib.h>
#include "timer.h"

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
    if (!g_game.active || g_game.game_over) return;

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

    // Check platform collisions
    g_game.player.on_platform = false;
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        if (!g_game.platforms[i].active) continue;

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
        int16_t platform_screen_y = g_game.platforms[i].y - g_game.camera_y;

        // Remove platforms that scrolled off the bottom
        if (platform_screen_y > GAME_HEIGHT + 20) {
            g_game.platforms[i].active = false;
        }

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

// Render the game
void game_render(painter_device_t device) {
    if (!g_game.active) return;

    // Clear framebuffer with sky blue
    fb_color_t sky_blue = fb_hsv_to_rgb565(150, 180, 255);
    fb_clear(sky_blue);

    // Draw platforms
    for (uint8_t i = 0; i < MAX_PLATFORMS; i++) {
        if (g_game.platforms[i].active) {
            draw_platform(&g_game.platforms[i]);
        }
    }

    // Draw player
    int16_t player_screen_y = g_game.player.y - g_game.camera_y;
    draw_player(g_game.player.x, player_screen_y);

    // Draw score in top right corner
    draw_score();

    // Draw game over text if needed
    if (g_game.game_over) {
        // Draw "GAME OVER" background in center
        fb_rect_hsv(GAME_WIDTH/2 - 30, GAME_HEIGHT/2 - 10,
                GAME_WIDTH/2 + 30, GAME_HEIGHT/2 + 10,
                0, 255, 255, true);
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

// Handle game update and rendering in housekeeping loop
bool game_housekeeping(painter_device_t display) {
    if (!g_game.active) return false;

    game_update();
    game_render(display);
    return true;  // Game handled the update
}
