# MM_HamsterOS_Port — Parity Roadmap

**Build:** `code=435KB → packed=182KB` on disk, `BSS=563KB` at runtime. Compressed with zlib deflate level 9.

---

## Done

- [x] Title sequence (SCREEN0-9 embedded; click/ESC skips to town select)
- [x] Title music (PC speaker kMM1Sound0 sequence, loops)
- [x] Wall rendering with correct per-wall EGA palette (TILE_COLORS)
- [x] 3D corridor view (240x132 sub-buf at scale=2 → 480x264 on screen)
- [x] Full-screen 640x440 layout at scale=1 (8px fonts)
- [x] MM1-style HUD (stone bevel buttons, command icons, move arrows, minimap, HP bars)
- [x] HUD buttons clickable (movement + Bash/Search/Unlock/Map/QuikRef/Save)
- [x] Condition indicators in party strip (coloured dot + PSN/PAR/KO/DIS/BLN/SLP)
- [x] Food counter in HUD side panel
- [x] All 55 maps + OVR scripts
- [x] Player movement + collision (WASD, arrows, strafe A/D)
- [x] Town entry message (map name on area change)
- [x] Large minimap with colour-coded service symbols
- [x] Compass + coordinates
- [x] Party strip with HP bars (colour-coded green/yellow/red)
- [x] Scripted tile events auto-show on step (OVR scripts AND tiles_all.inc manual table)
- [x] Y key + Enter both confirm Y/N prompts
- [x] Outdoor area grid edge crossing (4x5 overworld)
- [x] Scripted + random encounters (correct rates for towns vs dungeons vs outdoors)
- [x] Combat engine (real MM1 formulas, A/B/C target group selection)
- [x] Full spell system — all 49 spells dispatched (sleep, bless, raise dead, portal, food, escape/fly, protection, location, restore SP, levitate)
- [x] Non-combat spells filtered from combat menu; bless tracked with per-round attack bonus
- [x] Multiple monster sprites in combat (one per alive group, side-by-side layout)
- [x] HP bars per monster group + sleep indicator (Zzz) in combat list
- [x] Interactive combat item use (I key → backpack spell-items consumed)
- [x] SFX: MM_STEP on movement, MM_HIT after combat round with hits landed
- [x] SFX: MM_BUMP on wall, MM_VICTORY on win, MM_CHORD on level-up
- [x] Loot after combat (items added to backpack, announced)
- [x] Level-up system (auto after combat, XP thresholds, HP/SP/END gains)
- [x] Death handling ("YOUR PARTY HAS FALLEN", respawn in current town or Sorpigal)
- [x] Character sheet (1-6 keys: stats, equip, backpack, conditions, XP progress)
- [x] Town select (6 options always shown; Continue greyed when no save)
- [x] Inn (requires food, charges gold, restores HP+SP)
- [x] Temple (cures all non-eradicated conditions, flat 100g)
- [x] Food shop (3 rations for 5g)
- [x] Tavern (shows OVR rumour strings for current map)
- [x] Training Hall (charges level x 1000g, applies level gains)
- [x] Blacksmith (browse town stock, B=buy, S=sell from backpack)
- [x] Starvation (HP damage every 15 steps at food=0)
- [x] Full-screen map overlay M key (16x16 grid with walls/doors/services/player)
- [x] Unlock doors U key (Robber 70%, others 20%)
- [x] F1 Options menu (music, sounds, cheat, auto-search, save, load, quit)
- [x] Quit confirmation modal
- [x] Quicksave P — slot picker (3 named slots SAVE1-3.DAT / ROSTR1-3.DTA)
- [x] Load saved game — slot picker, map name shown per slot
- [x] Search X, Bash B
- [x] set_game_mode(true) — hides tray + header bar, suppresses all shell keys
- [x] All game data in single B:/MM1/ folder (mm1.img)
- [x] Starting food (10 rations on new game)
- [x] MAZEDATA.DTA + SCREEN0-9 embedded in .APP (zero startup floppy reads for read-only data)
- [x] Character creation — dice-roll (race base + class bonus + 3d6), accept/reroll, N key on town select
- [x] Tiered temple — Cure Conditions (50g) / Raise Dead (500g) / Resurrect Eradicated (1000g)
- [x] Floor trap damage — pit (1d10, Levitate bypasses), poison gas, acid, stalactites
- [x] Trapped chests — Robber disarm (level+d20 vs DC 14), 1d8 damage on fail
- [x] Bribe monsters — G key in combat, LCK roll vs DC 12, costs monster_level×10g
- [x] Tavern stat-boost drinks — 5g, rotating +1 to each stat in order (capped 25)
- [x] Shrine blessings — OVR keyword detection: stat/gem/heal/cure effects applied
- [x] Title cycling — SCREEN0-9 advance every 4 seconds
- [x] SFX defeat lament (MM_DEFEAT on party fallen screen)
- [x] SFX inn rest chord (MM_INN on successful rest)
- [x] Blacksmith E key: sell from equipped slots
- [x] Charsheet S key: sell from equipped slots
- [x] Multiple save slots (3 slots with picker UI on P and F1→L)
- [x] MM.ICO — sword icon generated with mkicon.py, deployed to B:/MM1/

---

## Remaining

All planned features complete.

---

## Known Behaviour / Notes

- **Floppy writes (P to save)** take 1–3 seconds — QEMU simulates real floppy timing. Batch mode minimises seeks; expect a brief pause.
- **OVR tavern rumours** quality varies by map — shows first string ≥12 chars from the OVR data.
- **Characters start with 10 food rations** — buy more at food shops (5g = 3 rations).

---

## Build / Deploy

```powershell
$p = "e:\OneDrive\Mean Hamster Group\New Mean Hamster"

# Build
docker compose -f "$p\HamsterOS\compose.yaml" run --rm -v "${p}:/parent" builder bash -c 'cd /parent/MM/MM_HamsterOS_Port && make all HAMSTEROS_DIR=/parent/HamsterOS MMDATA_DIR=/parent/MM/Original_Source 2>&1'

# Deploy to game disk (mm1.img) — MM.APP lives here alongside OVR/save files
docker compose -f "$p\HamsterOS\compose.yaml" run --rm -v "${p}:/parent" builder bash -c 'mdel -i /work/build/mm1.img ::MM1/MM.APP 2>/dev/null; mcopy -i /work/build/mm1.img /parent/MM/MM_HamsterOS_Port/dist/MM.APP ::MM1/MM.APP'
```

**Disk layout (`mm1.img` = B: drive in QEMU):**

```text
B:/MM1/
  MM.APP          ← game binary (177 KB packed)
  *.OVR  ×55      ← area scripts (loaded per map change)
  ROSTER.DTA      ← party data (read on open, written on save)
  SAVESTAT.DAT    ← position save (written on P-save)
```

MM.APP is **not** in `HamsterOS/build/Compiled_Apps/` — it ships on the game disk, not baked into the OS floppy.
