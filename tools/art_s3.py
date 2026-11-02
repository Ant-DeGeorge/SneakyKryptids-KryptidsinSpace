"""Stage 3 art: the Barrens Belt (space pine forest) and the Jersey Devil."""
import math, random
import numpy as np
import paint as PT
from paint import Canvas, shade_blob, ellipse_mask, stamp_mask, stars_canvas, mirror_canvas, BG_PALS

BAR_SPACE = '#060E10'
BG_PALS.update({
    'SPACE3': [BAR_SPACE, '#1C3A3A', '#6C9C90', '#E8FFF4'],
    'FOG':    [BAR_SPACE, '#0E2220', '#1C3C34', '#335C4C'],
    'ROCK':   [BAR_SPACE, '#2A1C14', '#6A4A30', '#A88058'],
    'PINE':   [BAR_SPACE, '#0C2618', '#2A6A30', '#78B048'],
    'CABIN':  [BAR_SPACE, '#24140C', '#6A3C20', '#F8B040'],
    'WISPBG': [BAR_SPACE, '#103030', '#40C0A0', '#C8FFF0'],
    'JD0':    [BAR_SPACE, '#3A1E36', '#D5C5AC', '#41EEF6'],      # sheet: dark wing, bone, cyan eye
    'JD1':    [BAR_SPACE, '#3A1E36', '#D5C5AC', '#FF5030'],      # angry red eye
    'JD0HIT': [BAR_SPACE, '#7A4E70', '#FFF4E0', '#C0FFFF'],
    'VOID3':  [BAR_SPACE, BAR_SPACE, '#0E1A1A', '#1C2A28'],
})
FOG, ROCK, PINE, CABIN, WISP = 1, 2, 3, 6, 7
JD0, JD1 = 6, 7


def rock_with_pines(w, h, seed, n_pines=3):
    """Floating asteroid chunk with little pine trees growing off its top."""
    cv = Canvas(w, h)
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:h, 0:w]
    cy = h * 0.68
    ang = np.arctan2(yy - cy, xx - w / 2)
    rx, ry = w * 0.46, h * 0.30
    rr = 1 + 0.12 * np.sin(ang * 3 + rng.uniform(0, 6)) + 0.06 * np.sin(ang * 7)
    m = ((xx - w / 2) / (rx * rr)) ** 2 + ((yy - cy) / (ry * rr)) ** 2 <= 1
    shade_blob(cv, m, ROCK, light=(-0.4, -1.0), seed=seed, levels=(0.35, 0.5), noise=0.02)
    # pines rooted along the top edge
    tops = [int(cy - ry * 0.8)] * w
    for x in range(w):
        col = np.nonzero(m[:, x])[0]
        if len(col): tops[x] = col[0]
    xs = sorted(rng.choice(np.arange(6, w - 6), size=min(n_pines, max(1, w // 10)), replace=False))
    for x0 in xs:
        base = tops[x0] + 1
        th = int(rng.integers(12, 20))
        for k in range(th):
            y = base - k
            half = (th - k) * 0.42 + (1.5 if (k % 4) == 0 else 0)
            for x in range(int(x0 - half), int(x0 + half) + 1):
                if 0 <= x < w and 0 <= y < h:
                    c = 3 if x < x0 - half * 0.2 else (2 if x < x0 + half * 0.5 else 1)
                    if abs(x - x0) >= half - 0.6: c = 1
                    cv.px(x, y, PINE, c)
        for y in range(base - 1, min(h, base + 3)):
            cv.px(x0, y, ROCK, 1)
    return cv


def cabin_stamp(seed):
    """Abandoned Barrens cabin on a rock: plank walls, roof, two lit windows."""
    cv = rock_with_pines(48, 48, seed, n_pines=1)
    x0, y0 = 12, 10
    for y in range(y0 + 8, y0 + 22):
        for x in range(x0, x0 + 24):
            cv.px(x, y, CABIN, 2 if (y - y0) % 3 else 1)
    for k in range(10):       # roof
        for x in range(x0 - 2 + k, x0 + 26 - k):
            cv.px(x, y0 + 8 - k, CABIN, 1 if k < 2 else 2)
    for wx in (x0 + 4, x0 + 15):
        cv.rect(wx, y0 + 12, wx + 5, y0 + 17, CABIN, 3)
        cv.rect(wx + 2, y0 + 12, wx + 3, y0 + 17, CABIN, 1)
    cv.rect(x0 + 10, y0 + 15, x0 + 14, y0 + 22, CABIN, 1)   # door
    return cv


def log_stamp(seed):
    """Drifting hollow log, 32x16."""
    cv = Canvas(32, 16)
    for y in range(3, 13):
        for x in range(3, 29):
            c = 3 if y < 6 else (2 if y < 10 else 1)
            if (x + y * 3) % 9 == 0: c = 1
            cv.px(x, y, ROCK, c)
    cv.mask(ellipse_mask(32, 16, 28, 8, 3.2, 5), ROCK, 1)
    cv.mask(ellipse_mask(32, 16, 28, 8, 1.6, 2.8), ROCK, 0)
    cv.mask(ellipse_mask(32, 16, 4, 8, 3, 5), ROCK, 3)
    cv.mask(ellipse_mask(32, 16, 4, 8, 1.5, 2.5), ROCK, 2)
    return cv


def fog_stamp(w, h, seed):
    cv = Canvas(w, h)
    PT.nebula_blob(cv, 0, 0, w, h, seed)
    m = cv.col > 0
    cv.col[m] = np.maximum(cv.col[m] - 1, 0)
    return cv


def barrens_canvas(h, seed, cabins=False, density=1.0):
    cv = stars_canvas(160, h, seed, 0.008)
    rng = random.Random(seed)
    if not hasattr(barrens_canvas, 'st'):
        barrens_canvas.st = [rock_with_pines(48, 40, 11, 3), rock_with_pines(40, 32, 12, 2), rock_with_pines(56, 40, 13, 4)]
        barrens_canvas.cab = [cabin_stamp(21), cabin_stamp(22)]
        barrens_canvas.fog = [fog_stamp(64, 40, 31), fog_stamp(48, 32, 32)]
        barrens_canvas.log = log_stamp(1)
    y = 8
    while y < h - 40:
        if rng.random() < 0.4:
            stamp_mask(cv, rng.choice(barrens_canvas.fog), rng.choice([40, 56, 72]), y)
        for side in (0, 1):
            if rng.random() < 0.8 * density:
                pool = barrens_canvas.cab if (cabins and rng.random() < 0.45) else barrens_canvas.st
                s = rng.choice(pool)
                if side: s = mirror_canvas(s)
                x = rng.choice([-16, -8, 0]) if side == 0 else 160 - s.w + rng.choice([0, 8, 16])
                stamp_mask(cv, s, x, y + rng.choice([0, 8]))
        if rng.random() < 0.3:
            stamp_mask(cv, barrens_canvas.log, rng.choice([48, 64, 80]), y + 24)
        y += 48
    return cv


# ---------------------------------------------------------------- Jersey Devil (112 x 64)
def jersey_devil(eye='open', mouth='shut'):
    W, H = 112, 64
    cv = Canvas(W, H)
    cx = 56
    yy, xx = np.mgrid[0:H, 0:W]
    d = np.abs(xx - cx + 0.5)
    # bat wings: membrane between finger bones, scalloped lower edge
    t = np.clip((d - 12) / 43.0, 0, 1)
    top = 20 - 16 * np.sin(t * math.pi / 2)
    scal = 5 * np.abs(np.sin((d - 12) * math.pi / 10.75))
    bot = 44 - 18 * t + scal - 6 * t ** 3
    wing = (d >= 10) & (d <= 55) & (yy >= top) & (yy <= bot)
    shade_blob(cv, wing, JD0, light=(0.0, -1.0), seed=3, levels=(0.3, 0.6), noise=0.01)
    for side in (-1, 1):               # finger bones
        for k in range(5):
            tip = 12 + (k + 0.5) * 10.75
            tt = min(1, (tip - 12) / 43.0)
            ty = 44 - 18 * tt - 6 * tt ** 3
            for s in np.linspace(0, 1, 40):
                x = cx + side * (12 + (tip - 12) * s)
                y = 22 + (ty - 22) * s - 8 * math.sin(s * math.pi) * (0.3 + k * 0.15)
                if 0 <= int(x) < W and 0 <= int(y) < H and wing[int(y), int(x)]:
                    cv.px(int(x), int(y), JD0, 3 if s < 0.8 else 1)
        cv.mask(ellipse_mask(W, H, cx + side * 50, 4, 2, 3) & ~wing, JD0, 1)   # wing claws
    # body + forelegs with hooves
    body = ellipse_mask(W, H, cx, 40, 11, 15)
    shade_blob(cv, body, JD0, light=(-0.4, -0.9), seed=4, levels=(0.3, 0.45), noise=0.01)
    for sx in (-7, 7):
        leg = np.zeros((H, W), bool)
        for y in range(46, 60):
            leg[y, int(cx + sx - 2):int(cx + sx + 2)] = True
        shade_blob(cv, leg, JD0, light=(-0.5, -0.5), seed=5, levels=(0.4, 0.4), noise=0.0)
        cv.rect(int(cx + sx - 3), 58, int(cx + sx + 3), 62, JD1, 1)
        cv.rect(int(cx + sx - 2), 58, int(cx + sx + 2), 59, JD0, 2)
    # long horse head, pointing down
    head = ellipse_mask(W, H, cx, 22, 8.5, 10) | ellipse_mask(W, H, cx, 33, 5.5, 6)
    shade_blob(cv, head, JD0, light=(-0.4, -1.0), seed=6, levels=(0.25, 0.5), noise=0.01)
    # horns curling out and up
    for side in (-1, 1):
        for s in np.linspace(0, 1, 30):
            x = cx + side * (6 + 12 * s)
            y = 14 - 12 * math.sin(s * math.pi * 0.8) + 4 * s
            r = 2.2 * (1 - s) + 0.6
            cv.mask(ellipse_mask(W, H, x, y, r, r), JD1, 1 if s > 0.15 else 1)
            if s < 0.85: cv.px(int(x), int(y - r + 1), JD0, 3)
    # ears
    for side in (-1, 1):
        cv.mask(ellipse_mask(W, H, cx + side * 9, 17, 2.5, 4), JD0, 2)
    # eyes
    for ex in (cx - 4, cx + 4):
        if eye == 'closed':
            for x in range(ex - 2, ex + 3): cv.px(x, 22, JD0, 1)
        else:
            cv.mask(ellipse_mask(W, H, ex, 22, 2.4, 1.8), JD1, 1)
            cv.mask(ellipse_mask(W, H, ex, 22, 1.6, 1.1), JD1, 3)
            if eye == 'angry':
                sgn = 1 if ex > cx else -1
                for k in range(-3, 4):
                    cv.px(ex + k, 19 + (k * sgn) // 2, JD0, 1)
    # nostrils + mouth
    for nx in (cx - 2, cx + 2):
        cv.px(nx, 35, JD0, 1); cv.px(nx, 36, JD0, 1)
    if mouth == 'open':
        cv.mask(ellipse_mask(W, H, cx, 38, 4.5, 3), JD1, 1)
        cv.mask(ellipse_mask(W, H, cx, 38, 3, 1.8), JD1, 2)
        cv.mask(ellipse_mask(W, H, cx, 38, 1.5, 1), JD1, 3)
    else:
        for x in range(cx - 3, cx + 4): cv.px(x, 38, JD0, 1)
    return cv


# ---------------------------------------------------------------- sprites
def mirror(half):
    return [h + h[::-1] for h in half]

# Leeds imp: little bat-winged devil (1 orange, 2 red, 3 dark)
IMP_A = mirror([
"........",
"3......3",
"33.....3",
"323...32",
"3223.332",
"32223322",
".3222312",
"..322212",
"...32222",
"....3222",
"....3.32",
"...3..3.",
"........",
"........",
"........",
"........",
])
IMP_B = mirror([
"........",
"........",
".......3",
"......32",
"3333.332",
"32223322",
"32222312",
".3322212",
"...32222",
"....3222",
"....3.32",
"....3.3.",
"........",
"........",
"........",
"........",
])
# pine-cone mine (1 glow, 2 bark, 3 dark)
CONE_A = mirror([
"........",
".....333",
"....3222",
"...32323",
"..323232",
"..232323",
".3232323",
".3323232",
".3232323",
"..323232",
"..332323",
"...33232",
"....3332",
".......3",
"........",
"........",
])
CONE_B = [r.replace('2', '1', 2) if 5 <= i <= 9 else r for i, r in enumerate(CONE_A)]
# will-o'-wisp (1 white, 2 green, 3 dark green)
WISP_A = mirror([
"........",
"........",
"......33",
".....322",
"....3221",
"...32211",
"...32111",
"...32111",
"...32211",
"....3221",
".....322",
"......32",
".......3",
"......3.",
"........",
"........",
])
WISP_B = mirror([
"........",
"........",
"........",
"......33",
".....322",
"....3221",
"....3211",
"....3211",
"....3221",
".....322",
"......32",
".....3.3",
"......3.",
".......3",
"........",
"........",
])
TAIL_SEG = [
"..3333..",
".322223.",
"32222223",
"32222223",
"32222223",
".322223.",
"..3333..",
"........",
] + ["........"] * 8
TAIL_TIP = [
"...33...",
"..3223..",
".321123.",
"32111123",
"32211223",
".322223.",
"..3223..",
"...33...",
] + ["........"] * 8
