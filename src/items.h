#pragma once
/*
 * items.h -- Item definitions and shop stock tables for MM1 C port.
 */

/* slot values */
#define SLOT_MISC      0
#define SLOT_WEAPON    1
#define SLOT_MISSILE   2
#define SLOT_TWOHANDED 3
#define SLOT_ARMOR     4
#define SLOT_SHIELD    5

/* alignment values (item and character) */
#define ALIGN_ANY     0
#define ALIGN_GOOD    1
#define ALIGN_NEUTRAL 2
#define ALIGN_EVIL    3

typedef struct {
    int  id;
    char name[28];
    int  cost;
    int  slot;
    int  ac_bonus;
    int  dmg_bonus;
    int  class_mask;
    int  spell_id;
} ItemDef;

/* Per-item extras: alignment restriction + stat/resistance bonus when equipped.
 * stat_idx 1-7 maps to stats[0..6] (INT MIG PER END SPD ACC LCK).
 * res_idx  1-8 maps to resistances[0..7] (magic fire cold elec acid fear poison sleep). */
typedef struct {
    int  id;
    int  alignment;   /* ALIGN_* — who can use this item */
    int  stat_idx;    /* 1-7 = which stat gets the bonus, 0 = none */
    int  stat_bonus;  /* bonus value (positive or negative) */
    int  res_idx;     /* 1-8 = which resistance gets the bonus, 0 = none */
    int  res_bonus;   /* bonus % */
} ItemExtras;

const ItemDef    *item_get(int id);
const ItemExtras *item_get_extras(int id);   /* returns &zero_extras if not found */
const char       *item_name(int id);
const int        *shop_stock(int town_idx, int *count);
