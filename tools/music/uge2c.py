#!/usr/bin/env python3
"""Convert a hUGETracker .uge song to GBDK C for hUGEDriver.

A port of hUGETracker's own "Export GBDK .c" (src/codegen.pas, RenderSongToGBDKC), so a song
edited in the tracker converts exactly as the tracker would export it.

  python3 tools/music/uge2c.py song.uge DESCRIPTOR out.c [--bank N]
"""
import sys, argparse, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import uge

NOTE_NAMES = [n + str(o) for o in range(3, 9) for n in ('C_', 'Cs', 'D_', 'Ds', 'E_', 'F_', 'Fs', 'G_', 'Gs', 'A_', 'As', 'B_')]
TYPE_PREFIX = {uge.SQUARE: 'itSquare', uge.WAVE: 'itWave', uge.NOISE: 'itNoise'}


def optimize(song):
    """OptimizeSong: every order entry points at the first pattern with identical content."""
    keys = list(song.patterns)
    packed = {k: b''.join(c.pack() for c in song.patterns[k]) for k in keys}
    first = {}
    for k in keys: first.setdefault(packed[k], k)
    orders = []
    for ch in song.orders:
        o = []
        for k in ch:
            if k not in song.patterns: song.patterns[k] = uge.blank_pattern(); packed[k] = b''.join(c.pack() for c in song.patterns[k]); first.setdefault(packed[k], k)
            o.append(first[packed[k]])
        orders.append(o)
    return orders


def used_stuff(song, orders):
    used, hi = set(), {0: -1, 1: -1, 2: -1, 3: -1}
    hi_wave = -1
    for ins in song.wave:
        if ins.sub_on:
            for c in ins.sub:
                if c.fx == 9: hi_wave = max(hi_wave, c.param)
    for ch in range(4):
        slot = 0 if ch < 2 else ch - 1          # 0 duty, 1 wave, 2 noise
        for k in orders[ch]:
            if k in used: continue
            used.add(k)
            for c in song.patterns[k]:
                if c.fx == 9 and ch == 2: hi_wave = max(hi_wave, c.param)
                if c.instr == 0: continue
                v = c.instr if 0 <= c.instr <= 15 else 0
                hi[slot] = max(hi[slot], v)
                if ch == 2: hi_wave = max(hi_wave, song.wave[c.instr - 1].waveform)
    return used, hi[0], hi[1], hi[2], hi_wave


def fx_str(c): return '0x%03X' % (((c.fx & 0xF) << 8) | (c.param & 0xFF))


def cell(c):
    n = NOTE_NAMES[c.note] if 0 <= c.note < 72 else '___'
    return f'DN({n},{c.instr if 0 <= c.instr <= 15 else 0},{fx_str(c)})'


def sub_cell(c, last):
    n = '___' if c.note == uge.NO_NOTE else str(c.note)
    j = 1 if last and c.vol == 0 else max(0, min(32, c.vol))
    return f'DN({n},{j},{fx_str(c)})'


def instr_bytes(i):
    if i.type == uge.SQUARE:
        nr10 = (i.sweep_shift & 7) | ((1 if i.sweep_incdec == 1 else 0) << 3) | ((i.sweep_time & 7) << 4)
        nr11 = (i.length & 63) | ((i.duty & 3) << 6)
        nr12 = (i.sweep_amt & 7) | ((1 if i.sweep_dir == 0 else 0) << 3) | ((i.vol & 15) << 4)
        nr14 = 0x80 | (0x40 if i.length_on else 0)
        return [nr10, nr11, nr12, nr14]
    if i.type == uge.WAVE:
        return [i.length & 0xFF, (i.out_level & 3) << 5, i.waveform, 0x80 | (0x40 if i.length_on else 0)]
    nr41 = i.length & 63
    nr42 = (i.sweep_amt & 7) | ((1 if i.sweep_dir == 0 else 0) << 3) | ((i.vol & 15) << 4)
    hm = nr41 | (0x40 if i.length_on else 0) | (0x80 if i.counter_step == 1 else 0)
    return [nr42, hm]


def render(song, name, bank=None):
    orders = optimize(song)
    used, hd, hw, hn, hwave = used_stuff(song, orders)
    L = []
    if bank is not None: L += [f'#pragma bank {bank}', '']
    L += ['// generated from a hUGETracker .uge by tools/music/uge2c.py', '#include "hUGEDriver.h"', '#include <stddef.h>', '']
    L += [f'static const unsigned char order_cnt = {max(len(o) for o in orders) * 2};', '']
    for k in song.patterns:
        if k in used:
            L.append(f'static const unsigned char P{k}[] = {{')
            L += [f'    {cell(c)},' for c in song.patterns[k]]
            L.append('};')
    L.append('')
    for bankname, ins_list, hi in (('duty', song.duty, hd), ('wave', song.wave, hw), ('noise', song.noise, hn)):
        for n, i in enumerate(ins_list, 1):
            if n <= hi and i.sub_on:
                L.append(f'static const unsigned char {TYPE_PREFIX[i.type]}SP{n}[] = {{')
                L += [f'    {sub_cell(i.sub[r], r == 31)},' for r in range(32)]
                L.append('};')
    for ch in range(4):
        L.append(f'static const unsigned char* const order{ch + 1}[] = {{' + ','.join(f'P{k}' for k in orders[ch]) + '};')
    L.append('')
    for bankname, ins_list, hi, ctype in (('duty', song.duty, hd, 'hUGEDutyInstr_t'), ('wave', song.wave, hw, 'hUGEWaveInstr_t'),
                                          ('noise', song.noise, hn, 'hUGENoiseInstr_t')):
        L.append(f'static const {ctype} {bankname}_instruments[] = {{')
        for n, i in enumerate(ins_list[:max(hi, 1)], 1):        # SDCC rejects an empty array: keep at least one
            b = [str(v) for v in instr_bytes(i)]
            sp = f'{TYPE_PREFIX[i.type]}SP{n}' if (i.sub_on and n <= hi) else '0'
            b.insert(len(b) - 1, sp)
            if i.type == uge.NOISE: b += ['0', '0']
            L.append('    {' + ','.join(b) + '},')
        L.append('};')
    L.append('')
    L.append('static const unsigned char waves[] = {')
    for w in song.waves[:max(hwave + 1, 1)]:
        L.append('    ' + ''.join(f'{(w[j] << 4) | w[j + 1]},' for j in range(0, 32, 2)))
    L.append('};')
    L.append('')
    if bank is not None: L.append(f'const void __at({bank}) __bank_{name};')
    L.append(f'const hUGESong_t {name} = {{{song.ticks_per_row}, &order_cnt, order1, order2, order3,order4, duty_instruments, '
             f'wave_instruments, noise_instruments, NULL, waves}};')
    return '\n'.join(L) + '\n'


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('uge'); ap.add_argument('name'); ap.add_argument('out'); ap.add_argument('--bank', type=int)
    a = ap.parse_args()
    open(a.out, 'w').write(render(uge.Song.load(a.uge), a.name, a.bank))
