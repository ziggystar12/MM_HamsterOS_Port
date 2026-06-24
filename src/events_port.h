#pragma once
#include <stdint.h>
#include "game.h"
#include "ovr.h"

typedef enum {
    SVC_NONE=0, SVC_INN, SVC_TEMPLE, SVC_TRAINING, SVC_BLACKSMITH,
    SVC_FOOD, SVC_TAVERN, SVC_STAIRS, SVC_STAIRS_UP, SVC_EXIT,
    SVC_PORTAL, SVC_TELEPORT, SVC_TRAP, SVC_FLOOR_TRAP, SVC_SHRINE,
    SVC_ENCOUNTER, SVC_PRISONER, SVC_LEVER, SVC_SPECIAL,
    SVC_NPC, SVC_MESSAGE
} ServiceType;

ServiceType detect_service(const char *text);

ServiceType peek_tile_event(const GameState *gs, const char *ovr_text,
                            const char **out_text);

/* Handle tile interaction (Enter key). Returns new map_idx (may change on transitions).
 * Sets player message for display. */
int handle_tile_event(GameState *gs, const OvrFile *ovr, const char *ovr_text);

/* Auto-fire floor traps after movement. */
void handle_floor_trap(GameState *gs, const OvrFile *ovr);

/* Outdoor area edge crossing. Returns new map_idx or -1. */
int area_edge_transition(const GameState *gs, const struct Map *m,
                         int *new_x, int *new_y);

/* Service symbol at tile for minimap. */
ServiceType tile_service_at(int map_idx, int x, int y);

#define NUM_TOWN_DUNGEONS 5
#define NUM_TOWN_EXITS    5
extern const int TOWN_DUNGEON[NUM_TOWN_DUNGEONS][2];
extern const int TOWN_EXIT[NUM_TOWN_EXITS][2];