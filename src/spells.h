#pragma once
/*
 * spells.h -- Spell definitions for MM1 C port.
 */

/* target constants */
#define SPELL_TARGET_SELF   0
#define SPELL_TARGET_MEMBER 1
#define SPELL_TARGET_PARTY  2
#define SPELL_TARGET_ENEMY1 3
#define SPELL_TARGET_ALL    4

/* effect constants */
#define SPELL_EFFECT_HEAL    1
#define SPELL_EFFECT_DAMAGE  2
#define SPELL_EFFECT_SLEEP   3
#define SPELL_EFFECT_BLESS   4
#define SPELL_EFFECT_CURE    5
#define SPELL_EFFECT_RAISE   6
#define SPELL_EFFECT_RESTORE 7  /* sp restore */
#define SPELL_EFFECT_ESCAPE  8  /* flee combat */
#define SPELL_EFFECT_LIGHT   9
#define SPELL_EFFECT_FOOD   10
#define SPELL_EFFECT_PORTAL 11
#define SPELL_EFFECT_RECHARGE  12   /* recharge a spell item (1d4 charges, 1% break) */
#define SPELL_EFFECT_DUPLICATE 13   /* copy an item from backpack to free slot */
#define SPELL_EFFECT_PROTECT   16   /* grant party protection buffer; param1=PROT_* idx, param2=duration */
#define SPELL_EFFECT_DETECT_MAGIC 14 /* detect magical items in party inventory */

/* class constants */
#define SPELL_CLASS_CLERIC 4
#define SPELL_CLASS_SORC   5
#define SPELL_CLASS_ANY    0

/* spell flags */
#define SPELL_FLAG_COMBAT    1  /* combat only */
#define SPELL_FLAG_NONCOMBAT 2  /* overworld only */
#define SPELL_FLAG_OUTDOORS  8  /* outdoors only */

typedef struct {
    int         id;
    const char *name;
    int         cls;
    int         level;
    int         sp_cost;
    int         target;
    int         effect;
    int         param1;
    int         param2;
    int         gem_cost;  /* gems consumed when cast (0 = none) */
    int         flags;     /* SPELL_FLAG_COMBAT=1, SPELL_FLAG_NONCOMBAT=2, SPELL_FLAG_OUTDOORS=8 */
    /*
     * heal:    param1=dice_sides  param2=bonus
     * damage:  param1=dice_sides  param2=bonus (num_dice = level/2+1)
     * sleep:   param1=rounds
     * cure:    param1=cond_mask
     * protect: param1=PROT_* index  param2=duration (rounds/steps)
     */
} SpellDef;

const SpellDef *spell_get(int id);
int             spell_count(void);
/* get spells for a class at a given level — fills ids[], returns count */
int             spells_for_class_level(int cls, int level, int *ids, int max);
