# MM_HamsterOS_Port — Claude Instructions

## What this project is

A HamsterOS external `.APP` port of the Might and Magic I C engine from
`../MM_C_Port/`. The goal: a playable MM1 `.APP` that runs on HamsterOS,
loads game data from the hard disk (`C:/DOS/MM/`), and fits within
HamsterOS's 16-color VGA display and RAM constraints.

**Sibling repositories (always at these relative paths):**

| Path | What it is |
|------|------------|
| `../MM_C_Port/` | The reference Win32/GDI C port — all game logic lives here |
| `../HamsterOS/` | The OS we're targeting — SDK, compiler, ABI, build tools |

Read `../MM_C_Port/CLAUDE.md` for full engine documentation (data
formats, source file guide, what works, known issues).

Read `../HamsterOS/docs/HAMSTEROS_APP_SDK.md` for the full HamsterOS app
developer reference (HAMA format, three-file structure, HamsterHostAPI).

Read `../HamsterOS/CLAUDE.md` for platform architecture (build commands,
event loop, VGA backbuffer rules, memory model).

---

## Platform constraints — read before touching any code

| Constraint | Detail |
|------------|--------|
| Display | VGA mode 12h — 640×480, **16 colors**, planar backbuffer |
| Colors | EGA 16-color palette — **same as MM1's native palette. No color loss.** |
| CPU | 32-bit i386 protected mode; `-m32 -march=i386 -O2 -ffreestanding -fno-builtin` |
| libc | **None.** No `printf`, no `malloc`/`free`, no `string.h`, no `stdio.h` |
| Memory | ~600–700 KB heap free at idle on a 2 MB machine |
| Floppy | 1.44 MB total; ~200 KB free → game data **must load from C: drive** |
| Game data | `C:/DOS/MM/` on an HD-installed HamsterOS; that directory already exists after INSTALL.APP |
| Event model | **Callback-driven**, not a polling loop. OS pushes events via `draw()`, `scancode()`, `ptr_down/up/move()`, `update()` |
| State | All app state declared `static` in one file; BSS zeroed at load time |
| Heap | `heap_alloc(size)` / `heap_free(ptr)` through HamsterHostAPI only |

### The 16-color advantage

MM1 was designed for EGA 16 colors. VGA mode 12h also outputs exactly 16
colors using the same EGA palette. **No color quantization is needed.**
Every wall texture in `WALLPIX.DTA`, every monster sprite in `MONPIX.DTA`,
and all UI colors in `hud.c` already use EGA color indexes 0–15, which map
directly to HamsterOS VGA color indexes 0–15 (`VGA_COLOR_*` in `drivers/vga.h`).

---

## Architecture

### Three-file HamsterOS app structure

Every HamsterOS app needs exactly three source files:

```
src/mm_ext_entry.c    ← AppDescriptor + app_entry() — always same shape
src/mm_stubs.c        ← One thin wrapper per HamsterHostAPI function used
src/mm_port.c         ← All game state + coordinator logic (the big file)
```

See `../HamsterOS/apps/piano_ext_entry.c` and
`../HamsterOS/apps/piano_stubs.c` for complete working examples to copy
from. The SDK hello-world at `../HamsterOS/sdk/hello/` is the minimal
starting point.

### Rendering model

MM_C_Port renders into an in-memory pixel surface then blits it. HamsterOS
works the same way:

```
game renders → uint8_t render_buf[320*200]  (1 byte/pixel, color index 0–15)
     ↓
g_host->blit_opaque_pixels(dest_x, dest_y, render_buf,
                             stride=320, sx=0, sy=0, sw=320, sh=200,
                             scale=2)          ← pixel-doubled to 640×400
     ↓
g_host->present_region(window_x, window_y, window_w, window_h)
     ↓
VGA planar hardware — frame visible on screen
```

`blit_opaque_pixels` signature (from `../HamsterOS/kernel/app_abi.h`):
```c
void blit_opaque_pixels(int16_t dx, int16_t dy,
                         const uint8_t *pixels, uint16_t stride,
                         uint16_t sx, uint16_t sy, uint16_t sw, uint16_t sh,
                         uint8_t scale);
```

**Recommended config:** 320×200 source buffer with `scale=2` fills 640×400,
matching original MM1 aspect ratio. The remaining 80 rows at the bottom of
the 640×480 window hold the window chrome (title bar = 20px + border).

### No SDL2 shim needed

MM_C_Port's `sdl_compat_win32.c` was a Win32/GDI translation layer.
**Do not port it.** HamsterOS apps have no `WinMain` and no message pump.
Input arrives through `scancode()` and `ptr_*()` callbacks; animation runs
in `update()`; rendering runs in `draw()`.

---

## What to copy from MM_C_Port vs what to replace

### Copy verbatim (no Win32/SDL2 deps — just need malloc/free stubbed):

| MM_C_Port file | Changes needed |
|----------------|----------------|
| `game.c / .h` | Replace `malloc`/`free` with stubs; remove `stdio.h` |
| `combat.c / .h` | Same |
| `spells.c / .h` | Same |
| `events.c / .h` | Replace `printf` with `g_host->serial_str` |
| `services.c / .h` | Same |
| `roster.c / .h` | Same; file I/O via `fat_load`/`fat_save` |
| `items.c / .h` | Same |
| `monsters.c / .h` | Same |
| `mazedata.c / .h` | Same; load via `fat_load` not `fopen` |
| `ovr.c / .h` | Same |
| `rle.c / .h` | **No changes** — pure algorithm |
| `font.c / .h` | **No changes** — embedded CP437, no SDL2 |
| `wallpix.c / .h` | Replace `SDL_Surface` alloc with `heap_alloc`; draw into `render_buf` |
| `monpix.c / .h` | Same |
| `render3d.c / .h` | Adapt to write into `render_buf` instead of SDL_Surface |
| `hud.c / .h` | Adapt draw calls to write into `render_buf` |
| `dialog.c / .h` | Adapt to draw into `render_buf` or use `fill_rect`/`draw_text` |
| `charsheet.c / .h` | Same |

### Replace entirely:

| MM_C_Port file | Replacement |
|----------------|-------------|
| `main.c` | `mm_port.c` + `mm_ext_entry.c` |
| `audio.c / .h` | `speaker_note_on(hz)` + SB16 driver via stubs |
| `embedded_data.c / .h` | Load from FAT (`C:/DOS/MM/`) |
| `sdl_compat.h / sdl_compat_win32.c` | Not needed |
| `CMakeLists.txt`, `*.bat`, `resources.rc` | `Makefile` in this repo |

---

## HamsterHostAPI quick reference

Always call through `g_host` (set by `mm_stubs_set_host` in `mm_stubs.c`).
**Never call OS functions directly — only through this pointer.**

### Drawing

```c
g_host->fill_rect(x, y, w, h, color);
g_host->draw_text(x, y, "text", color, scale);       // 6×9 px font
g_host->draw_text_bold(x, y, "text", color, scale);
g_host->draw_border(x, y, w, h, color);              // 1-px rect outline
g_host->draw_line(x0, y0, x1, y1, color);
g_host->blit_opaque_pixels(dx, dy, pixels, stride, sx, sy, sw, sh, scale);
g_host->present_region(x, y, w, h);                  // flush backbuffer → VRAM
```

### VGA color indexes (same as EGA palette)

```
0 = BLACK       4 = RED         8 = DARK_GRAY     12 = BRIGHT_RED
1 = BLUE        5 = MAGENTA     9 = BRIGHT_BLUE   13 = BRIGHT_MAGENTA
2 = GREEN       6 = BROWN      10 = BRIGHT_GREEN  14 = YELLOW
3 = CYAN        7 = LIGHT_GRAY 11 = BRIGHT_CYAN   15 = WHITE
```

### Window management

```c
// init() — call wnd_init once
g_host->wnd_init(&g_frame, x, y, w, h, min_w, min_h);

// draw() — call wnd_draw_frame first, then render game content
g_host->wnd_draw_frame(&g_frame, "MIGHT & MAGIC I");
int16_t cx, cy, cw, ch;
g_host->wnd_content_rect(&g_frame, 0, &cx, &cy, &cw, &ch);
// ... blit game pixels into content area ...
int16_t bx, by, bw, bh;
g_host->wnd_display_bounds(&g_frame, &bx, &by, &bw, &bh);
g_host->present_region(bx, by, bw, bh);

// ptr_down() — dispatch window hits
WindowHit hit = g_host->wnd_hit_test(&g_frame, x, y);
// WINDOW_HIT_TITLE → wnd_begin_drag
// WINDOW_HIT_RESIZE → wnd_begin_resize
// WINDOW_HIT_CLOSE → mm_port_close()
// WINDOW_HIT_BODY → game click

// ptr_move() — during drag/resize
g_host->wnd_update_pointer(&g_frame, x, y);

// ptr_up() — end drag/resize
g_host->wnd_end_interaction(&g_frame);
```

### FAT filesystem

```c
// Load a file from C:/DOS/MM/:
// 1. Find the DOS/MM directory cluster (do once on open):
uint32_t mm_dir = 0;
fat_find_subdir_cluster(FAT_DRIVE_HOST, 0, "DOS", &mm_dir);       // get /DOS/
fat_find_subdir_cluster(FAT_DRIVE_HOST, mm_dir, "MM", &mm_dir);   // get /DOS/MM/

// 2. Load a file:
uint8_t *buf = g_host->heap_alloc(max_size);
uint32_t loaded = 0; bool trunc = false;
bool ok = g_host->fat_load(FAT_DRIVE_HOST, mm_dir, "MAZEDATA.DTA",
                             (char*)buf, max_size, &loaded, &trunc);
if (!ok) { g_host->serial_str("MAZEDATA.DTA load failed\n"); }
```

`fat_find_subdir_cluster` is not in HamsterHostAPI directly — use the FAT
traverse helpers or the `fat_find_dir` function available in `fs/fat.h`.
A simpler approach for first pass: load all needed files at open time
into heap buffers and keep them resident.

### Memory

```c
void *buf = g_host->heap_alloc(n_bytes);   // NULL on OOM
g_host->heap_free(buf);                    // call in close() for every alloc
```

Replace all `malloc`/`calloc`/`free` in copied files with these.
Simplest approach: add `-Dmalloc=mm_malloc -Dfree=mm_free` to CC_FLAGS
and provide the wrappers in `mm_stubs.c`.

### Timing and audio

```c
uint32_t ms = g_host->pit_millis();    // always valid, milliseconds since boot
g_host->speaker_note_on(440);          // PC speaker tone (hz)
g_host->speaker_note_off();
```

For SB16 music (optional, skip if driver not loaded):
```c
const DriverDescriptor *drv = driver_loader_find_audio(); // stub this
if (drv && drv->audio_is_present()) {
    drv->audio_play_wav(wav_data, wav_size, 0);
}
```

### Debug

```c
g_host->serial_str("msg\n");    // → COM1, visible in QEMU terminal
g_host->serial_dec16(val);
g_host->serial_char('X');
```

---

## Scancode → game input

HamsterOS passes raw PC keyboard make codes (set 1):

```c
// In mm_port_scancode(uint8_t sc):
switch (sc) {
    case 0x11: /* W */      move_forward(); break;
    case 0x1F: /* S */      move_back(); break;
    case 0x20: /* D / E */  strafe_right(); break;  // 0x12 = E
    case 0x10: /* Q */      turn_left(); break;
    case 0xCB: /* LEFT  */  turn_left(); break;
    case 0xCD: /* RIGHT */  turn_right(); break;
    case 0xC8: /* UP    */  move_forward(); break;
    case 0xD0: /* DOWN  */  move_back(); break;
    case 0x30: /* B */      bash(); break;
    case 0x2C: /* Z */      sleep_rest(); break;
    case 0x16: /* U */      unlock(); break;
    case 0x01: /* ESC */    dismiss(); break;
    case 0x3B: /* F1  */    options_menu(); break;
    case 0x32: /* M */      fullscreen_map(); break;
    case 0x19: /* P */      quicksave(); break;
    case 0x1C: /* Enter */  confirm(); break;
    case 0x02: /* 1 */      charsheet(0); break;
    /* 0x03..0x07 = 2..6 */
}
```

The full key table is in `../MM_C_Port/src/main.c` — port it directly.

---

## Memory budget

| Allocation | Size | Notes |
|------------|------|-------|
| Render buffer (320×200×1) | 64 KB | Software pixel buffer, heap_alloc |
| Wall sprite cache | ~60 KB | 18 walls × ~3 KB decoded |
| Active monster page | ~50 KB | Load on demand, free on map change |
| MAZEDATA.DTA (55 maps) | 28 KB | Keep loaded throughout game |
| OVR file (current map) | ~4 KB | Load/free per map |
| GameState struct | ~20 KB | Static BSS (zero cost) |
| **Peak total** | **~226 KB** | ~450 KB free on 2 MB machine |

**MONPIX.DTA is ~500 KB — never load it entirely.** Load individual
monster sprite pages on demand; free previous page when changing areas.

---

## Build instructions

Build uses the HamsterOS Docker image:

```sh
# From this directory:
docker compose -f ../HamsterOS/compose.yaml run --rm builder make MM.APP
```

Or use `build.bat` (Windows):
```bat
build.bat
```

Output: `dist/MM.APP`. To test it in HamsterOS:
```bat
copy dist\MM.APP ..\HamsterOS\build\Compiled_Apps\
cd ..\HamsterOS
make-hamster.bat
run-hamster.bat
```

The game belongs in `/GAMES/` on the floppy. Add it to
`../HamsterOS/tools/mkfreedosfloppy.py` alongside `FREECL.APP` and
`POKER.APP`.

---

## Port plan — work in this order

### Phase 1: Skeleton (compiles, opens a window)
1. Copy `../HamsterOS/sdk/hello/hello_ext_entry.c` → `src/mm_ext_entry.c`, rename symbols to `mm_port_*`
2. Copy `../HamsterOS/apps/piano_stubs.c` → `src/mm_stubs.c`, rename symbols; keep only stubs needed
3. Write `src/mm_port.c` skeleton: opens a window, draws "MIGHT & MAGIC I" in title, no game logic
4. Write `Makefile`; verify it compiles and runs in HamsterOS

### Phase 2: Render pipeline
5. `heap_alloc` a 320×200 `render_buf` in `mm_port_init`
6. Fill with test pattern; `blit_opaque_pixels(scale=2)` + `present_region` in `mm_port_draw`
7. Copy `font.c/.h` and `rle.c/.h` unchanged

### Phase 3: Game data loading
8. Implement `mm_load_game_data()`: fat_load MAZEDATA.DTA from `C:/DOS/MM/`
9. Copy `mazedata.c/.h` and `ovr.c/.h`; replace `fopen`/`fread` with fat_load buffer ops
10. Display first map name on screen to confirm

### Phase 4: Basic movement
11. Copy `game.c/.h`, `events.c/.h`, `items.c/.h`, `monsters.c/.h`
12. Replace `malloc`/`free` with `mm_malloc`/`mm_free` stubs; remove `#include <stdlib.h>`
13. Wire `mm_port_scancode()` to game movement functions
14. Confirm player moves around the starting map

### Phase 5: 3D rendering
15. Copy `wallpix.c/.h`; replace SDL_Surface alloc with `heap_alloc` + raw pixel array
16. Copy `render3d.c/.h`; adapt output target from SDL_Surface to `render_buf`
17. Copy `hud.c/.h`; adapt draw calls to write into `render_buf`
18. Confirm the 3D corridor view appears correctly

### Phase 6: Full game systems
19. Copy `combat.c/.h`, `spells.c/.h`, `roster.c/.h`, `services.c/.h`
20. Copy `dialog.c/.h`, `charsheet.c/.h`
21. Copy `monpix.c/.h` with on-demand loading
22. Wire up save/load via `fat_save`/`fat_load` (ROSTER.DTA)

### Phase 7: Audio
23. Map MM_C_Port's `SFX_*` frequency table to `speaker_note_on(hz)` calls
24. For music: try SB16 WAV playback via driver; silence is acceptable fallback

### Phase 8: Polish
25. Generate MM icon with `../HamsterOS/tools/mkicon.py`
26. Add `MM.ICO` + `MM.APP` to `/GAMES/` in `mkfreedosfloppy.py`
27. Add to `/TOOLBAR/` stub if desired

---

## Key gotchas

**No standard headers.** Replace every `#include <stdlib.h>`, `<stdio.h>`,
`<string.h>` with freestanding equivalents or remove them:
```c
/* In mm_stubs.c — expose to game files via mm_stubs.h: */
void *mm_malloc(uint32_t n) { return g_host->heap_alloc(n); }
void  mm_free(void *p)      { g_host->heap_free(p); }
```
Add to CC_FLAGS: `-Dmalloc=mm_malloc -Dfree=mm_free -Dprintf=mm_dbg_ignore`

**Minimal `memset`/`memcpy`.** HamsterOS has no libc. Write thin local
versions in `mm_port.c` or copy from `../HamsterOS/drivers/vga.c`'s
`fill_bytes`/`copy_bytes` (32-bit aligned, fast on i386).

**`update()` return value.** Return `true` only when animation state
changed and a redraw is needed. Returning `true` every frame causes
continuous full-window repaint.

**Always present the full window bounds.** After drag/resize the window
moves — always call `wnd_display_bounds` to get current bounds before
calling `present_region`. Never hardcode a position.

**`FAT_DRIVE_HOST` = C: drive.** `FAT_DRIVE_BOOT` = A: (floppy).
Game data is on C: in `DOS/MM/`.

**Keep the .APP lean.** Code only — no embedded game data. Target
< 80 KB so the floppy has room for the full games suite.
