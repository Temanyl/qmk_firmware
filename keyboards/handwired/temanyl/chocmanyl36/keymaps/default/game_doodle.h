// Copyright 2024
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "quantum/painter/qp.h"

// Display dimensions
#define GAME_WIDTH 134
#define GAME_HEIGHT 121

// Game constants
#define MAX_PLATFORMS 8
#define PLATFORM_WIDTH 30
#define PLATFORM_HEIGHT 4
#define PLAYER_SIZE 6
#define GRAVITY 1
#define JUMP_VELOCITY -12
#define MOVE_SPEED 3
#define PLATFORM_SPAWN_Y -20
#define PLATFORM_MIN_GAP 20
#define PLATFORM_MAX_GAP 40

// Platform structure
typedef struct {
    int16_t x;
    int16_t y;
    uint8_t width;
    bool active;
} platform_t;

// Player structure
typedef struct {
    int16_t x;
    int16_t y;
    int16_t vx;  // velocity x
    int16_t vy;  // velocity y
    bool on_platform;
} player_t;

// Game state
typedef struct {
    player_t player;
    platform_t platforms[MAX_PLATFORMS];
    int16_t camera_y;  // Camera scroll position
    uint16_t score;
    bool active;
    bool game_over;
    uint32_t last_update;
} game_state_t;

// Input state
typedef struct {
    bool left;
    bool right;
    bool up;
    bool down;
} input_state_t;

// Function declarations
void game_init(void);
void game_update(void);
void game_render(painter_device_t device);
void game_set_input(bool left, bool right, bool up, bool down);
void game_cleanup(void);
bool game_is_active(void);

// Global game state
extern game_state_t g_game;
extern input_state_t g_input;
