/*
 * Framebuffer Implementation for chocmanyl36
 */

#include "framebuffer.h"
#include <stdlib.h>

// Global framebuffer instance
framebuffer_t fb;

// ============================================================================
// Helper Functions
// ============================================================================

// Min/max macros (guard against redefinition)
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif

// Swap two integers
static inline void swap_int16(int16_t *a, int16_t *b) {
    int16_t temp = *a;
    *a = *b;
    *b = temp;
}

// ============================================================================
// Color Conversion Functions
// ============================================================================

fb_color_t fb_hsv_to_rgb565(uint8_t hue, uint8_t sat, uint8_t val) {
    // Convert HSV to RGB888 using QMK's algorithm
    uint8_t  r, g, b;
    uint16_t h, s, v;

    if (sat == 0) {
        // Grayscale
        r = g = b = val;
    } else {
        h = hue;
        s = sat;
        v = val;

        uint8_t region    = h * 6 / 255;
        uint8_t remainder = (h * 2 - region * 85) * 3;

        uint8_t p = (v * (255 - s)) >> 8;
        uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

        switch (region) {
            case 6:
            case 0:
                r = v; g = t; b = p;
                break;
            case 1:
                r = q; g = v; b = p;
                break;
            case 2:
                r = p; g = v; b = t;
                break;
            case 3:
                r = p; g = q; b = v;
                break;
            case 4:
                r = t; g = p; b = v;
                break;
            default:
                r = v; g = p; b = q;
                break;
        }
    }

    // Convert RGB888 to RGB565 and byte swap for ST7789
    return fb_rgb888_to_rgb565(r, g, b);
}

fb_color_t fb_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    // RGB565: RRRRRGGGGGGBBBBB
    uint16_t rgb565 = (((uint16_t)r) >> 3) << 11 | (((uint16_t)g) >> 2) << 5 | (((uint16_t)b) >> 3);
    // ST7789 requires byte-swapped RGB565
    return __builtin_bswap16(rgb565);
}

// ============================================================================
// Core Framebuffer Functions
// ============================================================================

void fb_init(void) {
    fb_clear(FB_COLOR_BLACK);
    fb.is_dirty = false;
    fb.dirty_x1 = 0;
    fb.dirty_y1 = 0;
    fb.dirty_x2 = 0;
    fb.dirty_y2 = 0;
}

void fb_clear(fb_color_t color) {
    for (uint16_t y = 0; y < FB_HEIGHT; y++) {
        for (uint16_t x = 0; x < FB_WIDTH; x++) {
            fb.pixels[y][x] = color;
        }
    }
    fb_mark_dirty_all();
}

void fb_mark_dirty_all(void) {
    fb.is_dirty = true;
    fb.dirty_x1 = 0;
    fb.dirty_y1 = 0;
    fb.dirty_x2 = FB_WIDTH - 1;
    fb.dirty_y2 = FB_HEIGHT - 1;
}

void fb_mark_dirty_region(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // Clamp to screen bounds
    x1 = MIN(x1, FB_WIDTH - 1);
    x2 = MIN(x2, FB_WIDTH - 1);
    y1 = MIN(y1, FB_HEIGHT - 1);
    y2 = MIN(y2, FB_HEIGHT - 1);

    if (!fb.is_dirty) {
        fb.is_dirty = true;
        fb.dirty_x1 = x1;
        fb.dirty_y1 = y1;
        fb.dirty_x2 = x2;
        fb.dirty_y2 = y2;
    } else {
        // Expand dirty region
        fb.dirty_x1 = MIN(fb.dirty_x1, x1);
        fb.dirty_y1 = MIN(fb.dirty_y1, y1);
        fb.dirty_x2 = MAX(fb.dirty_x2, x2);
        fb.dirty_y2 = MAX(fb.dirty_y2, y2);
    }
}

void fb_flush(painter_device_t display) {
    if (!fb.is_dirty) {
        return; // Nothing to update
    }

    // Clamp dirty region to framebuffer area (upper region only, y=0 to FB_SPLIT_Y-1)
    // This ensures we don't overwrite the QP-rendered lower region (date, time, media, volume)
    uint16_t y1_clamped = fb.dirty_y1;
    uint16_t y2_clamped = MIN(fb.dirty_y2, FB_SPLIT_Y - 1);

    // Skip flush if dirty region is entirely outside framebuffer area
    if (y1_clamped >= FB_SPLIT_Y) {
        fb.is_dirty = false;
        return;
    }

    // Set viewport to dirty region (clamped to framebuffer area)
    qp_viewport(display, fb.dirty_x1, y1_clamped, fb.dirty_x2, y2_clamped);

    // Calculate region width
    uint16_t width = fb.dirty_x2 - fb.dirty_x1 + 1;

    // Stream pixel data row by row
    for (uint16_t y = y1_clamped; y <= y2_clamped; y++) {
        qp_pixdata(display, &fb.pixels[y][fb.dirty_x1], width);
    }

    // Clear dirty flag
    fb.is_dirty = false;
}

// ============================================================================
// Drawing Primitives - Pixel
// ============================================================================

void fb_set_pixel(int16_t x, int16_t y, fb_color_t color) {
    // Bounds checking
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT) {
        return;
    }

    fb.pixels[y][x] = color;
    fb_mark_dirty_region(x, y, x, y);
}

void fb_set_pixel_hsv(int16_t x, int16_t y, uint8_t hue, uint8_t sat, uint8_t val) {
    fb_set_pixel(x, y, fb_hsv_to_rgb565(hue, sat, val));
}

fb_color_t fb_get_pixel(int16_t x, int16_t y) {
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT) {
        return 0;
    }
    return fb.pixels[y][x];
}

// ============================================================================
// Drawing Primitives - Line
// ============================================================================

void fb_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, fb_color_t color) {
    // Bresenham's line algorithm
    int16_t dx = ABS(x2 - x1);
    int16_t dy = -ABS(y2 - y1);
    int16_t sx = x1 < x2 ? 1 : -1;
    int16_t sy = y1 < y2 ? 1 : -1;
    int16_t err = dx + dy;

    while (true) {
        fb_set_pixel(x1, y1, color);

        if (x1 == x2 && y1 == y2) break;

        int16_t e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void fb_line_hsv(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t hue, uint8_t sat, uint8_t val) {
    fb_line(x1, y1, x2, y2, fb_hsv_to_rgb565(hue, sat, val));
}

// ============================================================================
// Drawing Primitives - Rectangle
// ============================================================================

void fb_rect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, fb_color_t color, bool filled) {
    // Ensure x1 <= x2 and y1 <= y2
    if (x1 > x2) swap_int16(&x1, &x2);
    if (y1 > y2) swap_int16(&y1, &y2);

    // Clamp to screen bounds
    x1 = MAX(x1, 0);
    y1 = MAX(y1, 0);
    x2 = MIN(x2, FB_WIDTH - 1);
    y2 = MIN(y2, FB_HEIGHT - 1);

    if (filled) {
        // Fill rectangle
        for (int16_t y = y1; y <= y2; y++) {
            for (int16_t x = x1; x <= x2; x++) {
                fb.pixels[y][x] = color;
            }
        }
    } else {
        // Draw outline
        // Top and bottom edges
        for (int16_t x = x1; x <= x2; x++) {
            fb.pixels[y1][x] = color;
            fb.pixels[y2][x] = color;
        }
        // Left and right edges
        for (int16_t y = y1; y <= y2; y++) {
            fb.pixels[y][x1] = color;
            fb.pixels[y][x2] = color;
        }
    }

    fb_mark_dirty_region(x1, y1, x2, y2);
}

void fb_rect_hsv(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    fb_rect(x1, y1, x2, y2, fb_hsv_to_rgb565(hue, sat, val), filled);
}

// ============================================================================
// Drawing Primitives - Circle
// ============================================================================

void fb_circle(int16_t x0, int16_t y0, uint16_t radius, fb_color_t color, bool filled) {
    // Midpoint circle algorithm
    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;

    while (x >= y) {
        if (filled) {
            // Draw horizontal lines for filled circle
            fb_line(x0 - x, y0 + y, x0 + x, y0 + y, color);
            fb_line(x0 - x, y0 - y, x0 + x, y0 - y, color);
            fb_line(x0 - y, y0 + x, x0 + y, y0 + x, color);
            fb_line(x0 - y, y0 - x, x0 + y, y0 - x, color);
        } else {
            // Draw outline points (8-way symmetry)
            fb_set_pixel(x0 + x, y0 + y, color);
            fb_set_pixel(x0 + y, y0 + x, color);
            fb_set_pixel(x0 - y, y0 + x, color);
            fb_set_pixel(x0 - x, y0 + y, color);
            fb_set_pixel(x0 - x, y0 - y, color);
            fb_set_pixel(x0 - y, y0 - x, color);
            fb_set_pixel(x0 + y, y0 - x, color);
            fb_set_pixel(x0 + x, y0 - y, color);
        }

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void fb_circle_hsv(int16_t x, int16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    fb_circle(x, y, radius, fb_hsv_to_rgb565(hue, sat, val), filled);
}

// ============================================================================
// Drawing Primitives - Ellipse
// ============================================================================

void fb_ellipse(int16_t x0, int16_t y0, uint16_t rx, uint16_t ry, fb_color_t color, bool filled) {
    // Bresenham's ellipse algorithm
    int16_t x = 0;
    int16_t y = ry;

    // Initial decision parameters
    int32_t rx_sq = rx * rx;
    int32_t ry_sq = ry * ry;
    int32_t two_rx_sq = 2 * rx_sq;
    int32_t two_ry_sq = 2 * ry_sq;
    int32_t p;
    int32_t px = 0;
    int32_t py = two_rx_sq * y;

    // Region 1
    p = ry_sq - (rx_sq * ry) + (rx_sq / 4);

    while (px < py) {
        x++;
        px += two_ry_sq;

        if (p < 0) {
            p += ry_sq + px;
        } else {
            y--;
            py -= two_rx_sq;
            p += ry_sq + px - py;
        }

        if (filled) {
            fb_line(x0 - x, y0 + y, x0 + x, y0 + y, color);
            fb_line(x0 - x, y0 - y, x0 + x, y0 - y, color);
        } else {
            fb_set_pixel(x0 + x, y0 + y, color);
            fb_set_pixel(x0 - x, y0 + y, color);
            fb_set_pixel(x0 + x, y0 - y, color);
            fb_set_pixel(x0 - x, y0 - y, color);
        }
    }

    // Region 2
    p = ry_sq * (x + 1) * (x + 1) + rx_sq * (y - 1) * (y - 1) - rx_sq * ry_sq;

    while (y >= 0) {
        y--;
        py -= two_rx_sq;

        if (p > 0) {
            p += rx_sq - py;
        } else {
            x++;
            px += two_ry_sq;
            p += rx_sq - py + px;
        }

        if (filled) {
            fb_line(x0 - x, y0 + y, x0 + x, y0 + y, color);
            fb_line(x0 - x, y0 - y, x0 + x, y0 - y, color);
        } else {
            fb_set_pixel(x0 + x, y0 + y, color);
            fb_set_pixel(x0 - x, y0 + y, color);
            fb_set_pixel(x0 + x, y0 - y, color);
            fb_set_pixel(x0 - x, y0 - y, color);
        }
    }
}

void fb_ellipse_hsv(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t hue, uint8_t sat, uint8_t val, bool filled) {
    fb_ellipse(x, y, rx, ry, fb_hsv_to_rgb565(hue, sat, val), filled);
}
