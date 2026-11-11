# Rejected By The Machines: Studio Card (X1, final)

**This is the studio card every game uses.** Standard teal/red colours, stamped
lettering. Final as of 2026-11-11.
`rom/studio_card_test.gbc` (SHA-256 in `rom/SHA256SUMS`) is the approved reference
build, and `rom/RBTM_studio_card_ROM_SameBoy.mp4` shows it running on CGB-accurate SameBoy.

**Quick start for a new game:** copy the 9 files from `src/` (every file except
`main.c`) into your game, compile them with `-autobank`, link at least 64 KB
(`-Wl-yo4`), and call `studio_card_play();` first thing in `main()`. Details below.

---

## Module overview

A drop-in GBDK-2020 module that plays the studio card before your game:
static → the eye wakes and scans → red lock-on → claw slashes tear the
signal → the emblem lifts and the lettering is stamped on (REJECTED BY THE,
then MACHINES with its red ink). 5.5 s, 60 fps, with a
chiptune sting. A or START skips it.

Game Boy Color only (CGB double speed, two VRAM banks, CGB palettes).

## Goal
Call one function at boot, get the studio card, and then carry on with your game.

## Approach
- The art and timeline are **exported from the JS preview** (`tools/`), so the
  ROM matches the approved animation. In SameBoy, frames differ by ≤1.1% of
  pixels; the only exception is the random static.
- The whole card is one BG map. Effects use only what the hardware does:
  - per-scanline SCX from a STAT interrupt (tears and waves; exact per-frame tables)
  - SCY for the emblem lift
  - CGB palette rewrites (fade, red flash, and the two lettering stamps:
    palettes 6 and 7 go hidden → grey ghost → white impact → settled → red ink)
  - SCY screen shake on each stamp
  - 8 static tiles whose pixels change every 2 frames
  - eye and ring animation by swapping map entries to tiles preloaded in VRAM bank 1
  - claw slashes as 18 8x16 sprites (at most 8 on a line)
- All map writes are queued and flushed in VBlank by a deadline-aware routine.
  It checks LY before each write, so nothing is ever written during active
  display. Any leftovers carry over to the next frame.
- Sound is a precomputed register stream: pulse 1/2, wave (triangle) and
  noise, with explicit note-offs.

## Step by step: add it to a game

1. Copy these into your project's `src/`:
   ```
   studio_card.h  studio_card.c  studio_card_home.c
   studio_card_gfx.c  studio_card_scx.c
   studio_card_data.h  studio_card_gfx.h  studio_card_scx.h  studio_card_counts.h
   ```
2. Compile the four `.c` files with the rest of your game. Use `-autobank` (the
   module asks for 3 banks itself) and link a ROM with **at least 4 banks**
   (64 KB, `-Wl-yo4`). Game Boy Color only: `-Wm-yC` (0x143 = 0xC0).
   ```sh
   lcc -autobank -Wm-yC -Wm-yt0x19 -c -o studio_card.o      src/studio_card.c
   lcc -autobank -Wm-yC -Wm-yt0x19 -c -o studio_card_home.o src/studio_card_home.c
   lcc -autobank -Wm-yC -Wm-yt0x19 -c -o studio_card_gfx.o  src/studio_card_gfx.c
   lcc -autobank -Wm-yC -Wm-yt0x19 -c -o studio_card_scx.o  src/studio_card_scx.c
   ```
3. Call it first thing in `main()`, before you start music or load graphics:
   ```c
   #include "studio_card.h"
   void main(void) {
       studio_card_play();      /* returns after ~5.5 s, or ~0.6 s after A/START */
       /* ...load your tiles, map, CGB attributes, palettes, then DISPLAY_ON... */
   }
   ```
4. On return, these are restored: LCDC, IE, STAT sources, CPU speed, SCX/SCY = 0,
   sprites hidden, all palettes black, sound channels silent (APU left on).
   **These are left dirty:** VRAM tiles, the 0x9800 map, **and its CGB attribute
   map**. Clear the attributes (VBK_REG = 1) or load your own before showing anything.

### Optional colour theme (not used: games ship with the default colours)
Set `sc_theme` before calling the card. It's the same animation and the same
data; only the 11-colour RGB table changes (22 bytes):
```c
sc_theme = SC_THEME_PROTAN;   /* or SC_THEME_DEFAULT (0) */
studio_card_play();
```
It moves every accent onto the blue-yellow axis, which stays vivid with protanopia:
gold eye and gold MACHINES, blue claw slashes, blue ink shadow, blue lock-on
flash. A game can expose it as a "colour assist" option and set it from its save.
- In `rom/studio_card_test.gbc` it only appears if you hold SELECT at power-on; leave `sc_theme` at its default (0) in games.

### The STAT interrupt (important)
For tears and waves on every scanline, `studio_card_home.c` **owns the STAT
vector (0x48)**. GBDK's shared `add_LCD` dispatcher was too slow for 144
interrupts a frame: it cost 16+ frames per frame in testing.
- To keep the per-line tears, don't use `add_LCD()`/`remove_LCD()` or
  `#include <stdio.h>` in the same ROM; both claim 0x48.
- Need your own LCD/STAT interrupt? Write it as a plain function and set
  `extern void (* volatile sc_stat_hook)(void); sc_stat_hook = my_stat_handler;`.
  The vector forwards to it whenever the card isn't running.
- **If something else does take 0x48** (the GBDK linker does *not* warn), the card
  detects it at runtime and falls back to whole-screen shakes. It still plays at
  full speed. That was tested with stdio linked in.
- `-DSC_NO_RASTER` (`-Wf-DSC_NO_RASTER` on every module file) forces that fallback
  and leaves the vector alone entirely.

## Budget
| | |
|---|---|
| ROM0 (fixed bank) | ~90 bytes (STAT vector, two bank-switching loaders) |
| Banked ROM | 3 banks: code + tables 13.9 KB, scanline tables 12.4 KB, tiles 7.5 KB |
| WRAM | ~860 bytes |
| VRAM | all of both banks (reload yours afterwards) |
| Frame time | 60 fps locked: 328 frames from f1 to f329 in cycle-accurate SameBoy |

## Files
```
src/            module + test ROM (main.c: card, then a placeholder screen; START replays)
tools/          core.js + variants.js (the JS preview renderer), export.js (asset/timeline exporter)
test/           sameboy.py (headless libretro front end), sb_run.py (timing + frame capture)
rom/            studio_card_test.gbc (approved build) / studio_card_test_noraster.gbc / SameBoy video / SHA256SUMS
build.sh / build.bat
```

## Rebuild
- Test ROM: `./build.sh` (Linux/macOS, GBDK at ../gbdk or $GBDK_HOME) or
  `build.bat` (Windows, GBDK at C:\gbdk).
- After changing art or timing in `tools/variants.js`, regenerate the data headers:
  ```sh
  node tools/export.js src
  ```

## Test it
1. Emulator first: open `rom/studio_card_test.gbc` in **SameBoy** or
   **Emulicious** (both are CGB-accurate). BGB is fine too.
   *PyBoy is not accurate enough for the STAT raster: it reports massive
   slowdown that real CGB timing doesn't have.*
2. Flash with ChroMagician: C. MAGICIAN on → back up the cart → Write Homebrew
   → `studio_card_test.gbc` → verify → C. MAGICIAN off → power-cycle.

## Pitfalls
- **Attributes left on the map.** Without clearing VBK 1, your tiles show up in
  the card's palettes and VRAM bank.
- **Tears missing in your game but not in the test ROM?** Something else owns the
  STAT vector (stdio or `add_LCD`). The card falls back silently; move your
  handler to `sc_stat_hook` to get the tears back.
- **Call it before your music driver.** The card drives the APU directly while it runs.
- **Speed switching** is done with the LCD and APU off (SameBoy flags speed
  switches with either on). Don't call it from inside an interrupt.
- **SameBoy libretro test harness:** its audio arrives ~13 frames after the
  video. That's a front-end buffering artifact, not the ROM.

## Verified (SameBoy 1.0.1 core, CGB mode, color correction off)
- 0 dropped frames, on both the first play and a replay
- frame-by-frame match with the JS reference (0.0–1.1% pixels, excluding random static)
- A/START skip from f60, f120, f150, f200 and f290 → game screen 0.63 s later
- clean hand-off to the placeholder screen; replaying with START works
- `-DSC_NO_RASTER` build, and a ROM where stdio overwrote 0x48, both run at full speed using the fallback
- Not yet run on a physical Chromatic.
