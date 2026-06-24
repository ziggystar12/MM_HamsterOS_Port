#pragma once
#include <stdint.h>
/* Decode SCREEN[screen_idx] from in-memory buffer into render_buf.
 * stride = RENDER_W (320). Fills 320x200 pixels. */
void screen_decode(const uint8_t *data, uint32_t data_size,
                   int screen_idx,
                   uint8_t *render_buf, uint16_t stride);