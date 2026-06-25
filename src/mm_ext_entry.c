/* mm_ext_entry.c — HamsterOS app entry for MM_HamsterOS_Port
 * Shape is always the same; see ../HamsterOS/apps/piano_ext_entry.c */

#include "app_abi.h"
#include <stddef.h>

void mm_stubs_set_host(HamsterHostAPI *h);
void mm_port_init(void);
void mm_port_open(void);
void mm_port_open_file_at(FatDrive drive, uint32_t cluster, const char *name);
void mm_port_close(void);
bool mm_port_is_open(void);
bool mm_port_is_active(void);
bool mm_port_has_modal(void);
void mm_port_set_active(bool a);
bool mm_port_scancode(uint8_t sc);
bool mm_port_ptr_down(int16_t x, int16_t y);
bool mm_port_ptr_move(int16_t x, int16_t y);
bool mm_port_ptr_up(int16_t x, int16_t y);
void mm_port_draw(void);
bool mm_port_update(void);
void mm_port_bounds(int16_t *x, int16_t *y, int16_t *w, int16_t *h);

static AppDescriptor g_desc = {
    mm_port_init, mm_port_open,
    NULL, mm_port_open_file_at, mm_port_close,
    mm_port_is_open, mm_port_is_active, mm_port_has_modal, mm_port_set_active,
    mm_port_scancode,
    mm_port_ptr_down, mm_port_ptr_move, mm_port_ptr_up,
    NULL, NULL,
    mm_port_draw, NULL,
    mm_port_bounds, NULL, NULL,
    NULL, NULL,
    mm_port_update, NULL
};

AppDescriptor *app_entry(HamsterHostAPI *host) {
    mm_stubs_set_host(host);
    return &g_desc;
}
