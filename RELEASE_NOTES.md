# Sneaky Kryptids: Kryptids in Space — Release notes

Game Boy Color · 512 KB · MBC5 + RAM + battery · tested on a ModRetro Chromatic

## Downloads
| File | What it is |
|---|---|
| `sneaky_kryptid.gbc` | The game, with saves (use this one) |
| `sneaky_kryptid_nosave.gbc` | For flash carts without save RAM |

The standalone demos (Flatwoods Drift, Escape Run, The Tree Line) and the unlocked test ROM are retired:
everything they offered is in the game, with every mission open from the start.

---

## Final release (2026-11-23)
- **Every story mission is open from the start.** Play the six missions in any order; the star chart
  always opens with the cursor on Arcade, and START launches Arcade there as well as A (START also flies
  from the ship select), so START, START, START, START from power-on starts an Arcade run. Up or Left
  reaches the missions; START on a mission stop still opens the armory.
- **All seven Arcade ships are open from the start.**
- **Arcade laps:** each map runs twice with no reload. At the end of lap 1 the scroll speeds up into a
  warp, then lap 2 runs in new colours with enemies that fire three-shot fans and faster shots, and the
  boss at its end. The next map follows with no stage card.
- **Arcade-only weapons** in the weapon capsule's cycle: **Prism** (a rainbow fan of shards that bounce
  off the walls at full power), **Helix** (two orbs that wind around each other and go through enemies)
  and **Nova** (a slow orb that goes through enemies, then bursts into a ring of stars).
- **Title screen:** no version line and no save-memory warning (the debrief no longer shows NOT SAVED either).
- **Star chart:** Arcade has its own stop, an infinity-loop warp gate marked A, beside the spot where your ship parks.
- **Arcade ship select** has its own background: a neon sun behind your ship, mountains, a grid floor
  and a console panel for the ship's stats.
- **Browser player:** game controllers (Xbox, PlayStation, Switch) and a full-screen mode (F key, the
  Full screen button, or click a controller stick). The standalone demo pages are gone; the site is the game.

## v1.0 (2026-11-17)
- **New soundtrack:** all 17 songs rebuilt around the track *Frostbite Circuit*; the title screen
  plays a Game Boy Color cover of it.
- **Mission 5 (gun cam) is calmer, most of all on Hunter.** At most 3 shooters (1 elite), 3 rocks and
  3 orbs at once in every mode. On Hunter, enemies wait longer between shots, orbs take longer to
  arrive, elites fire a single orb and the boss no longer adds a big orb to its volleys.
- **Mission 5 sprite flicker fixed:** elite saucers are drawn smaller and there are fewer speed
  streaks, so far fewer lines go over the hardware's 10-sprite limit.
- **Mission 6 (Escape Run) on Hunter** uses Pilot's attack patterns, with faster shots and two-hit ships.
- **Saves on ModRetro cartridges:** the header now declares 32 KB of save RAM. The game tests save
  memory at power-on and warns on screen (NO SAVE MEMORY! / NOT SAVED: NO RAM!) if writes don't stick.

## Demo v1.6 (2026-11-16)
- **Arcade: Endless Run.** Move past the last open mission to ARCADE, pick a ship and fly missions 1-4
  in a loop with no continues. Enemy fire speeds up every stage. Best score is saved per difficulty.
- **Seven ships**, one more unlocked for each story mission cleared: Sneaky Kryptid, Interdictor,
  Glyph Fang, Rust Runner, Deep Space, Night Hull and Drydock, each with its own gun, bombs and handling.
- **Missions 1-4 are longer**, with a slower scroll, more waves and a new hazard run in each.
- **Smoother boss fights:** face changes, the health bar, the score and boss limbs are updated more
  efficiently, removing most slowdown.

## Demo v1.5 (2026-11-16)
- **Title music** carries through the menus until the mission starts.
- **Mission screen:** a picture of the destination, the target's name, and each mission's stars,
  best score and outcome.
- **Rimemaw:** two new attacks (falling hail and ink rain), plus a third phase on Cadet and Pilot.
- **Pickups** each have their own shape: twin, spread, wisp, bomb, heart, spare ship.
- **Shield:** a dome over the ship's nose absorbs one hit, then recharges.
- **Game over / quit card**, and SELECT: QUIT from the pause menu in every mission.
- **Armory:** every item explains what it does and how to use it.
- **Performance:** the per-frame code is about 3.3 KB smaller and faster.
- **Balance:** Hunter is fairer in missions 5 and 6; the shield recharges more slowly on Hunter.

## Demo v1.4 (2026-11-12)
- **Six missions:** the main game ends after Escape Run. The Tree Line moves to its own ROM.
- **Escape Run on Hunter** can now be beaten: two-hit ships, slightly slower shots, wider gaps and an
  extra checkpoint halfway down the dive.
- **New cryptid names**, with their old Earth names kept in the briefings, and each mission now
  teaches its boss's main attack.
- **Capturing works again in missions 1 and 3**, with Nubs back as the mid-boss.
- **Music:** the briefing and ending themes are full arrangements.

## The Tree Line preview (2026-11-12)
- The first mission on foot: Graul fights up a forest path with his chain-mace (A) and howl (B),
  holds the wreck at night while Forge repairs the guns, and faces the Watcher.

## Escape Run demo v1.0 (2026-11-11)
- Mission 6 as its own short demo, playable in the browser at `/escape/`.
- SELECT + A on the pilot screen starts at the atmosphere dive.

## Demo v1.2 (2026-11-11)
- **Mission 6, Escape Run:** an EMP kills the guns and you run for the planet below. Dodge bullet
  curtains, homing missiles, asteroid walls and warned laser columns across three phases.
- **Graze and dash:** near misses fill the BOOST meter; A dashes through danger. B is a shield pulse.
- **The steal:** a good run grabs the gang's brig pod and introduces Graul, a werewolf bounty hunter.
- **New soundtrack:** full four-channel music with a theme for every mission and boss.
- **Weapon flair:** recoil, weapon-coloured exhaust, per-weapon impacts and three Weapon Runes.
- **New ending:** a rank screen and crew banter about your record.
- Color-blind friendly hazard and weapon colours, and no full-screen flashes.
- Final Rejected By The Machines studio card.

## Demo v1.1
- **Mission 5, Flatwoods Drift:** a first-person gun-cam mission against the Flatwoods Monster, with a
  live bounty meter on the dashboard.
- Flatwoods Drift is also available as a standalone demo, playable in the browser at `/flatwoods/`.

## Known limits
- Game Boy Color only (will not run on an original Game Boy).
- Free to download and play for personal, non-commercial use. See LICENSE.
