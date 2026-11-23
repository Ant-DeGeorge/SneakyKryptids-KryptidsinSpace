"""Arcade laps + arcade weapons check: one map, both laps, the boss. Forces each arcade weapon in turn,
measures the game-loop rate from the scroll (1 px per 2 loops) and sprites per scanline.
usage: arcade_laps.py rom mode seed [shots_dir]"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bot import Bot
rom, mode, seed = sys.argv[1], sys.argv[2], int(sys.argv[3])
shots = sys.argv[4] if len(sys.argv) > 4 else None
if shots: os.makedirs(shots, exist_ok=True)
b = Bot(rom, mode, 'pro', seed, None, False, stage=0); g = b.g
b.start = lambda: None
g.tick(760)
for k in range(3): g.s8('pf_unlock', 6, k)
g.press('start'); g.tick(30)
for k in range({'cadet': 0, 'pilot': 1, 'hunter': 2}[mode]): g.press('down'); g.tick(4)
g.press('a'); g.tick(40)
for k in range(8): g.press('down'); g.tick(4)
g.press('a'); g.tick(60); g.press('a'); g.tick(120)
if mode == 'hunter': g.hold('a'); b.held.add('a')
stats = dict(over10=0, frames=0, maxline=0)
seg = {}                     # lap -> [frames, pos advance]
last = [None]
shot_at = {}
orig = g.tick
def tick(n=1):
    for _ in range(n):
        orig(1)
        f = g.frame
        lap, pos, st = g.m8('arc_lap'), g.m16('pos'), g.m8('st')
        if st == 0 and g.m8('scroll_on') and not g.m8('mb_active'):
            if last[0] and last[0][0] == lap and pos >= last[0][1]:
                s = seg.setdefault((lap, g.m8('wtype')), [0, 0]); s[0] += 1; s[1] += pos - last[0][1]
            last[0] = (lap, pos)
        else: last[0] = None
        # force the arcade weapons: prism, helix, nova, ~13 s each, at full power
        W = [int(x) for x in os.environ.get('WSET', '3,4,5').split(',')]; w = W[(f // 800) % len(W)]
        if st != 3 and g.m8('wtype') != w: g.s8('wtype', w); g.s8('wlevel', 2)
        oam = [g.p.memory[0xFE00 + i * 4] for i in range(40)]
        lines = [0] * 144
        for y in oam:
            for yy in range(y - 16, y):
                if 0 <= yy < 144: lines[yy] += 1
        m = max(lines); stats['frames'] += 1; stats['maxline'] = max(stats['maxline'], m)
        if m > 10: stats['over10'] += 1
        key = (lap, g.m8('pal_tint'), w)
        if shots and key not in shot_at and pos % 64 == 40:
            shot_at[key] = f; g.shot(f"{shots}/lap{lap}_tint{key[1]}_w{w}.png")
g.tick = tick
b.maxframes = g.frame + 60 * 60 * 12
r = b.run()
for k in range(200): tick(1)
if shots: g.shot(f"{shots}/after.png")
print(dict(mode=mode, seed=seed, cleared=r['cleared'], minutes=r['minutes'], hits=r['hits'], lives=r['lives'], lap=g.m8('arc_lap'),
           over10_pct=round(100 * stats['over10'] / max(1, stats['frames']), 1), maxline=stats['maxline'],
           loop_rate={f'lap{k[0]}_w{k[1]}': round(2 * v[1] / v[0], 3) for k, v in seg.items() if v[0] > 300}))
