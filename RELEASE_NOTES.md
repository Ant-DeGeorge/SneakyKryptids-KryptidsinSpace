# Sneaky Kryptid: Kryptids in Space — Demo v1.4

Game Boy Color · 512 KB · MBC5 + RAM + battery · tested on a ModRetro Chromatic

## Download
- `sneaky_kryptid.gbc` — the demo, with saves (use this one)
- `sneaky_kryptid_nosave.gbc` — for flash carts without save RAM
- `sneaky_kryptid_unlocked.gbc` — test build with every mission open (title shows DEMO V1.4 UNLOCKED)

## Demo v1.4 (2026-11-12)
- **Six missions.** The main game is the flying game again: missions 1-6, and the ending plays after
  Escape Run. The Tree Line is its own ROM for now (`tree_line.gbc`, `make treeline`), with its own save.
  Star totals on the pilot screen are out of 30; Hunter's high score adds up all six missions.
- **Escape Run on Hunter can be beaten.** Hunter ships take two hits here (the hearts show next to the ship
  count), shots are a little slower, attacks come a little less often, wall gaps are as wide as Pilot's,
  grazing fills the dash meter twice as fast, and one fewer falling rock per volley. A new checkpoint
  halfway down the dive, so a continue no longer starts at the top of it.
- **Mission 7 art:** boulders and the crashed ship sit on the dirt trail instead of in black squares.
- Also in this build (from `mission7-review`): new future names for the cryptids, each mission teaching
  its boss's main attack, and the forest trail art and bat-back mace for The Tree Line.
- Saves: unchanged layout (version 5), so v1.3 saves carry over.

## Escape Run: no-guns demo v1.0 (2026-11-11)
- `escape_run.gbc` (`escape_run_nosave.gbc` without save RAM): mission 6 as its own short demo.
  Studio card, title, pilot select, briefing, the run and the debrief, then back to pilot select.
- SELECT+A on the pilot screen starts at the atmosphere dive.
- Separate save block (`SKE`), so it can share a cart's save RAM with the full game and Flatwoods Drift.
- Browser: `docs/escape/` (GitHub Pages: /escape/).

## Coming in v1.3 (branch `tree-line`, not released yet)
- **Mission 7, The Tree Line:** the first mission on foot. Graul fights up a forest path with his
  chain-mace (A) and howl (B), through an ambush and a bruiser, then holds the wreck at night while
  Forge repairs the guns.
- **The Watcher:** the eyes from the ending become a boss over the tree line, with the eyes of the
  kin of your last capture. Bat its orbs back, smash its claw, let the turret work.
- The ending now plays after mission 7. Saves from every earlier version are upgraded (save version 5).
- Under the hood: Forge's armory and the crew's radio lines moved to their own banks to make room.
- **Smoother at max weapon:** the scrolling missions no longer slow down when your shots fill the
  screen (player-shot collisions now check a small per-frame list of live targets). In Hunter at
  max weapon, frames that ran long dropped from 14-54% to 1-16%, depending on the mission and weapon.
- The ending knows about mission 7: capture or kill the Watcher and someone bigger answers.
- Graul's own head icon on the Hunter HUD; he can no longer stand half inside a boulder; his
  colours sit further from the hazard colours for tritanopia.

## New in v1.2 (2026-11-11)
- **Mission 6, Escape Run:** Nubs's whole gang ambushes the ship, an EMP kills the guns, and you
  run for the planet below. No shooting: dodge bullet curtains, homing missiles, asteroid walls
  and warned laser columns across three phases (chase, gauntlet, atmosphere dive).
- **Graze and dash:** near misses fill the BOOST meter; A dashes through danger when it is full
  (Cadet dodges by itself). B is a shield pulse that clears nearby shots.
- **The steal:** a good run lets you grab the gang's brig pod, which counts as the capture and
  brings a new face into the debrief: Graul, a werewolf bounty hunter.
- The ending now plays after mission 6.
- Fix: stars no longer slide along with the bosses in missions 1-4.
- Colour-blind-safe hazards (warm and bright against a cool player side, with shape and blink
  warnings) and no full-screen flashes in the new mission.
- Saves from v1.0 and v1.1 are upgraded on boot (save version 4); mission 6 unlocks after Flatwoods Drift.

## Update (2026-11-11, later)
- **New soundtrack:** full four-channel music (hUGEDriver). New title theme (Bounty Call), map
  (Star Chart), and a stage and boss theme for every mission; Rimeveil Lane, the boss fights and
  both Flatwoods tracks are aggressive EDM. Every track has a name; see the sound test
  (`make jukebox`) for the list.
- **AGD PRESENTS** fades in once on its own, then the title; the title music plays straight
  through from AGD PRESENTS instead of hiccuping between the screens.
- **Weapon flair, part 2:** ship recoil and exhaust in the weapon colour, weapon-name pop on the HUD,
  per-weapon impacts and kill shards, and three Weapon Runes in Forge's Armory (Lance pierces,
  Bounce ricochets, Chain links wisps). The gun cam gets per-weapon shots too.
- **New ending:** a rank screen, crew banter that roasts your record, and something watching back.
- Saves from earlier versions are migrated on first boot.

## ROM refresh (2026-11-11)
- **Final Rejected By The Machines studio card:** static, the eye wakes and scans, the claws
  slash, then the lettering is stamped on (REJECTED BY THE, then MACHINES with its red ink).
  The second burst of static over the lettering is gone. Same card in both ROMs and in the browser builds.
- The ROMs now include the weapon flair update: colour-blind-safe weapon colours, new shot art,
  muzzle flash, hit sparks and a softer bomb flash.
- Saves carry over; nothing else changed.

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
- Six missions. Five bounty missions: Rimeveil Lane (Space Kraken), Stormwall Nebula (Thunderbird),
  Barrens Belt (Jersey Devil), Frostbite Moon (Wendigo), each with a mid-boss, and the
  gun-cam mission Flatwoods Drift (Flatwoods Monster); then Escape Run, with no guns at all.
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
