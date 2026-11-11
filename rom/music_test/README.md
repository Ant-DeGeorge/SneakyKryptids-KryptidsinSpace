# Music test build (branch music-hugedriver)

Work in progress: the new soundtrack engine, not a release.

- `sneaky_kryptid_music_test_unlocked.gbc`: the full game with every mission selectable, music played
  by hUGEDriver on all four channels. Title ("Bounty Call") and mission map ("Star Chart") are the new
  themes; every other song is the v1.1 tune carried over note for note.
- `title.mp3`, `map.mp3`: the two new themes rendered through SameBoy.
- Songs live in `music/*.uge` (hUGETracker 1.0 files). `make music` rebuilds them from
  `tools/music/compose.py`; `make jukebox` builds a sound test ROM; `tools/music/render.py` renders WAV/MP3.

Known issue: the Flatwoods Monster fight (mission 5, Cadet) drops an occasional frame with the new driver.
Same save format as Demo v1.1.
