"""Stage 4 art: Frostbite Moon surface, its cryptids, and the Wendigo."""
import math, random
import numpy as np
import paint as PT
from paint import Canvas, shade_blob, ellipse_mask, stamp_mask, stars_canvas, mirror_canvas, BG_PALS

FROST = '#0A1020'
BG_PALS.update({
    'SNOW':   [FROST, '#3A4C70', '#7088B0', '#A8BCD8'],      # moon surface (mid-tone so bullets read)
    'SNOWD':  [FROST, '#1C2848', '#3A4C70', '#7088B0'],      # shaded snow / crater walls
    'DOME':   [FROST, '#262C36', '#7088B0', '#D8E0E8'],      # col 2 == snow so dome tiles blend in
    'DLIGHT': [FROST, '#14181E', '#48E0C8', '#F8B040'],
    'CREV':   [FROST, '#060A18', '#183058', '#4880C0'],      # crevasses + frozen lakes
    'FROSTSKY': [FROST, '#24305A', '#8090C8', '#F0F4FF'],
    'WEN0':   [FROST, '#101010', '#6A6A4A', '#A49C7B'],      # sheet: dark, olive hide, tan antlers
    'WEN1':   [FROST, '#101010', '#EEE6CD', '#41EEF6'],      # bone skull, cyan breath
    'WEN0HIT':[FROST, '#4A4A3A', '#C8C8A0', '#FFFFFF'],
    'WHITE4': ['#F0F4FF', '#F0F4FF', '#F0F4FF', '#FFFFFF'],
})
SNOW, SNOWD, DOME, DLIGHT, CREV, WEN0, WEN1 = 0, 1, 2, 3, 6, 6, 7


def snow_base(h, seed):
    """Full-width moon surface built from a few repeating 8x8 textures (cheap on tiles)."""
    cv = Canvas(160, h)
    rng = random.Random(seed)
    pats = [
        [(1, 2, 2), (5, 5, 2), (6, 1, 3)],
        [(2, 6, 2), (6, 3, 3)],
        [(3, 3, 1), (4, 3, 2), (2, 5, 3)],
        [],
        [(0, 0, 3), (7, 4, 2)],
    ]
    cv.pal[:] = SNOW; cv.col[:] = 2
    for ty in range(h // 8):
        for tx in range(20):
            for (x, y, c) in pats[rng.randrange(len(pats))]:
                cv.px(tx * 8 + x, ty * 8 + y, SNOW, c)
    return cv


def crater_stamp(r, seed):
    s = (int(r * 2) + 8) // 8 * 8
    cv = Canvas(s, s)
    yy, xx = np.mgrid[0:s, 0:s]
    c = s / 2
    rim = ((xx - c) ** 2 + (yy - c) ** 2 <= r * r)
    cv.pal[:] = SNOW; cv.col[:] = 2
    shade_blob(cv, rim, SNOW, light=(0.6, 0.8), seed=seed, levels=(0.2, 0.3), noise=0.02, rim=False)
    inner = (xx - c - 1) ** 2 + (yy - c - 1) ** 2 <= (r * 0.72) ** 2
    shade_blob(cv, inner, SNOWD, light=(0.6, 0.8), seed=seed + 1, levels=(0.1, 0.5), noise=0.02, rim=False)
    return cv


def crevasse(cv, y0, h, seed):
    rng = random.Random(seed)
    x = rng.randrange(40, 120)
    for y in range(y0, y0 + h):
        x += rng.choice([-1, 0, 0, 1])
        w = 2 + int(2 * math.sin(y * 0.15) + 2)
        for dx in range(-w, w + 1):
            c = 1 if abs(dx) < w - 1 else 2
            cv.px(x + dx, y, CREV, c)
        cv.px(x - w - 1, y, SNOW, 3)


def lake_stamp(w, h, seed):
    cv = Canvas(w, h)
    cv.pal[:] = SNOW; cv.col[:] = 2
    m = ellipse_mask(w, h, w / 2, h / 2, w / 2 - 2, h / 2 - 2)
    cv.mask(m, CREV, 2)
    cv.mask(ellipse_mask(w, h, w / 2, h / 2, w / 2 - 2, h / 2 - 2) & ~ellipse_mask(w, h, w / 2 + 1, h / 2 + 1, w / 2 - 3, h / 2 - 3), SNOW, 3)
    rng = random.Random(seed)
    for _ in range(4):            # cracks and glints in the ice
        x, y = rng.randrange(8, w - 8), rng.randrange(6, h - 6)
        for k in range(6):
            cv.px(x + k, y + (k % 3) - 1, CREV, 3)
    return cv


def dome_stamp(seed, lit=True):
    """Abandoned mining dome: metal hemisphere with a lit window band."""
    cv = Canvas(40, 32)
    cv.pal[:] = SNOW; cv.col[:] = 2
    m = ellipse_mask(40, 32, 20, 24, 17, 15) & (np.mgrid[0:32, 0:40][0] < 28)
    shade_blob(cv, m, DOME, light=(-0.5, -0.8), seed=seed, levels=(0.3, 0.45), noise=0.01)
    cv.rect(4, 26, 36, 29, DOME, 1)
    for x in range(8, 33, 6):
        cv.rect(x, 17, x + 3, 19, DOME, 3 if lit else 1)
    cv.rect(17, 21, 23, 28, DOME, 1)
    return cv


def ridge_stamp(w, seed):
    """Ice ridge running along a screen edge."""
    cv = Canvas(w, 48)
    cv.pal[:] = SNOW; cv.col[:] = 2
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:48, 0:w]
    edge = w * 0.55 + 6 * np.sin(yy * 0.2 + rng.uniform(0, 6)) + 4 * np.sin(yy * 0.51)
    m = xx < edge
    shade_blob(cv, m, SNOWD, light=(1.0, -0.3), seed=seed, levels=(0.3, 0.6), noise=0.02)
    return cv


def moon_canvas(h, seed, domes=0.3, lakes=0.3, cracks=0.3, ridges=0.6):
    cv = snow_base(h, seed)
    rng = random.Random(seed)
    if not hasattr(moon_canvas, 'st'):
        moon_canvas.st = dict(craters=[crater_stamp(14, 1), crater_stamp(10, 2), crater_stamp(20, 3)],
                              lakes=[lake_stamp(48, 32, 4)], domes=[dome_stamp(5), dome_stamp(6, False)],
                              ridges=[ridge_stamp(40, 7), ridge_stamp(32, 8)])
    S = moon_canvas.st
    y = 0
    while y < h - 48:
        if rng.random() < cracks:
            crevasse(cv, y, 40, rng.randrange(999))
        y += 48
    y = 0
    rng = random.Random(seed + 1)
    while y < h - 48:
        for side in (0, 1):
            if rng.random() < ridges:
                r = rng.choice(S['ridges'])
                if side: r = mirror_canvas(r)
                PT.stamp(cv, r, 0 if side == 0 else 160 - r.w, y)
        rr = rng.random()
        if rr < domes:
            PT.stamp(cv, rng.choice(S['domes']), rng.choice([40, 64, 80]), y + 8)
        elif rr < domes + lakes:
            PT.stamp(cv, S['lakes'][0], rng.choice([40, 56, 72]), y + 8)
        else:
            c = rng.choice(S['craters'])
            PT.stamp(cv, c, rng.choice([40, 56, 72, 88]) // 8 * 8, y + 8)
        y += 48
    return cv


# ---------------------------------------------------------------- Wendigo (112 x 64)
def wendigo(eye='open', mouth='shut'):
    W, H = 112, 64
    cv = Canvas(W, H)
    cx = 56
    yy, xx = np.mgrid[0:H, 0:W]
    # antlers: branching tines spreading wide
    def line(x0, y0, x1, y1, r, slot, c):
        for s in np.linspace(0, 1, 30):
            x, y = x0 + (x1 - x0) * s, y0 + (y1 - y0) * s
            cv.mask(ellipse_mask(W, H, x, y, r, r), slot, c)
    for side in (-1, 1):
        bx, by = cx + side * 6, 12
        tips = [(cx + side * 34, 2), (cx + side * 48, 10), (cx + side * 22, 0)]
        line(bx, by, cx + side * 30, 6, 1.6, WEN0, 3)
        line(cx + side * 30, 6, cx + side * 48, 10, 1.2, WEN0, 3)
        line(cx + side * 18, 9, tips[2][0], tips[2][1], 1.0, WEN0, 3)
        line(cx + side * 30, 6, tips[0][0], tips[0][1], 1.0, WEN0, 3)
        line(cx + side * 40, 8, cx + side * 44, 1, 0.9, WEN0, 3)
    # gaunt body + ribs
    body = ellipse_mask(W, H, cx, 40, 12, 18)
    shade_blob(cv, body, WEN0, light=(-0.4, -0.9), seed=2, levels=(0.3, 0.6), noise=0.01)
    for k in range(4):
        y = 34 + k * 5
        for x in range(cx - 9 + k, cx + 10 - k):
            if abs(x - cx) > 1: cv.px(x, y, WEN0, 3 if abs(x - cx) < 7 else 2)
    # shoulders + long arms hanging out to the sides (the claws are sprite arms)
    for side in (-1, 1):
        arm = np.zeros((H, W), bool)
        for s in np.linspace(0, 1, 40):
            x = cx + side * (10 + 26 * s); y = 30 + 10 * s + 8 * s * s
            arm |= ellipse_mask(W, H, x, y, 3.2 - s, 3.2 - s)
        shade_blob(cv, arm, WEN0, light=(-side * 0.3, -1.0), seed=3, levels=(0.3, 0.5), noise=0.01)
    # deer-skull head
    head = ellipse_mask(W, H, cx, 20, 8.5, 9) | ellipse_mask(W, H, cx, 29, 5, 5)
    shade_blob(cv, head, WEN0, light=(-0.3, -1.0), seed=4, levels=(0.1, 0.35), noise=0.01)
    for ex in (cx - 4, cx + 4):
        cv.mask(ellipse_mask(W, H, ex, 20, 2.6, 2.4), WEN0, 1)          # sockets
        if eye != 'closed':
            cv.mask(ellipse_mask(W, H, ex, 20, 1.3, 1.3), WEN1, 3)
            if eye == 'angry':
                sgn = 1 if ex > cx else -1
                for k in range(-3, 4):
                    cv.px(ex + k, 17 + (k * sgn) // 2, WEN0, 1)
    cv.px(cx - 1, 27, WEN0, 1); cv.px(cx + 1, 27, WEN0, 1)              # nasal holes
    if mouth == 'open':
        cv.mask(ellipse_mask(W, H, cx, 33, 4, 3.5), WEN1, 1)
        cv.mask(ellipse_mask(W, H, cx, 33, 2.6, 2.2), WEN1, 2)
        for x in range(cx - 3, cx + 4, 2): cv.px(x, 30, WEN0, 3); cv.px(x, 36, WEN0, 3)
    else:
        for x in range(cx - 3, cx + 4): cv.px(x, 33, WEN0, 1)
        for x in range(cx - 3, cx + 4, 2): cv.px(x, 32, WEN0, 3)
    return cv


# ---------------------------------------------------------------- sprites ('1' light, '2' mid, '3' dark)
def mirror(half):
    return [h + h[::-1] for h in half]

TURRET_A = mirror([       # mining turret seen from above: armoured dome + barrel
"........",
"........",
".......3",
"......31",
"..33332 ".replace(' ', '1'),
".3222231",
"32122223",
"32211222",
"32222222",
"32222222",
".3222222",
"..322222",
"...33333",
"........",
"........",
"........",
])
TURRET_B = [r.replace('31', '13', 1) if i in (2, 3) else r for i, r in enumerate(TURRET_A)]
BAT_A = mirror([          # frost bat: ice-blue wings
"........",
"3.......",
"33......",
"323....3",
"3223..33",
"32223322",
".3222312",
"..322222",
"...32223",
"....3.32",
"......3.",
"........",
"........",
"........",
"........",
"........",
])
BAT_B = mirror([
"........",
"........",
"........",
"........",
"3333...3",
"32223333",
"32222312",
".3322222",
"...32223",
"....3.32",
"......3.",
"........",
"........",
"........",
"........",
"........",
])
SNAKE_HEAD = mirror([
"........",
"....3333",
"...31111",
"..311222",
".3112222",
".3121322",
".3122222",
".3122222",
"..312222",
"...31223",
"....3233",
".....3..",
"........",
"........",
"........",
"........",
])
SNAKE_SEG = [
"..3333..",
".311113.",
"31122113",
"31222213",
"31222213",
"31122113",
".311113.",
"..3333..",
] + ["........"] * 8
CLAW_SEG = [
"..3333..",
".322213.",
"32222113",
"32222213",
"32222223",
"32222223",
".322223.",
"..3333..",
] + ["........"] * 8
CLAW_TIP = [
"3..3..3.",
"31.31.31",
"31.31.31",
"3223223.",
"3222223.",
".32223..",
"..333...",
"........",
] + ["........"] * 8
# Beast of Bray Road (32x32): hunched wolf-man, glowing eyes.  1 eyes/fangs, 2 fur, 3 dark
BEAST = mirror([
"................",
"..........3.....",
".........323....",
"........3223....",
"........32223333",
".......322222222",
"......3222222222",
"......3222211222",
"......3222211222",
".....32222222222",
"..33.32222233122",
".3223322222231 1".replace(' ', '3'),
"322223322222233 ".replace(' ', '3'),
"3222222322222222",
"3222222232222222",
".322222223222222",
"..32222222322222",
"..32213222222222",
"...313.322222222",
"...3.3..32222222",
"........32222222",
".........3222223",
".........322223.",
"........32223...",
"........3223....",
"........323.....",
".........3......",
"................",
"................",
"................",
"................",
"................",
])
