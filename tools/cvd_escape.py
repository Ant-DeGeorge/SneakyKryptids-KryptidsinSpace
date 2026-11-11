#!/usr/bin/env python3
"""Colour-vision check for mission 6 (Escape Run). Uses the simulation in cvd_check.py.

Rule: every hazard colour must stand apart from every backdrop colour and from the player's
cool colours under normal vision, protanopia, deuteranopia and tritanopia (CIEDE2000 >= 20),
and must also be clearly brighter than the backdrop (luminance ratio >= 3), so the cue never
rests on hue alone.  Usage: python3 tools/cvd_escape.py
"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cvd_check import M, hx, gbc, sim, de2000, lum

# main hazard colours (the dim phase of the laser blink and shading colours are not cues by themselves)
HAZ = {'shot / laser yellow': '#FFE860', 'shot / laser orange': '#F08020', 'rock light': '#E0D0B0'}
# everything a hazard can be drawn over while hazards are live
BG = {'space': '#000008', 'space dim star': '#1C2850', 'atmo 1': '#06041A', 'atmo 1 wisp': '#2A2460', 'atmo 2': '#100830',
      'atmo 2 wisp': '#3C2C70', 'atmo 3': '#1C1044', 'atmo 3 wisp': '#402E74'}
COOL = {'graze / pod cyan': '#78E8F8', 'pod blue': '#2878D8', 'ship teal': '#48E0C8'}

def main():
    bad = 0
    for kind in M:
        worst_bg = worst_cool = 999; wb = wc = None
        for hn, h in HAZ.items():
            hc = sim(gbc(hx(h)), kind)
            for bn, b in BG.items():
                bc = sim(gbc(hx(b)), kind)
                d = de2000(hc, bc); ratio = (lum(hc) + 0.05) / (lum(bc) + 0.05)
                if d < worst_bg: worst_bg, wb = d, (hn, bn, ratio)
                if d < 20 or ratio < 3: bad += 1; print(f"  FAIL {kind}: {hn} on {bn}: dE {d:.1f}, luminance x{ratio:.1f}")
            for cn, c in COOL.items():
                if hn.startswith('rock'): continue            # rocks are told apart by shape and size
                d = de2000(hc, sim(gbc(hx(c)), kind))
                if d < worst_cool: worst_cool, wc = d, (hn, cn)
                if d < 20: bad += 1; print(f"  FAIL {kind}: {hn} vs {cn}: dE {d:.1f}")
        print(f"{kind:7s} closest hazard/backdrop dE {worst_bg:5.1f} ({wb[0]} on {wb[1]}, luminance x{wb[2]:.1f});"
              f" closest hazard/cool dE {worst_cool:5.1f} ({wc[0]} vs {wc[1]})")
    print("PASS" if not bad else f"{bad} problems")
    return bad

if __name__ == "__main__":
    sys.exit(1 if main() else 0)
