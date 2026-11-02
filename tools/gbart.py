"""Tile helpers + 5x7 font shared with Spooky Claw."""
def blank(w, h, c=0):
    return [[c] * w for _ in range(h)]

def parse(rows, w=None):
    out = []
    for r in rows:
        if w is not None:
            assert len(r) == w, (len(r), r)
        out.append([0 if ch == '.' else int(ch) for ch in r])
    return out

def tiles_from(img, w_tiles, h_tiles, x0=0, y0=0):
    """Cut an image into 8x8 tiles, row-major."""
    t = []
    for ty in range(h_tiles):
        for tx in range(w_tiles):
            t.append([row[x0 + tx*8: x0 + tx*8 + 8] for row in img[y0 + ty*8: y0 + ty*8 + 8]])
    return t

def encode(tile):
    b = []
    for row in tile:
        lo = hi = 0
        for x, c in enumerate(row):
            if c & 1: lo |= 0x80 >> x
            if c & 2: hi |= 0x80 >> x
        b += [lo, hi]
    return b

def scale2(img):
    out = []
    for row in img:
        r2 = []
        for c in row: r2 += [c, c]
        out.append(r2); out.append(r2[:])
    return out

def meta16(img):  # 16x16 -> TL,TR,BL,BR
    return tiles_from(img, 2, 2)

def hflip_t(t): return [row[::-1] for row in t]
def vflip_t(t): return t[::-1]
def key(t): return tuple(tuple(r) for r in t)

class TileSet:
    """Deduplicating tile set (with CGB flip matching)."""
    def __init__(self, base):
        self.base = base; self.tiles = []; self.index = {}
    def add(self, t, allow_flip=True):
        k = key(t)
        if k in self.index: return self.base + self.index[k], 0
        if allow_flip:
            for flags, ft in ((0x20, hflip_t(t)), (0x40, vflip_t(t)), (0x60, vflip_t(hflip_t(t)))):
                fk = key(ft)
                if fk in self.index: return self.base + self.index[fk], flags
        self.index[k] = len(self.tiles); self.tiles.append(t)
        return self.base + len(self.tiles) - 1, 0

class Screen:
    def __init__(self, ts):
        self.ts = ts; self.map = [[0]*20 for _ in range(18)]; self.attr = [[0]*20 for _ in range(18)]
    def put(self, x, y, tile_img, pal, flip=True):
        idx, fl = self.ts.add(tile_img, flip)
        self.map[y][x] = idx; self.attr[y][x] = pal | fl
    def put_raw(self, x, y, idx, pal):
        self.map[y][x] = idx; self.attr[y][x] = pal
    def put_img(self, x, y, img, wt, ht, pal):
        for i, t in enumerate(tiles_from(img, wt, ht)):
            self.put(x + i % wt, y + i // wt, t, pal)

# ---------------------------------------------------------------- font (5x7)
FONT = {
'A':[".###.","#...#","#...#","#####","#...#","#...#","#...#"],
'B':["####.","#...#","#...#","####.","#...#","#...#","####."],
'C':[".###.","#...#","#....","#....","#....","#...#",".###."],
'D':["####.","#...#","#...#","#...#","#...#","#...#","####."],
'E':["#####","#....","#....","####.","#....","#....","#####"],
'F':["#####","#....","#....","####.","#....","#....","#...."],
'G':[".###.","#...#","#....","#.###","#...#","#...#",".####"],
'H':["#...#","#...#","#...#","#####","#...#","#...#","#...#"],
'I':[".###.","..#..","..#..","..#..","..#..","..#..",".###."],
'J':["..###","...#.","...#.","...#.","#..#.","#..#.",".##.."],
'K':["#...#","#..#.","#.#..","##...","#.#..","#..#.","#...#"],
'L':["#....","#....","#....","#....","#....","#....","#####"],
'M':["#...#","##.##","#.#.#","#.#.#","#...#","#...#","#...#"],
'N':["#...#","#...#","##..#","#.#.#","#..##","#...#","#...#"],
'O':[".###.","#...#","#...#","#...#","#...#","#...#",".###."],
'P':["####.","#...#","#...#","####.","#....","#....","#...."],
'Q':[".###.","#...#","#...#","#...#","#.#.#","#..#.",".##.#"],
'R':["####.","#...#","#...#","####.","#.#..","#..#.","#...#"],
'S':[".####","#....","#....",".###.","....#","....#","####."],
'T':["#####","..#..","..#..","..#..","..#..","..#..","..#.."],
'U':["#...#","#...#","#...#","#...#","#...#","#...#",".###."],
'V':["#...#","#...#","#...#","#...#","#...#",".#.#.","..#.."],
'W':["#...#","#...#","#...#","#.#.#","#.#.#","#.#.#",".#.#."],
'X':["#...#","#...#",".#.#.","..#..",".#.#.","#...#","#...#"],
'Y':["#...#","#...#",".#.#.","..#..","..#..","..#..","..#.."],
'Z':["#####","....#","...#.","..#..",".#...","#....","#####"],
'0':[".###.","#...#","#..##","#.#.#","##..#","#...#",".###."],
'1':["..#..",".##..","..#..","..#..","..#..","..#..",".###."],
'2':[".###.","#...#","....#","...#.","..#..",".#...","#####"],
'3':["#####","...#.","..#..","...#.","....#","#...#",".###."],
'4':["...#.","..##.",".#.#.","#..#.","#####","...#.","...#."],
'5':["#####","#....","####.","....#","....#","#...#",".###."],
'6':["..##.",".#...","#....","####.","#...#","#...#",".###."],
'7':["#####","....#","...#.","..#..",".#...",".#...",".#..."],
'8':[".###.","#...#","#...#",".###.","#...#","#...#",".###."],
'9':[".###.","#...#","#...#",".####","....#","...#.",".##.."],
'!':["..#..","..#..","..#..","..#..","..#..",".....","..#.."],
'?':[".###.","#...#","....#","...#.","..#..",".....","..#.."],
'.':[".....",".....",".....",".....",".....",".##..",".##.."],
'-':[".....",".....",".....","#####",".....",".....","....."],
'x':[".....",".....","#...#",".#.#.","..#..",".#.#.","#...#"],
}
FONT_ORDER = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!?.-x"

def font_tile(ch):
    g = FONT[ch]; t = blank(8, 8)
    on = set()
    for y, row in enumerate(g):
        for x, c in enumerate(row):
            if c == '#': on.add((x, y)); on.add((x + 1, y))   # bold horizontally
    for (x, y) in on:
        if (x + 1, y + 1) not in on and x + 1 < 8 and y + 1 < 8: t[y+1][x+1] = 1
    for (x, y) in on: t[y][x] = 3
    return t

