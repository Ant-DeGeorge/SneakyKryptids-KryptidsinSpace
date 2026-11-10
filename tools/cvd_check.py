#!/usr/bin/env python3
"""Colour-vision check for the player weapon palettes (Project: weapon flair).

Simulates protanopia, deuteranopia and tritanopia (Machado, Oliveira & Fernandes 2009,
severity 1.0, applied in linear sRGB) on the GBC RGB555 colours and reports CIEDE2000
distances between each weapon's body colour and (a) the other weapons, (b) every stage's
enemy-bullet colour, (c) the fx/pickup palette. Also prints relative luminance so each
weapon also differs in brightness, not just hue.  Usage: python3 tools/cvd_check.py
"""
import math, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from weapon_fx import WEAPON_PALS, WEAPON_NAMES

M = {
 'normal': [[1,0,0],[0,1,0],[0,0,1]],
 'protan': [[0.152286,1.052583,-0.204868],[0.114503,0.786281,0.099216],[-0.003882,-0.048116,1.051998]],
 'deutan': [[0.367322,0.860646,-0.227968],[0.280085,0.672501,0.047413],[-0.011820,0.042940,0.968881]],
 'tritan': [[1.255528,-0.076749,-0.178779],[-0.078411,0.930809,0.147602],[0.004733,0.691367,0.303900]],
}
def hx(s): s = s.lstrip('#'); return [int(s[i:i+2], 16) for i in (0, 2, 4)]
def gbc(c): return [((v >> 3) << 3) | (v >> 5) for v in c]          # what RGB555 actually shows
def lin(v): v /= 255; return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4
def unlin(v): v = min(1, max(0, v)); return 255 * (12.92 * v if v <= 0.0031308 else 1.055 * v ** (1 / 2.4) - 0.055)
def sim(rgb, kind):
    l = [lin(v) for v in rgb]; m = M[kind]
    return [unlin(sum(m[r][k] * l[k] for k in range(3))) for r in range(3)]
def lab(rgb):
    r, g, b = [lin(v) for v in rgb]
    X = (0.4124*r + 0.3576*g + 0.1805*b) / 0.95047; Y = 0.2126*r + 0.7152*g + 0.0722*b; Z = (0.0193*r + 0.1192*g + 0.9505*b) / 1.08883
    f = lambda t: t ** (1/3) if t > 0.008856 else 7.787 * t + 16/116
    return [116*f(Y) - 16, 500*(f(X) - f(Y)), 200*(f(Y) - f(Z))]
def lum(rgb): r, g, b = [lin(v) for v in rgb]; return 0.2126*r + 0.7152*g + 0.0722*b
def de2000(c1, c2):
    L1, a1, b1 = lab(c1); L2, a2, b2 = lab(c2)
    C1 = math.hypot(a1, b1); C2 = math.hypot(a2, b2); Cb = (C1 + C2) / 2
    G = 0.5 * (1 - math.sqrt(Cb**7 / (Cb**7 + 25**7)))
    a1p, a2p = a1 * (1 + G), a2 * (1 + G)
    C1p, C2p = math.hypot(a1p, b1), math.hypot(a2p, b2)
    h1 = math.degrees(math.atan2(b1, a1p)) % 360; h2 = math.degrees(math.atan2(b2, a2p)) % 360
    dL = L2 - L1; dC = C2p - C1p
    dh = 0 if C1p * C2p == 0 else (h2 - h1 if abs(h2 - h1) <= 180 else h2 - h1 - 360 if h2 > h1 else h2 - h1 + 360)
    dH = 2 * math.sqrt(C1p * C2p) * math.sin(math.radians(dh / 2))
    Lb = (L1 + L2) / 2; Cbp = (C1p + C2p) / 2
    hb = (h1 + h2) / 2 if abs(h1 - h2) <= 180 else (h1 + h2 + 360) / 2 if h1 + h2 < 360 else (h1 + h2 - 360) / 2
    if C1p * C2p == 0: hb = h1 + h2
    T = 1 - 0.17*math.cos(math.radians(hb-30)) + 0.24*math.cos(math.radians(2*hb)) + 0.32*math.cos(math.radians(3*hb+6)) - 0.20*math.cos(math.radians(4*hb-63))
    SL = 1 + 0.015*(Lb-50)**2 / math.sqrt(20 + (Lb-50)**2); SC = 1 + 0.045*Cbp; SH = 1 + 0.015*Cbp*T
    RT = -2*math.sqrt(Cbp**7/(Cbp**7+25**7)) * math.sin(math.radians(60*math.exp(-((hb-275)/25)**2)))
    return math.sqrt((dL/SL)**2 + (dC/SC)**2 + (dH/SH)**2 + RT*(dC/SC)*(dH/SH))

ENEMY = {'S1 coral': '#FF7052', 'S2 yellow': '#F8E848', 'S3 orange': '#F89020', 'S4 pink': '#F84898',
         'fx gold': '#F8D038', 'fx orange': '#E86018'}
BGS = {'space': '#1E2A58', 'space hi': '#7888C8', 'nebula': '#5C3C94', 'ice': '#6CA8E0', 'metal': '#56606C'}

def report(out=sys.stdout):
    worst = {}
    for kind in M:
        print(f"\n== {kind}", file=out)
        body = {n: gbc(hx(p[2])) for n, p in zip(WEAPON_NAMES, WEAPON_PALS)}
        for n, c in body.items():
            s = sim(c, kind)
            print(f"  {n:7s} body {WEAPON_PALS[WEAPON_NAMES.index(n)][2]} -> sim #{''.join('%02X' % round(v) for v in s)}  Y={lum(s):.2f}", file=out)
        names = list(body)
        for i in range(len(names)):
            for j in range(i + 1, len(names)):
                d = de2000(sim(body[names[i]], kind), sim(body[names[j]], kind))
                worst[('weapon', kind)] = min(worst.get(('weapon', kind), 999), d)
                print(f"  dE00 {names[i]} vs {names[j]}: {d:5.1f}", file=out)
        for n, c in body.items():
            for en, ec in ENEMY.items():
                d = de2000(sim(c, kind), sim(gbc(hx(ec)), kind))
                worst[('enemy', kind)] = min(worst.get(('enemy', kind), 999), d)
            for bn, bc in BGS.items():
                d = de2000(sim(c, kind), sim(gbc(hx(bc)), kind))
                worst[('bg', kind)] = min(worst.get(('bg', kind), 999), d)
        dmin = min((de2000(sim(c, kind), sim(gbc(hx(ec)), kind)), n, en) for n, c in body.items() for en, ec in ENEMY.items())
        print(f"  closest weapon/enemy-or-fx pair: {dmin[1]} vs {dmin[2]} dE00={dmin[0]:.1f}", file=out)
    print("\nminimum dE00 (weapon-weapon / weapon-enemy+fx / weapon-background):", file=out)
    for kind in M:
        print(f"  {kind:7s} {worst[('weapon', kind)]:5.1f} / {worst[('enemy', kind)]:5.1f} / {worst[('bg', kind)]:5.1f}", file=out)
    return worst

if __name__ == '__main__':
    report()
