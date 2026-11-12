#!/usr/bin/env python3
"""Playtest bot for mission 7 "The Tree Line" (Graul on foot, the wreck, the watcher).

  pro - re-plans every 2 frames, looks ~0.5 s ahead, howls when cornered
  kid - re-plans every ~12 frames, looks less far, sometimes wanders
Usage: bot_ground.py ROM cadet|pilot|hunter pro|kid [seed] [--boss] [--shots DIR]
"""
import sys, os, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bot import Bot

def s16(v): return v - 65536 if v >= 32768 else v
XO, YO, FIELD_BOT = 16, 32, 128
WX, WY = XO + 80, YO + 77
DIRS = [(0, 0), (-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (1, -1), (-1, 1), (1, 1)]

class GroundBot(Bot):
    def gstate(self):
        g = self.g
        S = dict(st=g.m8('st'), px=g.m8('px'), py=g.m8('py'), top=g.m16('gr_top'), fscy=g.m8('gr_fscy'), phase=g.m8('gr_phase'),
                 vis=g.m8('gr_vis'), comm=g.m8('comm_on'), bombs=g.m8('bombs'), lives=g.m8('lives'), hearts=g.m8('hearts'),
                 hits=g.m16('hits_taken'), bh=g.m8('gr_bh'), ef=g.m8('gr_ef'), warn=g.m8('gr_warn'), warnx=g.m8('gr_warn_x'), swing=g.m8('gr_swing'))
        S['en'] = [(g.m8('gr_gt', i), g.m8('gr_gx', i), g.m8('gr_gy', i), g.m8('gr_gs', i)) for i in range(8) if g.m8('gr_gt', i)]
        S['bul'] = [(g.m16('eb_x', i * 2) / 256 + 3, g.m16('eb_y', i * 2) / 256 + 3, s16(g.m16('eb_vx', i * 2)) / 256, s16(g.m16('eb_vy', i * 2)) / 256,
                     g.m8('eb_big', i)) for i in range(14) if g.m8('eb_on', i) == 1]
        S['pl'] = [g.m8('gr_pl', r) for r in range(32)]; S['pr'] = [g.m8('gr_pr', r) for r in range(32)]
        S['ol'] = [g.m8('gr_ol', r) for r in range(32)]; S['or'] = [g.m8('gr_or', r) for r in range(32)]
        S['bb'] = (32 if S['comm'] else 0) + (min(S['vis'], 24) if S['comm'] else S['vis']) + YO
        return S

    def blocked(self, S, x, y):
        r = (((S['fscy'] + int(y) - YO) & 255) >> 3) & 31; c = ((int(x) - XO) & 255) >> 3
        return c < S['pl'][r] or c > S['pr'][r] or S['ol'][r] <= c <= S['or'][r]

    def free(self, S, x, y):
        return not any(self.blocked(S, x + a, y + b) for a in (-5, 5) for b in (1, 7)) and not any(self.blocked(S, x + a, y - 5) for a in (-4, 4))

    def danger(self, S, d, horizon):
        x, y = S['px'], S['py']
        for f in range(1, horizon + 1):
            nx, ny = x + d[0] * 1.5, y + d[1] * 1.5
            if XO + 7 <= nx <= XO + 153 and self.free(S, nx, y): x = nx
            if S['bb'] + 10 <= ny <= YO + FIELD_BOT - 10 and self.free(S, x, ny): y = ny
            for (bx, by, vx, vy, big) in S['bul']:
                if abs(bx + vx * f - x) < 8 and abs(by + vy * f - y) < 9: return f, x, y
            for (t, ex, ey, es) in S['en']:
                if t == 6:
                    if abs(ex - x) < 13 and ey - 30 < y < ey + 6: return f, x, y
                elif t == 5:
                    if abs(ex - x) < 19 and abs(ey - y) < 19: return f, x, y
                elif t == 4 and es in (2, 3):
                    if abs(ex - x) < 16 and abs(ey - y) < 16: return f, x, y
            if S['warn'] and S['warn'] < f + 6 and abs(S['warnx'] - x) < 15: return f, x, y
        return horizon + 10, x, y

    def run(self):
        g = self.g
        self.start()
        if self.mode == 'hunter': g.release('a'); self.held.discard('a')
        pro = self.profile == 'pro'
        dec_every = 2 if pro else self.rng.choice([10, 12, 14])
        horizon = 26 if pro else 14
        d = (0, 0); side = (0, 0); side_n = 0; shot_i = 0; f0 = g.frame; l0 = g.m16('gr_loops'); stuck = 0; lastpos = None; phases = {}
        while g.frame < self.maxframes:
            S = self.gstate()
            if S['st'] == 3: break
            phases.setdefault(S['phase'], g.frame - f0)
            if S['st'] == 0 and g.frame % dec_every == 0:
                px, py = S['px'], S['py']
                foes = [e for e in S['en'] if e[0] != 6]
                near = sorted(foes, key=lambda e: abs(e[1] - px) + abs(e[2] - py))
                if S['phase'] in (0,):
                    # up the path: aim at the middle of the open columns a few rows ahead, around any boulder
                    r = (((S['fscy'] + py - YO - 24) & 255) >> 3) & 31
                    lo, hi = S['pl'][r], S['pr'][r]
                    if S['ol'][r] <= S['or'][r]:
                        if S['ol'][r] - lo > hi - S['or'][r]: hi = S['ol'][r] - 1
                        else: lo = S['or'][r] + 1
                    tx, ty = XO + (lo + hi + 1) * 4, py - 30
                    if near and abs(near[0][2] - py) < 40 and near[0][0] in (1, 2, 3): tx, ty = near[0][1], near[0][2] + 14
                elif S['phase'] == 3:
                    chew = [e for e in foes if e[0] == 1 and e[3] == 1]
                    tgt = sorted(chew or foes, key=lambda e: abs(e[1] - px) + abs(e[2] - py))
                    tx, ty = (tgt[0][1], tgt[0][2] + 12) if tgt else (WX, WY + 34)
                elif S['phase'] == 6:
                    claw = [e for e in S['en'] if e[0] == 6 and e[3] == 1]
                    orbs = [b for b in S['bul'] if b[4]]
                    if claw: tx, ty = claw[0][1] + 18, claw[0][2] - 8
                    elif near: tx, ty = near[0][1], near[0][2] + 12
                    elif orbs: tx, ty = orbs[0][0], orbs[0][1] + 18
                    else: tx, ty = WX, WY + 36
                else:
                    tx, ty = (near[0][1], near[0][2] + 12) if near else (XO + 80, YO + 90)
                best, bs, bt = (0, 0), -1e9, 99
                for c in DIRS:
                    t, nx, ny = self.danger(S, c, horizon)
                    sc = min(t, horizon) * 8 - abs(nx - tx) * 0.6 - abs(ny - ty) * 0.6
                    if not pro: sc += self.rng.uniform(-10, 10)
                    if sc > bs: best, bs, bt = c, sc, t
                d = best
                if not pro and self.rng.random() < 0.08: d = self.rng.choice(DIRS)
                if lastpos == (px, py, S['top']) and S['phase'] == 0: stuck += 1
                else: stuck = 0
                lastpos = (px, py, S['top'])
                if stuck > 8 and not side_n: side = self.rng.choice([(-1, 0), (1, 0), (-1, 1), (1, 1)]); side_n = 14; stuck = 0     # boxed in under a boulder: step round it
                if side_n: side_n -= 1; d = side if side_n > 5 else (side[0], -1)
                want_a = any(abs(e[1] - px) < 30 and abs((e[2] - 12 if e[0] == 6 else e[2]) - py) < 30 for e in S['en']) or \
                         any(abs(b[0] - px) < 24 and abs(b[1] - py) < 24 for b in S['bul'])
                if self.mode == 'hunter' or pro:
                    if want_a: g.hold('a')
                    else: g.release('a')
                if S['bombs'] and ((pro and bt < 4) or (not pro and self.rng.random() < 0.004) or (pro and len(foes) >= 6)):
                    g.hold('b'); g.tick(2); g.release('b')
            self.set_dir(d if S['st'] == 0 else (0, 0))
            if self.mode == 'hunter' and S['st'] == 1 and S['lives'] == 1:
                g.tick(75 + 115); g.press('a'); g.tick(5)
                self.continues += 1
                if self.continues > 6: break
                continue
            if self.shots and g.frame % 240 == 0:
                g.shot(f"{self.shots}/{shot_i:03d}.png"); shot_i += 1
            g.tick(1)
        S = self.gstate()
        g.tick(120)
        return dict(mode=self.mode, profile=self.profile, cleared=S['st'] == 3, minutes=round((g.frame - f0) / 3600, 2), hits=S['hits'],
                    continues=self.continues, score=g.m32('score'), top=S['top'], phase=S['phase'], lives=S['lives'], stage=7,
                    bounty=g.m8('bounty_out'), bpts=g.m32('bounty_pts'), slow=(g.frame - f0) - ((g.m16('gr_loops') - l0) & 0xFFFF), phases=phases)

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("rom"); ap.add_argument("mode"); ap.add_argument("profile")
    ap.add_argument("seed", nargs="?", type=int, default=1)
    ap.add_argument("--shots"); ap.add_argument("--boss", action="store_true")
    a = ap.parse_args()
    if a.shots: os.makedirs(a.shots, exist_ok=True)
    print(GroundBot(a.rom, a.mode, a.profile, a.seed, a.shots, a.boss, stage=6).run())
