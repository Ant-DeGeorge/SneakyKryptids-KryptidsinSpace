"""A small score language that builds hUGETracker songs (.uge, via uge.py).

Channel text is a list of tokens, each taking one or more rows:
  D5:4        note D5 for 4 rows (scientific pitch, A4 = 440 Hz; flats/sharps: Bb4, C#5)
  R:2         rest: cut the note, then 2 empty rows
  -:4         hold whatever is sounding for 4 rows
  @3          switch to instrument 3 (no row)
  D5:8/437    note with effect 4 37 (vibrato). Arpeggio (0xy) and vibrato (4xy) repeat on every
              row of the note; other effects go on its first row only.
  |           bar line (ignored; checked: every bar must be 16 rows when bars=16 is given)
Drum text (channel 4) uses one letter per row: k kick, s snare, h hat, o open hat, . empty.
"""
import re
import uge

NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
FLAT = {'Db': 'C#', 'Eb': 'D#', 'Gb': 'F#', 'Ab': 'G#', 'Bb': 'A#', 'Cb': 'B', 'Fb': 'E'}


def midi(n):
    m = re.fullmatch(r"([A-G][#b]?)(-?\d)", n)
    if not m: raise ValueError(f'bad note {n!r}')
    name = FLAT.get(m.group(1), m.group(1))
    return NAMES.index(name) + (int(m.group(2)) + 1) * 12


def hnote(n, ch):
    """hUGE note number: squares play C_3 (0) at scientific C2; the wave channel an octave lower."""
    v = midi(n) - (24 if ch == 3 else 36)
    if not 0 <= v < 72: raise ValueError(f'{n} is out of range on channel {ch}')
    return v


DRUM_DEFAULT = {'k': (1, 22), 's': (2, 41), 'h': (3, 60), 'o': (4, 57), 'r': (5, 47)}


class Score:
    def __init__(self, name, tpr):
        self.song = uge.Song()
        self.song.name = name
        self.song.artist = 'Rejected By The Machines'
        self.song.ticks_per_row = tpr
        self.rows = {1: [], 2: [], 3: [], 4: []}
        self.drums = dict(DRUM_DEFAULT)
        self.marks = {}

    # ---------------------------------------------------------------- instruments
    def duty(self, slot, name, duty=2, vol=12, decay=0, up=False, sweep=(0, 0, 0)):
        """decay: envelope pace 0 (sustain) .. 7 (slowest fade). sweep: (time, down?, shift) on ch1 only."""
        i = self.song.duty[slot - 1]
        i.name, i.duty, i.vol, i.sweep_amt, i.sweep_dir = name, duty, vol, decay, 0 if up else 1
        i.sweep_time, i.sweep_incdec, i.sweep_shift = sweep[0], 1 if sweep[1] else 0, sweep[2]

    def wave(self, slot, name, waveform, level=1):
        """level: 1 full, 2 half, 3 quarter volume."""
        i = self.song.wave[slot - 1]
        i.name, i.waveform, i.out_level = name, waveform, level

    def noise(self, slot, name, vol=12, decay=1, metallic=False):
        i = self.song.noise[slot - 1]
        i.name, i.vol, i.sweep_amt, i.sweep_dir, i.counter_step = name, vol, decay, 1, 1 if metallic else 0

    def waveform(self, idx, nibbles):
        assert len(nibbles) == 32 and all(0 <= v <= 15 for v in nibbles)
        self.song.waves[idx] = list(nibbles)

    # ---------------------------------------------------------------- music
    def mark(self, label):
        """Remember the current row (all channels must be level) as a loop point."""
        n = {len(r) for r in self.rows.values()}
        assert len(n) == 1, f'channels out of step at mark {label}: {[(c, len(r)) for c, r in self.rows.items()]}'
        self.marks[label] = n.pop()

    def play(self, ch, text, bars=16):
        rows = self.rows[ch]
        instr = getattr(self, f'_ins{ch}', 1)
        bar_start = len(rows)
        for tok in text.split():
            if tok == '|':
                if bars and (len(rows) - bar_start) % bars: raise ValueError(f'ch{ch}: bar of {len(rows) - bar_start} rows before row {len(rows)}: ...{text[:60]}')
                bar_start = len(rows); continue
            if tok.startswith('@'):
                instr = int(tok[1:]); continue
            m = re.fullmatch(r'([^:/]+)(?::(\d+))?(?:/([0-9A-Fa-f]{3}))?', tok)
            if not m: raise ValueError(f'bad token {tok!r}')
            n, d, fx = m.group(1), int(m.group(2) or 1), m.group(3)
            fxc, fxp = (int(fx[0], 16), int(fx[1:], 16)) if fx else (0, 0)
            first = uge.Cell()
            if n == 'R':
                first.fx, first.param = 0xE, 0
            elif n != '-':
                first.note, first.instr = hnote(n, ch), instr
            if fx: first.fx, first.param = fxc, fxp
            rows.append(first)
            for _ in range(d - 1):
                c = uge.Cell()
                if fx and fxc in (0x0, 0x4): c.fx, c.param = fxc, fxp
                rows.append(c)
        if bars and (len(rows) - bar_start) % bars: raise ValueError(f'ch{ch}: last bar has {len(rows) - bar_start} rows')
        setattr(self, f'_ins{ch}', instr)

    def drum(self, text):
        rows = self.rows[4]
        if len(text.replace(' ', '').replace('|', '')) % 16:
            raise ValueError(f'drum part is {len(text.replace(" ", ""))} rows, not whole bars: {text[:40]}...')
        for ch in text:
            if ch in ' |': continue
            c = uge.Cell()
            if ch != '.':
                ins, note = self.drums[ch]
                c.note, c.instr = note, ins
            rows.append(c)

    def raw(self, ch, cells):
        self.rows[ch] += cells

    def silence(self, ch, n):
        self.rows[ch] += [uge.Cell() for _ in range(n)]

    def level(self):
        """Pad every channel with empty rows to the longest one."""
        n = max(len(r) for r in self.rows.values())
        for r in self.rows.values(): r += [uge.Cell() for _ in range(n - len(r))]

    # ---------------------------------------------------------------- build
    def build(self, loop_to=None, end_silent=False):
        """Cut the rows into 64-row patterns. loop_to: a mark name or row (multiple of 64) the song
        jumps back to at the end (default: the start). end_silent: play once, then hold silence.
        The jump sits on the last row of music, so a song need not fill its last pattern."""
        self.level()
        n = len(self.rows[1])                            # rows of music
        if n % 64:
            for r in self.rows.values(): r += [uge.Cell() for _ in range(64 - n % 64)]
        n_orders = len(self.rows[1]) // 64
        target = self.marks[loop_to] if isinstance(loop_to, str) else (loop_to or 0)
        assert target % 64 == 0, f'loop point {loop_to} is row {target}, not the start of a pattern'
        target //= 64
        if end_silent:
            for c in (1, 2, 3, 4):
                sil = uge.blank_pattern()
                sil[0].fx, sil[0].param = 0xE, 0
                self.rows[c] += sil
            target = n_orders                            # the silent pattern loops on itself
            n_orders += 1
            last_rows = [n - 1, n_orders * 64 - 1]
        else:
            last_rows = [n - 1]
        if not (target == 0 and not end_silent and n % 64 == 0):
            for row in last_rows:
                tgt = target
                for c in (4, 3, 2, 1):
                    cell = self.rows[c][row]
                    if cell.fx == 0 and cell.param == 0:
                        cell.fx, cell.param = 0xB, tgt + 1          # hUGEDriver: B01 = first order (B00 = next order)
                        break
                else:
                    raise ValueError(f'no free effect slot on row {row} for the loop jump')
        s = self.song
        s.patterns, s.orders = {}, [[], [], [], []]
        seen = {}
        for c in (1, 2, 3, 4):
            r = self.rows[c]
            for o in range(n_orders):
                pat = r[o * 64:(o + 1) * 64]
                key = b''.join(x.pack() for x in pat)
                if key not in seen:
                    seen[key] = len(seen)
                    s.patterns[seen[key]] = [x.copy() for x in pat]
                s.orders[c - 1].append(seen[key])
        return s
