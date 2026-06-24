/*
 * game.c -- Player state, movement, collision, and GameState init.
 *           Part of the Might and Magic I C port.
 *
 * Depends on:
 *   mazedata.h  -- Map type and can_pass()
 *   game.h      -- Player, Party, GameState types
 */

#include "game.h"
#include "mazedata.h"  /* Map, can_pass() */
#include "mm_util.h"

/* ========================================================================== */
/* Player                                                                       */
/* ========================================================================== */

void player_init(Player* p, int x, int y, int facing)
{
    p->x         = x;
    p->y         = y;
    p->facing    = facing;
    p->msg_ticks = 0;
    p->message[0]= '\0';
}

void player_set_message(Player* p, const char* msg, int ticks)
{
    mm_strncpy(p->message, msg, sizeof(p->message));
    
    p->msg_ticks = ticks;
}

void player_tick(Player* p)
{
    if (p->msg_ticks > 0)
        p->msg_ticks--;
}

/* -------------------------------------------------------------------------- */
/* Internal: attempt movement from current cell using given wall direction.    */
/* dx, dy are the target-relative offsets; wall_dir is the WALL_* index.      */
/* Returns 1 if moved, 0 if blocked.                                           */
/* -------------------------------------------------------------------------- */
static int try_move(Player* p, const Map* m, int dx, int dy, int wall_dir)
{
    if (can_pass(m, p->x, p->y, wall_dir)) {
        p->x += dx;
        p->y += dy;
        /* Clamp to valid range — out-of-bounds steps on outdoor maps
         * are handled by area_edge_transition in the game loop. */
        if (p->x < 0) p->x = 0;
        if (p->x >= MAP_SIZE) p->x = MAP_SIZE - 1;
        if (p->y < 0) p->y = 0;
        if (p->y >= MAP_SIZE) p->y = MAP_SIZE - 1;
        return 1;
    }
    player_set_message(p, "Blocked!", 80);
    return 0;
}

int player_forward(Player* p, const Map* m)
{
    return try_move(p, m,
                    FDX[p->facing], FDY[p->facing],
                    WALL_FWD[p->facing]);
}

int player_backward(Player* p, const Map* m)
{
    return try_move(p, m,
                    -FDX[p->facing], -FDY[p->facing],
                    WALL_BACK[p->facing]);
}

int player_strafe_l(Player* p, const Map* m)
{
    int lf = TURN_L[p->facing];
    return try_move(p, m, FDX[lf], FDY[lf], WALL_FWD[lf]);
}

int player_strafe_r(Player* p, const Map* m)
{
    int rf = TURN_R[p->facing];
    return try_move(p, m, FDX[rf], FDY[rf], WALL_FWD[rf]);
}

void player_turn_l(Player* p)
{
    p->facing = TURN_L[p->facing];
}

void player_turn_r(Player* p)
{
    p->facing = TURN_R[p->facing];
}

/* ========================================================================== */
/* Party                                                                        */
/* ========================================================================== */

void party_init(Party* party)
{
    mm_memset(party, 0, sizeof(*party));
    for (int i = 0; i < MAX_PARTY; i++)
        party->members[i].slot = -1;
}

/* ========================================================================== */
/* GameState                                                                    */
/* ========================================================================== */

/* Town start positions: {map_idx, x, y, facing}
 * Positions are each map's OVR safe_x/safe_y.
 * OVR_NAMES order: 0=sorpigal 1=portsmit 2=algary 3=dusk 4=erliquin */
static const struct { int map, x, y, f; } TOWN_STARTS[] = {
    { 0,  8,  3, DIR_N },  /* Sorpigal  map 0 */
    { 1, 11,  8, DIR_E },  /* Portsmith map 1 */
    { 2,  4,  4, DIR_N },  /* Algary    map 2 */
    { 3,  3, 12, DIR_W },  /* Dusk      map 3 */
    { 4, 14,  8, DIR_W },  /* Erliquin  map 4 */
};

void game_state_init(GameState* gs, const char* data_dir, const char* audio_dir)
{
    mm_memset(gs, 0, sizeof(*gs));
    gs->map_idx        = 0;
    gs->running        = 1;
    gs->show_cheatmap  = 0;
    gs->auto_search    = 1;  /* ON by default */

    (void)data_dir;  /* path unused on HamsterOS */
    (void)audio_dir;

    /* Default start: Sorpigal (map 0) */
    player_init(&gs->player, TOWN_STARTS[0].x, TOWN_STARTS[0].y, TOWN_STARTS[0].f);
    party_init(&gs->party);
}

/* Set start position for one of the 5 towns (0-4) */
void game_state_set_town(GameState* gs, int town_idx)
{
    if (town_idx < 0 || town_idx > 4) town_idx = 0;
    const int idx = town_idx;  /* silence warning */
    gs->map_idx = TOWN_STARTS[idx].map;
    player_init(&gs->player,
                TOWN_STARTS[idx].x,
                TOWN_STARTS[idx].y,
                TOWN_STARTS[idx].f);
}
