#pragma once
#include <stdint.h>
/*
 * game.h -- Player state, direction math, and GameState for MM1 C port.
 */

/* Forward declarations */
typedef struct Map   Map;
typedef struct Party Party;

/* -------------------------------------------------------------------------- */
/* Direction constants                                                          */
/* -------------------------------------------------------------------------- */
#define DIR_N 0
#define DIR_E 1
#define DIR_S 2
#define DIR_W 3

/* Forward deltas per facing (Y=0 at south edge, +Y = north)                  */
static const int FDX[4] = {  0,  1,  0, -1 };  /* N E S W */
static const int FDY[4] = {  1,  0, -1,  0 };  /* N E S W */

/* Wall direction indices relative to player facing                             */
static const int WALL_FWD[4]   = { 0, 1, 2, 3 };
static const int WALL_BACK[4]  = { 2, 3, 0, 1 };
static const int WALL_LEFT[4]  = { 3, 0, 1, 2 };
static const int WALL_RIGHT[4] = { 1, 2, 3, 0 };
static const int TURN_R[4]     = { 1, 2, 3, 0 };
static const int TURN_L[4]     = { 3, 0, 1, 2 };

static const char * const FACING_NAME[4] = { "North", "East", "South", "West" };
static const char * const FACING_CHR[4]  = { "N",     "E",    "S",     "W"    };

/* -------------------------------------------------------------------------- */
/* Player                                                                       */
/* -------------------------------------------------------------------------- */
typedef struct {
    int  x, y;          /* map position (0-15) */
    int  facing;        /* DIR_N .. DIR_W */
    char message[512];  /* timed status message */
    int  msg_ticks;     /* countdown; 0 = silent */
} Player;

void player_init        (Player* p, int x, int y, int facing);
void player_set_message (Player* p, const char* msg, int ticks);
void player_tick        (Player* p);   /* decrement msg_ticks */

/* Movement -- returns 1 if moved, 0 if blocked */
int  player_forward  (Player* p, const Map* m);
int  player_backward (Player* p, const Map* m);
int  player_strafe_l (Player* p, const Map* m);
int  player_strafe_r (Player* p, const Map* m);
void player_turn_l   (Player* p);
void player_turn_r   (Player* p);

/* -------------------------------------------------------------------------- */
/* Condition bitmask constants (used by Character.condition and spells.c)      */
/* -------------------------------------------------------------------------- */
#define COND_GOOD        0x00
#define COND_ASLEEP      0x01
#define COND_BLINDED     0x02
#define COND_SILENCED    0x04
#define COND_DISEASED    0x08
#define COND_POISONED    0x10
#define COND_PARALYZED   0x20
#define COND_UNCONSCIOUS 0x40
#define COND_STONED      0xA0   /* stoned/petrified */
#define COND_DEAD        0xC0   /* dead             */
#define COND_ERADICATED  0xFF   /* eradicated       */

/* True if the character is permanently removed from play */
#define IS_DEADLIKE(c) ((c) == 0xFF || (c) == 0xA0 || (c) == 0xC0)
/* True if the character can take actions */
#define CAN_ACT(c) (!IS_DEADLIKE(c) && \
                    !((c) & (COND_ASLEEP | COND_PARALYZED | COND_UNCONSCIOUS)))

/* -------------------------------------------------------------------------- */
/* Party / Character  (minimal; full data parsed from ROSTER.DTA elsewhere)    */
/* -------------------------------------------------------------------------- */
#define MAX_PARTY 6

typedef struct {
    int  slot;       /* roster slot 0-17; -1 = empty */
    char name[16];
    int  cls;        /* 1=Knight 2=Paladin 3=Archer 4=Cleric 5=Sorcerer 6=Robber */
    int  race;       /* 1=Human  2=Elf  3=Dwarf  4=Gnome  5=Half-Orc */
    int  level;
    int  hp, hp_max;
    int  sp, sp_max;
    int  food;
    int  condition;  /* bitmask: COND_* constants above */
    int  xp;
    int  gold;
    int  stats[7];      /* 0=INT 1=MIG 2=PER 3=END 4=SPD 5=ACC 6=LCK */
    int  stats_base[7]; /* permanent base values — spells modify stats[], rest resets to base */
    int  resistances[8];/* 0=magic 1=fire 2=cold 3=elec 4=acid 5=fear 6=poison 7=sleep */
    int  ac;         /* armor class (lower = better, base 0) */
    int  gems;
    int  equipped[6];  /* item IDs, 0=empty */
    int  backpack[6];  /* item IDs, 0=empty */
    int  thievery;       /* Robber-class skill: lock-pick / disarm (scales with level) */
    int  alignment;      /* ALIGN_ANY=0, ALIGN_GOOD=1, ALIGN_NEUTRAL=2, ALIGN_EVIL=3 */
    int  equip_bonus[7];     /* stat bonuses from equipped items (not serialized) */
    int  equip_res_bonus[8]; /* resistance bonuses from equipped items (not serialized) */
    uint8_t char_quests[8]; /* per-character quest flags (ROSTER.DTA offsets 0x70-0x77) */
    uint8_t visit_flags;    /* visit tracking (ROSTER.DTA offset 0x7b) */
} Character;

/* Protection buffer indices */
#define PROT_FEAR       0
#define PROT_COLD       1
#define PROT_FIRE       2
#define PROT_POISON     3
#define PROT_ACID       4
#define PROT_ELEC       5
#define PROT_MAGIC      6
#define PROT_PHYSICAL   7
#define PROT_LIGHT      8
#define PROT_LEVITATE   9
#define PROT_WATERWALK  10
#define PROT_GUARDDOG   11
#define PROT_BLESS      12
#define PROT_INVIS      13
#define PROT_SHIELD     14
#define PROT_PSHIELD    15
#define PROT_LEATHER    16
#define PROT_CURSED     17

struct Party {
    Character members[MAX_PARTY];
    int       count;
    int       shared_gold;
    int       shared_food;
    int       protections[18]; /* active spell buffers, decremented on rest */
    /* Index: 0=fear, 1=cold, 2=fire, 3=poison, 4=acid, 5=elec,
              6=magic, 7=physical, 8=light(steps), 9=levitate, 10=waterWalk,
              11=guardDog, 12=bless, 13=invisibility, 14=shield,
              15=powerShield, 16=leatherSkin, 17=cursed */
};

void party_init(Party* party);

/* -------------------------------------------------------------------------- */
/* GameState                                                                    */
/* -------------------------------------------------------------------------- */
/* quest_flags bitmask:
 *   bit 0 (0x01): scroll delivered to Wizard Agar in Erliquin
 *   bit 1 (0x02): Agar's message delivered to Telgoran in Dusk
 *   bit 3 (0x08): White Wolf "Secret of Portsmith" hint collected
 *   bit 4 (0x10): White Wolf "Pirate's Secret Cove" hint collected
 *   bit 5 (0x20): King Alamar throne blessing received (+500 XP)
 *   bit 6 (0x40): King Alamar royal audience received (+200 XP)
 *   bit 7 (0x80): Enforcers dog-statue quest award received (+10000 XP)
 *   bit 8 (0x100): Pyramid level 1 (pp1) first-discovery XP awarded
 *   bit 9 (0x200): Pyramid level 2 (pp2) first-discovery XP awarded
 *  bit 10 (0x400): Pyramid level 3 (pp3) first-discovery XP awarded
 *  bit 11 (0x800): Pyramid level 4 (pp4) first-discovery XP awarded
 */
typedef struct {
    int    map_idx;
    int    running;
    int    show_cheatmap;
    int    cheat_combat;   /* 1 = auto-win every fight */
    int    quest_flags;    /* bitmask for quest progress (see above) */
    int    auto_search;    /* 1 = search tile after combat victory (default ON) */
    Player player;
    Party  party;
    char   data_dir[512];
    char   audio_dir[512];
    char   roster_path[512];
} GameState;

void game_state_init(GameState* gs, const char* data_dir, const char* audio_dir);
