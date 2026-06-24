/* roster_port.c — ROSTER.DTA parser for HamsterOS port.
 * Reads binary records directly into game.h Party/Character (no SDL2, no fopen). */

#include "roster_port.h"
#include "mm_util.h"

#define CHAR_SIZE  127   /* bytes per record in ROSTER.DTA */
#define MAX_CHARS  18    /* total roster slots */

static void parse_char(const uint8_t *r, Character *c, int slot)
{
    int s;
    mm_memset(c, 0, sizeof(*c));
    c->slot = slot;

    mm_memcpy(c->name, r, 15);   /* copy 15 chars, leave [15] as NUL from memset */

    /* 0x13=race 0x14=cls */
    c->race = r[0x13];
    c->cls  = r[0x14];

    /* stats[7] cur+base at 0x15, each pair is 2 bytes */
    for (s = 0; s < 7; s++) {
        c->stats[s]      = r[0x15 + s * 2];
        c->stats_base[s] = r[0x15 + s * 2 + 1];
    }

    c->level   = r[0x23];       /* level_cur */
    c->xp      = (int)((uint32_t)r[0x27]        |
                       ((uint32_t)r[0x28] <<  8) |
                       ((uint32_t)r[0x29] << 16) |
                       ((uint32_t)r[0x2A] << 24));
    c->sp      = (int)((uint16_t)r[0x2B] | ((uint16_t)r[0x2C] << 8));
    c->sp_max  = (int)((uint16_t)r[0x2D] | ((uint16_t)r[0x2E] << 8));
    c->gems    = (int)((uint16_t)r[0x31] | ((uint16_t)r[0x32] << 8));
    c->hp      = (int)((uint16_t)r[0x33] | ((uint16_t)r[0x34] << 8));
    c->hp_max  = (int)((uint16_t)r[0x37] | ((uint16_t)r[0x38] << 8));
    c->gold    = (int)((uint32_t)r[0x39]        |
                       ((uint32_t)r[0x3A] <<  8) |
                       ((uint32_t)r[0x3B] << 16));
    c->ac        = r[0x3C];
    c->food      = r[0x3E];
    c->condition = r[0x3F];

    for (s = 0; s < 6; s++) {
        c->equipped[s] = r[0x40 + s];
        c->backpack[s] = r[0x46 + s];
    }

    /* quest flags 0x70-0x77, visit_flags 0x7B */
    mm_memcpy(c->char_quests, r + 0x70, 8);
    c->visit_flags = r[0x7B];
}

int roster_load_buf(const uint8_t *data, uint32_t data_size, Party *party)
{
    uint32_t expected = (uint32_t)MAX_CHARS * CHAR_SIZE + MAX_CHARS;
    int i, n = 0;

    if (!data || data_size < expected) return -1;

    const uint8_t *exists = data + (uint32_t)MAX_CHARS * CHAR_SIZE;
    party->count = 0;

    for (i = 0; i < MAX_CHARS; i++) {
        if (exists[i] && party->count < MAX_PARTY) {
            parse_char(data + (uint32_t)i * CHAR_SIZE,
                       &party->members[party->count], i);
            party->count++;
            n++;
        }
    }
    return n;
}