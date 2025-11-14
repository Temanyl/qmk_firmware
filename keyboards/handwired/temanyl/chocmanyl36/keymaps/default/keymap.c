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
#include <stdlib.h>
#include "display/display.h"
#include "display/framebuffer.h"
#include "game_manager.h"
#include "weather_transition.h"
#include "scenes/scenes.h"

// Custom keycodes
enum custom_keycodes {
    DISP_UP = SAFE_RANGE,  // Display brightness up
    DISP_DN,               // Display brightness down
};

// Layer Names
enum layer_names {
    _MAC_COLEMAK_DH,
    _MAC_CODE,
    _MAC_NAV,
    _MAC_NUM,
    _MAC_ARROW
};

// Tap Dance declarations
enum {
    TD_Q_ESC_EMOJI_RESET,
    TD_ESC_WINDOWS_EMOJI,
    TD_LAYER_NAV_NUM,
    TD_LAYER_DEFAULT_SHIFT,
    TD_OSL_CODE
};

// Define a type for as many tap dance states as you need
typedef enum {
    TD_NONE,
    TD_UNKNOWN,
    TD_SINGLE_TAP,
    TD_SINGLE_HOLD,
    TD_DOUBLE_TAP
} td_state_t;

typedef struct {
    bool is_press_action;
    td_state_t state;
} td_tap_t;

// Forward declarations for tap dance functions
td_state_t cur_dance(tap_dance_state_t *state);
void nav_num_finished(tap_dance_state_t *state, void *user_data);
void nav_num_reset(tap_dance_state_t *state, void *user_data);
void layer_default_shift_finished(tap_dance_state_t *state, void *user_data);
void layer_default_shift_reset(tap_dance_state_t *state, void *user_data);
void osl_code_finished(tap_dance_state_t *state, void *user_data);
void osl_code_reset(tap_dance_state_t *state, void *user_data);

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_MAC_COLEMAK_DH] = LAYOUT_ortho_3x10_6(
         TD(TD_Q_ESC_EMOJI_RESET), KC_W,  KC_F,    KC_P,  KC_B,           KC_J,  KC_L,          KC_U,              KC_Y,           LT(0,KC_SCLN),
         KC_A, LCTL_T(KC_R), LALT_T(KC_S), LGUI_T(KC_T),  KC_G,           KC_M,  LGUI_T(KC_N),  LALT_T(KC_E),      LCTL_T(KC_I),   KC_O,
         KC_Z, KC_X,         KC_C,         KC_D,          KC_V,           KC_K,  KC_H,          KC_COMMA,          KC_DOT,         KC_SLSH,
                MEH_T(KC_TAB), KC_LSFT, KC_SPC,      KC_BSPC, TD(TD_LAYER_NAV_NUM), OSL(_MAC_CODE)
     ),
    [_MAC_CODE] = LAYOUT_ortho_3x10_6(
        KC_UNDS, KC_LT,   KC_GT,   KC_LCBR, KC_RCBR,        KC_PIPE,  KC_AT,   KC_BSLS, KC_GRAVE, KC_ENT,
        KC_EXLM, KC_MINS, KC_EQL,  KC_LPRN, KC_RPRN,        KC_AMPR,  KC_QUOT, KC_DOWN, KC_DQUO, KC_NO,
        KC_CIRC, KC_PLUS, KC_ASTR, KC_LBRC, KC_RBRC,        KC_TILDE, KC_DLR,  KC_PERC, KC_HASH, RSFT_T(KC_BSLS),
                            KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,        KC_BSPC,  TO(_MAC_NAV), KC_NO
    ),
    [_MAC_NAV] = LAYOUT_ortho_3x10_6(
        KC_ESC,  MS_BTN1, MS_UP, MS_BTN2, KC_NO,          KC_VOLU, KC_PGUP, KC_UP,    KC_PGDN, KC_ENT,
        KC_NO,   KC_LCTL, KC_LALT, KC_LGUI, KC_MPLY,        KC_MUTE, KC_LEFT, KC_DOWN,  KC_RGHT, KC_NO,
        KC_NO,   MS_LEFT, MS_DOWN, MS_RGHT, KC_NO,          KC_VOLD, TO(_MAC_ARROW), KC_NO,    KC_NO,   KC_NO,
                          KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,          KC_BSPC, KC_NO, TO(_MAC_CODE)
    ),
    [_MAC_NUM] = LAYOUT_ortho_3x10_6(
         KC_F1,   KC_F2, KC_F3,   KC_F4,   KC_F5,          KC_DOT,   KC_7,   KC_8,  KC_9,   KC_ENT,
         KC_F6,   KC_F7, KC_F8,   KC_F9,   KC_F10,         KC_COMMA, KC_4,   KC_5,  KC_6,   DISP_UP,
         KC_F11,  KC_F12,KC_LCTL, KC_LALT, KC_LGUI,        KC_0,     KC_1,   KC_2,  KC_3,   DISP_DN,
                         KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,           KC_BSPC, TO(_MAC_NAV), KC_NO
    ),
    [_MAC_ARROW] = LAYOUT_ortho_3x10_6(
         KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,          KC_NO,   KC_NO,   KC_UP,   KC_NO,   KC_NO,
         KC_NO,   KC_LSFT, KC_NO,   KC_NO,   KC_NO,          KC_NO,   KC_LEFT, KC_DOWN, KC_RGHT, KC_NO,
         KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,          KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,
                           KC_NO,   KC_RSFT,  KC_NO,                  KC_NO,   KC_NO,   KC_NO
    )
};

// tap dances again
// Determine the current tap dance state
td_state_t cur_dance(tap_dance_state_t *state) {
    if (state->count == 1) {
        if (!state->pressed) return TD_SINGLE_TAP;
        else return TD_SINGLE_HOLD;
    } else if (state->count == 2) return TD_DOUBLE_TAP;
    else return TD_UNKNOWN;
}

// Initialize tap structure associated with example tap dance key
static td_tap_t ql_tap_state = {
    .is_press_action = true,
    .state = TD_NONE
};

// Functions that control what our tap dance key does
void nav_num_finished(tap_dance_state_t *state, void *user_data) {
    ql_tap_state.state = cur_dance(state);
    switch (ql_tap_state.state) {
        case TD_SINGLE_TAP:
           // Check to see if the layer is already set
           if (layer_state_is(_MAC_NAV)) {
               // If already set, then switch it off
               layer_off(_MAC_NAV);
           } else {
               // If not already set, then switch the layer on
               layer_on(_MAC_NAV);
           }
           break;
        case TD_SINGLE_HOLD:
            layer_on(_MAC_NUM);
            break;
        case TD_DOUBLE_TAP:
            // Check to see if the layer is already set
            if (layer_state_is(_MAC_NUM)) {
                // If already set, then switch it off
                layer_off(_MAC_NUM);
            } else {
                // If not already set, then switch the layer on
                layer_on(_MAC_NUM);
            }
            break;
        default:
            break;
    }
}

void nav_num_reset(tap_dance_state_t *state, void *user_data) {
    // If the key was held down and now is released then switch off the layer
    if (ql_tap_state.state != TD_DOUBLE_TAP) {
        layer_off(_MAC_NUM);
    }
    ql_tap_state.state = TD_NONE;
}

// Functions that control what our tap dance key does
void layer_default_shift_finished(tap_dance_state_t *state, void *user_data) {
    ql_tap_state.state = cur_dance(state);
    switch (ql_tap_state.state) {
        case TD_SINGLE_TAP:
           layer_clear();
           break;
        case TD_SINGLE_HOLD:
            register_code(KC_LSFT);
            break;
        default:
            break;
    }
}

void layer_default_shift_reset(tap_dance_state_t *state, void *user_data) {
    // If the key was held down and now is released then switch off the layer
    if (ql_tap_state.state == TD_SINGLE_HOLD) {
         unregister_code(KC_LSFT);
    }
    ql_tap_state.state = TD_NONE;
}

void osl_code_finished(tap_dance_state_t *state, void *user_data) {
    ql_tap_state.state = cur_dance(state);
    switch (ql_tap_state.state) {
        case TD_SINGLE_TAP:
            set_oneshot_layer(_MAC_CODE, ONESHOT_START);
            break;
        case TD_SINGLE_HOLD:
            layer_on(_MAC_CODE);
            break;
        default:
            break;
    }
}

void osl_code_reset(tap_dance_state_t *state, void *user_data) {
    ql_tap_state.state = cur_dance(state);
    // If the key was held down and now is released then switch off the layer
    if (ql_tap_state.state == TD_SINGLE_TAP) {
        clear_oneshot_layer_state(ONESHOT_PRESSED);
    } else {
        layer_clear();
    }
}

void td_q_esc_emoji_reset (tap_dance_state_t *state, void *user_data) {
    if (state->count == 1) {
        tap_code(KC_Q);
    } else if (state->count == 2) {
        tap_code(KC_ESC);
    } else if (state->count == 3) {
        tap_code16(C(G(KC_SPC)));
    } else if (state->count == 5) {
        reset_keyboard();
    }
}

// Tap Dance definitions
tap_dance_action_t tap_dance_actions[] = {
    [TD_Q_ESC_EMOJI_RESET]   = ACTION_TAP_DANCE_FN(td_q_esc_emoji_reset),
    [TD_LAYER_NAV_NUM]       = ACTION_TAP_DANCE_FN_ADVANCED(NULL, nav_num_finished, nav_num_reset),
    [TD_LAYER_DEFAULT_SHIFT] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, layer_default_shift_finished, layer_default_shift_reset),
    [TD_OSL_CODE]            = ACTION_TAP_DANCE_FN_ADVANCED(NULL, osl_code_finished, osl_code_reset)
};

uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case TD(TD_Q_ESC_EMOJI_RESET):
        case TD(TD_ESC_WINDOWS_EMOJI):
        case LGUI_T(KC_SPC):
        case LT(1, KC_TAB):
        case LT(2, KC_ENT):
            return 200;
        case TD(TD_LAYER_DEFAULT_SHIFT):
            return 180;
        case LT(0,KC_SCLN):
            return 155;
        default:
            return TAPPING_TERM;
    }
}

bool send_hold_code(uint16_t keycode, keyrecord_t *record) {
    if (!record->tap.count && record->event.pressed) {
        tap_code16(G(keycode)); // Intercept hold function to send Ctrl-X
        return false;
    }
    return true;
}

// Initialize variable holding the binary representation of active modifiers
uint8_t mod_state;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    mod_state = get_mods();

    // Handle game manager input on arrow layer
    if (layer_state_is(_MAC_ARROW)) {
        if (!game_manager_process_record(keycode, record, &current_display_layer)) {
            return false;  // Game manager handled the key
        }
    }

    switch (keycode) {
        case LT(0,KC_SCLN):
            if (!record->tap.count && record->event.pressed) {
                tap_code(KC_ENT);
                return false;
            }
            return true;
        case KC_BSPC:
        {
            // Initialize a boolean variable that keeps track
            // of the delete key status: registered or not?
            static bool delkey_registered;
            if (record->event.pressed) {
                // Detect the activation of either shift keys
                if (mod_state & MOD_MASK_SHIFT) {
                    // First temporarily canceling both shifts so that
                    // shift isn't applied to the KC_DEL keycode
                    del_mods(MOD_MASK_SHIFT);
                    register_code(KC_DEL);
                    // Update the boolean variable to reflect the status of KC_DEL
                    delkey_registered = true;
                    // Reapplying modifier state so that the held shift key(s)
                    // still work even after having tapped the Backspace/Delete key.
                    set_mods(mod_state);
                    return false;
                }
            } else { // on release of KC_BSPC
                // In case KC_DEL is still being sent even after the release of KC_BSPC
                if (delkey_registered) {
                    unregister_code(KC_DEL);
                    delkey_registered = false;
                    return false;
                }
            }
            // Let QMK process the KC_BSPC keycode as usual outside of shift
            return true;
        }
        case DISP_UP:
            if (record->event.pressed) {
                // Increase brightness by ~10% (25 steps)
                if (backlight_brightness < 230) {
                    set_backlight_brightness(backlight_brightness + 25);
                } else {
                    set_backlight_brightness(255); // Max brightness
                }
            }
            return false;
        case DISP_DN:
            if (record->event.pressed) {
                // Decrease brightness by ~10% (25 steps)
                if (backlight_brightness > 25) {
                    set_backlight_brightness(backlight_brightness - 25);
                } else {
                    set_backlight_brightness(1); // Min brightness (not off)
                }
            }
            return false;
    }
    return true;
}

void keyboard_post_init_kb(void) {
    // Initialize the display
    init_display();
}

// Layer state change callback
layer_state_t layer_state_set_user(layer_state_t state) {
    // Check if entering arrow layer
    if (layer_state_cmp(state, _MAC_ARROW) && !layer_state_cmp(layer_state, _MAC_ARROW)) {
        // Entering arrow layer - initialize game manager
        game_manager_init();
    }
    // Check if exiting arrow layer
    else if (!layer_state_cmp(state, _MAC_ARROW) && layer_state_cmp(layer_state, _MAC_ARROW)) {
        // Exiting arrow layer - cleanup game manager
        game_manager_cleanup();
        // Defer the full display redraw by 50ms to allow next keystroke to be processed
        // This prevents blocking the matrix scan during expensive seasonal animation redraws
        deferred_display_update_pending = true;
        deferred_display_update_timer = timer_read32();
    }
    return state;
}

// Raw HID receive callback - handles data from computer
void raw_hid_receive(uint8_t *data, uint8_t length) {
    // Protocol:
    // Byte 0: Command ID
    //   0x01 = Volume update (Byte 1: volume 0-100)
    //   0x02 = Media text update (Bytes 1-31: null-terminated string)
    //   0x03 = Date/Time update (Bytes 1-7: year_low, year_high, month, day, hour, minute, second)
    //   0x04 = Weather control (Byte 1: weather state 0=sunny, 1-3=rain, 4-6=snow, 7=cloudy, 8=overcast)

    if (length < 2) return;  // Need at least 2 bytes

    uint8_t command = data[0];

    switch (command) {
        case 0x01:  // Volume update
            current_volume = data[1];
            // Clamp to 0-100 range
            if (current_volume > 100) {
                current_volume = 100;
            }

            // Update the permanent volume bar
            uint8_t layer = get_highest_layer(layer_state);
            uint8_t hue, sat, val;
            get_layer_color(layer, &hue, &sat, &val);
            draw_volume_bar(hue, sat, val);
            qp_flush(display);  // Flush QP info area only
            break;

        case 0x02:  // Media text update
            {
                // Copy media text (null-terminated string starting at byte 1)
                bool text_changed = false;
                if (data[1] == 0) {
                    // Empty string = no media playing
                    if (media_active) {
                        media_active = false;
                        current_media[0] = '\0';
                        text_changed = true;
                    }
                } else {
                    // Copy media text
                    char new_media[sizeof(current_media)];
                    strncpy(new_media, (char*)&data[1], sizeof(new_media) - 1);
                    new_media[sizeof(new_media) - 1] = '\0';

                    // Only update if text changed
                    if (strcmp(current_media, new_media) != 0) {
                        strncpy(current_media, new_media, sizeof(current_media) - 1);
                        current_media[sizeof(current_media) - 1] = '\0';
                        media_active = true;
                        text_changed = true;
                    }
                }

                // Only redraw if text actually changed
                if (text_changed) {
                    // Reset scroll state for new text
                    scroll_position = 0;
                    text_length = 0;
                    needs_scroll = false;
                    scroll_timer = timer_read32();
                    draw_media_text();
                    qp_flush(display);  // Flush QP info area only
                }
            }
            break;

        case 0x03:  // Date/Time update
            if (length >= 8) {
#ifdef HARDCODE_DATE_TIME
    #if IGNORE_HID_TIME_UPDATES
                // Hard-coded date mode is active - ignore HID time updates
                break;
    #endif
#endif
                // Extract date/time components
                current_year = data[1] | (data[2] << 8);  // 16-bit year
                uint8_t new_month = data[3];
                current_day = data[4];
                current_hour = data[5];
                current_minute = data[6];
                // data[7] is seconds, but we don't display it

                // Validate ranges
                if (new_month < 1 || new_month > 12) new_month = 1;
                if (current_day < 1 || current_day > 31) current_day = 1;
                if (current_hour > 23) current_hour = 0;
                if (current_minute > 59) current_minute = 0;

                // If month changed, update weather to match new season default
                if (new_month != current_month) {
                    current_month = new_month;
                    weather_transition_init(current_month);

                    // Reset animation flags for new season/weather
                    extern bool rain_initialized, rain_background_saved;
                    extern bool snowflake_initialized, snowflake_background_saved;
                    extern bool cloud_initialized, cloud_background_saved;

                    rain_initialized = false;
                    rain_background_saved = false;
                    snowflake_initialized = false;
                    snowflake_background_saved = false;
                    cloud_initialized = false;
                    cloud_background_saved = false;

                    // Force complete display redraw
                    current_display_layer = 255;
                    draw_seasonal_animation();
                    fb_flush(display);
                } else {
                    current_month = new_month;
                }

                time_received = true;
                last_uptime_update = timer_read32();

                // Note: We do NOT trigger deferred_display_update_pending here for time changes
                // The hour/day change detection in display_housekeeping_task() (display.c:638-653)
                // will handle updates intelligently by only resetting background_saved flags
                // rather than doing a full animation reset. This allows animated elements
                // (snowflakes, rain, clouds, etc.) to persist across hour changes.
            }
            break;

        case 0x04:  // Weather control
            if (length >= 2) {
                uint8_t weather = data[1];

                // Validate weather state (0=sunny, 1-3=rain, 4-6=snow, 7-8=cloudy)
                if (weather <= 8) {
                    weather_transition_set_target((weather_state_t)weather);

                    // Reset animation flags to force re-initialization
                    // This ensures particles are redrawn when weather changes
                    extern bool rain_initialized, rain_background_saved;
                    extern bool snowflake_initialized, snowflake_background_saved;
                    extern bool cloud_initialized, cloud_background_saved;

                    rain_initialized = false;
                    rain_background_saved = false;
                    snowflake_initialized = false;
                    snowflake_background_saved = false;
                    cloud_initialized = false;
                    cloud_background_saved = false;

                    // Force a complete display redraw to clear old weather particles
                    extern uint8_t current_display_layer;
                    extern painter_device_t display;
                    current_display_layer = 255;  // Force full redraw on next update

                    // Redraw the scene immediately with new weather
                    draw_seasonal_animation();

                    // Flush the entire scene to display to clear old particles
                    fb_flush(display);
                }
            }
            break;

        default:
            // Check if it's a game high score command
            // Doodle Jump: 0x10-0x13, Tetris: 0x14-0x17
            if (command >= 0x10 && command <= 0x17) {
                game_manager_hid_receive(data, length);
            }
            // Unknown command, ignore
            break;
    }
}

// Periodically check and update display based on active layer
void housekeeping_task_user(void) {
    // Handle game manager when on arrow layer
    if (game_manager_housekeeping(display)) {
        return;  // Game manager handled the update, skip normal display updates
    }

    // Delegate all display-related housekeeping to the display module
    display_housekeeping_task();
}
