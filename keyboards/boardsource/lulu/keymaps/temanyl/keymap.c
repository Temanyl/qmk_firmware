// Copyright 2022 Cole Smith <cole@boadsource.xyz>
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

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

q
typedef struct {
    bool is_press_action;
    td_state_t state;
} td_tap_t;


// Declare the functions to be used with your tap dance key(s)

// Function associated with all tap dances
td_state_t cur_dance(qk_tap_dance_state_t *state);

// Functions associated with individual tap dances
void nav_num_finished(qk_tap_dance_state_t *state, void *user_data);
void nav_num_reset(qk_tap_dance_state_t *state, void *user_data);
void layer_default_shift_finished(qk_tap_dance_state_t *state, void *user_data);
void layer_default_shift_reset(qk_tap_dance_state_t *state, void *user_data);
void osl_code_finished(qk_tap_dance_state_t *state, void *user_data);
void osl_code_reset(qk_tap_dance_state_t *state, void *user_data);


// #############################################################

void td_q_esc_emoji_reset (qk_tap_dance_state_t *state, void *user_data) {
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
qk_tap_dance_action_t tap_dance_actions[] = {
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

    }
    return true;
}

// Layer Names
enum layer_names {
    _MAC_DEFAULT,
    _MAC_CODE,
    _MAC_NAV,
    _MAC_NUM
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

 [_MAC_DEFAULT] = LAYOUT(
  KC_ESC,   KC_1,   KC_2,    KC_3,    KC_4,    KC_5,                              KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_GRV,
  KC_TAB,   TD(TD_Q_ESC_EMOJI_RESET), KC_W, KC_E,    KC_R,    KC_T,               KC_Y, KC_U,  KC_I,      KC_O,   KC_P,    KC_MINS,
  KC_LCTL,  KC_A, LCTL_T(KC_S), LALT_T(KC_D),    LGUI_T(KC_F), KC_G,              KC_H, LGUI_T(KC_J), LALT_T(KC_K),    LCTL_T(KC_L), LT(0,KC_SCLN), KC_ENT,
  KC_LSFT,  KC_Z,   KC_X,    KC_C,    KC_V,    KC_B, KC_MPLY,           KC_MUTE,  KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,  KC_RSFT,
                          KC_LALT,     MEH_T(KC_TAB), KC_LSFT, KC_SPC,        KC_BSPC, TD(TD_LAYER_NAV_NUM), OSL(_MAC_CODE), KC_RGUI
),

[_MAC_CODE] = LAYOUT(
  _______, _______, _______, _______, _______, _______,                   _______, _______, _______,_______, _______, _______,
  KC_F1,   KC_UNDS, KC_LT,   KC_GT,   KC_LCBR, KC_RCBR,        KC_PIPE,  KC_AT,   KC_BSLS, KC_GRAVE, KC_DEL,  KC_F12,
  KC_GRV, KC_EXLM, KC_MINS, KC_EQL,  KC_LPRN, KC_RPRN,        KC_AMPR,  KC_QUOT, KC_DOWN, KC_DQUO, KC_ENT, KC_TILD,
  _______, KC_CIRC, KC_PLUS, KC_ASTR, KC_LBRC, KC_RBRC, _______, _______, KC_TILDE, KC_DLR,  KC_PERC, KC_HASH, RSFT_T(KC_BSLS), KC_PIPE,
                             _______, KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,        KC_BSPC,  TO(_MAC_NAV), KC_NO, _______
),

[_MAC_NAV] = LAYOUT(
  RGB_MOD, RGB_RMOD,RGB_SAI, RGB_SAD, RGB_VAI, RGB_VAD,                     BL_TOGG, RGB_TOG, _______, _______, _______, _______,
  KC_GRV,  KC_NO,   KC_BTN1, KC_MS_U, KC_BTN2, KC_MNXT,        KC_VOLU, KC_PGUP, KC_UP,    KC_PGDN, KC_BSPC,    _______,
  KC_F1,   KC_NO,   KC_LCTL, KC_LALT, KC_LGUI, KC_MPLY,        KC_MUTE, KC_LEFT, KC_DOWN,  KC_RGHT, KC_ENT, XXXXXXX,
  KC_F7,   KC_NO, KC_MS_L, KC_MS_D, KC_MS_R, KC_MPRV,   RGB_TOG, _______,  KC_VOLD, KC_NO,   KC_NO,    KC_NO,   KC_NO, KC_BSLS,
                             _______, KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,          KC_BSPC, KC_NO, TO(_MAC_CODE), _______
),

  [_MAC_NUM] = LAYOUT(
  XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,                   XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
  XXXXXXX, KC_F1,   KC_F2, KC_F3,   KC_F4,   KC_F5,          KC_DOT,   KC_7,   KC_8,  KC_9, KC_NO, XXXXXXX,
  XXXXXXX,KC_F6,   KC_F7, KC_F8,   KC_F9,   KC_F10,         KC_COMMA, KC_4,   KC_5,  KC_6, KC_ENT, XXXXXXX,
  XXXXXXX,  KC_F11,  KC_F12,KC_LCTL, KC_LALT, KC_LGUI, XXXXXXX, XXXXXXX, KC_0,     KC_1,   KC_2,  KC_3, KC_NO, XXXXXXX,
                             _______, KC_TAB, TD(TD_LAYER_DEFAULT_SHIFT), KC_SPC,           KC_BSPC, KC_NO, KC_NO, _______
  )
};

// tap dances again
// Determine the current tap dance state
td_state_t cur_dance(qk_tap_dance_state_t *state) {
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
void nav_num_finished(qk_tap_dance_state_t *state, void *user_data) {
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

void nav_num_reset(qk_tap_dance_state_t *state, void *user_data) {
    // If the key was held down and now is released then switch off the layer
    if (ql_tap_state.state == TD_SINGLE_HOLD) {
        layer_off(_MAC_NUM);
    }
    ql_tap_state.state = TD_NONE;
}

// Functions that control what our tap dance key does
void layer_default_shift_finished(qk_tap_dance_state_t *state, void *user_data) {
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

void layer_default_shift_reset(qk_tap_dance_state_t *state, void *user_data) {
    // If the key was held down and now is released then switch off the layer
    if (ql_tap_state.state == TD_SINGLE_HOLD) {
         unregister_code(KC_LSFT);
    }
    ql_tap_state.state = TD_NONE;
}

void osl_code_finished(qk_tap_dance_state_t *state, void *user_data) {
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

void osl_code_reset(qk_tap_dance_state_t *state, void *user_data) {
    ql_tap_state.state = cur_dance(state);
    // If the key was held down and now is released then switch off the layer
    if (ql_tap_state.state == TD_SINGLE_TAP) {
        clear_oneshot_layer_state(ONESHOT_PRESSED);
    } else {
        layer_clear();
    }
}

