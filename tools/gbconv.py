"""Image -> Game Boy Color tiles with per-tile palette choice.

convert(img, npal) quantizes an RGB image (dimensions multiple of 8) to
`npal` 4-colour palettes, each 8x8 tile using one of them (the CGB BG
attribute model).  Returns palettes, per-tile palette index, and the
per-tile 2bpp pixel indices.
"""
import numpy as np
from PIL import Image, ImageEnhance, ImageFilter

def to15(c):
    """Snap an RGB colour to the CGB's 5-bit-per-channel grid."""
    return tuple(int(round(v / 255 * 31)) * 255 // 31 for v in c)

def rgb555(c):
    r, g, b = (int(round(v / 255 * 31)) for v in c)
    return r | (g << 5) | (b << 10)

W = np.array([0.30, 0.59, 0.11]) ** 0.5 * 1.7   # perceptual-ish channel weights

def _kmeans(X, k, iters=12, seed=0):
    rng = np.random.default_rng(seed)
    if len(X) <= k:
        C = np.vstack([X, np.repeat(X[:1], k - len(X), 0)]) if len(X) else np.zeros((k, X.shape[1]))
        return C
    # k-means++ init
    C = [X[rng.integers(len(X))]]
    for _ in range(1, k):
        d = np.min(((X[:, None] - np.array(C)[None]) ** 2).sum(-1), 1)
        p = d / d.sum() if d.sum() > 0 else None
        C.append(X[rng.choice(len(X), p=p)])
    C = np.array(C, float)
    for _ in range(iters):
        lab = np.argmin(((X[:, None] - C[None]) ** 2).sum(-1), 1)
        for j in range(k):
            m = X[lab == j]
            if len(m): C[j] = m.mean(0)
    return C

def prep(img, size, contrast=1.15, sat=1.2, sharpen=True, ink=0):
    img = img.convert("RGB").resize(size, Image.LANCZOS)
    img = ImageEnhance.Contrast(img).enhance(contrast)
    img = ImageEnhance.Color(img).enhance(sat)
    if sharpen:
        img = img.filter(ImageFilter.UnsharpMask(radius=1, percent=80, threshold=2))
    if ink:
        # darken strong edges a little so line art survives the downscale
        g = np.asarray(img.convert("L").filter(ImageFilter.FIND_EDGES), float)
        a = np.asarray(img, float)
        a *= (1 - np.clip((g - 60) / 200, 0, ink))[..., None]
        img = Image.fromarray(a.clip(0, 255).astype(np.uint8))
    return img

def convert(img, npal, iters=8, fixed=None, seed=1):
    """fixed: optional list of colours forced into every palette (e.g. black)."""
    a = np.asarray(img.convert("RGB"), float)
    H, Wd = a.shape[:2]
    ty, tx = H // 8, Wd // 8
    tiles = a.reshape(ty, 8, tx, 8, 3).transpose(0, 2, 1, 3, 4).reshape(ty * tx, 64, 3) * W
    fixed = [np.array(f, float) * W for f in (fixed or [])]
    # init: cluster tiles by mean+std colour
    feat = np.hstack([tiles.mean(1), tiles.std(1)])
    tlab = np.argmin(((feat[:, None] - _kmeans(feat, npal, seed=seed)[None]) ** 2).sum(-1), 1)
    pals = None
    for it in range(iters):
        pals = []
        for p in range(npal):
            px = tiles[tlab == p].reshape(-1, 3)
            if not len(px):
                px = tiles.reshape(-1, 3)[::7]
            C = _kmeans(px, 4 - len(fixed), seed=seed + it)
            pals.append(np.vstack(fixed + [C]) if fixed else C)
        pals = np.array(pals)                      # npal x 4 x 3
        # error of each tile under each palette
        d = ((tiles[:, None, :, None, :] - pals[None, :, None]) ** 2).sum(-1)  # T x P x 64 x 4
        err = d.min(-1).sum(-1)
        tlab = err.argmin(1)
    d = ((tiles[:, None, :, None, :] - pals[None, :, None]) ** 2).sum(-1)
    idx = d.min(-1)  # noqa
    pix = np.array([d[t, tlab[t]].argmin(-1) for t in range(len(tiles))])   # T x 64
    pals_rgb = [[to15(tuple((c / W).clip(0, 255))) for c in pal] for pal in pals]
    # sort each palette light->dark so colour 0 is lightest (matches GB convention); remap pixels
    for p in range(npal):
        lum = [0.3 * c[0] + 0.59 * c[1] + 0.11 * c[2] for c in pals_rgb[p]]
        order = np.argsort(lum)[::-1]
        remap = np.empty(4, int); remap[order] = np.arange(4)
        pals_rgb[p] = [pals_rgb[p][o] for o in order]
        sel = tlab == p
        pix[sel] = remap[pix[sel]]
    return dict(pals=pals_rgb, tpal=tlab.reshape(ty, tx), pix=pix.reshape(ty, tx, 8, 8), tw=tx, th=ty)

def tile_bytes(px8):
    """8x8 array of colour indices 0-3 -> 16 bytes of GB 2bpp."""
    out = []
    for row in px8:
        lo = hi = 0
        for x, v in enumerate(row):
            if v & 1: lo |= 0x80 >> x
            if v & 2: hi |= 0x80 >> x
        out += [lo, hi]
    return out

def render(res, scale=1, pal_base=0, pals=None):
    pals = pals or res["pals"]
    th, tw = res["th"], res["tw"]
    img = Image.new("RGB", (tw * 8, th * 8))
    pxl = img.load()
    for ty in range(th):
        for tx in range(tw):
            pal = pals[res["tpal"][ty, tx]]
            for y in range(8):
                for x in range(8):
                    pxl[tx * 8 + x, ty * 8 + y] = pal[res["pix"][ty, tx, y, x]]
    return img.resize((tw * 8 * scale, th * 8 * scale), Image.NEAREST)
