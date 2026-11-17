# Sneaky Kryptid: Kryptids in Space — v1.0

Game Boy Color · 512 KB · MBC5 + RAM + battery · tested on a ModRetro Chromatic

## Download
- `sneaky_kryptid.gbc` — the game, with saves (use this one)
- `sneaky_kryptid_nosave.gbc` — for flash carts without save RAM
- `sneaky_kryptid_unlocked.gbc` — test build with every mission open (title shows V1.0 UNLOCKED)

## v1.0 (2026-11-17)
- **New soundtrack: Frostbite OST.** All 17 songs rebuilt from the track Frostbite Circuit; the
  title screen is a Game Boy Color cover of it.
- **Mission 5 (gun-cam) is calmer, most of all on Hunter.** Hunter-only enemies and rocks are gone.
  At most 3 shooters (1 elite) and 3 rocks at once, in every mode; at most 3 orbs in the air (Hunter
  was 5). On Hunter, enemies wait longer before their first shot and between shots, orbs take
  longer to arrive, elites fire one plain orb instead of a pair with a big one, half the stray rocks
  are gone, and the boss no longer adds a big orb to its volleys.
- **Mission 5 no longer breaks up on screen.** Elite saucers hover a little further out (drawn at
  16 px, not 32), and there are 4 speed streaks instead of 6. Frames with more than 10 sprites on a
  line (the hardware limit, where sprites flicker or vanish): about 33-41% before, 4-10% now.
- **Mission 6 (Escape Run) on Hunter** uses the same attack patterns as Pilot: no extra aimed orb,
  second missile or extra rock, a three-wide gap in shot curtains, fewer shots in rain and ring
  attacks, at most two missiles, attacks less often. Hunter keeps faster shots and two-hit ships.
- Clumsy test bot on Hunter: mission 5 went from 6-11 hits to 0-1; mission 6 from 14-16 hits and
  2 continues to 3-4 hits and none.
- Title screen shows V1.0.
- **Saves on ModRetro cartridges:** the header now declares 32 KB of save RAM (was 8 KB), matching
  the FRAM on ModRetro carts. At power-on the game tests save memory; if writes don't stick, the
  title says NO SAVE MEMORY! and the mission results say NOT SAVED: NO RAM!, and every save is
  read back after writing.

## Demo v1.6 (2026-11-16)
- **Arcade: Endless Run.** On the mission screen, move past the last open mission to ARCADE. Pick a
  ship, then fly missions 1-4 in a loop with no continues: score, ships, hull, bombs and weapons
  carry from stage to stage, enemy shots get faster every stage and the shield takes longer to
  return. Faster scroll than the story. Best score is saved per difficulty.
- **Seven ships** (arcade only). One more unlocks for each story mission cleared:
  SNEAKY KRYPTID (twin gun, 2 pips), INTERDICTOR (twin gun at full power, 2 bombs), GLYPH FANG
  (wisp gun, 5 bombs), RUST RUNNER (5 ships, spread gun, slow), DEEP SPACE (shield returns twice as
  fast, starts with a buddy), NIGHT HULL (fast, smaller hitbox, 2 ships), DRYDOCK (spread gun at
  full power, 4 ships, slow, slow shield).
- **Missions 1-4 are longer.** The scroll runs at 20 px/s (was 30), so the flight to each boss
  takes 1.5x as long, and each mission has about half as many waves again to fill it, including one
  new hazard run (ice, storm clouds, wisps, turrets). Capture scores were raised to match.
- **Boss fights no longer skip or drag** (see "Boss fight speed" below).
- Saves: the story save is unchanged (version 5). Arcade keeps its own small block.

## Demo v1.5 (2026-11-16)
- **Title music** now runs from the title screen through the pilot, mission, armory and briefing
  screens until the mission starts. Debriefs keep Contract Terms.
- **Mission screen:** a picture of where the chosen mission goes, the target's name, the mission
  list, and the chosen mission's stars, best score and how the hunt ended (CAGED / SLAIN / FLED).
- **Rimemaw:** two new attacks. Hail: ice shakes loose, hangs, then drops down its columns with a
  lane left open. Rain: rows of ink across the screen with a moving gap. Cadet and Pilot now get a
  third phase with a slow ink spiral. Health: Cadet 500, Pilot 700 (was 420), Hunter 820.
- **Pickups** each have their own shape: twin (two bolts in a box), spread (a fan), wisp (a little
  ghost), bomb, heart, spare ship.
- **Shield:** a dome over the ship's nose takes one hit, then recharges (Cadet 4 s, Pilot 7 s,
  Hunter 15 s). Scrolling missions 1-4 and Escape Run. Shielded hits do not count against you.
- **Escape Run, Cadet:** chevrons mark the safe lane toward the next gap, and arrows beside the ship
  say which way to move for walls, falling rocks and lasers.
- **Game over / quit card:** the place, "SHIP LOST" or "CONTRACT DROPPED", the target still out
  there, score. Pause now offers SELECT: QUIT in every mission.
- **Armory:** every item says what it does and how to use it; the weapon rune page shows the three
  gun pickups and explains pips.
- Saves: unchanged (version 5).

### Speed pass (2026-11-16)
- The per-frame code (stage, core, objs, s1-s4, escape, cockpit) is compiled with a deeper
  optimizer search: about 3.3 KB smaller overall and faster. A clean build now takes about 12 minutes.
- Shots: the enemy-bullet table is built inside the bullet update that already runs, the boss
  position is worked out once per frame, and nothing runs when no shots are in the air.
- Hunter at full weapon power, frames that ran long: 3-12% before, 1-7% after. Escape Run: about
  a third fewer.

### Polish (2026-11-16)
- Mission screen: the words change at once and the picture loads when the cursor rests, so moving
  through the list no longer blinks on every step. Missions you have not flown say NOT FLOWN YET.
- Shield: flickers out when it breaks, flickers in just before it returns, and is solid for half a
  second when it is back.
- Rimemaw: Static and the Captain call out hail and ink rain the first time each is used.
- Escape Run, Cadet: three arrows instead of two, lit for longer.
- Quit card: a line from the crew.

### Hunter balance (2026-11-16)
- **Mission 5 (gun-cam):** now has the shield (all modes; Hunter and Pilot see it as a pip on the
  HUD). Hunter ships take two hits, orbs take a little longer to arrive, at most 5 are in the air
  (was 6), and enemies wait longer between shots.
- **Mission 6 (Escape Run), Hunter:** the opening chase attacks less often, the extra aimed orb and
  the second missile hold off until later phases, missiles are slower and stop steering sooner, and
  every run starts with one dash ready (all modes).
- **Shield recharge on Hunter:** 25 s in missions 1-4 (was 15), 10 s in missions 5 and 6.
- Clumsy test bot on Hunter: mission 5 went from never clearing (three ships lost inside 45 s) to
  clearing with 0-2 continues; mission 6 from 1-3 continues to 0-1.

### Boss fight speed (2026-11-16)
- **Face changes no longer skip a frame.** A boss blinking or changing expression rewrote 19-34
  background tiles at once; it now does 5 per loop.
- **Boss health bar** only recalculates when the health changes, with 16-bit maths.
- **Score** on the HUD redraws every 8th frame while it is climbing, not on every hit.
- **Tentacles, arms and the Ashmare's tail** are positioned every other loop.
- Pilot, spread at full power, loops that ran long during the boss: mission 1 91% -> 6%,
  mission 2 5% -> 1%, mission 3 27% -> 4%, mission 4 97% -> 14%. Worst single loop about 1000
  scanlines -> about 270.

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

### Release pass (2026-11-12)
- **Capturing works again in missions 1 and 3.** Nubs is back as the mid-boss in both (his points are
  what make a capture reachable, and he needs introducing before his gang shows up in mission 6).
  Mission 3's new stomp lesson now comes just before him; its script rows were out of order and are fixed.
- **Hunter:** a clean run captures in missions 1-3 again (thresholds 64000 / 68000 / 69000), and a
  continue at a boss starts with a level-1 weapon instead of the starting gun.
- **Cadet, Escape Run:** the automatic dodge on a full boost meter is back, and the brig pod drifts
  toward the ship.
- **Story:** the mission 1 briefing says what the job is again, the Marrow briefing gets its punchline
  back, and the four boss call-outs are short again (the Old Earth names stay in the briefings).
- **Music:** the briefing theme (Contract Terms) and the ending theme (Long Way Home) were 6- and
  8-second loops; both are now full arrangements of the same tunes (32 and 43 seconds, with harmony,
  bass and drums).

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
