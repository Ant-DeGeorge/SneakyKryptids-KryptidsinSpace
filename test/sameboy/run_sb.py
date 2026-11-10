#!/usr/bin/env python3
"""SameBoy (accurate CGB) scripted run: run_sb.py ROM stage(1-5) boss|start frames outdir"""
import sys, os, re, subprocess
from PIL import Image
rom, stage, kind, frames, out = sys.argv[1], int(sys.argv[2]), sys.argv[3], int(sys.argv[4]), sys.argv[5]
os.makedirs(out, exist_ok=True)
noi = os.path.splitext(rom)[0] + ".noi"
sym = {m.group(1): int(m.group(2), 16) for m in (re.match(r"DEF _(\S+) 0x([0-9A-Fa-f]+)", l) for l in open(noi)) if m}
L = []
def press(f, k, n=3): L.append(f"{f} press {k}"); L.append(f"{f + n} release {k}")
f = 600; press(f, "START"); f += 40          # the studio card plays first
for p in range(3): L.append(f"{f} poke {sym['pf_unlock'] + p:x}=4")
f += 10; press(f, "A"); f += 60
for k in range(5 - stage): press(f, "UP"); f += 8
if kind == "boss":
    L.append(f"{f} press SELECT"); press(f + 2, "A"); L.append(f"{f + 6} release SELECT"); f += 40
else:
    press(f, "A"); f += 60; press(f, "START"); f += 30
t0 = f
for k in range(frames // 40):
    key = ["LEFT", "RIGHT", "UP", "DOWN"][(k * 3 + k // 3) % 4]
    L.append(f"{f} press {key}"); L.append(f"{f + 24} release {key}")
    f += 40
shots = []
for s in range(t0 + 60, f, 120):
    L.append(f"{s} shot s{s:05d}"); shots.append(f"s{s:05d}")
L.append(f"{f + 2} end")
L.sort(key=lambda l: int(l.split()[0]))
open(f"{out}/script.txt", "w").write("\n".join(L) + "\n")
r = subprocess.run(["./sb_harness", rom, f"{out}/script.txt", out, "."], capture_output=True, text=True, cwd=os.path.dirname(os.path.abspath(__file__)))
print(r.stdout.strip(), r.stderr.strip()[-300:])
ims = []
for s in shots:
    p = f"{out}/{s}.ppm"
    if os.path.exists(p): ims.append(Image.open(p).convert("RGB").resize((320, 288), Image.NEAREST))
W = Image.new("RGB", (6 * 324, ((len(ims) + 5) // 6) * 292), (80, 80, 80))
for i, im in enumerate(ims): W.paste(im, ((i % 6) * 324, (i // 6) * 292))
W.save(f"{out}/sheet.png")
print(len(ims), "shots")
