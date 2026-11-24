#!/usr/bin/env python3
"""Run any test bot on the Japanese ROM and check the glyph cache every second:
no glyph left out (ja_ovf) and every cached glyph still intact in VRAM (no other loader wrote over it).
Usage: JASHOTS=dir ja_check.py BOT.py ROM args...   (shots every 20 s and on the first problem)"""
import sys, os, re, runpy
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
import harness
out = os.environ.get('JASHOTS', '/tmp/ja_check'); os.makedirs(out, exist_ok=True)
inc = open(os.path.join(HERE, '..', 'build', 'ja_src', 'ja_font.inc'), encoding='latin-1').read()
font = [bytes(int(x, 16) for x in m.group(1).split(',')) for m in re.finditer(r'^    ((?:0x[0-9A-F]{2},?){16}),', inc, re.M)]
free0 = [int(x) for x in re.search(r'ja_free0\[\] = \{([^}]*)\}', inc).group(1).split(',')]
nf1 = int(re.search(r'#define JA_NFREE1 (\d+)', inc).group(1))
st = {'bad': None, 'sus': set()}
def check(g):
    p = g.p
    if g.m8('ja_ovf'): return 'overflow'
    new = set()
    try: return check_slots(g, p, new)
    finally: st['sus'] = new
def check_slots(g, p, new):
    for s in range(len(free0) + nf1):
        v = g.m8('slot_g', s)
        if v == 0 or v >= 0xFE: continue
        if s < len(free0): vb, t = 0, free0[s]
        else: vb, t = 1, g.m8('slot_t1', s - len(free0))
        a = 0x9000 + t * 16 if t < 128 else 0x8000 + t * 16
        if bytes(p.memory[vb, a + i] for i in range(16)) != font[v - 1]:
            key = (s, v)               # the game may be between noting a slot and writing its tile:
            if key in st['sus']: return f'slot {s} (bank {vb} tile {t}) overwritten'
            new.add(key)               # only a mismatch still there a second later counts
orig = harness.Game.tick
def tick(self, n=1):
    for _ in range(n):
        orig(self, 1)
        if self.frame % 60 == 0 and 'ja_ovf' in self.sym and not st['bad']:
            e = check(self)
            if e: st['bad'] = e; self.shot(f"{out}/PROBLEM_{self.frame}.png"); print("PROBLEM at", self.frame, e)
        if self.frame % 1200 == 0: self.shot(f"{out}/f{self.frame:06d}.png")
harness.Game.tick = tick
sys.argv = sys.argv[1:]
runpy.run_path(sys.argv[0], run_name='__main__')
print('ja check:', st['bad'] or 'clean')
