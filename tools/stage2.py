"""Stage 2: Stormwall Nebula (Mothman mid-boss, Thunderbird boss)."""
import paint as PT
import art_s2 as A
import art_dex as X

NAME = "STORMWALL NEBULA"

def build(sb):
    base = sb.slots({0: 'SPACE2', 1: 'NEB2', 2: 'CLOUD', 3: 'BEACON', 6: 'ZAP', 7: 'PYLON'})
    lair = sb.slots({0: 'SPACE2', 1: 'NEB2', 2: 'CLOUD', 3: 'BEACON', 6: 'TB0', 7: 'TB1'})
    sb.lib.add('CLOUDF'); sb.lib.add('ZAPF'); sb.lib.add('TB0HIT')

    calm = PT.stars_canvas(160, 8 * 40, seed=201)
    PT.stamp_mask(calm, A.soften(PT.nebula_stamp(64, 48, 23)), 24, 64)
    PT.stamp_mask(calm, A.soften(PT.nebula_stamp(48, 40, 24)), 96, 200)
    sb.section('open0', calm, base)
    sb.section('storm0', A.storm_canvas(8 * 128, seed=202, density=0.8, lit_frac=0.2), base)
    sb.section('relay0', A.storm_canvas(8 * 96, seed=203, density=0.5, lit_frac=0.2, pylons=True), base)
    sb.section('eye0', A.storm_canvas(8 * 120, seed=204, density=1.0, lit_frac=0.5), base)
    sb.section('approach0', PT.stars_canvas(160, 8 * 40, seed=205, density=0.006), base)
    tb = A.thunderbird
    sb.boss_image(tb('open', 'shut'),
                  [('open', tb('open', 'shut')), ('angry', tb('angry', 'shut')), ('closed', tb('closed', 'shut')),
                   ('screech', tb('angry', 'open')), ('charged', tb('angry', 'shut', True))],
                  lair, stars_seed=206)
    sb.checkpoint(1, sb.sections['relay0'] - 18 + 20)
    sb.checkpoint(2, sb.sections['approach0'] + 6)

    sb.enemy_pals(['#000000', '#FFFFFF', '#F8E848', '#4040A0'],
                  ['#000000', '#60E8FF', '#E8E8F0', '#202838'],     # impundulu, clouds, bolts
                  ['#000000', '#41EEF6', '#6E8A52', '#0C140A'],     # lindworm
                  ['#000000', '#FF3040', '#5E5090', '#0A0A14'])     # mothman
    sb.sprite("MOTH_A", X.IMPU_A, 16, pal=1)
    sb.sprite("MOTH_B", X.IMPU_B, 16, pal=1)
    sb.sprite("EEL_HEAD", X.lind_head(), 16, pal=2)
    sb.sprite("EEL_SEG", X.LIND_SEG, 8, pal=2)
    sb.sprite("BOLT", A.BOLT, 8, pal=1)
    sb.sprite("CLOUD_A", A.CLOUD_A, 16, pal=1)
    sb.sprite("CLOUD_B", A.CLOUD_B, 16, pal=1)
    sb.sprite("MOTHMAN", X.sheet32('mothman', 'idle0', {(8, 8, 16): 3, (57, 49, 82): 2, (255, 41, 57): 1}), 32, 32, pal=3)
    return 'TB0HIT'
