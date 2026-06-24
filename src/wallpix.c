#include "wallpix.h"
#include "rle.h"
#include "mm_util.h"
#include <stddef.h>

uint8_t *wall_sprites[WALL_TYPES][WALL_SPRITES];

const int SPRITE_W[WALL_SPRITES] = { 32, 40, 24, 16,  32, 40, 24, 16,  176, 96, 48, 16 };
const int SPRITE_H[WALL_SPRITES] = {128, 96, 64, 32, 128, 96, 64, 32,   96, 64, 32, 16 };

/* Heap alloc/free exposed by mm_stubs.c */
extern void *heap_alloc(uint32_t size);
extern void  heap_free(void *ptr);

int wallpix_load(const uint8_t *data, uint32_t data_size)
{
    int w, s;

    if (!data || data_size < 74) return -1;

    /* Header: 2-byte header_size, then 18 x uint32-LE offsets.
     * Data starts at byte 74 = 2 + 72. */
    const int data_start = 74;

    uint32_t offsets[WALL_TYPES];
    for (w = 0; w < WALL_TYPES; w++) {
        int base = 2 + w * 4;
        offsets[w] = (uint32_t)data[base]         |
                     ((uint32_t)data[base+1] <<  8) |
                     ((uint32_t)data[base+2] << 16) |
                     ((uint32_t)data[base+3] << 24);
    }

    mm_memset(wall_sprites, 0, sizeof(wall_sprites));

    for (w = 0; w < WALL_TYPES; w++) {
        int entry_start = data_start + (int)offsets[w];
        if (entry_start + 2 > (int)data_size) continue;

        int stream_pos = entry_start + 2;

        for (s = 0; s < WALL_SPRITES; s++) {
            int W = SPRITE_W[s];
            int H = SPRITE_H[s];
            int raw_size = (W / 4) * H;  /* 2bpp packed bytes */
            uint8_t *raw = (uint8_t *)heap_alloc((uint32_t)raw_size);
            if (!raw) continue;

            stream_pos = rle_decode_stream(data, (int)data_size,
                                           stream_pos, raw, W, H);
            wall_sprites[w][s] = raw;
        }
    }
    return 0;
}

void wallpix_free(void)
{
    int w, s;
    for (w = 0; w < WALL_TYPES; w++) {
        for (s = 0; s < WALL_SPRITES; s++) {
            if (wall_sprites[w][s]) {
                heap_free(wall_sprites[w][s]);
                wall_sprites[w][s] = NULL;
            }
        }
    }
}