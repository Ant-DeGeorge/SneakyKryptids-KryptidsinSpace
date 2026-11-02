"""Read KRYPTID-DEX strips (ref/dex/*_strip_1x.png: 10 frames of 32x32) and turn
frames into sprite row-strings ('.123') for the shooter.

Frame order: idle0 idle1 move0 move1 move2 move3 atk0 atk1 hurt sleep.
"""
import os
import numpy as np
from PIL import Image

DEX = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "ref", "dex")
FR = {n: i for i, n in enumerate("idle0 idle1 move0 move1 move2 move3 atk0 atk1 hurt sleep".split())}


def colors(name):
    im = Image.open(os.path.join(DEX, f"{name}_strip_1x.png")).convert("RGBA")
    return sorted({c[:3] for n, c in im.getcolors(65536) if c[3]}, key=lambda c: sum(c))


def frame(name, f, cmap=None):
    """Index array (0 = transparent). cmap: {(r,g,b): idx}; default = darkest 3,
    cyan accent 1, everything else 2 (by brightness)."""
    im = Image.open(os.path.join(DEX, f"{name}_strip_1x.png")).convert("RGBA")
    k = FR[f] if isinstance(f, str) else f
    fr = np.array(im.crop((k * 32, 0, k * 32 + 32, 32)))
    if cmap is None:
        cs = colors(name)
        cmap = {}
        for c in cs:
            if c == (65, 238, 246): cmap[c] = 1
            elif c == cs[0]: cmap[c] = 3
            else: cmap[c] = 2
    out = np.zeros((32, 32), np.uint8)
    for y in range(32):
        for x in range(32):
            if fr[y, x, 3]: out[y, x] = cmap[tuple(fr[y, x, :3])]
    return out


def bbox(a):
    ys, xs = np.nonzero(a)
    return xs.min(), ys.min(), xs.max() + 1, ys.max() + 1


def crop(a, box):
    x0, y0, x1, y1 = box
    return a[y0:y1, x0:x1].copy()


def half(a, outline=3, accent=1):
    """2x downscale that keeps the silhouette: accent wins, then body majority, then outline."""
    h, w = (a.shape[0] + 1) // 2 * 2, (a.shape[1] + 1) // 2 * 2
    p = np.zeros((h, w), np.uint8); p[:a.shape[0], :a.shape[1]] = a
    out = np.zeros((h // 2, w // 2), np.uint8)
    for y in range(h // 2):
        for x in range(w // 2):
            b = list(p[y * 2:y * 2 + 2, x * 2:x * 2 + 2].flatten())
            op = [v for v in b if v]
            if len(op) < 2 and not (op and op[0] == accent): continue
            if accent in op: out[y, x] = accent
            elif sum(1 for v in op if v != outline) >= 2: out[y, x] = [v for v in op if v != outline][0]
            else: out[y, x] = outline
    return out


def reoutline(a, outline=3):
    """Ring every body pixel with outline where it touches transparency (after scaling)."""
    out = a.copy()
    h, w = a.shape
    for y in range(h):
        for x in range(w):
            if a[y, x] and a[y, x] != outline:
                for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                    yy, xx = y + dy, x + dx
                    if not (0 <= yy < h and 0 <= xx < w) or not a[yy, xx]:
                        out[y, x] = outline
                        break
    return out


def place(a, w=16, h=16, ax='c', ay='c'):
    """Put a into a w x h canvas (clipped), centred or aligned ('l','r','t','b')."""
    out = np.zeros((h, w), np.uint8)
    ah, aw = a.shape
    x0 = (w - aw) // 2 if ax == 'c' else (0 if ax == 'l' else w - aw)
    y0 = (h - ah) // 2 if ay == 'c' else (0 if ay == 't' else h - ah)
    for y in range(ah):
        for x in range(aw):
            yy, xx = y + y0, x + x0
            if 0 <= yy < h and 0 <= xx < w and a[y, x]: out[yy, xx] = a[y, x]
    return out


def fit(a, w=16, h=16, ax='c', ay='c'):
    """Crop to content, halve if it does not fit, then place."""
    a = crop(a, bbox(a))
    if a.shape[0] > h or a.shape[1] > w:
        a = half(a)
        a = crop(a, bbox(a))
    return place(a, w, h, ax, ay)


def remap(a, m):
    out = a.copy()
    for k, v in m.items(): out[a == k] = v
    return out


def rows(a):
    return ["".join('.' if v == 0 else str(int(v)) for v in r) for r in a]


def flipx(a): return a[:, ::-1].copy()
def rot_cw(a): return np.rot90(a, -1).copy()
def rot_ccw(a): return np.rot90(a, 1).copy()


def scale2(a):
    """EPX/Scale2x: 2x upscale that rounds diagonal edges (for BG boss art)."""
    h, w = a.shape
    out = np.zeros((h * 2, w * 2), a.dtype)
    g = lambda y, x: a[y, x] if 0 <= y < h and 0 <= x < w else 0
    for y in range(h):
        for x in range(w):
            P = a[y, x]; A = g(y - 1, x); B = g(y, x + 1); C = g(y, x - 1); D = g(y + 1, x)
            o = [P, P, P, P]
            if C == A and C != D and A != B: o[0] = A
            if A == B and A != C and B != D: o[1] = B
            if D == C and D != B and C != A: o[2] = C
            if B == D and B != A and D != C: o[3] = D
            out[y * 2, x * 2], out[y * 2, x * 2 + 1], out[y * 2 + 1, x * 2], out[y * 2 + 1, x * 2 + 1] = o
    return out


def preview(arrs, pal, path, scale=6):
    """arrs: list of index arrays; pal: 4 hex colours (0 drawn as grey)."""
    hx = lambda s: tuple(int(s[i:i + 2], 16) for i in (1, 3, 5))
    W = sum(a.shape[1] + 2 for a in arrs); H = max(a.shape[0] for a in arrs)
    im = Image.new("RGB", (W, H), (70, 70, 90)); px = im.load(); x0 = 0
    for a in arrs:
        for y in range(a.shape[0]):
            for x in range(a.shape[1]):
                if a[y, x]: px[x0 + x, y] = hx(pal[a[y, x]])
        x0 += a.shape[1] + 2
    im.resize((W * scale, H * scale), Image.NEAREST).save(path)
