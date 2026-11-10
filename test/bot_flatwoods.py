#!/usr/bin/env python3
"""Playtest bot for the standalone Flatwoods Drift demo (build/flatwoods_drift.gbc).
Same player as bot_cockpit.py; only the menu path differs (title -> pilot -> mission).
Usage: bot_flatwoods.py ROM cadet|pilot|hunter pro|kid [seed] [--boss] [--shots DIR]"""
import sys, os, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bot_cockpit import CockpitBot

class FlatwoodsBot(CockpitBot):
    def start(self):
        g = self.g
        g.tick(760)                                      # studio card, AGD PRESENTS fade, then the title
        g.press('start'); g.tick(30)                     # title -> pilot select
        for k in range({'cadet': 0, 'pilot': 1, 'hunter': 2}[self.mode]):
            g.press('down'); g.tick(4)
        if self.boss:
            g.hold('select'); g.press('a'); g.release('select'); g.tick(40)
        else:
            g.press('a'); g.tick(50)
            g.press('start'); g.tick(30)                 # skip the briefing
        if self.mode == 'hunter': g.hold('a'); self.held.add('a')

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("rom"); ap.add_argument("mode"); ap.add_argument("profile")
    ap.add_argument("seed", nargs="?", type=int, default=1)
    ap.add_argument("--shots"); ap.add_argument("--boss", action="store_true")
    a = ap.parse_args()
    if a.shots: os.makedirs(a.shots, exist_ok=True)
    print(FlatwoodsBot(a.rom, a.mode, a.profile, a.seed, a.shots, a.boss, stage=4).run())
