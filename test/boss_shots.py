#!/usr/bin/env python3
"""Boss-practice bot run with periodic screenshots: boss_shots.py ROM stage mode out"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bot import Bot
rom, stage, mode, out = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4]
os.makedirs(out, exist_ok=True)
b = Bot(rom, mode, 'pro', 3, boss=True, stage=stage - 1, maxframes=60 * 60 * 6)
g = b.g
b.start()
names = []
f0 = g.frame
while g.frame - f0 < 60 * 150:
    S = b.state()
    if S['st'] == 3: break
    d, bomb = b.decide(S); b.set_dir(d)
    if (g.frame - f0) % 150 == 0 and len(names) < 20:
        p = f"{out}/{len(names):02d}.png"; g.shot(p); names.append(p)
    g.tick(1)
print(b.state()['khp'], b.state()['st'], b.state()['hits'], (g.frame - f0) / 60)
from PIL import Image
W = Image.new('RGB', (5 * 324, ((len(names) + 4) // 5) * 292), (80, 80, 80))
for i, f in enumerate(names): W.paste(Image.open(f), ((i % 5) * 324, (i // 5) * 292))
W.save(f"{out}/sheet.png")
