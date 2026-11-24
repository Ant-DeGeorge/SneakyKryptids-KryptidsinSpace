#!/usr/bin/env python3
"""Screenshot tour of the Japanese ROM: menus, a briefing, a stage with comms, armory, arcade.
Fails if any glyph did not fit the VRAM cache (ja_ovf). Usage: ja_tour.py ROM OUTDIR"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from harness import Game
rom, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
g = Game(rom); n = [0]
def shot(tag):
    g.shot(f"{out}/{n[0]:02d}_{tag}.png"); n[0] += 1
    assert g.m8('ja_ovf') == 0, f"glyph cache overflow at {tag}"
g.tick(700); shot('title')
for k in range(3): g.s8('pf_unlock', 6, k)
g.press('start'); g.tick(40); shot('pilot')
g.press('a'); g.tick(60); shot('chart_arcade')
g.press('a'); g.tick(60); shot('arcade_ship1')
for i in range(2, 8): g.press('right'); g.tick(20); shot(f'arcade_ship{i}')
g.press('b'); g.tick(60)
for k in range(7): g.press('up'); g.tick(4)
shot('chart_m1')
for k in range(3): g.press('down'); g.tick(6); shot(f'chart_m{k+2}')
for k in range(3): g.press('up'); g.tick(6)
g.press('start'); g.tick(60); shot('armory1')
for t in range(3): g.press('right'); g.tick(20); shot(f'armory_tab{t+2}')
g.press('b'); g.tick(60)
for k in range(7): g.press('up'); g.tick(4)
g.press('a'); g.tick(50)
for i in range(7):                                   # briefing: finish each line, shoot, next
    g.press('a'); g.tick(10); shot(f'brief{i}'); g.press('a'); g.tick(30)
g.tick(120)
last = 0
for f in range(60 * 60 * 3):                          # fly (invincible-ish: keep lives topped up), shoot every comm
    g.hold('a'); g.tick(1)
    if 'lives' in g.sym: g.s8('lives', 5)
    on = g.m8('comm_on')
    if on and not last: start = g.frame
    if on and g.frame - start == 110: shot('comm')
    last = on
    if f == 1500: g.press('start'); g.tick(20); shot('pause'); g.press('start'); g.tick(5)
    if g.m8('st') == 3: break
g.release('a'); g.tick(300); shot('clear')
print('ok', n[0], 'shots, sweeps', g.m8('ja_gcs'), 'overflow', g.m8('ja_ovf'))
