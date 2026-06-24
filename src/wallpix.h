#pragma once
#include <stdint.h>

/* WALLPIX.DTA loader — HamsterOS port.
 * Sprites stored as 2bpp packed arrays (no SDL_Surface).
 * Palette: index 0 = transparent, 1 = EGA 2 (green), 2 = EGA 4 (red), 3 = EGA 15 (white).
 * Each wall_sprites[w][s] points to (SPRITE_W[s]/4)*SPRITE_H[s] bytes. */

#define WALL_TYPES   18
#define WALL_SPRITES 12

extern uint8_t *wall_sprites[WALL_TYPES][WALL_SPRITES];

/* Sub-sprite pixel dimensions (index 0-11) */
extern const int SPRITE_W[WALL_SPRITES];
extern const int SPRITE_H[WALL_SPRITES];

/* Load from an in-memory WALLPIX.DTA buffer.
 * Returns 0 on success, -1 on error. */
int  wallpix_load(const uint8_t *data, uint32_t data_size);

/* Free all heap allocations from wallpix_load(). */
void wallpix_free(void);