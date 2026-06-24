#include "render3d.h"
#include "mm_util.h"

const SubSpriteSize R3D_SPRITE_SIZES[R3D_NUM_SUBSPRITES] = {
    { 32,128},{ 40, 96},{ 24, 64},{ 16, 32},
    { 32,128},{ 40, 96},{ 24, 64},{ 16, 32},
    {176, 96},{ 96, 64},{ 48, 32},{ 16, 16},
};

const SpritePos R3D_SPRITE_POS[R3D_NUM_SUBSPRITES] = {
    {  0,  0},{ 32, 16},{ 72, 32},{ 96, 48},
    {208,  0},{168, 16},{144, 32},{128, 48},
    { 32, 16},{ 72, 32},{ 96, 48},{112, 56},
};

/* ---- Region -> wall sprite-set index ---- */
static const uint8_t s_region_d0[55] = {
    0,0,0,0,0, 3,3,3,3,3,3,3,3,3,
    6,6,6,6, 6,6,6,8, 6,6,6,8, 8,8,8,8, 8,8,8,8,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const uint8_t s_region_d1[55] = {
    1,1,1,1,1, 4,4,4,4,4,4,4,4,0,
    13,13,7,7, 13,13,7,7, 7,7,7,7, 7,7,7,10, 7,7,7,7,
    17,17,17,17,17,17, 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
};
static const uint8_t s_region_d2[55] = {
    2,2,2,2,2, 5,5,5,5,5,5,5,5,1,
    12,11,11,11, 12,12,11,11, 8,8,8,11, 9,9,10,11, 9,9,12,10,
    1,16,16,16,16, 1,1, 14,14,14,14,14,14,14,14,14,14,14,14,14,1
};

static int region_wall_id(int region_id, int wall_type)
{
    if (region_id < 0 || region_id >= 55) return 0;
    if (wall_type <= 0) return -1;
    int idx;
    switch (wall_type) {
    case 1: idx = s_region_d0[region_id]; break;
    case 2: idx = s_region_d1[region_id]; break;
    default: idx = s_region_d2[region_id]; break;
    }
    return idx % R3D_NUM_WALL_TYPES;
}

/* ---- Map helpers ---- */
static int _wall_at(const struct Map *map, int x, int y, int dir)
{
    if (x < 0 || x >= 16 || y < 0 || y >= 16) return WT_WALL;
    return map->cells[y][x].wall[dir];
}
static int _is_blocked(const struct Map *map, int x, int y, int dir)
{
    if (x < 0 || x >= 16 || y < 0 || y >= 16) return 1;
    return !map->cells[y][x].pass[dir];
}

/* Per-wall 4-color palette from CLAUDE.md TILE_COLORS[18].
 * Each byte: lo nibble = color for 2bpp index 1, hi nibble = color for index 2.
 * Index 0 = transparent (0), index 3 = white (15). */
static const uint8_t TILE_COLORS[18] = {
    0xe6,0xe6,0xe6, 0x72,0x72,0x72, 0x62,0x62,0x62,
    0x62,0x62,0xe1, 0x53,0x53,0xff, 0x43,0x43,0x63
};
static uint8_t wall_pal(int wall_id, int idx) {
    if (wall_id < 0 || wall_id >= 18) wall_id = 0;
    switch (idx) {
    case 0: return 0;
    case 1: return (uint8_t)(TILE_COLORS[wall_id] & 0x0Fu);
    case 2: return (uint8_t)((TILE_COLORS[wall_id] >> 4) & 0x0Fu);
    case 3: return 15;
    }
    return 0;
}

/* ---- Pixel blitter for full sprite ---- */
static void _blit(uint8_t *buf, uint16_t stride, int ox, int oy,
                  const uint8_t *raw, int W, int H,
                  int dest_x, int dest_y, int fake, int wid)
{
    int cols = W / 4;
    int row, b, p;
    for (row = 0; row < H; row++) {
        int py = dest_y + row;
        if (py < 0 || py >= R3D_VH) continue;
        for (b = 0; b < cols; b++) {
            uint8_t byte_val = raw[row * cols + b];
            for (p = 0; p < 4; p++) {
                uint8_t ega = wall_pal(wid, (byte_val >> (6 - p*2)) & 3);
                if (!ega) continue;
                int px = dest_x + b*4 + p;
                if (px < 0 || px >= R3D_VW) continue;
                if (fake && ((px + py) & 1)) continue;
                buf[(oy + py) * stride + (ox + px)] = ega;
            }
        }
    }
}

/* ---- Partial column blit (for partial side-wall reveals) ----
 * src_byte: starting byte column in the 2bpp row
 * n_bytes:  number of byte columns to copy (each = 4 pixels) */
static void _blit_cols(uint8_t *buf, uint16_t stride, int ox, int oy,
                       const uint8_t *raw, int W, int H,
                       int dest_x, int dest_y,
                       int src_byte, int n_bytes, int fake, int wid)
{
    int total_cols = W / 4;
    int row, b, p;
    for (row = 0; row < H; row++) {
        int py = dest_y + row;
        if (py < 0 || py >= R3D_VH) continue;
        const uint8_t *src_row = raw + row * total_cols + src_byte;
        for (b = 0; b < n_bytes; b++) {
            if (src_byte + b >= total_cols) break;
            uint8_t byte_val = src_row[b];
            for (p = 0; p < 4; p++) {
                uint8_t ega = wall_pal(wid, (byte_val >> (6 - p*2)) & 3);
                if (!ega) continue;
                int px = dest_x + b*4 + p;
                if (px < 0 || px >= R3D_VW) continue;
                if (fake && ((px + py) & 1)) continue;
                buf[(oy + py) * stride + (ox + px)] = ega;
            }
        }
    }
}

/* ---- Background ---- */
static void _draw_background(uint8_t *buf, uint16_t stride, int ox, int oy, int outdoor)
{
    int row, col;
    if (outdoor) {
        /* Sky: dark blue -> cyan */
        static const struct { int y0, y1; uint8_t c; } sky[] = {
            {  0, 20, 1  },  /* EGA blue */
            { 20, 40, 9  },  /* EGA bright blue */
            { 40, 55, 11 },  /* EGA bright cyan */
            { 55, 66, 3  },  /* EGA cyan */
        };
        static const struct { int y0, y1; uint8_t c; } gnd[] = {
            { 66, 80, 6  },  /* EGA brown */
            { 80,132, 2  },  /* EGA green */
        };
        int i;
        for (i = 0; i < 4; i++)
            for (row = sky[i].y0; row < sky[i].y1; row++)
                mm_memset(buf + (oy+row)*stride + ox, sky[i].c, R3D_VW);
        for (i = 0; i < 2; i++)
            for (row = gnd[i].y0; row < gnd[i].y1; row++)
                mm_memset(buf + (oy+row)*stride + ox, gnd[i].c, R3D_VW);
    } else {
        /* Dungeon: ceiling gradient + black floor */
        static const struct { int y0, y1; uint8_t c; } bands[] = {
            {  0, 24, 0 },  /* black */
            { 24, 56, 8 },  /* dark gray */
            { 56, 66, 7 },  /* light gray (near horizon) */
            { 66,132, 0 },  /* black floor */
        };
        int i;
        for (i = 0; i < 4; i++)
            for (row = bands[i].y0; row < bands[i].y1; row++)
                mm_memset(buf + (oy+row)*stride + ox, bands[i].c, R3D_VW);
    }
    (void)col;
}

/* ---- Draw command queue ---- */
#define MAX_DRAW_CMDS 64
typedef struct {
    int wall_id, sprite_idx;
    int dest_x, dest_y;
    int fake;
    int depth;
} DrawCmd;

/* ---- Main renderer ---- */
void render_3d_view(uint8_t *buf, uint16_t stride, int ox, int oy,
                    const struct Map *map, int px, int py,
                    int facing, int outdoor, int dark_mode)
{
    _draw_background(buf, stride, ox, oy, outdoor);
    if (!map) return;

    int region = map->region_id;

    static const int VIEW_ARR2[4]  = { 6, 4, 2, 1};
    static const int VIEW_ARR4[4]  = { 4, 9, 6, 2};
    static const int VIEW_ARR5[4]  = { 4, 5, 3, 2};
    static const int VIEW_ARR7[4]  = { 0, 8,18,24};
    static const int VIEW_ARR8[4]  = { 0, 0,12,24};
    static const int VIEW_ARR10[4] = {52,42,36,32};
    static const int VIEW_ARR15[4] = {36, 4, 0, 0};
    static const int VIEW_ARR16[4] = {36,14, 6, 0};

    int draw_flags[10];
    mm_memset(draw_flags, 0, sizeof(draw_flags));

    DrawCmd cmds[MAX_DRAW_CMDS];
    int n_cmds = 0;

    int right_dir = TURN_R[facing];
    int d;

    for (d = 0; d < 4; d++) {
        int cx = px + FDX[facing] * d;
        int cy = py + FDY[facing] * d;
        int left_dir = TURN_L[facing];
        int lx = cx + FDX[left_dir];
        int ly = cy + FDY[left_dir];

        /* ---- LEFT WALL ---- */
        int left_wall = _wall_at(map, cx, cy, left_dir);
        int left_fake = (left_wall != WT_NONE) && !_is_blocked(map, cx, cy, left_dir);

        if (left_wall != WT_NONE) {
            int wid = region_wall_id(region, left_wall);
            int sp_idx = SP_LEFT1 + d;
            if (wid >= 0 && sp_idx <= SP_LEFT4 && n_cmds < MAX_DRAW_CMDS) {
                cmds[n_cmds].wall_id    = wid;
                cmds[n_cmds].sprite_idx = sp_idx;
                cmds[n_cmds].dest_x     = R3D_SPRITE_POS[sp_idx].x;
                cmds[n_cmds].dest_y     = R3D_SPRITE_POS[sp_idx].y;
                cmds[n_cmds].fake       = left_fake;
                cmds[n_cmds].depth      = d;
                n_cmds++;
            }
            draw_flags[d + 1] = 1;
        } else {
            int lfw  = _wall_at(map, lx, ly, facing);
            int lf_fake = (lfw != WT_NONE) && !_is_blocked(map, lx, ly, facing);
            if (lfw != WT_NONE) {
                draw_flags[d + 1] = 1;
                int wid = region_wall_id(region, lfw);
                int mid_idx = SP_MID1 + d;
                if (wid >= 0 && wall_sprites[wid][mid_idx]) {
                    int src_byte, n_bytes, dst_px;
                    if (draw_flags[d]) {
                        src_byte = VIEW_ARR16[d];
                        n_bytes  = VIEW_ARR5[d] * 2;
                        dst_px   = VIEW_ARR7[d] * 4;
                    } else {
                        src_byte = VIEW_ARR15[d];
                        n_bytes  = VIEW_ARR4[d] * 2;
                        dst_px   = VIEW_ARR8[d] * 4;
                    }
                    int dst_py = (8 - VIEW_ARR2[d]) * 8;
                    _blit_cols(buf, stride, ox, oy,
                               wall_sprites[wid][mid_idx],
                               SPRITE_W[mid_idx], SPRITE_H[mid_idx],
                               dst_px, dst_py, src_byte, n_bytes, lf_fake, wid);
                }
            }
        }

        /* ---- RIGHT WALL ---- */
        int rx = cx + FDX[right_dir];
        int ry = cy + FDY[right_dir];
        int right_wall = _wall_at(map, cx, cy, right_dir);
        int right_fake = (right_wall != WT_NONE) && !_is_blocked(map, cx, cy, right_dir);
        (void)rx; (void)ry;

        if (right_wall != WT_NONE) {
            int wid = region_wall_id(region, right_wall);
            int sp_idx = SP_RIGHT1 + d;
            if (wid >= 0 && sp_idx <= SP_RIGHT4 && n_cmds < MAX_DRAW_CMDS) {
                cmds[n_cmds].wall_id    = wid;
                cmds[n_cmds].sprite_idx = sp_idx;
                cmds[n_cmds].dest_x     = R3D_SPRITE_POS[sp_idx].x;
                cmds[n_cmds].dest_y     = R3D_SPRITE_POS[sp_idx].y;
                cmds[n_cmds].fake       = right_fake;
                cmds[n_cmds].depth      = d;
                n_cmds++;
            }
            draw_flags[d + 6] = 1;
        } else {
            int rfw  = _wall_at(map, rx, ry, facing);
            int rf_fake = (rfw != WT_NONE) && !_is_blocked(map, rx, ry, facing);
            if (rfw != WT_NONE) {
                draw_flags[d + 6] = 1;
                int wid = region_wall_id(region, rfw);
                int mid_idx = SP_MID1 + d;
                if (wid >= 0 && wall_sprites[wid][mid_idx]) {
                    int n_bytes = (draw_flags[d+5] ? VIEW_ARR5[d] : VIEW_ARR4[d]) * 2;
                    int dst_px  = VIEW_ARR10[d] * 4;
                    int dst_py  = (8 - VIEW_ARR2[d]) * 8;
                    _blit_cols(buf, stride, ox, oy,
                               wall_sprites[wid][mid_idx],
                               SPRITE_W[mid_idx], SPRITE_H[mid_idx],
                               dst_px, dst_py, 0, n_bytes, rf_fake, wid);
                }
            }
        }

        /* ---- FRONT WALL ---- */
        int front_wall = _wall_at(map, cx, cy, facing);
        int front_fake = (front_wall != WT_NONE) && !_is_blocked(map, cx, cy, facing);

        if (front_wall != WT_NONE) {
            int wid = region_wall_id(region, front_wall);
            int sp_idx = SP_MID1 + d;
            if (wid >= 0 && sp_idx <= SP_MID4 && n_cmds < MAX_DRAW_CMDS) {
                cmds[n_cmds].wall_id    = wid;
                cmds[n_cmds].sprite_idx = sp_idx;
                cmds[n_cmds].dest_x     = R3D_SPRITE_POS[sp_idx].x;
                cmds[n_cmds].dest_y     = R3D_SPRITE_POS[sp_idx].y;
                cmds[n_cmds].fake       = front_fake;
                cmds[n_cmds].depth      = d;
                n_cmds++;
            }
            if (!front_fake) break;
        }
    }

    /* Render back-to-front */
    {
        int pass, i;
        for (pass = 3; pass >= 0; pass--) {
            for (i = 0; i < n_cmds; i++) {
                if (cmds[i].depth != pass) continue;
                int wid = cmds[i].wall_id;
                int sp  = cmds[i].sprite_idx;
                if (wall_sprites[wid][sp]) {
                    _blit(buf, stride, ox, oy,
                          wall_sprites[wid][sp],
                          SPRITE_W[sp], SPRITE_H[sp],
                          cmds[i].dest_x, cmds[i].dest_y,
                          cmds[i].fake, wid);
                }
            }
        }
    }

    /* Dark mode: fill viewport with black */
    if (dark_mode) {
        int row;
        for (row = 0; row < R3D_VH; row++)
            mm_memset(buf + (oy+row)*stride + ox, 0, (uint32_t)R3D_VW);
    }
}