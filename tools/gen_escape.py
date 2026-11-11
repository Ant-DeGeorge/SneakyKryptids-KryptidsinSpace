#!/usr/bin/env python3
"""Art for mission 6 "Escape Run". Standalone: needs only Pillow + numpy and ref/escape/graul_*.png.
Writes src/escape_data.h and src/escape_art.c (autobanked, BANK(escape_art)).

VRAM use in the escape stage
  BG  bank0 ids N_COMMON..      field: streak stars, boulders, laser column, cloud wisps
  BG  bank1 ids 0..             the pursuer (Nubs's flagship), shown in the 9C00 map rows 8-14
  BG  bank1 ids 112..127        comm mini portrait (engine)
  OBJ bank1                     missiles, rocks, chevrons, sparks, streaks, brig pod
BG palettes: 0 space, 1 rock, 2/3 pursuer, 4 comm, 5 HUD, 6 laser, 7 status line
Colour rules: hazards are warm and bright (white / yellow / orange), the player side is cool
(cyan / blue), and nothing depends on telling red from green.
"""
import os, sys, math, random
import numpy as np
from PIL import Image, ImageDraw, ImageEnhance
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gbconv
from emit import pal_words

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
PREV = os.path.join(ROOT, "build", "preview")
os.makedirs(PREV, exist_ok=True)
hx = lambda s: tuple(int(s[i:i + 2], 16) for i in (1, 3, 5))
H, C = [], []                      # header lines, C lines


def carray(name, data, typ="uint8_t"):
    data = [int(v) for v in data]
    H.append(f"extern const {typ} {name}[];  // {len(data)}")
    body = ",".join(f"0x{v:04X}" for v in data) if typ == "uint16_t" else ",".join(map(str, data))
    C.append(f"const {typ} {name}[] = {{{body}}};")
    carray.size += len(data) * (2 if typ == "uint16_t" else 1)
carray.size = 0
def define(n, v): H.append(f"#define {n} {v}")
enc = gbconv.tile_bytes


class Tiles:
    def __init__(self):
        self.t, self.k = [], {}
    def add(self, a, flip=False):
        a = np.asarray(a, np.uint8)
        key = a.tobytes()
        if key in self.k: return self.k[key], 0
        if flip and a[:, ::-1].tobytes() in self.k: return self.k[a[:, ::-1].tobytes()], 0x20
        self.k[key] = len(self.t); self.t.append(a)
        return self.k[key], 0
    def data(self):
        return sum((enc(t) for t in self.t), [])


def render(img, palfn, path, scale=3):
    h, w = img.shape
    im = Image.new("RGB", (w, h)); px = im.load()
    for y in range(h):
        for x in range(w):
            px[x, y] = hx(palfn(x // 8, y // 8)[img[y, x]])
    im.resize((w * scale, h * scale), Image.NEAREST).save(path)


SPACE = '#000008'
PAL_SPACE = [SPACE, '#1C2850', '#5C6CB0', '#E8F0FF']
PAL_ROCK = [SPACE, '#3A2C28', '#8C7460', '#E0D0B0']
PAL_STEEL = [SPACE, '#262C3C', '#5C6880', '#B4C0D4']
PAL_WARM = [SPACE, '#5C6880', '#F08020', '#FFE860']
# laser column: one tile, three palettes. Warning shows only the thin centre line.
PAL_LASER = [[SPACE, '#FFE860', SPACE, SPACE],          # warning, bright
             [SPACE, '#A06010', SPACE, SPACE],          # warning, dim
             [SPACE, '#FFFFFF', '#FFE860', '#F08020']]  # firing
PAL_TAG = ['#0C0E18', '#2C3448', '#F08020', '#FFE860']  # status line: amber on the HUD panel
# space -> upper atmosphere (dusky violet): 4 steps, hazards stay the brightest things on screen
ATMO = [PAL_SPACE,
        ['#06041A', '#2A2460', '#6C64B8', '#E8F0FF'],
        ['#100830', '#3C2C70', '#8470C0', '#F0F0FF'],
        ['#1C1044', '#402E74', '#9C84C8', '#F8F4FF']]
# final dive glow: soft fade toward a pale haze, then out
GLOW = [['#1C1044', '#503C84', '#9C84C8', '#F8F4FF'],
        ['#3C2C64', '#70609C', '#B4A4D4', '#F8F4FF'],
        ['#6C5C8C', '#9888B4', '#CCC0E0', '#F8F4FF'],
        ['#A498B8', '#C0B4D0', '#E0D8EC', '#F8F4FF'],
        ['#6C5C8C', '#70609C', '#8470A8', '#A498B8'],
        ['#241C38', '#2C2444', '#342C50', '#3C3458'],
        ['#000000', '#000000', '#000000', '#000000']]

# ============================================================ field tiles (BG bank 0)
rng = random.Random(606)
ft = []
def ftile(name, a):
    define(f"ER_T_{name}", f"(N_COMMON + {len(ft)})"); ft.append(np.asarray(a, np.uint8))
Z = lambda: np.zeros((8, 8), np.uint8)
ftile("BLANK", Z())
a = Z(); a[1:4, 2] = 1; a[4:6, 2] = 2; a[6, 2] = 3; ftile("STAR0", a)          # short streak
a = Z(); a[0:3, 5] = 1; a[3:6, 5] = 2; a[6:8, 5] = 3; ftile("STAR1", a)         # long streak
a = Z(); a[2:5, 1] = 1; a[5, 1] = 2; a[1:3, 6] = 1; ftile("STAR2", a)           # two faint
def boulder(seed):
    r = random.Random(seed); a = Z()
    cx, cy = 3.5 + r.uniform(-.4, .4), 3.5 + r.uniform(-.4, .4)
    for y in range(8):
        for x in range(8):
            ang = math.atan2(y - cy, x - cx)
            rad = 4.1 + 0.5 * math.sin(ang * 3 + seed) + 0.3 * math.sin(ang * 5 + seed * 2)
            d = math.hypot(x - cx, y - cy)
            if d < rad:
                lit = (x - cx) * -0.6 + (y - cy) * -0.7
                a[y, x] = 3 if lit > 1.6 and d < rad - 1 else 2 if lit > -1.2 else 1
    for _ in range(2):
        x, y = r.randrange(2, 6), r.randrange(2, 6); a[y, x] = 1
    return a
for i in range(4): ftile(f"ROCK{i}" if i else "ROCK", boulder(11 + i * 7))
a = Z(); a[:, 0] = 3; a[:, 7] = 3; a[:, 1:3] = 2; a[:, 5:7] = 2; a[:, 3:5] = 1; a[::2, 0] = 0; a[1::2, 7] = 0
ftile("LASER", a)
def wisp(seed):
    r = random.Random(seed); a = Z()
    for y in range(8):
        for x in range(8):
            v = math.sin((x + seed) * 0.9) + math.sin(y * 1.7 + seed) + r.uniform(-.5, .5)
            if abs(y - 3.5 - math.sin((x + seed) * 0.8)) < 1.6 and v > -1.2: a[y, x] = 1      # one dark colour: wisps must never compete with a shot
    return a
for i in range(3): ftile(f"WISP{i}" if i else "WISP", wisp(3 + i * 5))
a = Z(); a[:, :] = 1; a[::2, ::2] = 2; a[1::2, 1::2] = 2; ftile("HAZE", a)     # cloud deck at the very end
carray("er_field_tiles", sum((enc(t) for t in ft), []))
define("ER_FIELD_NT", len(ft))

# ============================================================ the pursuer: Nubs's flagship (160 x 56)
SS = 4
W_, H_ = 160, 56
im = Image.new("L", (W_ * SS, H_ * SS), 0); d = ImageDraw.Draw(im)
P = lambda pts: [(x * SS, y * SS) for x, y in pts]
hull = [(4, 0), (156, 0), (151, 13), (129, 25), (113, 30), (101, 43), (89, 52), (80, 55), (71, 52), (59, 43), (47, 30), (31, 25), (9, 13)]
d.polygon(P(hull), fill=2, outline=1); d.line(P(hull + hull[:1]), fill=1, width=SS)
def mir(fn):
    for s in (0, 1): fn(lambda x: x if not s else W_ - x)
# plating: lighter upper deck, panel seams
mir(lambda X: d.polygon(P([(X(10), 1), (X(60), 1), (X(56), 10), (X(16), 12)]), fill=3))
mir(lambda X: d.line(P([(X(34), 1), (X(30), 24)]), fill=1, width=SS))
mir(lambda X: d.line(P([(X(12), 14), (X(50), 20), (X(60), 42)]), fill=1, width=SS))
mir(lambda X: d.line(P([(X(64), 12), (X(64), 34)]), fill=1, width=SS))
d.line(P([(80, 30), (80, 54)]), fill=1, width=SS)
# cannon pylons with warm muzzles
MUZ = [22, 40]
def pylon(X, cx, y1):
    xa, xb = sorted((X(cx - 5), X(cx + 5)))
    d.rectangle([xa * SS, 8 * SS, xb * SS, y1 * SS], fill=2, outline=1, width=SS)
    d.rectangle([(xa + 2) * SS, 10 * SS, (xa + 4) * SS, (y1 - 6) * SS], fill=3)
    d.rectangle([(xa + 2) * SS, (y1 - 4) * SS, (xb - 2) * SS, (y1 + 1) * SS], fill=5)
    d.rectangle([(xa + 4) * SS, (y1 - 2) * SS, (xb - 4) * SS, (y1 + 1) * SS], fill=6)
mir(lambda X: pylon(X, MUZ[0], 30)); mir(lambda X: pylon(X, MUZ[1], 36))
# bridge: two slanted amber windows (a scowl), and a hazard chevron band
mir(lambda X: d.polygon(P([(X(60), 18), (X(76), 24), (X(76), 29), (X(62), 25)]), fill=5, outline=1))
mir(lambda X: d.polygon(P([(X(66), 22), (X(74), 25), (X(74), 27), (X(67), 25)]), fill=6))
for i in range(8):
    x0 = 48 + i * 8
    d.polygon(P([(x0, 4), (x0 + 4, 4), (x0 + 8, 9), (x0 + 4, 9)]), fill=5)
d.line(P([(46, 3), (114, 3)]), fill=1, width=SS); d.line(P([(48, 10), (112, 10)]), fill=1, width=SS)
# scrap teeth along the prow
for i in range(5):
    mir(lambda X, i=i: d.polygon(P([(X(60 + i * 4), 42 + i * 2.2), (X(64 + i * 4), 44 + i * 2.2), (X(60.5 + i * 4), 48.5 + i * 2.2)]), fill=3, outline=1))
# prow lamp
d.ellipse([77 * SS, 44 * SS, 83 * SS, 50 * SS], fill=5, outline=1); d.ellipse([79 * SS, 46 * SS, 81 * SS, 48 * SS], fill=6)
arr = np.asarray(im).reshape(H_, SS, W_, SS).transpose(0, 2, 1, 3).reshape(H_, W_, SS * SS)
pur = np.zeros((H_, W_), np.uint8)
for y in range(H_):
    for x in range(W_):
        v = np.bincount(arr[y, x], minlength=7)
        if v[0] > SS * SS * 0.5: continue
        v[0] = 0; pur[y, x] = int(v.argmax())
r = random.Random(9)
for _ in range(46):                                   # rivets and rust flecks
    x, y = r.randrange(6, 154), r.randrange(1, 50)
    if pur[y, x] == 2 and pur[y, min(x + 1, 159)] == 2: pur[y, x] = r.choice([1, 3, 3])
pur[:, 80:] = pur[:, 79::-1]                          # exact mirror, so tiles pair up with X-flip
pts = Tiles(); pmap, pattr = [], []
ppal = np.zeros((7, 20), np.uint8)
ptile = np.zeros((H_, W_), np.uint8)
for ty in range(7):
    for tx in range(20):
        t = pur[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8].copy()
        warm = (t >= 4).any()
        if warm: t = np.choose(t, [0, 0, 1, 1, 2, 2, 3]).astype(np.uint8)
        ppal[ty, tx] = 1 if warm else 0
        ptile[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8] = t
        i, fl = pts.add(t, flip=True)
        pmap.append(i); pattr.append((3 if warm else 2) | 0x08 | fl)
assert len(pts.t) <= 112, len(pts.t)
carray("er_pur_tiles", pts.data()); define("ER_PUR_NT", len(pts.t))
carray("er_pur_map", pmap); carray("er_pur_attr", pattr)
define("ER_PUR_W", 20); define("ER_PUR_H", 7)
carray("er_muz_x", [MUZ[0], MUZ[1], W_ - MUZ[1], W_ - MUZ[0], 80])      # muzzle centres, px from the image's left edge
carray("er_pur_pals", pal_words(PAL_STEEL) + pal_words(PAL_WARM), "uint16_t")
# HUD: distance bar cells (0..8 px filled) and a planet icon, BG bank 1 after the pursuer (HUD palette: 1 slate, 3 white)
hud = []
for f in range(9):
    a = np.zeros((8, 8), np.uint8); a[1, :] = 1; a[6, :] = 1; a[2:6, :f] = 3; hud.append(a)
pl = np.zeros((8, 8), np.uint8)
for y in range(8):
    for x in range(8):
        dd = math.hypot(x - 3.5, y - 3.5)
        if dd < 3.7: pl[y, x] = 1 if (x - 3.5) + (y - 3.5) > 2.2 else 3
hud.append(pl)
carray("er_hud_tiles", sum((enc(t) for t in hud), [])); define("ER_HUD_BASE", 64); define("ER_HUD_NT", len(hud))
render(ptile, lambda tx, ty: PAL_WARM if ppal[ty, tx] else PAL_STEEL, f"{PREV}/er_pursuer.png", 4)
print("pursuer tiles:", len(pts.t))

# ============================================================ sprites (OBJ VRAM bank 1), 8x16 units
spr = []
def add_sprite(name, a):
    a = np.asarray(a, np.uint8); h, w = a.shape
    define(f"ER_ST_{name}", len(spr))
    for col in range(w // 8):
        spr.append(a[0:8, col * 8:col * 8 + 8]); spr.append(a[8:16, col * 8:col * 8 + 8])
def rows(lst):
    return np.array([[0 if c == '.' else int(c) for c in r] for r in lst], np.uint8)
def disc(w, h, cx, cy, rr, seed, lumpy=0.6):
    a = np.zeros((h, w), np.uint8)
    for y in range(h):
        for x in range(w):
            ang = math.atan2(y - cy, x - cx)
            rad = rr + lumpy * math.sin(ang * 3 + seed) + lumpy * 0.6 * math.sin(ang * 5 + seed * 1.7)
            dd = math.hypot(x - cx, y - cy)
            if dd < rad:
                lit = (x - cx) * -0.6 + (y - cy) * -0.7
                a[y, x] = 1 if dd > rad - 1.1 or lit < -rr * 0.45 else 3 if lit > rr * 0.4 else 2
    return a
# homing missile: a finned seeker, nose down (pal: 1 white, 2 yellow, 3 orange); two flame frames
MIS = ["...33...", "...33...", "..3223..", "..3223..", ".332233.", ".3.22.3.", "...22...", "..2112..",
       "..2112..", "..2112..", "..2112..", "..2112..", "...11...", "...11...", "........", "........"]
m0 = rows(MIS); m1 = m0.copy(); m1[0:2, 3:5] = 0; m1[0, 3] = 3
add_sprite("MISSILE", m0); add_sprite("MISSILE2", m1)
MISD = ["3.......", "33......", ".332....", ".3222...", "..2223..", "..3211..", "...2111.", "....111.",
        ".....11.", "........", "........", "........", "........", "........", "........", "........"]
add_sprite("MISSILE_D", rows(MISD))                       # diagonal (down-right); X-flip for down-left
MISS = ["........", "........", "........", "........", "3.3.....", "33322211", "33322211", "3.3.....",
        "........", "........", "........", "........", "........", "........", "........", "........"]
add_sprite("MISSILE_S", rows(MISS))                       # sideways (right); X-flip for left
add_sprite("ROCK_A", disc(16, 16, 7.5, 7.5, 6.6, 2)); add_sprite("ROCK_B", disc(16, 16, 7.5, 7.5, 6.2, 9, 0.9))
CHEV = ["........", "........", "........", "........", "1......1", "21....12", "221..122", ".221122.",
        "..2222..", "...22...", "1......1", "21....12", ".21..12.", "..2112..", "...22...", "........"]
add_sprite("CHEV", rows(CHEV))                            # "gap here" marker
SPK = ["........", "........", "........", "...1....", "...1....", ".1.2.1..", "..222...", "112.211.",
       "..222...", ".1.2.1..", "...1....", "...1....", "........", "........", "........", "........"]
add_sprite("GRAZE", rows(SPK))
a = np.zeros((16, 8), np.uint8); a[0:6, 3] = 3; a[6:12, 3] = 2; a[12:16, 3] = 1; add_sprite("STREAK", a)
a = np.zeros((16, 8), np.uint8); a[0:16, 2] = 2; a[2:14, 5] = 3; a[4:12, 2] = 1; add_sprite("HEAT", a)
# brig pod: a banded capsule with a barred window (cool colours: it is a pickup, not a hazard)
pod = np.zeros((16, 16), np.uint8)
for y in range(16):
    for x in range(16):
        dx, dy = (x - 7.5) / 5.6, (y - 7.5) / 7.4
        if dx * dx + dy * dy < 1: pod[y, x] = 3 if dx * dx + dy * dy > 0.72 else 2
pod[4:9, 5:11] = 1; pod[4:9, 6] = 3; pod[4:9, 9] = 3; pod[11, 4:12] = 3; pod[2, 6:10] = 3
add_sprite("POD", pod)
a = np.zeros((16, 8), np.uint8); a[6:10, 2:6] = 2; a[7:9, 1:7] = 2; a[7:9, 3:5] = 1; add_sprite("PULSE", a)
carray("er_spr", sum((enc(t) for t in spr), [])); define("ER_SPR_NT", len(spr))
OBJ = [['#000000', '#FFFFFF', '#FFE860', '#F08020'],     # 3 bullets / missiles: warm, bright
       ['#000000', '#FFE860', '#F08020', '#FFFFFF'],     # 4 markers
       ['#000000', '#3A2C28', '#8C7460', '#E0D0B0'],     # 5 rocks
       ['#000000', '#FFFFFF', '#78E8F8', '#2878D8']]     # 6 cool: graze sparks, streaks, pod, pulse
carray("er_objpal", sum((pal_words(p) for p in OBJ), []), "uint16_t")
carray("er_bgpal", pal_words(PAL_SPACE) + pal_words(PAL_ROCK), "uint16_t")
carray("er_laserpal", sum((pal_words(p) for p in PAL_LASER), []), "uint16_t")
carray("er_tagpal", pal_words(PAL_TAG), "uint16_t")
carray("er_atmo", sum((pal_words(p) for p in ATMO), []), "uint16_t")
carray("er_glow", sum((pal_words(p) for p in GLOW), []), "uint16_t"); define("ER_GLOW_N", len(GLOW))

# ============================================================ Graul portraits (48 x 48, 4 palettes) for the cutscenes
def portrait(name, path):
    img = Image.open(path)
    img = ImageEnhance.Brightness(img).enhance(1.35)
    img = gbconv.prep(img, (48, 48), contrast=1.35, sat=2.2)
    res = gbconv.convert(img, 3)
    # the amber eyes are too small to win a palette on their own, so the tiles that hold them get a fixed fourth palette
    EYE = [(250, 170, 40), (150, 160, 170), (70, 80, 92), (12, 14, 20)]
    res["pals"] = list(res["pals"]) + [EYE]
    rgb = np.asarray(img.convert("RGB"), int)
    ts = Tiles(); tmap, amap = [], []
    for ty in range(6):
        for tx in range(6):
            blk = rgb[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
            amber = (blk[..., 0] > 170) & (blk[..., 1] > 90) & (blk[..., 2] < 110) & (blk[..., 0] - blk[..., 2] > 90)
            if amber.sum() >= 2:
                dist = ((blk[:, :, None, :] - np.array(EYE)[None, None]) ** 2).sum(-1)
                px = dist.argmin(-1); px[amber] = 0
                res["pix"][ty, tx] = px; res["tpal"][ty, tx] = 3
    for ty in range(6):
        for tx in range(6):
            i, _ = ts.add(res["pix"][ty, tx]); tmap.append(i); amap.append(3 + int(res["tpal"][ty, tx]))
    carray(f"{name}_tiles", ts.data()); carray(f"{name}_pals", sum((pal_words(p) for p in res["pals"]), []), "uint16_t")
    carray(f"{name}_map", tmap); carray(f"{name}_attr", amap); define(f"{name.upper()}_NT", len(ts.t))
    gbconv.render(res, 4).save(f"{PREV}/{name}.png")
portrait("por_graul_a", f"{ROOT}/ref/escape/graul_a.png")
portrait("por_graul_b", f"{ROOT}/ref/escape/graul_b.png")

# ============================================================ cutscene scene: crash site (160 x 72, 3 palettes)
SKY = ['#140C30', '#48307C', '#A078C0', '#F8E8D0']
GND = ['#06040C', '#1C2840', '#3C5878', '#A0C8E0']
sc = np.zeros((72, 160), np.uint8)
r = random.Random(42)
for y in range(48):
    for x in range(160):
        band = y / 48 * 2.2 + (0.5 if (x + y) % 2 else 0)
        sc[y, x] = 0 if band < 1.1 else 1
for _ in range(34):
    x, y = r.randrange(160), r.randrange(22); sc[y, x] = r.choice([2, 3])
for y in range(48):                                      # a ringed moon
    for x in range(160):
        dd = math.hypot(x - 124, y - 16)
        if dd < 11: sc[y, x] = 3 if (x - 124) * -0.5 + (y - 16) * -0.6 > 1 - dd * 0.2 else 2
        elif abs((y - 16) - (x - 124) * 0.22) < 1.2 and 13 < abs(x - 124) < 22: sc[y, x] = 2
def ridge(x, k): return 44 + 5 * math.sin(x * 0.07 + k) + 3 * math.sin(x * 0.19 + k * 2)
for x in range(160):
    for y in range(int(ridge(x, 1)), 48): sc[y, x] = 0   # far ridge silhouette
for y in range(48, 72):
    for x in range(160):
        v = (y - 48) / 24 + r.uniform(-.12, .12)
        sc[y, x] = 1 if v < 0.35 else 2 if v < 0.8 or (x + y) % 3 == 0 else 1
        if r.random() < 0.02: sc[y, x] = 3
# the Sneaky Kryptid, nose down in a furrow, as a dark silhouette with lit ports and smoke
wreck = [(52, 58), (70, 36), (86, 30), (96, 34), (92, 46), (80, 60), (64, 64)]
wi = Image.new("L", (160, 72), 0); wd = ImageDraw.Draw(wi)
wd.polygon(wreck, fill=1); wd.polygon([(86, 30), (104, 20), (100, 36)], fill=1); wd.polygon([(70, 36), (60, 26), (78, 33)], fill=1)
wm = np.asarray(wi) > 0
for y in range(72):
    for x in range(160):
        if wm[y, x]: sc[y, x] = 0
for (x, y) in ((74, 44), (79, 40), (84, 37)): sc[y:y + 2, x:x + 2] = 3
for i in range(26):                                      # smoke trail drifting up-left
    cx, cy = 90 - i * 2.2 + r.uniform(-2, 2), 26 - i * 0.9 + r.uniform(-2, 2)
    for y in range(int(cy - 3), int(cy + 3)):
        for x in range(int(cx - 3), int(cx + 3)):
            if 0 <= y < 44 and 0 <= x < 160 and math.hypot(x - cx, y - cy) < 2.6 and (x + y + i) % 2 == 0 and not wm[y, x]: sc[y, x] = 2
for x in range(40, 112):                                 # furrow
    y = 62 + int(2 * math.sin(x * 0.2));
    if not wm[y, x]: sc[y, x] = 0
sts = Tiles(); smap, sattr = [], []
for ty in range(9):
    for tx in range(20):
        i, _ = sts.add(sc[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]); smap.append(i); sattr.append(0 if ty < 6 else 1)
carray("scene_crash_tiles", sts.data()); define("SCENE_CRASH_NT", len(sts.t))
carray("scene_crash_pals", pal_words(SKY) + pal_words(GND) + pal_words(GND), "uint16_t")
carray("scene_crash_map", smap); carray("scene_crash_attr", sattr)
render(sc, lambda tx, ty: SKY if ty < 6 else GND, f"{PREV}/scene_crash.png")
print("scene tiles:", len(sts.t))

# sprite sheet preview
sh = Image.new("RGB", (len(spr) // 2 * 9 + 1, 18), (40, 0, 40)); px = sh.load()
for i in range(len(spr) // 2):
    for half in range(2):
        t = spr[i * 2 + half]
        for y in range(8):
            for x in range(8):
                if t[y, x]: px[1 + i * 9 + x, 1 + half * 8 + y] = hx(OBJ[0][t[y, x]])
sh.resize((sh.width * 4, sh.height * 4), Image.NEAREST).save(f"{PREV}/er_sprites.png")

hdr = ["// generated by tools/gen_escape.py - do not edit", "#ifndef ESCAPE_DATA_H", "#define ESCAPE_DATA_H",
       "#include <gb/gb.h>", "#include <stdint.h>", "BANKREF_EXTERN(escape_art)"] + H + ["#endif"]
open(os.path.join(ROOT, "src", "escape_data.h"), "w").write("\n".join(hdr) + "\n")
open(os.path.join(ROOT, "src", "escape_art.c"), "w").write(
    "// generated by tools/gen_escape.py - do not edit\n#pragma bank 255\n#include <gb/gb.h>\n#include <stdint.h>\nBANKREF(escape_art)\n" + "\n".join(C) + "\n")
print("escape art bytes:", carray.size)
assert carray.size < 16000
