#!/usr/bin/env python3
"""Art for mission 5, the gun-cam (cockpit) stage. Standalone: needs only Pillow + numpy
and ref/dex/flatwoods_strip_1x.png. Writes src/cockpit_data.h and src/cockpit_b25.c / _b26.c.

VRAM use in the cockpit stage
  BG  bank0 ids N_COMMON..      backdrop (space, planet limb, moon)
  BG  bank1 ids 0..             dashboard rim, radar, canopy arch
  BG  bank1 ids 112..127        comm mini portrait (engine)
  BG  bank1 ids 128..           Flatwoods Monster (two poses)
  OBJ bank1                     crosshair, bolts, saucers, rocks, orbs, stars
BG palettes: 0 space, 1 planet, 2/3 boss, 4 comm, 5 HUD, 6 metal, 7 radar
"""
import os, sys, math, random
import numpy as np
from PIL import Image, ImageDraw
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dex, gbconv
from emit import Out, pal_words

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
PREV = os.path.join(ROOT, "build", "preview")
os.makedirs(PREV, exist_ok=True)
N_COMMON = 66
BANK, BANK2 = 26, 27
o = Out(os.path.join(ROOT, "src"), header="cockpit_data.h", prefix="cockpit_b")
o.define("BANK_CK", BANK)
o.define("BANK_CK2", BANK2)
rng = random.Random(511)
hx = lambda s: tuple(int(s[i:i + 2], 16) for i in (1, 3, 5))

SPACE = '#000008'
PAL_SPACE = [SPACE, '#1E2A58', '#7888C8', '#F0F4FF']
PAL_PLANET = [SPACE, '#0E3A2C', '#2C8858', '#A8F0A0']
PAL_BOSS_A = [SPACE, '#2A1810', '#6A4A31', '#41EEF6']     # outline, cloak, eyes/jet
PAL_BOSS_B = [SPACE, '#2A1810', '#6A4A31', '#FF4A31']     # outline, cloak, red gem
PAL_METAL = [SPACE, '#1A2230', '#48566C', '#A8B8CC']
PAL_RADAR = ['#02140C', '#0C4028', '#28A060', '#B0F8C0']


def enc(t):
    return gbconv.tile_bytes(t)


class Tiles:
    def __init__(self):
        self.t, self.k = [], {}
    def add(self, a):
        key = bytes(np.asarray(a, np.uint8).flatten())
        if key not in self.k:
            self.k[key] = len(self.t); self.t.append(np.asarray(a, np.uint8))
        return self.k[key]
    def data(self):
        return sum((enc(t) for t in self.t), [])


def tileize(img, ts, attr_fn):
    """img: HxW index array. Returns (map, attr) lists row-major."""
    h, w = img.shape
    m, a = [], []
    for ty in range(h // 8):
        for tx in range(w // 8):
            m.append(ts.add(img[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]))
            a.append(attr_fn(tx, ty))
    return m, a


def render(img, palmap, path, scale=3):
    """palmap: function (tx, ty) -> palette (4 hex)."""
    h, w = img.shape
    im = Image.new("RGB", (w, h)); px = im.load()
    for y in range(h):
        for x in range(w):
            px[x, y] = hx(palmap(x // 8, y // 8)[img[y, x]])
    im.resize((w * scale, h * scale), Image.NEAREST).save(path)


# ============================================================ backdrop (256 x 128)
BW, BH = 256, 128
bg = np.zeros((BH, BW), np.uint8)
bgpal = np.zeros((BH // 8, BW // 8), np.uint8)
# stars
for _ in range(64):
    x, y = rng.randrange(BW), rng.randrange(BH)
    bg[y, x] = 1
for _ in range(26):
    x, y = rng.randrange(BW), rng.randrange(BH)
    bg[y, x] = 2
for _ in range(11):
    x, y = rng.randrange(2, BW - 2), rng.randrange(2, BH - 2)
    bg[y, x] = 3
    if rng.random() < 0.6:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)): bg[y + dy, x + dx] = 2
# nebula wisps left and right of the boss block (tile-aligned dither keeps tiles few)
def wisp(x0, y0, w, h, seed):
    r = random.Random(seed)
    ph = [r.random() * 6.28 for _ in range(4)]
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            u, v = (x - x0) / w, (y - y0) / h
            d = math.sin(u * 5 + ph[0]) * 0.25 + math.sin(v * 7 + u * 3 + ph[1]) * 0.25 + 0.55
            d *= math.sin(u * math.pi) * math.sin(v * math.pi)
            if d > 0.42 and (x + y) % 2 == 0 and y % 2 == 0: bg[y, x % BW] = max(bg[y, x % BW], 1)
            elif d > 0.30 and x % 4 == 0 and y % 4 == 0: bg[y, x % BW] = max(bg[y, x % BW], 1)
wisp(8, 8, 72, 48, 3)
wisp(176, 24, 72, 40, 9)
# planet limb along the bottom: centre (128, 424) r 312
PCX, PCY, PR = 128, 409, 312
for y in range(BH):
    for x in range(BW):
        d = math.hypot(x - PCX, y - PCY)
        if d <= PR:
            depth = PR - d
            lat = (y * 3 + int(8 * math.sin(x / 23.0))) // 5
            if depth < 1.5: c = 3                                  # bright atmosphere rim
            elif depth < 4: c = 2 if (x + y) % 2 else 3
            elif depth < 9: c = 2
            elif depth < 14: c = 2 if ((x + y) % 2 or lat % 2) else 1
            else: c = 1 if (lat % 3 or (x + y) % 2) else 2         # cloud bands
            bg[y, x] = c
            bgpal[y // 8, x // 8] = 1
        elif d <= PR + 2 and (x + y) % 2 == 0:
            bg[y, x] = 1; bgpal[y // 8, x // 8] = 1                # thin glow
# a small moon, right of the boss block
MX, MY, MR = 206, 46, 9
for y in range(MY - MR, MY + MR + 1):
    for x in range(MX - MR, MX + MR + 1):
        d = math.hypot(x - MX, y - MY)
        if d <= MR:
            lit = (x - MX) * -0.7 + (y - MY) * -0.7
            c = 3 if lit > 4 else 2 if lit > -2 else 1
            if (x * 7 + y * 3) % 11 == 0 and c > 1: c -= 1
            bg[y, x] = c
for ty in range(BH // 8):
    for tx in range(BW // 8):
        if bgpal[ty, tx] == 1:                                    # planet tiles: stars become pale green
            pass
bts = Tiles()
bts.add(np.zeros((8, 8), np.uint8))
bmap, battr = tileize(bg, bts, lambda tx, ty: int(bgpal[ty, tx]))
assert len(bts.t) <= 256 - N_COMMON, len(bts.t)
o.carray(BANK, "ck_bg_tiles", bts.data())
o.define("CK_BG_NT", len(bts.t))
o.carray(BANK, "ck_bg_map", bmap)
o.carray(BANK, "ck_bg_attr", battr)
o.define("CK_BG_W", BW // 8); o.define("CK_BG_H", BH // 8)
render(bg, lambda tx, ty: PAL_PLANET if bgpal[ty, tx] else PAL_SPACE, f"{PREV}/ck_backdrop.png")
print("backdrop tiles:", len(bts.t))

# ============================================================ Flatwoods Monster (BG, 64 x 64, two poses)
# sheet colours: (16,8,8) outline, (106,74,49) cloak, (156,24,24)/(255,74,49) gem, (65,238,246) eyes/jet
CM = {(16, 8, 8): 1, (106, 74, 49): 2, (156, 24, 24): 5, (255, 74, 49): 4, (65, 238, 246): 3}
def boss_pose(fr):
    a = dex.frame('flatwoods', fr, CM)
    big = dex.scale2(a)                       # 64 x 64, EPX
    return big
def boss_tiles(big, ts):
    m, at = [], []
    for ty in range(8):
        for tx in range(8):
            t = big[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8].copy()
            red = ((t == 4) | (t == 5)).any()
            if red:
                t[t == 3] = 2                 # (no tile has both eyes and gem)
                t[t == 5] = 1; t[t == 4] = 3
                pal = 3
            else:
                pal = 2
            if not t.any():                   # empty cell: the backdrop shows through
                k = (BOSS_ROW + ty) * (BW // 8) + BOSS_COL + tx
                m.append(N_COMMON + bmap[k]); at.append(battr[k])
            else:
                m.append(128 + ts.add(t)); at.append(pal | 0x08)
    return m, at
BOSS_COL, BOSS_ROW = 12, 4
o.define("BOSS_COL", BOSS_COL); o.define("BOSS_ROW", BOSS_ROW)
kts = Tiles()
poses = [boss_pose('idle0'), boss_pose('atk0'), boss_pose('hurt')]
# keep the three poses on a common footprint: shift so the cloak stays put
bm = [boss_tiles(p, kts) for p in poses]
assert len(kts.t) <= 128, len(kts.t)
o.carray(BANK, "ck_boss_tiles", kts.data())
o.define("CK_BOSS_NT", len(kts.t))
for i, (m, at) in enumerate(bm):
    o.carray(BANK, f"ck_boss_map{i}", m)
    o.carray(BANK, f"ck_boss_attr{i}", at)
print("boss tiles:", len(kts.t))
def fade(pal, k):                             # k 0..3 -> black..full
    return [SPACE] + ['#%02X%02X%02X' % tuple(int(v * k / 3) for v in hx(c)) for c in pal[1:]]
fades = []
for k in range(4):
    fades += pal_words(fade(PAL_BOSS_A, k)) + pal_words(fade(PAL_BOSS_B, k))
o.carray(BANK, "ck_boss_fade", fades, "uint16_t")          # 4 steps x 2 palettes
o.carray(BANK, "ck_boss_hit", pal_words([SPACE, '#F8F8F8', '#F8F8F8', '#F8F8F8']) * 2, "uint16_t")
o.carray(BANK, "ck_boss_charge", pal_words([SPACE, '#2A1810', '#8A3A31', '#F8F8A0']) + pal_words([SPACE, '#2A1810', '#8A3A31', '#F8F8A0']), "uint16_t")
for i, p in enumerate(poses):
    dex.preview([p], ['#000008', '#2A1810', '#6A4A31', '#41EEF6', '#FF4A31', '#9C1818'], f"{PREV}/ck_boss{i}.png", 4)
ys, xs = np.nonzero(poses[0])
print("boss bbox idle:", xs.min(), ys.min(), xs.max(), ys.max())

# ============================================================ dashboard (BG bank 1 from id 0)
dts = Tiles()
# canopy arch: 160 x 16
can = np.zeros((16, 160), np.uint8)
for x in range(160):
    u = (x - 79.5) / 80.0
    th = int(round(3 + 12 * u * u))
    for y in range(th):
        can[y, x] = 2
    can[th - 1, x] = 1
    if th >= 2: can[0, x] = 3
    if th >= 6 and x % 2 == 0: can[th - 3, x] = 1
for x in (76, 77, 78, 79, 80, 81, 82, 83):              # centre brace
    for y in range(6): can[y, x] = 2
    can[5, x] = 1
can[2, 79] = can[2, 80] = 3
for bx in (8, 28, 131, 151):                            # bolts
    can[2:4, bx:bx + 2] = 3; can[4, bx:bx + 2] = 1
cmap_, cattr = tileize(can, dts, lambda tx, ty: 6)
# rim: 160 x 8 (every pixel non-zero so sprites stay behind it)
rim = np.full((8, 160), 2, np.uint8)
rim[0, :] = 1; rim[1, :] = 3; rim[2, :] = 3; rim[6, :] = 1; rim[7, :] = 1
for x in range(160):
    if x % 16 in (7, 8): rim[3:5, x] = 1
    if x % 16 == 7: rim[3, x] = 3
rmap, rattr = tileize(rim, dts, lambda tx, ty: 6 | 0x80)
# radar scope: 32 x 32
rad = np.full((32, 32), 0, np.uint8)
for y in range(32):
    for x in range(32):
        d = math.hypot(x - 15.5, y - 15.5)
        if d > 15.5: rad[y, x] = 1
        elif d > 14.2: rad[y, x] = 2
        elif abs(d - 8) < 0.6 or (abs(x - 15.5) < 0.6 or abs(y - 15.5) < 0.6) and (x + y) % 2: rad[y, x] = 1
        ang = math.atan2(y - 15.5, x - 15.5)
        if d < 14 and -1.1 < ang < -0.5: rad[y, x] = 1 if (x + y) % 2 else rad[y, x]
for (x, y) in ((21, 8), (9, 12), (19, 22)):
    rad[y, x] = 3; rad[y, x + 1] = 2; rad[y + 1, x] = 2
dmap, dattr = tileize(rad, dts, lambda tx, ty: 7 | 0x80)
# divider column
div = np.full((8, 8), 0, np.uint8)
div[:, 3] = 1; div[:, 4] = 2
t_div = dts.add(div)
assert len(dts.t) <= 112, len(dts.t)
o.carray(BANK, "ck_dash_tiles", dts.data())
o.define("CK_DASH_NT", len(dts.t))
o.carray(BANK, "ck_canopy_map", cmap_); o.carray(BANK, "ck_canopy_attr", [a | 0x08 | 0x80 for a in cattr])
o.carray(BANK, "ck_rim_map", rmap); o.carray(BANK, "ck_rim_attr", [a | 0x08 for a in rattr])
o.carray(BANK, "ck_radar_map", dmap); o.carray(BANK, "ck_radar_attr", [a | 0x08 for a in dattr])
o.define("CK_T_DIV", t_div)
print("dash tiles:", len(dts.t))

bgp = []
for p in (PAL_SPACE, PAL_PLANET, PAL_BOSS_A, PAL_BOSS_B): bgp += pal_words(p)
o.carray(BANK, "ck_bgpal", bgp, "uint16_t")                 # slots 0-3
o.carray(BANK, "ck_dashpal", pal_words(PAL_METAL) + pal_words(PAL_RADAR), "uint16_t")   # slots 6-7

# ============================================================ sprites (OBJ bank 1)
SS = 4
def canvas(w, h):
    im = Image.new("L", (w * SS, h * SS), 0)
    return im, ImageDraw.Draw(im)
def down(im, w, h):
    a = np.asarray(im).reshape(h, SS, w, SS).transpose(0, 2, 1, 3).reshape(h, w, SS * SS)
    out = np.zeros((h, w), np.uint8)
    for y in range(h):
        for x in range(w):
            v = np.bincount(a[y, x], minlength=4)
            if v[0] > SS * SS * 0.55: continue
            v[0] = 0
            out[y, x] = int(v.argmax())
    return out
def E(d, box, fill):
    d.ellipse([v * SS for v in box], fill=fill)
def R(d, box, fill):
    d.rectangle([v * SS for v in box], fill=fill)

def saucer(w, h):
    """Flatwoods scout saucer, front view. Returns h x w index array."""
    im, d = canvas(w, h)
    cx, cy = w / 2, h * 0.56
    rw, rh = w * 0.47, h * 0.20
    E(d, (cx - rw, cy - rh, cx + rw, cy + rh), 3)
    E(d, (cx - rw + 1, cy - rh + 1, cx + rw - 1, cy + rh - 1), 2)
    E(d, (cx - rw * 0.8, cy - rh * 0.9, cx + rw * 0.8, cy - rh * 0.05), 1)        # top sheen
    dw, dh = w * 0.22, h * 0.26
    E(d, (cx - dw, cy - rh - dh * 0.9, cx + dw, cy - rh * 0.2), 3)                 # dome
    E(d, (cx - dw + 1, cy - rh - dh * 0.9 + 1, cx + dw - 1, cy - rh * 0.2 - 0.5), 1)
    if w >= 16:
        E(d, (cx - dw * 0.45, cy - rh - dh * 0.5, cx + dw * 0.1, cy - rh * 0.55), 2)  # pilot shadow
        n = 5 if w >= 32 else 3
        for i in range(n):                                                         # belly lights
            lx = cx + (i - (n - 1) / 2) * rw * 1.5 / n
            E(d, (lx - w * 0.035, cy + rh * 0.25, lx + w * 0.035, cy + rh * 0.25 + w * 0.07), 1)
        E(d, (cx - rw * 0.3, cy + rh * 0.7, cx + rw * 0.3, cy + rh * 1.5), 3)      # emitter
        E(d, (cx - rw * 0.16, cy + rh * 0.9, cx + rw * 0.16, cy + rh * 1.35), 1)
    return down(im, w, h)

def rock(w, h, seed):
    r = random.Random(seed)
    im, d = canvas(w, h)
    cx, cy = w / 2, h / 2
    n = 9
    rad = [min(w, h) * (0.36 + r.random() * 0.12) for _ in range(n)]
    pts = [(cx + math.cos(i * 2 * math.pi / n) * rad[i], cy + math.sin(i * 2 * math.pi / n) * rad[i]) for i in range(n)]
    d.polygon([(x * SS, y * SS) for x, y in pts], fill=3)
    d.polygon([((cx + (x - cx) * 0.86) * SS, (cy + (y - cy) * 0.86) * SS) for x, y in pts], fill=2)
    d.polygon([((cx - w * 0.06 + (x - cx) * 0.5) * SS, (cy - h * 0.1 + (y - cy) * 0.45) * SS) for x, y in pts], fill=1)
    if w >= 16:
        for _ in range(3 if w >= 32 else 1):
            ax, ay = cx + (r.random() - 0.4) * w * 0.4, cy + (r.random() - 0.2) * h * 0.4
            s = w * (0.05 + r.random() * 0.05)
            E(d, (ax - s, ay - s * 0.7, ax + s, ay + s * 0.7), 3)
    return down(im, w, h)

def orb(w, h, r):
    im, d = canvas(w, h)
    cx, cy = w / 2, h / 2
    E(d, (cx - r, cy - r, cx + r, cy + r), 3)
    E(d, (cx - r + 0.8, cy - r + 0.8, cx + r - 0.8, cy + r - 0.8), 2)
    if r > 2:
        E(d, (cx - r * 0.55, cy - r * 0.6, cx + r * 0.2, cy + r * 0.15), 1)
    return down(im, w, h)

spr = []
def add_sprite(name, a):
    """a: h x w index array, w multiple of 8, h multiple of 16."""
    h, w = a.shape
    first = len(spr)
    for band in range(h // 16):
        for col in range(w // 8):
            spr.append(a[band * 16:band * 16 + 8, col * 8:col * 8 + 8])
            spr.append(a[band * 16 + 8:band * 16 + 16, col * 8:col * 8 + 8])
    o.define(f"CK_ST_{name}", first)
def pad(a, w, h):
    return dex.place(a, w, h)
def rows(lst):
    return np.array([[0 if c == '.' else int(c) for c in r] for r in lst], np.uint8)

XH = rows([
 "......1111......",
 "....11....11....",
 "...1........1...",
 "..1....11....1..",
 ".1.....11.....1.",
 ".1............1.",
 "1..............1",
 "1..11......11..1",
 "1..11......11..1",
 "1..............1",
 ".1............1.",
 ".1.....11.....1.",
 "..1....11....1..",
 "...1........1...",
 "....11....11....",
 "......1111......"])
XL = rows([
 "111..........111",
 "1..............1",
 "1.....2222.....1",
 "....22....22....",
 "...2........2...",
 "...2...11...2...",
 "..2....11....2..",
 "..2..11..11..2..",
 "..2..11..11..2..",
 "..2....11....2..",
 "...2...11...2...",
 "...2........2...",
 "....22....22....",
 "1.....2222.....1",
 "1..............1",
 "111..........111"])
add_sprite("XHAIR", XH)
add_sprite("XLOCK", XL)
BOLT = np.zeros((16, 8), np.uint8)                       # diagonal laser streak (lower-left to upper-right)
for i in range(8):
    y = 15 - i * 2
    BOLT[y, i] = 1; BOLT[y - 1, i] = 1
    if i > 0: BOLT[y, i - 1] = 2
    if i < 7: BOLT[y - 1, i + 1] = 2
add_sprite("BOLT", BOLT)
BOLT2 = np.zeros((16, 8), np.uint8)                      # shorter, nearer the target
for i in range(2, 6):
    y = 13 - i * 2
    BOLT2[y, i] = 1; BOLT2[y + 1, i] = 2
add_sprite("BOLT2", BOLT2)
S1 = np.zeros((16, 8), np.uint8); S1[8, 3] = 2
S2 = np.zeros((16, 8), np.uint8); S2[8, 3] = 1; S2[8, 4] = 2
S3 = np.zeros((16, 8), np.uint8); S3[7:9, 3:5] = 1
add_sprite("STAR1", S1); add_sprite("STAR2", S2); add_sprite("STAR3", S3)
for nm, fn in (("UFO", saucer),):
    add_sprite(nm + "_S", pad(fn(8, 8), 8, 16))
    add_sprite(nm + "_M", fn(16, 16))
    add_sprite(nm + "_L", fn(32, 32))
add_sprite("ROCK_S", pad(rock(8, 8, 5), 8, 16))
add_sprite("ROCK_M", rock(16, 16, 6))
add_sprite("ROCK_L", rock(32, 32, 7))
add_sprite("ORB_S", pad(orb(8, 8, 2.4), 8, 16))
add_sprite("ORB_M", orb(16, 16, 4.6))
add_sprite("ORB_L", orb(16, 16, 7.4))
GL = np.zeros((16, 8), np.uint8)                         # eye glow for the beam charge
GL[5:11, 2:6] = 2; GL[6:10, 1:7] = 2; GL[7:9, 3:5] = 1; GL[6:10, 3:5] = 1; GL[7:9, 2:6] = 1
add_sprite("GLOW", GL)
assert len(spr) <= 128, len(spr)
o.carray(BANK, "ck_spr", sum((enc(t) for t in spr), []))
o.define("CK_SPR_NT", len(spr))
OBJP = [
    ['#000000', '#F8FFF0', '#58E860', '#0A4A1C'],   # 3 orbs (green plasma)
    ['#000000', '#D8F4FF', '#5C7C9C', '#101828'],   # 4 scout saucer
    ['#000000', '#E0BC90', '#8A5A3A', '#2A1810'],   # 5 rock
    ['#000000', '#FFE890', '#D03838', '#380810'],   # 6 elite saucer / big orb / warning
]
o.carray(BANK, "ck_objpal", sum((pal_words(p) for p in OBJP), []), "uint16_t")
print("sprite tiles:", len(spr))
# preview sheet
def sheet():
    items = [(XH, 0), (XL, 0), (saucer(8, 8), 1), (saucer(16, 16), 1), (saucer(32, 32), 1), (saucer(32, 32), 3),
             (rock(8, 8, 5), 2), (rock(16, 16, 6), 2), (rock(32, 32, 7), 2), (orb(8, 8, 2.4), 0), (orb(16, 16, 4.6), 0), (orb(16, 16, 7.4), 0), (orb(16, 16, 7.4), 3)]
    pals = [OBJP[0], OBJP[1], OBJP[2], OBJP[3]]
    W = sum(a.shape[1] + 3 for a, _ in items)
    im = Image.new("RGB", (W, 34), (0, 0, 8)); px = im.load(); x0 = 1
    for a, p in items:
        for y in range(a.shape[0]):
            for x in range(a.shape[1]):
                if a[y, x]: px[x0 + x, 1 + y] = hx((['#000', '#48E0C8', '#F8F8F8', '#2870C8'] if p == 0 and a is XH or a is XL else pals[p])[a[y, x]] if not (a is XH or a is XL) else ['#000000', '#48E0C8', '#F8D038', '#2870C8'][a[y, x]])
        x0 += a.shape[1] + 3
    im.resize((W * 4, 34 * 4), Image.NEAREST).save(f"{PREV}/ck_sprites.png")
sheet()

# ============================================================ cutscene scene (160 x 72, 3 palettes) -> bank 26
# palettes: 0 space/planet/saucers (greens), 1 monster (eyes), 2 monster (gem)
SCP = [[SPACE, '#0E3A2C', '#2C8858', '#C8F8C0'], PAL_BOSS_A, PAL_BOSS_B]
sc = np.zeros((72, 160), np.uint8); scp = np.zeros((9, 20), np.uint8)
r2 = random.Random(77)
for _ in range(60):
    sc[r2.randrange(60), r2.randrange(160)] = r2.choice([1, 2, 2, 3])
for y in range(72):
    for x in range(160):
        d = math.hypot(x - 80, y - 262)
        if d < 200:
            depth = 200 - d
            sc[y, x] = 3 if depth < 1.5 else 2 if depth < 5 or ((x + y) % 2 and depth < 9) else 1
su = saucer(32, 32)
for (ox, oy) in ((6, 8), (122, 16)):
    for y in range(32):
        for x in range(32):
            if su[y, x]: sc[oy + y, ox + x] = {1: 3, 2: 2, 3: 1}[int(su[y, x])]
big = dex.scale2(dex.frame('flatwoods', 'atk0', CM))
for ty in range(8):
    for tx in range(8):
        t = big[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8].copy()
        if not t.any(): continue
        if ((t == 4) | (t == 5)).any():
            t[t == 3] = 2; t[t == 5] = 1; t[t == 4] = 3; scp[ty, 6 + tx] = 2
        else: scp[ty, 6 + tx] = 1
        sc[ty * 8:ty * 8 + 8, 48 + tx * 8:56 + tx * 8] = t
sts = Tiles()
smap, sattr = tileize(sc, sts, lambda tx, ty: int(scp[ty, tx]))
o.carray(BANK2, "scene_flat_tiles", sts.data())
o.carray(BANK2, "scene_flat_pals", sum((pal_words(p) for p in SCP), []), "uint16_t")
o.carray(BANK2, "scene_flat_map", smap)
o.carray(BANK2, "scene_flat_attr", sattr)
o.define("SCENE_FLAT_NT", len(sts.t))
render(sc, lambda tx, ty: SCP[scp[ty, tx]], f"{PREV}/scene_flat.png")
print("scene tiles:", len(sts.t))

print(o.write("COCKPIT_DATA_H"))
