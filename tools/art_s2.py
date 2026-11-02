"""Stage 2 art: Stormwall Nebula scenery, sprites and the Thunderbird."""
import math, random
import numpy as np
import paint as PT
from paint import Canvas, shade_blob, ellipse_mask, stamp_mask, stars_canvas, BG_PALS

STORM_SPACE = '#0A0820'
BG_PALS.update({
    'SPACE2': [STORM_SPACE, '#262058', '#7880C8', '#F0F4FF'],
    'NEB2':   [STORM_SPACE, '#1C1240', '#382670', '#5C46A8'],
    'CLOUD':  [STORM_SPACE, '#1E2848', '#44588C', '#90A4D4'],
    'CLOUDF': [STORM_SPACE, '#44588C', '#A8B8F0', '#FFFFFF'],      # lightning flash
    'ZAP':    [STORM_SPACE, '#2C2058', '#7058B0', '#C8B4F0'],
    'ZAPF':   [STORM_SPACE, '#7058B0', '#F8E848', '#FFFFFF'],
    'PYLON':  [STORM_SPACE, '#262C36', '#5A6672', '#A8B0BC'],
    'BEACON': [STORM_SPACE, '#14181E', '#48E0C8', '#F84858'],
    'TB0':    [STORM_SPACE, '#1C1008', '#6A4426', '#D04030'],     # sheet: dark, brown, red crest
    'TB1':    [STORM_SPACE, '#1C1008', '#41EEF6', '#F0C84A'],     # lightning accent cyan, yellow beak
    'TB0HIT': [STORM_SPACE, '#6A4020', '#C89060', '#FF9080'],
})
CLOUD, NEB, PYL, BCN, ZAP, TB0, TB1 = 2, 1, 7, 3, 6, 6, 7


def cloud_stamp(w, h, seed, lit=False):
    """Puffy storm cloud bank: overlapping balls, each ball lit on its own top."""
    cv = Canvas(w, h)
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:h, 0:w]
    balls = []
    for _ in range(12):
        cx, cy = rng.uniform(0.12, 0.88) * w, rng.uniform(0.3, 0.7) * h
        r = rng.uniform(0.14, 0.26) * w
        balls.append((cy, cx, r))
    balls.sort()                       # far (top) balls first, near balls overlap them
    slot = ZAP if lit else CLOUD
    for (cy, cx, r) in balls:
        m = (xx - cx) ** 2 + ((yy - cy) * 1.15) ** 2 <= r * r
        shade_blob(cv, m, slot, light=(-0.35, -1.0), seed=int(cx * 7 + cy), levels=(0.55, 0.62), noise=0.015)
    return cv


def soften(cv):
    """Knock a nebula stamp down one shade so it reads as gas, not rock."""
    m = cv.col > 0
    cv.col[m] = np.maximum(cv.col[m] - 1, 0)
    return cv


def pylon_stamp(h, seed, left=True):
    """Stormwall relay pylon: lattice tower with a beacon on top, 24 px wide."""
    cv = Canvas(24, h)
    for y in range(8, h):
        cv.px(4, y, PYL, 2); cv.px(19, y, PYL, 2)
        cv.px(5, y, PYL, 3 if left else 1); cv.px(18, y, PYL, 1 if left else 3)
        k = (y - 8) % 12
        cv.px(5 + k * 13 // 11, y, PYL, 1); cv.px(18 - k * 13 // 11, y, PYL, 1)
        if k == 0:
            cv.rect(4, y, 20, y + 1, PYL, 3)
    # dish + beacon
    cv.rect(2, 4, 22, 8, PYL, 2); cv.rect(2, 4, 22, 5, PYL, 3); cv.rect(2, 7, 22, 8, PYL, 1)
    cv.rect(10, 0, 14, 4, BCN, 3); cv.rect(11, 1, 13, 3, BCN, 2)
    return cv


def storm_canvas(h, seed, density=1.0, lit_frac=0.25, pylons=False):
    cv = stars_canvas(160, h, seed, 0.008)
    rng = random.Random(seed)
    if not hasattr(storm_canvas, 'stamps'):
        storm_canvas.stamps = [cloud_stamp(56, 40, 3), cloud_stamp(48, 32, 4), cloud_stamp(64, 40, 5),
                               cloud_stamp(56, 40, 6, lit=True), cloud_stamp(40, 32, 7, lit=True)]
        storm_canvas.nebs = [soften(PT.nebula_stamp(64, 48, 21)), soften(PT.nebula_stamp(48, 40, 22))]
    st = storm_canvas.stamps
    y = 0
    while y < h - 32:
        if rng.random() < 0.35:
            stamp_mask(cv, rng.choice(storm_canvas.nebs), rng.choice([40, 56, 72]), y)
        for side in (0, 1):
            if rng.random() < 0.85 * density:
                lit = rng.random() < lit_frac
                s = rng.choice(st[3:] if lit else st[:3])
                x = rng.choice([-24, -16, -8]) if side == 0 else 160 - s.w + rng.choice([8, 16, 24])
                stamp_mask(cv, s, x, y + rng.choice([0, 8]))
        if pylons and rng.random() < 0.6:
            side = rng.choice((0, 1))
            p = pylon_stamp(48, rng.randrange(99))
            stamp_mask(cv, p, 8 if side == 0 else 128, y)
        y += 40
    # nebula palette recolour: stamps were made with slot 1 = NEB2 already
    return cv


# ---------------------------------------------------------------- Thunderbird (128 x 64)
def thunderbird(eye='open', beak='shut', charged=False):
    W, H = 128, 64
    cv = Canvas(W, H)
    yy, xx = np.mgrid[0:H, 0:W]
    cx = 64
    d = np.abs(xx - cx + 0.5)
    t = np.clip((d - 9) / 54.0, 0, 1)
    # wing envelope
    top = 24 - 18 * np.sin(t * math.pi / 2) ** 0.9
    scallop = 4.5 * np.abs(np.sin((d - 9) * math.pi / 7.0))
    bot = 50 - 30 * t ** 0.85 + scallop * (0.4 + 0.6 * t)
    wings = (d >= 7) & (d <= 63) & (yy >= top) & (yy <= bot)
    shade_blob(cv, wings, TB0, light=(0.0, -1.0), seed=2, levels=(0.25, 0.35), noise=0.01, rim=True)
    # primary feather lines: from scallop tips back toward the shoulder
    for side in (-1, 1):
        for k in range(1, 9):
            dd = 9 + k * 7
            tt = min(1, (dd - 9) / 54.0)
            y1 = 50 - 30 * tt ** 0.85 + 4.5 * (0.4 + 0.6 * tt) * 0.15
            for s in np.linspace(0, 1, 30):
                x = cx + side * (dd - s * 10)
                y = y1 - s * 14
                if 0 <= int(x) < W and 0 <= int(y) < H and wings[int(y), int(x)]:
                    cv.px(int(x), int(y), TB0, 1)
    # covert band (lighter row of feathers along the wing top)
    band = wings & (yy < top + 6) & (d < 56)
    cv.col[band & (cv.col == 2)] = 3
    # body
    body = ellipse_mask(W, H, cx, 36, 12.5, 18)
    shade_blob(cv, body, TB0, light=(-0.4, -0.9), seed=4, levels=(0.4, 0.45), noise=0.01)
    for (fx, fy) in [(60, 40), (68, 40), (64, 45), (58, 47), (70, 47), (64, 51)]:   # chest feather chevrons
        for k in range(-2, 3):
            cv.px(fx + k, fy + abs(k) // 2, TB0, 1)
    # head + crest
    head = ellipse_mask(W, H, cx, 21, 10.5, 9.5)
    for k, (hx_, hy_) in enumerate([(56, 8), (64, 5), (72, 8)]):
        for s in np.linspace(0, 1, 14):
            x = hx_ + (cx - hx_) * s * 0.6; y = hy_ + (15 - hy_) * s
            r = 1 + s * 2.2
            head |= ellipse_mask(W, H, x, y, r, r)
    shade_blob(cv, head, TB0, light=(-0.3, -1.0), seed=5, levels=(0.3, 0.5), noise=0.01)
    cv.col[(cv.pal == TB0) & (cv.col == 3)] = 2
    # red crest plumes (sheet crest) and red wattles under the eyes
    crest = head & (yy < 14)
    cv.col[crest & (cv.col == 2)] = 3
    for wx in (cx - 9, cx + 9):
        cv.mask(ellipse_mask(W, H, wx, 25, 2.6, 3.2) & head, TB0, 3)
    # lightning accent: cyan zig-zag face patch around each eye
    for side in (-1, 1):
        ex = cx + side * 5
        for k, (dx, dy) in enumerate([(-4, -3), (-2, -4), (0, -3), (2, -4), (4, -3), (5, -1), (4, 1), (2, 3), (0, 4), (-2, 3), (-4, 2), (-5, 0)]):
            cv.px(ex + dx * side, 21 + dy, TB1, 2)
    # eyes
    for ex in (cx - 5, cx + 5):
        e = ellipse_mask(W, H, ex, 21, 3.2, 2.6)
        cv.mask(e, TB1, 1)
        if eye == 'closed':
            for x in range(ex - 3, ex + 4):
                cv.px(x, 21, TB1, 1); cv.px(x, 20, TB1, 1)
            cv.mask(ellipse_mask(W, H, ex, 20, 2.6, 0.6), TB0, 2)
        else:
            cv.mask(ellipse_mask(W, H, ex, 21, 2.2, 1.7), TB1, 3)
            cv.px(ex, 21, TB1, 1); cv.px(ex, 22, TB1, 1)
            cv.px(ex - 1, 20, TB1, 2)
        if eye == 'angry':
            sgn = 1 if ex > cx else -1
            for k in range(-4, 5):
                yb = 17 + (k * sgn) // 3
                cv.px(ex + k, yb, TB0, 1); cv.px(ex + k, yb - 1, TB0, 1)
    # beak (hooked, pointing down at the player)
    bk = np.zeros((H, W), bool)
    for y in range(24, 37):
        half = max(0, 4.5 - (y - 24) * 0.33)
        bk[y, int(cx - half):int(cx + half + 1)] = True
    if beak == 'open':
        bk |= ellipse_mask(W, H, cx, 31, 5.5, 6.5)
    cv.mask(bk, TB1, 2)
    edge = bk & ~np.roll(bk, 1, 1) | bk & ~np.roll(bk, -1, 1)
    cv.mask(edge, TB1, 1)
    if beak == 'open':
        cv.mask(ellipse_mask(W, H, cx, 31, 3.5, 4.5), TB1, 1)
        cv.mask(ellipse_mask(W, H, cx, 31, 2.0, 2.5), TB1, 3)
    else:
        for y in range(26, 36):
            cv.px(cx, y, TB1, 1)
    # talons
    for tx_ in (cx - 7, cx + 7):
        for k in (-2, 0, 2):
            for y in range(52, 57):
                cv.px(tx_ + k + (y - 52) * (1 if k > 0 else (-1 if k < 0 else 0)) // 2, y, TB1, 2)
            cv.px(tx_ + k, 57, TB1, 1)
    # lightning at the wing tips (and across the wings when charged)
    rng = random.Random(7 if charged else 3)
    paths = [(-60, 8), (60, 8)] + ([(-40, 16), (40, 16), (-22, 26), (22, 26)] if charged else [])
    for (dx, y0) in paths:
        x, y = cx + dx, y0
        for _ in range(10 if charged else 6):
            nx = x + rng.choice([-3, -2, 2, 3]); ny = y + rng.choice([2, 3])
            for s in np.linspace(0, 1, 6):
                px_, py_ = int(x + (nx - x) * s), int(y + (ny - y) * s)
                if 0 <= px_ < W and 0 <= py_ < H and cv.col[py_, px_]:
                    cv.px(px_, py_, TB1, 2)
            x, y = nx, ny
    return cv


# ---------------------------------------------------------------- sprites (16x16, '1' light '2' mid '3' dark)
def mirror(half):
    return [h + h[::-1] for h in half]

# Mothling: dusty wings with red eye-spots, fuzzy body, red eyes.  1 red, 2 dust, 3 dark
MOTH_A = mirror([
"........",
".333....",
"32223...",
"322223..",
"3212223.",
"3211223.",
"32122233",
".3222313",
"..322332",
".322233.",
"32223.3.",
"3223..3.",
".33.....",
"........",
"........",
"........",
])
MOTH_B = mirror([
"........",
"........",
"........",
"..33....",
".32233..",
"3212223.",
"3211222 ".replace(' ', '3'),
".3222313",
"..322332",
"..3223 3".replace(' ', '.'),
".3223.3.",
".323..3.",
"..3.....",
"........",
"........",
"........",
])
EEL_HEAD = mirror([   # storm eel head (16x16), facing down
"........",
"....3333",
"...31111",
"..311222",
".3112222",
".3122222",
".3213222",
".3211222",
".3122222",
"..312222",
"..312223",
"...31223",
"....3233",
".....3..",
"........",
"........",
])
EEL_SEG = [         # body segment (8x8 in an 8x16 tile)
"..3333..",
".311223.",
"31112223",
"31122223",
"32122223",
"32222233",
".322233.",
"..3333..",
] + ["........"] * 8
CLOUD_A = [
"................",
".....3333.......",
"...33111133.....",
"..3111111113333.",
".311111111111113",
"3111111111111113",
"3122111111122213",
"3222222222222223",
".33333333333333.",
"................",
"................",
"................",
"................",
"................",
"................",
"................",
]
CLOUD_B = CLOUD_A[:7] + [
"3222222122222223",
".33333311333333.",
".......13.......",
"......13........",
".......31.......",
"........3.......",
"................",
"................",
"................",
]
BOLT = [   # 8x16 lightning column segment (full height so columns join)
"...11...",
"...121..",
"....121.",
"....121.",
"...121..",
"..121...",
"..121...",
"...121..",
"....121.",
"....121.",
"...121..",
"..121...",
"..121...",
"...121..",
"...11...",
"...1....",
]
WARN_COL = [
"...33...",
"..3113..",
".312213.",
"31222213",
".312213.",
"..3113..",
"...33...",
"........",
] + ["........"] * 8
# Mothman (32x32): dark body, big wings, two huge red eyes (colour 1 of his palette = red)
# Mothman (32x32): grey-brown wings, dark body, two huge glowing red eyes.  1 red, 2 grey-brown, 3 dark
MOTHMAN = mirror([
"................",
"..33............",
".3223...........",
"322223..........",
"3222223.....3333",
"32222223...32222",
"322222223.322222",
"3222222223321112",
"3222222223211112",
"3222222223221112",
"3222222222322222",
"3222222222332222",
"32222222223.3222",
".3222222223.3222",
".3222222233.3222",
".322222233..3222",
"..3222233...3222",
"..322233....3222",
"...3233.....3222",
"...323......3222",
"....3.......3222",
".............322",
".............322",
"............3223",
"............322.",
"............33..",
"................",
"................",
"................",
"................",
"................",
"................",
])
