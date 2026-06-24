/* mm_port.c — HamsterOS coordinator for MM_HamsterOS_Port
 *
 * Phase 1 skeleton: opens a window and draws the game title.
 * Work through the phases in CLAUDE.md to complete the port.
 *
 * All state is static (BSS, zeroed at load time).
 * All OS calls go through the stubs declared below (which call g_host).
 */

#include "../../../HamsterOS/kernel/app_abi.h"
#include "../../../HamsterOS/apps/window.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Stub declarations (implemented in mm_stubs.c) */
extern void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c);
extern void draw_text_bold(int16_t x, int16_t y, const char *s, uint8_t c, uint8_t sc);
extern void draw_border(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c);
extern void blit_opaque_pixels(int16_t dx, int16_t dy, const uint8_t *pixels,
                                uint16_t stride, uint16_t sx, uint16_t sy,
                                uint16_t sw, uint16_t sh, uint8_t scale);
extern void present_region(int16_t x, int16_t y, int16_t w, int16_t h);
extern void wnd_init(WindowFrame *f, int16_t x, int16_t y, int16_t w, int16_t h,
                      int16_t mw, int16_t mh);
extern void wnd_draw_frame(const WindowFrame *f, const char *title);
extern void wnd_content_rect(const WindowFrame *f, int16_t inset,
                               int16_t *x, int16_t *y, int16_t *w, int16_t *h);
extern void wnd_display_bounds(const WindowFrame *f,
                                int16_t *x, int16_t *y, int16_t *w, int16_t *h);
extern WindowHit wnd_hit_test(const WindowFrame *f, int16_t x, int16_t y);
extern void wnd_begin_drag(WindowFrame *f, int16_t x, int16_t y);
extern void wnd_begin_resize(WindowFrame *f, int16_t x, int16_t y);
extern bool wnd_update_pointer(WindowFrame *f, int16_t x, int16_t y);
extern void wnd_end_interaction(WindowFrame *f);
extern void serial_str(const char *s);

/* ---- Game window -------------------------------------------------------- */
#define WIN_X       20
#define WIN_Y       20
#define WIN_W       640
#define WIN_H       460
#define WIN_MIN_W   320
#define WIN_MIN_H   240

/* 320x200 software render buffer — pixel-doubled to 640x400 on blit */
#define RENDER_W    320
#define RENDER_H    200

static WindowFrame  g_frame;
static bool         g_open;
static bool         g_active;
static uint8_t      g_render_buf[RENDER_W * RENDER_H]; /* ~64 KB in BSS */

/* ---- App lifecycle ------------------------------------------------------ */

void mm_port_init(void)
{
    wnd_init(&g_frame, WIN_X, WIN_Y, WIN_W, WIN_H, WIN_MIN_W, WIN_MIN_H);
}

void mm_port_open(void)
{
    g_open   = true;
    g_active = true;
    serial_str("MM_HamsterOS_Port: open\n");
    /* TODO Phase 3: load game data from C:/DOS/MM/ here */
}

void mm_port_close(void)
{
    g_open   = false;
    g_active = false;
    /* TODO: free all heap_alloc buffers here */
    serial_str("MM_HamsterOS_Port: closed\n");
}

bool mm_port_is_open(void)    { return g_open; }
bool mm_port_is_active(void)  { return g_active; }
bool mm_port_has_modal(void)  { return false; /* TODO: return true when dialog is open */ }
void mm_port_set_active(bool a) { g_active = a; }

void mm_port_bounds(int16_t *x, int16_t *y, int16_t *w, int16_t *h)
{
    wnd_display_bounds(&g_frame, x, y, w, h);
}

/* ---- Rendering ---------------------------------------------------------- */

void mm_port_draw(void)
{
    int16_t cx, cy, cw, ch;
    int16_t bx, by, bw, bh;

    if (!g_open) return;

    wnd_draw_frame(&g_frame, "MIGHT & MAGIC I");
    wnd_content_rect(&g_frame, 0, &cx, &cy, &cw, &ch);

    /* Phase 1: fill render buffer with test pattern */
    /* TODO Phase 5: replace with render3d + hud drawing */
    for (uint16_t y = 0; y < RENDER_H; y++) {
        for (uint16_t x = 0; x < RENDER_W; x++) {
            /* Simple gradient: sky=1 (blue), ground=2 (green) */
            g_render_buf[y * RENDER_W + x] =
                (y < RENDER_H / 2) ? 1 : 2;
        }
    }
    /* Title text in the center */
    /* (blit_opaque_pixels first, then draw_text_bold on top) */

    blit_opaque_pixels(cx, cy, g_render_buf,
                        RENDER_W,          /* stride */
                        0, 0, RENDER_W, RENDER_H,
                        2);                /* scale=2 → 640x400 */

    draw_text_bold((int16_t)(cx + cw/2 - 60), (int16_t)(cy + ch/2 - 8),
                   "MIGHT & MAGIC I", 15 /* WHITE */, 1);

    wnd_display_bounds(&g_frame, &bx, &by, &bw, &bh);
    present_region(bx, by, bw, bh);
}

bool mm_port_update(void)
{
    /* TODO Phase 7: handle animation ticks (combat, door open, etc.)
     * Return true only when game state changed and a redraw is needed. */
    return false;
}

/* ---- Input -------------------------------------------------------------- */

bool mm_port_scancode(uint8_t sc)
{
    if (!g_open) return false;

    /* TODO Phase 4: map scancodes to game actions (see CLAUDE.md table) */
    switch (sc) {
        case 0x01: /* ESC */ mm_port_close(); return true;
        default: break;
    }
    return false;
}

bool mm_port_ptr_down(int16_t x, int16_t y)
{
    if (!g_open) return false;
    WindowHit hit = wnd_hit_test(&g_frame, x, y);
    switch (hit) {
        case WINDOW_HIT_TITLE:   wnd_begin_drag(&g_frame, x, y);   return true;
        case WINDOW_HIT_RESIZE:  wnd_begin_resize(&g_frame, x, y); return true;
        case WINDOW_HIT_CLOSE:   mm_port_close();                  return true;
        case WINDOW_HIT_BODY:    /* TODO: game click */             return true;
        default: break;
    }
    return false;
}

bool mm_port_ptr_move(int16_t x, int16_t y)
{
    if (!g_open) return false;
    return wnd_update_pointer(&g_frame, x, y);
}

bool mm_port_ptr_up(int16_t x, int16_t y)
{
    if (!g_open) return false;
    wnd_end_interaction(&g_frame);
    return true;
}
