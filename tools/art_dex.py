"""Shooter sprites redrawn from the KRYPTID-DEX sheets (ref/dex). Small enemies are
hand-drawn 16x16 interpretations of the sheet silhouettes (top-down camera);
mid-bosses use the 32x32 sheet frames directly; chain heads are native crops.
Index: 1 light/accent, 2 body, 3 outline/dark."""
import numpy as np
import dex as D

def chk(name, rows, w=16, h=16):
    assert len(rows) == h, (name, len(rows))
    for r in rows: assert len(r) == w, (name, r, len(r))
    return rows

def arr(rows): return np.array([[0 if c == '.' else int(c) for c in r] for r in rows], np.uint8)

# ---------------------------------------------------------------- Stage 1: krakenlings (mini Kraken: mantle, eyes, beak, two tentacles)
KL_A = chk('KL_A', [
"......3333......",
".....322223.....",
"....32222223....",
"...3322222233...",
"..323222222323..",
"..3.32222223.3..",
"....32222223....",
"...3211221123...",
"...3211221123...",
"...3222112223...",
"....32222223....",
".....322223.....",
"....32.33.23....",
"....32....23....",
".....32..23.....",
"......3..3......"])
KL_B = chk('KL_B', KL_A[:11] + [
".....322223.....",
"...322.33.223...",
"..32........23..",
"..3..........3..",
"................"])
# kraken spawn asleep (buddy), eyes shut, tentacles curled; Z above
BUDZ0 = chk('BUDZ0', [
"...........111..",
"............1...",
"...........111..",
"......3333......",
".....322223.....",
"....32222223....",
"...3322222233...",
"...3222222223...",
"...3233223323...",
"...3222112223...",
"....32222223....",
"...3322222233...",
"..32.322223.23..",
"..3223.33.3223..",
"...33......33...",
"................"])

# ---------------------------------------------------------------- Stage 2: Impundulu (black/white lightning bird, diving), Lindworm
# palette: 1 cyan bolt/eyes, 2 white, 3 black
IMPU_A = chk('IMPU_A', [
"3..............3",
"32............23",
"332..........233",
"3332........2333",
".3332......2333.",
".33332.33.23333.",
"..333332233333..",
"...3333223333...",
".....322223.....",
".....322223.....",
"......3223......",
"......3333......",
"......3113......",
".......33.......",
"................",
"................"])
IMPU_B = chk('IMPU_B', [   # attack frame: wings swept, bolt from the beak
"................",
"................",
"3..............3",
"32............23",
"3332........2333",
".33332.33.23333.",
"..333332233333..",
"....33322333....",
".....322223.....",
".....322223.....",
"......3223......",
"......3113......",
".......33.......",
"......11........",
".......11.......",
"......11........"])
BUDZ1 = chk('BUDZ1', [   # impundulu chick asleep (buddy palette: 1 white, 2 cyan, 3 blue)
"...........111..",
"............1...",
"...........111..",
"................",
"................",
"......3333......",
".....322223.....",
"....32222223....",
"...3332222333...",
"..333322223333..",
"..333333223333..",
"..3332223222333.",
"...33222222233..",
"....33333333....",
".....3....3.....",
"................"])

# ---------------------------------------------------------------- Stage 3: Pukwudgie (small body, grey face, spear), Will-o'-the-wisp, Devil's egg
PUK_A = chk('PUK_A', [
"...........1....",
"..........313...",
"....3333..313...",
"...311113.33....",
"..31311313.3....",
"..31111113.3....",
"...311113..3....",
"....3333...3....",
"...322223..3....",
"..32222223.3....",
"..32322323133...",
"..3.3223.3313...",
"....3223...3....",
"....3..3...3....",
"...33..33..3....",
"................"])
PUK_B = chk('PUK_B', [   # throwing: spear forward
"................",
"....3333........",
"...311113.......",
"..31311313......",
"..31111113......",
"...311113.......",
"....3333........",
"...322223.......",
"..3222222333333.",
"..32322323111131",
"..3.3223.333333.",
"....3223........",
"....3223........",
"....3..3........",
"...33..33.......",
"................"])
BUDZ2 = chk('BUDZ2', [
"...........111..",
"............1...",
"...........111..",
"................",
"................",
"................",
"................",
"...3333.........",
"..311113........",
"..33113333333...",
"..311113222223..",
"...333322222223.",
"....3222222223..",
"..33333333333333",
".31111111111113.",
"................"])
WISP_FACE = [(6, 6), (9, 6), (7, 9), (8, 9)]   # eyes + mouth on the orb

def wisp(lit):
    a = D.frame('will_o_wisp', 'idle0' if lit else 'idle1')
    a = D.place(D.crop(a, D.bbox(a)), 16, 16)
    # remap: glow ring -> 1 (lit) / 3 (dim); core -> 2
    out = a.copy()
    out[a == 1] = 1 if lit else 3
    out[a == 2] = 2 if lit else 3
    out[a == 3] = 3
    if not lit:
        out[(a == 1) & (np.indices(a.shape).sum(0) % 2 == 0)] = 0
    for (x, y) in WISP_FACE:
        out[y, x] = 3 if lit else 2
    return D.rows(out)

EGG_A = chk('EGG_A', [   # Jersey Devil egg: leathery, cracked, glowing seam
"................",
"......3333......",
".....322223.....",
"....32222223....",
"...3222222223...",
"...3221222223...",
"..322212222223..",
"..322221222223..",
"..322212222223..",
"..322221122223..",
"..322222212223..",
"...3222221223...",
"...3222222223...",
"....32222223....",
".....333333.....",
"................"])
EGG_B = [r.replace('1', '3') for r in EGG_A]
EGG_B[6] = "..322212222223.."
EGG_B = chk('EGG_B', [r if i not in (5, 7, 9) else EGG_A[i] for i, r in enumerate(EGG_B)])

# ---------------------------------------------------------------- Stage 4: frost bat (Ahool wing shape, ice colours), Tatzelwurm
FBAT_A = chk('FBAT_A', [   # wings up: long finger bones and scalloped membrane like the Ahool
"3..............3",
"33.....33.....33",
"323...3223...323",
"3223..3113..3223",
"32223.3223.32223",
"3222233223322223",
"3232223223222323",
"3.32322222232 3."[:16],
"...3.322223.3...",
"......3223......",
"......3..3......",
"................",
"................",
"................",
"................",
"................"])
FBAT_A[7] = "3.323222222323.3"
FBAT_B = chk('FBAT_B', [   # down-stroke
"................",
"................",
"................",
".......33.......",
"......3223......",
"......3113......",
"..333332233333..",
".32222322322223.",
"3222223223222223",
"3232322222232323",
"3.3.33222233.3.3",
"......3223......",
"......3..3......",
"................",
"................",
"................"])
TATZ_SEG = chk('TATZ_SEG', [
"...33...",
"..3223..",
".322223.",
"32211223",
"32222223",
"32322323",
".322223.",
"..3333..",
] + ["........"] * 8, 8, 16)
LIND_SEG = chk('LIND_SEG', [
"..3333..",
".322223.",
"32322323",
"32222223",
"32232223",
"32222223",
".322223.",
"..3333..",
] + ["........"] * 8, 8, 16)
BUDZ3 = chk('BUDZ3', [   # tatzelwurm pup asleep: curled, cat ears, Z
"...........111..",
"............1...",
"...........111..",
"................",
"................",
"................",
"...3.3..........",
"..32323.........",
"..322223333.....",
"..3333222222 3.."[:16],
".3222232223223..",
".322222222 223.."[:16],
"..3322222222 3.."[:16],
"....3333333333..",
"................",
"................"])
BUDZ3[9] = "..33332222223..."
BUDZ3[11] = ".32222222222223."
BUDZ3[12] = "..332222222233.."
BUDZ3[10] = ".3222232223223.."

def head_crop(name, frame, box):
    a = D.frame(name, frame)
    return D.crop(a, box)


def tatz_head():
    """Cat face of the Tatzelwurm (sheet crop), 16x16, facing right; flipped in code."""
    a = D.frame('tatzelwurm', 'idle0')
    return D.rows(D.place(D.crop(a, (13, 15, 26, 25)), 16, 16, 'r', 'c'))


def lind_head():
    a = D.frame('lindworm', 'idle0')
    return D.rows(D.place(D.crop(a, (17, 5, 31, 17)), 16, 16, 'r', 'c'))


def seg(name, box, w=8, h=16):
    a = D.frame(name, 'idle0')
    return D.rows(D.place(D.crop(a, box), w, h))


def sleep_small(name, cmap=None):
    a = D.frame(name, 'sleep', cmap)
    a = D.half(D.crop(a, D.bbox(a)))
    a = D.crop(a, D.bbox(a))
    return D.rows(D.place(a, 16, 16, 'c', 'b'))


def sheet32(name, frame, cmap=None, flip=False):
    a = D.frame(name, frame, cmap)
    a = D.place(D.crop(a, D.bbox(a)), 32, 32, 'c', 'b')
    if flip: a = D.flipx(a)
    return D.rows(a)


# ---------------------------------------------------------------- big bosses (BG canvases, 64 px tall)
import paint as PT
from paint import Canvas


def frame_canvas(name, frame, slotmap, scale=2, box=None, W=None, H=64, ax='c', ay='b', cmap=None, flip=False):
    """Sheet frame -> Canvas. slotmap: sheet colour index -> (slot, col). Scaled with EPX."""
    a = D.frame(name, frame, cmap)
    a = D.crop(a, box if box else D.bbox(a))
    if flip: a = D.flipx(a)
    for _ in range({1: 0, 2: 1, 4: 2}[scale]): a = D.scale2(a)
    w = W or ((a.shape[1] + 7) // 8 * 8)
    a = D.place(a, w, H, ax, ay)
    cv = Canvas(w, H)
    for k, (slot, col) in slotmap.items():
        m = a == k
        cv.pal[m] = slot; cv.col[m] = col
    return cv


def sheet_boss(name, slotmap, variants, head_box, W, scale=2, ax='c'):
    """Build boss canvases from sheet frames. slotmap: sheet RGB -> (slot, col).
    variants: list of (vname, frame, mods) - the head_box region is taken from `frame`
    over the idle0 body; mods: dict RGB -> RGB recolours inside the head box (eyes etc.)."""
    cs = D.colors(name)
    cm = {c: i + 1 for i, c in enumerate(cs)}
    base = D.frame(name, 'idle0', cm)
    bb = D.bbox(base)
    out = []
    x0, y0, x1, y1 = head_box
    for vname, fr, mods in variants:
        a = base.copy()
        h = D.frame(name, fr, cm)
        a[y0:y1, x0:x1] = h[y0:y1, x0:x1]
        for src, dst in (mods or {}).items():
            reg = a[y0:y1, x0:x1]
            reg[reg == cm[src]] = cm[dst] if dst in cm else 99
        a = D.crop(a, bb)
        for _ in range({1: 0, 2: 1}[scale]): a = D.scale2(a)
        a = D.place(a, W, 64, ax, 'b')
        cv = Canvas(W, 64)
        for rgb, k in cm.items():
            slot, col = slotmap[rgb]
            cv.pal[a == k] = slot; cv.col[a == k] = col
        if 99 in a:                                  # recoloured to a colour outside the sheet: slot 7 col 3
            cv.pal[a == 99] = 7; cv.col[a == 99] = 3
        out.append((vname, cv))
    return out


def jersey_devil_boss():
    m = {(24, 16, 24): (6, 1), (213, 197, 172): (6, 2), (65, 238, 246): (6, 3)}
    return sheet_boss('jersey_devil', m, [
        ('open', 'idle0', None),
        ('angry', 'atk0', {(65, 238, 246): 'RED'}),
        ('closed', 'idle0', {(65, 238, 246): (213, 197, 172)}),
        ('howl', 'atk1', {(65, 238, 246): 'RED'})], (14, 2, 30, 17), 64)


def wendigo_boss():
    m = {(16, 16, 16): (6, 1), (106, 106, 74): (6, 2), (164, 156, 123): (6, 3),
         (238, 230, 205): (7, 2), (65, 238, 246): (7, 3)}
    return sheet_boss('wendigo', m, [
        ('open', 'idle0', None),
        ('angry', 'atk0', None),
        ('closed', 'idle0', {(238, 230, 205): (164, 156, 123)}),
        ('howl', 'atk1', None)], (6, 0, 26, 16), 48)
