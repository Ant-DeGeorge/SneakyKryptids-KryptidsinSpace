# Arcade ship-select background: a neon sun behind the ship, mountains, a grid floor running to the
# horizon, and a dark console panel for the ship's name and stats. 160x144, five palettes (0-4) that all
# start with the text background colour, so the font (palettes 5-7) sits on it with no boxes.
# Text rows: 0 title, 2 "SHIP n/7", 9-10 name/role, 12-13 description, 14 best, 16-17 controls.
import os, math, random
from PIL import Image
import numpy as np
W, H = 160, 144
BG = (0, 0, 8)
WHITE = (248, 248, 248)
PALS = [
    [BG, (40, 24, 96), (128, 128, 216), WHITE],               # 0 sky and stars
    [BG, (248, 224, 72), (248, 136, 48), (232, 56, 136)],     # 1 sun: yellow, orange, pink
    [BG, (88, 16, 104), (224, 48, 200), (72, 224, 232)],      # 2 grid: deep magenta, neon magenta, cyan
    [BG, (16, 48, 80), (72, 216, 232), (248, 208, 56)],       # 3 console: dim teal, teal, gold
    [BG, (40, 16, 72), (104, 40, 136), (72, 224, 232)],       # 4 mountains (+ the horizon's cyan)
]
img = Image.new('RGB', (W, H), BG); px = img.load()
def put(x, y, c):
    if 0 <= x < W and 0 <= y < H: px[x, y] = c
HOR = 53
rnd = random.Random(7)
# row 1: neon rule under the title
for x in range(W):
    put(x, 9, PALS[2][2]); put(x, 12, PALS[2][3])
    if x % 4 == 0: put(x, 10, PALS[2][1])
# stars
for _ in range(46):
    x, y = rnd.randrange(W), rnd.randrange(15, HOR - 6)
    put(x, y, rnd.choice([PALS[0][2], PALS[0][2], PALS[0][1], WHITE]))
for x, y in [(14, 22), (142, 30), (30, 44), (124, 18), (150, 50)]:
    for d in (-1, 1): put(x + d, y, PALS[0][2]); put(x, y + d, PALS[0][2])
    put(x, y, WHITE)
# sun, striped toward the horizon
CX, CY, R = 80, 41, 18
for y in range(CY - R, HOR):
    for x in range(CX - R, CX + R + 1):
        if (x - CX) ** 2 + (y - CY) ** 2 > R * R: continue
        if y > CY - 2 and (y - CY) % 4 == 0: continue        # stripes toward the horizon
        t = (y - (CY - R)) / (2 * R)
        put(x, y, PALS[1][1] if t < 0.38 else PALS[1][2] if t < 0.62 else PALS[1][3])
# mountains either side of the sun
def ridge(x0, x1, peak_x, peak_h):
    for x in range(x0, x1):
        h = max(0, peak_h - abs(x - peak_x) * peak_h // max(1, (x1 - x0) // 2))
        h += (x * 7 % 5 == 0)
        for y in range(HOR - h, HOR):
            put(x, y, PALS[4][2] if y == HOR - h else PALS[4][1])
ridge(0, 60, 24, 12); ridge(0, 40, 6, 7); ridge(100, 160, 134, 10); ridge(124, 160, 154, 14)
# horizon and grid floor
for x in range(W): put(x, HOR, PALS[1][3] if abs(x - CX) <= R + 2 else PALS[2][3])   # pink glow under the sun
ys, y, step = [], HOR + 2, 1
while y < 71: ys.append(y); step += 1; y += step
for yy in ys:
    for x in range(W): put(x, yy, PALS[2][2] if yy > HOR + 4 else PALS[2][1])
for i in range(-9, 10):                                  # rails to the vanishing point, drawn as solid lines
    x0, y0, x1, y1 = 80 + i * 5, HOR + 1, 80 + i * 26, 70
    n = max(abs(x1 - x0), y1 - y0)
    for k in range(n + 1):
        yy = round(y0 + (y1 - y0) * k / n); xx = round(x0 + (x1 - x0) * k / n)
        put(xx, yy, PALS[2][2] if yy > HOR + 5 else PALS[2][1])
for x in range(W): put(x, 70, PALS[2][3]); put(x, 71, PALS[2][1] if x % 2 else BG)
# console panel: side rails with gold bolts, two dotted dividers in the free rows
for y in range(72, H):
    put(1, y, PALS[3][2]); put(158, y, PALS[3][2]); put(2, y, PALS[3][1]); put(157, y, PALS[3][1])
for y in (76, 108, 140):
    for x in (1, 158): put(x, y, PALS[3][3])
for yy in (91, 123):
    for x in range(12, 148, 3): put(x, yy, PALS[3][1])
    for x in (10, 149): put(x, yy, PALS[3][2])

# ---- fit each 8x8 tile to its best palette
a = np.asarray(img, float)
out = np.zeros_like(a); tpal = np.zeros((18, 20), int); idx = np.zeros((18, 20, 8, 8), int); bad = 0
P = np.array(PALS, float)
for ty in range(18):
    for tx in range(20):
        t = a[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8].reshape(64, 3)
        d = ((t[None, :, None, :] - P[:, None, :, :]) ** 2).sum(-1)
        e = d.min(-1).sum(-1); p = int(e.argmin()); tpal[ty, tx] = p
        if e[p] > 0: bad += 1
        ix = d[p].argmin(-1); idx[ty, tx] = ix.reshape(8, 8)
        out[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] = P[p][ix].reshape(8, 8, 3)
HERE = os.path.dirname(os.path.abspath(__file__))
res = Image.fromarray(out.astype(np.uint8))
os.makedirs(os.path.join(HERE, '..', 'art', 'arcade_select'), exist_ok=True)
res.save(os.path.join(HERE, '..', 'art', 'arcade_select', 'arcade_select_160x144.png'))
uniq, tmap, tattr = [], [], []
for ty in range(18):
    for tx in range(20):
        t = idx[ty, tx]
        for fl, v in ((0, t), (0x20, t[:, ::-1]), (0x40, t[::-1]), (0x60, t[::-1, ::-1])):
            hit = next((i for i, u in enumerate(uniq) if u.tobytes() == v.tobytes()), None)
            if hit is not None: break
        if hit is None: uniq.append(t.copy()); hit = len(uniq) - 1; fl = 0
        tmap.append(hit); tattr.append(int(tpal[ty, tx]) | 0x08 | fl)     # 0x08: tile data in VRAM bank 1
def tb(px8):
    o = []
    for row in px8:
        lo = hi = 0
        for x, v in enumerate(row):
            if v & 1: lo |= 0x80 >> x
            if v & 2: hi |= 0x80 >> x
        o += [lo, hi]
    return o
def rgb555(c): return min(31, round(c[0] / 255 * 31)) | (min(31, round(c[1] / 255 * 31)) << 5) | (min(31, round(c[2] / 255 * 31)) << 10)
data = sum((tb(u) for u in uniq), [])
SRC = os.path.join(HERE, '..', 'src')
with open(os.path.join(SRC, 'arcade_art.c'), 'w') as f:
    f.write('// generated by tools/gen_arcade_art.py: the Arcade ship-select background\n#pragma bank 255\n#include <gb/gb.h>\n#include <stdint.h>\n#include "arcade_art.h"\nBANKREF(arcade_art)\n')
    f.write('const uint8_t arc_art_tiles[] = {%s};\n' % ','.join(map(str, data)))
    f.write('const uint8_t arc_art_map[] = {%s};\n' % ','.join(map(str, tmap)))
    f.write('const uint8_t arc_art_attr[] = {%s};\n' % ','.join(map(str, tattr)))
    f.write('const uint16_t arc_art_pals[] = {%s};\n' % ','.join('0x%04X' % rgb555(c) for p in PALS for c in p))
with open(os.path.join(SRC, 'arcade_art.h'), 'w') as f:
    f.write('#ifndef ARCADE_ART_H\n#define ARCADE_ART_H\n#include <gb/gb.h>\n#include <stdint.h>\nBANKREF_EXTERN(arcade_art)\n')
    f.write('#define ARC_ART_NT %d\n#define ARC_ART_NPAL %d\n' % (len(uniq), len(PALS)))
    f.write('extern const uint8_t arc_art_tiles[], arc_art_map[], arc_art_attr[];\nextern const uint16_t arc_art_pals[];\n#endif\n')
print('arcade_art:', len(uniq), 'tiles | tiles that lost a colour', bad)
