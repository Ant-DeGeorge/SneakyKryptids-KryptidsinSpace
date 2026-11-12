#!/usr/bin/env python3
"""Art for mission 7 "The Tree Line" (Graul on foot). Needs Pillow + numpy only.
Writes src/ground_data.h and src/ground_art.c (autobanked, BANK(ground_art)).

VRAM use in the mission
  BG  bank0 ids N_COMMON..   field: forest floor, canopy, boulder, the wreck
  BG  bank1 ids 0..          the watcher's head (9C00 map rows 8-14), 64.. HUD bar, 128.. its eyes
  OBJ bank1                  Graul, chain-mace, critters, troopers, bruiser, claw, bolts
BG palettes: 0 floor, 1 canopy, 2 steel (boulder, wreck), 3 watcher eyes, 4 comm, 5 HUD, 6 watcher head, 7 status line
Colour rules: hazards warm and bright, Graul and the ship's guns cool; no red-vs-green cues.
"""
import os, sys, math, random
import numpy as np
from PIL import Image, ImageDraw
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gbconv
from emit import pal_words
import gen_eyes

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
PREV = os.path.join(ROOT, "build", "preview")
os.makedirs(PREV, exist_ok=True)
hx = lambda s: tuple(int(s[i:i + 2], 16) for i in (1, 3, 5))
H, C = [], []
SIZE = [0]


def carray(name, data, typ="uint8_t"):
    data = [int(v) for v in data]
    H.append(f"extern const {typ} {name}[];  // {len(data)}")
    body = ",".join(f"0x{v:04X}" for v in data) if typ == "uint16_t" else ",".join(map(str, data))
    C.append(f"const {typ} {name}[] = {{{body}}};")
    SIZE[0] += len(data) * (2 if typ == "uint16_t" else 1)
def define(n, v): H.append(f"#define {n} {v}")
enc = gbconv.tile_bytes
Z = lambda: np.zeros((8, 8), np.uint8)


class Tiles:
    def __init__(self): self.t, self.k = [], {}
    def add(self, a, flip=False):
        a = np.asarray(a, np.uint8); key = a.tobytes()
        if key in self.k: return self.k[key], 0
        if flip and a[:, ::-1].tobytes() in self.k: return self.k[a[:, ::-1].tobytes()], 0x20
        self.k[key] = len(self.t); self.t.append(a)
        return self.k[key], 0
    def data(self): return sum((enc(t) for t in self.t), [])


def rows(lst, w=16):
    for r in lst: assert len(r) == w, (r, len(r))
    return np.array([[0 if c == '.' else int(c) for c in r] for r in lst], np.uint8)

FLOOR = '#0C1818'
PAL_FLOOR = [FLOOR, '#183030', '#2C4C40', '#5C8068']
PAL_TREE = [FLOOR, '#0A2420', '#16443A', '#2C6C50']
PAL_STEEL = [FLOOR, '#2A3444', '#5C6880', '#B4C0D4']
N_FLOOR = ['#060C10', '#0C1C20', '#183030', '#345448']      # night, for the watcher fight
N_TREE = ['#060C10', '#06161A', '#0C2C2C', '#1A4840']
N_STEEL = ['#060C10', '#1C2434', '#44506C', '#98A8C4']
PAL_TAG = ['#0C0E18', '#2C3448', '#F08020', '#FFE860']

# ============================================================ field tiles
ft = []
def ftile(name, a):
    define(f"GR_T_{name}", f"(N_COMMON + {len(ft)})"); ft.append(np.asarray(a, np.uint8))
r = random.Random(77)
ftile("BLANK", Z())
a = Z(); a[5, 2] = 2; a[4, 3] = 2; a[5, 4] = 1; a[6, 3] = 1; ftile("GRASS0", a)
a = Z(); a[2, 5] = 2; a[1, 6] = 3; a[3, 5] = 1; a[6, 1] = 1; ftile("GRASS1", a)
a = Z(); a[3, 2:4] = 1; a[4, 1:5] = 1; a[4, 2] = 2; a[6, 6] = 1; ftile("GRASS2", a)
def canopy(seed, edge=False):
    rr = random.Random(seed); a = Z()
    for y in range(8):
        for x in range(8):
            v = math.sin((x + seed) * 1.3) + math.sin((y + seed * 2) * 1.1) + rr.uniform(-.6, .6)
            a[y, x] = 3 if v > 1.1 else 2 if v > -0.2 else 1
            if edge:
                lim = 3 + 2.2 * math.sin((y + seed) * 0.9)
                if x > lim + 1: a[y, x] = 0
                elif x > lim: a[y, x] = 1
    return a
for i in range(4): ftile(f"TREE{i}" if i else "TREE", canopy(5 + i * 3))
ftile("EDGE", canopy(21, True)); ftile("EDGE1", canopy(34, True))      # canopy on the left, floor on the right; X-flip for the other side
bo = np.zeros((16, 16), np.uint8)
for y in range(16):
    for x in range(16):
        ang = math.atan2(y - 7.5, x - 7.5); rad = 7.2 + 0.5 * math.sin(ang * 3 + 1) + 0.3 * math.sin(ang * 5)
        d = math.hypot(x - 7.5, y - 7.5)
        if d < rad:
            lit = (x - 7.5) * -0.6 + (y - 7.5) * -0.7
            bo[y, x] = 1 if d > rad - 1.2 or lit < -3 else 3 if lit > 2.5 else 2
bo[9:11, 5:9] = 1
define("GR_T_BOULDER", f"(N_COMMON + {len(ft)})")
for q in ((0, 0), (0, 8), (8, 0), (8, 8)): ft.append(bo[q[0]:q[0] + 8, q[1]:q[1] + 8])
define("GR_FIELD_FIXED", len(ft))

# the wreck: the Sneaky Kryptid seen from above, nose dug in at the top (80 x 40, 10 x 5 tiles)
SS = 4; WW, WH = 80, 40
im = Image.new("L", (WW * SS, WH * SS), 0); d = ImageDraw.Draw(im)
P = lambda pts: [(x * SS, y * SS) for x, y in pts]
hull = [(40, 1), (47, 6), (50, 16), (64, 22), (77, 30), (76, 36), (54, 34), (48, 38), (32, 38), (26, 34), (4, 36), (3, 30), (16, 22), (30, 16), (33, 6)]
d.polygon(P(hull), fill=2, outline=1); d.line(P(hull + hull[:1]), fill=1, width=SS)
d.polygon(P([(40, 4), (45, 9), (44, 20), (36, 20), (35, 9)]), fill=3, outline=1)          # canopy glass
d.line(P([(40, 20), (40, 37)]), fill=1, width=SS)
for sx in (-1, 1):
    d.line(P([(40 + sx * 9, 18), (40 + sx * 30, 31)]), fill=1, width=SS)
    d.rectangle([(40 + sx * 20 - 3) * SS, 27 * SS, (40 + sx * 20 + 3) * SS, 31 * SS], fill=3, outline=1)   # gun pods
    d.rectangle([(40 + sx * 6 - 2) * SS, 33 * SS, (40 + sx * 6 + 2) * SS, 38 * SS], fill=1)                  # engines
arr = np.asarray(im).reshape(WH, SS, WW, SS).transpose(0, 2, 1, 3).reshape(WH, WW, SS * SS)
wk = np.zeros((WH, WW), np.uint8)
for y in range(WH):
    for x in range(WW):
        v = np.bincount(arr[y, x], minlength=4)
        if v[0] > SS * SS * 0.5: continue
        v[0] = 0; wk[y, x] = int(v.argmax())
for _ in range(30):                                          # scorch marks
    x, y = r.randrange(6, 74), r.randrange(8, 36)
    if wk[y, x] == 2: wk[y, x] = 1
wts = Tiles(); wmap = []
for ty in range(5):
    for tx in range(10):
        i, _ = wts.add(wk[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]); wmap.append(i)
define("GR_T_WRECK", f"(N_COMMON + {len(ft)})"); define("GR_WRECK_W", 10); define("GR_WRECK_H", 5)
ft += wts.t
carray("gr_field_tiles", sum((enc(t) for t in ft), [])); define("GR_FIELD_NT", len(ft))
carray("gr_wreck_map", wmap)
carray("gr_bgpal", pal_words(PAL_FLOOR) + pal_words(PAL_TREE) + pal_words(PAL_STEEL), "uint16_t")
carray("gr_bgpal_night", pal_words(N_FLOOR) + pal_words(N_TREE) + pal_words(N_STEEL), "uint16_t")
carray("gr_tagpal", pal_words(PAL_TAG), "uint16_t")

# ============================================================ the watcher's head (160 x 56): a silhouette above the tree line
BW, BH = 160, 56
hd = np.zeros((BH, BW), np.uint8)
for y in range(BH):
    for x in range(BW // 2):
        dx = 79.5 - x
        top = 10 + (dx / 62.0) ** 2 * 30 - 4 * math.sin(dx * 0.35) * (1 if dx > 20 else 0)       # shaggy dome
        if y >= top and dx < 66 + 3 * math.sin(y * 0.7): hd[y, x] = 1
        # antler / ear spike
        if 34 < dx < 58 and y >= 2 + abs(dx - 46) * 1.6 and y < 22: hd[y, x] = 1
        if hd[y, x] and (math.sin(x * 0.9 + y * 0.5) + math.sin(y * 1.3 - x * 0.2)) > 1.45 and y < 44: hd[y, x] = 2      # fur glints
# muzzle and fangs along the bottom
for x in range(52, 80):
    for y in range(44, 56):
        if hd[y, x] and y >= 46 and (x - 52) % 7 < 4 and y < 46 + 4 + ((x - 52) % 7 < 2) * 4 + (4 - abs((x - 52) % 7 - 1.5)) : hd[y, x] = 3
hd[40:43, 70:76] = 2
hd[:, 80:] = hd[:, 79::-1]
bts = Tiles(); bmap, battr = [], []
for ty in range(7):
    for tx in range(20):
        i, fl = bts.add(hd[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8], flip=True)
        bmap.append(i); battr.append(6 | 0x08 | fl)
assert len(bts.t) <= 64, len(bts.t)
carray("gr_head_tiles", bts.data()); define("GR_HEAD_NT", len(bts.t))
carray("gr_head_map", bmap); carray("gr_head_attr", battr)
print("head tiles:", len(bts.t))
# eyes: the same eyes as the ending (tools/gen_eyes.py), 5 shapes x open/half/shut x 6x3 tiles, left eye
ed = []
for s in gen_eyes.SHAPES:
    for f in range(3):
        for t in gen_eyes.tiles_of(gen_eyes.eye(s, f)): ed += enc(np.asarray(t, np.uint8))
carray("gr_eye_tiles", ed); define("GR_EYE_BASE", 128); define("GR_EYE_FT", 18); define("GR_EYE_SHAPE_NT", 54)
# HUD bar cells (0..8 px filled) + wrench icon + eye icon
hud = []
for f in range(9):
    a = Z(); a[1, :] = 1; a[6, :] = 1; a[2:6, :f] = 3; hud.append(a)
a = Z(); a[1:3, 1:3] = 3; a[1, 2] = 0; a[3, 3] = 3; a[4, 4] = 3; a[5:7, 5:7] = 3; a[6, 5] = 0; hud.append(a)
a = Z(); a[3:5, 1:7] = 3; a[2, 2:6] = 3; a[5, 2:6] = 3; a[3:5, 3:5] = 1; hud.append(a)
wolf = rows(["3......3", "33....33", "3333.333", "33333333", ".313313.", ".333333.", "..3113..", "...33..."], 8)
wolf = np.where(wolf == 1, 0, wolf).astype(np.uint8)       # eyes as holes; the HUD palette's colour 3 for the head
hud.append(wolf)                                         # Graul's head: the lives icon on the Hunter HUD
carray("gr_hud_tiles", sum((enc(t) for t in hud), [])); define("GR_HUD_BASE", 64); define("GR_HUD_NT", len(hud))

# ============================================================ sprites (OBJ bank 1), 8x16 units, 16-px frames = 2 columns
spr = []
def add_sprite(name, a):
    a = np.asarray(a, np.uint8); h, w = a.shape
    define(f"GR_ST_{name}", len(spr))
    for row in range(h // 16):
        for col in range(w // 8):
            spr.append(a[row * 16:row * 16 + 8, col * 8:col * 8 + 8]); spr.append(a[row * 16 + 8:row * 16 + 16, col * 8:col * 8 + 8])
# Graul: 1 fur, 2 armour, 3 teal glow
G_DOWN = rows(["....1......1....", "...111....111...", "...1111111111...", "..111111111111..", "..113111111311..", "..111112211111..", "...1111221111...", ".22211111111222.",
               "2222211111122222", "2232222222222322", ".22223222232222.", "..122222222221..", "..112222222211..", "...1112222111...", "...111....111...", "...22......22..."])
G_UP = rows(["....1......1....", "...111....111...", "...1111111111...", "..111111111111..", "..111111111111..", "..111111111111..", "...1111111111...", ".22211111111222.",
             "2222221111222222", "2232222222222322", ".22222322322222.", "..122222222221..", "..112221122211..", "...1112112111...", "...111.11.111...", "...22...1..22..."])
G_SIDE = rows(["......1...1.....", ".....111.111....", "....111111111...", "...11111111311..", "...1111111111221", "...11111111122..", "....111111111...", "...2221111122...",
               "..222222112222..", "..2232222222322.", "1.2222322222221.", "11122222222221..", ".1112222222211..", "...11122221111..", "...111....111...", "...22......22..."])
for nm, g in (("G_DOWN", G_DOWN), ("G_UP", G_UP), ("G_SIDE", G_SIDE)):
    add_sprite(nm, g)
    b = g.copy(); b[14] = np.roll(g[14], 1); b[15] = np.roll(g[15], -1); b[13, 3:6] = 0
    add_sprite(nm + "2", b)
ball = np.zeros((16, 8), np.uint8)
for y in range(8):
    for x in range(8):
        dd = math.hypot(x - 3.5, y - 3.5)
        if dd < 3.3: ball[y + 4, x] = 2 if (x - 3.5) + (y - 3.5) < -1 else 1
for (x, y) in ((0, 7), (7, 8), (3, 3), (4, 12), (0, 4), (7, 11), (7, 4), (0, 11)): ball[y, x] = 3      # spikes
add_sprite("BALL", ball)
a = np.zeros((16, 8), np.uint8); a[6:10, 2:6] = 2; a[7:9, 3:5] = 0; a[6, 2] = 0; a[9, 5] = 0; add_sprite("LINK", a)
a = np.zeros((16, 8), np.uint8); a[4:12, 3:5] = 3; a[6:10, 2:6] = 3; a[7:9, 3:5] = 1; add_sprite("BOLT", a)   # the ship's turret shot (cool)
a = np.zeros((16, 8), np.uint8); a[7, 1:7] = 3; a[8, 2:6] = 2; a[6, 3:5] = 2; add_sprite("HOWL", a)
# critters: 1 dark rust, 2 orange, 3 yellow
CRAWL = rows(["................", "..1..........1..", "...1...11...1...", "1...1.1221.1...1", ".1..11222211..1.", "..1.12233221.1..", "...1223223221...", "11112232232211.1",
              "...1222222221.1.", "..1.12222221.1..", ".1..11222211..1.", "1...1.1111.1...1", "...1...11...1...", "..1..........1..", "................", "................"])
c2 = CRAWL.copy(); c2[1] = 0; c2[13] = 0; c2[2, 2] = 1; c2[2, 13] = 1; c2[12, 2] = 1; c2[12, 13] = 1
add_sprite("CRAWL", CRAWL); add_sprite("CRAWL2", c2)
SPIT = rows(["................", "......1111......", "....11222211....", "...1222332221...", "..122331133221..", "..123311113321..", ".12231111113221.", ".12231111113221.",
             ".12233111133221.", "..122333333221..", "..112222222211..", "...1112222111...", "..1..111111..1..", ".1....1..1....1.", "1.....1..1.....1", "................"])
add_sprite("SPIT", SPIT)
CHARGE = rows(["................", ".33..........33.", ".133........331.", "..1331111113311.", "..113222222311..", "..122222222221..", ".12223222232221.", ".12222222222221.",
               ".12222211222221.", ".12222133122221.", "..122211112221..", "..112222222211..", "...1122222211...", "...11.1111.11...", "..11........11..", "................"])
add_sprite("CHARGE", CHARGE)
# troopers: 1 steel, 2 light, 3 amber visor
TROOP = rows(["................", "......1111......", ".....122221.....", "....12222221....", "....13333331....", "....12222221....", "..111122221111..", ".12221111112221.",
              ".12212222221221.", ".12212222221221.", "..1112222221331.", "....12211221.3..", "....1221.1221...", "....1221.1221...", "....111..111....", "...1111..1111..."])
t2 = TROOP.copy(); t2[14] = np.roll(TROOP[14], 1); t2[15] = np.roll(TROOP[15], -1)
add_sprite("TROOP", TROOP); add_sprite("TROOP2", t2)
br = np.kron(TROOP, np.ones((2, 2), np.uint8))               # the bruiser: a trooper twice the size, with shoulder spikes
br[12:16, 2:6] = 3; br[12:16, 26:30] = 3
add_sprite("BRUISER", br)
claw = np.zeros((32, 16), np.uint8)
for y in range(32):
    for x in range(16):
        w_ = 3.2 + y * 0.12
        if abs(x - 7.5) < w_ and y < 24: claw[y, x] = 1 if abs(x - 7.5) > w_ - 1.3 else 2
for k, cx in enumerate((3, 7.5, 12)):
    for y in range(22, 32):
        ww = (32 - y) * 0.22
        for x in range(16):
            if abs(x - cx) < ww: claw[y, x] = 3
add_sprite("CLAW", claw)
carray("gr_spr", sum((enc(t) for t in spr), [])); define("GR_SPR_NT", len(spr))
assert len(spr) <= 128, len(spr)
OBJ = [['#000000', '#FFFFFF', '#FFE860', '#F08020'],         # 3 shots: warm, bright
       ['#000000', '#7A3410', '#F08020', '#FFE860'],         # 4 critters, the claw
       ['#000000', '#4A5468', '#B4C0D4', '#FFB020'],         # 5 troopers
       ['#000000', '#6C7890', '#C0D0F0', '#48D8E8']]         # 6 Graul, his mace, the ship's guns: cool
carray("gr_objpal", sum((pal_words(p) for p in OBJ), []), "uint16_t")

# previews
def render(img, pal, path, scale=4):
    h, w = img.shape; im = Image.new("RGB", (w, h)); px = im.load()
    for y in range(h):
        for x in range(w): px[x, y] = hx(pal[img[y, x]])
    im.resize((w * scale, h * scale), Image.NEAREST).save(path)
render(wk, PAL_STEEL, f"{PREV}/gr_wreck.png"); render(hd, ['#04080C', '#28183C', '#40285C', '#C8C8BE'], f"{PREV}/gr_head.png", 3)
sh = Image.new("RGB", (len(spr) // 2 * 9 + 1, 18), (40, 0, 40)); px = sh.load()
for i in range(len(spr) // 2):
    for half in range(2):
        t = spr[i * 2 + half]
        for y in range(8):
            for x in range(8):
                if t[y, x]: px[1 + i * 9 + x, 1 + half * 8 + y] = hx(OBJ[3 if i < 28 else 1][t[y, x]])
sh.resize((sh.width * 3, sh.height * 3), Image.NEAREST).save(f"{PREV}/gr_sprites.png")

hdr = ["// generated by tools/gen_ground.py - do not edit", "#ifndef GROUND_DATA_H", "#define GROUND_DATA_H",
       "#include <gb/gb.h>", "#include <stdint.h>", "BANKREF_EXTERN(ground_art)"] + H + ["#endif"]
open(os.path.join(ROOT, "src", "ground_data.h"), "w").write("\n".join(hdr) + "\n")
open(os.path.join(ROOT, "src", "ground_art.c"), "w").write(
    "// generated by tools/gen_ground.py - do not edit\n#pragma bank 255\n#include <gb/gb.h>\n#include <stdint.h>\nBANKREF(ground_art)\n" + "\n".join(C) + "\n")
print("ground art bytes:", SIZE[0], "sprite tiles:", len(spr), "field tiles:", len(ft))
assert SIZE[0] < 16000
