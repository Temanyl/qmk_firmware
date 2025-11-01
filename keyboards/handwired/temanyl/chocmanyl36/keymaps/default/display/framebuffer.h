/*
 * Framebuffer System for chocmanyl36
 *
 * Provides an in-memory framebuffer for the ST7789 240x135 display
 * Using RGB565 format (16-bit color)
 *
 * Memory usage: 135 x 240 x 2 bytes = 64,800 bytes (~63 KB)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "qp.h"

// Display dimensions (portrait mode)
#define FB_WIDTH  135
#define FB_HEIGHT 240

// Screen split for hybrid rendering
// Upper region: Framebuffer (logo, scenic animations)
// Lower region: QP direct (date, time, media, volume)
#define FB_SPLIT_Y 155  // Split at y=155

// RGB565 color type
typedef uint16_t fb_color_t;

// Framebuffer structure
typedef struct {
    fb_color_t pixels[FB_HEIGHT][FB_WIDTH];
} framebuffer_t;

// Global framebuffer instance
extern framebuffer_t fb;

// Background buffer for storing original scene (without animated elements)
// Used for efficient raindrop animation - restore old positions from here
extern framebuffer_t fb_background;

// ============================================================================
// Color Conversion Functions
// ============================================================================

/**
 * Convert HSV to RGB565
 * @param hue 0-255
 * @param sat 0-255
 * @param val 0-255
 * @return RGB565 color value
 */
fb_color_t fb_hsv_to_rgb565(uint8_t hue, uint8_t sat, uint8_t val);

/**
 * Convert RGB888 to RGB565
 * @param r 0-255
 * @param g 0-255
 * @param b 0-255
 * @return RGB565 color value
 */
fb_color_t fb_rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);

/**
 * Convert RGB565 to RGB888
 * @param color RGB565 color value
 * @param r Pointer to store red component (0-255)
 * @param g Pointer to store green component (0-255)
 * @param b Pointer to store blue component (0-255)
 */
void fb_rgb565_to_rgb888(fb_color_t color, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * Convert RGB565 to HSV
 * @param color RGB565 color value
 * @param h Pointer to store hue (0-255)
 * @param s Pointer to store saturation (0-255)
 * @param v Pointer to store value (0-255)
 */
void fb_rgb565_to_hsv(fb_color_t color, uint8_t *h, uint8_t *s, uint8_t *v);

// ============================================================================
// Core Framebuffer Functions
// ============================================================================

/**
 * Initialize the framebuffer (clears to black)
 */
void fb_init(void);

/**
 * Clear the entire framebuffer to a color
 * @param color RGB565 color value
 */
void fb_clear(fb_color_t color);

/**
 * Flush the framebuffer to the physical display
 * Renders the entire framebuffer region (y=0 to FB_SPLIT_Y-1)
 * @param display The QP display device handle
 */
void fb_flush(painter_device_t display);

/**
 * Flush a specific region of the framebuffer to the physical display
 * Only updates the specified rectangular region for efficiency
 * @param display The QP display device handle
 * @param x1 Left coordinate
 * @param y1 Top coordinate
 * @param x2 Right coordinate
 * @param y2 Bottom coordinate
 */
void fb_flush_region(painter_device_t display, int16_t x1, int16_t y1, int16_t x2, int16_t y2);

/**
 * Flush the entire framebuffer to the physical display (fullscreen mode)
 * Unlike fb_flush(), this bypasses the FB_SPLIT_Y limitation and renders
 * the entire framebuffer to the full screen height (0 to FB_HEIGHT-1)
 * Use this for fullscreen games or animations that need the entire display
 * @param display The QP display device handle
 */
void fb_flush_fullscreen(painter_device_t display);

/**
 * Save current framebuffer to background buffer
 * Used to preserve the base scene before drawing animated elements
 */
void fb_save_to_background(void);

/**
 * Restore a rectangular region from background buffer to main framebuffer
 * Used to "erase" animated elements by restoring the original background
 * @param x1 Left coordinate
 * @param y1 Top coordinate
 * @param x2 Right coordinate
 * @param y2 Bottom coordinate
 */
void fb_restore_from_background(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

// ============================================================================
// Drawing Primitives
// ============================================================================

/**
 * Set a single pixel
 * @param x X coordinate (0-134)
 * @param y Y coordinate (0-239)
 * @param color RGB565 color value
 */
void fb_set_pixel(int16_t x, int16_t y, fb_color_t color);

/**
 * Set a single pixel using HSV color
 * @param x X coordinate
 * @param y Y coordinate
 * @param hue 0-255
 * @param sat 0-255
 * @param val 0-255
 */
void fb_set_pixel_hsv(int16_t x, int16_t y, uint8_t hue, uint8_t sat, uint8_t val);

/**
 * Get the color of a pixel
 * @param x X coordinate
 * @param y Y coordinate
 * @return RGB565 color value (0 if out of bounds)
 */
fb_color_t fb_get_pixel(int16_t x, int16_t y);

/**
 * Get the RGB888 color of a pixel
 * @param x X coordinate
 * @param y Y coordinate
 * @param r Pointer to store red component (0-255)
 * @param g Pointer to store green component (0-255)
 * @param b Pointer to store blue component (0-255)
 * @return true if pixel is in bounds, false otherwise
 */
bool fb_get_pixel_rgb(int16_t x, int16_t y, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * Get the HSV color of a pixel
 * @param x X coordinate
 * @param y Y coordinate
 * @param h Pointer to store hue (0-255)
 * @param s Pointer to store saturation (0-255)
 * @param v Pointer to store value (0-255)
 * @return true if pixel is in bounds, false otherwise
 */
bool fb_get_pixel_hsv(int16_t x, int16_t y, uint8_t *h, uint8_t *s, uint8_t *v);

/**
 * Draw a line from (x1,y1) to (x2,y2)
 * @param x1 Start X
 * @param y1 Start Y
 * @param x2 End X
 * @param y2 End Y
 * @param color RGB565 color
 */
void fb_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, fb_color_t color);

/**
 * Draw a line using HSV color
 */
void fb_line_hsv(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t hue, uint8_t sat, uint8_t val);

/**
 * Draw a rectangle
 * @param x1 Left
 * @param y1 Top
 * @param x2 Right
 * @param y2 Bottom
 * @param color RGB565 color
 * @param filled true to fill, false for outline
 */
void fb_rect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, fb_color_t color, bool filled);

/**
 * Draw a rectangle using HSV color
 */
void fb_rect_hsv(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t hue, uint8_t sat, uint8_t val, bool filled);

/**
 * Draw a circle
 * @param x Center X
 * @param y Center Y
 * @param radius Radius in pixels
 * @param color RGB565 color
 * @param filled true to fill, false for outline
 */
void fb_circle(int16_t x, int16_t y, uint16_t radius, fb_color_t color, bool filled);

/**
 * Draw a circle using HSV color
 */
void fb_circle_hsv(int16_t x, int16_t y, uint16_t radius, uint8_t hue, uint8_t sat, uint8_t val, bool filled);

/**
 * Draw an ellipse
 * @param x Center X
 * @param y Center Y
 * @param rx Horizontal radius
 * @param ry Vertical radius
 * @param color RGB565 color
 * @param filled true to fill, false for outline
 */
void fb_ellipse(int16_t x, int16_t y, uint16_t rx, uint16_t ry, fb_color_t color, bool filled);

/**
 * Draw an ellipse using HSV color
 */
void fb_ellipse_hsv(int16_t x, int16_t y, uint16_t rx, uint16_t ry, uint8_t hue, uint8_t sat, uint8_t val, bool filled);

// ============================================================================
// Utility Macros for Common Colors
// ============================================================================

// RGB565 color format: RRRRRGGGGGGBBBBB (byte-swapped for ST7789)
// Helper macro to byte-swap RGB565 at compile time
#define FB_RGB565_SWAP(c) ((uint16_t)((((c) >> 8) & 0xFF) | (((c) << 8) & 0xFF00)))

#define FB_COLOR_BLACK   FB_RGB565_SWAP(0x0000)
#define FB_COLOR_WHITE   FB_RGB565_SWAP(0xFFFF)
#define FB_COLOR_RED     FB_RGB565_SWAP(0xF800)
#define FB_COLOR_GREEN   FB_RGB565_SWAP(0x07E0)
#define FB_COLOR_BLUE    FB_RGB565_SWAP(0x001F)
#define FB_COLOR_YELLOW  FB_RGB565_SWAP(0xFFE0)
#define FB_COLOR_CYAN    FB_RGB565_SWAP(0x07FF)
#define FB_COLOR_MAGENTA FB_RGB565_SWAP(0xF81F)
#define FB_COLOR_ORANGE  FB_RGB565_SWAP(0xFC00)
#define FB_COLOR_PURPLE  FB_RGB565_SWAP(0x8010)
#define FB_COLOR_TEAL    FB_RGB565_SWAP(0x0410)
#define FB_COLOR_GRAY    FB_RGB565_SWAP(0x8410)
