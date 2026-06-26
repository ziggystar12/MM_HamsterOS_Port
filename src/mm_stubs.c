/* mm_stubs.c — Routes HamsterHostAPI calls for MM_HamsterOS_Port
 * One thin wrapper per OS function used.
 * Reference: ../HamsterOS/apps/piano_stubs.c */

#include "app_abi.h"
#include "fat.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

static HamsterHostAPI *g_host = NULL;
void mm_stubs_set_host(HamsterHostAPI *h) { g_host = h; }

/* Drawing */
void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c)
    { g_host->fill_rect(x, y, w, h, c); }
void draw_text(int16_t x, int16_t y, const char *s, uint8_t c, uint8_t sc)
    { g_host->draw_text(x, y, s, c, sc); }
void draw_text_bold(int16_t x, int16_t y, const char *s, uint8_t c, uint8_t sc)
    { g_host->draw_text_bold(x, y, s, c, sc); }
void draw_border(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c)
    { g_host->draw_border(x, y, w, h, c); }
void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t c)
    { g_host->draw_line(x0, y0, x1, y1, c); }
void blit_opaque_pixels(int16_t dx, int16_t dy, const uint8_t *pixels,
                         uint16_t stride, uint16_t sx, uint16_t sy,
                         uint16_t sw, uint16_t sh, uint8_t scale)
    { g_host->blit_opaque_pixels(dx, dy, pixels, stride, sx, sy, sw, sh, scale); }
void present_region(int16_t x, int16_t y, int16_t w, int16_t h)
    { g_host->present_region(x, y, w, h); }

/* Window manager */
void wnd_init(WindowFrame *f, int16_t x, int16_t y, int16_t w, int16_t h,
              int16_t mw, int16_t mh)
    { g_host->wnd_init(f, x, y, w, h, mw, mh); }
void wnd_draw_frame(const WindowFrame *f, const char *title)
    { g_host->wnd_draw_frame(f, title); }
void wnd_content_rect(const WindowFrame *f, int16_t inset,
                       int16_t *x, int16_t *y, int16_t *w, int16_t *h)
    { g_host->wnd_content_rect(f, inset, x, y, w, h); }
void wnd_display_bounds(const WindowFrame *f,
                         int16_t *x, int16_t *y, int16_t *w, int16_t *h)
    { g_host->wnd_display_bounds(f, x, y, w, h); }
WindowHit wnd_hit_test(const WindowFrame *f, int16_t x, int16_t y)
    { return g_host->wnd_hit_test(f, x, y); }
void wnd_begin_drag(WindowFrame *f, int16_t x, int16_t y)
    { g_host->wnd_begin_drag(f, x, y); }
void wnd_begin_resize(WindowFrame *f, int16_t x, int16_t y)
    { g_host->wnd_begin_resize(f, x, y); }
bool wnd_update_pointer(WindowFrame *f, int16_t x, int16_t y)
    { return g_host->wnd_update_pointer(f, x, y); }
void wnd_end_interaction(WindowFrame *f)
    { g_host->wnd_end_interaction(f); }

/* FAT filesystem */
bool fat_load_file(FatDrive drive, uint32_t dir, const char *name,
                    char *buf, uint32_t max, uint32_t *out_size, bool *trunc)
    { return g_host->fat_load(drive, dir, name, buf, max, out_size, trunc); }
bool fat_save_file(FatDrive drive, uint32_t dir, const char *name,
                    const char *buf, uint32_t size)
    { return g_host->fat_save(drive, dir, name, buf, size); }

/* Memory (use these everywhere instead of malloc/free) */
void *heap_alloc(uint32_t size)  { return g_host->heap_alloc(size); }
void  heap_free(void *ptr)       { g_host->heap_free(ptr); }
/* Aliases for game code: compile with -Dmalloc=mm_malloc -Dfree=mm_free */
void *mm_malloc(uint32_t n)      { return g_host->heap_alloc(n); }
void  mm_free(void *p)           { g_host->heap_free(p); }

/* Timing + audio */
uint32_t pit_millis(void)         { return g_host->pit_millis(); }
void speaker_note_on(uint16_t hz) { g_host->speaker_note_on(hz); }
void speaker_note_off(void)       { g_host->speaker_note_off(); }

/* Debug output → COM1 serial (visible in QEMU terminal) */
void serial_str(const char *s)   { g_host->serial_str(s); }
void serial_dec16(int16_t v)     { g_host->serial_dec16(v); }
void serial_char(char c)         { g_host->serial_char(c); }

/* FAT directory listing — used to validate the launch-folder asset base. */
bool mm_fat_list_dir(FatDrive drive, uint32_t dir, FatRootEntry *entries,
                     uint32_t max, uint32_t start, uint32_t *count,
                     bool *more, const char *ext_filter)
    { return g_host->fat_list_dir(drive, dir, entries, max, start, count, more, ext_filter); }

/* VGA geometry */
int16_t vga_width(void)           { return g_host->vga_width(); }
int16_t vga_height(void)          { return g_host->vga_height(); }

/* Full game mode: hides tray, header bar, suppresses shell keys, disables screensaver */
void set_game_mode(bool g) { g_host->set_game_mode(g); }

/* SB16 audio */
bool sb16_present(void)                              { return g_host->sb16_card_present(); }
void sb16_play_wav(const uint8_t *wav, uint32_t sz)  { g_host->sb16_play_wav_buf(wav, sz); }
void sb16_stop(void)                                 { g_host->sb16_audio_stop(); }

/* FAT batch mode — groups writes to reduce floppy seek overhead */
void fat_batch_begin(FatDrive d) { g_host->fat_begin_batch(d); }
void fat_batch_end(FatDrive d)   { g_host->fat_end_batch(d); }
