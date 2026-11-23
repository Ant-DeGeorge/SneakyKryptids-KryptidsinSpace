"""Arcade test: pick a ship, fly N stages of the endless run with the stage bot.
usage: bot_arcade.py rom mode profile seed ship [stages] [--shots dir]"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bot import Bot
rom, mode, prof, seed, ship = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5])
stages = int(sys.argv[6]) if len(sys.argv) > 6 and not sys.argv[6].startswith('--') else 2
shots = sys.argv[sys.argv.index('--shots') + 1] if '--shots' in sys.argv else None
if shots: os.makedirs(shots, exist_ok=True)
b = Bot(rom, mode, prof, seed, None, False, stage=0); g = b.g
b.start = lambda: None                                   # the menus are driven here, not by Bot.start
g.tick(760)
for k in range(3): g.s8('pf_unlock', 6, k)
g.press('start'); g.tick(30)
for k in range({'cadet': 0, 'pilot': 1, 'hunter': 2}[mode]): g.press('down'); g.tick(4)
g.press('a'); g.tick(40)
for k in range(8): g.press('down'); g.tick(4)            # past the last mission: Arcade
if shots: g.tick(10); g.shot(f'{shots}/map_arcade.png')
g.press('a'); g.tick(60)
for k in range(ship): g.press('right'); g.tick(12)
if shots: g.tick(20); g.shot(f'{shots}/select_{ship}.png')
g.press('a'); g.tick(40)
out = []
for n in range(stages):
    if shots: g.shot(f'{shots}/card_{n}.png')
    g.tick(120)                                          # maps follow each other with no card
    if mode == 'hunter': g.hold('a'); b.held.add('a')
    f0 = g.frame; b.maxframes = g.frame + 60 * 60 * 9
    if shots: g.tick(400); g.shot(f'{shots}/stage_{n}.png')
    r = b.run()
    out.append(dict(stage=n + 1, cleared=r['cleared'], minutes=round((g.frame - f0) / 3600, 2), hits=r['hits'], lives=g.m8('lives'), score=r['score'], wtype=g.m8('wtype'), wlevel=g.m8('wlevel')))
    if not r['cleared']: break
    for k in b.held.copy(): g.release(k)
    b.held.clear()
    g.tick(200)                                          # ST_CLEAR fly-off, then the next map loads
if not out[-1]['cleared']:
    g.tick(400)
    if shots: g.shot(f'{shots}/over.png')
print(dict(mode=mode, ship=ship, runs=out))
