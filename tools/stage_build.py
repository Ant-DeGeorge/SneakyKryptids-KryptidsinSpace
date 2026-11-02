"""StageBuilder: turns painted canvases, sprite art and a boss image into
banked GBC data for one stage of the shooter.

VRAM per stage
  BG  bank0 ids N_COMMON..255   scenery (first part)
  BG  bank1 ids 0..COMM_TILE-1  scenery overflow
  BG  bank1 ids 112..127        comm mini portrait (engine)
  BG  bank1 ids 128..255        boss image (unique tiles, face swaps)
  OBJ bank1 tiles 0..127        this stage's enemies / boss parts
Palettes
  BG slot 4 = comm portrait, slot 5 = HUD; stage uses 0-3, 6, 7
  OBJ 3 = enemy bullets, 4-6 = stage enemies (loaded per stage)
"""
import os
from PIL import Image
import paint as PT
from paint import hx, BG_PALS
from gbart import parse, encode, TileSet, hflip_t, vflip_t, key
from emit import pal_words

ROWS_PER_BANK = 16384 // 40
BOSS_BASE = 128
COMM_TILE = 112


def norm(rows, w):
    return [(r.replace(' ', '3') + '.' * w)[:w] for r in rows]


class PalLib:
    """Global BG palette library (names -> index)."""
    def __init__(self):
        self.names, self.cols = [], []
    def add(self, name, cols=None):
        if name in self.names:
            return self.names.index(name)
        cols = cols or BG_PALS[name]
        BG_PALS.setdefault(name, cols)
        self.names.append(name); self.cols.append(cols)
        return len(self.names) - 1


class StageBuilder:
    def __init__(self, num, out, lib, n_common, data_bank, row_banks, prev_dir):
        self.n = num
        self.P = f"S{num}_"
        self.out, self.lib, self.n_common = out, lib, n_common
        self.dbank, self.row_banks = data_bank, row_banks
        self.prev = prev_dir
        self.scen = TileSet(0)
        self.scen.add([[0] * 8 for _ in range(8)])
        self.rows = []                 # (tiles, attrs, slotpal-dict) bottom-up
        self.sections = {}
        self.spr = []
        self.obj_pals = []
        self.boss = None
        self.init_slots = None
        self.cps = [0, 0, 0]

    # ------------------------------------------------------------ palettes
    def slots(self, mapping):
        """mapping slot->palette name; registers names in the library."""
        for s, nm in mapping.items():
            self.lib.add(nm)
        return dict(mapping)

    # ------------------------------------------------------------ scenery
    def section(self, name, cv, slot_pals):
        self.sections[name] = len(self.rows)
        tm, am = PT.tileize(cv, slot_pals, self.scen)
        for r in range(len(tm) - 1, -1, -1):
            self.rows.append((tm[r], am[r], slot_pals))
        if self.init_slots is None:
            self.init_slots = dict(slot_pals)
        print(f"  S{self.n} {name}: {len(tm)} rows, scenery tiles {len(self.scen.tiles)}")

    def checkpoint(self, k, row):
        self.cps[k] = row

    # ------------------------------------------------------------ boss (BG image W x 8 tiles at top of the level)
    def boss_image(self, base_cv, variants, slot_pals, stars_seed=71, hit_pal=None):
        """base_cv: W*8 x 64 canvas; variants: list of (name, canvas). Must be the last section."""
        W = base_cv.w // 8
        assert base_cv.h == 64 and W <= 16
        self.sections['lair0'] = len(self.rows)
        sky = PT.stars_canvas(160, 64 + 80, seed=stars_seed, density=0.004)
        lower = PT.Canvas(160, 80); lower.pal[:] = sky.pal[64:]; lower.col[:] = sky.col[64:]
        tm, am = PT.tileize(lower, slot_pals, self.scen)
        for r in range(len(tm) - 1, -1, -1):
            self.rows.append((tm[r], am[r], slot_pals))

        def uniq(cv):
            tiles, attrs = [], []
            for ty in range(8):
                for tx in range(W):
                    sub = PT.Canvas(8, 8)
                    sub.pal[:] = cv.pal[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
                    sub.col[:] = cv.col[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
                    tmp = TileSet(0)
                    _, a = PT.tileize(sub, slot_pals, tmp, flip=False)
                    tiles.append(tmp.tiles[0]); attrs.append(a[0][0])
            return tiles, attrs
        bt, ba = uniq(base_cv)
        var = [(nm,) + uniq(cv) for nm, cv in variants]
        swap = sorted({i for _, vt, va in var for i in range(W * 8) if key(vt[i]) != key(bt[i])})
        # a swapped tile keeps one palette for every version: use the most common one
        for i in swap:
            pals = [ba[i] & 7] + [va[i] & 7 for _, vt, va in var]
            ba[i] = (ba[i] & ~7) | max(set(pals), key=pals.count)
        x0 = (20 - W) // 2
        sky_rows = []
        for ty in range(8):
            strip = PT.Canvas(160, 8); strip.pal[:] = sky.pal[ty * 8:ty * 8 + 8]; strip.col[:] = sky.col[ty * 8:ty * 8 + 8]
            tmr, amr = PT.tileize(strip, slot_pals, self.scen)
            trow, arow = [], []
            for tx in range(20):
                if x0 <= tx < x0 + W:
                    i = ty * W + (tx - x0)
                    trow.append(('B', BOSS_BASE + i)); arow.append(ba[i] | 0x08)
                else:
                    trow.append(tmr[0][tx]); arow.append(amr[0][tx])
            sky_rows.append((trow, arow, slot_pals))
        for r in range(7, -1, -1):
            self.rows.append(sky_rows[r])
        self.boss = dict(W=W, x0=x0, tiles=bt, attrs=ba, swap=swap, var=var, slot_pals=slot_pals, hit_pal=hit_pal)
        print(f"  S{self.n} boss {W}x8 tiles, {len(swap)} swap tiles, {len(var)} variants")

    # ------------------------------------------------------------ sprites (bank-1 OBJ set)
    def sprite(self, name, rows, w, h=16, pal=1):
        img = parse(norm(rows, w)[:h], w)
        img += [[0] * w for _ in range(h - len(img))]
        first = len(self.spr)
        for band in range(h // 16):
            for col in range(w // 8):
                top = [row[col * 8:col * 8 + 8] for row in img[band * 16:band * 16 + 8]]
                bot = [row[col * 8:col * 8 + 8] for row in img[band * 16 + 8:band * 16 + 16]]
                self.spr.append(top); self.spr.append(bot)
        self.out.define(f"{self.P}ST_{name}", first)
        self._sprite_art = getattr(self, '_sprite_art', []) + [(name, rows, w, h, pal)]
        return first

    def enemy_pals(self, bullet, p4, p5, p6):
        self.obj_pals = [bullet, p4, p5, p6]

    # ------------------------------------------------------------ emit
    def emit(self, music_stage, music_boss, logic_bank, boss_hit_name):
        o, P, db = self.out, self.P, self.dbank
        n0 = 256 - self.n_common
        ids = [(0, self.n_common + i) if i < n0 else (1, i - n0) for i in range(len(self.scen.tiles))]
        nb1 = max(0, len(self.scen.tiles) - n0)
        assert nb1 <= COMM_TILE, f"S{self.n}: scenery overflow ({nb1} bank-1 tiles)"
        assert len(self.spr) <= 128, f"S{self.n}: {len(self.spr)} sprite tiles"
        o.carray(db, f"s{self.n}_scen0", sum((encode(t) for t in self.scen.tiles[:n0]), []))
        o.carray(db, f"s{self.n}_scen1", sum((encode(t) for t in self.scen.tiles[n0:]), []))
        o.carray(db, f"s{self.n}_spr", sum((encode(t) for t in self.spr), []))
        o.carray(db, f"s{self.n}_objpal", sum((pal_words(p) for p in self.obj_pals), []), "uint16_t")
        init = [self.lib.add(self.init_slots.get(s, 'SPACE')) for s in range(8)]
        init[4] = self.lib.add('TEXT'); init[5] = self.lib.add('HUD')
        o.carray(db, f"s{self.n}_initbp", init)
        b = self.boss
        o.carray(db, f"s{self.n}_btiles", sum((encode(t) for t in b['tiles']), []))
        o.carray(db, f"s{self.n}_bswap", [BOSS_BASE + i for i in b['swap']])
        vdata = []
        for nm, vt, va in b['var']:
            for i in b['swap']:
                vdata += encode(vt[i])
        o.carray(db, f"s{self.n}_bvar", vdata)
        for k, (nm, _, _) in enumerate(b['var']):
            o.define(f"{P}BV_{nm.upper()}", k)
        o.define(f"{P}BOSS_W", b['W']); o.define(f"{P}BOSS_X0", b['x0'])
        # rows
        rb = []
        for (trow, arow, _) in self.rows:
            tb, ab = [], []
            for t, a in zip(trow, arow):
                if isinstance(t, tuple):
                    tb.append(t[1]); ab.append(a)
                else:
                    bank, tid = ids[t]
                    tb.append(tid); ab.append(a | (0x08 if bank else 0))
            rb.append(tb + ab)
        chunks = [rb[i:i + ROWS_PER_BANK] for i in range(0, len(rb), ROWS_PER_BANK)]
        assert len(chunks) <= len(self.row_banks), f"S{self.n}: needs {len(chunks)} row banks"
        for k, ch in enumerate(chunks):
            o.carray(self.row_banks[k], f"s{self.n}_rows{k}", sum(ch, []))
        o.define(f"{P}N_ROWS", len(self.rows))
        for k, v in self.sections.items():
            o.define(f"{P}ROW_{k.upper()}", v)
        o.define(f"{P}CP1", self.cps[1]); o.define(f"{P}CP2", self.cps[2])
        self.preview(ids)
        return dict(n=self.n, db=db, n_scen0=min(len(self.scen.tiles), n0), n_scen1=nb1, n_spr=len(self.spr),
                    chunks=[(self.row_banks[k], f"s{self.n}_rows{k}") for k in range(len(chunks))],
                    n_rows=len(self.rows), cps=self.cps, n_btiles=len(b['tiles']), n_swap=len(b['swap']),
                    mus=(music_stage, music_boss), logic=logic_bank, hit_bp=self.lib.add(boss_hit_name),
                    boss_bp6=init[6] if False else self.lib.add(b['slot_pals'][6]))

    def preview(self, ids):
        R = len(self.rows)
        im = Image.new('RGB', (160, R * 8))
        px = im.load()
        for r, (trow, arow, sp) in enumerate(self.rows):
            y = (R - 1 - r) * 8
            for tx, (t, a) in enumerate(zip(trow, arow)):
                if isinstance(t, tuple):
                    tile = self.boss['tiles'][t[1] - BOSS_BASE]
                else:
                    tile = self.scen.tiles[t]
                    if a & 0x20: tile = hflip_t(tile)
                    if a & 0x40: tile = vflip_t(tile)
                cols = [hx(c) for c in BG_PALS[sp[a & 7]]]
                for yy in range(8):
                    for xx in range(8):
                        px[tx * 8 + xx, y + yy] = cols[tile[yy][xx]]
        im.save(os.path.join(self.prev, f"s{self.n}_level.png"))
        # sprite sheet preview with the stage's OBJ palettes
        art = getattr(self, '_sprite_art', [])
        if art:
            W = sum(w + 2 for _, _, w, _, _ in art) + 2
            sh = Image.new('RGB', (max(W, 8), 34), (40, 40, 48))
            x = 2
            for nm, rows, w, h, pk in art:
                pal = self.obj_pals[pk]
                for yy, rr in enumerate(norm(rows, w)[:h]):
                    for xx, c in enumerate(rr):
                        if c in '123':
                            sh.putpixel((x + xx, 1 + yy), hx(pal[int(c)]))
                x += w + 2
            sh.resize((sh.width * 3, sh.height * 3), Image.NEAREST).save(os.path.join(self.prev, f"s{self.n}_sprites.png"))
