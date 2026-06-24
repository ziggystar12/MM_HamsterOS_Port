#pragma once
/*
 * monsters.h -- Monster type table and encounter generation for MM1 C port.
 */

#define MAX_MONSTER_GROUP 12

/* Special attack types for MonsterType.special */
#define MON_SPECIAL_NONE     0
#define MON_SPECIAL_FIRE     1   /* fire breath */
#define MON_SPECIAL_COLD     2   /* cold beam */
#define MON_SPECIAL_LIGHTNING 3  /* lightning */
#define MON_SPECIAL_POISON   4   /* poison gas */
#define MON_SPECIAL_PARALYZE 5   /* paralyze ray */
#define MON_SPECIAL_DEATH    6   /* death gaze */
#define MON_SPECIAL_PSYCHIC  7   /* psychic blast */

typedef struct {
    int id;
    const char *name;
    int max_group;
    int hp;
    int ac;
    int max_dmg;
    int num_attacks;
    int xp;
    int undead;
    int level;
    int loot;
    int touch; /* 0=none 1=poison 2=paralysis 3=disease 4=sleep */
    int sprite_id;
    int special; /* MON_SPECIAL_* (0=none) */
} MonsterType;

typedef struct {
    const MonsterType *type;
    int count;
    int hp[MAX_MONSTER_GROUP];
    int alive;
    int sleep_rounds;
} MonsterGroup;

void               monster_rng_seed(unsigned seed);
int                monster_roll(int sides);
const MonsterType *monster_get(int id);
int                monster_count(void);
int                monster_sprite_id(int monster_index);
void               encounter_generate(int max_level, int max_qty,
                                      MonsterGroup *out, int *out_count);
