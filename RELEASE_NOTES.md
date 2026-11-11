# Sneaky Kryptid: Kryptids in Space — Demo v1.1

Game Boy Color · 512 KB · MBC5 + RAM + battery · tested on a ModRetro Chromatic

## Download
- `sneaky_kryptid.gbc` — the demo, with saves (use this one)
- `sneaky_kryptid_nosave.gbc` — for flash carts without save RAM

## Coming in v1.2 (branch `escape-run`, not released yet)
- **Mission 6, Escape Run:** Nubs's whole gang ambushes the ship, an EMP kills the guns, and you
  run for the planet below. No shooting: dodge bullet curtains, homing missiles, asteroid walls
  and warned laser columns across three phases (chase, gauntlet, atmosphere dive).
- **Graze and dash:** near misses fill the BOOST meter; A dashes through danger when it is full
  (Cadet dodges by itself). B is a shield pulse that clears nearby shots.
- **The steal:** a good run lets you grab the gang's brig pod, which counts as the capture and
  brings a new face into the debrief: Graul, a werewolf bounty hunter.
- New title intro (AGD PRESENTS) and a new ending after mission 6: career tally, the crew's
  verdict on your flying, and the eyes of something that wants its kin back.
- Colour-blind-safe hazards (warm and bright against a cool player side, with shape and blink
  warnings) and no full-screen flashes in the new mission.
- Saves from v1.0 and v1.1 are upgraded on boot (save version 4); mission 6 unlocks after Flatwoods Drift.

## New in v1.1
- **Mission 5, Flatwoods Drift:** a first-person gun-cam mission. The helm is jammed, so you
  aim a crosshair from the cockpit, shoot down what is fired at the ship, and face the
  Flatwoods Monster. New music, new briefing and three endings to the bounty.
- The dashboard shows a live bounty meter (ESCAPE / KILL / CAPTURE) in that mission.
- Your v1.0 save carries over; mission 5 unlocks after Frostbite Moon.
- Fix: a cutscene could draw garbled art if the previous mission ended while the crew was talking.

## Flatwoods Drift: gun-cam demo v1.0
- `flatwoods_drift.gbc` (`flatwoods_drift_nosave.gbc` without save RAM): mission 5 as its own short demo.
  Studio card, title, pilot select, briefing, the mission and the debrief, then back to pilot select.
- SELECT+A on the pilot screen goes straight to the Flatwoods Monster.
- Separate save block, so it can share a cart's save RAM history with the full game.
- Browser: `docs/flatwoods/` (GitHub Pages: /flatwoods/).

## What's in it
- Five bounty missions: Rimeveil Lane (Space Kraken), Stormwall Nebula (Thunderbird),
  Barrens Belt (Jersey Devil), Frostbite Moon (Wendigo), each with a mid-boss, and the
  gun-cam mission Flatwoods Drift (Flatwoods Monster).
- Three difficulties: CADET (easy, autofire), PILOT (3 hearts, autofire), HUNTER (one-hit, hold A).
- Fly clean and score high to CAPTURE the bounty (triple pay); a good run KILLS it; a weak run lets it ESCAPE.
- Shoot enemy bullets out of the air. Spend your pay at Forge's Armory (START on the mission map).
- Rescue sleeping baby cryptids to fly as wingmen.

## Controls
D-pad fly (mission 5: aim) · A fire (Hunter; mission 6: dash) · B bomb (mission 6: shield pulse) · START pause / armory · SELECT+A on the map: boss practice ·
SELECT+B on the title: erase saves.

## Known limits
- Game Boy Color only (will not run on an original Game Boy).
- Free to download and play for personal, non-commercial use. See LICENSE.
