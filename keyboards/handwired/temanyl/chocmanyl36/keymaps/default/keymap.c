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
#include <qp.h>
#include "draw_logo.h"

// Display configuration
painter_device_t display;
static uint8_t current_display_layer = 255; // Track currently displayed layer
static uint8_t backlight_brightness = 26;   // Current brightness level (10% default)

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

// Function to set background color based on layer
void set_layer_background(uint8_t layer) {
    // Only update if the layer actually changed
    if (layer == current_display_layer) {
        return;
    }
    current_display_layer = layer;

    switch (layer) {
        case _MAC_COLEMAK_DH:
            // White background for base layer (HSV: no hue, no saturation, full brightness)
            qp_rect(display, 0, 0, 134, 239, 0, 0, 255, true);
            break;
        case _MAC_NAV:
            // Pastel green for navigation layer (HSV: green hue ~85, low saturation for pastel)
            qp_rect(display, 0, 0, 134, 239, 85, 150, 230, true);
            break;
        case _MAC_CODE:
            // Pastel red for symbols layer (HSV: red hue 0, low saturation for pastel)
            qp_rect(display, 0, 0, 134, 239, 0, 150, 255, true);
            break;
        case _MAC_NUM:
            // Pastel blue for numbers layer (HSV: blue hue ~170, low saturation for pastel)
            qp_rect(display, 0, 0, 134, 239, 170, 70, 255, true);
            break;
    }

    // Redraw the logo after changing background
    draw_amboss_logo_teal(display, 7, 60);
    qp_flush(display);
}

// Update display based on current layer state
void update_display_for_layer(void) {
    set_layer_background(get_highest_layer(layer_state));
}

// Set backlight brightness via PWM
void set_backlight_brightness(uint8_t brightness) {
    backlight_brightness = brightness;
    // Update PWM duty cycle (channel A compare value)
    *(volatile uint32_t*)(0x40050028 + 0x0C) = brightness;
}

// Initialize the ST7789 display
static void init_display(void) {
    // CRITICAL: Enable display power on GP22 (LILYGO board power enable)
    setPinOutput(GP22);
    writePinHigh(GP22);

    // Small delay to let power stabilize
    wait_ms(50);

    // Create display: 135x240 portrait mode (rotated 90°)
    // Using SPI mode 3 and slower divisor (16) for reliable communication
    display = qp_st7789_make_spi_device(135, 240, GP5, GP1, GP0, 16, 3);

    // LILYGO T-Display RP2040: Portrait mode with proper offsets
    qp_set_viewport_offsets(display, 52, 40);

    // Initialize with 180° rotation (controller mounted upside down)
    if (!qp_init(display, QP_ROTATION_180)) {
        return;  // Initialization failed
    }

    // Power on display
    if (!qp_power(display, true)) {
        return;  // Power on failed4
    }

    // Wait for display to stabilize
    wait_ms(50);

    // Dim backlight on GP4 using PWM (50% brightness)
    // First unreset the PWM peripheral (RESETS_BASE=0x4000c000, bit 14 for PWM)
    *(volatile uint32_t*)(0x4000c000) &= ~(1 << 14);  // Clear PWM reset bit

    // Wait for PWM reset to complete
    while (!(*(volatile uint32_t*)(0x4000c008) & (1 << 14))) {
        wait_ms(1);
    }

    // Set GPIO4 to PWM function
    *(volatile uint32_t*)(0x40014024) = 4;

    /*
      - 26 = 10%
      - 51 = 20%
      - 77 = 30%
      - 102 = 40%
      - 128 = 50%
      - 191 = 75%
      - 255 = 100%
    */
    // Configure PWM slice 2 (GP4 = PWM2_A)
    *(volatile uint32_t*)(0x40050028 + 0x04) = 16 << 4;  // DIV: no division
    *(volatile uint32_t*)(0x40050028 + 0x10) = 255;      // TOP: wrap at 255
    *(volatile uint32_t*)(0x40050028 + 0x0C) = 102;      // CC: channel A = 128 (50%)
    *(volatile uint32_t*)(0x40050028 + 0x00) = 0x01;     // CSR: enable

    // Fill screen with white background (135x240 portrait)
    qp_rect(display, 0, 0, 134, 239, 0, 0, 255, true);
    wait_ms(50);

    // Draw the Amboss logo in teal using line-by-line rendering
    // Logo is 120x120, centered horizontally (135-120)/2 = 7.5, vertically centered at 60
    draw_amboss_logo_teal(display, 7, 60);

    // Force flush to ensure everything is drawn
    qp_flush(display);
}

void keyboard_post_init_kb(void) {
    // Initialize the display
    init_display();
}


// Tap Dance declarations
enum {
    TD_Q_ESC_EMOJI_RESET,
    TD_ESC_WINDOWS_EMOJI,
    TD_LAYER_NAV_NUM,
    TD_LAYER_DEFAULT_SHIFT,
};

// Define a type for as many tap dance states as you need
typedef enum {
    TD_NONE,
    TD_UNKNOWN,
    TD_SINGLE_TAP,
    TD_SINGLE_HOLD,
    TD_DOUBLE_TAP,
    TD_OSL_CODE
} td_state_t;


typedef struct {
    bool is_press_action;
    td_state_t state;
} td_tap_t;


// Declare the functions to be used with your tap dance key(s)

// Function associated with all tap dances
td_state_t cur_dance(tap_dance_state_t *state);

// Functions associated with individual tap dances
void nav_num_finished(tap_dance_state_t *state, void *user_data);
void nav_num_reset(tap_dance_state_t *state, void *user_data);
void layer_default_shift_finished(tap_dance_state_t *state, void *user_data);
void layer_default_shift_reset(tap_dance_state_t *state, void *user_data);
void osl_code_finished(tap_dance_state_t *state, void *user_data);
void osl_code_reset(tap_dance_state_t *state, void *user_data);


// #############################################################

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
        case TD(TD_Q_ESC_EMOJI_RESET) :
        case TD(TD_ESC_WINDOWS_EMOJI) :
        case LGUI_T(KC_SPC) :
        case LT(1, KC_TAB) :
        case LT(2, KC_ENT) :
            return 200;
        case TD(TD_LAYER_DEFAULT_SHIFT):
            return 180;
        case LT(0,KC_SCLN) :
            return 155;
    default:
      return TAPPING_TERM;
  }
};

bool send_hold_code(uint16_t keycode, keyrecord_t *record) {
        if (!record->tap.count && record->event.pressed) {
            tap_code16(G(keycode)); // Intercept hold function to send Ctrl-X
            return false;
        }
        return true;
}

// Initialize variable holding the binary
// representation of active modifiers.
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

// Periodically check and update display based on active layer
void housekeeping_task_user(void) {
    update_display_for_layer();
}

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

// OLED
//#ifdef OLED_ENABLE
// Draw to OLED
//bool oled_task_user() {
//
//    // Layer text
//    oled_set_cursor(0, 1);
//    switch (get_highest_layer(layer_state)) {
//        case _MAC_CODE :
//            oled_write_P(PSTR("MAC"), false);
//            oled_set_cursor(0, 2);
//            oled_write_P(PSTR("SYM"), false);
//            break;
//        case _MAC_NUM :
//            oled_write_P(PSTR("MAC"), false);
//            oled_set_cursor(0, 2);
//            oled_write_P(PSTR("NUM"), false);
//            break;
//        case _MAC_NAV :
//            oled_write_P(PSTR("MAC"), false);
//            oled_set_cursor(0, 2);
//            oled_write_P(PSTR("NAV"), false);
//            break;
//        case _MAC_COLEMAK_DH :
//            oled_write_P(PSTR("MAC"), false);
//            oled_set_cursor(0, 2);
//            oled_write_P(PSTR("COLE"), false);
//            break;
//    }
//
//    // Caps lock text
//    led_t led_state = host_keyboard_led_state();
//    oled_set_cursor(0, 3);
//    oled_write_P(led_state.caps_lock ? PSTR("CAPS") : PSTR(""), false);
//
//    return false;
//}
//#endif
