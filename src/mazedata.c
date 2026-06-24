#include "mazedata.h"
#include "mm_util.h"

#define MAZEDATA_MAP_BYTES  512

static const char* const OVR_NAMES[NUM_MAPS] = {
    "sorpigal","portsmit","algary","dusk","erliquin",
    "cave1","cave2","cave3","cave4","cave5","cave6","cave7","cave8","cave9",
    "areaa1","areaa2","areaa3","areaa4",
    "areab1","areab2","areab3","areab4",
    "areac1","areac2","areac3","areac4",
    "aread1","aread2","aread3","aread4",
    "areae1","areae2","areae3","areae4",
    "doom","blackrn","blackrs","qvl1","qvl2","rwl1","rwl2",
    "enf1","enf2","whitew","dragad",
    "udrag1","udrag2","udrag3","demon","alamar",
    "pp1","pp2","pp3","pp4","astral"
};

static const char* const DISPLAY_NAMES[NUM_MAPS] = {
    "Town of Sorpigal","Portsmith","Algary","Dusk","Erliquin",
    "Sorpigal Dungeon","Cave 2","Cave 3","Cave 4","Cave 5",
    "Cave 6","Cave 7","Cave 8","Cave 9",
    "Area A-1","Area A-2","Area A-3","Area A-4",
    "Area B-1","Area B-2","Area B-3","Area B-4",
    "Area C-1","Area C-2","Area C-3","Area C-4",
    "Area D-1","Area D-2","Area D-3","Area D-4",
    "Area E-1","Area E-2","Area E-3","Area E-4",
    "Castle Doom","Castle Blackridge N","Castle Blackridge S",
    "Queensway L1","Queensway L2","Rogue Way L1","Rogue Way L2",
    "Enforcers L1","Enforcers L2","White Wolf","Castle Dragadune",
    "Under Dragadune 1","Under Dragadune 2","Under Dragadune 3",
    "Demon City","Castle Alamar",
    "Peril Path 1","Peril Path 2","Peril Path 3","Peril Path 4","Astral Plane"
};

static int is_outdoor(int idx) { return (idx >= 14 && idx <= 33); }

int mazedata_load(struct Map maps[NUM_MAPS],
                  const uint8_t *data, uint32_t total_size)
{
    int loaded = 0;
    int mi;

    if (!data || total_size < MAZEDATA_MAP_BYTES)
        return -1;

    for (mi = 0; mi < NUM_MAPS; mi++) {
        uint32_t off = (uint32_t)mi * MAZEDATA_MAP_BYTES;
        const uint8_t *buf;
        struct Map *m;
        int x, y;

        if (off + MAZEDATA_MAP_BYTES > total_size)
            break;

        buf = data + off;
        m   = &maps[mi];

        mm_strncpy(m->name, OVR_NAMES[mi], sizeof(m->name));
        m->map_idx   = mi;
        m->region_id = mi;
        m->outdoor   = is_outdoor(mi);

        for (y = 0; y < MAP_SIZE; y++) {
            for (x = 0; x < MAP_SIZE; x++) {
                uint8_t wb = buf[16*y + x];
                uint8_t fb = buf[256 + 16*y + x];
                Cell *c = &m->cells[y][x];
                c->wall[0] = (wb >> 6) & 3;
                c->wall[1] = (wb >> 4) & 3;
                c->wall[2] = (wb >> 2) & 3;
                c->wall[3] = (wb >> 0) & 3;
                c->pass[0] = (fb & 0x40) ? 0 : 1;
                c->pass[1] = (fb & 0x10) ? 0 : 1;
                c->pass[2] = (fb & 0x04) ? 0 : 1;
                c->pass[3] = (fb & 0x01) ? 0 : 1;
                c->event_id    = (fb & 0x80) ? 1 : 0;
                c->dark        = (fb & 0x20) ? 1 : 0;
                c->no_magic    = (fb & 0x02) ? 1 : 0;
                c->danger_rest = (fb & 0x08) ? 1 : 0;
            }
        }
        loaded++;
    }
    return loaded;
}

const Cell* map_cell(const struct Map* m, int x, int y)
{
    if (!m || x < 0 || x >= MAP_SIZE || y < 0 || y >= MAP_SIZE) return 0;
    return &m->cells[y][x];
}

int can_pass(const struct Map* m, int x, int y, int dir)
{
    const Cell* c = map_cell(m, x, y);
    if (!c || dir < 0 || dir > 3) return 0;
    return c->pass[dir];
}

int wall_type(const struct Map* m, int x, int y, int dir)
{
    const Cell* c = map_cell(m, x, y);
    if (!c || dir < 0 || dir > 3) return WT_WALL;
    return c->wall[dir];
}

const char* map_ovr_name(int map_idx)
{
    if (map_idx < 0 || map_idx >= NUM_MAPS) return "";
    return OVR_NAMES[map_idx];
}

const char* map_display_name(int map_idx)
{
    if (map_idx < 0 || map_idx >= NUM_MAPS) return "";
    return DISPLAY_NAMES[map_idx];
}
