# MM_HamsterOS_Port — Parity Roadmap

**Build:** `code=124KB → packed=54KB` on disk, `BSS=561KB` at runtime. Compressed with zlib deflate level 9.

---

## Done

- [x] Title sequence (SCREEN0-9, click/ESC skips to town select, music)
- [x] Title music (TITLE.WAV via SB16, PC speaker fallback)
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
- [x] S key in combat opens spell menu (Cleric/Sorcerer heals, damage, cure)
- [x] Loot after combat (items added to backpack, announced)
- [x] Monster sprite in combat (MONPIX.DTA centred over 3D view)
- [x] Level-up system (auto after combat, XP thresholds, HP/SP/END gains)
- [x] Death handling ("YOUR PARTY HAS FALLEN", respawn at OVR safe coords)
- [x] Character sheet (1-6 keys: stats, equip, backpack, conditions, XP progress)
- [x] Town select (6 options always shown; Continue greyed when no save)
- [x] Inn (requires food, charges gold, restores HP+SP)
- [x] Temple (cures all conditions, 100g)
- [x] Food shop (3 rations for 5g)
- [x] Tavern (shows OVR rumour strings for current map)
- [x] Training Hall (charges level x 1000g, applies level gains)
- [x] Blacksmith (browse town stock, B=buy, S=sell from backpack)
- [x] Starvation (HP damage every 15 steps at food=0)
- [x] Full-screen map overlay M key (16x16 grid with walls/doors/services/player)
- [x] Unlock doors U key (Robber 70%, others 20%)
- [x] F1 Options menu (music, sounds, cheat, auto-search, save, load, quit)
- [x] Quit confirmation modal
- [x] Quicksave P — batched FAT writes, only flags saved on verified success
- [x] Load saved game — Continue option updates within same session after save
- [x] Search X, Bash B
- [x] set_game_mode(true) — hides tray + header bar, suppresses all shell keys
- [x] All game data in single B:/MM1/ folder
- [x] Starting food (10 rations on new game)

---

## Remaining

- [ ] **Blacksmith sell from equipped** (sell directly from equipment slots)
- [ ] **Game music in dungeons/overworld** (game.wav, dungeon.wav via SB16 — need files)
- [ ] **Save slot select** (multiple named saves, choose on load)
- [ ] **MM icon** (generate MM.ICO with mkicon.py, add to floppy)
- [ ] **Add MM.APP to main HamsterOS floppy** (mkfreedosfloppy.py GAMES list)
- [ ] **Charsheet sell from equipped** (sell equipped items directly)

---

## Known Behaviour / Notes

- **Floppy writes (P to save)** take 1–3 seconds — QEMU simulates real floppy timing. Batch mode is used to minimise seeks; expect a brief pause.
- **Mouse cursor disappears** — HamsterOS redraws the backbuffer and re-composites the cursor each frame. During the ~5s floppy load (WALLPIX.DTA) the event loop is blocked, causing the cursor to vanish until load completes. Not fixable in MM.APP — it is an HamsterOS cooperative-multitasking limitation.
- **OVR tavern rumours** quality varies by map — shows first string ≥12 chars from the OVR data.
- **Characters start with 10 food rations** — buy more at food shops (5g = 3 rations).

---

## Build / Deploy

```powershell
$p = "e:\OneDrive\Mean Hamster Group\New Mean Hamster"
# Build
docker compose -f "$p\HamsterOS\compose.yaml" run --rm -v "${p}:/parent" builder bash -c 'cd /parent/MM/MM_HamsterOS_Port && make all HAMSTEROS_DIR=/parent/HamsterOS MMDATA_DIR=/parent/MM/Original_Source 2>&1'
# Deploy — copy to Compiled_Apps then rebuild hamster.img
Copy-Item "$p\MM\MM_HamsterOS_Port\dist\MM.APP" "$p\HamsterOS\build\Compiled_Apps\MM.APP" -Force
cd "$p\HamsterOS"; .\Make-hamster.bat
```
