"""Screenshots of the title intro and the ending (build/endtest.gbc).
usage: python3 test/shot_ending.py ROM OUTDIR PROFILE OUTS BESTS [title]
  OUTS  = one digit per mission (5 public, 4 family), bounty outcome per mission (1 escaped, 2 killed, 3 captured)
  BESTS = 5 digits, best rating per mission (1..5)
"""
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from harness import Game

rom, out, prof, outs, bests = sys.argv[1:6]
title = len(sys.argv) > 6
os.makedirs(out, exist_ok=True)
g = Game(rom)
prof = int(prof)
# studio card: mash through it
for i in range(60):
    g.tick(20)
    if title and i in (14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34):
        g.shot(f"{out}/title_{g.frame:05d}.png")
g.tick(300)
if title:
    g.shot(f"{out}/title_final.png")
n = len(outs)
for i in range(n):
    g.s8("pf_out", int(outs[i]), prof * n + i)
    g.s8("pf_best", int(bests[i]), prof * n + i)
    g.s8("pf_hi", 0x50, (prof * n + i) * 4 + 1)
g.s8("profile", prof)
g.s8("gmode", int(sys.argv[7]) if len(sys.argv) > 7 else prof)
g.press("start")
g.tick(150)
g.shot(f"{out}/1_tally.png")
g.press("a")
for k in range(4):
    g.tick(260)
    g.shot(f"{out}/2_banter{k}.png")
    g.press("a")
# watcher: sample the sequence
t0 = g.frame
for k in range(24):
    g.tick(40)
    g.shot(f"{out}/3_eyes_{g.frame - t0:04d}.png")
print("done", g.frame)
