#!/usr/bin/env python3
"""Playtest bot for Kryptids in Space: The Sneaky Kryptid.

Plays one stage (title -> pilot -> map -> stage -> boss) by reading game RAM.
Profiles:
  pro  - looks 20 frames ahead every frame, chases pickups, bombs when cornered
  kid  - decides every ~12 frames, 30% random moves, short lookahead, mashes B
Usage: bot.py ROM cadet|pilot|hunter pro|kid [seed] [--stage N] [--shots DIR] [--boss]
"""
import sys, os, random, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from harness import Game

XO, YO = 16, 32
DIRS = [(0, 0), (-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (1, -1), (-1, 1), (1, 1)]
BTN = {(-1, 0): ['left'], (1, 0): ['right'], (0, -1): ['up'], (0, 1): ['down'],
       (-1, -1): ['left', 'up'], (1, -1): ['right', 'up'], (-1, 1): ['left', 'down'], (1, 1): ['right', 'down'], (0, 0): []}

def s16(v): return v - 65536 if v >= 32768 else v

class Bot:
    def __init__(self, rom, mode, profile, seed, shots=None, boss=False, maxframes=60 * 60 * 12, stage=0):
        self.g = Game(rom)
        self.mode, self.profile, self.rng = mode, profile, random.Random(seed)
        self.shots, self.boss, self.maxframes = shots, boss, maxframes
        self.held = set()
        self.cur = (0, 0)
        self.log = []
        self.continues = 0
        self.stats = {}
        self.stage = stage
        self.buddies = 0

    # ---------------------------------------------------------------- RAM views
    def state(self):
        g = self.g
        S = {}
        S['st'] = g.m8('st'); S['pos'] = g.m16('pos')
        S['px'] = g.m8('px'); S['py'] = g.m8('py')
        S['warn_t'] = g.m8('warn_t'); S['warn_x'] = g.m8('warn_x')
        b = []
        for i in range(14):
            if g.m8('eb_on', i) == 1:
                x = g.m16('eb_x', i * 2) / 256; y = g.m16('eb_y', i * 2) / 256
                vx = s16(g.m16('eb_vx', i * 2)) / 256; vy = s16(g.m16('eb_vy', i * 2)) / 256
                big = g.m8('eb_big', i)
                r = 4 if big else 3
                b.append((x + r, y + r, vx, vy, r))
        S['bul'] = b
        e, slp = [], []
        for i in range(8):
            t = g.m8('e_type', i)
            if t in (0, 1): continue
            x = g.m16('e_x', i * 2) / 256; y = g.m16('e_y', i * 2) / 256
            if t == 2: slp.append((x + 8, y + 8)); continue
            if g.m8('e_fl', i) & 1: continue
            if t in (7, 11, 15, 19): e.append((x + 16, y + 12, 0, 0.3, 15, t))
            else: e.append((x + 8, y + 8, 0, 0.7, 8, t))
            if t in (9, 18):
                c = g.m8('e_c', i)
                if c < 2:
                    for k in range(5):
                        e.append((g.m8('ch_x', c * 5 + k), g.m8('ch_y', c * 5 + k), 0, 0, 5, t))
        S['ene'] = e; S['slp'] = slp
        S['khp'] = g.m16('k_hp'); S['khpmax'] = g.m16('k_hpmax'); S['kstate'] = g.m8('k_state')
        S['sway'] = g.m8('k_sway'); S['sway'] = S['sway'] - 256 if S['sway'] > 127 else S['sway']
        S['hdx'] = g.m8('k_hdx'); S['hw'] = g.m8('k_hw'); S['hy1'] = g.m8('k_hy1'); S['ret'] = g.m8('k_retreat')
        tn = []
        if S['khpmax']:
            for i in range(g.m8('bp_n')):
                if g.m8('bp_hurt', i): tn.append((g.m8('bp_x', i), g.m8('bp_y', i)))
        S['tn'] = tn
        hz = []
        for i in range(3):
            on = g.m8('hz_on', i)
            if on: hz.append((on, g.m8('hz_x', i), g.m8('hz_y0', i), g.m8('hz_t', i)))
        S['hz'] = hz
        pk = []
        for i in range(3):
            if g.m8('pk_on', i):
                pk.append((g.m16('pk_x', i * 2) / 256 + 8, g.m16('pk_y', i * 2) / 256 + 8))
        S['pk'] = pk
        S['hearts'] = g.m8('hearts'); S['lives'] = g.m8('lives'); S['bombs'] = g.m8('bombs')
        S['hits'] = g.m16('hits_taken'); S['score'] = g.m32('score')
        return S

    # ---------------------------------------------------------------- decisions
    def danger(self, S, d, T, margin):
        spd = 1.75 if self.mode == 'hunter' else 1.62
        pcx, pcy = S['px'] + 8, S['py'] + 12
        hb = 2 if self.mode == 'hunter' else 3
        boss = S['khpmax'] > 0
        ymax = (100 if boss else 108) + YO
        tot = 0.0
        for t in range(1, T + 1):
            x = min(max(S['px'] + d[0] * spd * t, XO), 144 + XO) + 8
            y = min(max(S['py'] + d[1] * spd * t, 8 + YO), ymax) + 12
            w = 1.0 / (1 + t * 0.15)
            for (bx, by, vx, vy, r) in S['bul']:
                if abs(bx + vx * t - x) < r + hb + margin and abs(by + vy * t - y) < r + hb + margin:
                    tot += 10 * w
            for (ex, ey, vx, vy, r, typ) in S['ene']:
                if abs(ex - x) < r + hb + margin + 2 and abs(ey + vy * t - y) < r + hb + margin + 2:
                    tot += 8 * w
            for (tx, ty) in S['tn']:
                if abs(tx - x) < 7 + margin and abs(ty - y) < 7 + margin:
                    tot += 6 * w
            for (on, hx_, hy0, ht) in S['hz']:
                if (on == 2 and t <= ht + 2) or (on == 1 and ht < t + 6):
                    if abs(hx_ - x) < 7 + margin and y > hy0: tot += 9 * w
            if boss and y < S['hy1'] - S['ret'] and abs(x - (96 + S['sway'] + (S['hdx'] - 256 if S['hdx'] > 127 else S['hdx']))) < S['hw'] + 4:
                tot += 6 * w
        return tot

    def goal(self, S, d):
        spd = 1.7
        x = S['px'] + 8 + d[0] * spd * 6; y = S['py'] + 12 + d[1] * spd * 6
        tx = None
        if S['slp']:
            tx, ty = min(S['slp'], key=lambda p: abs(p[0] - x) + abs(p[1] - y))
            if ty > 40 + YO or self.profile == 'pro':
                return -(abs(tx - x) + abs(ty - y)) * 0.08
        if S['pk'] and self.profile == 'pro':
            tx, ty = min(S['pk'], key=lambda p: abs(p[0] - x) + abs(p[1] - y))
            return -(abs(tx - x) + abs(ty - y)) * 0.08
        if S['khpmax']:
            tx = 96 + S['sway'] + self.rng.choice([-10, 10]) * 0
            ty = 128 + YO - 14
        else:
            cands = [e for e in S['ene'] if e[1] < S['py'] + 4]
            if cands:
                e = min(cands, key=lambda e: abs(e[0] - x))
                tx = e[0]
            ty = 112 + YO
        c = 0.0
        if tx is not None: c -= abs(tx - x) * 0.06
        c -= abs(ty - y) * 0.03
        return c

    def decide(self, S):
        pro = self.profile == 'pro'
        T = 20 if pro else 5
        margin = 2 if pro else 0
        best, bestv = (0, 0), -1e9
        scores = []
        for d in DIRS:
            v = -self.danger(S, d, T, margin) + self.goal(S, d)
            if pro and S['khpmax'] and S['warn_t']:          # a blinking marker: get out of that column
                nx = S['px'] + 8 + d[0] * 12
                if abs(nx - (S['warn_x'] + 4)) < 20: v -= 30
            scores.append((v, d))
            if v > bestv: bestv, best = v, d
        bomb = False
        if pro and self.rng.random() < 0.06:
            best = self.rng.choice(DIRS)          # a little human error
        if pro:
            mind = min(self.danger(S, d, 8, 0) for d in DIRS)
            if mind > 6 and S['bombs'] > 0: bomb = True
        else:
            if self.rng.random() < 0.004: bomb = True
            if self.rng.random() < 0.40: best = self.rng.choice(DIRS)
        return best, bomb

    def set_dir(self, d):
        want = set(BTN[d])
        for b in list(self.held):
            if b in ('left', 'right', 'up', 'down') and b not in want:
                self.g.release(b); self.held.discard(b)
        for b in want:
            if b not in self.held:
                self.g.hold(b); self.held.add(b)

    # ---------------------------------------------------------------- flow
    def start(self):
        g = self.g
        g.tick(500)                                       # studio card plays (~5.5 s), then the title
        for k in range(3): g.s8('pf_unlock', 4, k)       # all missions open for testing
        g.press('start'); g.tick(30)                     # title -> pilot select
        for k in range({'cadet': 0, 'pilot': 1, 'hunter': 2}[self.mode]):   # slot 0 CADET, 1 PILOT, 2 HUNTER
            g.press('down'); g.tick(4)
        g.press('a'); g.tick(40)                         # -> mission map
        for k in range(5): g.press('up'); g.tick(3)
        for k in range(self.stage): g.press('down'); g.tick(4)
        if self.boss:
            g.hold('select'); g.press('a'); g.release('select'); g.tick(40)
        else:
            g.press('a'); g.tick(50)
            g.press('start'); g.tick(30)                 # skip briefing
        if self.mode == 'hunter': g.hold('a'); self.held.add('a')

    def run(self):
        g = self.g
        self.start()
        dec_every = 1 if self.profile == 'pro' else self.rng.choice([12, 14, 16])
        shot_i = 0
        boss_f0 = None
        last_hits = 0
        events = []
        while g.frame < self.maxframes:
            S = self.state()
            if S['st'] == 3:  # clear
                break
            if S['st'] == 1:   # dying: continue prompt handling
                pass
            if g.frame % dec_every == 0:
                d, bomb = self.decide(S)
                self.set_dir(d)
                if bomb:
                    g.hold('b'); g.tick(2); g.release('b')
            if S['khpmax'] and boss_f0 is None: boss_f0 = g.frame; events.append(('boss', g.frame, S['pos']))
            if S['hits'] != last_hits:
                events.append(('hit', g.frame, S['pos'], S['khp'])); last_hits = S['hits']
            # continue prompt: if lives == 1 and st == dying long, hunter presses A
            if self.mode == 'hunter' and S['st'] == 1 and S['lives'] == 1:
                g.tick(75)
                g.release('a'); g.tick(5); g.press('a'); g.tick(5); g.hold('a')
                self.continues += 1
                events.append(('continue', g.frame))
                if self.continues > 6: break
                continue
            if self.shots and g.frame % 600 == 0:
                g.shot(f"{self.shots}/{shot_i:03d}.png"); shot_i += 1
            g.tick(1)
        S = self.state()
        res = dict(mode=self.mode, profile=self.profile, cleared=S['st'] == 3, frames=g.frame,
                   minutes=round(g.frame / 3600, 2), hits=S['hits'], continues=self.continues, score=S['score'],
                   boss_seconds=round((g.frame - boss_f0) / 60, 1) if boss_f0 else None,
                   lives=S['lives'], pos=S['pos'], khp=S['khp'], stage=self.stage + 1, buddies=g.m8('bud_n') if 'bud_n' in g.sym else -1,
                   bounty=g.m8('bounty_out'), mid=g.m8('mid_out'), bpts=g.m32('bounty_pts'))
        deaths = [e for e in events if e[0] == 'hit']
        res['hit_positions'] = [e[2] for e in deaths][:40]
        return res

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("rom"); ap.add_argument("mode"); ap.add_argument("profile")
    ap.add_argument("seed", nargs="?", type=int, default=1)
    ap.add_argument("--shots"); ap.add_argument("--boss", action="store_true")
    ap.add_argument("--stage", type=int, default=1)
    a = ap.parse_args()
    if a.shots: os.makedirs(a.shots, exist_ok=True)
    print(Bot(a.rom, a.mode, a.profile, a.seed, a.shots, a.boss, stage=a.stage - 1).run())
