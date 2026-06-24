#pragma once
/*
 * mazedata.h -- Map data structures for MM_HamsterOS_Port.
 * Identical to MM_C_Port except mazedata_load() takes an in-memory buffer
 * instead of a file path (no fopen/fread on HamsterOS).
 */

#include <stdint.h>

#define MAP_SIZE    16
#define NUM_MAPS    55

#define WT_NONE   0
#define WT_WALL   1
#define WT_DOOR   2
#define WT_TORCH  3

#define PASS_N 0
#define PASS_E 1
#define PASS_S 2
#define PASS_W 3

typedef struct {
    uint8_t wall[4];       /* wall type N/E/S/W: WT_* */
    uint8_t pass[4];       /* passable N/E/S/W: 0=blocked 1=open */
    uint8_t event_id;      /* non-zero if a script event exists here */
    uint8_t dark;          /* bit 5: darkness */
    uint8_t no_magic;      /* bit 1: spells cannot be cast here */
    uint8_t danger_rest;   /* bit 3: resting triggers a random encounter */
} Cell;

struct Map {
    char    name[32];       /* OVR base name, e.g. "sorpigal" */
    int     map_idx;        /* index 0-54 */
    int     region_id;      /* sprite region */
    int     outdoor;        /* 1 = outdoor / surface map */
    Cell    cells[MAP_SIZE][MAP_SIZE]; /* cells[y][x], y=0 at south */
};

/* -------------------------------------------------------------------------- */
/* API                                                                         */
/* -------------------------------------------------------------------------- */

/* Load all maps from an in-memory MAZEDATA.DTA buffer.
 * data       : pointer to the raw file bytes
 * total_size : number of bytes in data
 * Returns number of maps loaded (up to NUM_MAPS), or -1 on failure. */
int mazedata_load(struct Map maps[NUM_MAPS],
                  const uint8_t *data, uint32_t total_size);

/* Return cell at (x, y), or NULL if out of bounds. */
const Cell* map_cell(const struct Map* m, int x, int y);

/* Return 1 if the player can exit through wall_dir (0=N 1=E 2=S 3=W). */
int  can_pass(const struct Map* m, int x, int y, int wall_dir);

/* Return wall type (WT_*) for the wall in direction wall_dir from (x,y). */
int  wall_type(const struct Map* m, int x, int y, int wall_dir);

/* OVR file base name for map index (e.g. 0->"sorpigal"). */
const char* map_ovr_name(int map_idx);

/* Human-readable display name for map index. */
const char* map_display_name(int map_idx);
