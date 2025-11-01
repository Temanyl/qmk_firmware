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
#include "framebuffer.h"
#include "display.h"
#include "scenes.h"

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
    _MAC_NUM
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
    [_MAC_CODE] = LAYOUT_ortho_3x10_6(
        KC_UNDS, KC_LT,   KC_GT,   KC_LCBR, KC_RCBR,        KC_PIPE,  KC_AT,   KC_BSLS, KC_GRAVE, KC_ENT,
        KC_EXLM, KC_MINS, KC_EQL,  KC_LPRN, KC_RPRN,        KC_AMPR,  KC_QUOT, KC_DOWN, KC_DQUO, KC_NO,
        KC_CIRC, KC_PLUS, KC_ASTR, KC_LBRC, KC_RBRC,        KC_TILDE, KC_DLR,  KC_PERC, KC_HASH, RSFT_T(KC_BSLS),
                            KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,        KC_BSPC,  TO(_MAC_NAV), KC_NO
    ),
    [_MAC_NAV] = LAYOUT_ortho_3x10_6(
        KC_ESC,  MS_BTN1, MS_UP, MS_BTN2, KC_NO,          KC_VOLU, KC_PGUP, KC_UP,    KC_PGDN, KC_ENT,
        KC_NO,   KC_LCTL, KC_LALT, KC_LGUI, KC_MPLY,        KC_MUTE, KC_LEFT, KC_DOWN,  KC_RGHT, KC_NO,
        KC_NO,   MS_LEFT, MS_DOWN, MS_RGHT, KC_NO,          KC_VOLD, KC_NO,   KC_NO,    KC_NO,   KC_NO,
                          KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,          KC_BSPC, KC_NO, TO(_MAC_CODE)
    ),
    [_MAC_NUM] = LAYOUT_ortho_3x10_6(
         KC_F1,   KC_F2, KC_F3,   KC_F4,   KC_F5,          KC_DOT,   KC_7,   KC_8,  KC_9,   KC_ENT,
         KC_F6,   KC_F7, KC_F8,   KC_F9,   KC_F10,         KC_COMMA, KC_4,   KC_5,  KC_6,   DISP_UP,
         KC_F11,  KC_F12,KC_LCTL, KC_LALT, KC_LGUI,        KC_0,     KC_1,   KC_2,  KC_3,   DISP_DN,
                         KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,           KC_BSPC, TO(_MAC_NAV), KC_NO
    ),
    [_MAC_COLEMAK_DH] = LAYOUT_ortho_3x10_6(
         TD(TD_Q_ESC_EMOJI_RESET), KC_W,  KC_F,    KC_P,  KC_B,           KC_J,  KC_L,          KC_U,              KC_Y,           LT(0,KC_SCLN),
         KC_A, LCTL_T(KC_R), LALT_T(KC_S), LGUI_T(KC_T),  KC_G,           KC_M,  LGUI_T(KC_N),  LALT_T(KC_E),      LCTL_T(KC_I),   KC_O,
         KC_Z, KC_X,         KC_C,         KC_D,          KC_V,           KC_K,  KC_H,          KC_COMMA,          KC_DOT,         KC_SLSH,
                MEH_T(KC_TAB), KC_LSFT, KC_SPC,      KC_BSPC, TD(TD_LAYER_NAV_NUM), OSL(_MAC_CODE)
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

// Raw HID receive callback - handles data from computer
void raw_hid_receive(uint8_t *data, uint8_t length) {
    // Protocol:
    // Byte 0: Command ID
    //   0x01 = Volume update (Byte 1: volume 0-100)
    //   0x02 = Media text update (Bytes 1-31: null-terminated string)
    //   0x03 = Date/Time update (Bytes 1-7: year_low, year_high, month, day, hour, minute, second)

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
                // Extract date/time components
                current_year = data[1] | (data[2] << 8);  // 16-bit year
                current_month = data[3];
                current_day = data[4];
                current_hour = data[5];
                current_minute = data[6];
                // data[7] is seconds, but we don't display it

                // Validate ranges
                if (current_month < 1 || current_month > 12) current_month = 1;
                if (current_day < 1 || current_day > 31) current_day = 1;
                if (current_hour > 23) current_hour = 0;
                if (current_minute > 59) current_minute = 0;

                time_received = true;
                last_uptime_update = timer_read32();

                // Update tracking variables for hour/day change detection
                last_hour = current_hour;
                last_day = current_day;

                // Force full redraw of scene (season and sun/moon position depend on time)
                current_display_layer = 255;  // Invalidate layer cache
                update_display_for_layer();
            }
            break;

        default:
            // Unknown command, ignore
            break;
    }
}

// Periodically check and update display based on active layer
void housekeeping_task_user(void) {
    update_display_for_layer();

    uint32_t current_time = timer_read32();
    bool needs_flush = false;

    // Check if hour or day changed (for seasonal animation updates)
    bool hour_changed = (current_hour != last_hour);
    bool day_changed = (current_day != last_day);

    // Update date/time display once per minute (or when time is received)
    if (time_received && (current_time - last_uptime_update >= 60000)) {
        last_uptime_update = current_time;
        // Increment minute (host will send updated time periodically)
        current_minute++;
        if (current_minute >= 60) {
            current_minute = 0;
            current_hour++;
            hour_changed = true;
            if (current_hour >= 24) {
                current_hour = 0;
                current_day++;
                day_changed = true;

                // Handle day rollover with proper month boundaries
                uint8_t days_in_month = 31; // Default
                if (current_month == 2) {
                    // February: check for leap year
                    bool is_leap = (current_year % 4 == 0 && current_year % 100 != 0) || (current_year % 400 == 0);
                    days_in_month = is_leap ? 29 : 28;
                } else if (current_month == 4 || current_month == 6 || current_month == 9 || current_month == 11) {
                    days_in_month = 30; // April, June, September, November
                }

                if (current_day > days_in_month) {
                    current_day = 1;
                    current_month++;
                    if (current_month > 12) {
                        current_month = 1;
                        current_year++;
                    }
                    // Force full redraw when month changes (season may change)
                    current_display_layer = 255;
                    update_display_for_layer();
                    needs_flush = false; // Already flushed by update_display_for_layer
                }
            }
        }
        if (needs_flush) {
            draw_date_time();
        }
        needs_flush = true;
    }

    // Redraw seasonal animation when hour or day changes (sun/moon position, moon phase)
    if (hour_changed || day_changed) {
        // Reset background flags to force re-saving with updated scene
        // (sun/moon positions change, so background buffer must be updated)
        rain_background_saved = false;
        ghost_background_saved = false;
        smoke_background_saved = false;

        draw_seasonal_animation();
        last_hour = current_hour;
        last_day = current_day;
        needs_flush = true;
    }

    // Handle brightness display timeout
    if (brightness_display_active) {
        if (current_time - brightness_display_timer >= BRIGHTNESS_DISPLAY_TIMEOUT) {
            // Timeout expired, hide brightness indicator
            brightness_display_active = false;
            // Force a full redraw by invalidating the current layer
            current_display_layer = 255;
            update_display_for_layer();
            needs_flush = true;
        }
    }

    // Handle media text scrolling (character-based tick scrolling)
    if (needs_scroll && media_active) {
        uint32_t elapsed = current_time - scroll_timer;

        // Start scrolling after initial pause
        if (elapsed >= SCROLL_PAUSE_START) {
            // Calculate how many ticks should have occurred
            uint32_t scroll_ticks = (elapsed - SCROLL_PAUSE_START) / SCROLL_SPEED;
            uint8_t target_position = scroll_ticks % (text_length + 3); // +3 for spacing

            // Only redraw if scroll position actually changed
            if (target_position != scroll_position) {
                scroll_position = target_position;
                draw_media_text();
                needs_flush = true;
            }
        }
    }

    // Handle rain animation (during fall season)
    // Note: animate_raindrops() handles its own region-based flushing
    if (rain_initialized && rain_background_saved) {
        uint8_t season = get_season(current_month);
        if (season == 3) { // Fall season
            if (current_time - rain_animation_timer >= RAIN_ANIMATION_SPEED) {
                rain_animation_timer = current_time;
                animate_raindrops();
                // No needs_flush = true here - raindrops flush their own regions
            }
        }
    }

    // Region-based animation with smart overlap detection
    // Store old positions before updating
    cloud_t old_clouds[NUM_CLOUDS];
    ghost_t old_ghosts[NUM_GHOSTS];
    bool clouds_updated = false;
    bool ghosts_updated = false;
    uint8_t season = get_season(current_month);
    uint8_t num_active_clouds = (season == 3) ? 5 : 3;

    // Update cloud positions if timer elapsed
    if (cloud_initialized && cloud_background_saved) {
        if (current_time - cloud_animation_timer >= CLOUD_ANIMATION_SPEED) {
            // Store old positions
            for (uint8_t i = 0; i < num_active_clouds; i++) {
                old_clouds[i] = clouds[i];
            }
            cloud_animation_timer = current_time;
            animate_clouds();  // Updates positions only
            clouds_updated = true;
        }
    }

    // Update ghost positions if timer elapsed (during Halloween event)
    bool ghosts_active = ghost_initialized && ghost_background_saved && is_halloween_event();
    if (ghosts_active) {
        if (current_time - ghost_animation_timer >= GHOST_ANIMATION_SPEED) {
            // Store old positions
            for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
                old_ghosts[i] = ghosts[i];
            }
            ghost_animation_timer = current_time;
            animate_ghosts();  // Updates positions only
            ghosts_updated = true;
        }
    }

    // Smart region-based rendering with z-ordering
    if (clouds_updated || ghosts_updated) {
        // Track dirty bounds for efficient flushing
        int16_t dirty_x1 = 134, dirty_y1 = 121;
        int16_t dirty_x2 = 0, dirty_y2 = 12;

        // Helper to expand dirty region
        #define EXPAND_DIRTY(x1, y1, x2, y2) do { \
            if ((x1) < dirty_x1) dirty_x1 = (x1); \
            if ((y1) < dirty_y1) dirty_y1 = (y1); \
            if ((x2) > dirty_x2) dirty_x2 = (x2); \
            if ((y2) > dirty_y2) dirty_y2 = (y2); \
        } while(0)

        // Helper to check rectangle overlap
        #define RECTS_OVERLAP(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2) \
            (!((ax2) < (bx1) || (ax1) > (bx2) || (ay2) < (by1) || (ay1) > (by2)))

        // Track which objects need redrawing
        bool redraw_clouds[NUM_CLOUDS] = {false};
        bool redraw_ghosts[NUM_GHOSTS] = {false};

        // Process cloud updates
        if (clouds_updated) {
            for (uint8_t i = 0; i < num_active_clouds; i++) {
                // Cloud bounds: x-16 to x+18, y-11 to y+10 (conservative)
                int16_t old_x1 = old_clouds[i].x - 16;
                int16_t old_y1 = old_clouds[i].y - 11;
                int16_t old_x2 = old_clouds[i].x + 18;
                int16_t old_y2 = old_clouds[i].y + 10;

                // Restore old position
                fb_restore_from_background(old_x1, old_y1, old_x2, old_y2);
                EXPAND_DIRTY(old_x1, old_y1, old_x2, old_y2);

                // Mark this cloud for redraw
                redraw_clouds[i] = true;

                // Check if any ghosts overlap old cloud position
                if (ghosts_active) {
                    for (uint8_t j = 0; j < NUM_GHOSTS; j++) {
                        int16_t gx1 = ghosts[j].x - 8;
                        int16_t gy1 = ghosts[j].y - 8;
                        int16_t gx2 = ghosts[j].x + 8;
                        int16_t gy2 = ghosts[j].y + 14;
                        if (RECTS_OVERLAP(old_x1, old_y1, old_x2, old_y2, gx1, gy1, gx2, gy2)) {
                            redraw_ghosts[j] = true;
                        }
                    }
                }
            }
        }

        // Process ghost updates
        if (ghosts_updated) {
            for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
                // Ghost bounds: x-8 to x+8, y-8 to y+14
                int16_t old_x1 = old_ghosts[i].x - 8;
                int16_t old_y1 = old_ghosts[i].y - 8;
                int16_t old_x2 = old_ghosts[i].x + 8;
                int16_t old_y2 = old_ghosts[i].y + 14;

                // Restore old position
                fb_restore_from_background(old_x1, old_y1, old_x2, old_y2);
                EXPAND_DIRTY(old_x1, old_y1, old_x2, old_y2);

                // Mark this ghost for redraw
                redraw_ghosts[i] = true;

                // Check if any clouds overlap old ghost position
                if (cloud_initialized && cloud_background_saved) {
                    for (uint8_t j = 0; j < num_active_clouds; j++) {
                        int16_t cx1 = clouds[j].x - 16;
                        int16_t cy1 = clouds[j].y - 11;
                        int16_t cx2 = clouds[j].x + 18;
                        int16_t cy2 = clouds[j].y + 10;
                        if (RECTS_OVERLAP(old_x1, old_y1, old_x2, old_y2, cx1, cy1, cx2, cy2)) {
                            redraw_clouds[j] = true;
                        }
                    }
                }
            }
        }

        // Redraw affected objects in z-order (clouds first, then ghosts)
        // Draw clouds (background layer)
        if (cloud_initialized && cloud_background_saved) {
            for (uint8_t i = 0; i < num_active_clouds; i++) {
                if (redraw_clouds[i]) {
                    int16_t x = clouds[i].x;
                    int16_t y = clouds[i].y;
                    if (x >= -30 && x <= 165) {
                        // Expand dirty region for new position
                        EXPAND_DIRTY(x - 16, y - 11, x + 18, y + 10);

                        if (season == 3) {
                            // Fall: darker rain clouds
                            fb_circle_hsv(x, y, 9, 0, 0, 120, true);
                            fb_circle_hsv(x + 10, y + 2, 7, 0, 0, 120, true);
                            fb_circle_hsv(x - 8, y + 2, 7, 0, 0, 120, true);
                            fb_circle_hsv(x + 5, y - 4, 6, 0, 0, 110, true);
                        } else {
                            // Winter: lighter clouds
                            draw_cloud(x, y);
                        }
                    }
                }
            }
        }

        // Draw ghosts (foreground layer)
        if (ghosts_active) {
            for (uint8_t i = 0; i < NUM_GHOSTS; i++) {
                if (redraw_ghosts[i]) {
                    // Expand dirty region for new position
                    EXPAND_DIRTY(ghosts[i].x - 8, ghosts[i].y - 8, ghosts[i].x + 8, ghosts[i].y + 14);
                    draw_ghost(ghosts[i].x, ghosts[i].y);
                }
            }
        }

        // Flush only the dirty region (much smaller than full area)
        if (dirty_x2 >= dirty_x1 && dirty_y2 >= dirty_y1) {
            fb_flush_region(display, dirty_x1, dirty_y1, dirty_x2, dirty_y2);
        }

        #undef EXPAND_DIRTY
        #undef RECTS_OVERLAP
    }

    // Handle smoke animation (all seasons except summer)
    // Note: animate_smoke() handles its own region-based flushing
    if (smoke_initialized && smoke_background_saved) {
        uint8_t season = get_season(current_month);
        if (season != 2) { // Not summer
            if (current_time - smoke_animation_timer >= SMOKE_ANIMATION_SPEED) {
                smoke_animation_timer = current_time;
                animate_smoke();
                // No needs_flush = true here - smoke flushes its own regions
            }
        }
    }

    // Handle Santa sleigh animation (on Christmas Day Dec 25 and after)
    if (is_christmas_season() && current_day >= 25) {
        if (current_time - santa_animation_timer >= SANTA_ANIMATION_SPEED) {
            santa_animation_timer = current_time;
            update_santa_animation();
            // Redraw seasonal animation to show updated Santa position
            draw_seasonal_animation();
            needs_flush = true;
        }
    } else {
        // Reset Santa state when not Christmas Day
        if (santa_initialized) {
            santa_initialized = false;
        }
    }

    // Single flush at the end to batch all updates
    if (needs_flush) {
        fb_flush(display);
    }
}
