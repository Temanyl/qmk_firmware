# Framebuffer System for chocmanyl36

A complete in-memory framebuffer implementation for the ST7789 display (135×240 pixels).

## What is it?

Instead of drawing directly to the display with `qp_*` functions (which are slow and can cause flickering), you now draw to an in-memory buffer and then "flush" it all at once to the display.

**Benefits:**
- No flickering or tearing
- Faster rendering (memory writes vs SPI transfers)
- Compositing: draw multiple things, then show all at once
- Read pixels back: enables effects like blur, invert, pixelate
- Automatic dirty region tracking: only updates changed areas

## Files

- `framebuffer.h` - API declarations and color definitions
- `framebuffer.c` - Implementation (drawing algorithms, flush logic)
- `framebuffer_example.c` - 8 example functions showing various use cases
- `FRAMEBUFFER_INTEGRATION.md` - Detailed integration guide

## Memory Usage

- **Framebuffer size**: 64,800 bytes (~63 KB)
- **Format**: RGB565 (16-bit color: 5 red, 6 green, 5 blue)
- **Impact**: Uses 24% of RP2040's 264KB SRAM
- **Remaining**: ~200KB for code/stack/variables

## Quick Start

### 1. Initialize in your code

Add to your display initialization:

```c
#include "framebuffer.h"

void keyboard_post_init_kb(void) {
    // ... your existing display init code ...

    // Initialize framebuffer
    fb_init();
}
```

### 2. Replace qp_* calls with fb_* calls

**Before:**
```c
qp_rect(display, 10, 10, 50, 50, 128, 255, 255, true);
qp_circle(display, 70, 30, 15, 0, 255, 255, true);
```

**After:**
```c
fb_rect_hsv(10, 10, 50, 50, 128, 255, 255, true);
fb_circle_hsv(70, 30, 15, 0, 255, 255, true);
fb_flush(display);  // Update display
```

### 3. Batch your drawing operations

Group related drawings together, then flush once:

```c
void render_ui(void) {
    // Clear to black
    fb_clear(FB_COLOR_BLACK);

    // Draw all elements
    draw_logo();
    draw_time();
    draw_volume_bar();
    draw_layer_indicator();

    // Update display once
    fb_flush(display);
}
```

## API Summary

### Initialization
```c
fb_init()                    // Initialize (clears to black)
fb_clear(color)              // Clear to specific color
```

### Drawing (HSV colors - matches QP API)
```c
fb_set_pixel_hsv(x, y, h, s, v)
fb_line_hsv(x1, y1, x2, y2, h, s, v)
fb_rect_hsv(x1, y1, x2, y2, h, s, v, filled)
fb_circle_hsv(x, y, radius, h, s, v, filled)
fb_ellipse_hsv(x, y, rx, ry, h, s, v, filled)
```

### Drawing (RGB565 direct colors)
```c
fb_set_pixel(x, y, color)
fb_line(x1, y1, x2, y2, color)
fb_rect(x1, y1, x2, y2, color, filled)
fb_circle(x, y, radius, color, filled)
fb_ellipse(x, y, rx, ry, color, filled)
```

### Color utilities
```c
fb_hsv_to_rgb565(h, s, v)         // Convert HSV to RGB565
fb_rgb888_to_rgb565(r, g, b)      // Convert RGB888 to RGB565
fb_get_pixel(x, y)                // Read pixel color
```

### Predefined colors
```c
FB_COLOR_BLACK, FB_COLOR_WHITE, FB_COLOR_RED, FB_COLOR_GREEN, FB_COLOR_BLUE
FB_COLOR_YELLOW, FB_COLOR_CYAN, FB_COLOR_MAGENTA, FB_COLOR_ORANGE
FB_COLOR_PURPLE, FB_COLOR_TEAL, FB_COLOR_GRAY
```

### Display update
```c
fb_flush(display)            // Send framebuffer to display (smart partial update)
fb_mark_dirty_all()          // Force full screen refresh
```

## Examples

### Example 1: Simple Test Pattern

```c
fb_clear(FB_COLOR_BLACK);
fb_rect_hsv(10, 10, 40, 40, 0, 255, 255, true);      // Red square
fb_circle_hsv(67, 67, 30, 128, 255, 255, true);      // Teal circle
fb_line_hsv(0, 0, 134, 239, 0, 0, 255);              // White diagonal
fb_flush(display);
```

### Example 2: Animation

```c
static int16_t ball_x = 0;

void update_animation(void) {
    fb_clear(FB_COLOR_BLACK);
    fb_circle_hsv(ball_x, 120, 10, 0, 255, 255, true);
    ball_x = (ball_x + 1) % FB_WIDTH;
    fb_flush(display);
}
```

### Example 3: Layer Indicator

```c
void draw_layer(uint8_t layer) {
    uint8_t hues[] = {128, 85, 0, 43};  // Teal, Green, Red, Yellow
    fb_circle_hsv(120, 10, 8, hues[layer % 4], 255, 255, true);
    fb_flush(display);
}
```

## Performance

### Timing (approximate)
- **Pixel/line/rect in framebuffer**: < 1 μs (memory write)
- **Circle (r=20, filled)**: ~100 μs
- **Full screen clear**: ~3 ms
- **Flush (full screen)**: ~5-10 ms
- **Flush (small dirty region)**: ~1-3 ms

### Optimization tips
1. **Minimize flushes** - Draw many elements, flush once per frame
2. **Use dirty regions** - Automatic, no work needed
3. **Batch operations** - Group related drawings together
4. **Avoid unnecessary clears** - Only clear when needed

## Limitations

1. **No text rendering** - Still use `qp_drawtext*` directly or implement custom
2. **No QGF/image support** - Images still need QP functions
3. **Single buffer** - Not true double-buffering (would need 127 KB)
4. **No rotation** - Framebuffer is fixed to display orientation

## Integration Steps

1. **Test**: Copy examples from `framebuffer_example.c` to verify it works
2. **Convert**: Replace `qp_*` drawing calls with `fb_*` equivalents
3. **Optimize**: Group drawings and add `fb_flush()` at strategic points
4. **Measure**: Compare before/after for flickering and performance

## Example Migration

See `FRAMEBUFFER_INTEGRATION.md` for detailed migration guide.

### Before (direct QP)
```c
void draw_ui(void) {
    qp_rect(display, 0, 0, 134, 239, 0, 0, 0, true);  // Clear
    qp_rect(display, 10, 10, 50, 50, 128, 255, 255, true);
    qp_circle(display, 70, 30, 15, 0, 255, 255, true);
    qp_flush(display);
}
```

### After (framebuffer)
```c
void draw_ui(void) {
    fb_clear(FB_COLOR_BLACK);  // Clear
    fb_rect_hsv(10, 10, 50, 50, 128, 255, 255, true);
    fb_circle_hsv(70, 30, 15, 0, 255, 255, true);
    fb_flush(display);  // Update display once
}
```

## Testing

Build and flash the firmware:
```bash
qmk compile -kb handwired/temanyl/chocmanyl36 -km default
qmk flash -kb handwired/temanyl/chocmanyl36 -km default
```

The framebuffer system is now compiled into your firmware. To use it, you need to:
1. Add `#include "framebuffer.h"` to your keymap.c
2. Call `fb_init()` during initialization
3. Replace your drawing calls with `fb_*` functions
4. Call `fb_flush(display)` when you want to update the screen

## Troubleshooting

### Compiler errors about memory
- Check available SRAM with `arm-none-eabi-size handwired_temanyl_chocmanyl36_default.elf`
- If running low, consider:
  - 8-bit indexed color (32KB instead of 63KB)
  - Smaller framebuffer (crop unused areas)
  - Move large arrays to flash (const)

### Display shows nothing
- Make sure `fb_init()` is called after display initialization
- Verify `fb_flush(display)` is being called
- Check that display handle is valid

### Display shows garbage
- Ensure viewport offsets match your display
- Verify RGB565 byte order (may need swap)
- Check SPI initialization order

### Slow performance
- Reduce flush frequency (once per frame, not per element)
- Profile with `timer_read32()` to find bottlenecks
- Consider smaller dirty regions

## Advanced Usage

### Custom color conversion
```c
// Create custom color from RGB888
fb_color_t my_color = fb_rgb888_to_rgb565(128, 200, 64);
fb_rect(0, 0, 100, 50, my_color, true);
```

### Read and modify pixels
```c
// Invert a region
for (int y = 0; y < 50; y++) {
    for (int x = 0; x < 50; x++) {
        fb_color_t color = fb_get_pixel(x, y);
        fb_set_pixel(x, y, ~color);
    }
}
fb_flush(display);
```

### Partial updates
```c
// Only update top half of screen
fb_mark_dirty_region(0, 0, FB_WIDTH-1, FB_HEIGHT/2);
fb_flush(display);
```

## Next Steps

1. Read `FRAMEBUFFER_INTEGRATION.md` for detailed integration guide
2. Try examples from `framebuffer_example.c`
3. Convert one drawing function at a time
4. Profile and measure improvements
5. Enjoy flicker-free graphics!
