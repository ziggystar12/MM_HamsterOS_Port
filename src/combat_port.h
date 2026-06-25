#pragma once
#include "game.h"
#include <stdbool.h>
#include "monsters.h"

typedef enum { CR_VICTORY=0, CR_DEFEAT=1, CR_FLED=2 } CombatResult;
typedef enum { COMBAT_ACT_NONE=0, COMBAT_ACT_FIGHT=1, COMBAT_ACT_SHOOT=2 } CombatPendingAction;

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
    int          pending_gems;
    int          target_group; /* which group to attack; -1 = first alive */
    int          found_items[4];
    int          n_found_items;
    CombatResult result;
    bool         over;
    int          bless_rounds;  /* rounds remaining on party bless buff */
    int          bless_bonus;   /* attack roll bonus while bless active */
    int          n_hits_dealt;  /* hits landed this round (party+monsters) for SFX */
    int          current_char;  /* party member waiting for input, -1 if none */
    int          pending_action;/* COMBAT_ACT_* while choosing A-D target */
    int          checked_party[MAX_PARTY];
    int          checked_group[4];
    int          can_attack[MAX_PARTY];
    int          group_range[4];/* 0 = melee, higher values must advance */
    int          defending[MAX_PARTY];
    int          protecting[MAX_PARTY];
    int          treasure_flags;
    int          treasure_container;
    int          treasure_trap;
    int          reward_applied;
} CombatSession;

/* Run ONE round targeting a specific group (or first alive if target<0). */
bool combat_round_targeted(CombatSession *cs, GameState *gs, int target);
/* Roll for loot after victory. Returns count of items added to party. */
int  combat_loot(CombatSession *cs, GameState *gs);

void combat_init(CombatSession *cs, GameState *gs);
void combat_set_ovr_limits(int max_level, int max_qty);
void combat_start_flow(CombatSession *cs, GameState *gs, const struct Map *m);
bool combat_advance_flow(CombatSession *cs, GameState *gs, const struct Map *m);
bool combat_action_fight(CombatSession *cs, GameState *gs, const struct Map *m, int target, bool shoot);
bool combat_action_block(CombatSession *cs, GameState *gs, const struct Map *m);
bool combat_action_protect(CombatSession *cs, GameState *gs, const struct Map *m);
bool combat_action_delay(CombatSession *cs, GameState *gs, const struct Map *m);
bool combat_action_exchange(CombatSession *cs, GameState *gs, const struct Map *m);
void combat_finish_player_action(CombatSession *cs, GameState *gs, const struct Map *m);
int  combat_current_char(const CombatSession *cs);
int  combat_can_current_attack(const CombatSession *cs);
int  combat_group_in_range(const CombatSession *cs, int target, bool shoot);
void combat_note_monster_kill(CombatSession *cs, const MonsterType *mt);
/* Run one full round (party attacks + monsters attack).
 * Returns true if combat ended this round. */
bool combat_round(CombatSession *cs, GameState *gs);
/* Attempt flee. Returns true if successful. */
bool combat_flee(CombatSession *cs, GameState *gs);
/* Let monsters answer after a non-attack party action such as spell/item/bribe.
 * Returns true if combat ended. */
bool combat_monsters_retaliate(CombatSession *cs, GameState *gs);
/* Apply XP/gold rewards to party after victory. */
void combat_reward(CombatSession *cs, GameState *gs);
