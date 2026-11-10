#!/usr/bin/env python3
"""Playtest bot for mission 5 (the gun-cam stage). Reads the cockpit object pool from RAM.

  pro - picks the most urgent target every frame, bombs when an orb is about to land
  kid - re-decides every ~12 frames, often picks a random target, wobbly aim, random bombs
Usage: bot_cockpit.py ROM cadet|pilot|hunter pro|kid [seed] [--boss] [--shots DIR]
"""
import sys, os, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bot import Bot

def s16(v): return v - 65536 if v >= 32768 else v
def s8(v): return v - 256 if v >= 128 else v
ORB, BIG, CAPS = 4, 5, 6

class CockpitBot(Bot):
    def cstate(self):
        g = self.g
        S = dict(st=g.m8('st'), cx=g.m8('ck_cx'), cy=g.m8('ck_cy'), kstate=g.m8('k_state'), khp=g.m16('k_hp'),
                 bx=s8(g.m8('ck_bx')), by=s8(g.m8('ck_by')), charge=g.m8('ck_charge'), lives=g.m8('lives'),
                 hearts=g.m8('hearts'), bombs=g.m8('bombs'), hits=g.m16('hits_taken'), score=g.m32('score'), tick=g.m8('ck_tick'))
        o = []
        if not hasattr(self, 'age'): self.age = [0] * 12; self.aty = [0] * 12
        react = 14 if self.profile == 'pro' else 34      # frames before a new object is noticed
        for i in range(12):
            t = g.m8('ck_type', i)
            self.age[i] = self.age[i] + 1 if t == self.aty[i] else 0
            self.aty[i] = t
            if t in (0, 7) or self.age[i] < react: continue
            o.append(dict(t=t, x=g.m8('ck_sx', i) - 48, y=g.m8('ck_sy', i) - 48, z=g.m8('ck_z', i), b=g.m8('ck_b', i)))   # view coords are biased by 48
        S['obj'] = o
        return S

    def pick(self, S):
        pro = self.profile == 'pro'
        vis = [o for o in S['obj'] if 6 <= o['x'] <= 154 and 10 <= o['y'] <= 92]
        boss = (80 + S['bx'] - 1, 48 + S['by'] - 4) if S['kstate'] == 2 else None
        if not pro and self.rng.random() < 0.35:
            c = [(o['x'], o['y']) for o in vis] + ([boss] if boss else [])
            return self.rng.choice(c) if c else None
        def urgency(o):
            if o['t'] in (ORB, BIG): return o['z']                       # frames until it lands
            if o['t'] == 3: return (o['z'] - 26) * (1 if o['b'] & 3 >= 2 else 2) + (0 if o['b'] & 0x10 else 400)
            if o['t'] == CAPS: return 150
            return 200 + o['z']
        vis.sort(key=urgency)
        if boss and S['charge'] and not (vis and urgency(vis[0]) < 45): return boss
        if vis and (urgency(vis[0]) < 600 or not boss): return (vis[0]['x'], vis[0]['y'])
        return boss

    def run(self):
        g = self.g
        self.start()
        dec_every = 6 if self.profile == 'pro' else self.rng.choice([12, 14, 16])
        tgt = None; jx = jy = 0; shot_i = 0; boss_f0 = None; last_hits = 0; events = []
        while g.frame < self.maxframes:
            S = self.cstate()
            if S['st'] == 3: break
            if g.frame % dec_every == 0:
                tgt = self.pick(S)
                jx, jy = (self.rng.randint(-2, 2), self.rng.randint(-2, 2)) if self.profile == 'pro' else (self.rng.randint(-6, 6), self.rng.randint(-5, 5))
                bomb = False
                if self.profile == 'pro':
                    if S['bombs'] and any(o['t'] in (ORB, BIG) and o['z'] < 10 for o in S['obj']) and self.mode == 'hunter': bomb = True
                    if S['bombs'] and S['charge'] and S['charge'] < 12: bomb = True
                elif self.rng.random() < 0.02: bomb = True
                if bomb: g.hold('b'); g.tick(2); g.release('b')
            d = (0, 0)
            if tgt:
                dx, dy = tgt[0] + jx - S['cx'], tgt[1] + jy - S['cy']
                d = ((dx > 2) - (dx < -2), (dy > 2) - (dy < -2))
            self.set_dir(d)
            if S['kstate'] and boss_f0 is None: boss_f0 = g.frame
            if S['hits'] != last_hits: events.append((S['tick'], S['kstate'])); last_hits = S['hits']
            if self.mode == 'hunter' and S['st'] == 1 and S['lives'] == 1:
                g.tick(75 + 105)
                g.release('a'); g.tick(5); g.press('a'); g.tick(5); g.hold('a')
                self.continues += 1
                if self.continues > 6: break
                continue
            if self.shots and g.frame % 300 == 0:
                g.shot(f"{self.shots}/{shot_i:03d}.png"); shot_i += 1
            g.tick(1)
        S = self.cstate()
        return dict(mode=self.mode, profile=self.profile, cleared=S['st'] == 3, minutes=round(g.frame / 3600, 2), hits=S['hits'],
                    continues=self.continues, score=S['score'], boss_seconds=round((g.frame - boss_f0) / 60, 1) if boss_f0 else None,
                    lives=S['lives'], khp=S['khp'], stage=5, bounty=g.m8('bounty_out'), bpts=g.m32('bounty_pts'), hit_at=events[:30])

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("rom"); ap.add_argument("mode"); ap.add_argument("profile")
    ap.add_argument("seed", nargs="?", type=int, default=1)
    ap.add_argument("--shots"); ap.add_argument("--boss", action="store_true")
    a = ap.parse_args()
    if a.shots: os.makedirs(a.shots, exist_ok=True)
    print(CockpitBot(a.rom, a.mode, a.profile, a.seed, a.shots, a.boss, stage=4).run())
