#pragma once
#include <stdint.h>
#include "game.h"

/* Load ROSTER.DTA from an in-memory buffer directly into game.h Party.
 * Returns number of characters found (0-18), or -1 on bad data. */
int roster_load_buf(const uint8_t *data, uint32_t data_size, Party *party);

/* Full 18-slot helpers used by the inn/town roster flow.  exists[i] is 1
 * when chars[i] contains a valid ROSTER.DTA character record. */
int roster_load_full_buf(const uint8_t *data, uint32_t data_size,
                         Character chars[18], uint8_t exists[18]);
void roster_build_full_buf(uint8_t *raw,
                           const Character chars[18], const uint8_t exists[18]);
