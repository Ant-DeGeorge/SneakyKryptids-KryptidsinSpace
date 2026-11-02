#!/usr/bin/env python3
"""Walk the menus and grab screenshots: title, pilots, map, briefing, each stage."""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from harness import Game
rom, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
names = []
def shot(g, n):
    p = f"{out}/{len(names):02d}_{n}.png"; g.shot(p); names.append(p)
g = Game(rom)
g.tick(100); shot(g, "title")
for k in range(3): g.s8('pf_unlock', 2, k)
g.press('start'); g.tick(40); shot(g, "pilots")
g.press('a'); g.tick(60); g.press('down'); g.tick(10); shot(g, "map")
for st in range(3):
    if st:
        g = Game(rom); g.tick(80)
        for k in range(3): g.s8('pf_unlock', 2, k)
        g.press('start'); g.tick(30); g.press('a'); g.tick(40)
        for k in range(st): g.press('down'); g.tick(4)
    else:
        g.press('up'); g.tick(4)
    g.press('a'); g.tick(160); shot(g, f"s{st+1}_brief")
    g.press('start'); g.tick(200); shot(g, f"s{st+1}_start")
    g.tick(900); shot(g, f"s{st+1}_15s")
    g.tick(1500); shot(g, f"s{st+1}_40s")
from PIL import Image
W = Image.new('RGB', (5 * 324, ((len(names) + 4) // 5) * 292), (80, 80, 80))
for i, f in enumerate(names): W.paste(Image.open(f), ((i % 5) * 324, (i // 5) * 292))
W.save(f"{out}/sheet.png")
