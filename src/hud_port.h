#pragma once
#include <stdint.h>
#include "game.h"
#include "mazedata.h"

/* 640x440 render_buf @ scale=1 blit (full screen content area).
 * 3D view is rendered into a separate 240x132 sub-buffer, then blitted
 * at scale=2 by mm_port.c to produce a 480x264 apparent 3D view.
 *
 *   Screen layout (640x440 pixels):
 *   [  0-479,  0-263 ] 3D view  (480x264 — from 240x132 sub-buf at scale=2)
 *   [480-639,  0-199 ] minimap  (160x200)
 *   [480-639,200-263 ] compass  (160x64)
 *   [  0-639,    264 ] separator
 *   [  0-639, 265-281] message  (2 lines x 8px)
 *   [  0-639,    282 ] separator
 *   [  0-639, 283-439] party strip (157px = ~19 rows x 8px) */

#define RENDER_W     640
#define RENDER_H     440

/* 3D sub-buffer (separate, blitted at scale=2 by mm_port.c) */
#define VIEW3D_W     240
#define VIEW3D_H     132

/* HUD regions in 640x440 space */
#define HUD_MINI_X   480
#define HUD_MINI_Y     0
#define HUD_MINI_W   160
#define HUD_MINI_H   200
#define HUD_SIDE_X   480
#define HUD_SIDE_Y   200
#define HUD_SIDE_W   160
#define HUD_SIDE_H    64
#define HUD_SEP1_Y   264
#define HUD_MSG_Y    265
#define HUD_SEP2_Y   282

/* Right panel button layout (for click-hit detection in mm_port.c) */
#define RP_X  481
#define RP_W  159
#define MM_H_MINI 108   /* minimap height */
#define CB_X  481
#define CB_Y  109       /* MM_H_MINI + 1 */
#define CB_W  159
#define CB_H  113
#define CB_BW  53       /* CB_W / 3 */
#define CB_BH  37       /* CB_H / 3 */
#define MB_X  481
#define MB_Y  223       /* CB_Y + CB_H + 1 */
#define MB_W  159
#define MB_H   62
#define MB_BW  53       /* MB_W / 3 */
#define MB_BH  31       /* MB_H / 2 */
#define HUD_PARTY_Y  283

/* Fills the HUD outside the 3D view area (right panel + bottom) */
void hud_draw_chrome(uint8_t *buf);
void hud_draw_minimap(uint8_t *buf, const struct Map *m, const Player *p);
void hud_draw_compass(uint8_t *buf, const Player *p,
                      const GameState *gs, const struct Map *m);
void hud_draw_status(uint8_t *buf, const GameState *gs, const struct Map *m);
void hud_draw_party(uint8_t *buf, const GameState *gs);