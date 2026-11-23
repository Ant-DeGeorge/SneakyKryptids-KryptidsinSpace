# Star-chart background for the mission screen. 160x144, drawn pixel by pixel in seven hand-picked
# Game Boy Color palettes (every one starts with the sky colour), so one palette stays free for text.
import sys, math, random

from PIL import Image
import numpy as np
W, H = 160, 144
BG = (8, 10, 28)
GRID, SBLUE, WHITE = (26, 38, 78), (132, 156, 208), (248, 248, 248)
YEL, TEAL = (248, 208, 72), (72, 216, 232)
ICE = [WHITE, (168, 224, 248), (72, 128, 208)]
ST = [(232, 168, 248), (168, 88, 216), (96, 40, 152)]; BOLT = (248, 240, 120)
RK = [(232, 176, 120), (184, 112, 64), (112, 56, 40)]
LG = [(224, 248, 168), (128, 224, 96), (40, 136, 72)]
PL = [(152, 224, 200), (64, 160, 152), (32, 88, 120)]
PALS = [[BG, GRID, SBLUE, WHITE], [BG, YEL, TEAL, WHITE], [BG] + ICE, [BG, ST[0], ST[1], BOLT], [BG] + RK, [BG] + LG, [BG] + PL]
NAMES = ['chart', 'route', 'ice', 'storm', 'rock', 'lamp', 'planet']
img = Image.new('RGB', (W, H), BG); px = img.load()
def put(x, y, c):
    if 0 <= x < W and 0 <= y < H: px[x, y] = c
rnd = random.Random(11)
# chart grid: dotted lines every 16 px, a small cross at each joint
for y in range(H):
    for x in range(W):
        if (x % 16 == 7 and y % 2 == 0) or (y % 16 == 7 and x % 2 == 0): put(x, y, GRID)
for gy in range(7, H, 16):
    for gx in range(7, W, 16):
        for d in (-2, -1, 0, 1, 2): put(gx + d, gy, GRID); put(gx, gy + d, GRID)
# scan arcs around the last stop
PCX, PCY = 150, 136
for r in (46, 62, 80):
    for a in range(0, 3600):
        t = math.radians(a / 10.0)
        if (a // 50) % 3 != 2: put(int(round(PCX + r * math.cos(t))), int(round(PCY + r * math.sin(t))), GRID)
# stars
for _ in range(64):
    put(rnd.randrange(W), rnd.randrange(H), rnd.choice([WHITE, SBLUE, SBLUE]))
for x, y in [(12, 62), (44, 98), (86, 72), (30, 128), (74, 116), (150, 66), (118, 6), (56, 12)]:
    for d in (-1, 1): put(x + d, y, SBLUE); put(x, y + d, SBLUE)
    put(x, y, WHITE)

def disc(cx, cy, r, cols, lx=-0.55, ly=-0.6, tex=None, k=3.4):
    """ball lit from the upper left: three shades, and the far edge falls away into the sky"""
    L = math.sqrt(lx * lx + ly * ly + 0.49)
    for y in range(cy - r, cy + r + 1):
        for x in range(cx - r, cx + r + 1):
            dx, dy = (x - cx) / r, (y - cy) / r
            if dx * dx + dy * dy > 1.0: continue
            dz = math.sqrt(max(0, 1 - dx * dx - dy * dy))
            l = (-(dx * lx + dy * ly) * -1 + dz * 0.7) / L
            l = (dx * lx + dy * ly + dz * 0.7) / L
            v = (1 - max(0.0, l)) * k
            if tex: v += tex(x, y)
            i = int(v)
            if v - i > 0.62 and (x + y) & 1: i += 1
            if i >= 3:
                if (i == 3 and (x + y) & 1 == 0) or r < 6: put(x, y, cols[2])
                else: put(x, y, BG)
            else: put(x, y, cols[max(0, i)])
def ring(cx, cy, r, c):
    for a in range(0, 720):
        t = math.radians(a / 2.0); put(int(round(cx + r * math.cos(t))), int(round(cy + r * math.sin(t))), c)

STOPS = [(22, 30), (62, 52), (100, 24), (138, 46), (112, 88), (150, 136)]
RAD = [15, 14, 16, 13, 13, 36]
def bez(p0, p1, bend):
    mx, my = (p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2
    nx, ny = -(p1[1] - p0[1]), p1[0] - p0[0]; Ln = math.hypot(nx, ny)
    cx, cy = mx + nx / Ln * bend, my + ny / Ln * bend
    return [((1 - t) ** 2 * p0[0] + 2 * t * (1 - t) * cx + t * t * p1[0], (1 - t) ** 2 * p0[1] + 2 * t * (1 - t) * cy + t * t * p1[1]) for t in [k / 300.0 for k in range(301)]]
for i, bend in enumerate([8, -8, 8, 10, -10]):      # the route: yellow dashes that stop short of each world
    pts = bez(STOPS[i], STOPS[i + 1], bend); dist = 0; last = pts[0]
    for p in pts:
        dist += math.hypot(p[0] - last[0], p[1] - last[1]); last = p
        if math.hypot(p[0] - STOPS[i][0], p[1] - STOPS[i][1]) < RAD[i] or math.hypot(p[0] - STOPS[i + 1][0], p[1] - STOPS[i + 1][1]) < RAD[i + 1]: continue
        if int(dist) % 6 < 3: put(int(round(p[0])), int(round(p[1])), YEL)

# 1 Rimeveil Lane: an ice world with a tilted ring
x, y = STOPS[0]
def ring_pt(t): return int(round(x + 15 * math.cos(t))), int(round(y + 5 * math.sin(t) - 2 * math.cos(t)))
for a in range(0, 720):
    t = math.radians(a / 2.0)
    if math.sin(t) < 0: put(*ring_pt(t), ICE[2])
disc(x, y, 8, ICE)
for a in range(0, 720):
    t = math.radians(a / 2.0)
    if math.sin(t) >= 0:
        xx, yy = ring_pt(t); put(xx, yy, ICE[0] if (a // 14) % 2 else ICE[1]); put(xx, yy + 1, ICE[2])
# 2 Stormwall Nebula: a violet storm with a bolt through it
x, y = STOPS[1]
for cx, cy, r in [(-7, 2, 6), (7, 2, 6), (0, -2, 8), (2, 5, 6), (-3, 5, 5)]:
    for yy in range(-r, r + 1):
        for xx in range(-r, r + 1):
            if xx * xx + yy * yy <= r * r:
                lit = (-xx * 0.4 - yy * 0.9) / r
                put(x + cx + xx, y + cy + yy, ST[0] if lit > 0.55 else ST[1] if lit > -0.35 or (xx + yy) & 1 else BG)
for bx, by in [(2, -7), (1, -6), (0, -5), (-1, -4), (-2, -3), (-1, -3), (0, -3), (1, -3), (2, -3), (1, -2), (0, -1), (-1, 0), (-2, 1), (-1, 1), (0, 1), (1, 1), (0, 2), (-1, 3), (-2, 4), (-3, 5), (-4, 6)]:
    put(x + bx, y + by, BOLT); put(x + bx + 1, y + by, BOLT)
# 3 Barrens Belt: a drift of rust rocks
x, y = STOPS[2]
for cx, cy, r in [(-9, 2, 4), (-2, -3, 6), (7, 3, 5), (12, -5, 2), (-13, -5, 2), (1, 8, 3), (15, 6, 2), (-6, 9, 2)]:
    disc(x + cx, y + cy, r, RK)
for cx, cy in [(-4, -4), (-1, -1), (6, 4), (8, 2)]: put(x + cx, y + cy, RK[2])
# 4 Frostbite Moon: a cratered white moon
x, y = STOPS[3]
disc(x, y, 10, ICE)
for cx, cy, r in [(-3, -3, 2), (2, 2, 3), (-4, 4, 1), (4, -5, 1)]:
    ring(x + cx, y + cy, r, ICE[2]); put(x + cx, y + cy, ICE[1])
# 5 Lamp Drift: a green saucer and its beam
x, y = STOPS[4]
for yy in range(0, 13):
    for xx in range(-2 - yy // 2, 3 + yy // 2):
        if (xx + yy) % 2 == 0: put(x + xx, y + 5 + yy, LG[2])
for xx in range(-11, 12):
    h = 3 if abs(xx) < 8 else 2 if abs(xx) < 10 else 1
    for yy in range(-h + 1, h): put(x + xx, y + 1 + yy, LG[1] if yy < 1 else LG[2])
    put(x + xx, y + 1 - h, LG[0]); put(x + xx, y + 1 + h, LG[2])
for yy in range(-6, 0):
    for xx in range(-5, 6):
        if xx * xx + (yy * 1.3) ** 2 <= 30: put(x + xx, y - 2 + yy, LG[0] if xx + yy < -3 else LG[1])
for xx in (-8, -4, 0, 4, 8): put(x + xx, y + 1, LG[0])
# 6 Escape Run: the big world in the corner; land shows as lighter patches
x, y = STOPS[5]
def land(xx, yy):
    v = math.sin(xx * 0.29 + yy * 0.16) + math.sin(xx * 0.12 - yy * 0.27) + 0.5 * math.sin(xx * 0.5 + yy * 0.45)
    return -0.9 if v > 1.0 else 0.35 if v < -1.2 else 0
disc(x, y, 34, PL, lx=0.45, ly=-0.75, tex=land, k=4.6)
for a in range(0, 2880):                             # thin lit atmosphere on the sunward limb
    t = math.radians(a / 8.0); ang = math.atan2(math.sin(t), math.cos(t))
    if -3.05 < ang < -0.9: put(int(round(x + 35.4 * math.cos(t))), int(round(y + 35.4 * math.sin(t))), PL[0])
# Arcade: an endless loop, a warp gate shaped like an infinity sign, in the open lower left
GX, GY, GA = 54, 104, 24
for k in range(0, 2400):
    t = 2 * math.pi * k / 2400; d = 1 + math.sin(t) ** 2
    xx, yy = GX + GA * math.cos(t) / d, GY + GA * math.sin(t) * math.cos(t) / d * 1.25
    for ox, oy in ((0, -1), (0, 1), (-1, 0), (1, 0)): put(int(round(xx + ox)), int(round(yy + oy)), TEAL)
for k in range(0, 2400):
    t = 2 * math.pi * k / 2400; d = 1 + math.sin(t) ** 2
    put(int(round(GX + GA * math.cos(t) / d)), int(round(GY + GA * math.sin(t) * math.cos(t) / d * 1.25)), YEL if (k // 100) % 3 else WHITE)
for sx, sy in [(GX - 17, GY), (GX + 17, GY)]:            # a bright core in each loop
    for d in (-1, 1): put(sx + d, sy, YEL); put(sx, sy + d, YEL)
    put(sx, sy, WHITE)
# stop markers: a teal box with the mission number
DIG = {1: ["010", "110", "010", "010", "111"], 2: ["110", "001", "010", "100", "111"], 3: ["110", "001", "010", "001", "110"],
       4: ["101", "101", "111", "001", "001"], 5: ["111", "100", "110", "001", "110"], 6: ["011", "100", "110", "101", "010"],
       7: ["010", "101", "111", "101", "101"]}                  # A: Arcade
MARK = [(2, 1), (6, 4), (12, 0), (18, 3), (11, 9), (13, 14), (6, 10)]            # tile column, row of each number box
for n, (mx, my) in enumerate(MARK, 1):
    bx, by = mx * 8 + 2, my * 8 + 3
    for yy in range(-2, 8):
        for xx in range(-2, 6):
            edge = xx in (-2, 5) or yy in (-2, 7); corner = xx in (-2, 5) and yy in (-2, 7)
            put(bx + xx, by + yy, TEAL if edge and not corner else BG)
    for r, row in enumerate(DIG[n]):
        for c, ch in enumerate(row):
            if ch == '1': put(bx + c + 0, by + r, WHITE)

# ---- fit: each 8x8 tile takes whichever of the seven palettes loses least
a = np.asarray(img, float)
out = np.zeros_like(a); tpal = np.zeros((18, 20), int); idx = np.zeros((18, 20, 8, 8), int); bad = 0
P = np.array(PALS, float)
for ty in range(18):
    for tx in range(20):
        t = a[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8].reshape(64, 3)
        d = ((t[None, :, None, :] - P[:, None, :, :]) ** 2).sum(-1)       # pal x px x col
        e = d.min(-1).sum(-1); p = int(e.argmin()); tpal[ty, tx] = p
        if e[p] > 0: bad += 1
        ix = d[p].argmin(-1); idx[ty, tx] = ix.reshape(8, 8)
        out[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] = P[p][ix].reshape(8, 8, 3)
res = Image.fromarray(out.astype(np.uint8))
res.save('art/mission_chart/star_chart_160x144.png'); res.resize((W * 4, H * 4), Image.NEAREST).save('art/mission_chart/star_chart_preview_4x.png')
tiles = set(idx[ty, tx].tobytes() for ty in range(18) for tx in range(20))
print('palettes', len(PALS), '| unique tiles', len(tiles), '| tiles that lost a colour', bad, '| palette use', {NAMES[i]: int((tpal == i).sum()) for i in range(7)})


# ---- emit src/chart_art.c / .h: tiles deduplicated (with flips), map, attributes, palettes
import os
SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src')
def rgb555(c): return min(31, round(c[0] / 255 * 31)) | (min(31, round(c[1] / 255 * 31)) << 5) | (min(31, round(c[2] / 255 * 31)) << 10)
uniq, tmap, tattr = [], [], []
for ty in range(18):
    for tx in range(20):
        t = idx[ty, tx]
        for fl, v in ((0, t), (0x20, t[:, ::-1]), (0x40, t[::-1]), (0x60, t[::-1, ::-1])):
            k = v.tobytes()
            hit = next((i for i, u in enumerate(uniq) if u.tobytes() == k), None)
            if hit is not None: break
        if hit is None: uniq.append(t.copy()); hit = len(uniq) - 1; fl = 0
        tmap.append(hit); tattr.append(int(tpal[ty, tx]) | 0x08 | fl)       # 0x08: tile data in VRAM bank 1
def tb(px8):
    out = []
    for row in px8:
        lo = hi = 0
        for x, v in enumerate(row):
            if v & 1: lo |= 0x80 >> x
            if v & 2: hi |= 0x80 >> x
        out += [lo, hi]
    return out
data = sum((tb(u) for u in uniq), [])
pals = [rgb555(c) for p in PALS for c in p]
with open(os.path.join(SRC, 'chart_art.c'), 'w') as f:
    f.write('// generated by tools/gen_mission_chart.py: the mission screen star chart\n#pragma bank 255\n#include <gb/gb.h>\n#include <stdint.h>\n#include "chart_art.h"\nBANKREF(chart_art)\n')
    f.write('const uint8_t chart_tiles[] = {%s};\n' % ','.join(map(str, data)))
    f.write('const uint8_t chart_map[] = {%s};\n' % ','.join(map(str, tmap)))
    f.write('const uint8_t chart_attr[] = {%s};\n' % ','.join(map(str, tattr)))
    f.write('const uint16_t chart_pals[] = {%s};\n' % ','.join('0x%04X' % v for v in pals))
with open(os.path.join(SRC, 'chart_art.h'), 'w') as f:
    f.write('#ifndef CHART_ART_H\n#define CHART_ART_H\n#include <gb/gb.h>\n#include <stdint.h>\nBANKREF_EXTERN(chart_art)\n')
    f.write('#define CHART_NT %d\n#define CHART_NPAL %d\n' % (len(uniq), len(PALS)))
    f.write('extern const uint8_t chart_tiles[], chart_map[], chart_attr[];\nextern const uint16_t chart_pals[];\n')
    f.write('// where the ship parks for each mission (sprite x, y), next to its stop on the chart\n')
    PARK = [(4, 42), (34, 54), (70, 18), (142, 58), (82, 84), (84, 112), (4, 96)]   # the last one is Arcade, in the open lower left      # screen x, y of the 16x16 ship, clear of each world
    f.write('#define CHART_STOPS {%s}\n#endif\n' % ', '.join('{%d, %d}' % (x + 8, y + 16) for x, y in PARK))
print('chart_art:', len(uniq), 'tiles,', len(data) + 720 + len(pals) * 2, 'bytes')
