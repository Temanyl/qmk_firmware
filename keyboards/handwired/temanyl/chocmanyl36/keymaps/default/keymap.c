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
static uint8_t backlight_brightness = 102;  // Current brightness level (40% default)
static uint32_t last_uptime_update = 0;     // Track last uptime display update

// Volume indicator state (permanent bar at bottom)
static uint8_t current_volume = 0;          // Current volume level (0-100)

// Brightness indicator state (temporary overlay)
static uint8_t last_brightness_value = 102; // Track last brightness for change detection
static uint32_t brightness_display_timer = 0; // Timer for brightness display timeout
static bool brightness_display_active = false; // Whether brightness indicator is shown
#define BRIGHTNESS_DISPLAY_TIMEOUT 3000      // Show brightness for 3 seconds

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

// Forward declarations
void draw_volume_bar(uint8_t hue, uint8_t sat, uint8_t val);
void draw_uptime_timer(void);
void get_layer_color(uint8_t layer, uint8_t *hue, uint8_t *sat, uint8_t *val);
void draw_brightness_indicator(void);

// Helper function to draw a single digit using 7-segment style
void draw_digit(uint16_t x, uint16_t y, uint8_t digit, uint8_t hue, uint8_t sat, uint8_t val) {
    // Each segment is a rectangle
    // Segment positions (7-segment display):
    //  AAA
    // F   B
    //  GGG
    // E   C
    //  DDD

    bool seg_a = false, seg_b = false, seg_c = false, seg_d = false;
    bool seg_e = false, seg_f = false, seg_g = false;

    switch(digit) {
        case 0: seg_a = seg_b = seg_c = seg_d = seg_e = seg_f = true; break;
        case 1: seg_b = seg_c = true; break;
        case 2: seg_a = seg_b = seg_d = seg_e = seg_g = true; break;
        case 3: seg_a = seg_b = seg_c = seg_d = seg_g = true; break;
        case 4: seg_b = seg_c = seg_f = seg_g = true; break;
        case 5: seg_a = seg_c = seg_d = seg_f = seg_g = true; break;
        case 6: seg_a = seg_c = seg_d = seg_e = seg_f = seg_g = true; break;
        case 7: seg_a = seg_b = seg_c = true; break;
        case 8: seg_a = seg_b = seg_c = seg_d = seg_e = seg_f = seg_g = true; break;
        case 9: seg_a = seg_b = seg_c = seg_d = seg_f = seg_g = true; break;
    }

    // Draw segments (larger size, ~14x20 digit)
    if (seg_a) qp_rect(display, x + 2, y, x + 11, y + 2, hue, sat, val, true);          // top
    if (seg_b) qp_rect(display, x + 11, y + 2, x + 13, y + 9, hue, sat, val, true);     // top right
    if (seg_c) qp_rect(display, x + 11, y + 11, x + 13, y + 18, hue, sat, val, true);   // bottom right
    if (seg_d) qp_rect(display, x + 2, y + 18, x + 11, y + 20, hue, sat, val, true);    // bottom
    if (seg_e) qp_rect(display, x, y + 11, x + 2, y + 18, hue, sat, val, true);         // bottom left
    if (seg_f) qp_rect(display, x, y + 2, x + 2, y + 9, hue, sat, val, true);           // top left
    if (seg_g) qp_rect(display, x + 2, y + 9, x + 11, y + 11, hue, sat, val, true);     // middle
}

// Get color for a given layer
void get_layer_color(uint8_t layer, uint8_t *hue, uint8_t *sat, uint8_t *val) {
    switch (layer) {
        case _MAC_COLEMAK_DH:
            *hue = 128; *sat = 255; *val = 255;  // Teal
            break;
        case _MAC_NAV:
            *hue = 85; *sat = 255; *val = 255;   // Green
            break;
        case _MAC_CODE:
            *hue = 0; *sat = 255; *val = 255;    // Red
            break;
        case _MAC_NUM:
            *hue = 43; *sat = 255; *val = 255;   // Yellow
            break;
        default:
            *hue = 128; *sat = 255; *val = 255;  // Default to teal
            break;
    }
}

// Draw uptime timer at top of display
void draw_uptime_timer(void) {
    // Get uptime in milliseconds
    uint32_t uptime_ms = timer_read32();
    uint32_t uptime_seconds = uptime_ms / 1000;

    // Calculate days, hours, minutes, seconds
    uint16_t days = uptime_seconds / 86400;
    uint8_t hours = (uptime_seconds % 86400) / 3600;
    uint8_t minutes = (uptime_seconds % 3600) / 60;
    uint8_t seconds = uptime_seconds % 60;

    // Get current layer color for the timer
    uint8_t layer = get_highest_layer(layer_state);
    uint8_t hue, sat, val;
    get_layer_color(layer, &hue, &sat, &val);

    // Clear timer area above brightness bar (white background)
    qp_rect(display, 0, 165, 134, 225, 0, 0, 255, true);

    // Draw "Day" label and number centered (much larger, more readable)
    uint16_t day_x = (135 - 65) / 2;  // Center position for wider text
    uint16_t day_y = 168;  // Start position above brightness bar

    // Draw "Day" text - much larger (~8x11 pixels per letter, 2px thick)
    // D
    qp_rect(display, day_x, day_y, day_x + 2, day_y + 10, hue, sat, val, true);      // left vertical (thick)
    qp_rect(display, day_x + 2, day_y, day_x + 7, day_y + 2, hue, sat, val, true);   // top horizontal (thick)
    qp_rect(display, day_x + 6, day_y + 2, day_x + 8, day_y + 8, hue, sat, val, true);  // right vertical (thick)
    qp_rect(display, day_x + 2, day_y + 8, day_x + 7, day_y + 10, hue, sat, val, true); // bottom horizontal (thick)

    // a
    qp_rect(display, day_x + 11, day_y + 4, day_x + 16, day_y + 6, hue, sat, val, true);  // top (thick)
    qp_rect(display, day_x + 15, day_y + 4, day_x + 17, day_y + 10, hue, sat, val, true); // right vertical (thick)
    qp_rect(display, day_x + 11, day_y + 8, day_x + 16, day_y + 10, hue, sat, val, true); // bottom (thick)
    qp_rect(display, day_x + 10, day_y + 6, day_x + 12, day_y + 10, hue, sat, val, true);  // left vertical short (thick)

    // y
    qp_rect(display, day_x + 21, day_y + 4, day_x + 23, day_y + 11, hue, sat, val, true);  // left with descender (thick)
    qp_rect(display, day_x + 26, day_y + 4, day_x + 28, day_y + 9, hue, sat, val, true);  // right vertical (thick)
    qp_rect(display, day_x + 23, day_y + 8, day_x + 26, day_y + 10, hue, sat, val, true); // bottom connection (thick)

    // Space before number
    uint16_t num_x = day_x + 34;

    // Draw day number using same-size 7-segment digits
    if (days >= 10) {
        // Draw tens digit
        uint8_t tens = days / 10;
        draw_digit(num_x, day_y - 1, tens, hue, sat, val);
        num_x += 16;
    }
    // Draw ones digit
    draw_digit(num_x, day_y - 1, days % 10, hue, sat, val);

    // Main timer position (below day label)
    uint16_t timer_y = 193;

    // Draw time in HH:MM:SS format centered
    // Each digit is ~14px wide, with spacing: total ~100px wide
    uint16_t start_x = (135 - 100) / 2;

    // Draw hours (2 digits)
    draw_digit(start_x, timer_y, hours / 10, hue, sat, val);
    draw_digit(start_x + 16, timer_y, hours % 10, hue, sat, val);

    // Draw first colon
    qp_rect(display, start_x + 32, timer_y + 5, start_x + 35, timer_y + 7, hue, sat, val, true);
    qp_rect(display, start_x + 32, timer_y + 13, start_x + 35, timer_y + 15, hue, sat, val, true);

    // Draw minutes (2 digits)
    draw_digit(start_x + 38, timer_y, minutes / 10, hue, sat, val);
    draw_digit(start_x + 54, timer_y, minutes % 10, hue, sat, val);

    // Draw second colon
    qp_rect(display, start_x + 70, timer_y + 5, start_x + 73, timer_y + 7, hue, sat, val, true);
    qp_rect(display, start_x + 70, timer_y + 13, start_x + 73, timer_y + 15, hue, sat, val, true);

    // Draw seconds (2 digits)
    draw_digit(start_x + 76, timer_y, seconds / 10, hue, sat, val);
    draw_digit(start_x + 92, timer_y, seconds % 10, hue, sat, val);

    qp_flush(display);
}

// Function to draw logo with color based on layer
void set_layer_background(uint8_t layer) {
    // Only update if the layer actually changed
    if (layer == current_display_layer) {
        return;
    }
    current_display_layer = layer;

    // Always draw white background
    qp_rect(display, 0, 0, 134, 239, 0, 0, 255, true);

    // Select logo color based on layer
    uint8_t hue, sat, val;
    get_layer_color(layer, &hue, &sat, &val);

    // Draw the logo at the top with the selected color
    draw_amboss_logo(display, 7, 10, hue, sat, val);

    // Redraw uptime timer above volume bar (will handle its own clearing)
    draw_uptime_timer();

    // Redraw volume bar with the same color at the bottom
    draw_volume_bar(hue, sat, val);

    qp_flush(display);
}

// Update display based on current layer state
void update_display_for_layer(void) {
    set_layer_background(get_highest_layer(layer_state));
}

// Draw volume bar at bottom of screen (permanent)
void draw_volume_bar(uint8_t hue, uint8_t sat, uint8_t val) {
    // Calculate bar width based on volume (max width 120 pixels, leaving margins)
    uint16_t bar_width = (current_volume * 120) / 100;

    // Clear bottom area with white background
    qp_rect(display, 0, 225, 134, 239, 0, 0, 255, true);

    // Draw volume bar outline (thin dark grey border)
    qp_rect(display, 5, 230, 127, 236, 0, 0, 80, false);

    // Draw filled volume bar using the logo color
    if (bar_width > 0) {
        qp_rect(display, 6, 231, 6 + bar_width, 235, hue, sat, val, true);
    }

    qp_flush(display);
}

// Draw brightness indicator overlay (temporary, appears when brightness changes)
void draw_brightness_indicator(void) {
    // Brightness indicator appears as an overlay at the top of the screen
    // Box dimensions: 100x40 at top center
    uint16_t box_x = 17;   // (135 - 100) / 2
    uint16_t box_y = 10;   // Top of screen
    uint16_t box_w = 100;
    uint16_t box_h = 40;

    // Draw semi-transparent background (light grey box with border)
    qp_rect(display, box_x, box_y, box_x + box_w, box_y + box_h, 0, 0, 220, true);
    qp_rect(display, box_x, box_y, box_x + box_w, box_y + box_h, 0, 0, 100, false);

    // Draw "BRI" text using simple rectangles at top of box
    uint16_t text_y = box_y + 6;
    uint16_t text_x = box_x + 8;

    // B
    qp_rect(display, text_x, text_y, text_x + 1, text_y + 9, 0, 0, 0, true);
    qp_rect(display, text_x, text_y, text_x + 6, text_y + 1, 0, 0, 0, true);
    qp_rect(display, text_x, text_y + 4, text_x + 5, text_y + 5, 0, 0, 0, true);
    qp_rect(display, text_x, text_y + 9, text_x + 6, text_y + 10, 0, 0, 0, true);
    qp_rect(display, text_x + 5, text_y + 1, text_x + 7, text_y + 4, 0, 0, 0, true);
    qp_rect(display, text_x + 5, text_y + 5, text_x + 7, text_y + 9, 0, 0, 0, true);

    // R
    text_x += 10;
    qp_rect(display, text_x, text_y, text_x + 1, text_y + 9, 0, 0, 0, true);
    qp_rect(display, text_x, text_y, text_x + 6, text_y + 1, 0, 0, 0, true);
    qp_rect(display, text_x, text_y + 4, text_x + 5, text_y + 5, 0, 0, 0, true);
    qp_rect(display, text_x + 5, text_y + 1, text_x + 7, text_y + 4, 0, 0, 0, true);
    qp_rect(display, text_x + 4, text_y + 5, text_x + 7, text_y + 9, 0, 0, 0, true);

    // I
    text_x += 10;
    qp_rect(display, text_x, text_y, text_x + 5, text_y + 1, 0, 0, 0, true);
    qp_rect(display, text_x + 2, text_y + 1, text_x + 3, text_y + 9, 0, 0, 0, true);
    qp_rect(display, text_x, text_y + 9, text_x + 5, text_y + 10, 0, 0, 0, true);

    // Draw percentage using 7-segment digits
    uint16_t digit_x = box_x + 36;
    uint16_t digit_y = box_y + 18;

    // Get current layer color for digits
    uint8_t layer = get_highest_layer(layer_state);
    uint8_t hue, sat, val;
    get_layer_color(layer, &hue, &sat, &val);

    // Calculate brightness percentage (backlight_brightness is 0-255, convert to 0-100)
    uint8_t brightness_percent = (backlight_brightness * 100) / 255;

    // Draw brightness percentage (3 digits max: 0-100)
    uint8_t hundreds = brightness_percent / 100;
    uint8_t tens = (brightness_percent % 100) / 10;
    uint8_t ones = brightness_percent % 10;

    if (hundreds > 0) {
        draw_digit(digit_x, digit_y, hundreds, hue, sat, val);
        digit_x += 14;
    }

    if (hundreds > 0 || tens > 0) {
        draw_digit(digit_x, digit_y, tens, hue, sat, val);
        digit_x += 14;
    }

    draw_digit(digit_x, digit_y, ones, hue, sat, val);
    digit_x += 14;

    // Draw "%" symbol
    qp_rect(display, digit_x + 2, digit_y + 2, digit_x + 4, digit_y + 4, hue, sat, val, true);
    qp_rect(display, digit_x + 4, digit_y + 5, digit_x + 6, digit_y + 7, hue, sat, val, true);
    qp_rect(display, digit_x + 5, digit_y + 8, digit_x + 7, digit_y + 10, hue, sat, val, true);
    qp_rect(display, digit_x + 7, digit_y + 12, digit_x + 9, digit_y + 14, hue, sat, val, true);
    qp_rect(display, digit_x + 2, digit_y + 16, digit_x + 4, digit_y + 18, hue, sat, val, true);

    qp_flush(display);
}

// Set backlight brightness via PWM
void set_backlight_brightness(uint8_t brightness) {
    backlight_brightness = brightness;
    // Update PWM duty cycle (channel A compare value)
    *(volatile uint32_t*)(0x40050028 + 0x0C) = brightness;

    // Show brightness indicator overlay and reset timer
    brightness_display_active = true;
    brightness_display_timer = timer_read32();
    last_brightness_value = brightness;

    // Draw the brightness indicator overlay
    draw_brightness_indicator();
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

    // Draw the Amboss logo at the top in teal using line-by-line rendering
    // Logo is 120x120, centered horizontally (135-120)/2 = 7.5, positioned at top (y=10)
    draw_amboss_logo(display, 7, 10, 128, 255, 255);  // Teal color

    // Update brightness variable to match the PWM setting
    backlight_brightness = 102;
    last_brightness_value = 102;

    // Draw initial uptime timer above volume bar
    draw_uptime_timer();

    // Draw initial volume bar with teal color (base layer) at bottom
    draw_volume_bar(128, 255, 255);

    // Force flush to ensure everything is drawn
    qp_flush(display);
}

void keyboard_post_init_kb(void) {
    // Initialize the display
    init_display();
}

// Raw HID receive callback - handles data from computer
void raw_hid_receive(uint8_t *data, uint8_t length) {
    // Protocol:
    // Byte 0: Command ID
    //   0x01 = Volume update
    // Byte 1: Volume level (0-100)

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
            break;

        default:
            // Unknown command, ignore
            break;
    }
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

    uint32_t current_time = timer_read32();

    // Update uptime timer once per second
    if (current_time - last_uptime_update >= 1000) {
        last_uptime_update = current_time;
        draw_uptime_timer();
    }

    // Handle brightness display timeout
    if (brightness_display_active) {
        if (current_time - brightness_display_timer >= BRIGHTNESS_DISPLAY_TIMEOUT) {
            // Timeout expired, hide brightness indicator
            brightness_display_active = false;
            // Force a full redraw by invalidating the current layer
            current_display_layer = 255;
            update_display_for_layer();
        }
    }
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
