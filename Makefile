# Kryptids in Space: The Sneaky Kryptid - Game Boy Color (512 KB, MBC5)
GBDK_HOME ?= $(HOME)/tools/gbdk
LCC = $(GBDK_HOME)/bin/lcc

NAME   = sneaky_kryptid
TITLE  = SNEAKYKRYPT
SRC    = src/main0.c src/core.c src/sound.c src/sound_data.c src/script_data.c src/stage_defs.c \
         src/objs.c src/stage.c src/s1.c src/s2.c src/s3.c src/s4.c src/scenes.c \
         $(wildcard src/assets_b*.c)
GEN    = src/assets.h src/sound_data.c src/songs.h src/script_data.c src/script_ids.h src/stage_defs.c

# -Wm-yC CGB only; -Wm-yo32 thirty-two 16 KB ROM banks (512 KB)
CFLAGS = -Wf--opt-code-size -Wa-l -Wl-m -Wl-j -Wm-yC -Wm-yn"$(TITLE)" -Wm-yo32 -autobank -Isrc

# Rejected By The Machines studio card: its own module, compiled for speed, autobanked
CARD   = studio_card studio_card_home studio_card_gfx studio_card_scx
CARD_O = $(CARD:%=build/card/%.o)
build/card/%.o: src/studio_card/%.c src/studio_card/studio_card.h
	@mkdir -p build/card
	$(LCC) -Wa-l -Wm-yC -autobank -Wf--opt-code-speed -c -o $@ $<

all: build/$(NAME).gbc build/$(NAME)_nosave.gbc

# Generated sources (src/assets*, sound_data.c, script_data.c, stage_defs.c) are committed,
# so a normal build needs only GBDK. Regenerate after editing art/music/script:
#   make assets      (Python 3 + Pillow + numpy; portrait/title art also needs ref/IMG_50xx.png)
#   make script      (dialog only)   make music   (songs only)
.PHONY: all assets script music clean
assets:
	python3 tools/gen_assets.py
script:
	python3 tools/gen_script.py
music:
	python3 tools/gen_sound.py

# MBC5 + RAM + BATTERY (0x1B): keeps high scores
build/$(NAME).gbc: $(SRC) $(CARD_O) src/core.h src/game.h src/sound.h src/objs.h src/stage_def.h
	mkdir -p build
	$(LCC) $(CFLAGS) -Wm-ya1 -Wm-yt0x1B -o $@ $(SRC) $(CARD_O)

# plain MBC5 (0x19): same game, no save
build/$(NAME)_nosave.gbc: $(SRC) $(CARD_O) src/core.h src/game.h src/sound.h src/objs.h src/stage_def.h
	mkdir -p build
	$(LCC) $(CFLAGS) -Wm-yt0x19 -o $@ $(SRC) $(CARD_O)

clean:
	rm -rf build src/*.o src/*.lst src/*.asm src/*.sym src/studio_card/*.lst

.PHONY: all clean
