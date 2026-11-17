"""Sneaky Kryptid OST, "Frostbite" overhaul (2026-11-17). All compositions original.

Every song is built from the DNA of the user's track "Frostbite Circuit" (sneakykryptid, Suno instrumental,
~143 BPM, D Phrygian): an icy shimmer over a low drone, a breakbeat, a palm-muted chug riff that rubs the
root against the flat 2nd and flat 7th, turntable stabs, and a bridge stripped to bass and glass.
On the Game Boy the closest steady tempo is 149 BPM (6 ticks per row); bosses run at 179 (5 ticks).

Shared vocabulary
  riff      ch1 palm-muted chug (25% duty, fast decay, notes cut between hits): x root, 2 flat 2nd,
            7 flat 7th below, 5 fifth, 3 minor 3rd, 4 4th, o octave, - hold, . cut
  hook      the title hook: root, root, down a 4th, root, flat 2nd, root, flat 7th, 5th below
            (D5 D5 A4 D5 Eb5 D5 C5 A4), recast in every song's key
  scratch   ch2 stabs that slide down while they sound (2xx on every row) = turntable "zip"
  glass     12.5% duty bell arps / glass waveform on ch3, fading
  break     breakbeat on ch4 (kick, ghost snare, snare on 2 and 4, rolls)

  TITLE     Frostbite Circuit   D Phrygian 149    title screen, mission screen, intro briefings (GBC cover of the source)
  MAP       Cold Boot           D minor    128    arcade ship select
  BRIEF     Thin Ice            D Phrygian 112    debrief cutscenes
  STAGE     Rime Runner         E Phrygian 149    M1 Rimeveil Lane, Escape Run
  BOSS      Riff Engine         F# Phrygian 179   mid-bosses, Space Kraken, Escape Run / Tree Line bruiser
  WARN      Whiteout            D          jingle boss warning
  STAGE2    Arc Breaker         G Phrygian 149    M2 Stormwall Nebula
  TBIRD     Thunder Riff        B Phrygian 179    Thunderbird
  STAGE3    Wisp Breaks         A Phrygian 128    M3 Barrens Belt, Tree Line path
  JDEVIL    Leeds Breakdown     G Phrygian dominant 179   Jersey Devil
  STAGE4    Frost Line          D Phrygian 149    M4 Frostbite Moon, Tree Line wreck
  WENDIGO   Hunger Engine       C Phrygian 179    Wendigo, The Watcher
  STAGE5    Dead Air            C# Phrygian 128   M5 Flatwoods Drift (gun cam)
  FLATWOODS Braxton Breaker     Bb Phrygian 179   Flatwoods Monster, Escape Run chase
  CLEAR     Thaw                D major    jingle
  OVER      Frozen Out          D Phrygian jingle
  END       Frozen Shimmer      D Phrygian 112    ending (the riff decays into frozen shimmer)
"""
import math, re
from score import Score, midi
from levels import nm, tr, arp, broken, bassline, wave_from_fn, W_SINE, W_GLASS, W_SAW

W_REESE = wave_from_fn(lambda x: 0.9 * (1 - 2 * x) * 0.6 + 0.4 * (1 if x < 0.5 else -1) * 0.9)
W_GRIT = wave_from_fn(lambda x: 0.85 * max(-1, min(1, 2.2 * math.sin(2 * math.pi * x))) * 0.8 + 0.2 * (1 - 2 * x))
W_ICE = wave_from_fn(lambda x: 0.55 * math.sin(2 * math.pi * x) + 0.3 * math.sin(10 * math.pi * x) + 0.15 * math.sin(14 * math.pi * x))


def setup(sc, bass=W_REESE):
    sc.duty(1, 'Chug 25% mute', duty=1, vol=13, decay=1)
    sc.duty(2, 'Lead 25%', duty=1, vol=13, decay=0)
    sc.duty(3, 'Glass 12.5%', duty=0, vol=10, decay=2)
    sc.duty(4, 'Scratch 50%', duty=2, vol=14, decay=1)
    sc.duty(5, 'Riser', duty=0, vol=11, decay=0)
    sc.duty(6, 'Lead 50% soft', duty=2, vol=11, decay=0)
    sc.duty(7, 'Alarm', duty=1, vol=12, decay=0)
    sc.duty(8, 'Chug 50% heavy', duty=2, vol=14, decay=1)
    sc.wave(1, 'Riff bass', 0, level=1)
    sc.wave(2, 'Glass pad', 1, level=2)
    sc.wave(3, 'Drone', 2, level=2)
    sc.wave(4, 'Ice bells', 3, level=1)
    sc.waveform(0, bass)
    sc.waveform(1, W_GLASS)
    sc.waveform(2, W_SINE)
    sc.waveform(3, W_ICE)
    sc.noise(1, 'Kick', vol=15, decay=1)
    sc.noise(2, 'Snare', vol=13, decay=2)
    sc.noise(3, 'Hat', vol=6, decay=1, metallic=True)
    sc.noise(4, 'Open hat', vol=7, decay=2, metallic=True)
    sc.noise(5, 'Crash', vol=12, decay=5)
    sc.noise(6, 'Roll', vol=11, decay=1)
    sc.noise(7, 'Ghost', vol=6, decay=1)
    sc.drums = {'k': (1, 18), 's': (2, 41), 'h': (3, 60), 'o': (4, 55), 'c': (5, 33), 'r': (6, 44), 'g': (7, 43)}


# ---------------------------------------------------------------- building blocks
OFF = {'x': 0, 'o': 12, '2': 1, '7': -2, '5': 7, '3': 3, '4': 5, '6': 8, 'f': -5}

def riff(chords, pats, lo='C3', instr=1):
    """Chug riff: one pattern (16 chars) per bar, cycled; each bar's root is the chord root at/above lo."""
    from levels import bars, chord, place
    out = [f'@{instr}']
    for k, (name, rows) in enumerate(bars(chords)):
        pc, _ = chord(name)
        root = place(pc, lo)
        pat = pats[k % len(pats)].replace(' ', '')
        assert len(pat) == rows, pat
        toks = []
        for c in pat:
            if c == '-': toks[-1][1] += 1
            elif c == '.': toks.append(['R', 1])
            else: toks.append([nm(root + OFF[c]), 1])
        out += [f'{n}:{d}' for n, d in toks]
        out.append('|')
    return ' '.join(out[:-1])


def scratch(note, rows=2, speed='30'):
    return f'{note}:{rows}/*2{speed}'


def hook(key, octave=5, tail='A'):
    """The title hook in any key (one bar). tail A falls to the 5th below, tail B climbs to the 3rd."""
    r = midi(key + str(octave))
    n = lambda s: nm(r + s)
    if tail == 'A':
        return f'{n(0)}:2 R:1 {n(0)}:1 {n(-5)}:2 {n(0)}:2 {n(1)}:3 {n(0)}:1 {n(-2)}:2 {n(-5)}:2'
    return f'{n(0)}:2 R:1 {n(0)}:1 {n(-5)}:2 {n(0)}:2 {n(1)}:3 {n(0)}:1 {n(3)}:2 {n(7)}:2'


GLASS = [0, 1, 2, 3, 4, 3, 2, 1]
def glass(chords, lo='D5', step=1, fig=GLASS, instr=3):
    return f'@{instr} ' + broken(chords, fig, step=step, lo=lo)


BRK_A = 'k.h.s.hkh.k.s.ho'
BRK_B = 'k.hgs.h.hgkks.hs'
BRK_C = 'k.hks.hgk.hks.ho'
BRK_FILL = 'k.s.k.ssk.ssrrrr'
HUGE_A = 'c.oks.okk.oks.ok'
HUGE_B = 'k.oks.okk.oksrsr'
HALF = 'k.h.h.h.s.h.h.hk'
FAST_A = 'k.kks.hkk.kks.hs'            # 179 BPM double-kick
FAST_B = 'kkhks.kkk.hks.ks'

def beat(n, pats, crash=True, fill=True):
    out = [pats[k % len(pats)] for k in range(n)]
    if crash: out[0] = 'c' + out[0][1:]
    if fill: out[-1] = BRK_FILL
    return ' '.join(out)

ROOT_8 = [0, 'R', 0, 12, 0, 'R', 0, 1]                  # reese bass on 8ths with octave pop and flat-2 pickup
ROOT_16 = [0, 0, 'R', 0, 12, 0, 'R', 0, 0, 0, 'R', 0, 12, 0, -2, 0]


# ================================================================ TITLE: "Frostbite Circuit" (D Phrygian, 149 BPM)
def frostbite_circuit():
    sc = Score('Frostbite Circuit', 6)
    setup(sc)
    # intro (8 bars): sparse icy shimmer over a low drone, the hook far away
    sc.play(1, 'R:16 | ' + glass('Dm | Eb | Gm | Dm | Eb | Gm', lo='D5', step=2) + ' | @5 D4:16/*101')
    sc.play(2, 'R:16 | R:16 | R:16 | R:16 | @6 ' + hook('D', 5) + ' | Eb5:8/442 D5:8/442 | ' + hook('D', 5, 'B') + ' | A5:16/443')
    sc.play(3, '@3 D2:16/422 | -:16 | Eb2:16/422 | -:16 | D2:16/422 | -:16 | G1:16/422 | A1:12 R:4')
    sc.drum('................ ................ ................ ........h.......'
            ' h...h...h...h... h...h...h...h.h. h.h.h.h.h.h.h.h. r.r.r.r.rrrrrrrr')
    # build (4 bars): breakbeat enters, palm-muted riff
    RA = ['x.x.x7x.x.x2x.x.', 'x.x.x7x.x.x2x.x.', 'x.x.x7x.x.x2x.x.', 'x.xx.xx.xxxxo-o-']
    sc.play(1, riff('Dm | Eb | Gm | Dm', RA, lo='D3'))
    sc.play(2, 'R:16 | R:16 | R:16 | @5 D5:16/*101')
    sc.play(3, '@1 ' + bassline('Dm | Eb | Gm', [0, 'R', 'R', 'R', 0, 'R', 0, 'R'], step=2, lo='C2') + ' | D2:8 R:4 D2:2 D2:2')
    sc.drum('k...s..kk.k.s... k.h.s.hkh.k.s.h. k.hgs.h.hgkks.hs k.s.k.ssrrrrrrrr')
    # drop A (8 bars): full riff engine, the hook
    sc.mark('DROP')
    CH = 'Dm | Eb | Gm | Dm | Dm | Eb | Bb | Dm'
    R1 = ['x.x.x7x.x.x2x.x.', 'x.xx.x7.x.xx.2-.']
    sc.play(1, riff(CH, R1, lo='D3'))
    lead = (hook('D', 5) + ' | Bb4:2 R:1 Bb4:1 G4:2 Bb4:2 C5:2 D5:2 Eb5:2 F5:2 | '
            'G5:3 F5:1 D5:2 Bb4:2 C5:2 D5:2 F5:2 G5:2 | A5:4 G5:2 F5:2 Eb5:2 D5:2 C5:2 A4:2 | '
            + hook('D', 6) + ' | Eb6:4 D6:2 Bb5:2 G5:2 Bb5:2 Eb6:2 F6:2 | '
            'D6:2 Bb5:2 F5:2 Bb5:2 D6:2 F6:2 Eb6:2 D6:2 | D6:6 C6:2 A5:4 Eb5:2 D5:2')
    sc.play(2, '@2 ' + lead)
    sc.play(3, '@1 ' + bassline(CH, ROOT_8, step=2, lo='C2'))
    sc.drum(beat(8, [BRK_A, BRK_B]))
    # drop B (8 bars): turntable stabs answer the hook
    stabs = (hook('D', 5) + ' | ' + f'{scratch("Eb6")} R:2 {scratch("Eb6", 1)} R:1 {scratch("Eb6")} Bb5:2 G5:2 Eb5:2 F5:2 | '
             'G5:3 F5:1 D5:2 Bb4:2 ' + f'{scratch("G6")} R:2 {scratch("G6")} R:2 | '
             f'{scratch("D6")} R:1 {scratch("D6", 1)} {scratch("A5")} R:2 {scratch("D6")} {scratch("Eb6")} {scratch("D6")} R:2 | '
             + hook('D', 6) + ' | ' + f'{scratch("Eb6", 1)} {scratch("Eb6", 1)} R:2 {scratch("Bb5")} R:2 Eb6:2 F6:2 G6:4 | '
             'F6:2 D6:2 Bb5:2 F5:2 ' + f'{scratch("D6")} {scratch("D6")} {scratch("F6")} {scratch("Bb6", 2, "40")} | '
             'A6:4 G6:2 F6:2 Eb6:2 D6:2 C6:2 A5:2')
    sc.play(1, riff(CH, ['x.x.x7x.x.x2x.x.', 'xxx.x7x.xxx.x2x.'], lo='D3'))
    sc.play(2, '@2 ' + stabs)
    sc.play(3, '@1 ' + bassline(CH, ROOT_16, step=1, lo='C2'))
    sc.drum(beat(8, [BRK_C, BRK_B]))
    # bridge (4 bars): stripped to bass and glass texture
    sc.play(1, glass('Gm | Eb | Gm | A', lo='D5', fig=[0, 2, 3, 4, 3, 2, 1, 2]))
    sc.play(2, '@6 R:8 D6:8/442 | Eb6:16/442 | Bb5:8 D6:8/442 | @5 A4:16/*101')
    sc.play(3, '@1 G1:6 G1:2 R:4 G1:4 | Eb2:6 Eb2:2 R:4 Eb2:4 | G1:6 G1:2 R:4 G1:4 | A1:4 A1:4 A1:2 A1:2 A1:1 A1:1 R:2')
    sc.drum('k.......h.......  h.......k.......  k.......h.....k. r.r.r.r.rrrrrrrr')
    # final chorus (8 bars): riff doubled, drums huge
    CH2 = CH
    R2 = R1
    sc.play(1, riff(CH2, R2, lo='D3'))
    sc.play(2, '@2 ' + hook('D', 6) + ' | Eb6:4 D6:2 Bb5:2 G5:2 Bb5:2 Eb6:2 F6:2 | '
            'G6:3 F6:1 D6:2 Bb5:2 C6:2 D6:2 F6:2 G6:2 | A6:4 G6:2 F6:2 Eb6:2 D6:2 C6:2 A5:2 | '
            + riff('Dm | Eb', R2, lo='D5', instr=1) + ' | @2 F6:2 D6:2 Bb5:2 F5:2 Bb5:2 D6:2 F6:2 Bb6:2 | '
            'A6:4 ' + scratch('A6') + ' ' + scratch('A6') + ' F6:2 Eb6:2 D6:4')
    sc.play(3, '@1 ' + bassline(CH2, ROOT_16, step=1, lo='C2'))
    sc.drum(' '.join(['c' + HUGE_A[1:], HUGE_B, HUGE_A, HUGE_B, 'c' + HUGE_A[1:], HUGE_B, HUGE_A, 'k.s.k.ssrrrrrrrr']))
    return sc.build(loop_to='DROP')



# ================================================================ generic mission / boss song
def trc(chords, semis):
    """Transpose a chord line ('Dm | Eb | Daug')."""
    out = []
    for t in chords.split():
        if t == '|': out.append(t); continue
        m = re.fullmatch(r'([A-G][#b]?)(.*)', t)
        r = nm(midi(m.group(1) + '4') + semis)[:-1]
        out.append(r + m.group(2))
    return ' '.join(out)

def L(*bars):
    return ' | '.join(bars)

def S(note, rows=2, speed='30'):
    return scratch(note, rows, speed)

def mission(name, semis, tpr, CH_A, LEAD_A, CH_B, LEAD_B, BRK_CH, BRK_LEAD, riff_a, riff_b=None,
            drums_a=(BRK_A, BRK_B), drums_b=None, b_ch1='riff', intro='riff', bass_a=ROOT_8,
            bass_b=ROOT_16, lead_ins=2, b_ins=2,
            bass_wave=W_REESE, glass_lo='D5', setup_fn=None):
    """intro 4 bars -> [DROP] A 8 bars -> B 8 bars -> break 4 bars -> back to DROP. All text written in D,
    moved by `semis`."""
    sc = Score(name, tpr)
    setup(sc, bass_wave)
    if setup_fn: setup_fn(sc)
    T = lambda text: tr(text, semis)
    C = lambda ch: trc(ch, semis)
    first = CH_A.split('|')[0].strip()
    # intro
    if intro == 'riff':
        sc.play(1, riff(C(f'{first} | {first} | {first} | {first}'), [riff_a[0], riff_a[0], riff_a[0], 'x.xx.xx.xxxxo-o-'], lo='A2'))
    elif intro == 'glass':
        sc.play(1, glass(C(' | '.join(CH_A.split('|')[:4])), lo=glass_lo, step=2))
    else:
        sc.play(1, intro(C, T))
    sc.play(2, 'R:16 | R:16 | R:16 | @5 ' + T('A4:16/*101'))
    sc.play(3, '@3 ' + T('D2:16/422') + ' | -:16 | @1 ' + bassline(C(f'{first}'), [0, 'R', 'R', 'R', 0, 'R', 0, 'R'], step=2, lo='A1')
            + ' | ' + T('D2:8 R:4 D2:2 D2:2'))
    INTRO_DR = 'h...h...h...h... h...h...h.h.h.h. k...s..kk.k.s... k.s.k.ssrrrrrrrr'
    sc.drum(INTRO_DR)
    # drop A
    sc.mark('DROP')
    sc.play(1, riff(C(CH_A), riff_a, lo='A2'))
    sc.play(2, f'@{lead_ins} ' + T(LEAD_A))
    sc.play(3, '@1 ' + bassline(C(CH_A), bass_a, step=2 if len(bass_a) == 8 else 1, lo='A1'))
    sc.drum(beat(8, list(drums_a)))
    # drop B
    if b_ch1 == 'riff':
        sc.play(1, riff(C(CH_B), riff_b or riff_a, lo='A2', instr=8))
    elif b_ch1 == 'glass':
        sc.play(1, glass(C(CH_B), lo=glass_lo, step=1))
    elif b_ch1 == 'wt':
        sc.play(1, '@3 ' + broken(C(CH_B), [0, 1, 2, 3, 2, 1, 0, 1], step=1, lo='D4'))
    else:
        sc.play(1, b_ch1(C, T))
    sc.play(2, f'@{b_ins} ' + T(LEAD_B))
    sc.play(3, '@1 ' + bassline(C(CH_B), bass_b, step=2 if len(bass_b) == 8 else 1, lo='A1'))
    sc.drum(beat(8, list(drums_b or drums_a)))
    # break
    sc.play(1, glass(C(BRK_CH), lo=glass_lo, fig=[0, 2, 3, 4, 3, 2, 1, 2]))
    sc.play(2, '@6 ' + T(BRK_LEAD))
    sc.play(3, '@1 ' + bassline(C(BRK_CH), [0, 'R', 'R', 0, 'R', 'R', 0, 'R'], step=2, lo='A1'))
    sc.drum(INTRO_DR)                       # the break rebuilds like the intro (same pattern: saves ROM)
    return sc.build(loop_to='DROP')


R_STD = ['x.x.x7x.x.x2x.x.', 'x.xx.x7.x.xx.2-.']
R_GAL = ['xxx.xxx.xxx.x7x.', 'xxx.xxx.x2x.x7x.']
R_HALF = ['x.....x7x.....2.', 'x...x...x.x7x...']
R_HALFSTEP = ['x2x2x.x.x2x2x.7.', 'x2x2x.x.x2x2xx7.']


# ---------------------------------------------------------------- M1 Rimeveil Lane: "Rime Runner" (E Phrygian, 149)
def rime_runner():
    return mission('Rime Runner', 2, 6,
        'Dm | Eb | Gm | Dm | Dm | Eb | C | Dm',
        L('D5:3 D5:1 A4:2 D5:2 Eb5:2 D5:2 C5:2 A4:2', 'Eb5:2 R:1 Eb5:1 Bb4:2 Eb5:2 G5:4 F5:2 Eb5:2',
          'G5:3 F5:1 D5:2 Bb4:2 G4:2 Bb4:2 D5:2 F5:2', 'A5:6 G5:2 F5:2 Eb5:2 D5:4',
          'D6:3 D6:1 A5:2 D6:2 Eb6:2 D6:2 C6:2 A5:2', 'Eb6:4 D6:2 Bb5:2 G5:2 Bb5:2 Eb6:4',
          'C6:2 E6:2 G6:2 E6:2 C6:2 G5:2 E5:2 C5:2', 'D5:2 Eb5:2 F5:2 A5:2 D6:8'),
        'Bb | C | Dm | Dm | Bb | C | Eb | A',
        L(f'Bb5:2 R:1 Bb5:1 F5:2 Bb5:2 {S("D6")} R:2 {S("D6")} C6:2', f'C6:2 R:1 C6:1 G5:2 C6:2 {S("E6")} R:2 {S("E6")} D6:2',
          'D6:4 C6:2 A5:2 F5:2 A5:2 D6:2 F6:2', f'Eb6:2 D6:2 C6:2 A5:2 {S("D6")} {S("D6")} {S("A6")} R:2',
          'Bb5:2 D6:2 F6:2 D6:2 Bb5:2 F5:2 D5:2 F5:2', f'C6:2 E6:2 G6:2 E6:2 {S("G6")} R:2 {S("G6")} {S("C7")}',
          'G6:4 F6:2 Eb6:2 Bb5:2 G5:2 Eb5:4', f'E5:2 G5:2 A5:2 C#6:2 E6:2 A6:2 {S("A6")} R:2'),
        'Gm | Eb | Gm | A', 'R:8 D6:8/442 | Eb6:16/442 | Bb5:8 D6:8/442 | @5 A4:16/*101',
        R_STD, ['x.x.x7x.x.x2x.x.', 'xxx.x7x.xxx.x2x.'])


# ---------------------------------------------------------------- M2 Stormwall Nebula: "Arc Breaker" (G Phrygian, 149, electric)
def arc_breaker():
    return mission('Arc Breaker', -7, 6,
        'Dm | Eb | Gm | Dm | Dm | Eb | C | Dm',
        L('D5:1 R:1 D6:1 R:1 D5:1 R:1 C6:2 A5:2 D5:2 Eb5:2 F5:2', 'Eb5:1 R:1 Eb6:1 R:1 Eb5:1 R:1 D6:2 Bb5:2 G5:2 Bb5:2 D6:2',
          'G5:4 F5:2 D5:2 F5:2 G5:2 Bb5:2 C6:2', 'A5:8/442 R:2 A5:1 A5:1 C6:2 A5:2',
          'D6:1 R:1 D7:1 R:1 D6:1 R:1 C7:2 A6:2 D6:2 Eb6:2 F6:2', 'Eb6:2 D6:2 C6:2 Bb5:2 G5:2 Bb5:2 C6:2 D6:2',
          'C6:2 E6:2 C6:2 G5:2 E5:2 G5:2 C6:2 E6:2', 'D6:12/443 R:4'),
        'Bb | Gm | Eb | Dm | Bb | C | Eb | A',
        L('F5:8/442 G5:4 A5:4', 'Bb5:12/442 A5:4', 'G5:8/442 F5:4 Eb5:4', f'D5:8/442 {S("D6")} {S("D6")} {S("A6")} R:2',
          'F5:8/442 G5:4 A5:4', 'Bb5:8/442 C6:8/442', 'Eb6:8/442 D6:4 C6:4', f'C#6:8/443 {S("E6")} {S("E6")} {S("A6")} R:2'),
        'Gm | Eb | Gm | A', 'D6:2 R:2 D6:2 R:2 G6:8/442 | Eb6:16/442 | Bb5:2 R:2 Bb5:2 R:2 D6:8/442 | @5 A4:16/*101',
        R_STD, b_ch1='glass', lead_ins=2, b_ins=6, glass_lo='A4')


# ---------------------------------------------------------------- M3 Barrens Belt: "Wisp Breaks" (A Phrygian, 128, spooky)
def wisp_breaks():
    return mission('Wisp Breaks', -5, 7,
        'Dm | Eb | Dm | Eb | Gm | A | Dm | Dm',
        L('R:4 D5:4/442 Eb5:4 D5:4', 'A4:12/443 R:4', 'R:4 D5:4 Eb5:4 F5:4', 'G5:8/442 F5:4 Eb5:4',
          'D5:4 G5:4 Bb5:4 A5:4', 'C#6:8/443 Bb5:4 A5:4', 'D6:4 C6:2 A5:2 Eb5:4 D5:4', 'D5:16/443'),
        'Dm | Eb | C | Dm | Dm | Eb | Bb | A',
        L(hook('D', 5), 'Eb5:2 R:1 Eb5:1 Bb4:2 Eb5:2 G5:4 F5:4', f'E5:2 G5:2 C6:4 {S("C6")} R:2 {S("G6")} R:2', 'A5:8/443 D6:8/443',
          hook('D', 6), 'Eb6:4 Bb5:4 G5:4 Eb5:4', f'F5:2 Bb5:2 D6:2 F6:2 {S("D6")} R:2 {S("Bb5")} R:2', 'C#6:4 E6:4 A6:8/443'),
        'Gm | Eb | Gm | A', 'R:8 D6:8/443 | Eb6:16/443 | R:8 Bb5:8/443 | @5 A4:16/*101',
        R_HALF, R_STD, drums_a=(HALF, 'k.h.h.hks.h.h.ho'), drums_b=(BRK_A, BRK_B), intro='glass', lead_ins=6, glass_lo='A5',
        bass_a=[0, 'R', 'R', 'R', 0, 'R', 'R', 12])


# ---------------------------------------------------------------- M4 Frostbite Moon: "Frost Line" (D Phrygian, 149, glass)
def frost_line():
    return mission('Frost Line', 0, 6,
        'Dm | Eb | Gm | Dm | Bb | Eb | Gm | A',
        L('D5:4 D5:2 A4:2 D5:4 Eb5:4', 'D5:2 C5:2 A4:4 Bb4:4 G4:4', 'Bb4:4 D5:2 G5:2 F5:4 D5:4', 'A4:16/443',
          'D6:4 D6:2 A5:2 D6:4 Eb6:4', 'Bb5:4 G5:2 Eb5:2 G5:4 Bb5:4', 'G5:2 Bb5:2 D6:2 G6:2 F6:4 D6:4', 'E6:4 C#6:4 A5:8/443'),
        'Gm | Eb | Bb | A | Gm | Eb | Bb | Dm',
        L('G5:8 Bb5:4 A5:4', 'G5:8/442 Eb5:8/442', 'F5:8 D5:4 F5:4', f'E5:8/442 {S("A6")} {S("A6")} {S("E6")} R:2',
          'G5:4 Bb5:4 D6:8/442', 'Eb6:8/442 Bb5:8', 'D6:4 F6:4 Bb6:8/442', 'A6:12/443 R:4'),
        'Gm | Eb | Gm | A', 'R:8 D6:8/442 | Eb6:16/442 | Bb5:8 D6:8/442 | @5 A4:16/*101',
        R_STD, b_ch1='glass', intro='glass', drums_b=(HUGE_A, HUGE_B), b_ins=6, lead_ins=2, glass_lo='D5')


# ---------------------------------------------------------------- M5 Flatwoods Drift (gun cam): "Dead Air" (C# Phrygian, 128, UFO)
def dead_air():
    return mission('Dead Air', -1, 7,
        'Dm | Dm | Dm | Daug | Dm | Dm | Gm | A',
        L('D5:2 R:2 A4:2 D5:2 Eb5:2 D5:2 R:4', 'F5:2 R:2 Eb5:2 D5:2 C5:4 A4:4', 'D5:2 R:2 A4:2 D5:2 Eb5:2 F5:2 G#5:2 A#5:2',
          'C6:8/442 A#5:4 G#5:4', 'D6:2 R:2 A5:2 D6:2 Eb6:2 D6:2 R:4', 'F6:2 R:2 Eb6:2 D6:2 C6:4 A5:4',
          'G5:2 Bb5:2 D6:2 G6:2 F6:2 D6:2 Bb5:2 G5:2', 'A5:4 C#6:4 E6:4 A6:4'),
        'Daug | Eaug | Daug | Eaug | Gm | Eb | Gm | A',
        L(f'{S("D6")} R:2 {S("F#6")} R:2 {S("A#6")} R:2 D7:4', 'E6:8/442 C6:8/442', f'{S("D6")} R:2 {S("F#6")} R:2 {S("A#6")} R:2 F#6:4',
          'G#6:8/442 E6:8/442', 'G5:4 Bb5:4 D6:8/442', 'Eb6:4 G6:4 Bb6:8/442', 'D6:2 Bb5:2 G5:2 Bb5:2 D6:2 G6:2 F6:2 D6:2', 'C#6:4 E6:4 A6:8/443'),
        'Daug | Eaug | Gm | A', 'D6:4 F#6:4 A#6:8/442 | E6:4 G#6:4 C7:8/442 | G6:16/442 | @5 A4:16/*101',
        ['x.xox.x2xox.x7o.', 'x.xox.x2x.oxx7x.'], b_ch1='wt', bass_a=[0, 0, 12, 0, 'R', 0, 13, 0, 0, 12, 0, 10, 'R', 0, 7, 12],
        bass_b=[0, 0, 12, 0, 'R', 0, 13, 0, 0, 12, 0, 10, 'R', 0, 7, 12], drums_a=(BRK_A, BRK_C), bass_wave=W_SAW)


# ---------------------------------------------------------------- BOSS (mid-bosses, Space Kraken): "Riff Engine" (F# Phrygian, 179)
def riff_engine():
    return mission('Riff Engine', 4, 5,
        'Dm | Eb | Dm | Eb | Gm | Eb | Gm | A',
        L('D5:2 R:1 D5:1 A4:2 D5:2 Eb5:4 D5:4', 'G5:2 R:1 G5:1 Eb5:2 Bb4:2 G5:4 F5:4', 'D5:2 R:1 D5:1 A4:2 D5:2 F5:4 A5:4',
          'Bb5:4 G5:2 Eb5:2 G5:4 Bb5:4', 'D6:6 C6:2 Bb5:4 G5:4', 'Eb6:6 D6:2 C6:4 Bb5:4',
          'D6:2 Bb5:2 G5:2 Bb5:2 D6:2 G6:2 F6:2 D6:2', f'E6:4 C#6:4 A5:4 {S("A6")} R:2'),
        'Bb | C | Dm | Dm | Bb | C | Eb | A',
        L(f'{S("F6")} R:2 {S("F6")} R:2 D6:2 Bb5:2 F5:4', f'{S("G6")} R:2 {S("G6")} R:2 E6:2 C6:2 G5:4', hook('D', 6), 'A5:4 D6:4 F6:4 A6:4',
          f'{S("Bb6")} R:2 {S("Bb6")} R:2 F6:2 D6:2 Bb5:4', f'{S("G6")} R:2 {S("G6")} R:2 E6:2 C6:2 G6:4',
          'Eb6:2 G6:2 Bb6:2 G6:2 Eb6:4 Bb5:4', f'A5:2 C#6:2 E6:2 A6:2 {S("A6")} {S("A6")} {S("A6")} R:2'),
        'Gm | Eb | Gm | A', 'D6:8/442 C6:8 | Eb6:16/442 | D6:8 Bb5:8/442 | @5 A4:16/*102',
        R_GAL, R_STD, drums_a=(FAST_A, FAST_B))


# ---------------------------------------------------------------- M2 boss Thunderbird: "Thunder Riff" (B Phrygian, 179)
def thunder_riff():
    return mission('Thunder Riff', -3, 5,
        'Dm | Dm | Eb | Eb | Gm | Gm | Eb | A',
        L('D5:4 A4:2 D5:2 F5:4 Eb5:2 D5:2', 'A5:6 G5:2 F5:4 D5:4', 'Eb5:4 Bb4:2 Eb5:2 G5:4 F5:2 Eb5:2', 'Bb5:6 A5:2 G5:4 Eb5:4',
          'G5:4 D5:2 G5:2 Bb5:4 A5:2 G5:2', 'D6:6 C6:2 Bb5:4 G5:4', 'Eb6:4 D6:2 C6:2 Bb5:2 G5:2 Eb5:4', 'C#6:4 A5:4 E5:4 C#5:4'),
        'Gm | A | Dm | Bb | Gm | A | Eb | A',
        L(f'G5:2 R:1 G5:1 D5:2 G5:2 {S("Bb5")} R:2 {S("Bb5")} {S("Ab5")}', 'A5:2 R:1 A5:1 E5:2 A5:2 C#6:4 E6:4', hook('D', 6),
          'Bb5:4 D6:4 F6:4 Bb6:4', f'G6:2 R:1 G6:1 D6:2 G6:2 {S("Bb6")} R:2 {S("Bb6")} {S("Ab6")}', 'A6:4 G6:2 E6:2 C#6:4 A5:4',
          'Eb6:2 G6:2 Bb6:2 G6:2 Eb6:2 Bb5:2 G5:2 Eb5:2', f'C#5:2 E5:2 A5:2 C#6:2 {S("E6")} {S("E6")} {S("A6")} R:2'),
        'Gm | Eb | Gm | A', 'G5:16/443 | Eb6:16/443 | D6:8 G6:8/443 | @5 A4:16/*102',
        R_GAL, R_GAL, drums_a=(FAST_A, 'kkhks.kkc.hks.ks'))


# ---------------------------------------------------------------- M3 boss Jersey Devil: "Leeds Breakdown" (G Phrygian dominant, 179)
def leeds_breakdown():
    return mission('Leeds Breakdown', 5, 5,
        'D | Eb | D | Eb | Gm | Eb | D | D',
        L('D5:1 Eb5:1 F#5:1 G5:1 A5:2 F#5:2 G5:1 F#5:1 Eb5:1 D5:1 Eb5:4', 'G5:1 F#5:1 Eb5:1 D5:1 C5:2 Eb5:2 D5:4 R:4',
          'D5:1 Eb5:1 F#5:1 G5:1 A5:2 Bb5:2 C6:2 Bb5:2 A5:4', 'Bb5:2 A5:2 G5:2 F#5:2 Eb5:4 D5:4',
          'G5:2 Bb5:2 D6:4 C6:2 Bb5:2 A5:4', 'Bb5:2 G5:2 Eb5:2 G5:2 Bb5:4 C6:4',
          'D6:1 C6:1 Bb5:1 A5:1 G5:1 F#5:1 Eb5:1 D5:1 F#5:4 A5:4', f'D6:8/443 {S("D6")} {S("D6")} {S("F#6")} R:2'),
        'Gm | D | Gm | D | Eb | D | Eb | D',
        L('G5:4 D5:2 G5:2 Bb5:4 A5:2 G5:2', f'F#5:4 D5:2 F#5:2 A5:4 {S("D6")} {S("D6")}', 'G5:2 Bb5:2 D6:2 G6:2 F#6:2 D6:2 Bb5:2 G5:2',
          'A5:2 F#5:2 D5:2 F#5:2 A5:2 D6:2 Eb6:2 D6:2', 'Eb6:4 Bb5:2 G5:2 Eb5:4 G5:4', f'F#5:2 A5:2 D6:2 F#6:2 {S("A6")} R:2 {S("A6")} {S("D6")}',
          'G6:2 Eb6:2 Bb5:2 G5:2 Eb5:2 G5:2 Bb5:2 Eb6:2', f'D6:8/443 {S("F#6")} {S("D6")} {S("A5")} R:2'),
        'Gm | Eb | Gm | D', 'G5:8/442 Bb5:8 | Eb6:16/442 | D6:8 Bb5:8/442 | @5 A4:16/*102',
        ['x.x2x.x2x.x.x2x.', 'x2x.x2x.xo.ox2x.'], R_GAL, drums_a=(FAST_A, FAST_B))


# ---------------------------------------------------------------- M4 boss Wendigo: "Hunger Engine" (C Phrygian, 179)
def hunger_engine():
    return mission('Hunger Engine', -2, 5,
        'Dm | Eb | Dm | Eb | Dm | Eb | Gm | A',
        L('A4:8/*103 D5:8/442', 'Eb5:4 D5:2 C5:2 A4:8/442', 'D5:2 D5:2 R:2 D5:2 Eb5:2 Eb5:2 R:2 Eb5:2', 'F5:4 Eb5:2 D5:2 C5:4 A4:4',
          'A4:8/*103 F5:8/442', 'G5:4 F5:2 Eb5:2 D5:4 C5:4', 'Bb4:4 D5:4 G5:8/442', 'A5:4 G5:2 F5:2 E5:4 C#5:4'),
        'Dm | Dm | Eb | Eb | Gm | Gm | A | A',
        L(f'{S("D6")} R:2 {S("D6")} R:2 {S("Eb6")} {S("D6")} R:4', 'A5:8/442 D6:8/442', f'{S("Eb6")} R:2 {S("Eb6")} R:2 {S("F6")} {S("Eb6")} R:4',
          'Bb5:8/442 Eb6:8/442', f'{S("G6")} R:2 {S("G6")} R:2 {S("A6")} {S("G6")} R:4', 'D6:4 C6:4 Bb5:4 G5:4',
          f'{S("A6")} R:2 {S("A6")} R:2 {S("Bb6")} {S("A6")} R:4', 'C#6:4 E6:4 A6:8/443'),
        'Dm | Eb | Gm | A', 'A4:16/*102 | Bb4:16/*102 | D5:16/442 | @5 A4:16/*102',
        R_HALFSTEP, R_GAL, drums_a=(FAST_A, FAST_B), bass_a=[0, 1, 0, 1, 0, 'R', 0, 1])


# ---------------------------------------------------------------- M5 boss Flatwoods Monster: "Braxton Breaker" (Bb Phrygian, 179)
def braxton_breaker():
    return mission('Braxton Breaker', -4, 5,
        'Dm | Bb | Eb | A | Dm | Bb | Eb | A',
        L('D5:2 R:1 D5:1 A4:2 D5:2 Eb5:2 F5:2 Eb5:2 D5:2', 'F5:2 R:1 F5:1 D5:2 Bb4:2 D5:2 F5:2 Bb5:4',
          'G5:2 R:1 G5:1 Eb5:2 Bb4:2 Eb5:2 G5:2 Bb5:4', f'A5:1 B5:1 C#6:1 D#6:1 F6:1 G6:1 A6:2 {S("A6")} R:2 {S("E6")} {S("A6")}',
          'D6:2 R:1 D6:1 A5:2 D6:2 Eb6:2 F6:2 Eb6:2 D6:2', 'F6:4 D6:2 Bb5:2 F6:4 D6:4',
          'Eb6:2 G6:2 Bb6:2 G6:2 Eb6:2 Bb5:2 G5:2 Eb5:2', 'C#6:4 A5:4 E5:4 C#5:4'),
        'Daug | Eaug | Daug | Eaug | Bb | C | Eb | A',
        L('D5:1 E5:1 F#5:1 G#5:1 A#5:1 C6:1 D6:2 C6:2 A#5:2 F#5:4', 'E5:1 F#5:1 G#5:1 A#5:1 C6:1 D6:1 E6:2 D6:2 C6:2 G#5:4',
          f'{S("D6")} R:2 {S("A#5")} {S("F#5")} {S("D6")} R:2 F#6:4', f'{S("E6")} R:2 {S("C6")} {S("G#5")} {S("E6")} R:2 G#6:4',
          'Bb5:2 D6:2 F6:2 Bb6:2 A6:4 F6:4', 'C6:2 E6:2 G6:2 C7:2 B6:4 G6:4', 'Bb6:4 G6:4 Eb6:4 Bb5:4',
          'A5:2 C#6:2 E6:2 A6:2 G6:2 E6:2 C#6:2 A5:2'),
        'Bb | C | Daug | A', 'D6:8/442 F6:8 | E6:8/442 G6:8 | F#6:16/442 | @5 A4:16/*102',
        R_GAL, b_ch1='wt', drums_a=(FAST_A, FAST_B))


# ---------------------------------------------------------------- MAP (arcade ship select): "Cold Boot" (D minor, 128)
def cold_boot():
    sc = Score('Cold Boot', 7)
    setup(sc)
    CH = 'Dm | Bb | Gm | A | Dm | Eb | Gm | A | Bb | C | Dm | A'
    sc.play(1, glass(CH, lo='D5', step=2))
    sc.play(2, '@6 ' + L('D5:4 A4:4 D5:4 Eb5:4', 'D5:8 F5:8', 'G5:4 F5:4 D5:4 Bb4:4', 'A4:12 R:4', 'D5:4 A4:4 D5:4 Eb5:4',
                          'G5:8/442 Bb5:8', 'A5:4 G5:4 F5:4 D5:4', f'E5:8/442 {S("A5")} R:2 {S("A5")} R:2', 'F5:8 D5:8', 'E5:8 G5:8',
                          'A5:4 F5:4 D5:4 A4:4', 'C#5:12/442 R:4'))
    sc.play(3, '@1 ' + bassline(CH, [0, 'R', 'R', 0, 'R', 0, 12, 'R'], step=2, lo='A1'))
    sc.drum(' '.join([HALF, 'k.h.h.hks.h.h.ho'] * 5 + [HALF, 'k.h.s.h.k.s.srrr']))
    return sc.build()


# ---------------------------------------------------------------- BRIEF (debrief cutscenes): "Thin Ice" (D Phrygian, 112)
def thin_ice():
    sc = Score('Thin Ice', 8)
    setup(sc)
    CH = 'Dm | Dm | Eb | Gm | Dm | Dm | Eb | A | Gm | Dm | Eb | Dm'
    sc.play(1, glass(CH, lo='A4', step=2, fig=[0, 2, 3, 4, 3, 2, 1, 2]))
    sc.play(2, '@6 ' + L('R:16', 'D5:6 A4:2 D5:4 Eb5:4', 'Eb5:8/442 D5:4 C5:4', 'Bb4:16/442', 'R:16', 'D6:6 A5:2 D6:4 Eb6:4',
                          'Eb6:8/442 D6:4 Bb5:4', 'A5:16/443', 'Bb5:8 G5:8', 'A5:8 F5:8', 'G5:4 F5:4 Eb5:4 C5:4', 'D5:16/443'))
    sc.play(3, '@3 ' + L('D2:16/422', '-:16', 'Eb2:16/422', 'G1:16/422', 'D2:16/422', '-:16', 'Eb2:16/422', 'A1:16/422',
                          'G1:16/422', 'D2:16/422', 'Eb2:16/422', 'D2:16/422'))
    sc.drum(' '.join(['h...h...h...h...'] * 4 + ['k...h...s...h.k.', 'k...h...s...h...'] * 3 + ['k...h...s...h.k.', 'k...h...s.h.r.r.']))
    return sc.build()


# ---------------------------------------------------------------- END: "Frozen Shimmer" (D Phrygian, 112): the riff decays into shimmer
def frozen_shimmer():
    sc = Score('Frozen Shimmer', 8)
    setup(sc)
    sc.duty(1, 'Chug fading', duty=1, vol=10, decay=1)
    # A: the riff one last time, half time
    sc.play(1, riff('Dm | Eb | Dm | A', R_HALF, lo='D3'))
    sc.play(2, '@2 ' + L(hook('D', 5), 'Eb5:4 D5:4 C5:4 A4:4', hook('D', 5, 'B'), 'A5:16/443'))
    sc.play(3, '@1 ' + bassline('Dm | Eb | Dm | A', [0, 'R', 'R', 0, 'R', 'R', 0, 'R'], step=2, lo='A1'))
    sc.drum(' '.join([HALF, 'k.h.h.hks.h.h.ho', HALF, 'k.h.s.h.k.s.srrr']))
    # B: glass and the long way home
    CH = 'Dm | F | Gm | A | Dm | Gm | A | Dm'
    sc.play(1, glass(CH, lo='D5', step=2))
    sc.play(2, '@6 ' + L('F5:8 E5:4 D5:4', 'C5:8 D5:8', 'Bb4:8 C5:4 D5:4', 'A4:16/442', 'F5:8 G5:4 A5:4', 'Bb5:8 A5:4 G5:4',
                          'A5:8 C#6:8', 'D6:16/443'))
    sc.play(3, '@2 ' + bassline(CH, [0, 'R', 7, 'R', 12, 'R', 7, 'R'], step=2, lo='D2'))
    sc.drum(' '.join(['k...h...s...h...', 'k...h...s...h.k.'] * 4))
    # C: frozen shimmer
    sc.play(1, glass('Dm | Eb | Dm | Eb', lo='D6', step=2, fig=[0, 1, 2, 3]))
    sc.play(2, '@6 R:16 | D6:16/443 | R:16 | Eb6:8/443 D6:8/443')
    sc.play(3, '@3 D2:16/422 | -:16 | Eb2:16/422 | D2:12/422 R:4')
    sc.drum('h.......h....... h.......h....... h.......h....... h...h...h.h.h.h.')
    return sc.build()


# ---------------------------------------------------------------- WARN: "Whiteout" (jingle, 8 bars)
def whiteout():
    sc = Score('Whiteout', 6)
    setup(sc)
    alarm = ' '.join(['D6:2 Eb6:2'] * 4)
    sc.play(1, riff('Dm | Dm | Eb | Eb | Dm | Dm | Eb | A', ['x...x...x...x...', 'x.x.x.x.x.x.x.x.', 'x.x.x.x.x.x.x.x.', 'x.x.x.x.x.x.x.x.',
                                                          'xxx.xxx.xxx.x7x.', 'xxx.xxx.x2x.x7x.', 'xxx.xxx.xxx.x2x.', 'xxxxxxxxxxxxxxxx'], lo='D3', instr=8))
    sc.play(2, '@7 ' + L(alarm, alarm, tr(alarm, 1), tr(alarm, 1), hook('D', 6), f'{S("D6")} R:2 {S("D6")} R:2 {S("A6")} {S("D6")} R:4',
                          'Eb6:4 Bb5:4 G5:4 Eb5:4', 'A5:2 C#6:2 E6:2 A6:2 A6:8/443'))
    sc.play(3, '@1 ' + bassline('Dm | Dm | Eb | Eb | Dm | Dm | Eb | A', [0, 'R', 0, 'R', 0, 'R', 0, 1], step=2, lo='A1'))
    sc.drum('k...k...k...k... k...k...k...k... k.h.s.hkh.k.s.ho k.hgs.h.hgkks.hs '
            'c.kks.hkk.kks.hs kkhks.kkk.hks.ks r.r.r.r.rrrrrrrr rrrrrrrrrrrrrrrr')
    return sc.build(end_silent=True)


# ---------------------------------------------------------------- CLEAR: "Thaw" (jingle, D major)
def thaw():
    sc = Score('Thaw', 6)
    setup(sc)
    sc.play(1, riff('D | G | D | D', ['x.x.x5x.x.x5x.x.', 'x.x.x5x.x.x5x.x.', 'x.xx.xx.xxx.o-o-', 'x---------------'], lo='D3', instr=8))
    sc.play(2, '@2 ' + L('D5:2 R:1 D5:1 A4:2 D5:2 E5:2 F#5:2 A5:4', 'G5:2 A5:2 B5:2 D6:2 E6:4 C#6:4',
                          f'D6:2 {S("D6")} {S("A6")} {S("D6")} F#6:2 E6:2 F#6:4', 'D6:12/443 R:4'))
    sc.play(3, '@1 ' + L('D2:2 R:2 D2:2 D3:2 D2:2 R:2 D2:2 E2:2', 'G1:2 R:2 G1:2 G2:2 G1:2 R:2 A1:2 A2:2',
                          'D2:2 R:2 D2:2 D3:2 A1:2 A1:2 D2:4', 'D2:12 R:4'))
    sc.drum('c.hks.hgk.hks.ho k.hks.hgk.kks.sr k.s.k.ssk.s.rrrr c...............')
    return sc.build(end_silent=True)


# ---------------------------------------------------------------- OVER: "Frozen Out" (jingle)
def frozen_out():
    sc = Score('Frozen Out', 10)
    setup(sc)
    sc.play(1, riff('Dm | Eb | Dm | Dm', ['x.....x7x.....2.', 'x.....x.x.....7.', 'x...x...x.x.x...', 'x---------------'], lo='D3'))
    sc.play(2, '@6 ' + L('D5:4 A4:4 D5:4 Eb5:4', 'D5:4 C5:4 A4:4 G4:4', 'F4:4 Eb4:4 D4:8/442', f'{S("D5", 2, "18")} R:14'))
    sc.play(3, '@3 D2:16/422 | Eb2:16/422 | D2:16/422 | D2:12/422 R:4')
    sc.drum('k.......s....... k.......s....... k...k...k.k.kkkk c...............')
    return sc.build(end_silent=True)


SONGS = {'MAP': cold_boot, 'BRIEF': thin_ice, 'END': frozen_shimmer, 'WARN': whiteout, 'CLEAR': thaw, 'OVER': frozen_out,
         'TITLE': frostbite_circuit, 'STAGE': rime_runner, 'STAGE2': arc_breaker, 'STAGE3': wisp_breaks,
         'STAGE4': frost_line, 'STAGE5': dead_air, 'BOSS': riff_engine, 'TBIRD': thunder_riff,
         'JDEVIL': leeds_breakdown, 'WENDIGO': hunger_engine, 'FLATWOODS': braxton_breaker}
