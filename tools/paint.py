"""Procedural background painting for Kryptids in Space (GBC).

A Canvas holds, per pixel, a BG palette slot (0-7) and a colour index (0-3).
Colour 0 is the shared deep-space colour in every BG palette, so palette
choice only matters for pixels with colour 1-3.  tileize() picks one
palette per 8x8 tile (majority vote) and remaps stray pixels to the nearest
colour of that palette, mirroring the CGB attribute model.
"""
import math, random
import numpy as np

def hx(h):
    h = h.lstrip('#'); return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))

SPACE = '#060818'
BG_PALS = {
    'SPACE':  [SPACE, '#1E2A58', '#7888C8', '#F0F4FF'],
    'NEBULA': [SPACE, '#180E34', '#341C5C', '#5C3C94'],
    'METAL':  [SPACE, '#262C36', '#56606C', '#A0A8B4'],
    'LIGHTS': [SPACE, '#14181E', '#48E0C8', '#F89830'],
    'TEXT':   ['#0C0E18', '#2C3448', '#48E0C8', '#F8F8F8'],
    'ICE':    [SPACE, '#1C3C6C', '#6CA8E0', '#E0F4FF'],
    'TENT':   [SPACE, '#3A0C30', '#A02870', '#F070B0'],
    'RUST':   [SPACE, '#2C1A14', '#8A4C2C', '#D8945C'],
    'K0':     [SPACE, '#3A0C30', '#A02870', '#F070B0'],
    'K1':     [SPACE, '#3A0C30', '#A02870', '#F8E040'],
    'HUD':    ['#0C0E18', '#2C3448', '#E83048', '#F8F8F8'],
}
# slot assignment during the stage
SLOT = {'SPACE': 0, 'NEBULA': 1, 'METAL': 2, 'LIGHTS': 3, 'COMM': 4, 'TEXT': 5,
        'ICE': 6, 'TENT': 6, 'K0': 6, 'RUST': 7, 'K1': 7}

class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.pal = np.zeros((h, w), np.uint8)
        self.col = np.zeros((h, w), np.uint8)
    def px(self, x, y, slot, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.pal[y, x] = slot; self.col[y, x] = c
    def rect(self, x0, y0, x1, y1, slot, c):
        x0, y0 = max(0, x0), max(0, y0)
        x1, y1 = min(self.w, x1), min(self.h, y1)
        if x1 > x0 and y1 > y0:
            self.pal[y0:y1, x0:x1] = slot; self.col[y0:y1, x0:x1] = c
    def mask(self, m, slot, c):
        self.pal[m] = slot; self.col[m] = c

def rgb_of(palname, c):
    return hx(BG_PALS[palname][c])

def tileize(cv, slot_pals, tileset, attr_bank=0, priority=False, flip=True):
    """slot_pals: dict slot -> palette name (for colour remapping).
    Returns (map rows [th][tw] of tile ids, attr rows)."""
    th, tw = cv.h // 8, cv.w // 8
    tmap = [[0] * tw for _ in range(th)]
    amap = [[0] * tw for _ in range(th)]
    for ty in range(th):
        for tx in range(tw):
            P = cv.pal[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
            C = cv.col[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
            nz = C > 0
            if nz.any():
                vals, cnt = np.unique(P[nz], return_counts=True)
                slot = int(vals[cnt.argmax()])
            else:
                slot = 0
            t = [[0] * 8 for _ in range(8)]
            tgt = [hx(c) for c in BG_PALS[slot_pals[slot]]]
            for y in range(8):
                for x in range(8):
                    c = int(C[y, x])
                    if c and int(P[y, x]) != slot:
                        src = hx(BG_PALS[slot_pals[int(P[y, x])]][c])
                        c = min(range(1, 4), key=lambda k: sum((a - b) ** 2 for a, b in zip(src, tgt[k])))
                    t[y][x] = c
            idx, fl = tileset.add(t, flip)
            tmap[ty][tx] = idx
            amap[ty][tx] = slot | fl | (0x08 if attr_bank else 0) | (0x80 if priority else 0)
    return tmap, amap

# ---------------------------------------------------------------- shapes
def ellipse_mask(w, h, cx, cy, rx, ry):
    yy, xx = np.mgrid[0:h, 0:w]
    return ((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2 <= 1.0

def shade_blob(cv, m, slot, light=(-0.6, -0.8), seed=0, levels=(0.25, 0.62), rim=True, noise=0.05):
    """Fill mask m with 3-tone shading (1 dark .. 3 light) lit from `light`."""
    ys, xs = np.nonzero(m)
    if not len(xs): return
    cx, cy = xs.mean(), ys.mean()
    rx = max(1, (xs.max() - xs.min()) / 2); ry = max(1, (ys.max() - ys.min()) / 2)
    nx = (xs - cx) / rx; ny = (ys - cy) / ry
    rr = np.clip(nx * nx + ny * ny, 0, 1)
    nz = np.sqrt(1 - rr)
    L = np.array([light[0], light[1], 0.75]); L = L / np.linalg.norm(L)
    d = (nx * L[0] + ny * L[1] + nz * L[2]) * 1.6 - 0.6   # spherical lambert, roughly -2..1
    rng = np.random.default_rng(seed)
    d = d + rng.normal(0, noise, len(d))
    c = np.where(d > levels[1], 3, np.where(d > -levels[0], 2, 1))
    cv.pal[ys, xs] = slot; cv.col[ys, xs] = c
    if rim:   # dark outline: mask pixels with a non-mask neighbour
        h, w = m.shape
        edge = np.zeros_like(m)
        edge[1:, :] |= m[1:, :] & ~m[:-1, :]
        edge[:-1, :] |= m[:-1, :] & ~m[1:, :]
        edge[:, 1:] |= m[:, 1:] & ~m[:, :-1]
        edge[:, :-1] |= m[:, :-1] & ~m[:, 1:]
        cv.col[edge] = 1

# ---------------------------------------------------------------- the Space Kraken (96x64)
def kraken(eye='open', beak='shut'):
    W, H = 96, 64
    cv = Canvas(W, H)
    K0, K1 = 6, 7
    # tentacle roots: thick curving arms sweeping out at the bottom
    yy, xx = np.mgrid[0:H, 0:W]
    roots = np.zeros((H, W), bool)
    for side in (-1, 1):
        for k, (spread, thick, drop) in enumerate([(40, 6.5, 6), (26, 6.0, 12), (12, 5.0, 16)]):
            for t in np.linspace(0, 1, 160):
                x = 48 + side * (6 + spread * t)
                y = 40 + drop * t + 10 * t * t - 4 * math.sin(t * 3.0) * (k == 0)
                r = thick * (1 - 0.55 * t)
                roots |= (xx - x) ** 2 + (yy - y) ** 2 <= r * r
    shade_blob(cv, roots, K0, light=(0, -1), seed=3, levels=(0.0, 0.7), noise=0.03)
    # suckers on the roots
    rng = random.Random(5)
    for _ in range(26):
        x, y = rng.randrange(8, 88), rng.randrange(44, 63)
        if roots[y, x] and cv.col[y, x] >= 2:
            cv.px(x, y, K0, 3)
    # mantle: tall rounded cone with fins, then the head bulb in front of it
    mantle = np.zeros((H, W), bool)
    for y in range(0, 46):
        t = y / 45.0
        half = 8 + 22 * math.sin(min(1, t * 1.15) * math.pi / 2) ** 1.4
        mantle[y, int(48 - half):int(48 + half)] = True
    for fx in (30, 66):
        shade_blob(cv, ellipse_mask(W, H, fx, 8, 13, 6), K0, light=(-0.4, -0.9), seed=fx, levels=(0.55, 0.5), noise=0.02)
    shade_blob(cv, mantle, K0, light=(-0.45, -0.6), seed=7, levels=(0.6, 0.55), noise=0.02)
    head = ellipse_mask(W, H, 48, 40, 27, 13)
    shade_blob(cv, head, K0, light=(-0.3, -0.9), seed=8, levels=(0.6, 0.5), noise=0.02)
    body = mantle | head
    # bioluminescent spots
    for (x, y, r) in [(40, 10, 2), (55, 7, 1.6), (48, 18, 2.2), (36, 24, 1.5), (60, 22, 1.8), (44, 31, 1.4), (53, 30, 1.6),
                      (26, 9, 1.4), (70, 9, 1.4)]:
        m = ellipse_mask(W, H, x, y, r, r) & body
        cv.mask(m, K0, 3)
        cv.mask(ellipse_mask(W, H, x, y, r + 1.2, r + 1.2) & body & ~ellipse_mask(W, H, x, y, r, r) & (cv.col == 2), K0, 1)
    # eyes
    for ex in (34, 62):
        e = ellipse_mask(W, H, ex, 39, 7.5, 6.5)
        cv.mask(e, K1, 1)
        inner = ellipse_mask(W, H, ex, 39, 6, 5)
        if eye == 'open':
            cv.mask(inner, K1, 3)
            cv.mask(ellipse_mask(W, H, ex, 39, 1.4, 4.5), K1, 1)        # slit pupil
            cv.px(ex - 3, 36, K1, 2); cv.px(ex - 2, 36, K1, 2)            # glint
        elif eye == 'angry':
            cv.mask(inner, K1, 3)
            cv.mask(ellipse_mask(W, H, ex, 40, 3.2, 3.2), K1, 1)
            # heavy brow
            for x in range(ex - 8, ex + 9):
                yb = 32 + (abs(x - ex) * (1 if (x - ex) * (ex - 48) > 0 else -1)) // 3 + 2
                for y in range(yb - 2, yb + 1):
                    cv.px(x, y, K1, 2)
        else:   # closed
            cv.mask(inner, K1, 2)
            for x in range(ex - 6, ex + 7):
                cv.px(x, 39 + (abs(x - ex) < 4), K1, 1)
    # beak
    bx, by = 48, 50
    if beak == 'shut':
        cv.mask(ellipse_mask(W, H, bx, by, 5, 4), K0, 1)
        for y in range(by - 3, by + 4):
            cv.px(bx, y, K0, 3)
    else:
        cv.mask(ellipse_mask(W, H, bx, by + 1, 7, 6), K0, 1)
        cv.mask(ellipse_mask(W, H, bx, by + 2, 4, 4), K0, 0)
        cv.mask(ellipse_mask(W, H, bx, by + 2, 4, 4), K0, 1)
        cv.mask(ellipse_mask(W, H, bx, by + 2, 2.5, 2.5), K1, 3)  # glowing throat
    return cv

# ---------------------------------------------------------------- scenery
def _star_patterns():
    pats = []
    for (x, y) in [(1, 2), (5, 1), (3, 5), (6, 6), (2, 7), (4, 3)]:
        pats.append([(x, y, 1)])                       # dim dots
    for (x, y) in [(2, 2), (5, 5), (6, 2)]:
        pats.append([(x, y, 2)])                       # medium
    pats.append([(3, 3, 3), (2, 3, 2), (4, 3, 2), (3, 2, 2), (3, 4, 2)])   # bright cross
    pats.append([(1, 1, 1), (6, 5, 2)])               # pair
    return pats
STAR_PATS = _star_patterns()

def stars_canvas(w, h, seed, density=0.012):
    """Stars are placed per 8x8 cell from a fixed pattern set, so they cost
    at most len(STAR_PATS) tiles no matter how long the level is."""
    cv = Canvas(w, h)
    rng = random.Random(seed)
    p = min(0.6, density * 25)
    for ty in range(h // 8):
        for tx in range(w // 8):
            if rng.random() < p:
                pat = STAR_PATS[min(len(STAR_PATS) - 1, int(rng.random() ** 1.8 * len(STAR_PATS)))]
                for (x, y, c) in pat:
                    cv.px(tx * 8 + x, ty * 8 + y, 0, c)
    return cv

def stamp(cv, src, x, y):
    """Copy src canvas into cv at tile-aligned (x, y) px, replacing whole cells."""
    assert x % 8 == 0 and y % 8 == 0
    for ty in range(src.h // 8):
        for tx in range(src.w // 8):
            dx, dy = x + tx * 8, y + ty * 8
            if 0 <= dx < cv.w and 0 <= dy < cv.h:
                cell = src.col[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
                if cell.any() or True:
                    cv.pal[dy:dy + 8, dx:dx + 8] = src.pal[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
                    cv.col[dy:dy + 8, dx:dx + 8] = cell

def stamp_mask(cv, src, x, y):
    """Like stamp() but only replaces cells where src has content."""
    for ty in range(src.h // 8):
        for tx in range(src.w // 8):
            dx, dy = x + tx * 8, y + ty * 8
            cell = src.col[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
            if cell.any() and 0 <= dx < cv.w and 0 <= dy < cv.h:
                cv.pal[dy:dy + 8, dx:dx + 8] = src.pal[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
                cv.col[dy:dy + 8, dx:dx + 8] = cell

def nebula_blob(cv, x0, y0, w, h, seed):
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:h, 0:w]
    v = np.zeros((h, w))
    for _ in range(7):
        cx, cy = rng.uniform(0.2, 0.8) * w, rng.uniform(0.2, 0.8) * h
        rx, ry = rng.uniform(0.15, 0.4) * w, rng.uniform(0.2, 0.45) * h
        v += np.exp(-(((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2))
    # ordered dithering for soft falloff
    bayer = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0
    th = bayer[yy % 4, xx % 4] * 0.35
    lvl = np.where(v > 1.4 + th, 3, np.where(v > 0.85 + th, 2, np.where(v > 0.45 + th, 1, 0)))
    for y in range(h):
        for x in range(w):
            if lvl[y, x] and 0 <= y0 + y < cv.h and 0 <= x0 + x < cv.w and cv.col[y0 + y, x0 + x] == 0:
                cv.px(x0 + x, y0 + y, 1, int(lvl[y, x]))

def station_canvas(h, seed=11):
    """Starhollow Station departure: hull walls both sides, docking clamps, lights."""
    w = 160
    cv = stars_canvas(w, h, seed)
    rng = random.Random(seed)
    M, L = 2, 3
    def module(x0, y0, x1, y1, lights=True):
        cv.rect(x0, y0, x1, y1, M, 2)
        cv.rect(x0, y0, x1, y0 + 1, M, 3)              # lit top edge
        cv.rect(x0, y1 - 1, x1, y1, M, 1)
        cv.rect(x1 - 1, y0, x1, y1, M, 1) if x0 < 80 else cv.rect(x0, y0, x0 + 1, y1, M, 1)
        for py in range(y0 + 6, y1 - 2, 8):            # panel seams
            cv.rect(x0 + 1, py, x1 - 1, py + 1, M, 1)
        if lights:
            for py in range(y0 + 3, y1 - 3, 8):
                lx = (x1 - 4) if x0 < 80 else (x0 + 2)
                cv.rect(lx, py, lx + 2, py + 2, L, 2 if (py // 8) % 3 else 3)
    def truss(x0, x1, y0, y1):
        for y in range(y0, y1):
            cv.px(x0, y, M, 2); cv.px(x1 - 1, y, M, 2)
            k = (y - y0) % 8
            cv.px(x0 + k * (x1 - x0 - 1) // 7, y, M, 1); cv.px(x1 - 1 - k * (x1 - x0 - 1) // 7, y, M, 1)
    y = 0
    while y < h:
        hh = rng.choice([24, 32, 40, 48])
        wl = rng.choice([24, 32, 40]); wr = rng.choice([24, 32, 40])
        module(0, y, wl, y + hh - 8); truss(4, 12, y + hh - 8, y + hh)
        module(160 - wr, y + 8, 160, y + hh); truss(148, 156, y, y + 8)
        # docking arm reaching inward occasionally
        if rng.random() < 0.4:
            ay = y + hh // 2
            cv.rect(wl, ay, wl + 16, ay + 6, M, 2); cv.rect(wl, ay, wl + 16, ay + 1, M, 3)
            cv.rect(wl + 14, ay - 2, wl + 18, ay + 8, L, 1); cv.rect(wl + 15, ay + 2, wl + 17, ay + 4, L, 3)
        y += hh
    return cv

def launch_bay(cv):
    """Bottom screen: launch rails + bay floor stripes under the ship."""
    M, L = 2, 3
    h = cv.h
    for x in (56, 96):
        cv.rect(x, h - 96, x + 8, h, M, 2); cv.rect(x, h - 96, x + 1, h, M, 3); cv.rect(x + 7, h - 96, x + 8, h, M, 1)
        for y in range(h - 92, h, 8):
            cv.rect(x + 2, y, x + 6, y + 2, L, 2)
    for y in range(h - 24, h, 8):
        for x in range(64, 96, 8):
            cv.rect(x, y, x + 4, y + 4, L, 3 if (x // 8 + y // 8) % 2 else 1)

def nebula_stamp(w, h, seed):
    cv = Canvas(w, h)
    nebula_blob(cv, 0, 0, w, h, seed)
    return cv

def rock_stamp(size, seed):
    """Shaded ice rock in ICE palette (slot 6), size px square (multiple of 8)."""
    cv = Canvas(size, size)
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:size, 0:size]
    c = size / 2 - 0.5
    r = size / 2 - 1.5
    ang = np.arctan2(yy - c, xx - c)
    rr = r * (1 + 0.10 * np.sin(ang * 3 + rng.uniform(0, 6)) + 0.05 * np.sin(ang * 7 + 1)) / 1.15
    m = (xx - c) ** 2 + (yy - c) ** 2 <= rr ** 2
    shade_blob(cv, m, 6, seed=seed, levels=(0.3, 0.45), noise=0.02)
    # a couple of crack lines
    for _ in range(2):
        x0, y0 = int(rng.integers(size // 4, size * 3 // 4)), int(rng.integers(size // 4, size * 3 // 4))
        for k in range(size // 4):
            x, y = x0 + k * (1 if seed % 2 else -1) // 2, y0 + k // 2 + (k % 3 == 0)
            if 0 <= x < size and 0 <= y < size and m[y, x]:
                cv.px(x, y, 6, 1)
    return cv

ROCKS = None
def ice_canvas(h, seed=21):
    global ROCKS
    if ROCKS is None:
        ROCKS = [rock_stamp(40, 1), rock_stamp(32, 2), rock_stamp(24, 3), rock_stamp(16, 4)]
    w = 160
    cv = stars_canvas(w, h, seed, 0.010)
    rng = random.Random(seed)
    y = 0
    while y < h:
        for side in (0, 1):
            if rng.random() < 0.85:
                rk = rng.choice(ROCKS[:3])
                x = (rng.choice([-16, -8, 0, 8]) if side == 0 else 160 - rk.w + rng.choice([16, 8, 0, -8]))
                stamp_mask(cv, rk, x, y + rng.choice([0, 8, 16]))
        if rng.random() < 0.5:
            stamp_mask(cv, ROCKS[3], rng.choice([48, 64, 80, 96]), y + 24)
        y += 48
    return cv

def tank_stamp(variant):
    """Bright Basin tanker hull fragment (left side; mirror for right), 56x48."""
    W, H = 56, 48
    cv = Canvas(W, H)
    M, T, R = 2, 6, 7
    rng = random.Random(variant)
    for yy in range(4, H - 4):
        t = (yy - 4) / (H - 8)
        c = 3 if t < 0.2 else (2 if t < 0.65 else 1)
        for xx in range(0, W - 8):
            cv.px(xx, yy, R, c)
    for xx in range(4, W - 8, 8):
        cv.rect(xx, 4, xx + 2, H - 4, R, 1)
    for yy in range(4, H - 4, 2):                  # torn edge, stepped in pairs of rows
        j = rng.randrange(0, 7)
        cv.rect(W - 8 - j, yy, W - 8, yy + 2, 0, 0)
    cv.rect(0, 12, W - 12, 14, M, 3)               # stencil band
    # tentacle coiling across the tank
    yy_, xx_ = np.mgrid[0:H, 0:W]
    m = np.zeros((H, W), bool)
    phase = variant * 1.7
    for i in range(70):
        t = i / 69
        px = W - 16 + 7 * math.sin(t * 6 + phase) - 22 * t * (1 - t) * 4 * (1 if variant % 2 else 0.6)
        py = 1 + t * (H - 2)
        r = 4.5 - 2.6 * t
        m |= (xx_ - px) ** 2 + (yy_ - py) ** 2 <= r * r
    shade_blob(cv, m, T, seed=variant + 5, levels=(0.2, 0.5), noise=0.02)
    return cv

def mirror_canvas(src):
    cv = Canvas(src.w, src.h)
    cv.pal[:] = src.pal[:, ::-1]; cv.col[:] = src.col[:, ::-1]
    return cv

TANKS = None
def wreck_canvas(h, seed=31):
    global TANKS
    if TANKS is None:
        TANKS = [tank_stamp(1), tank_stamp(2)]
    w = 160
    cv = stars_canvas(w, h, seed, 0.010)
    rng = random.Random(seed)
    y = 0
    while y < h:
        tk = rng.choice(TANKS)
        if rng.random() < 0.5:
            stamp_mask(cv, tk, rng.choice([-8, 0]), y)
        else:
            stamp_mask(cv, mirror_canvas(tk), 160 - tk.w + rng.choice([8, 0]), y)
        # debris chunks (tile-aligned 8x8)
        for _ in range(2):
            dx, dy = rng.choice([56, 72, 88, 96]), y + rng.choice([8, 24, 40])
            cv.rect(dx + 2, dy + 3, dx + 6, dy + 5, 2, rng.choice([1, 2]))
        y += 48 + rng.choice([8, 16, 24])
    return cv


def convoy_canvas(h, seed=81):
    """Abandoned Bright Basin haulers drifting in open space (reuses wreck stamps)."""
    global TANKS
    if TANKS is None:
        TANKS = [tank_stamp(1), tank_stamp(2)]
    cv = stars_canvas(160, h, seed, 0.010)
    rng = random.Random(seed)
    y = 8
    while y < h - 48:
        tk = rng.choice(TANKS)
        if rng.random() < 0.5:
            stamp_mask(cv, tk, rng.choice([-24, -16]), y)
        else:
            stamp_mask(cv, mirror_canvas(tk), 160 - tk.w + rng.choice([24, 16]), y)
        y += 80 + rng.choice([0, 16, 32])
    return cv
