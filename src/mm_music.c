#include "mm_music.h"

#define P(t,d) {(d)?(uint16_t)(1193182u/(d)):0u,(uint16_t)((t)*55u)}

/* kMM1Sound0 — title theme (83 entries, loops) */
const MusicNote MM_TITLE[MM_TITLE_LEN] = {
    P(1,0x0a98),P(1,0x0970),P(1,0x08e8),P(1,0x07ef),
    P(14,0x0712),P(2,0),P(6,0x0712),P(2,0),
    P(6,0x0712),P(1,0),P(1,0x0e24),P(1,0x0a98),
    P(6,0x08e8),P(2,0),P(4,0x0a98),P(4,0x0970),
    P(4,0x08e8),P(3,0x07ef),P(1,0),P(3,0x0712),
    P(1,0),P(2,0x06ac),P(1,0x08e8),P(1,0x0712),
    P(8,0x05f2),P(4,0x06ac),P(4,0x0712),P(8,0x06ac),
    P(4,0x0712),P(2,0x07ef),P(1,0x12e0),P(1,0x0b3a),
    P(12,0x0712),P(4,0x06ac),P(4,0x0712),P(3,0x07ef),
    P(1,0),P(3,0x08e8),P(1,0),P(3,0x0970),
    P(1,0),P(14,0x0a98),P(2,0),P(6,0x0e24),
    P(2,0),P(5,0x0a98),P(1,0),P(1,0x0e24),
    P(1,0x0be4),P(3,0x08e8),P(1,0),P(3,0x0be4),
    P(1,0),P(2,0x08e8),P(2,0x0970),P(2,0x08e8),
    P(2,0x07ef),P(14,0x0712),P(2,0),P(2,0x06ac),
    P(2,0x0712),P(2,0x07ef),P(2,0x08e8),P(2,0x0712),
    P(2,0x07ef),P(2,0x08e8),P(2,0x0970),P(2,0x07ef),
    P(2,0x08e8),P(2,0x0970),P(2,0x0a98),P(2,0x08e8),
    P(2,0x0970),P(2,0x0a98),P(2,0x0b3a),P(4,0x06ac),
    P(5,0x0712),P(6,0x07ef),P(7,0x08e8),P(2,0x0e24),
    P(3,0x0a98),P(32,0x0868),P(4,0)
};

/* kMM1Sound3 — victory / monster defeated (49 entries) */
const MusicNote MM_VICTORY[MM_VICTORY_LEN] = {
    P(1,0x11d1),P(1,0x0be4),P(3,0x0712),P(6,0),
    P(2,0x0712),P(1,0),P(12,0x0712),P(6,0),
    P(3,0x0712),P(3,0),P(2,0x06ac),P(2,0),
    P(2,0x0be4),P(2,0),P(2,0x06ac),P(1,0x11d1),
    P(1,0x0be4),P(3,0x0712),P(6,0),P(2,0x0712),
    P(1,0),P(12,0x0712),P(6,0),P(3,0x0712),
    P(2,0),P(1,0x17c8),P(1,0x0be4),P(2,0x07ef),
    P(2,0),P(2,0x08e8),P(2,0),P(2,0x07ef),
    P(2,0),P(1,0x0e24),P(1,0x0be4),P(3,0x08e8),
    P(6,0),P(2,0x0be4),P(1,0),P(1,0x8e84),
    P(1,0x5f1e),P(1,0x4742),P(1,0x2f8f),P(1,0x23a2),
    P(1,0x17c8),P(1,0x11d1),P(1,0x0e24),P(1,0x0be4),
    P(12,0x08e8)
};

/* kMM1Sound1 — wall bump / blocked: single 180 Hz, 80ms */
const MusicNote MM_BUMP[MM_BUMP_LEN] = { {180, 80} };

/* hit landed: single 220 Hz, 120ms */
const MusicNote MM_HIT[MM_HIT_LEN] = { {220, 120} };

/* monster slain: single 440 Hz, 180ms */
const MusicNote MM_KILL[MM_KILL_LEN] = { {440, 180} };

/* kMM1Sound8 — chord: level-up / item find (6 entries) */
const MusicNote MM_CHORD[MM_CHORD_LEN] = {
    P(8,0x0b3a),P(8,0x0be4),P(8,0x0c98),P(8,0x0be4),P(2,0),P(16,0x0d59)
};

/* footstep: single low 120 Hz, 40ms blip (NOT a tune) */
const MusicNote MM_STEP[MM_STEP_LEN] = { {120, 40} };

/* Defeat lament — slow descending from C4 down to low C (party fallen) */
const MusicNote MM_DEFEAT[MM_DEFEAT_LEN] = {
    P(16,0x11d1),P(4,0),          /* C4  880ms + rest */
    P(12,0x1540),P(4,0),          /* A3  660ms + rest */
    P(12,0x1b40),P(4,0),          /* F3  660ms + rest */
    P(24,0x23a2),P(8,0)           /* C3 1320ms + final rest */
};

/* Inn rest chord — gentle ascending G4→B4→C5 (successful inn rest) */
const MusicNote MM_INN[MM_INN_LEN] = {
    P(6,0x0be4),P(2,0),           /* G4 */
    P(6,0x0970),P(2,0),           /* B4 */
    P(12,0x08e8)                  /* C5 */
};

#undef P