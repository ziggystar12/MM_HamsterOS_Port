#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MONPIX_NUM_MONSTERS 76
#define MONPIX_W            104
#define MONPIX_H             96
/* 2bpp packed: (W/4)*H = 26*96 = 2496 bytes per sprite */
#define MONPIX_RAW_SIZE     ((MONPIX_W/4)*MONPIX_H)

/* Per-monster 4-color EGA palette word.
 * bits 3-0=color0(transparent), 7-4=color1, 11-8=color2, 15-12=color3 */
extern const uint16_t MONPIX_PAL[MONPIX_NUM_MONSTERS];

/* Load and keep the raw MONPIX.DTA buffer.
 * Returns 0 on success. Keeps data in heap — call monpix_free() to release. */
int  monpix_load(const uint8_t *data, uint32_t data_size);

/* Decode sprite for monster_id (0-75) into out_buf (must be MONPIX_RAW_SIZE bytes).
 * Returns true on success. out_buf contains 2bpp row-major pixels:
 *   index 0 = transparent, index 1-3 = EGA color from MONPIX_PAL[id]. */
bool monpix_decode(int monster_id, uint8_t *out_buf);

/* Blit decoded 2bpp sprite onto render_buf at (dest_x, dest_y).
 * stride = render buffer row stride. Color 0 = transparent. */
void monpix_blit(const uint8_t *sprite_2bpp, int monster_id,
                 uint8_t *buf, uint16_t stride,
                 int dest_x, int dest_y);

/* Free the raw data buffer. */
void monpix_free(void);