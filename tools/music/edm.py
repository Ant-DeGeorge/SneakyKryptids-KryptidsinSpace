"""Aggressive EDM themes (round 2 notes: stage 1, the shared boss theme and both Flatwoods songs).
All compositions original; each quotes the title hook (root, down a fourth, root, step, step, step, root).

  STAGE     "Overdrive"     Rimeveil Lane      F minor electro, 150 BPM
  BOSS      "Hard Target"   mid-bosses + Kraken, D minor hard dance, 179 BPM
  STAGE5    "Signal Lost"   Flatwoods Drift    C minor acid house, 128 BPM, whole-tone UFO builds
  FLATWOODS "Braxton Breach" Flatwoods Monster Bb minor hard dance, 179 BPM, whole-tone runs

Game Boy EDM toolkit used here:
  * sidechain pump: chords on ch1 use a rising volume envelope and retrigger on every kick, so they
    duck and swell like a compressed supersaw
  * off-kick bass: the wave bass rests on the kick and fills the 16ths after it
  * builds: snare roll 1/4 -> 1/8 -> 1/16, a riser (slide up) on ch2, then a gap before the drop
  * drops: four-on-the-floor kick, clap on 2 and 4, open hats on the off-beats, stabby lead hook
"""
from score import Score
from levels import arp, broken, bassline, tr, W_SAW, W_PUNCH, wave_from_fn
import math

W_REESE = wave_from_fn(lambda x: 0.9 * (1 - 2 * x) * 0.6 + 0.4 * (1 if x < 0.5 else -1) * 0.9)   # saw + square: fat, buzzy


def edm_setup(sc, bass_wave=W_SAW):
    sc.duty(1, 'Pump chords', duty=1, vol=2, decay=1, up=True)      # swells back after each kick
    sc.duty(2, 'Stab lead 25%', duty=1, vol=14, decay=2)
    sc.duty(3, 'Lead 50% hold', duty=2, vol=13, decay=0)
    sc.duty(4, 'Riser', duty=0, vol=11, decay=0)
    sc.duty(5, 'Pluck 12.5%', duty=0, vol=10, decay=1)
    sc.duty(6, 'Alarm', duty=1, vol=12, decay=0)
    sc.wave(1, 'Drop bass', 0, level=1)
    sc.wave(2, 'Break bass', 1, level=2)
    sc.waveform(0, bass_wave)
    sc.waveform(1, wave_from_fn(lambda x: math.sin(2 * math.pi * x)))
    sc.noise(1, 'Kick', vol=15, decay=1)
    sc.noise(2, 'Clap', vol=13, decay=2)
    sc.noise(3, 'Hat', vol=6, decay=1, metallic=True)
    sc.noise(4, 'Open hat', vol=7, decay=2, metallic=True)
    sc.noise(5, 'Crash', vol=12, decay=5)
    sc.noise(6, 'Roll snare', vol=11, decay=1)
    sc.drums = {'k': (1, 18), 's': (2, 41), 'h': (3, 60), 'o': (4, 55), 'c': (5, 33), 'r': (6, 44)}


DROP = 'k.o.s.o.k.o.s.o.'
DROP2 = 'k.ohs.ohk.ohs.ok'
def drop_drums(n, last_fill=True):
    """n bars of drop beat, crash on the first downbeat, fill at the end."""
    bars = ['c.o.s.o.k.o.s.o.'] + [DROP if k % 2 else DROP2 for k in range(1, n)]
    if last_fill: bars[-1] = 'k.o.s.o.k.s.ssss'
    return ' '.join(bars)

BUILD = 'r...r...r...r... r.r.r.r.r.r.r.r. rrrrrrrrrrrrrrrr rrrrrrrrrrrr....'
PUMP = lambda ch, lo='F3': '@1 ' + arp(ch, pulse=4, lo=lo)
OFFBASS = ['R', 0, 12, 0]                          # rest on the kick, then 16ths


# ================================================================ STAGE: "Overdrive" (F minor, 150 BPM)
def overdrive():
    sc = Score('Overdrive', 6)
    edm_setup(sc, W_REESE)
    CH = 'Fm | Db | Ab | Eb | Fm | Db | Ab | Eb'
    # intro (4 bars): kick and pumping chords, bass creeps in
    sc.play(1, PUMP('Fm | Db | Ab | Eb'))
    sc.play(2, 'R:16 | R:16 | R:16 | @6 C5:2 R:2 C5:2 R:2 C5:2 R:2 Eb5:2 G5:2')
    sc.play(3, 'R:16 | R:16 | @2 ' + bassline('Ab | Eb', [0, 'R', 0, 'R'], step=4, lo='E1'))
    sc.drum('k...k...k...k... k...k...k...k... k.o.k.o.k.o.k.o. k.o.k.o.k.o.k.ok')
    # build (4 bars)
    sc.mark('BUILD')
    sc.play(1, '@5 ' + broken('Fm | Db | Ab | Eb', [0, 1, 2, 3], step=2, lo='F4'))
    sc.play(2, '@3 F5:2 R:2 C5:2 F5:2 G5:2 Ab5:2 G5:2 F5:2 | Ab5:4 F5:4 Db5:8 | @4 C5:32/*101 | ')
    sc.play(3, '@1 ' + bassline('Fm | Db | Ab', [0], step=2, lo='E1') + ' | Eb1:12 R:4')
    sc.drum(BUILD)
    # drop A (8 bars)
    lead = ('@2 F5:2 R:2 C5:2 F5:2 G5:2 Ab5:2 G5:2 F5:2 | Ab5:4 F5:2 Db5:2 R:2 Db5:2 F5:2 Ab5:2 | '
            'C6:2 R:2 C6:2 Bb5:2 Ab5:4 Eb5:4 | G5:2 Bb5:2 Eb6:2 R:2 D6:2 Bb5:2 G5:4 | '
            'F5:2 R:2 C5:2 F5:2 G5:2 Ab5:2 C6:2 F6:2 | F6:4 Eb6:2 Db6:2 R:2 Db6:2 C6:2 Ab5:2 | '
            'Eb6:2 C6:2 Ab5:2 C6:2 Eb6:2 Ab6:2 G6:2 Eb6:2 | G6:4 F6:2 Eb6:2 D6:4 Bb5:4')
    sc.play(1, PUMP(CH))
    sc.play(2, lead)
    sc.play(3, '@1 ' + bassline(CH, OFFBASS, step=1, lo='E1'))
    sc.drum(drop_drums(8))
    # break (4 bars): no kick, held chords, hook on a soft lead
    sc.play(1, '@5 ' + arp('Db | Ab | Eb | Eb', pulse=8, lo='F4'))
    sc.play(2, '@3 F5:4 C5:2 F5:2 G5:2 Ab5:2 G5:2 F5:2 | Eb5:16/433 | G5:4 Bb5:4 Eb6:8/433 | D6:16/*101')
    sc.play(3, '@2 Db2:16 | Ab1:16 | Eb2:16 | Eb2:8 R:8')
    sc.drum('c...h...h...h... ....h...h...h... r...r...r...r... r.r.r.r.rrrrrrrr')
    # drop B (8 bars): 16th plucks on ch1, the hook climbs
    lead_b = lead.split(' | ')[:4] + [
        'C6:2 F6:2 C6:2 Ab5:2 F5:2 Ab5:2 C6:2 F6:2', 'F6:2 Db6:2 Ab5:2 F5:2 Ab5:2 Db6:2 F6:2 Ab6:2',
        'G6:4 Eb6:4 C6:4 Ab5:4', 'Bb5:2 D6:2 Eb6:2 G6:2 Bb6:8/433']
    sc.play(1, '@5 ' + broken(CH, [0, 2, 3, 2], step=1, lo='F4'))
    sc.play(2, ' | '.join(lead_b))
    sc.play(3, '@1 ' + bassline(CH, OFFBASS, step=1, lo='E1'))
    sc.drum(drop_drums(8))
    return sc.build(loop_to='BUILD')


# ================================================================ BOSS: "Hard Target" (D minor, 179 BPM)
def hard_target():
    sc = Score('Hard Target', 5)
    edm_setup(sc, W_REESE)
    # intro: alarm over kicks, then a roll
    sc.play(1, '@6 ' + ' '.join(['G#5:2 D5:2'] * 8) + ' | ' + PUMP('Dm | A', lo='A3'))
    sc.play(2, 'R:16 | R:16 | @4 A4:32/*101')
    sc.play(3, '@1 ' + bassline('Dm | Dm', [0, 'R', 'R', 'R'], step=4, lo='A1') + ' | ' + bassline('Dm', [0], step=2, lo='A1') + ' | A1:12 R:4')
    sc.drum('k...k...k...k... k...k...k...k... r.r.r.r.r.r.r.r. rrrrrrrrrrrr....')
    A_CH = 'Dm | Bb | C | A | Dm | Bb | C | A'
    A = ('@2 D5:2 R:2 A4:2 D5:2 E5:2 F5:2 E5:2 D5:2 | F5:2 R:2 F5:2 D5:2 Bb4:2 D5:2 F5:2 Bb5:2 | '
         'G5:2 R:2 G5:2 E5:2 C5:2 E5:2 G5:2 C6:2 | C#6:4 A5:2 E5:2 C#5:2 E5:2 A5:2 C#6:2 | '
         'D6:2 R:2 A5:2 D6:2 E6:2 F6:2 E6:2 D6:2 | F6:4 D6:2 Bb5:2 F6:4 D6:4 | '
         'E6:2 G6:2 E6:2 C6:2 G5:2 C6:2 E6:2 G6:2 | A6:4 G6:2 E6:2 C#6:4 A5:4')
    B_CH = 'Gm | Bb | C | A | Gm | Bb | Eb | A'
    B = ('@2 G5:1 G5:1 R:2 D5:2 G5:2 A5:2 Bb5:2 A5:2 G5:2 | F5:1 F5:1 R:2 D5:2 F5:2 Bb5:4 A5:4 | '
         'G5:1 G5:1 R:2 E5:2 G5:2 C6:4 Bb5:4 | A5:4 C#6:4 E6:4 G6:4 | '
         'Bb6:4 A6:2 G6:2 D6:4 Bb5:4 | D6:2 F6:2 D6:2 Bb5:2 F5:2 Bb5:2 D6:2 F6:2 | '
         'G6:4 Eb6:2 Bb5:2 G5:4 Bb5:4 | A5:2 C#6:2 E6:2 A6:2 G6:2 E6:2 C#6:2 A5:2')
    sc.mark('DROP')
    sc.play(1, PUMP(A_CH, lo='A3'))
    sc.play(2, A)
    sc.play(3, '@1 ' + bassline(A_CH, OFFBASS, step=1, lo='A1'))
    sc.drum(drop_drums(8))
    sc.play(1, '@5 ' + broken(B_CH, [0, 1, 2, 3], step=1, lo='A3'))
    sc.play(2, B)
    sc.play(3, '@1 ' + bassline(B_CH, ['R', 0, 12, 12], step=1, lo='A1'))
    sc.drum(drop_drums(8))
    # mini break: half-time stomp, roll back into the drop
    sc.play(1, PUMP('Bb | C | A | A', lo='A3'))
    sc.play(2, '@3 D6:8 C6:8 | E6:8 G6:8 | @4 A4:32/*101')
    sc.play(3, '@1 Bb1:4 R:4 Bb1:4 R:4 | C2:4 R:4 C2:4 R:4 | ' + bassline('A', [0], step=2, lo='A1') + ' | A1:12 R:4')
    sc.drum('k.......s....... k.......s...k.k. r.r.r.r.r.r.r.r. rrrrrrrrrrrr....')
    return sc.build(loop_to='DROP')


# ================================================================ STAGE5: "Signal Lost" (C minor acid house, 128 BPM)
def signal_lost():
    sc = Score('Signal Lost', 7)
    edm_setup(sc, W_SAW)
    sc.duty(6, 'Beep', duty=1, vol=10, decay=1)
    ACID = [0, 0, (12, '303'), 0, 'R', 0, (15, '303'), 0, 0, (12, '303'), 0, 10, 'R', 0, 7, (12, '303')]
    CH = 'Cm | Ab | Fm | G | Cm | Ab | Fm | G'
    # intro: UFO beeps, kick, the acid line arrives
    sc.play(1, '@6 C6:2 R:2 F#6:2 R:2 C6:2 R:2 F#6:2 R:2 | R:16 | ' + PUMP('Cm | Cm', lo='G3'))
    sc.play(2, 'R:16 | R:16 | R:16 | @6 G5:2 R:2 G5:2 R:2 Bb5:2 R:2 B5:2 D6:2')
    sc.play(3, 'R:16 | @1 ' + bassline('Cm | Cm | Cm', ACID, step=1, lo='E1'))
    sc.drum('k...k...k...k... k...k...k...k... k.o.s.o.k.o.s.o. k.o.s.o.k.o.s.ok')
    # build: whole-tone 16ths (the saucer signal), riser, roll
    sc.mark('BUILD')
    sc.play(1, '@5 ' + broken('Caug | Ebaug | Caug | Ebaug', [0, 1, 2, 3], step=1, lo='C4'))
    sc.play(2, 'R:16 | R:16 | @4 C5:32/*101')
    sc.play(3, '@1 ' + bassline('Cm | Cm | Cm', ACID, step=1, lo='E1') + ' | G1:12 R:4')
    sc.drum(BUILD)
    lead = ('@2 C5:2 R:2 G4:2 C5:2 D5:2 Eb5:2 D5:2 C5:2 | Eb5:4 C5:2 Ab4:2 R:2 Ab4:2 C5:2 Eb5:2 | '
            'F5:2 R:2 F5:2 Ab5:2 C6:4 Ab5:4 | G5:2 B5:2 D6:2 R:2 B5:2 G5:2 D5:4 | '
            'C6:2 R:2 G5:2 C6:2 D6:2 Eb6:2 D6:2 C6:2 | Eb6:4 C6:2 Ab5:2 Eb6:4 C6:4 | '
            'F6:2 Eb6:2 C6:2 Ab5:2 F5:2 Ab5:2 C6:2 F6:2 | G6:4 F6:2 D6:2 B5:4 G5:4')
    # drop A
    sc.play(1, PUMP(CH, lo='G3'))
    sc.play(2, lead)
    sc.play(3, '@1 ' + bassline(CH, ACID, step=1, lo='E1'))
    sc.drum(drop_drums(8))
    # break: acid alone with beeps, then the whole-tone signal again
    sc.play(1, '@6 C6:2 R:2 F#6:2 R:2 C6:2 R:2 F#6:2 R:2 | D6:2 R:2 G#6:2 R:2 D6:2 R:2 G#6:2 R:2 | @5 '
            + broken('Caug | Ebaug', [0, 1, 2, 3], step=1, lo='C5'))
    sc.play(2, 'R:16 | R:16 | @4 G4:32/*101')
    sc.play(3, '@1 ' + bassline('Cm | Cm | Cm', ACID, step=1, lo='E1') + ' | G1:12 R:4')
    sc.drum('h.h.o.h.h.h.o.h. h.h.o.h.h.h.o.hh r.r.r.r.r.r.r.r. rrrrrrrrrrrr....')
    # drop B: 16th plucks, the whole-tone turn on every 4th bar
    CH_B = 'Cm | Ab | Fm | Ebaug | Cm | Ab | Fm | Ebaug'
    sc.play(1, '@5 ' + broken(CH_B, [0, 2, 3, 2], step=1, lo='G4'))
    sc.play(2, tr(lead, 0))
    sc.play(3, '@1 ' + bassline(CH, ACID, step=1, lo='E1'))
    sc.drum(drop_drums(8))
    return sc.build(loop_to='BUILD')


# ================================================================ FLATWOODS: "Braxton Breach" (Bb minor, 179 BPM)
def braxton_breach():
    sc = Score('Braxton Breach', 5)
    edm_setup(sc, W_REESE)
    # intro: siren and kicks, roll
    sc.play(1, 'R:16 | R:16 | ' + PUMP('Bbm | F', lo='F3'))
    sc.play(2, '@6 Bb4:8/*106 Bb5:8/*206 | Bb4:8/*106 Bb5:8/*206 | @4 F4:32/*102')
    sc.play(3, '@1 ' + bassline('Bbm | Bbm', [0, 'R', 'R', 'R'], step=4, lo='F1') + ' | ' + bassline('Bbm', [0], step=2, lo='F1') + ' | F1:12 R:4')
    sc.drum('k...k...k...k... k...k...k...k... r.r.r.r.r.r.r.r. rrrrrrrrrrrr....')
    A_CH = 'Bbm | Gb | Db | F | Bbm | Gb | Db | F'
    A = ('@2 Bb4:2 R:2 F4:2 Bb4:2 C5:2 Db5:2 C5:2 Bb4:2 | Db5:2 R:2 Db5:2 Bb4:2 Gb4:2 Bb4:2 Db5:2 Gb5:2 | '
         'F5:2 R:2 F5:2 Db5:2 Ab4:2 Db5:2 F5:2 Ab5:2 | A5:4 F5:2 C5:2 A4:2 C5:2 F5:2 A5:2 | '
         'Bb5:2 R:2 F5:2 Bb5:2 C6:2 Db6:2 C6:2 Bb5:2 | Db6:4 Bb5:2 Gb5:2 Db6:4 Bb5:4 | '
         'Ab6:2 F6:2 Db6:2 Ab5:2 F5:2 Ab5:2 Db6:2 F6:2 | E6:4 C6:2 A5:2 F5:4 C6:4')
    B_CH = 'Bbaug | Baug | Bbaug | Baug | Gb | Ab | Bbaug | F'
    B = ('@2 Bb4:1 C5:1 D5:1 E5:1 F#5:1 G#5:1 Bb5:2 G#5:2 F#5:2 D5:4 | B4:1 C#5:1 D#5:1 F5:1 G5:1 A5:1 B5:2 A5:2 G5:2 D#5:4 | '
         'D6:2 R:2 Bb5:2 F#5:2 D6:2 R:2 F#6:4 | D#6:2 R:2 B5:2 G5:2 D#6:2 R:2 G6:4 | '
         'Gb5:2 Bb5:2 Db6:2 Gb6:2 F6:4 Db6:4 | Ab5:2 C6:2 Eb6:2 Ab6:2 G6:4 Eb6:4 | '
         'F#6:4 D6:4 Bb5:4 F#5:4 | A5:2 C6:2 F6:2 A6:2 G6:2 F6:2 E6:2 C6:2')
    sc.mark('DROP')
    sc.play(1, PUMP(A_CH, lo='F3'))
    sc.play(2, A)
    sc.play(3, '@1 ' + bassline(A_CH, OFFBASS, step=1, lo='F1'))
    sc.drum(drop_drums(8))
    sc.play(1, '@5 ' + broken(B_CH, [0, 1, 2, 3], step=1, lo='F3'))
    sc.play(2, B)
    sc.play(3, '@1 ' + bassline(B_CH, ['R', 0, 12, 12], step=1, lo='F1'))
    sc.drum(drop_drums(8))
    # siren break back into the drop
    sc.play(1, PUMP('Gb | Ab | F | F', lo='F3'))
    sc.play(2, '@6 Bb4:8/*106 Bb5:8/*206 | Bb4:8/*106 Bb5:8/*206 | @4 F4:32/*101')
    sc.play(3, '@1 Gb1:4 R:4 Gb1:4 R:4 | Ab1:4 R:4 Ab1:4 R:4 | ' + bassline('F', [0], step=2, lo='F1') + ' | F1:12 R:4')
    sc.drum('k.......s....... k.......s...k.k. r.r.r.r.r.r.r.r. rrrrrrrrrrrr....')
    return sc.build(loop_to='DROP')


SONGS = {'STAGE': overdrive, 'BOSS': hard_target, 'STAGE5': signal_lost, 'FLATWOODS': braxton_breach}
