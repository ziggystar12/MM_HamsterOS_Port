#pragma once
#include <stdint.h>

/* Authentic Might & Magic I PC speaker sequences.
 * Reverse-engineered by ScummVM team (GPLv3).
 * Format: {hz, ms} — hz=0 is a rest. */

typedef struct { uint16_t hz; uint16_t ms; } MusicNote;

/* Convert PIT sequence entry: ticks*55ms, 1193182/divisor Hz */
#define P(t,d) {(d)?(uint16_t)(1193182u/(d)):0u,(uint16_t)((t)*55u)}

/* ── kMM1Sound0 — title theme (loops throughout entire game) ── */
#define MM_TITLE_LEN 83
extern const MusicNote MM_TITLE[MM_TITLE_LEN];

/* ── kMM1Sound3 — victory / monster defeated fanfare ── */
#define MM_VICTORY_LEN 49
extern const MusicNote MM_VICTORY[MM_VICTORY_LEN];

/* ── kMM1Sound1 — wall bump / blocked ── */
#define MM_BUMP_LEN 5
extern const MusicNote MM_BUMP[MM_BUMP_LEN];

/* ── kMM1Sound6 — hit / spell result (single high beep) ── */
#define MM_HIT_LEN 1
extern const MusicNote MM_HIT[MM_HIT_LEN];

/* ── kMM1Sound8 — chord (level-up / item found) ── */
#define MM_CHORD_LEN 6
extern const MusicNote MM_CHORD[MM_CHORD_LEN];

/* ── kMM1Sound9 — staccato (step / search) ── */
#define MM_STEP_LEN 9
extern const MusicNote MM_STEP[MM_STEP_LEN];

/* ── Defeat lament — slow descending (party fallen screen) ── */
#define MM_DEFEAT_LEN 8
extern const MusicNote MM_DEFEAT[MM_DEFEAT_LEN];

/* ── Inn rest chord — gentle ascending (successful rest) ── */
#define MM_INN_LEN 5
extern const MusicNote MM_INN[MM_INN_LEN];

#undef P