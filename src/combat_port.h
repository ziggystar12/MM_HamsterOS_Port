#pragma once
#include "game.h"
#include <stdbool.h>
#include "monsters.h"
#include <stdbool.h>

typedef enum { CR_VICTORY=0, CR_DEFEAT=1, CR_FLED=2 } CombatResult;

#define COMBAT_LOG_LINES 8
#define COMBAT_LOG_LEN   72

/* Combat session state — stored in mm_port.c static storage */
typedef struct {
    MonsterGroup groups[4];
    int          n_groups;
    int          round;
    int          party_fled;
    char         log[COMBAT_LOG_LINES][COMBAT_LOG_LEN];
    int          log_head;
    int          log_count;
    int          pending_xp;
    int          pending_gold;
    int          target_group; /* which group to attack; -1 = first alive */
    int          found_items[4];
    int          n_found_items;
    CombatResult result;
    bool         over;
    int          bless_rounds;  /* rounds remaining on party bless buff */
    int          bless_bonus;   /* attack roll bonus while bless active */
    int          n_hits_dealt;  /* hits landed this round (party+monsters) for SFX */
} CombatSession;

/* Run ONE round targeting a specific group (or first alive if target<0). */
bool combat_round_targeted(CombatSession *cs, GameState *gs, int target);
/* Roll for loot after victory. Returns count of items added to party. */
int  combat_loot(CombatSession *cs, GameState *gs);

void combat_init(CombatSession *cs, GameState *gs);
/* Run one full round (party attacks + monsters attack).
 * Returns true if combat ended this round. */
bool combat_round(CombatSession *cs, GameState *gs);
/* Attempt flee. Returns true if successful. */
bool combat_flee(CombatSession *cs, GameState *gs);
/* Apply XP/gold rewards to party after victory. */
void combat_reward(CombatSession *cs, GameState *gs);