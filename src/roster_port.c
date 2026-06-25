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

static void write_char(uint8_t *r, const Character *c)
{
    int s;
    mm_memset(r, 0, CHAR_SIZE);
    mm_memcpy(r, c->name, 15);
    r[0x13] = (uint8_t)c->race;
    r[0x14] = (uint8_t)c->cls;
    for (s = 0; s < 7; s++) {
        r[0x15 + s * 2]     = (uint8_t)c->stats[s];
        r[0x15 + s * 2 + 1] = (uint8_t)c->stats_base[s];
    }
    r[0x23] = (uint8_t)c->level;
    r[0x27] = (uint8_t)c->xp;
    r[0x28] = (uint8_t)(c->xp >> 8);
    r[0x29] = (uint8_t)(c->xp >> 16);
    r[0x2A] = (uint8_t)(c->xp >> 24);
    r[0x2B] = (uint8_t)c->sp;
    r[0x2C] = (uint8_t)(c->sp >> 8);
    r[0x2D] = (uint8_t)c->sp_max;
    r[0x2E] = (uint8_t)(c->sp_max >> 8);
    r[0x31] = (uint8_t)c->gems;
    r[0x32] = (uint8_t)(c->gems >> 8);
    r[0x33] = (uint8_t)c->hp;
    r[0x34] = (uint8_t)(c->hp >> 8);
    r[0x37] = (uint8_t)c->hp_max;
    r[0x38] = (uint8_t)(c->hp_max >> 8);
    r[0x39] = (uint8_t)c->gold;
    r[0x3A] = (uint8_t)(c->gold >> 8);
    r[0x3B] = (uint8_t)(c->gold >> 16);
    r[0x3C] = (uint8_t)c->ac;
    r[0x3E] = (uint8_t)c->food;
    r[0x3F] = (uint8_t)c->condition;
    for (s = 0; s < 6; s++) {
        r[0x40 + s] = (uint8_t)c->equipped[s];
        r[0x46 + s] = (uint8_t)c->backpack[s];
    }
    mm_memcpy(r + 0x70, c->char_quests, 8);
    r[0x7B] = c->visit_flags;
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

int roster_load_full_buf(const uint8_t *data, uint32_t data_size,
                         Character chars[18], uint8_t exists_out[18])
{
    uint32_t expected = (uint32_t)MAX_CHARS * CHAR_SIZE + MAX_CHARS;
    const uint8_t *exists;
    int i, n = 0;

    if (!data || data_size < expected || !chars || !exists_out) return -1;

    exists = data + (uint32_t)MAX_CHARS * CHAR_SIZE;
    for (i = 0; i < MAX_CHARS; i++) {
        exists_out[i] = exists[i] ? 1 : 0;
        if (exists_out[i]) {
            parse_char(data + (uint32_t)i * CHAR_SIZE, &chars[i], i);
            n++;
        } else {
            mm_memset(&chars[i], 0, sizeof(chars[i]));
            chars[i].slot = i;
        }
    }
    return n;
}

void roster_build_full_buf(uint8_t *raw,
                           const Character chars[18], const uint8_t exists[18])
{
    int i;
    if (!raw || !chars || !exists) return;
    mm_memset(raw, 0, (uint32_t)MAX_CHARS * CHAR_SIZE + MAX_CHARS);
    for (i = 0; i < MAX_CHARS; i++) {
        if (exists[i]) {
            write_char(raw + (uint32_t)i * CHAR_SIZE, &chars[i]);
            raw[(uint32_t)MAX_CHARS * CHAR_SIZE + i] = 1;
        }
    }
}
