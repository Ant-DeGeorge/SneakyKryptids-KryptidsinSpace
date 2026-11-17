#!/usr/bin/env python3
"""One-shot: lengthen the mission 1-4 wave scripts for the slower scroll (v1.6).
The scroll went from 30 to 20 px/s, so each mission lasts 1.5x as long. This adds about half as
many waves again so the action stays as dense: new formations dropped into the gaps between the
old ones, plus one hazard run per mission. Rewrites the script[] tables in src/s1..s4.c in place.
Run once; the result is committed source."""
import re, random, sys, os
SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src')
# per mission: commands that spawn things, the hazard command, the section the hazard run goes in
CFG = {
 's1': dict(spawn=['C_KLLINE', 'C_KLV', 'C_KLSWOOP', 'C_JELLY', 'C_SKIFF', 'C_ICE'], hz=[('C_ICE', 40), ('C_ICE', 120), ('C_ICE', 80), ('C_ICE', 56), ('C_ICE', 104)], sect='S1_ROW_CONVOY0'),
 's2': dict(spawn=['C_MOTHS', 'C_MOTHSIDE', 'C_EEL', 'C_CLOUD'], hz=[('C_CLOUD', 40), ('C_CLOUD', 120), ('C_CLOUD', 80), ('C_CLOUD', 64), ('C_CLOUD', 104)], sect='RL'),
 's3': dict(spawn=['C_IMPS', 'C_AMBUSH', 'C_CONES', 'C_WISP'], hz=[('C_WISP', 48), ('C_WISP', 112), ('C_WISP', 80), ('C_WISP', 64), ('C_WISP', 104)], sect='BG'),
 's4': dict(spawn=['C_TURRET', 'C_BATS', 'C_SNAKE'], hz=[('C_TURRET', 40), ('C_TURRET', 120), ('C_TURRET', 80), ('C_TURRET', 56), ('C_TURRET', 104)], sect='MI'),
}
ENT = re.compile(r'\{\s*(RAT\(\s*(\w+)\s*,\s*(-?\d+)\s*\)|[^,{}]+?)\s*,\s*(C_\w+)(\s*\|\s*H)?\s*,\s*([^,{}]+?)\s*,\s*([^,{}]+?)\s*\}')
def run(name):
    cfg = CFG[name]; rng = random.Random(name)
    p = os.path.join(SRC, name + '.c'); s = open(p).read()
    a = s.index('static const Cmd script[] = {'); b = s.index('\n};', a)
    body = s[a:b]
    if 'v1.6 wave' in body: print(name, 'already extended'); return
    ents = [dict(text=m.group(0), base=m.group(2), off=int(m.group(3)) if m.group(3) else None, kind=m.group(4), h=bool(m.group(5)), a=m.group(6), b=m.group(7)) for m in ENT.finditer(body)]
    pool = [(e['kind'], e['a'], e['b']) for e in ents if e['kind'] in cfg['spawn'] and not e['h'] and (name != 's3' or e['kind'] != 'C_WISP')]
    n_spawn = sum(e['kind'] in cfg['spawn'] for e in ents)
    gaps = [i for i in range(len(ents) - 1) if ents[i]['base'] and ents[i]['base'] == ents[i + 1]['base'] and ents[i + 1]['off'] - ents[i]['off'] >= 6]
    hz_gaps = [i for i in range(len(ents) - 1) if ents[i]['base'] == cfg['sect'] and ents[i + 1]['base'] == cfg['sect'] and ents[i + 1]['off'] - ents[i]['off'] >= 4][:5]
    want = max(0, n_spawn // 2 - len(hz_gaps))
    rest = [g for g in gaps if g not in hz_gaps]
    step = max(1.0, len(rest) / max(1, want))
    pick = sorted(set(rest[int(k * step)] for k in range(min(want, len(rest)))))
    ins = {}
    for k, g in enumerate(hz_gaps):
        mid = (ents[g]['off'] + ents[g + 1]['off']) // 2
        ins[g] = '{RAT(%s, %d), %s, %d, 0}' % (cfg['sect'], mid, cfg['hz'][k % len(cfg['hz'])][0], cfg['hz'][k % len(cfg['hz'])][1])
    for g in pick:
        near = {ents[g]['kind'], ents[g + 1]['kind']}
        cand = [t for t in pool if t[0] not in near] or pool
        kind, pa, pb = rng.choice(cand)
        mid = (ents[g]['off'] + ents[g + 1]['off']) // 2
        ins[g] = '{RAT(%s, %d), %s, %s, %s}' % (ents[g]['base'], mid, kind, pa, pb)
    out, pos = [], 0
    for i, m in enumerate(ENT.finditer(body)):
        out.append(body[pos:m.end()]); pos = m.end()
        if i in ins: out.append(', /* v1.6 wave */ ' + ins[i])
    out.append(body[pos:])
    open(p, 'w').write(s[:a] + ''.join(out) + s[b:])
    print(name, 'spawns', n_spawn, '+', len(ins), '(hazard run %d)' % len(hz_gaps))
for n in sys.argv[1:] or CFG: run(n)
