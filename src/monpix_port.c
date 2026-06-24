/* monpix_port.c — MONPIX.DTA loader for HamsterOS port.
 * Stores raw compressed data; decodes single sprites on demand.
 * No SDL_Surface — outputs 2bpp EGA pixel arrays. */

#include "monpix_port.h"
#include "hud_port.h"
#include "rle.h"
#include "mm_util.h"
#include <stddef.h>

extern void *heap_alloc(uint32_t size);
extern void  heap_free(void *ptr);

/* Per-monster 4-color EGA palette (from monpix.py) */
const uint16_t MONPIX_PAL[MONPIX_NUM_MONSTERS] = {
    0xf470,0xf420,0xfe20,0xf630,0xf420,0xf620,0xf460,0xf6e0,
    0xf510,0xfe40,0xf420,0xf410,0xfd50,0xfc90,0xf430,0xfc30,
    0xf770,0xfc30,0xf420,0xf430,0xf420,0xf490,0xf110,0xf4e0,
    0xf430,0xfd60,0xf430,0xfc20,0xf2a0,0xf470,0xf4e0,0xf250,
    0xf430,0xf320,0xfee0,0xf420,0xf220,0xf420,0xfdd0,0xf420,
    0xf620,0xfc20,0xfc10,0xf520,0xf420,0xf220,0xf420,0xfa50,
    0xfe20,0xf620,0xf470,0xf420,0xfe10,0xf4e0,0xfe40,0xf140,
    0xf290,0xf410,0xf520,0xf410,0xfc10,0xf120,0xf420,0xfe10,
    0xf520,0xf4a0,0xfe60,0xfe60,0xf620,0xf620,0xfce0,0xf420,
    0xfc20,0xfc20,0xfd90,0xf420,
};

static const uint8_t *s_data      = NULL;
static uint32_t       s_data_size = 0;
static int            s_data_start= 0;
static uint32_t       s_offsets[MONPIX_NUM_MONSTERS];

int monpix_load(const uint8_t *data, uint32_t data_size)
{
    int i;
    if (!data || data_size < 2 + 4*MONPIX_NUM_MONSTERS) return -1;

    uint16_t hdr_size = (uint16_t)(data[0] | (data[1] << 8));
    s_data_start = 2 + (int)hdr_size;

    for (i = 0; i < MONPIX_NUM_MONSTERS; i++) {
        int b = 2 + i*4;
        s_offsets[i] = (uint32_t)(data[b] | (data[b+1]<<8) |
                                   (data[b+2]<<16) | (data[b+3]<<24));
    }
    s_data      = data;
    s_data_size = data_size;
    return 0;
}

bool monpix_decode(int id, uint8_t *out_buf)
{
    if (!s_data || id < 0 || id >= MONPIX_NUM_MONSTERS) return false;

    /* Handle shared sprites (same offset as a previous entry) */
    int i;
    for (i = 0; i < id; i++) {
        if (s_offsets[i] == s_offsets[id]) {
            /* Re-decode from the shared offset */
            id = i; break;
        }
    }

    int abs_off = s_data_start + (int)s_offsets[id];
    if (abs_off + 2 > (int)s_data_size) return false;

    mm_memset(out_buf, 0, MONPIX_RAW_SIZE);
    rle_decode_stream(s_data, (int)s_data_size, abs_off + 2,
                      out_buf, MONPIX_W, MONPIX_H);
    return true;
}

void monpix_blit(const uint8_t *spr, int monster_id,
                 uint8_t *buf, uint16_t stride,
                 int dest_x, int dest_y)
{
    uint16_t pw = MONPIX_PAL[monster_id >= 0 && monster_id < MONPIX_NUM_MONSTERS
                              ? monster_id : 0];
    /* EGA color indices for each 2bpp value (0=transparent) */
    uint8_t pal[4];
    pal[0] = 0;
    pal[1] = (uint8_t)((pw >>  4) & 0xF);
    pal[2] = (uint8_t)((pw >>  8) & 0xF);
    pal[3] = (uint8_t)((pw >> 12) & 0xF);

    int cols = MONPIX_W / 4;
    int row, b, p;
    for (row = 0; row < MONPIX_H; row++) {
        for (b = 0; b < cols; b++) {
            uint8_t byte_val = spr[row * cols + b];
            for (p = 0; p < 4; p++) {
                uint8_t idx = (byte_val >> (6 - p*2)) & 3;
                if (!pal[idx]) continue;   /* transparent */
                int px = dest_x + b*4 + p;
                int py = dest_y + row;
                if (px >= 0 && py >= 0 && px < RENDER_W && py < RENDER_H)
                    buf[py * stride + px] = pal[idx];
            }
        }
    }
}

void monpix_free(void)
{
    s_data      = NULL;
    s_data_size = 0;
}