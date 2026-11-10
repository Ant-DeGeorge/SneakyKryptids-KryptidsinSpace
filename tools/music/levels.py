"""Mission stage and boss themes for Sneaky Kryptid (hUGEDriver). All compositions original.

Every theme quotes the title hook from "Bounty Call": root, down a fourth, root, step, step, step, root
(D5 A4 D5 E5 F5 E5 D5), recast in each mission's key and style.

  STAGE, BOSS, STAGE5, FLATWOODS: aggressive EDM, see edm.py (round 2 notes from the user)
  STAGE2    "Static Charge"   Stormwall Nebula E minor, 16th synth arps, saw bass, 150 BPM
  TBIRD     "Thunderhead"     Thunderbird      B minor, gallop bass, thunder crashes, 179 BPM
  STAGE3    "Wisp Hollow"     Barrens Belt     A minor 6/8 spooky folk jig, drone fiddle
  JDEVIL    "Leeds Point"     Jersey Devil     G harmonic minor 6/8 tarantella, 120 BPM (dotted)
  STAGE4    "Glass Moon"      Frostbite Moon   F# minor, glass bells, 112 BPM
  WENDIGO   "Hunger Howl"     Wendigo          C minor, icy 16ths, half-step bass, howls, 150 BPM
"""
import math, re
from score import Score, midi, NAMES

def wave_from_fn(f, amp=1.0):
    return [max(0, min(15, int(round(7.5 + 7.5 * amp * f(i / 32))))) for i in range(32)]

W_PUNCH = wave_from_fn(lambda x: 0.75 * max(-1, min(1, 1.6 * math.sin(2 * math.pi * x) + 0.35 * math.sin(4 * math.pi * x))))
W_TRI = wave_from_fn(lambda x: 1 - 4 * abs(x - 0.5))
W_SAW = wave_from_fn(lambda x: 0.8 * (1 - 2 * x))
W_SINE = wave_from_fn(lambda x: math.sin(2 * math.pi * x))
W_GLASS = wave_from_fn(lambda x: 0.7 * math.sin(2 * math.pi * x) + 0.3 * math.sin(6 * math.pi * x))
W_ROUND = wave_from_fn(lambda x: 0.8 * math.sin(2 * math.pi * x) + 0.2 * math.sin(6 * math.pi * x))

# ---------------------------------------------------------------- note helpers
def nm(m):
    return NAMES[m % 12] + str(m // 12 - 1)

def tr(text, semis):
    """Transpose every note token in a score text."""
    out = []
    for t in text.split():
        if t == '|' or t.startswith('@'): out.append(t); continue
        n, sep, rest = t.partition(':')
        if n not in ('R', '-'): n = nm(midi(n) + semis)
        out.append(n + sep + rest)
    return ' '.join(out)

QUAL = {'m': (0, 3, 7), '': (0, 4, 7), 'aug': (0, 4, 8), '7': (0, 4, 10), 'dim': (0, 3, 6)}

def chord(name):
    """'F#m' -> (pitch class, intervals)."""
    m = re.fullmatch(r'([A-G][#b]?)(m|aug|7|dim)?', name)
    root, q = m.group(1), m.group(2) or ''
    return midi(root + '4') % 12, QUAL[q]

def place(pc, lo):
    """Lowest midi note of pitch class pc at or above note lo."""
    b = midi(lo)
    return b + (pc - b) % 12

def bars(chords, bar_rows=16):
    """'Dm | Bb:2 C:2' -> [(name, rows)] (':n' = beats of bar_rows/4 rows)."""
    out = []
    for tok in chords.split():
        if tok == '|': continue
        name, _, beats = tok.partition(':')
        out.append((name, int(beats) * bar_rows // 4 if beats else bar_rows))
    return out

def arp(chords, pulse=4, lo='C4', bar_rows=16):
    """Chord retriggered every `pulse` rows with a 0xy arpeggio."""
    out = []
    for name, rows in bars(chords, bar_rows):
        pc, iv = chord(name)
        r = nm(place(pc, lo))
        for _ in range(rows // pulse):
            out.append(f'{r}:{pulse}/0{iv[1]:X}{iv[2]:X}')
    return ' '.join(out)

def broken(chords, figure, step=2, lo='C5', bar_rows=16):
    """Broken-chord ostinato: figure indexes chord tones (0 1 2, 3 = octave root, -1 = fifth below)."""
    out = []
    for name, rows in bars(chords, bar_rows):
        pc, iv = chord(name)
        root = place(pc, lo)
        tones = {0: root, 1: root + iv[1], 2: root + iv[2], 3: root + 12, 4: root + 12 + iv[1], -1: root + iv[2] - 12}
        for k in range(rows // step):
            out.append(f'{nm(tones[figure[k % len(figure)]])}:{step}')
    return ' '.join(out)

def bassline(chords, offsets, step=2, lo='E1', bar_rows=16, fifth=None):
    """Bass figure from semitone offsets off each chord root (rows per note = step). 'f' = the chord's fifth."""
    out = []
    for name, rows in bars(chords, bar_rows):
        pc, iv = chord(name)
        root = place(pc, lo)
        n = rows // step
        for k in range(n):
            o = offsets[k % len(offsets)]
            if o == 'R': out.append(f'R:{step}'); continue
            fx = ''
            if isinstance(o, tuple): o, fx = o[0], '/' + o[1]
            if o == 'f': o = iv[2]
            if o == 'F': o = iv[2] - 12 if root + iv[2] - 12 >= midi('C1') else iv[2]
            out.append(f'{nm(root + o)}:{step}{fx}')
    return ' '.join(out)

def echo(sc, ch, text, delay, semis=0, instr=None, bars=16):
    """Play text on ch `delay` rows late (an echo/slapback), trimmed so the channel stays level."""
    if instr: text = re.sub(r'@\d+', f'@{instr}', text)
    if semis: text = tr(text, semis)
    n0 = len(sc.rows[ch])
    sc.play(ch, f'R:{delay} ' + text, bars=0)
    del sc.rows[ch][len(sc.rows[ch]) - delay:]

def kit(sc, soft=False):
    v = 0 if not soft else -3
    sc.noise(1, 'Kick', vol=13 + v, decay=1)
    sc.noise(2, 'Snare', vol=12 + v, decay=2)
    sc.noise(3, 'Hat', vol=6 + min(v, 0) // 2, decay=1, metallic=True)
    sc.noise(4, 'Open hat', vol=5, decay=3, metallic=True)
    sc.noise(5, 'Crash', vol=11 + v, decay=5)
    sc.drums = {'k': (1, 22), 's': (2, 41), 'h': (3, 60), 'o': (4, 57), 'c': (5, 33), 't': (2, 30), 'r': (2, 47)}


# ================================================================ 3. STAGE2: "Static Charge" (Stormwall Nebula)
def static_charge():
    sc = Score('Static Charge', 6)                   # 150 BPM
    sc.duty(1, 'Arp 12.5% tick', duty=0, vol=8, decay=1)
    sc.duty(2, 'Lead 25% electric', duty=1, vol=13, decay=0)
    sc.duty(3, 'Zap', duty=0, vol=12, decay=0)
    sc.duty(4, 'Lead 50% storm', duty=2, vol=13, decay=0)
    sc.wave(1, 'Saw bass', 0, level=1)
    sc.waveform(0, W_SAW)
    kit(sc)
    FIG = [0, 1, 2, 3, 2, 1, 2, 3]                   # rising/falling 16ths

    sc.play(1, '@1 ' + broken('Em | Em | C | D', FIG, step=1, lo='E4'))
    sc.play(2, '@3 R:16 | R:12 C7:4/*2C0 | R:16 | R:12 C7:4/*2C0')
    sc.play(3, 'R:16 | R:16 | @1 ' + bassline('C | D', [0, 12], lo='B1'))
    sc.drum('................ ............c... k.hhs.h.k.hhs.h. k.s.k.s.ssssssss')

    A_ch = 'Em | C | D | Bm | Em | C | Am | B'
    A_lead = ('@2 E5:4 B4:2 E5:2 F#5:2 G5:2 F#5:2 E5:2 | G5:6/422 E5:2 C5:8/432 | D5:4 F#5:2 A5:2 A5:4 G5:2 F#5:2 | F#5:12/433 B4:2 D5:2 | '
              'E5:4 B4:2 E5:2 F#5:2 G5:2 A5:2 B5:2 | C6:6/422 B5:2 A5:4 G5:4 | A5:4 F#5:2 E5:2 A5:4 B5:2 A5:2 | B5:8/433 D#5:4 F#5:4')
    B_ch = 'C | D | Em | Em | C | D | B | B'
    B_lead = ('@4 G5:2 G5:2 R:2 G5:2 E5:4 G5:4 | A5:2 A5:2 R:2 A5:2 F#5:4 A5:4 | B5:8/00C B5:8/433 | G5:4 F#5:4 E5:4 B4:4 | '
              'C5:2 E5:2 G5:2 C6:2 B5:4 G5:4 | D5:2 F#5:2 A5:2 D6:2 C6:4 A5:4 | B5:16/00C | A5:4 F#5:4 D#5:4 F#5:4')
    beat = 'k.hhs.h.k.hhs.hk'
    sc.mark('A')
    sc.play(1, '@1 ' + broken(A_ch, FIG, step=1, lo='E4'))
    sc.play(2, A_lead)
    sc.play(3, '@1 ' + bassline(A_ch, [0, 12], lo='B1'))
    sc.drum('c.hhs.h.k.hhs.hk' + beat * 6 + 'k.hhs.h.s.s.ssss')
    sc.play(1, '@1 ' + broken(B_ch, [0, 2, 3, 2], step=1, lo='E4'))
    sc.play(2, B_lead)
    sc.play(3, '@1 ' + bassline(B_ch, [0, 0, 12, 0], step=1, lo='B1'))
    sc.drum('c.hhs.h.k.hhs.hk' + beat + 'c.hhs.h.k.hhs.hk' + beat + 'c.hhs.h.k.hhs.hk' + beat + 'c.hhs.h.k.hhs.hk' + 'k.s.k.s.ssssssss')
    # A' with a zap answering every 2 bars on ch1 between arps
    sc.play(1, '@1 ' + broken(A_ch, FIG, step=1, lo='E5'))
    sc.play(2, A_lead)
    sc.play(3, '@1 ' + bassline(A_ch, [0, 12], lo='B1'))
    sc.drum('c.hhs.h.k.hhs.hk' + beat * 6 + 'k.hhs.h.s.s.ssss')
    return sc.build(loop_to='A')


# ================================================================ 4. TBIRD: "Thunderhead" (Thunderbird)
def thunderhead():
    sc = Score('Thunderhead', 5)                     # 179 BPM
    sc.duty(1, 'Arp 12.5%', duty=0, vol=9, decay=1)
    sc.duty(2, 'Lead 50%', duty=2, vol=13, decay=0)
    sc.duty(3, 'Thunder drop', duty=2, vol=14, decay=3)
    sc.duty(4, 'Lead 25% high', duty=1, vol=12, decay=0)
    sc.wave(1, 'Bass punch', 0, level=1)
    sc.waveform(0, W_PUNCH)
    kit(sc)
    sc.noise(5, 'Thunder', vol=14, decay=7)
    GALLOP = [0, 0, 12, 0, 0, 12, 0, 12]

    sc.play(1, 'R:16 | R:16 | @1 ' + arp('Bm | F#', pulse=2, lo='F#3'))
    sc.play(2, '@3 B4:8/*208 R:8 | F#4:8/*208 R:8 | @2 B4:4 R:4 B4:4 R:4 | A#4:4 C#5:4 E5:4 F#5:4')
    sc.play(3, 'R:16 | R:16 | @1 ' + bassline('Bm | F#', GALLOP, lo='F#1'))
    sc.drum('c............... c............... k.h.s.hkk.h.s.hk k.s.k.s.ssssssss')

    A_ch = 'Bm | G | A | F# | Bm | G | Em | F#'
    A_lead = ('@2 B4:4 F#4:2 B4:2 C#5:2 D5:2 C#5:2 B4:2 | D5:6 E5:2 D5:4 B4:4 | C#5:4 E5:2 A5:2 A5:4 G5:2 F#5:2 | F#5:12/433 E5:2 C#5:2 | '
              'B5:4 F#5:2 B5:2 C#6:2 D6:2 C#6:2 B5:2 | D6:6 B5:2 G5:4 B5:4 | E6:4 D6:2 B5:2 G5:4 E5:4 | F#5:4 A#5:4 C#6:4 E6:4')
    B_ch = 'G | A | Bm | Bm | G | A | F# | F#'
    B_lead = ('@4 G5:2 F#5:2 G5:2 B5:2 D6:8/433 | C#6:2 B5:2 A5:2 C#6:2 E6:8/433 | D6:4 C#6:4 B5:4 F#5:4 | B5:12/443 R:4 | '
              'G5:2 B5:2 D6:2 G6:2 F#6:4 D6:4 | A5:2 C#6:2 E6:2 A6:2 G6:4 E6:4 | F#6:8/433 E6:4 C#6:4 | A#5:4 C#6:4 F#5:8')
    beat = 'k.h.s.hkk.h.s.hk'
    sc.mark('A')
    sc.play(1, '@1 ' + arp(A_ch, pulse=2, lo='F#3'))
    sc.play(2, A_lead)
    sc.play(3, '@1 ' + bassline(A_ch, GALLOP, lo='F#1'))
    sc.drum('c.h.s.hkk.h.s.hk' + beat * 3 + 'c.h.s.hkk.h.s.hk' + beat * 2 + 'k.h.s.hks.s.ssss')
    sc.play(1, '@1 ' + arp(B_ch, pulse=1, lo='F#3'))
    sc.play(2, B_lead)
    sc.play(3, '@1 ' + bassline(B_ch, [0, 0, 12, 0], step=1, lo='F#1'))
    sc.drum('c.hhs.hkk.hhs.hk' + 'k.hhs.hkk.hhs.hk' + 'c.hhs.hkk.hhs.hk' + 'k.hhs.hkkkkks.ss' + 'c.hhs.hkk.hhs.hk' + 'k.hhs.hkk.hhs.hk' + 'c.hhs.hkk.hhs.hk' + 'k.s.k.s.ssssssss')
    return sc.build(loop_to='A')


# ================================================================ 5. STAGE3: "Wisp Hollow" (Barrens Belt), 6/8
def wisp_hollow():
    sc = Score('Wisp Hollow', 6)                     # 6/8, dotted quarter = 100 BPM
    sc.duty(1, 'Drone 12.5%', duty=0, vol=6, decay=0)
    sc.duty(2, 'Fiddle 50%', duty=2, vol=11, decay=0)
    sc.duty(3, 'Ghost echo', duty=1, vol=4, decay=2)
    sc.duty(4, 'Wisp twinkle', duty=0, vol=8, decay=2)
    sc.wave(1, 'Folk bass', 0, level=1)
    sc.waveform(0, W_TRI)
    kit(sc, soft=True)
    B = 12
    folk = lambda ch: bassline(ch, [0, 'R', 'F', 'R'], step=3, lo='E1', bar_rows=B)

    sc.play(1, '@1 ' + arp('Am | Am', pulse=12, lo='A3', bar_rows=B).replace('/037', '/070'), bars=B)
    sc.play(2, 'R:12 | @2 R:6 E4:2 G4:2 G#4:2', bars=B)
    sc.play(3, '@1 A1:6 R:6 | A1:6 E1:6', bars=B)
    sc.drum('......h.....' + '......h.s.s.', bars=B)

    A_ch = 'Am | Am | G | G | F | G | Am | E'
    A_lead = ('@2 A4:4 E4:2 A4:2 B4:2 C5:2 | B4:2 A4:2 G#4:2 A4:6/423 | B4:4 G4:2 B4:2 C5:2 D5:2 | C5:2 B4:2 A4:2 G4:6/423 | '
              'A4:2 C5:2 F5:2 E5:4 D5:2 | D5:2 B4:2 G4:2 B4:4 D5:2 | C5:2 E5:2 A5:2 G#5:4 E5:2 | E5:6/433 D5:2 C5:2 B4:2')
    B_ch = 'Dm | Dm | Am | Am | F | E | Am | E'
    B_lead = ('@2 D5:4 A4:2 D5:2 E5:2 F5:2 | E5:2 D5:2 C#5:2 D5:6/423 | E5:4 C5:2 E5:2 A5:2 G#5:2 | A5:6/433 E5:6 | '
              'F5:2 E5:2 D5:2 C5:4 A4:2 | B4:2 D5:2 E5:2 G#5:4 E5:2 | A5:2 G#5:2 E5:2 C5:2 B4:2 A4:2 | B4:2 C5:2 B4:2 G#4:6/423')
    drone = lambda ch: arp(ch, pulse=12, lo='A3', bar_rows=B).replace('/037', '/070').replace('/047', '/070')
    jig = 'k.h.h.s.h.h.'
    sc.play(1, '@1 ' + drone(A_ch), bars=B)
    sc.play(2, A_lead, bars=B)
    sc.play(3, '@1 ' + folk(A_ch), bars=B)
    sc.drum(jig * 7 + 'k.h.h.s.s.ss', bars=B)
    # B: wisps twinkle up high on ch1
    sc.play(1, '@4 ' + broken(B_ch, [3, 2, 0, 2, 3, 1], step=2, lo='A5', bar_rows=B), bars=B)
    sc.play(2, B_lead, bars=B)
    sc.play(3, '@1 ' + folk(B_ch), bars=B)
    sc.drum(jig * 3 + 'k.h.h.s.h.hh' + jig * 3 + 'k.s.s.s.ssss', bars=B)
    # A': the fiddle with a ghost echo a dotted eighth behind, an octave down
    sc.play(2, A_lead, bars=B)
    echo(sc, 1, A_lead, 3, semis=-12, instr=3, bars=B)
    sc.play(3, '@1 ' + folk(A_ch), bars=B)
    sc.drum(jig * 7 + 'k.h.h.s.s.ss', bars=B)
    # turnaround (2 bars) back to the drone
    sc.play(1, '@1 ' + drone('Am | E'), bars=B)
    sc.play(2, '@2 A4:6/423 R:6 | R:6 E4:2 G4:2 G#4:2', bars=B)
    sc.play(3, '@1 A1:6 R:6 | E1:6 R:6', bars=B)
    sc.drum('k.....h.....' + 'k.h.h.s.s.s.', bars=B)
    return sc.build()


# ================================================================ 6. JDEVIL: "Leeds Point" (Jersey Devil), 6/8
def leeds_point():
    sc = Score('Leeds Point', 5)                     # 6/8, dotted quarter = 120 BPM
    sc.duty(1, 'Stab 25%', duty=1, vol=9, decay=1)
    sc.duty(2, 'Fiddle 50% hot', duty=2, vol=13, decay=0)
    sc.duty(3, 'Screech', duty=0, vol=13, decay=0)
    sc.duty(4, 'Lead 25% wild', duty=1, vol=13, decay=0)
    sc.wave(1, 'Bass punch', 0, level=1)
    sc.waveform(0, W_PUNCH)
    kit(sc)
    B = 12
    TAR = [0, 'f', 12, 0, 'f', 12]
    stab = lambda ch: ' '.join(t if k % 3 != 1 else 'R:2' for k, t in enumerate(arp(ch, pulse=2, lo='G3', bar_rows=B).split()))

    sc.play(1, 'R:12 | R:12 | @1 ' + stab('Gm | D'), bars=B)
    sc.play(2, '@3 G6:12/*20C | D6:12/*20C | @2 G4:2 R:4 G4:2 R:4 | F#4:2 A4:2 C5:2 D5:6', bars=B)
    sc.play(3, 'R:12 | R:12 | @1 ' + bassline('Gm | D', TAR, lo='D1', bar_rows=B), bars=B)
    sc.drum('c........... c........kkk k.s.h.k.s.h. k.s.s.ssssss', bars=B)

    A_ch = 'Gm | Gm | Eb | D | Gm | Cm | D | D'
    A_lead = ('@2 G4:4 D4:2 G4:2 A4:2 Bb4:2 | A4:2 G4:2 F#4:2 G4:6/423 | Bb4:4 G4:2 Bb4:2 C5:2 D5:2 | Eb5:2 D5:2 C5:2 D5:6/433 | '
              'G5:4 D5:2 G5:2 A5:2 Bb5:2 | C6:2 Bb5:2 A5:2 G5:4 Eb5:2 | D5:2 F#5:2 A5:2 C6:4 A5:2 | F#5:2 A5:2 F#5:2 D5:6/433')
    B_ch = 'Cm | Cm | Gm | Gm | Eb | D | Gm | D'
    B_lead = ('@4 C5:1 D5:1 Eb5:1 F5:1 G5:2 Eb5:2 C5:4 | G5:2 F5:2 Eb5:2 D5:2 C5:4 | D5:1 Eb5:1 F#5:1 G5:1 A5:2 Bb5:2 G5:4 | Bb5:2 A5:2 G5:2 D5:6/433 | '
              'Eb5:2 G5:2 Bb5:2 Eb6:4 D6:2 | C6:2 Bb5:2 A5:2 F#5:6/433 | G5:2 D5:2 Bb4:2 G5:2 A5:2 Bb5:2 | A5:2 F#5:2 D5:2 A4:2 C5:2 F#5:2')
    beat = 'k.hhs.k.hhs.'
    sc.play(1, '@1 ' + stab(A_ch), bars=B)
    sc.play(2, A_lead, bars=B)
    sc.play(3, '@1 ' + bassline(A_ch, TAR, lo='D1', bar_rows=B), bars=B)
    sc.drum('c.hhs.k.hhs.' + beat * 6 + 'k.s.s.ssssss', bars=B)
    sc.play(1, '@1 ' + arp(B_ch, pulse=2, lo='G3', bar_rows=B), bars=B)
    sc.play(2, B_lead, bars=B)
    sc.play(3, '@1 ' + bassline(B_ch, [0, 12], step=1, lo='D1', bar_rows=B), bars=B)
    sc.drum('c.hhs.k.hhs.' + beat * 3 + 'c.hhs.k.hhs.' + beat * 2 + 'k.s.k.ssssss', bars=B)
    return sc.build()


# ================================================================ 7. STAGE4: "Glass Moon" (Frostbite Moon)
def glass_moon():
    sc = Score('Glass Moon', 8)                      # 112 BPM
    sc.duty(1, 'Glass bell 12.5%', duty=0, vol=9, decay=2)
    sc.duty(2, 'Lead 12.5% crystal', duty=0, vol=12, decay=0)
    sc.duty(3, 'Lead 25% sparkle', duty=1, vol=11, decay=0)
    sc.wave(1, 'Ice bass', 0, level=1)
    sc.waveform(0, W_GLASS)
    kit(sc, soft=True)
    sc.noise(3, 'Ice tick', vol=5, decay=1, metallic=True)
    BELL = [0, 2, 1, 3, 2, 1, 4, 2]

    sc.play(1, '@1 ' + broken('F#m | D | F#m | C#', BELL, step=2, lo='F#4'))
    sc.play(2, 'R:16 | R:16 | R:16 | @2 R:8 C#5:4 E5:4')
    sc.play(3, 'R:16 | R:16 | @1 F#1:12 C#2:4 | C#2:12 G#1:4')
    sc.drum('................ ......h.......h. k.......s....h.. k.......s.s.s.hh')

    A_ch = 'F#m | D | A | E | F#m | D | Bm | C#'
    A_lead = ('F#5:4/424 C#5:2 F#5:2 G#5:2 A5:2 G#5:2 F#5:2 | A5:8/423 F#5:4 D5:4 | E5:4 A5:4 C#6:4 B5:4 | G#5:12/423 R:4 | '
              'F#5:4 C#5:2 F#5:2 G#5:2 A5:2 B5:2 C#6:2 | D6:8/423 C#6:4 A5:4 | B5:4 F#5:4 D5:4 F#5:4 | G#5:8/423 F5:4 C#5:4')
    B_ch = 'Bm | Bm | D | D | F#m | F#m | E | C#'
    B_lead = ('D6:4 C#6:4 B5:8/423 | F#5:4 A5:4 B5:8/423 | F#6:4 E6:4 D6:8/423 | A5:4 D6:4 E6:8/423 | '
              'C#6:6 A5:2 F#5:8/423 | A5:4 G#5:4 F#5:8/423 | G#5:4 B5:4 E6:8/423 | F5:4 G#5:4 C#6:8/423')
    bass = lambda ch: bassline(ch, [0, 0, 0, 0, 0, 0, 'f', 12], step=2, lo='E1')
    tick = 'k.......s....h..'
    sc.mark('A')
    sc.play(1, '@1 ' + broken(A_ch, BELL, step=2, lo='F#4'))
    sc.play(2, '@2 ' + A_lead)
    sc.play(3, '@1 ' + bass(A_ch))
    sc.drum(tick * 7 + 'k.......s.s.s.hh')
    sc.play(1, '@1 ' + broken(B_ch, [3, 2, 1, 0, 1, 2, 3, 4], step=2, lo='F#4'))
    sc.play(2, '@3 ' + B_lead)
    sc.play(3, '@1 ' + bassline(B_ch, [0, 'f'], step=8, lo='E1'))
    sc.drum(('k.......h.......' + '........s.....h.') * 3 + 'k.......h....... k.......s.s.ssss')
    # A' an octave of sparkle: lead doubled by the bell part playing the hook a beat late
    sc.play(1, '@1 ' + broken(A_ch, BELL, step=2, lo='F#5'))
    sc.play(2, '@3 ' + A_lead)
    sc.play(3, '@1 ' + bass(A_ch))
    sc.drum('k...h...s...h...' * 7 + 'k...h...s.s.s.hh')
    return sc.build(loop_to='A')


# ================================================================ 8. WENDIGO: "Hunger Howl"
def hunger_howl():
    sc = Score('Hunger Howl', 6)                     # 150 BPM
    sc.duty(1, 'Ice 12.5% tick', duty=0, vol=8, decay=1)
    sc.duty(2, 'Lead 50% cold', duty=2, vol=13, decay=0)
    sc.duty(3, 'Howl 25%', duty=1, vol=13, decay=0)
    sc.duty(4, 'Tremolo', duty=0, vol=9, decay=0)
    sc.wave(1, 'Bass punch', 0, level=1)
    sc.waveform(0, W_PUNCH)
    kit(sc)
    OST = [0, 0, 12, 0, 1, 0, 12, 0]                 # C C C' C Db C C' C

    sc.play(1, '@4 ' + ' '.join(['G4:1 Ab4:1'] * 16) + ' | @1 ' + broken('Cm | G', [3, 2, 1, 0], step=1, lo='C5'))
    sc.play(2, '@3 C5:4 G5:12/*304 | G5:16/448 | Eb5:4 C5:12/*304 | B4:8/448 D5:4 G5:4')
    sc.play(3, 'R:16 | R:16 | @1 ' + bassline('Cm | G', OST, lo='G1'))
    sc.drum('c............... ................ k..hs..hk.k.s..h k.s.k.s.ssssssss')

    A_ch = 'Cm | Ab | Cm | G | Cm | Ab | Fm | G'
    A_lead = ('@2 C5:4 G4:2 C5:2 D5:2 Eb5:2 D5:2 C5:2 | Eb5:8/433 C5:4 Ab4:4 | G5:4 Eb5:2 G5:2 Ab5:4 G5:4 | B4:4 D5:4 G5:8/433 | '
              'C6:8/433 G5:4 Eb5:4 | Ab5:4 G5:4 F5:4 Eb5:4 | F5:4 Ab5:2 C6:2 Ab5:4 F5:4 | G5:4 Ab5:2 G5:2 F5:2 Eb5:2 D5:2 B4:2')
    B_ch = 'Ab | Ab | Fm | Fm | Cm | Cm | Ab:2 Bb:2 | G'
    B_lead = ('@3 C6:12/448 Bb5:2 Ab5:2 | Eb5:12/448 F5:2 G5:2 | Ab5:12/448 G5:2 F5:2 | C5:16/448 | '
              'G5:4 C6:4 Eb6:8/448 | D6:4 C6:4 G5:8 | Ab5:4 C6:4 Bb5:4 D6:4 | B5:16/448')
    beat = 'k..hs..hk.k.s..h'
    sc.mark('A')
    sc.play(1, '@1 ' + broken(A_ch, [0, 1, 2, 3, 4, 3, 2, 1], step=1, lo='C5'))
    sc.play(2, A_lead)
    sc.play(3, '@1 ' + bassline(A_ch, OST, lo='G1'))
    sc.drum('c..hs..hk.k.s..h' + beat * 6 + 'k..hs..hs.s.ssss')
    sc.play(1, '@4 ' + broken(B_ch, [0, 1], step=1, lo='C5'))
    sc.play(2, B_lead)
    sc.play(3, '@1 ' + bassline(B_ch, [0, 'R', 0, 12], step=4, lo='G1'))
    sc.drum(('c.......s.......' + 'k.......s.....kk') * 3 + 'c.......s....... k.s.k.s.ssssssss')
    return sc.build(loop_to='A')


SONGS = {'STAGE2': static_charge, 'TBIRD': thunderhead, 'STAGE3': wisp_hollow, 'JDEVIL': leeds_point,
         'STAGE4': glass_moon, 'WENDIGO': hunger_howl}
import edm
SONGS.update(edm.SONGS)
