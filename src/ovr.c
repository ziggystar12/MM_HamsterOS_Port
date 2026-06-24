/* ovr.c -- OVR overlay file parser for HamsterOS port (buffer-based). */

#include "ovr.h"
#include "mm_util.h"
#include <stddef.h>

static void default_constants(OvrConstants* c)
{
    mm_memset(c, 0, sizeof(*c));
    c->safe_x = 7;
    c->safe_y = 7;
    c->section_type = 2;
}

static void parse_data_section(OvrFile* ovr, const uint8_t* ds, uint32_t ds_len)
{
    if (ds_len < 51) {
        default_constants(&ovr->constants);
        ovr->num_scripts = 0;
        ovr->num_strings = 0;
        return;
    }

    OvrConstants* c = &ovr->constants;
    c->outdoor        = (ds[0] & 0x80) ? 1 : 0;
    c->section_type   = ds[1];
    c->safe_x         = (ds_len > 23) ? ds[23] : 7;
    c->safe_y         = (ds_len > 24) ? ds[24] : 7;
    c->encounter_rand = (ds_len > 29) ? ds[29] : 0;
    c->wall1_word     = (ds_len > 30) ? (ds[30] < 6 ? ds[30] : 5) : 0;
    c->door_word      = (ds_len > 31) ? (ds[31] < 6 ? ds[31] : 5) : 0;
    c->wall2_word     = (ds_len > 32) ? (ds[32] < 6 ? ds[32] : 5) : 0;
    c->max_mon_level  = (ds_len > 33) ? ds[33] : 0;
    c->max_mon_qty    = (ds_len > 34) ? ds[34] : 0;
    c->danger_rate    = (ds_len > 44) ? ds[44] : 0;
    uint8_t flags     = (ds_len > 45) ? ds[45] : 0;
    c->dark_map       = (flags & 1) ? 1 : 0;
    c->no_teleport    = (flags & 2) ? 1 : 0;

    int num_scripts = (int)ds[50];
    if (num_scripts > OVR_MAX_SCRIPTS) num_scripts = OVR_MAX_SCRIPTS;
    ovr->num_scripts = 0;
    uint32_t xy_base  = 51;
    uint32_t dir_base = xy_base  + (uint32_t)num_scripts;
    uint32_t off_base = dir_base + (uint32_t)num_scripts;
    for (int i = 0; i < num_scripts; i++) {
        uint32_t off = off_base + (uint32_t)i * 2;
        if (xy_base + (uint32_t)i >= ds_len ||
            dir_base + (uint32_t)i >= ds_len ||
            off + 1 >= ds_len) break;
        OvrScript* s   = &ovr->scripts[ovr->num_scripts++];
        uint8_t yx     = ds[xy_base + (uint32_t)i];
        s->x           = yx & 0x0F;
        s->y           = (yx >> 4) & 0x0F;
        s->direction   = ds[dir_base + (uint32_t)i];
        s->code_offset = (uint16_t)(ds[off] | ((uint16_t)ds[off+1] << 8));
    }

    ovr->num_strings = 0;
    uint32_t i = 51 + (uint32_t)num_scripts * 4;
    while (i < ds_len && ovr->num_strings < OVR_MAX_STRINGS) {
        if (ds[i] == 0) { i++; continue; }
        char* dest = ovr->strings[ovr->num_strings];
        int   di   = 0;
        while (i < ds_len && ds[i] != 0 && di < OVR_STRING_MAX - 1) {
            uint8_t ch = ds[i++];
            dest[di++] = (ch < 32) ? ' ' : (char)ch;
        }
        dest[di] = '\0';
        if (i < ds_len && ds[i] == 0) i++;
        while (di > 0 && dest[di-1] == ' ') dest[--di] = '\0';
        int printable = 1;
        for (int j = 0; j < di; j++) {
            if (!mm_isprint((unsigned char)dest[j])) { printable = 0; break; }
        }
        if (di >= 3 && printable)
            ovr->num_strings++;
    }
}

int ovr_load_buf(OvrFile* ovr, const uint8_t *raw, uint32_t fsz)
{
    mm_memset(ovr, 0, sizeof(*ovr));
    if (!raw || fsz < OVR_HEADER_SIZE) {
        default_constants(&ovr->constants);
        return 0;
    }
    uint16_t code_size = (uint16_t)(raw[4] | ((uint16_t)raw[5] << 8));
    uint32_t ds_offset = OVR_HEADER_SIZE + (uint32_t)code_size;
    if (ds_offset >= fsz) {
        default_constants(&ovr->constants);
        return 0;
    }
    parse_data_section(ovr, raw + ds_offset, fsz - ds_offset);
    return 1;
}

static uint8_t facing_mask(int facing)
{
    switch (facing) {
    case 0: return 0xC0;
    case 1: return 0x30;
    case 2: return 0x0C;
    case 3: return 0x03;
    default: return 0x00;
    }
}

int ovr_scripts_at(const OvrFile* ovr, int x, int y, int facing,
                   const OvrScript** out, int max_out)
{
    if (!ovr || !out || max_out <= 0) return 0;
    const OvrScript* all[OVR_MAX_SCRIPTS];
    int n_all = 0, i;
    for (i = 0; i < ovr->num_scripts; i++) {
        const OvrScript* s = &ovr->scripts[i];
        if (s->x == x && s->y == y) all[n_all++] = s;
    }
    if (n_all == 0) return 0;
    if (facing < 0) {
        int cnt = (n_all < max_out) ? n_all : max_out;
        for (i = 0; i < cnt; i++) out[i] = all[i];
        return cnt;
    }
    uint8_t mask = facing_mask(facing);
    const OvrScript* exact[OVR_MAX_SCRIPTS];
    int n_exact = 0;
    for (i = 0; i < n_all; i++) {
        if (all[i]->direction != 0xFF && mask &&
            (all[i]->direction & mask) == mask)
            exact[n_exact++] = all[i];
    }
    if (n_exact > 0) {
        int cnt = (n_exact < max_out) ? n_exact : max_out;
        for (i = 0; i < cnt; i++) out[i] = exact[i];
        return cnt;
    }
    const OvrScript* wild[OVR_MAX_SCRIPTS];
    int n_wild = 0;
    for (i = 0; i < n_all; i++)
        if (all[i]->direction == 0xFF) wild[n_wild++] = all[i];
    if (n_wild > 0) {
        int cnt = (n_wild < max_out) ? n_wild : max_out;
        for (i = 0; i < cnt; i++) out[i] = wild[i];
        return cnt;
    }
    int cnt = (n_all < max_out) ? n_all : max_out;
    for (i = 0; i < cnt; i++) out[i] = all[i];
    return cnt;
}

static int dialog_string_idx(const OvrFile* ovr, int idx, const char** out_str)
{
    int di = 0, i;
    for (i = 0; i < ovr->num_strings; i++) {
        const char* s = ovr->strings[i];
        if ((int)mm_strlen(s) >= 4) {
            if (di == idx) { *out_str = s; return 1; }
            di++;
        }
    }
    return 0;
}

const char* ovr_string_for_script(const OvrFile* ovr, int idx)
{
    const char* s = NULL;
    if (dialog_string_idx(ovr, idx, &s)) return s;
    return "";
}

const char* ovr_text_at(const OvrFile* ovr, int x, int y, int facing)
{
    const OvrScript* scripts[OVR_MAX_SCRIPTS];
    int n = ovr_scripts_at(ovr, x, y, facing, scripts, OVR_MAX_SCRIPTS);
    if (n == 0) return "";
    int idx = (int)(scripts[0] - ovr->scripts);
    return ovr_string_for_script(ovr, idx);
}