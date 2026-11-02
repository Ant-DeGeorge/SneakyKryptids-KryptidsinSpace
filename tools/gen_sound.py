#!/usr/bin/env python3
"""Music for Kryptids in Space: Sneaky Kryptid. All compositions original.

Songs are note strings compiled to (note, frames) byte pairs.  Note 0 =
rest; 0xFF then a loop flag ends a track.  Melody plays on channel 2
(square), bass on channel 3 (wave).
"""
import os, re

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "sound_data.c")
NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
FLAT = {'Db': 'C#', 'Eb': 'D#', 'Gb': 'F#', 'Ab': 'G#', 'Bb': 'A#'}
LOW_OCT, HIGH_OCT = 1, 7

def note_index(n):
    m = re.fullmatch(r"([A-G][#b]?)(\d)", n)
    name = FLAT.get(m.group(1), m.group(1))
    return NAMES.index(name) + (int(m.group(2)) - LOW_OCT) * 12 + 1

def freq(i):
    return 440.0 * 2 ** (((i - 1) + LOW_OCT * 12 - (4 * 12 + 9)) / 12)

N_NOTES = (HIGH_OCT - LOW_OCT + 1) * 12
sq = [0] + [max(0, min(2047, round(2048 - 131072 / freq(i)))) for i in range(1, N_NOTES + 1)]
wv = [0] + [max(0, min(2047, round(2048 - 65536 / freq(i)))) for i in range(1, N_NOTES + 1)]

def track(s, tick):
    out = []
    for tok in s.split():
        n, d = (tok.split(':') + ['1'])[:2]
        out += [0 if n == 'R' else note_index(n), int(d) * tick]
    return out

def stacc(s, tick):
    """Each note sounds for half its length (punchy bass)."""
    out = []
    for tok in s.split():
        n, d = (tok.split(':') + ['1'])[:2]
        d = int(d) * tick
        if n == 'R': out += [0, d]
        else: out += [note_index(n), max(1, d // 2), 0, d - max(1, d // 2)]
    return out

def octs(roots, per=8):
    """Driving bass: root/octave alternating eighths for each bar root."""
    out = []
    for r in roots.split():
        lo = r; hi = r[:-1] + str(int(r[-1]) + 1)
        out += [lo, hi] * (per // 2)
    return " ".join(out)

songs = {}

# ---------------------------------------------------------------- TITLE: "Sneaky Kryptid" (D minor)
T = 8
ta = "D4:2 A4:2 F4 G4 A4:2  C5:2 A4 G4 F4:2 E4:2  D4:2 A4:2 F4 G4 A4 C5  D5:6 R:2"
tb = "A#4:2 A4 G4 A4:2 F4:2  G4:2 F4 E4 F4:2 D4:2  E4 F4 G4 A4 A#4 A4 G4 E4  D4:6 R:2"
tc = "F4:2 A4:2 C5:2 A4:2  G4:2 A#4:2 D5:2 A#4:2  A4 G4 F4 E4 F4:2 A4:2  E4:6 R:2"
title_mel = track(" ".join([ta, tb, tc, ta]), T)
title_bas = stacc(" ".join([octs("D2 F2 D2 D2"), octs("A#1 C2 A1 D2"), octs("F2 G2 F2 A1"), octs("D2 F2 D2 D2")]), T)
songs['TITLE'] = (title_mel, title_bas, 0x84, 1)

# ---------------------------------------------------------------- STAGE 1: "Rimeveil Run" (E minor, driving)
T = 7
sa1 = "E4 E4 G4 E4 A4 E4 B4 A4  G4 G4 B4 G4 D5:2 B4:2  C5 B4 A4 G4 A4:2 F#4:2  G4 A4 B4 G4 E4:4"
sa2 = "E4 E4 G4 E4 A4 E4 B4 A4  G4 G4 B4 G4 D5:2 B4:2  C5 B4 A4 G4 A4:2 B4:2  E5:6 R:2"
sb1 = "B4:2 E5:2 D5 B4 A4 B4  G4:2 B4:2 A4 G4 F#4 G4  E4 F#4 G4 A4 B4 C5 D5 B4  E5:4 D#5:4"
sb2 = "C5:2 E5:2 D5 C5 B4 A4  B4:2 D5:2 C5 B4 A4 G4  A4 B4 C5 D5 E5 D5 C5 B4  B4:6 R:2"
stage_mel = track(" ".join([sa1, sa2, sb1, sb2]), T)
bA = octs("E2 G2 A2 E2") ; bA2 = octs("E2 G2 A2 B1")
bB = octs("E2 G2 C2 B1") ; bB2 = octs("A1 G2 A1 B1")
stage_bas = stacc(" ".join([bA, bA2, bB, bB2]), T)
songs['STAGE'] = (stage_mel, stage_bas, 0x72, 1)

# ---------------------------------------------------------------- BOSS: "Kraken Rising" (D minor, chromatic)
T = 6
ba1 = "D4 D4 D#4 D4 F4 D4 G#4 G4  D4 D4 D#4 D4 A#4:2 A4:2  D5 C5 A#4 A4 G#4 G4 F4 D#4  D4:2 R D4 D#4 D4 C#4:2"
ba2 = "A4:3 G#4 G4:2 F4:2  A#4:3 A4 G#4:2 G4:2  C5:3 A#4 A4:2 G4:2  A4 A#4 A4 G#4 A4:4"
ba3 = "D5:2 R D5 C#5 D5 F5 D5  C5:2 R C5 B4 C5 D#5 C5  A#4 A4 G#4 G4 F4 D#4 D4 C#4  D4:4 A3:4"
boss_mel = track(" ".join([ba1, ba1, ba2, ba3]), T)
bb = octs("D2 D2 D#2 D2") ; bb2 = octs("F2 G2 A2 A1") ; bb3 = octs("D2 C2 A#1 A1")
boss_bas = stacc(" ".join([bb, bb, bb2, bb3]), T)
songs['BOSS'] = (boss_mel, boss_bas, 0x82, 1)

# ---------------------------------------------------------------- BRIEFING: calm comms room (A minor)
T = 12
br = "A4 C5 E5 C5 A4 C5 E5 C5  F4 A4 C5 A4 F4 A4 C5 A4  G4 B4 D5 B4 G4 B4 D5 B4  E4 G#4 B4 G#4 E4:4"
brief_mel = track(br, T)
brief_bas = track("A2:8 F2:8 G2:8 E2:8", T)
songs['BRIEF'] = (brief_mel, brief_bas, 0x53, 1)

# ---------------------------------------------------------------- WARNING sting (before the Kraken)
T = 8
warn_mel = track("D5:2 R:2 D5:2 R:2 C#5:2 R:2 C#5:2 R:2 D5:2 R:2 D5:2 R:2 A4:8", T)
warn_bas = track("D2:4 D2:4 C#2:4 C#2:4 D2:4 D2:4 A1:8", T)
songs['WARN'] = (warn_mel, warn_bas, 0xA2, 0)

# ---------------------------------------------------------------- CLEAR fanfare
F = 6
clear_mel = track("E4 G4 B4 E5:2 B4 E5:2 R  D5 C5 D5 E5:6 R:2", F)
clear_bas = track("E2:4 G2:4 C2:4 B1:2 E2:6", F)
songs['CLEAR'] = (clear_mel, clear_bas, 0xA4, 0)

# ---------------------------------------------------------------- GAME OVER
F = 10
over_mel = track("E4:2 D#4:2 D4:2 C#4:2 C4:6 R:2", F)
over_bas = track("A2:4 G#2:4 A1:8", F)
songs['OVER'] = (over_mel, over_bas, 0x87, 0)

# ---------------------------------------------------------------- ENDING (victory theme, D major lift of the title)
T = 8
ea = "D4:2 A4:2 F#4 G4 A4:2  B4:2 A4 G4 F#4:2 E4:2  D4:2 A4:2 F#4 G4 A4 B4  D5:6 R:2"
eb = "G4:2 A4 B4 A4:2 F#4:2  E4:2 F#4 G4 A4:2 D4:2  E4 F#4 G4 A4 B4 C#5 D5 E5  D5:6 R:2"
end_mel = track(" ".join([ea, eb]), T)
end_bas = stacc(" ".join([octs("D2 G2 D2 D2"), octs("G2 A2 G2 A1")]), T)
songs['END'] = (end_mel, end_bas, 0x84, 1)


# ---------------------------------------------------------------- STAGE 2: "Stormwall" (G minor, urgent)
T = 6
s2a = "G4 G4 D5 G4 C5 G4 A#4 A4  G4 G4 D5 G4 D#5:2 D5:2  C5 A#4 A4 G4 F#4 G4 A4 F#4  G4:4 D4:4"
s2b = "G4 G4 D5 G4 C5 G4 A#4 A4  G4 G4 D5 G4 F5:2 D#5:2  D5 C5 A#4 A4 A#4 C5 D5 F#4  G4:6 R:2"
s2c = "D#5:2 D5 C5 A#4:2 C5:2  D5:2 C5 A#4 A4:2 D5:2  C5 A#4 A4 G4 A4 A#4 C5 D5  D5:4 F#5:4"
s2d = "D#5:2 G5:2 F5 D#5 D5 C5  D5:2 A#4:2 C5 A#4 A4 G4  F#4 G4 A4 A#4 C5 D5 C5 A4  G4:6 R:2"
stage2_mel = track(" ".join([s2a, s2b, s2c, s2d]), T)
stage2_bas = stacc(" ".join([octs("G2 G2 D#2 D2"), octs("G2 G2 C2 D2"), octs("C2 D2 D#2 D2"), octs("C2 D#2 D2 G1")]), T)
songs['STAGE2'] = (stage2_mel, stage2_bas, 0x72, 1)

# ---------------------------------------------------------------- THUNDERBIRD (C minor, big leaps)
T = 6
tba = "C4:2 G4:2 C5:3 R  D#5 D5 C5 G4:2 G#4:2 G4  C4:2 G4:2 C5:3 R  F5 D#5 D5 C5 D5:4"
tbb = "D#5:2 D#5 F5 G5:2 D#5:2  D5:2 D5 D#5 F5:2 D5:2  C5 D5 D#5 F5 G5 G#5 G5 F5  G5:4 G4:4"
tbc = "G#4:2 C5:2 D#5:3 R  G5 F5 D#5 C5 D5:2 B4:2  C5 B4 C5 D5 D#5 D5 C5 B4  C5:6 R:2"
tbird_mel = track(" ".join([tba, tbb, tbc]), T)
tbird_bas = stacc(" ".join([octs("C2 G#1 C2 G1"), octs("D#2 D2 C2 G1"), octs("G#1 G1 C2 C2")]), T)
songs['TBIRD'] = (tbird_mel, tbird_bas, 0x82, 1)

# ---------------------------------------------------------------- STAGE 3: "Barrens Belt" (A minor, spooky folk lilt)
T = 7
s3a = "A4:2 E4 A4:2 B4 C5:2 B4 A4:2 E4  F4:2 A4 C5:2 D5 E5:3 R:3  D5:2 C5 B4:2 A4 G#4:2 A4 B4:2 E4  A4:3 E5:3 A4:6"
s3b = "C5:2 E5 D5:2 C5 B4:2 C5 D5:2 B4  A4:2 C5 E5:2 A5 G#5:3 R:3  F5:2 E5 D5:2 C5 B4:2 C5 D5:2 G#4  A4:6 R:6"
stage3_mel = track(" ".join([s3a, s3b]), T)
b3 = "A2:3 E2:3 A2:3 E2:3  F2:3 C2:3 F2:3 C2:3  D2:3 A1:3 E2:3 B1:3  A1:3 E2:3 A1:6"
stage3_bas = stacc(" ".join([b3, b3]), T)
songs['STAGE3'] = (stage3_mel, stage3_bas, 0x62, 1)

# ---------------------------------------------------------------- JERSEY DEVIL (E minor, frantic)
T = 5
jda = "E4 E4 F4 E4 G4 E4 A#4 A4  E4 E4 F4 E4 B4:2 A#4:2  E5 D5 C5 B4 A#4 A4 G4 F4  E4:2 R E4 F4 E4 D#4:2"
jdb = "B4:3 A#4 A4:2 G4:2  C5:3 B4 A#4:2 A4:2  D5:3 C5 B4:2 A4:2  B4 C5 B4 A#4 B4:4"
jdc = "E5:2 R E5 D#5 E5 G5 E5  D5:2 R D5 C#5 D5 F5 D5  C5 B4 A#4 A4 G4 F4 E4 D#4  E4:4 B3:4"
jd_mel = track(" ".join([jda, jda, jdb, jdc]), T)
jb = octs("E2 E2 F2 E2"); jb2 = octs("G2 A2 B2 B1"); jb3 = octs("E2 D2 C2 B1")
jd_bas = stacc(" ".join([jb, jb, jb2, jb3]), T)
songs['JDEVIL'] = (jd_mel, jd_bas, 0x82, 1)

# ---------------------------------------------------------------- STAGE 4: "Frostbite Moon" (D minor, glassy and driving)
T = 6
s4a = "D5 A4 F5 A4 E5 A4 D5 A4  C5 G4 E5 G4 D5 G4 C5 G4  A#4 F4 D5 F4 C5 F4 A#4 F4  A4 E4 C#5 E4 A4:4"
s4b = "F5:2 E5 D5 E5:2 A4:2  G5:2 F5 E5 F5:2 D5:2  E5 F5 G5 A5 G5 F5 E5 D5  C#5:4 E5:4"
s4c = "D5:2 F5:2 A5:3 R  G5 F5 E5 D5 E5:2 C5:2  A#4:2 D5:2 F5:3 R  E5 D5 C#5 E5 D5:4"
stage4_mel = track(" ".join([s4a, s4b, s4c]), T)
stage4_bas = stacc(" ".join([octs("D2 C2 A#1 A1"), octs("D2 C2 A#1 A1"), octs("D2 C2 A#1 A1")]), T)
songs['STAGE4'] = (stage4_mel, stage4_bas, 0x62, 1)

# ---------------------------------------------------------------- WENDIGO (B minor, stalking then frantic)
T = 6
wda = "B3:2 R B3 D4:2 C#4:2  B3:2 R B3 F#4:2 F4:2  B3:2 R B3 D4 E4 F#4 G4  F#4:4 A#3:4"
wdb = "B4:2 A#4 B4 D5:2 C#5:2  B4 A#4 B4 C#5 D5:2 F#4:2  G4 A4 B4 C#5 D5 E5 F#5 E5  F#5:4 F#4:4"
wdc = "G5:2 F#5 E5 D5:2 B4:2  E5:2 D5 C#5 B4:2 G4:2  F#4 G4 A#4 B4 C#5 D5 C#5 A#4  B4:6 R:2"
wend_mel = track(" ".join([wda, wdb, wdc]), T)
wend_bas = stacc(" ".join([octs("B1 B1 G1 F#1"), octs("B1 G1 E2 F#1"), octs("E2 G1 F#1 B1")]), T)
songs['WENDIGO'] = (wend_mel, wend_bas, 0x82, 1)

# ---------------------------------------------------------------- MISSION MAP (C major, steady)
T = 9
mp = "C4:2 G4:2 E4 F4 G4:2  A4:2 G4 F4 E4:2 C4:2  D4:2 A4:2 F4 G4 A4:2  G4:6 R:2  C5:2 B4 A4 G4:2 E4:2  F4:2 E4 D4 C4:2 E4:2  D4 E4 F4 G4 A4 B4 C5 D5  C5:6 R:2"
map_mel = track(mp, T)
map_bas = track("C3:4 G2:4 F2:4 C3:4  D3:4 F2:4 G2:4 G2:4  A2:4 E2:4 F2:4 C3:4  G2:4 G2:4 C3:4 C3:4", T)
songs['MAP'] = (map_mel, map_bas, 0x63, 1)

for _k, (_m, _b, _e, _l) in songs.items():
    _lm, _lb = sum(_m[1::2]), sum(_b[1::2])
    if _lm != _lb: print('LENGTH MISMATCH', _k, _lm, _lb)
ORDER = ['TITLE', 'STAGE', 'BOSS', 'BRIEF', 'WARN', 'CLEAR', 'OVER', 'END', 'STAGE2', 'TBIRD', 'STAGE3', 'JDEVIL', 'MAP', 'STAGE4', 'WENDIGO']

def carr(name, data, typ="uint8_t"):
    return f"const {typ} {name}[] = {{{','.join(map(str, data))}}};"

lines = ["// generated by tools/gen_sound.py", "#pragma bank 8", "#include <stdint.h>",
         carr("sq_freq", sq, "uint16_t"), carr("wave_freq", wv, "uint16_t")]
for k in ORDER:
    mel, bas, env, loop = songs[k]
    lines.append(carr(f"mus_{k.lower()}_mel", mel + [0xFF, loop]))
    lines.append(carr(f"mus_{k.lower()}_bas", bas + [0xFF, loop]))
lines.append("const uint8_t * const song_mel[] = {" + ",".join(f"mus_{k.lower()}_mel" for k in ORDER) + "};")
lines.append("const uint8_t * const song_bas[] = {" + ",".join(f"mus_{k.lower()}_bas" for k in ORDER) + "};")
lines.append(carr("song_env", [songs[k][2] for k in ORDER]))
open(OUT, "w").write("\n".join(lines) + "\n")
with open(os.path.join(os.path.dirname(OUT), "songs.h"), "w") as f:
    f.write("// generated by tools/gen_sound.py\n#ifndef SONGS_H\n#define SONGS_H\n")
    f.write("enum { " + ", ".join(f"MUS_{k}" for k in ORDER) + ", MUS_COUNT };\n#endif\n")
for k in ORDER:
    m = songs[k][0]
    print(k, "melody frames:", sum(m[1::2]), "bass frames:", sum(songs[k][1][1::2]))
