#pragma once
#include <stdint.h>
#include "game.h"

/* Load ROSTER.DTA from an in-memory buffer directly into game.h Party.
 * Returns number of characters found (0-18), or -1 on bad data. */
int roster_load_buf(const uint8_t *data, uint32_t data_size, Party *party);