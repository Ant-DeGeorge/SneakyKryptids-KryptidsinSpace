"""Read and write hUGETracker .uge files (format version 6, hUGETracker 1.0.x).

The format is FreePascal packed records written raw (see hUGETracker src/song.pas):
Integer = int32, enums = int32, Boolean and small subranges = 1 byte, ShortString = 1 length
byte + 255 chars. A pattern is 64 cells of (note, instrument, volume, effect code: int32, params: byte).
"""
import struct

NO_NOTE = 90
SQUARE, WAVE, NOISE = 0, 1, 2


class PStr(str):
    """A ShortString that remembers its 256 raw bytes (Pascal leaves junk after the text),
    so an unchanged string is written back byte for byte."""
    raw = None


def _sstr(b, o):
    n = b[o]
    v = PStr(b[o + 1:o + 1 + n].decode('latin-1')); v.raw = b[o:o + 256]
    return v, o + 256


def _wsstr(s):
    if isinstance(s, PStr) and s.raw is not None: return s.raw
    e = s.encode('latin-1')[:255]
    return bytes([len(e)]) + e + bytes(255 - len(e))


class Cell:
    __slots__ = ('note', 'instr', 'vol', 'fx', 'param')
    def __init__(self, note=NO_NOTE, instr=0, vol=0, fx=0, param=0):
        self.note, self.instr, self.vol, self.fx, self.param = note, instr, vol, fx, param
    def pack(self):
        return struct.pack('<iiiiB', self.note, self.instr, self.vol, self.fx, self.param)
    def copy(self):
        return Cell(self.note, self.instr, self.vol, self.fx, self.param)
    def __eq__(self, o): return self.pack() == o.pack()


def blank_pattern(n=64):
    return [Cell() for _ in range(n)]


def _read_pattern(b, o):
    p = []
    for _ in range(64):
        p.append(Cell(*struct.unpack_from('<iiiiB', b, o))); o += 17
    return p, o


class Instrument:
    """One of the 45 instrument slots (15 duty, 15 wave, 15 noise)."""
    FIELDS = 'type name length length_on vol sweep_dir sweep_amt sweep_time sweep_incdec sweep_shift duty out_level waveform counter_step sub_on'.split()
    def __init__(self, type_=SQUARE, **kw):
        self.type = type_; self.name = ''; self.length = 0; self.length_on = False
        self.vol = 15; self.sweep_dir = 1; self.sweep_amt = 0          # sweep_dir: 0 up, 1 down (TSweepType stUp, stDown)
        self.sweep_time = 0; self.sweep_incdec = 1; self.sweep_shift = 0
        self.duty = 2; self.out_level = 1; self.waveform = 0; self.counter_step = 0; self.sub_on = False
        self.sub = blank_pattern()
        for k, v in kw.items(): setattr(self, k, v)

    @staticmethod
    def read(b, o):
        i = Instrument()
        i.type, = struct.unpack_from('<i', b, o); o += 4
        i.name, o = _sstr(b, o)
        i.length, i.length_on, i.vol, i.sweep_dir, i.sweep_amt, i.sweep_time, i.sweep_incdec, i.sweep_shift, i.duty, \
            i.out_level, i.waveform, i.counter_step, i.sub_on = struct.unpack_from('<i?BiBiiiBiii?', b, o)
        o += struct.calcsize('<i?BiBiiiBiii?')
        i.sub, o = _read_pattern(b, o)
        return i, o

    def pack(self):
        return struct.pack('<i', self.type) + _wsstr(self.name) + struct.pack(
            '<i?BiBiiiBiii?', self.length, self.length_on, self.vol, self.sweep_dir, self.sweep_amt, self.sweep_time,
            self.sweep_incdec, self.sweep_shift, self.duty, self.out_level, self.waveform, self.counter_step, self.sub_on) + \
            b''.join(c.pack() for c in self.sub)


class Song:
    def __init__(self):
        self.version = 6
        self.name = self.artist = self.comment = ''
        self.duty = [Instrument(SQUARE) for _ in range(15)]
        self.wave = [Instrument(WAVE, waveform=i) for i in range(15)]
        self.noise = [Instrument(NOISE) for _ in range(15)]
        self.waves = [[0] * 32 for _ in range(16)]
        self.ticks_per_row = 7
        self.timer_on = False; self.timer_div = 0
        self.patterns = {}              # key -> list of 64 Cells
        self.orders = [[], [], [], []]  # pattern keys per channel (without the tracker's trailing dummy)
        self.routines = [''] * 16

    @staticmethod
    def load(path):
        b = open(path, 'rb').read()
        s = Song(); o = 0
        s.version, = struct.unpack_from('<i', b, o); o += 4
        if s.version != 6: raise ValueError(f'{path}: .uge version {s.version}; open and re-save it in hUGETracker 1.0+')
        s.name, o = _sstr(b, o); s.artist, o = _sstr(b, o); s.comment, o = _sstr(b, o)
        ins = []
        for _ in range(45):
            i, o = Instrument.read(b, o); ins.append(i)
        s.duty, s.wave, s.noise = ins[:15], ins[15:30], ins[30:]
        s.waves = []
        for _ in range(16):
            s.waves.append(list(b[o:o + 32])); o += 32
        s.ticks_per_row, s.timer_on, s.timer_div = struct.unpack_from('<i?i', b, o); o += 9
        n, = struct.unpack_from('<i', b, o); o += 4
        for _ in range(n):
            k, = struct.unpack_from('<i', b, o); o += 4
            s.patterns[k], o = _read_pattern(b, o)
        for c in range(4):
            n, = struct.unpack_from('<i', b, o); o += 4
            s.orders[c] = list(struct.unpack_from(f'<{n}i', b, o))[:-1]; o += 4 * n    # last entry is the tracker's off-by-one dummy
        for r in range(16):
            n, = struct.unpack_from('<i', b, o); o += 4
            s.routines[r] = b[o:o + n].decode('latin-1'); o += n
        s._tail = b[o:]
        return s

    def save(self, path):
        out = [struct.pack('<i', 6), _wsstr(self.name), _wsstr(self.artist), _wsstr(self.comment)]
        out += [i.pack() for i in self.duty + self.wave + self.noise]
        out += [bytes(w) for w in self.waves]
        out.append(struct.pack('<i?i', self.ticks_per_row, self.timer_on, self.timer_div))
        out.append(struct.pack('<i', len(self.patterns)))
        for k in self.patterns:                 # file order (the tracker does not sort)
            p = self.patterns[k]; assert len(p) == 64
            out.append(struct.pack('<i', k) + b''.join(c.pack() for c in p))
        for c in range(4):
            o = list(self.orders[c]) + [0]
            out.append(struct.pack(f'<{len(o) + 1}i', len(o), *o))
        for r in self.routines:
            e = r.encode('latin-1'); out.append(struct.pack('<i', len(e)) + e)
        open(path, 'wb').write(b''.join(out))
