#pragma once
/*
 * ovr.h -- OVR overlay file parser for MM1 C port.
 *
 * Binary layout (from ovr.py and original source analysis):
 *   Bytes 0-3:   signature / reserved
 *   Bytes 4-5:   code section size (uint16 LE)
 *   Bytes 6-13:  additional header (ignored)
 *   [HEADER_SIZE = 14 bytes total]
 *   Then: code section (code_size bytes, not executed)
 *   Then: data section:
 *     ds[0]      outdoor flag  (bit 7)
 *     ds[1]      section type  (1=town 2=surface 3=castle)
 *     ds[2-7]    sprite IDs    (3x uint16 LE)
 *     ds[8-19]   exit dirs     (4x uint16+uint8 = 3 bytes each? varies)
 *     ds[23]     safe_x
 *     ds[24]     safe_y
 *     ds[29]     encounter_rand
 *     ds[30]     wall1_word
 *     ds[31]     door_word
 *     ds[32]     wall2_word
 *     ds[33]     max_mon_level
 *     ds[34]     max_mon_qty
 *     ds[44]     danger_rate
 *     ds[45]     flags (bit0=dark, bit1=no_teleport)
 *     ds[50]     num_scripts
 *   Scripts at ds+51 are stored as parallel arrays:
 *     num_scripts bytes: packed y<<4|x coordinates
 *     num_scripts bytes: direction masks
 *     num_scripts words: code offsets (uint16 LE, unused in C port)
 *   Strings: NUL-separated text after scripts
 */

#include <stdint.h>

#define OVR_HEADER_SIZE  14
#define OVR_MAX_SCRIPTS  64
#define OVR_MAX_STRINGS  128
#define OVR_STRING_MAX   256

typedef struct {
    uint8_t outdoor;       /* 1 = outdoor */
    uint8_t section_type;  /* 1=town 2=surface 3=castle */
    uint8_t safe_x, safe_y;
    uint8_t encounter_rand;
    uint8_t wall1_word, door_word, wall2_word;
    uint8_t max_mon_level, max_mon_qty;
    uint8_t danger_rate;
    uint8_t dark_map;      /* 1 = dark map */
    uint8_t no_teleport;   /* 1 = teleport disabled */
} OvrConstants;

typedef struct {
    uint8_t x, y;
    uint8_t direction;     /* direction bitmask: N=C0 E=30 S=0C W=03 */
    uint16_t code_offset;  /* not executed in C port */
} OvrScript;

typedef struct {
    OvrConstants  constants;
    OvrScript     scripts[OVR_MAX_SCRIPTS];
    int           num_scripts;
    char          strings[OVR_MAX_STRINGS][OVR_STRING_MAX];
    int           num_strings;
} OvrFile;

/*
 * Load an OVR file from disk.
 * ovr_dir: directory containing OVR files (Original_Source/)
 * ovr_name: base name without extension, e.g. "SORPIGAL"
 * Returns 1 on success, 0 if file not found (fills in safe defaults).
 */
/* Load OVR from in-memory buffer (HamsterOS port - no fopen). */
int ovr_load_buf(OvrFile* ovr, const uint8_t *raw, uint32_t fsz);

/*
 * Return all scripts at (x, y).  If facing != -1, prefer direction-matching
 * scripts (exact match first, then wildcard 0xFF, then any).
 * Fills out[0..max_out) and returns count found.
 */
int ovr_scripts_at(const OvrFile* ovr, int x, int y, int facing,
                   const OvrScript** out, int max_out);

/*
 * Return dialog string for script index idx (into the dialog_strings subset
 * of the string table, i.e. strings with len >= 4 and printable ASCII).
 * Returns "" if idx out of range.
 */
const char* ovr_string_for_script(const OvrFile* ovr, int idx);

/*
 * Return the best text to show for the tile at (x,y) with given facing.
 * Returns "" if no scripts exist there.
 */
const char* ovr_text_at(const OvrFile* ovr, int x, int y, int facing);
