#pragma once
#include <stdint.h>
#include "mazedata.h"
#include "game.h"
#include "wallpix.h"

/* First-person 3D renderer — HamsterOS port.
 * Renders into a uint8_t pixel buffer (1 byte = EGA color index 0-15).
 * Viewport: R3D_VW x R3D_VH pixels, placed at (buf_ox, buf_oy) in the buffer. */

#define R3D_VW 240
#define R3D_VH 132

#define R3D_NUM_WALL_TYPES 18
#define R3D_NUM_SUBSPRITES 12

#define SP_LEFT1  0
#define SP_LEFT2  1
#define SP_LEFT3  2
#define SP_LEFT4  3
#define SP_RIGHT1 4
#define SP_RIGHT2 5
#define SP_RIGHT3 6
#define SP_RIGHT4 7
#define SP_MID1   8
#define SP_MID2   9
#define SP_MID3   10
#define SP_MID4   11

typedef struct { int w, h; } SubSpriteSize;
extern const SubSpriteSize R3D_SPRITE_SIZES[R3D_NUM_SUBSPRITES];

typedef struct { int x, y; } SpritePos;
extern const SpritePos R3D_SPRITE_POS[R3D_NUM_SUBSPRITES];

/* Render the first-person view.
 * buf/stride: the render buffer and its row stride.
 * buf_ox/buf_oy: top-left corner of the viewport within the buffer.
 * wt may be NULL (draws background only). */
void render_3d_view(uint8_t *buf, uint16_t stride, int buf_ox, int buf_oy,
                    const struct Map *map, int px, int py,
                    int facing, int outdoor, int dark_mode);