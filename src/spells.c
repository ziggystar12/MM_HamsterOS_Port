/*
 * spells.c -- Spell table for MM1 C port.
 *
 * Condition bitmask (matches game.h):
 *   COND_ASLEEP      = 0x01
 *   COND_BLINDED     = 0x02
 *   COND_SILENCED    = 0x04
 *   COND_DISEASED    = 0x08
 *   COND_POISONED    = 0x10
 *   COND_PARALYZED   = 0x20
 *   COND_UNCONSCIOUS = 0x40
 */

#include "spells.h"
#include "game.h"    /* PROT_* defines */
#include <stddef.h>

/* sp_cost ≈ spell_level * 2 */
/* spell IDs: Cleric 1-20, Sorcerer 101-120 */

static const SpellDef SPELL_TABLE[] = {
    /* ------------------------------------------------------------------ */
    /* Cleric spells (cls=4)                                               */
    /* ------------------------------------------------------------------ */
    /* id  name                    cls  lv  sp  target               effect                  p1    p2  gem flg */
    {   1, "Awaken",               4,   1,  2,  SPELL_TARGET_MEMBER, SPELL_EFFECT_CURE,      0x01,  0,  0, 0 },
    {   2, "Bless",                4,   1,  2,  SPELL_TARGET_PARTY,  SPELL_EFFECT_BLESS,        1,  0,  0, SPELL_FLAG_COMBAT },
    {   3, "First Aid",            4,   1,  2,  SPELL_TARGET_MEMBER, SPELL_EFFECT_HEAL,          4,  1,  0, 0 },
    {   4, "Lasting Light",        4,   1,  2,  SPELL_TARGET_PARTY,  SPELL_EFFECT_LIGHT,         0,  0,  0, 0 },
    {   5, "Cure Wounds",          4,   2,  4,  SPELL_TARGET_MEMBER, SPELL_EFFECT_HEAL,          8,  2,  0, 0 },
    {   6, "Cure Blindness",       4,   2,  4,  SPELL_TARGET_MEMBER, SPELL_EFFECT_CURE,      0x02,  0,  0, 0 },
    {   7, "Create Food",          4,   2,  4,  SPELL_TARGET_PARTY,  SPELL_EFFECT_FOOD,          0,  0,  0, SPELL_FLAG_NONCOMBAT },
    {   8, "Turn Undead",          4,   2,  4,  SPELL_TARGET_ALL,    SPELL_EFFECT_DAMAGE,        8,  0,  0, SPELL_FLAG_COMBAT },
    {   9, "Cure Disease",         4,   3,  6,  SPELL_TARGET_MEMBER, SPELL_EFFECT_CURE,      0x08,  0,  0, 0 },
    {  10, "Neutralize Poison",    4,   3,  6,  SPELL_TARGET_MEMBER, SPELL_EFFECT_CURE,      0x10,  0,  0, 0 },
    {  11, "Deadly Swarm",         4,   3,  6,  SPELL_TARGET_ALL,    SPELL_EFFECT_DAMAGE,       12,  0,  0, SPELL_FLAG_COMBAT },
    {  12, "Restore Energy",       4,   3,  6,  SPELL_TARGET_MEMBER, SPELL_EFFECT_RESTORE,      10,  0,  0, 0 },
    {  13, "Power Cure",           4,   4,  8,  SPELL_TARGET_MEMBER, SPELL_EFFECT_HEAL,           8,  8,  0, 0 },
    {  14, "Remove Condition",     4,   4,  8,  SPELL_TARGET_MEMBER, SPELL_EFFECT_CURE,      0x7F,  0,  0, 0 },
    {  15, "Holy Word",            4,   4,  8,  SPELL_TARGET_ALL,    SPELL_EFFECT_DAMAGE,       20,  0,  0, SPELL_FLAG_COMBAT },
    {  16, "Raise Dead",           4,   5, 10,  SPELL_TARGET_MEMBER, SPELL_EFFECT_RAISE,          0,  0,  0, 0 },
    {  17, "Surface",              4,   5, 10,  SPELL_TARGET_PARTY,  SPELL_EFFECT_ESCAPE,        0,  0,  0, SPELL_FLAG_NONCOMBAT },
    {  18, "Rejuvenate",           4,   6, 12,  SPELL_TARGET_MEMBER, SPELL_EFFECT_HEAL,         99,  0,  1, 0 },
    {  19, "Town Portal",          4,   6, 12,  SPELL_TARGET_PARTY,  SPELL_EFFECT_PORTAL,        0,  0,  1, SPELL_FLAG_NONCOMBAT },
    {  20, "Divine Intervention",  4,   7, 14,  SPELL_TARGET_PARTY,  SPELL_EFFECT_HEAL,         99,  0,  1, 0 },
    /* Protection From * spells (Cleric lv 3-5) */
    {  21, "Prot From Fear",       4,   3,  6,  SPELL_TARGET_PARTY,  SPELL_EFFECT_PROTECT, PROT_FEAR,   5,  0, 0 },
    {  22, "Prot From Cold",       4,   3,  6,  SPELL_TARGET_PARTY,  SPELL_EFFECT_PROTECT, PROT_COLD,   5,  0, 0 },
    {  23, "Prot From Fire",       4,   4,  8,  SPELL_TARGET_PARTY,  SPELL_EFFECT_PROTECT, PROT_FIRE,   5,  0, 0 },
    {  24, "Prot From Poison",     4,   4,  8,  SPELL_TARGET_PARTY,  SPELL_EFFECT_PROTECT, PROT_POISON, 5,  0, 0 },
    {  25, "Prot From Acid",       4,   5, 10,  SPELL_TARGET_PARTY,  SPELL_EFFECT_PROTECT, PROT_ACID,   5,  1, 0 },
    {  26, "Prot From Electricity",4,   5, 10,  SPELL_TARGET_PARTY,  SPELL_EFFECT_PROTECT, PROT_ELEC,   5,  1, 0 },

    /* ------------------------------------------------------------------ */
    /* Sorcerer spells (cls=5)                                             */
    /* ------------------------------------------------------------------ */
    { 101, "Energy Blast",         5,   1,  2,  SPELL_TARGET_ENEMY1, SPELL_EFFECT_DAMAGE,        6,  0,  0, SPELL_FLAG_COMBAT },
    { 102, "Sleep",                5,   1,  2,  SPELL_TARGET_ALL,    SPELL_EFFECT_SLEEP,          3,  0,  0, SPELL_FLAG_COMBAT },
    { 103, "Scare",                5,   1,  2,  SPELL_TARGET_ENEMY1, SPELL_EFFECT_SLEEP,          2,  0,  0, SPELL_FLAG_COMBAT },
    { 104, "Levitate",             5,   2,  4,  SPELL_TARGET_PARTY,  SPELL_EFFECT_PROTECT, PROT_LEVITATE, 5, 0, 0 },
    { 105, "Flame Arrow",          5,   2,  4,  SPELL_TARGET_ENEMY1, SPELL_EFFECT_DAMAGE,        10,  0,  0, SPELL_FLAG_COMBAT },
    { 106, "Slow",                 5,   2,  4,  SPELL_TARGET_ALL,    SPELL_EFFECT_SLEEP,          1,  0,  0, SPELL_FLAG_COMBAT },
    { 107, "Electric Arrow",       5,   3,  6,  SPELL_TARGET_ENEMY1, SPELL_EFFECT_DAMAGE,        12,  0,  0, SPELL_FLAG_COMBAT },
    { 108, "Heal",                 5,   3,  6,  SPELL_TARGET_SELF,   SPELL_EFFECT_HEAL,           8,  4,  0, 0 },
    { 109, "Haste",                5,   3,  6,  SPELL_TARGET_PARTY,  SPELL_EFFECT_BLESS,          2,  0,  0, SPELL_FLAG_COMBAT },
    { 110, "Fire Ball",            5,   4,  8,  SPELL_TARGET_ALL,    SPELL_EFFECT_DAMAGE,        16,  0,  0, SPELL_FLAG_COMBAT },
    { 111, "Lightning Bolt",       5,   4,  8,  SPELL_TARGET_ENEMY1, SPELL_EFFECT_DAMAGE,        20,  0,  0, SPELL_FLAG_COMBAT },
    { 112, "Location",             5,   4,  8,  SPELL_TARGET_PARTY,  SPELL_EFFECT_LIGHT,          0,  0,  0, 0 },
    { 113, "Acid Beam",            5,   5, 10,  SPELL_TARGET_ENEMY1, SPELL_EFFECT_DAMAGE,        24,  0,  0, SPELL_FLAG_COMBAT },
    { 114, "Quickness",            5,   5, 10,  SPELL_TARGET_PARTY,  SPELL_EFFECT_BLESS,          3,  0,  0, SPELL_FLAG_COMBAT },
    { 115, "Power",                5,   5, 10,  SPELL_TARGET_PARTY,  SPELL_EFFECT_BLESS,          4,  0,  0, SPELL_FLAG_COMBAT },
    { 116, "Fly",                  5,   6, 12,  SPELL_TARGET_PARTY,  SPELL_EFFECT_ESCAPE,         1,  0,  1, SPELL_FLAG_NONCOMBAT },
    { 117, "Teleport",             5,   6, 12,  SPELL_TARGET_PARTY,  SPELL_EFFECT_PORTAL,         0,  0,  1, SPELL_FLAG_NONCOMBAT },
    { 118, "Shelter",              5,   6, 12,  SPELL_TARGET_PARTY,  SPELL_EFFECT_LIGHT,          0,  0,  1, 0 },
    { 119, "Disintegration",       5,   7, 14,  SPELL_TARGET_ENEMY1, SPELL_EFFECT_DAMAGE,        50,  0,  2, SPELL_FLAG_COMBAT },
    { 120, "Finger of Death",      5,   7, 14,  SPELL_TARGET_ENEMY1, SPELL_EFFECT_DAMAGE,        99,  0,  2, SPELL_FLAG_COMBAT },
    /* Level-6 Sorcerer */
    { 121, "Recharge Item",        5,   6, 12,  SPELL_TARGET_SELF,   SPELL_EFFECT_RECHARGE,       4,  0,  1, SPELL_FLAG_NONCOMBAT },
    /* Level-7 Sorcerer */
    { 122, "Duplication",          5,   7, 14,  SPELL_TARGET_SELF,   SPELL_EFFECT_DUPLICATE,      0,  0,  2,  SPELL_FLAG_NONCOMBAT },
    /* Level-1 Sorcerer -- Detect Magic */
    { 123, "Detect Magic",         5,   1,  5,  SPELL_TARGET_SELF,   SPELL_EFFECT_DETECT_MAGIC,   0,  0,  0,  SPELL_FLAG_NONCOMBAT },
};

#define SPELL_TABLE_SIZE ((int)(sizeof(SPELL_TABLE) / sizeof(SPELL_TABLE[0])))

const SpellDef *spell_get(int id)
{
    for (int i = 0; i < SPELL_TABLE_SIZE; i++) {
        if (SPELL_TABLE[i].id == id)
            return &SPELL_TABLE[i];
    }
    return NULL;
}

int spell_count(void)
{
    return SPELL_TABLE_SIZE;
}

int spells_for_class_level(int cls, int level, int *ids, int max)
{
    int n = 0;
    for (int i = 0; i < SPELL_TABLE_SIZE && n < max; i++) {
        if (SPELL_TABLE[i].cls == cls && SPELL_TABLE[i].level == level) {
            ids[n++] = SPELL_TABLE[i].id;
        }
    }
    return n;
}