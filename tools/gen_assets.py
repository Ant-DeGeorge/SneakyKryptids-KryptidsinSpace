#!/usr/bin/env python3
"""Asset compiler for Kryptids in Space: The Sneaky Kryptid (GBC, 512 KB).

Bank map
   2  common sprites, font/HUD tiles, BG palette library
   3  crew portraits
   6  title art, logo, cutscene scenes
  12  stage 1 data      4, 5   stage 1 level rows
  13  stage 2 data     15, 16  stage 2 level rows
  14  stage 3 data     17, 18  stage 3 level rows
  19  boss cutscene scenes
"""
import os, sys
from PIL import Image, ImageEnhance
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gbart import parse, tiles_from, encode, TileSet, FONT, font_tile
import sprites as S
import paint as PT
from paint import hx, BG_PALS
import gbconv
from crops import CROPS
from emit import Out, pal_words
from stage_build import StageBuilder, PalLib, norm, COMM_TILE, BOSS_BASE
import stage1, stage2, stage3, stage4, art_s2, art_s3, art_s4, art_dex

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SRC = os.path.join(ROOT, "src")
REF = os.path.join(ROOT, "ref")
PREV = os.path.join(ROOT, "build", "preview")
os.makedirs(PREV, exist_ok=True)

o = Out(SRC)
lib = PalLib()

# ======================================================================= OBJ palettes (fixed ones)
OBJ_PALS = [
    ['#000000', '#48E0C8', '#5A6470', '#12161C'],   # 0 ship
    ['#000000', '#F8A040', '#5A6470', '#12161C'],   # 1 engine
    ['#000000', '#F8F8F8', '#48E0C8', '#2870C8'],   # 2 player shots + buddies
    ['#000000', '#F8F8F8', '#F848A8', '#401040'],   # 3 enemy bullets (per stage)
    ['#000000', '#F890C0', '#B83888', '#300820'],   # 4-6 per stage
    ['#000000', '#F0F8FF', '#70C8F0', '#183868'],
    ['#000000', '#B8F050', '#3C9030', '#102008'],
    ['#000000', '#F8F8F8', '#F8D038', '#E86018'],   # 7 fx / pickups
]
o.carray(2, "obj_pals", sum((pal_words(p) for p in OBJ_PALS), []), "uint16_t")
o.carray(2, "white_pal", pal_words(['#F8F8F8'] * 4), "uint16_t")

# ======================================================================= common sprites (OBJ VRAM bank 0)
spr = []
def add_sprite(name, rows, w, h=16):
    img = parse(norm(rows, w)[:h], w)
    img += [[0] * w for _ in range(h - len(img))]
    first = len(spr)
    for band in range(h // 16):
        for col in range(w // 8):
            spr.append([row[col * 8:col * 8 + 8] for row in img[band * 16:band * 16 + 8]])
            spr.append([row[col * 8:col * 8 + 8] for row in img[band * 16 + 8:band * 16 + 16]])
    o.define(f"ST_{name}", first)

add_sprite("SHIP_TOP", S.SHIP_TOP, 16)
add_sprite("SHIP_BOT", S.SHIP_BOT, 16)
add_sprite("SHIP_BOTB", S.SHIP_BOT_B, 16)
add_sprite("SHOT_TWIN", S.SHOT_TWIN, 8)
add_sprite("SHOT_SPREAD", S.SHOT_SPREAD, 8)
add_sprite("SHOT_WISP", S.SHOT_WISP, 8)
add_sprite("EB_ORB", S.EB_ORB, 8)
add_sprite("EB_INK", S.EB_INK, 8)
for i, e in enumerate(S.EXPL):
    add_sprite(f"EXPL{i}", e, 16)
for k, name in [('T', 'PK_T'), ('S', 'PK_S'), ('H', 'PK_H'), ('B', 'PK_B'), ('h', 'PK_HEART'), ('1', 'PK_1UP')]:
    add_sprite(name, S.PICKUPS[k], 16)
add_sprite("ARROW", ["..3.....", "..33....", "..313...", "..3113..", "..313...", "..33....", "..3.....", "........"] + ["........"] * 8, 8)
add_sprite("WARN", S.TENT_WARN, 8)
ZZZ = [".3333...", "...3....", "..3.....", ".3333...", ".....333", "......3.", ".....333", "........",
       "........", "........", "........", "........", "........", "........", "........", "........"]
add_sprite("ZZZ", ZZZ, 8)
# buddies: friendly-coloured versions of each stage's critter (OBJ palette 2)
# awake wingmen: kraken spawn, impundulu chick, pukwudgie, tatzelwurm pup
add_sprite("BUD0_A", art_dex.KL_A, 16);   add_sprite("BUD0_B", art_dex.KL_B, 16)
add_sprite("BUD1_A", art_dex.IMPU_A, 16); add_sprite("BUD1_B", art_dex.IMPU_B[:13] + ['.' * 16] * 3, 16)
add_sprite("BUD2_A", art_dex.PUK_A, 16);  add_sprite("BUD2_B", art_dex.PUK_A, 16)
add_sprite("BUD3_A", art_dex.tatz_head(), 16); add_sprite("BUD3_B", art_dex.tatz_head(), 16)
# asleep (the dex sleep pose, with the Z)
for k, z in enumerate([art_dex.BUDZ0, art_dex.BUDZ1, art_dex.BUDZ2, art_dex.BUDZ3]):
    add_sprite(f"BUDZ{k}", z, 16)
add_sprite("SPARK", ["........", "1..1..1.", ".1.1.1..", "..111...", "1111111.", "..111...", ".1.1.1..", "1..1..1."] + ["........"] * 8, 8)
add_sprite("RUNE", ["..333...", ".31113..", "3121213.", "3112113.", "3121213.", ".31113..", "..333...", "........"] + ["........"] * 8, 8)
assert len(spr) <= 128, len(spr)
o.carray(2, "spr_tiles", sum((encode(t) for t in spr), []))
o.define("N_SPR_TILES", len(spr))
print("common sprite tiles:", len(spr))

# ======================================================================= common BG tiles
FONT["'"] = [".##..", ".##..", "..#..", ".....", ".....", ".....", "....."]
FONT[","] = [".....", ".....", ".....", ".....", ".##..", "..#..", ".#..."]
FONT[":"] = [".....", ".##..", ".##..", ".....", ".##..", ".##..", "....."]
FONT["/"] = ["....#", "...#.", "...#.", "..#..", ".#...", ".#...", "#...."]
FONT["&"] = [".##..", "#..#.", "#.#..", ".#...", "#.#.#", "#..#.", ".##.#"]
FONT["+"] = [".....", "..#..", "..#..", "#####", "..#..", "..#..", "....."]
FONT["("] = ["...#.", "..#..", ".#...", ".#...", ".#...", "..#..", "...#."]
FONT[")"] = [".#...", "..#..", "...#.", "...#.", "...#.", "..#..", ".#..."]
FONT["$"] = ["..#..", ".####", "#.#..", ".###.", "..#.#", "####.", "..#.."]
CHARS = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!?.-x',:/&+()$"
common = []
def font_panel(ch):
    if ch == ' ':
        return [[1] * 8 for _ in range(8)]
    t = font_tile(ch)
    return [[3 if v == 3 else (0 if v == 1 else 1) for v in row] for row in t]
for ch in CHARS:
    common.append(font_panel(ch))
o.define("T_FONT", 0)
asc = [0] * 128
for i, ch in enumerate(CHARS):
    asc[ord(ch)] = i
for c in "abcdefghijklmnopqrstuvwyz":
    asc[ord(c)] = CHARS.index(c.upper())
o.carray(2, "ascii_map", asc)
o.define("T_SPACE", len(common))
common.append([[0] * 8 for _ in range(8)])
HUD = {
'heart':  ["........", ".22.22..", "2332222.", "2322222.", "2222222.", ".22222..", "..222...", "...2...."],
'heartE': ["........", ".00.00..", "0110110.", "0111110.", "0111110.", ".01110..", "..010...", "...0...."],
'bomb':   ["....3.3.", "...3.3..", "..222...", ".23322..", ".232222.", ".222222.", "..2222..", "........"],
'ship':   ["..3..3..", ".33..33.", ".333333.", "33322333", "33333333", ".3....3.", ".2....2.", "........"],
'pip':    ["........", "........", "..333...", ".33333..", ".33333..", "..333...", "........", "........"],
'pipE':   ["........", "........", "..000...", ".0...0..", ".0...0..", "..000...", "........", "........"],
'star':   ["...3....", "...3....", ".33333..", "..333...", ".3...3..", "........", "........", "........"],
'starE':  ["...0....", "...0....", ".00000..", "..000...", ".0...0..", "........", "........", "........"],
'bar':    ["11111111", "00000000", "22222222", "22222222", "22222222", "00000000", "11111111", "11111111"],
'barE':   ["11111111", "00000000", "00000000", "00000000", "00000000", "00000000", "11111111", "11111111"],
'frameT': ["00000000", "33333333", "11111111", "11111111", "11111111", "11111111", "11111111", "11111111"],
'frameB': ["11111111", "11111111", "11111111", "11111111", "11111111", "11111111", "33333333", "00000000"],
'lock':   ["........", "..000...", ".0...0..", ".0...0..", "0000000.", "0003000.", "0000000.", "........"],
'bud':    ["........", "..3333..", ".322223.", ".32..23.", ".322223.", "..3..3..", ".3....3.", "........"],
}
for k, rows in HUD.items():
    t = [[{'.': 1, '0': 0, '1': 1, '2': 2, '3': 3}[c] for c in r] for r in rows]
    o.define(f"T_HUD_{k.upper()}", len(common))
    common.append(t)
N_COMMON = len(common)
o.define("N_COMMON", N_COMMON)
o.carray(2, "common_tiles", sum((encode(t) for t in common), []))
print("common BG tiles:", N_COMMON)

# ======================================================================= stages
for nm in ('TEXT', 'HUD', 'SPACE'):
    lib.add(nm)
STAGES = [
    (stage1, 12, [4, 5], 'MUS_STAGE', 'MUS_BOSS', 9),
    (stage2, 13, [15, 16], 'MUS_STAGE2', 'MUS_TBIRD', 10),
    (stage3, 14, [17, 18], 'MUS_STAGE3', 'MUS_JDEVIL', 11),
    (stage4, 20, [21, 22], 'MUS_STAGE4', 'MUS_WENDIGO', 23),
]
infos = []
for k, (mod, dbank, rbanks, ms, mb, logic) in enumerate(STAGES):
    sb = StageBuilder(k + 1, o, lib, N_COMMON, dbank, rbanks, PREV)
    hit = mod.build(sb)
    infos.append((mod, sb.emit(ms, mb, logic, hit)))
o.define("COMM_TILE", COMM_TILE)
o.define("BOSS_BASE", BOSS_BASE)

# BG palette library
o.carray(2, "bg_pal_lib", sum((pal_words(c) for c in lib.cols), []), "uint16_t")
for i, n in enumerate(lib.names):
    o.define(f"BP_{n}", i)
print("palette library:", len(lib.names))

# stage table (ROM0)
L = ['// generated by tools/gen_assets.py', '#include <stdint.h>', '#include "assets.h"', '#include "songs.h"',
     '#include "stage_def.h"', 'const StageDef stage_defs[N_STAGES] = {']
for mod, i in infos:
    n = i['n']
    ch = i['chunks'] + [(0, None)] * (3 - len(i['chunks']))
    L.append("  {" + ", ".join([
        str(i['db']), f"s{n}_scen0", str(i['n_scen0']), f"s{n}_scen1", str(i['n_scen1']),
        f"s{n}_spr", str(i['n_spr']), f"s{n}_objpal", f"s{n}_initbp", str(len(i['chunks'])),
        "{" + ",".join(str(b) for b, _ in ch) + "}",
        "{" + ",".join(p if p else "0" for _, p in ch) + "}",
        str(i['n_rows']), "{" + ",".join(str(c) for c in i['cps']) + "}",
        f"s{n}_btiles", str(i['n_btiles']), f"s{n}_bswap", str(i['n_swap']), f"s{n}_bvar",
        str(i['hit_bp']), str(i['boss_bp6']), i['mus'][0], i['mus'][1], str(i['logic'])]) + "},")
L.append("};")
L.append("const char * const stage_names[N_STAGES] = {" + ",".join(f'"{m.NAME}"' for m, _ in infos) + "};")
open(os.path.join(SRC, "stage_defs.c"), "w").write("\n".join(L) + "\n")

# ======================================================================= portraits (bank 3)
def conv_img(img, npal, name, pal_base, bank=3, uniq=False):
    res = gbconv.convert(img, npal)
    tw, th = res["tw"], res["th"]
    ts = TileSet(0)
    tmap, amap = [], []
    for ty in range(th):
        for tx in range(tw):
            t = res["pix"][ty, tx].tolist()
            if uniq:
                ts.tiles.append(t); idx, fl = len(ts.tiles) - 1, 0
            else:
                idx, fl = ts.add(t)
            tmap.append(idx); amap.append((pal_base + int(res["tpal"][ty, tx])) | fl)
    o.carray(bank, f"{name}_tiles", sum((encode(t) for t in ts.tiles), []))
    o.carray(bank, f"{name}_pals", sum((pal_words(p) for p in res["pals"]), []), "uint16_t")
    o.carray(bank, f"{name}_map", tmap)
    o.carray(bank, f"{name}_attr", amap)
    o.define(f"{name.upper()}_NT", len(ts.tiles))
    gbconv.render(res, 2).save(f"{PREV}/{name}.png")
    return len(ts.tiles)

CREW = ['cap', 'static', 'forge', 'doc', 'vex']
for who in CREW:
    for ex in ('a', 'b'):
        f, (x, y, s) = CROPS[f'{who}_{ex}']
        img = gbconv.prep(Image.open(f"{REF}/{f}").crop((x, y, x + s, y + s)), (48, 48))
        if who == 'vex':
            img = ImageEnhance.Brightness(img).enhance(1.25)
        conv_img(img, 4, f"por_{who}_{ex}", 3)
    f, (x, y, s) = CROPS[f'{who}_a']
    m = s * 0.12
    img = gbconv.prep(Image.open(f"{REF}/{f}").crop((x + m, y + m * 0.6, x + s - m, y + s - m * 1.4)), (32, 32), contrast=1.25, sat=1.3)
    if who == 'vex':
        img = ImageEnhance.Brightness(img).enhance(1.3)
    conv_img(img, 1, f"mini_{who}", 4, uniq=True)

# ======================================================================= title, logo, scenes
ship_ref = Image.open(f"{REF}/IMG_5070.png")
conv_img(gbconv.prep(ship_ref.crop((60, 180, 2400, 1350)), (160, 80), contrast=1.2, sat=1.35), 6, "title_art", 0, bank=6)
conv_img(gbconv.prep(ship_ref.crop((0, 60, 2528, 1460)), (160, 72), contrast=1.2, sat=1.35), 3, "scene_ship", 0, bank=6)
conv_img(gbconv.prep(ship_ref.crop((120, 120, 760, 410)), (160, 72), contrast=1.3, sat=1.4), 3, "scene_station", 0, bank=6)

def boss_scene(cv, slots, space_pal, name, bank):
    img = Image.new('RGB', (160, 72), hx(BG_PALS[space_pal][0]))
    st = PT.stars_canvas(160, 72, 99, 0.01)
    p = img.load()
    for yy in range(72):
        for xx in range(160):
            p[xx, yy] = hx(BG_PALS[space_pal][int(st.col[yy, xx])])
    x0 = (160 - cv.w) // 2
    for yy in range(64):
        for xx in range(cv.w):
            c = int(cv.col[yy, xx])
            if c:
                p[x0 + xx, 4 + yy] = hx(BG_PALS[slots[int(cv.pal[yy, xx])]][c])
    conv_img(img, 3, name, 0, bank=bank)

boss_scene(PT.kraken('angry', 'open'), {0: 'SPACE', 6: 'K0', 7: 'K1'}, 'SPACE', "scene_kraken", 6)
boss_scene(art_s2.thunderbird('angry', 'open', True), {0: 'SPACE2', 6: 'TB0', 7: 'TB1'}, 'SPACE2', "scene_tbird", 19)
boss_scene(art_dex.jersey_devil_boss()[1][1], {0: 'SPACE3', 6: 'JD0', 7: 'JD1'}, 'SPACE3', "scene_jdevil", 19)
# storm + barrens vistas for briefings
sv = art_s2.storm_canvas(72, 777, density=1.0, lit_frac=0.4)
img = Image.new('RGB', (160, 72)); p = img.load()
SL2 = {0: 'SPACE2', 1: 'NEB2', 2: 'CLOUD', 3: 'BEACON', 6: 'ZAP', 7: 'PYLON'}
for yy in range(72):
    for xx in range(160):
        p[xx, yy] = hx(BG_PALS[SL2[int(sv.pal[yy, xx])]][int(sv.col[yy, xx])])
conv_img(img, 3, "scene_storm", 0, bank=19)
bv = art_s3.barrens_canvas(72 + 48, 778, cabins=True)
img = Image.new('RGB', (160, 72)); p = img.load()
SL3 = {0: 'SPACE3', 1: 'FOG', 2: 'ROCK', 3: 'PINE', 6: 'CABIN', 7: 'WISPBG'}
for yy in range(72):
    for xx in range(160):
        p[xx, yy] = hx(BG_PALS[SL3[int(bv.pal[yy + 8, xx])]][int(bv.col[yy + 8, xx])])
conv_img(img, 3, "scene_barrens", 0, bank=19)
boss_scene(art_dex.wendigo_boss()[3][1], {0: 'FROSTSKY', 6: 'WEN0', 7: 'WEN1'}, 'FROSTSKY', "scene_wendigo", 24)
fv = art_s4.moon_canvas(72 + 96, 779, domes=0.6, lakes=0.3, cracks=0.4)
img = Image.new('RGB', (160, 72)); p = img.load()
SL4 = {0: 'SNOW', 1: 'SNOWD', 2: 'DOME', 3: 'DLIGHT', 6: 'CREV', 7: 'FROSTSKY'}
sky4 = PT.stars_canvas(160, 24, 780, 0.02)
for yy in range(72):
    for xx in range(160):
        if yy < 24:
            p[xx, yy] = hx(BG_PALS['FROSTSKY'][int(sky4.col[yy, xx])])
        else:
            p[xx, yy] = hx(BG_PALS[SL4[int(fv.pal[yy + 40, xx])]][int(fv.col[yy + 40, xx])])
conv_img(img, 3, "scene_frost", 0, bank=24)

def big_text(txt):
    w = len(txt) * 11 + 2
    img = [[0] * (w + (8 - w % 8) % 8) for _ in range(16)]
    for i, ch in enumerate(txt):
        if ch == ' ': continue
        for gy, row in enumerate(FONT[ch]):
            for gx, c in enumerate(row):
                if c == '#':
                    for dy in range(2):
                        for dx in range(2):
                            img[1 + gy * 2 + dy][1 + i * 11 + gx * 2 + dx] = 3
    H, W = len(img), len(img[0])
    out = [row[:] for row in img]
    for y in range(H):
        for x in range(W):
            if img[y][x] == 0 and any(0 <= y + dy < H and 0 <= x + dx < W and img[y + dy][x + dx] == 3
                                      for dy in (-1, 0, 1) for dx in (-1, 0, 1)):
                out[y][x] = 1
    for y in range(9, H):
        for x in range(W):
            if out[y][x] == 3: out[y][x] = 2
    return out
logo = big_text("SNEAKY KRYPTID")
lw = len(logo[0]) // 8
lts = TileSet(0); lmap = []
for t in tiles_from(logo, lw, 2):
    idx, fl = lts.add(t, False); lmap.append(idx)
o.carray(6, "logo_tiles", sum((encode(t) for t in lts.tiles), []))
o.carray(6, "logo_map", lmap)
o.define("LOGO_W", lw); o.define("LOGO_NT", len(lts.tiles))
o.carray(6, "logo_pal", pal_words(['#060818', '#12161C', '#2E9C8C', '#48E0C8']), "uint16_t")

o.raw_h('#define BANK_A2 2')
o.raw_h('#define BANK_A3 3')
sizes = o.write()
print("bank sizes:", dict(sorted(sizes.items())))
