# Framebuffer Integration Guide

This guide explains how to integrate the framebuffer system into your keymap.

## Overview

The framebuffer system provides an in-memory buffer (63 KB) where you can draw graphics before flushing them to the display. This approach offers several advantages:

1. **Eliminates flickering** - All drawing happens in memory, then appears instantly
2. **Allows complex compositions** - Draw multiple elements, then show all at once
3. **Enables read-modify-write** - You can read pixel values and modify them
4. **Dirty region optimization** - Only updates changed areas of the screen

## Quick Start

### 1. Include the header

```c
#include "framebuffer.h"
```

### 2. Initialize the framebuffer

In your `keyboard_post_init_kb()` or after display initialization:

```c
void keyboard_post_init_kb(void) {
    // ... existing display initialization code ...

    // Initialize framebuffer
    fb_init();
}
```

### 3. Replace drawing calls

Replace existing `qp_*` calls with `fb_*` equivalents:

**Before:**
```c
qp_rect(display, 10, 20, 50, 60, 128, 255, 255, true);  // HSV teal filled rect
qp_circle(display, 30, 40, 10, 0, 255, 255, true);      // HSV red filled circle
qp_line(display, 0, 0, 100, 100, 85, 255, 255);         // HSV green line
```

**After:**
```c
fb_rect_hsv(10, 20, 50, 60, 128, 255, 255, true);       // Draw to framebuffer
fb_circle_hsv(30, 40, 10, 0, 255, 255, true);           // Draw to framebuffer
fb_line_hsv(0, 0, 100, 100, 85, 255, 255);              // Draw to framebuffer

fb_flush(display);  // Send all changes to display at once
```

### 4. Flush at appropriate times

Call `fb_flush(display)` when you want to update the screen:

```c
void render_scene(void) {
    // Clear to black
    fb_clear(FB_COLOR_BLACK);

    // Draw your scene
    fb_rect_hsv(10, 10, 50, 50, 128, 255, 255, true);
    fb_circle_hsv(70, 30, 15, 0, 255, 255, true);

    // Update the display
    fb_flush(display);
}
```

## API Reference

### Initialization

```c
void fb_init(void);                          // Initialize framebuffer (clear to black)
void fb_clear(fb_color_t color);             // Clear to specific color
```

### Drawing Functions (HSV variants)

These match QP's HSV API (hue, sat, val all 0-255):

```c
void fb_set_pixel_hsv(int16_t x, int16_t y, uint8_t hue, uint8_t sat, uint8_t val);
void fb_line_hsv(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t hue, uint8_t sat, uint8_t val);
void fb_rect_hsv(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t hue, uint8_t sat, uint8_t val, bool filled);
void fb_circle_hsv(int16_t x, int16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled);
void fb_ellipse_hsv(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t hue, uint8_t sat, uint8_t val, bool filled);
```

### Drawing Functions (RGB565 variants)

For direct RGB565 color values:

```c
void fb_set_pixel(int16_t x, int16_t y, fb_color_t color);
void fb_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, fb_color_t color);
void fb_rect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, fb_color_t color, bool filled);
void fb_circle(int16_t x, int16_t y, uint16_t radius, fb_color_t color, bool filled);
void fb_ellipse(int16_t x, int16_t y, uint16_t rx, uint16_t ry, fb_color_t color, bool filled);
```

### Color Utilities

```c
fb_color_t fb_hsv_to_rgb565(uint8_t hue, uint8_t sat, uint8_t val);
fb_color_t fb_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);
fb_color_t fb_get_pixel(int16_t x, int16_t y);  // Read pixel color
```

### Predefined Colors

```c
FB_COLOR_BLACK, FB_COLOR_WHITE, FB_COLOR_RED, FB_COLOR_GREEN, FB_COLOR_BLUE
FB_COLOR_YELLOW, FB_COLOR_CYAN, FB_COLOR_MAGENTA, FB_COLOR_ORANGE
FB_COLOR_PURPLE, FB_COLOR_TEAL, FB_COLOR_GRAY
```

### Display Update

```c
void fb_flush(painter_device_t display);     // Send framebuffer to display (smart update)
void fb_mark_dirty_all(void);                // Force full screen refresh
```

## Conversion Examples

### Example 1: Simple shapes

**Original QP code:**
```c
void draw_ui(void) {
    qp_rect(display, 0, 0, 134, 239, 0, 0, 0, true);  // Clear screen
    qp_circle(display, 67, 120, 30, 128, 255, 255, true);  // Teal circle
    qp_rect(display, 50, 10, 84, 30, 0, 255, 255, true);   // Red rect
}
```

**Framebuffer code:**
```c
void draw_ui(void) {
    fb_clear(FB_COLOR_BLACK);  // Clear screen
    fb_circle_hsv(67, 120, 30, 128, 255, 255, true);  // Teal circle
    fb_rect_hsv(50, 10, 84, 30, 0, 255, 255, true);   // Red rect
    fb_flush(display);  // Update display
}
```

### Example 2: Animation with double buffering effect

```c
uint32_t animation_timer = 0;
int16_t ball_x = 0;

void update_animation(void) {
    if (timer_elapsed32(animation_timer) > 16) {  // ~60 FPS
        animation_timer = timer_read32();

        // Clear and redraw
        fb_clear(FB_COLOR_BLACK);

        // Draw moving ball
        fb_circle_hsv(ball_x, 120, 5, 0, 255, 255, true);
        ball_x = (ball_x + 1) % FB_WIDTH;

        // Draw background elements
        fb_rect_hsv(0, 230, FB_WIDTH-1, 239, 128, 255, 128, true);

        // Update display once per frame
        fb_flush(display);
    }
}
```

### Example 3: Reading and modifying pixels

```c
void invert_region(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    for (int16_t y = y1; y <= y2; y++) {
        for (int16_t x = x1; x <= x2; x++) {
            fb_color_t color = fb_get_pixel(x, y);
            fb_set_pixel(x, y, ~color);  // Invert color
        }
    }
    fb_flush(display);
}
```

## Performance Considerations

### Memory Usage

- **Framebuffer size**: 135 × 240 × 2 bytes = **64,800 bytes (~63 KB)**
- **RP2040 total SRAM**: 264 KB
- **Remaining for code/stack**: ~200 KB

### Optimization Tips

1. **Batch updates**: Draw multiple elements, then flush once
2. **Avoid unnecessary clears**: Only clear if needed
3. **Use dirty regions automatically**: The system tracks changed areas
4. **Minimize flushes**: Flush once per frame, not per element

### Timing

- **Framebuffer operations**: Near-instant (memory writes)
- **Flush operation**: ~5-10ms for full screen update
- **Dirty region flush**: ~1-3ms for small regions

## Known Limitations

1. **No text rendering yet**: You'll still need to use `qp_drawtext*` functions directly on the display, or implement custom text rendering
2. **No image/QGF support**: Images still need to be drawn with QP functions
3. **Single buffer**: True double-buffering would require 2× memory (127 KB)

## Next Steps

1. Convert your existing drawing functions to use framebuffer
2. Group related drawing operations together
3. Call `fb_flush(display)` at strategic points (end of frame, after scene updates)
4. Test and compare performance/visual quality

## Example: Converting your draw_digit function

**Original:**
```c
void draw_digit(uint16_t x, uint16_t y, uint8_t digit, uint8_t hue, uint8_t sat, uint8_t val) {
    // ... segment logic ...
    if (seg_a) qp_rect(display, x + 2, y, x + 11, y + 2, hue, sat, val, true);
    if (seg_b) qp_rect(display, x + 11, y + 2, x + 13, y + 9, hue, sat, val, true);
    // ... more segments ...
}
```

**Framebuffer version:**
```c
void draw_digit(uint16_t x, uint16_t y, uint8_t digit, uint8_t hue, uint8_t sat, uint8_t val) {
    // ... segment logic ...
    if (seg_a) fb_rect_hsv(x + 2, y, x + 11, y + 2, hue, sat, val, true);
    if (seg_b) fb_rect_hsv(x + 11, y + 2, x + 13, y + 9, hue, sat, val, true);
    // ... more segments ...
    // Note: No fb_flush() here - let the caller decide when to flush
}
```

Then in your main rendering:
```c
void draw_time(void) {
    draw_digit(10, 20, 1, 128, 255, 255);
    draw_digit(25, 20, 2, 128, 255, 255);
    draw_digit(45, 20, 3, 128, 255, 255);
    draw_digit(60, 20, 0, 128, 255, 255);

    fb_flush(display);  // Update display once with all digits
}
```
