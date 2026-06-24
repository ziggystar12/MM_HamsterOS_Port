/* screen_port.c — SCREEN0-9 title screen decoder for HamsterOS.
 * Decodes CGA 2bpp RLE data directly into render_buf. */

#include "screen_port.h"
#include <stdbool.h>
#include "rle.h"
#include <stdbool.h>
#include "mm_util.h"
#include <stdbool.h>

/* SCREEN palette: most screens use EGA 0,2,4,15; screen 2 uses EGA 0,3,5,15 */
static const uint8_t PAL_NORMAL[4] = { 0, 2, 4, 15 };
static const uint8_t PAL_SCREEN2[4] = { 0, 3, 5, 15 };

void screen_decode(const uint8_t *data, uint32_t data_size,
                   int screen_idx,
                   uint8_t *render_buf, uint16_t stride)
{
    /* 2-byte header, then RLE stream */
    if (!data || data_size < 4) return;

    static uint8_t raw[16000];   /* BSS not stack — 16 KB would overflow the app stack */
    mm_memset(raw, 0, sizeof(raw));
    rle_decode_stream(data, (int)data_size, 2, raw, 320, 200);

    const uint8_t *pal = (screen_idx == 2) ? PAL_SCREEN2 : PAL_NORMAL;

    /* Expand 2bpp row-major to 1-byte-per-pixel render_buf */
    int byte_idx, p;
    for (byte_idx = 0; byte_idx < 16000; byte_idx++) {
        uint8_t b = raw[byte_idx];
        int base_px = byte_idx * 4;
        for (p = 0; p < 4; p++) {
            int idx = (b >> (6 - p*2)) & 3;
            int px  = base_px + p;
            if (px < 320*200)
                render_buf[(px / 320) * stride + (px % 320)] = pal[idx];
        }
    }
}