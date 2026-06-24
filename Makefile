# MM_HamsterOS_Port — builds MM.APP using the HamsterOS Docker builder
# Run: docker compose -f ../HamsterOS/compose.yaml run --rm builder make MM.APP
# Or:  build.bat (Windows shortcut)

HAMSTEROS_DIR := ../HamsterOS
MMCPORT_DIR   := ../MM_C_Port

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
          -Wall -Wextra -O2 \
          -Dmalloc=mm_malloc -Dfree=mm_free

# Source files from this repo
LOCAL_SRCS := \
    src/mm_port.c \
    src/mm_stubs.c

# Source files copied from MM_C_Port (add as you port each phase)
# GAME_SRCS := \
#     src/game.c \
#     src/rle.c \
#     src/font.c \
#     src/mazedata.c \
#     src/ovr.c

ALL_SRCS := $(LOCAL_SRCS)

OBJS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(ALL_SRCS))
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
	    --elf $(ELF) --name MM --output $(DIST_DIR)/MM.APP
	@echo "MM.APP built: $(DIST_DIR)/MM.APP"

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
