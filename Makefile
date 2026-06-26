# MM_HamsterOS_Port — builds MM.APP using the HamsterOS Docker builder
# Run: docker compose -f ../HamsterOS/compose.yaml run --rm builder make MM.APP
# Or:  build.bat (Windows shortcut)

HAMSTEROS_DIR := ../HamsterOS
MMCPORT_DIR   := ../MM_C_Port
MMDATA_DIR    := /parent/MM/Original_Source

CC  := gcc
LD  := ld
BUILD_DIR   := build
DIST_DIR    := dist
ELF         := $(BUILD_DIR)/mm_app.elf

# Compiler flags must match HamsterOS exactly
CFLAGS := -m32 -march=i386 -std=c11 -ffreestanding -fno-builtin \
          -fno-pic -fno-pie -fno-stack-protector \
          -fno-asynchronous-unwind-tables \
          -I$(HAMSTEROS_DIR)/kernel \
          -I$(HAMSTEROS_DIR)/drivers \
          -I$(HAMSTEROS_DIR)/apps \
          -I$(HAMSTEROS_DIR)/fs \
          -Isrc \
          -Wall -Wextra -Wno-unused-variable -Wno-missing-field-initializers -O2 \
          -Dmalloc=mm_malloc -Dfree=mm_free

# ── Embedded binary data (generated from Original_Source/) ──
# Baking WALLPIX + MONPIX + MAZEDATA + SCREEN files eliminates all
# floppy-load freezes. Only ROSTER.DTA and SAVESTAT.DAT stay on floppy.
WALLPIX_SRC  := src/wallpix_data.c
MONPIX_SRC   := src/monpix_data.c
MAZEDATA_SRC := src/mazedata_data.c
SCREEN_SRC   := src/screen_data.c

$(WALLPIX_SRC): | $(BUILD_DIR)
	@echo "Embedding WALLPIX.DTA..."
	@python3 -c "d=open('$(MMDATA_DIR)/WALLPIX.DTA','rb').read(); f=open('$@','w'); f.write('#include <stdint.h>\nconst uint8_t wallpix_dta_data[]={'+','.join(str(b) for b in d)+'};\nconst uint32_t wallpix_dta_size=%d;\n'%len(d))"

$(MONPIX_SRC): | $(BUILD_DIR)
	@echo "Embedding MONPIX.DTA..."
	@python3 -c "d=open('$(MMDATA_DIR)/MONPIX.DTA','rb').read(); f=open('$@','w'); f.write('#include <stdint.h>\nconst uint8_t monpix_dta_data[]={'+','.join(str(b) for b in d)+'};\nconst uint32_t monpix_dta_size=%d;\n'%len(d))"

$(MAZEDATA_SRC): | $(BUILD_DIR)
	@echo "Embedding MAZEDATA.DTA..."
	@python3 -c "d=open('$(MMDATA_DIR)/MAZEDATA.DTA','rb').read(); f=open('$@','w'); f.write('#include <stdint.h>\nconst uint8_t mazedata_dta_data[]={'+','.join(str(b) for b in d)+'};\nconst uint32_t mazedata_dta_size=%d;\n'%len(d))"

$(SCREEN_SRC): | $(BUILD_DIR)
	@echo "Embedding SCREEN0-9..."
	@python3 -c "import os; D='$(MMDATA_DIR)'; fs=[open(os.path.join(D,'SCREEN%d'%i),'rb').read() if os.path.exists(os.path.join(D,'SCREEN%d'%i)) else b'' for i in range(10)]; raw=b''.join(fs); off=[sum(len(x) for x in fs[:i]) for i in range(10)]; f=open('$@','w'); f.write('#include <stdint.h>\nconst uint8_t screen_dta_data[]={'+(','.join(str(b) for b in raw) if raw else '0')+'};\nconst uint32_t screen_dta_offsets[10]={'+','.join(str(o) for o in off)+'};\nconst uint32_t screen_dta_sizes[10]={'+','.join(str(len(x)) for x in fs)+'};\n')"

# Source files from this repo
LOCAL_SRCS := \
    src/mm_port.c \
    src/mm_stubs.c

# Phase 2: render support
RENDER_SRCS := \
    src/rle.c \
    src/font.c

# Phase 3: map data
DATA_SRCS := \
    src/mazedata.c

# Phase 4-5: game logic + rendering + music
GAME_SRCS := \
    src/game.c \
    src/wallpix.c \
    src/render3d.c \
    src/hud_port.c \
    src/roster_port.c \
    src/items.c \
    src/monsters.c \
    src/spells.c \
    src/ovr.c \
    src/events_port.c \
    src/combat_port.c \
    src/screen_port.c \
    src/monpix_port.c \
    src/mm_music.c \
    $(WALLPIX_SRC) \
    $(MONPIX_SRC) \
    $(MAZEDATA_SRC) \
    $(SCREEN_SRC)

ALL_SRCS := $(LOCAL_SRCS) $(RENDER_SRCS) $(DATA_SRCS) $(GAME_SRCS)

# Object files — generated files go to build/
OBJS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(filter src/%,$(ALL_SRCS)))

# Entry point must be first in linker command
ENTRY_OBJ := $(BUILD_DIR)/mm_entry.o

.PHONY: all MM.APP clean

all: MM.APP

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(DIST_DIR):
	mkdir -p $(DIST_DIR)

$(BUILD_DIR)/mm_entry.o: src/mm_ext_entry.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(ENTRY_OBJ) $(OBJS) $(HAMSTEROS_DIR)/tools/app.ld
	$(LD) -T $(HAMSTEROS_DIR)/tools/app.ld -melf_i386 --emit-relocs -nostdlib \
	    $(ENTRY_OBJ) $(OBJS) -o $@
	@echo "ld: $(ELF) built"

MM.APP: $(ELF) | $(DIST_DIR)
	python3 $(HAMSTEROS_DIR)/tools/mkapp.py \
	    --elf $(ELF) --name MM --open-launch-at --output $(DIST_DIR)/MM.APP
	@echo "MM.APP built: $(DIST_DIR)/MM.APP"

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR) $(WALLPIX_SRC) $(MONPIX_SRC) $(MAZEDATA_SRC) $(SCREEN_SRC)
