#!/usr/bin/env python3
"""Playtest bot for mission 6 "Escape Run" (no shooting: dodge, graze, dash, dive).

  pro - re-plans every 2 frames, looks ~0.5 s ahead, dashes and pulses when cornered
  kid - re-plans every ~12 frames, looks less far, sometimes drifts, random pulses
Usage: bot_escape.py ROM cadet|pilot|hunter pro|kid [seed] [--sprint] [--shots DIR]
"""
import sys, os, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bot import Bot

def s16(v): return v - 65536 if v >= 32768 else v
XO, YO, FIELD_BOT = 16, 32, 128
DIRS = [(0, 0), (-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (1, -1), (-1, 1), (1, 1)]

class EscapeBot(Bot):
    def estate(self):
        g = self.g
        S = dict(st=g.m8('st'), px=g.m8('px') - XO, py=g.m8('py') - YO, row=g.m16('er_row'), fscy=g.m8('er_fscy'),
                 vis=g.m8('er_vis'), comm=g.m8('comm_on'), boost=g.m8('er_boost'), bombs=g.m8('bombs'), lives=g.m8('lives'),
                 hearts=g.m8('hearts'), hits=g.m16('hits_taken'), score=g.m32('score'), las=g.m8('er_las_state'),
                 pod=g.m8('er_pod'), podx=g.m8('er_pod_x') - XO, pody=g.m8('er_pod_y') - YO,
                 nwarn=g.m8('er_next_warn'), nc=g.m8('er_next_c'))
        S['lcols'] = [g.m8('er_las_col', i) for i in range(g.m8('er_las_n'))] if S['las'] in (1, 2, 3) else []
        S['bul'] = [(g.m16('eb_x', i * 2) / 256 + 3 - XO, g.m16('eb_y', i * 2) / 256 + 3 - YO,
                     s16(g.m16('eb_vx', i * 2)) / 256, s16(g.m16('eb_vy', i * 2)) / 256, 6) for i in range(14) if g.m8('eb_on', i)]
        for i in range(3):
            if g.m8('er_mis_on', i):
                x, y = g.m16('er_mis_x', i * 2) / 256 - XO, g.m16('er_mis_y', i * 2) / 256 - YO
                dx, dy = S['px'] - x, S['py'] - y; n = max(1.0, (dx * dx + dy * dy) ** 0.5)
                S['bul'].append((x, y, dx / n * 1.5, dy / n * 1.5, 10))
        S['rocks'] = [(g.m8('er_rk_x', i) - XO, g.m16('er_rk_y', i * 2) / 256 - YO, g.m8('er_rk_t', i) if g.m8('er_rk_on', i) == 1 else 0) for i in range(4) if g.m8('er_rk_on', i)]
        walls = []
        for r in range(32):
            if g.m8('er_wall', r):
                y = (r * 8 - S['fscy']) & 255
                if y < 150 or y > 248: walls.append((y if y < 150 else y - 256, g.m8('er_wl', r) * 8, g.m8('er_wr', r) * 8 + 8))
        S['walls'] = walls
        S['top'] = (32 if S['comm'] else 0) + (min(S['vis'], 24) if S['comm'] else S['vis']) + 10
        return S

    def danger(self, S, d, horizon, speed):
        """Frames until a hit if we hold direction d (horizon = safe)."""
        x, y = S['px'], S['py']
        for f in range(1, horizon + 1):
            x = min(152, max(8, x + d[0] * 2)); y = min(FIELD_BOT - 12, max(S['top'], y + d[1] * 2))
            for (bx, by, vx, vy, r) in S['bul']:
                if abs(bx + vx * f - x) < r and abs(by + vy * f - y) < r: return f
            for (rx, ry, wait) in S['rocks']:
                ry2 = ry - (speed + 0.4) * max(0, f - wait)
                if abs(rx - x) < 12 and abs(ry2 - y) < 12: return f
            for (wy, gl, gr) in S['walls']:
                wy2 = wy - speed * f
                if wy2 - 2 <= y <= wy2 + 10 and (x - 6 < gl or x + 6 > gr): return f
            if S['lcols'] and f > 6:
                for c in S['lcols']:
                    if abs(c * 8 + 4 - x) < 9: return f + 6
        return horizon + 10

    def run(self):
        g = self.g
        self.start()
        if self.mode == 'hunter': g.release('a'); self.held.discard('a')
        pro = self.profile == 'pro'
        dec_every = 2 if pro else self.rng.choice([10, 12, 14])
        horizon = 34 if pro else 20
        d = (0, 0); shot_i = 0; last_hits = 0; events = []; last_f = None; speed = 2.0; f0 = g.frame; l0 = g.m16('er_loops')
        while g.frame < self.maxframes:
            S = self.estate()
            if S['st'] == 3: break
            if last_f is not None:
                dv = (S['fscy'] - last_f) & 255
                if dv < 8: speed = speed * 0.8 + dv * 0.2
            last_f = S['fscy']
            if S['st'] == 0 and g.frame % dec_every == 0:
                # where we would like to be
                ahead = sorted((w for w in S['walls'] if w[0] > S['py'] - 4), key=lambda w: w[0])
                if S['pod'] == 1: tx, ty = S['podx'], S['pody']
                elif ahead: tx, ty = (ahead[0][1] + ahead[0][2]) / 2, max(S['top'] + 8, 64)
                elif S['nwarn']: tx, ty = S['nc'] * 8 + 4, 70
                else: tx, ty = 80, 78
                best, bs = (0, 0), -1e9
                for c in DIRS:
                    t = self.danger(S, c, horizon, speed)
                    nx, ny = S['px'] + c[0] * 12, S['py'] + c[1] * 12
                    sc = min(t, horizon) * 6 - abs(nx - tx) * 0.5 - abs(ny - ty) * 0.25
                    if not pro: sc += self.rng.uniform(-18, 18)
                    if sc > bs: best, bs, bt = c, sc, t
                d = best
                if not pro and self.rng.random() < 0.12: d = self.rng.choice(DIRS)
                if pro and bt < 8:
                    if S['boost'] >= 12 and self.mode != 'cadet': g.hold('a'); g.tick(2); g.release('a')
                    elif S['bombs'] and bt < 5: g.hold('b'); g.tick(2); g.release('b')
                elif not pro and self.rng.random() < 0.006 and S['bombs']: g.hold('b'); g.tick(2); g.release('b')
                elif S['boost'] >= 12 and self.mode != 'cadet' and self.rng.random() < (0.05 if pro else 0.02): g.hold('a'); g.tick(2); g.release('a')
            self.set_dir(d if S['st'] == 0 else (0, 0))
            if S['hits'] != last_hits: events.append((S['row'], S['las'], len(S['bul']), len(S['walls']))); last_hits = S['hits']
            if self.mode == 'hunter' and S['st'] == 1 and S['lives'] == 1:
                g.tick(75 + 115); g.press('a'); g.tick(5)
                self.continues += 1
                if self.continues > 6: break
                continue
            if self.shots and g.frame % 240 == 0:
                g.shot(f"{self.shots}/{shot_i:03d}.png"); shot_i += 1
            g.tick(1)
        S = self.estate()
        g.tick(120)
        return dict(mode=self.mode, profile=self.profile, cleared=S['st'] == 3, minutes=round((g.frame - f0) / 3600, 2), hits=S['hits'],
                    continues=self.continues, score=g.m32('score'), row=S['row'], pod=S['pod'], lives=S['lives'], stage=6,
                    bounty=g.m8('bounty_out'), bpts=g.m32('bounty_pts'), slow=(g.frame - f0) - ((g.m16('er_loops') - l0) & 0xFFFF), hit_at=events[:40])

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("rom"); ap.add_argument("mode"); ap.add_argument("profile")
    ap.add_argument("seed", nargs="?", type=int, default=1)
    ap.add_argument("--shots"); ap.add_argument("--sprint", action="store_true")
    a = ap.parse_args()
    if a.shots: os.makedirs(a.shots, exist_ok=True)
    print(EscapeBot(a.rom, a.mode, a.profile, a.seed, a.shots, a.sprint, stage=5).run())
