# Kryptids in Space: The Sneaky Kryptid - Game Boy Color (512 KB, MBC5)
GBDK_HOME ?= $(HOME)/tools/gbdk
LCC = $(GBDK_HOME)/bin/lcc

NAME   = sneaky_kryptid
TITLE  = SNEAKYKRYPT
SRC    = src/main0.c src/core.c src/sound.c src/script_data.c src/script_scenes.c src/stage_defs.c \
         src/objs.c src/stage.c src/s1.c src/s2.c src/s3.c src/s4.c src/cockpit.c src/scenes.c \
         $(wildcard src/assets_b*.c) $(wildcard src/cockpit_b*.c) $(wildcard src/music/*.c) $(wildcard src/weapon_fx2_b*.c)
OBJ    = $(SRC:src/%.c=build/obj/%.o)
HDR    = $(wildcard src/*.h)

# -Wm-yC CGB only; -Wm-yo32 thirty-two 16 KB ROM banks (512 KB)
CFLAGS = -Wf--opt-code-size -Wa-l -Wl-m -Wl-j -Wm-yC -Wm-yn"$(TITLE)" -Wm-yo32 -autobank -Isrc -Isrc/hUGE

# Rejected By The Machines studio card: its own module, compiled for speed, autobanked
CARD   = studio_card studio_card_home studio_card_gfx studio_card_scx
CARD_O = $(CARD:%=build/card/%.o)
# hUGEDriver (music), prebuilt from third_party/hUGEDriver
CARD_O += src/hUGE/hUGEDriver.o
build/card/%.o: src/studio_card/%.c src/studio_card/studio_card.h
	@mkdir -p build/card
	$(LCC) -Wa-l -Wm-yC -autobank -Wf--opt-code-speed -c -o $@ $<

all: build/$(NAME).gbc build/$(NAME)_nosave.gbc

# Generated sources (src/assets*, cockpit_b*, src/music/*.c, script_*.c, stage_defs.c) are committed,
# so a normal build needs only GBDK. Regenerate after editing art/music/script:
#   make assets      (Python 3 + Pillow + numpy; portrait/title art also needs ref/IMG_50xx.png)
#   make cockpit     (mission 5 art; Python 3 + Pillow + numpy)
#   make script      (dialog only)
#   make music       (songs: music/*.uge from tools/music/compose.py, then C for hUGEDriver)
#   make weaponfx    (weapon colours + shot/effect sprites -> src/weapon_fx.h; also prints the colour-blind check)
.PHONY: all assets script music cockpit weaponfx clean
assets:
	python3 tools/gen_assets.py
cockpit:
	python3 tools/gen_cockpit.py
script:
	python3 tools/gen_script.py
music:
	python3 tools/music/compose.py
	python3 tools/music/build_music.py
weaponfx:
	python3 tools/gen_weapon_fx.py
	python3 tools/gen_weapon_fx2.py
	python3 tools/cvd_check.py

build/obj/%.o: src/%.c $(HDR)
	@mkdir -p $(dir $@)
	$(LCC) $(CFLAGS) -c -o $@ $<

# MBC5 + RAM + BATTERY (0x1B): keeps high scores
build/$(NAME).gbc: $(OBJ) $(CARD_O)
	$(LCC) $(CFLAGS) -Wm-ya1 -Wm-yt0x1B -o $@ $(OBJ) $(CARD_O)

# plain MBC5 (0x19): same game, no save
build/$(NAME)_nosave.gbc: $(OBJ) $(CARD_O)
	$(LCC) $(CFLAGS) -Wm-yt0x19 -o $@ $(OBJ) $(CARD_O)

# test build: every mission selectable from the start (same save format as the demo)
.PHONY: unlocked
unlocked: build/$(NAME)_unlocked.gbc
build/obj/scenes_unlocked.o: src/scenes.c $(HDR)
	@mkdir -p build/obj
	$(LCC) $(CFLAGS) -Wf-DUNLOCK_ALL -c -o $@ $<
build/$(NAME)_unlocked.gbc: $(OBJ) $(CARD_O) build/obj/scenes_unlocked.o
	$(LCC) $(CFLAGS) -Wm-ya1 -Wm-yt0x1B -o $@ $(filter-out build/obj/scenes.o,$(OBJ)) build/obj/scenes_unlocked.o $(CARD_O)

# Flatwoods Drift on its own: the gun-cam mission as a standalone demo (title -> pilot -> mission 5)
.PHONY: flatwoods
flatwoods: build/flatwoods_drift.gbc build/flatwoods_drift_nosave.gbc
build/obj/scenes_flatwoods.o: src/scenes.c $(HDR)
	@mkdir -p build/obj
	$(LCC) $(CFLAGS) -Wf-DFLATWOODS_DEMO -c -o $@ $<
FW_OBJ = $(filter-out build/obj/scenes.o,$(OBJ)) build/obj/scenes_flatwoods.o $(CARD_O)
FW_CF  = $(filter-out -Wm-yn"$(TITLE)",$(CFLAGS)) -Wm-yn"FLATWOODS"
build/flatwoods_drift.gbc: $(OBJ) $(CARD_O) build/obj/scenes_flatwoods.o
	$(LCC) $(FW_CF) -Wm-ya1 -Wm-yt0x1B -o $@ $(FW_OBJ)
build/flatwoods_drift_nosave.gbc: $(OBJ) $(CARD_O) build/obj/scenes_flatwoods.o
	$(LCC) $(FW_CF) -Wm-yt0x19 -o $@ $(FW_OBJ)

# sound test ROM: every song and effect through the game's sound engine (tools/music/jukebox)
.PHONY: jukebox
jukebox: build/jukebox.gbc
build/jukebox.gbc: tools/music/jukebox/jukebox.c build/obj/sound.o $(filter build/obj/music/%,$(OBJ)) src/hUGE/hUGEDriver.o
	$(LCC) -Wm-yC -Wm-yn"KRYPTIDSND" -Wm-yo32 -Wm-yt0x19 -Isrc -Isrc/hUGE -Wl-j -o $@ tools/music/jukebox/jukebox.c build/obj/sound.o $(filter build/obj/music/%,$(OBJ)) src/hUGE/hUGEDriver.o

clean:
	rm -rf build src/*.o src/*.lst src/*.asm src/*.sym src/studio_card/*.lst

# lcc keeps its intermediate files next to each other: parallel jobs (make -j) corrupt objects
.NOTPARALLEL:
