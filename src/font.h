#pragma once
#include <stdint.h>

/* IBM PC 8x8 CP437 bitmap font — HamsterOS render-buffer edition.
 * Renders directly into a uint8_t pixel buffer (1 byte = EGA color index 0-15).
 * No SDL2 dependency. */

#define FONT_W      8
#define FONT_H      8
#define FONT_CHAR_W FONT_W
#define FONT_CHAR_H FONT_H

/* The raw font bitmap: font_data[c][row] — MSB = leftmost pixel. */
extern const uint8_t font_data[256][8];

/* Render one character at pixel (sx, sy) into buf.
 * buf   : render buffer (1 byte per pixel, EGA color index)
 * stride: bytes per row in buf (typically RENDER_W)
 * c     : character (CP437)
 * color : EGA color index 0-15
 * scale : pixel magnification (1 = 8x8 output pixels, 2 = 16x16, ...) */
void font_putchar(uint8_t *buf, uint16_t stride,
                  unsigned char c, int sx, int sy,
                  uint8_t color, int scale);

/* Render a NUL-terminated string. */
void font_print(uint8_t *buf, uint16_t stride,
                const char *s, int sx, int sy,
                uint8_t color, int scale);

/* Returns pixel width of string s at given scale. */
int font_str_width(const char *s, int scale);
