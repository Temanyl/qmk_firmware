/*
 * Framebuffer Example / Test Code
 *
 * This file demonstrates how to use the framebuffer system.
 * You can copy these examples into your keymap.c
 */

#include "framebuffer.h"
#include "qp.h"

// Example 1: Simple test pattern
void fb_test_pattern(painter_device_t display) {
    // Clear to black
    fb_clear(FB_COLOR_BLACK);

    // Draw colored rectangles
    fb_rect_hsv(10, 10, 40, 40, 0, 255, 255, true);      // Red
    fb_rect_hsv(50, 10, 80, 40, 85, 255, 255, true);     // Green
    fb_rect_hsv(90, 10, 120, 40, 170, 255, 255, true);   // Blue

    // Draw circles
    fb_circle_hsv(25, 70, 15, 128, 255, 255, true);      // Teal
    fb_circle_hsv(67, 70, 15, 43, 255, 255, true);       // Yellow
    fb_circle_hsv(109, 70, 15, 213, 255, 255, true);     // Magenta

    // Draw lines
    fb_line_hsv(0, 100, 134, 100, 0, 0, 255);            // White horizontal line
    fb_line_hsv(67, 110, 67, 150, 0, 0, 255);            // White vertical line

    // Draw ellipses
    fb_ellipse_hsv(40, 180, 30, 20, 200, 255, 200, false);  // Blue outline
    fb_ellipse_hsv(95, 180, 20, 30, 20, 255, 200, true);    // Orange filled

    // Flush everything to display at once
    fb_flush(display);
}

// Example 2: Gradient demonstration
void fb_gradient_demo(painter_device_t display) {
    fb_clear(FB_COLOR_BLACK);

    // Horizontal hue gradient
    for (uint16_t x = 0; x < FB_WIDTH; x++) {
        uint8_t hue = (x * 255) / FB_WIDTH;
        fb_line_hsv(x, 0, x, 100, hue, 255, 255);
    }

    // Vertical brightness gradient
    for (uint16_t y = 110; y < 230; y++) {
        uint8_t val = ((y - 110) * 255) / 120;
        fb_line_hsv(0, y, FB_WIDTH - 1, y, 128, 255, val);
    }

    fb_flush(display);
}

// Example 3: Animation frame
void fb_bouncing_ball_frame(painter_device_t display, int16_t ball_x, int16_t ball_y) {
    // Clear screen
    fb_clear(FB_COLOR_BLACK);

    // Draw "ground"
    fb_rect_hsv(0, 230, FB_WIDTH - 1, FB_HEIGHT - 1, 128, 255, 128, true);

    // Draw ball with shadow
    fb_ellipse_hsv(ball_x, 230, 10, 3, 0, 0, 50, true);  // Shadow
    fb_circle_hsv(ball_x, ball_y, 8, 0, 255, 255, true);  // Ball

    // Draw score/UI bar at top
    fb_rect_hsv(0, 0, FB_WIDTH - 1, 15, 43, 200, 100, true);

    fb_flush(display);
}

// Example 4: Pixel manipulation - Create a pixelated effect
void fb_pixelate_region(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t block_size) {
    for (int16_t y = y1; y < y2; y += block_size) {
        for (int16_t x = x1; x < x2; x += block_size) {
            // Sample one pixel from this block
            fb_color_t color = fb_get_pixel(x, y);

            // Fill the entire block with that color
            fb_rect(x, y, x + block_size - 1, y + block_size - 1, color, true);
        }
    }
    fb_flush(display);
}

// Example 5: Performance test - stress test the framebuffer
void fb_performance_test(painter_device_t display) {
    uint32_t start_time = timer_read32();

    // Test 1: 1000 random pixels
    fb_clear(FB_COLOR_BLACK);
    for (uint16_t i = 0; i < 1000; i++) {
        int16_t x = rand() % FB_WIDTH;
        int16_t y = rand() % FB_HEIGHT;
        uint8_t hue = rand() % 256;
        fb_set_pixel_hsv(x, y, hue, 255, 255);
    }
    fb_flush(display);

    uint32_t pixel_time = timer_elapsed32(start_time);

    // Test 2: 100 circles
    start_time = timer_read32();
    fb_clear(FB_COLOR_BLACK);
    for (uint8_t i = 0; i < 100; i++) {
        int16_t x = rand() % FB_WIDTH;
        int16_t y = rand() % FB_HEIGHT;
        uint8_t radius = (rand() % 10) + 5;
        uint8_t hue = rand() % 256;
        fb_circle_hsv(x, y, radius, hue, 255, 200, true);
    }
    fb_flush(display);

    uint32_t circle_time = timer_elapsed32(start_time);

    // Test 3: Full screen fill
    start_time = timer_read32();
    fb_clear(fb_hsv_to_rgb565(128, 255, 255));
    fb_flush(display);

    uint32_t fill_time = timer_elapsed32(start_time);

    // Results can be printed via console or stored for analysis
    // pixel_time, circle_time, fill_time contain milliseconds
}

// Example 6: Layer indicator (practical example)
void fb_draw_layer_indicator(painter_device_t display, uint8_t layer) {
    // Define colors for each layer
    const uint8_t layer_colors[4][3] = {
        {128, 255, 255},  // Layer 0: Teal
        {85, 255, 255},   // Layer 1: Green
        {0, 255, 255},    // Layer 2: Red
        {43, 255, 255}    // Layer 3: Yellow
    };

    if (layer >= 4) layer = 0;

    // Draw a small indicator in corner
    uint8_t hue = layer_colors[layer][0];
    uint8_t sat = layer_colors[layer][1];
    uint8_t val = layer_colors[layer][2];

    // Circle in top-right corner
    fb_circle_hsv(FB_WIDTH - 15, 10, 8, hue, sat, val, true);

    // Optional: Draw layer number
    // (You'd need to implement text rendering or use small shapes)

    fb_flush(display);
}

// Example 7: Using predefined colors
void fb_traffic_light(painter_device_t display) {
    fb_clear(FB_COLOR_BLACK);

    // Draw traffic light housing
    fb_rect(50, 50, 84, 150, FB_COLOR_GRAY, true);
    fb_rect(50, 50, 84, 150, FB_COLOR_WHITE, false);  // Border

    // Red light
    fb_circle(67, 70, 12, FB_COLOR_RED, true);

    // Yellow light
    fb_circle(67, 100, 12, FB_COLOR_YELLOW, true);

    // Green light
    fb_circle(67, 130, 12, FB_COLOR_GREEN, true);

    fb_flush(display);
}

// Example 8: Integration with your existing code
// Shows how to convert your draw_digit function to use framebuffer
void fb_draw_digit_example(uint16_t x, uint16_t y, uint8_t digit, uint8_t hue, uint8_t sat, uint8_t val) {
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

    // Draw segments using framebuffer (no display parameter needed!)
    if (seg_a) fb_rect_hsv(x + 2, y, x + 11, y + 2, hue, sat, val, true);
    if (seg_b) fb_rect_hsv(x + 11, y + 2, x + 13, y + 9, hue, sat, val, true);
    if (seg_c) fb_rect_hsv(x + 11, y + 11, x + 13, y + 18, hue, sat, val, true);
    if (seg_d) fb_rect_hsv(x + 2, y + 18, x + 11, y + 20, hue, sat, val, true);
    if (seg_e) fb_rect_hsv(x, y + 11, x + 2, y + 18, hue, sat, val, true);
    if (seg_f) fb_rect_hsv(x, y + 2, x + 2, y + 9, hue, sat, val, true);
    if (seg_g) fb_rect_hsv(x + 2, y + 9, x + 11, y + 11, hue, sat, val, true);

    // Note: Don't flush here - let caller decide when to update display
}
