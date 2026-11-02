"""Stage 1: The Rimeveil Lane (Space Kraken)."""
import paint as PT
import sprites as S
import art_dex as X

NAME = "RIMEVEIL LANE"

def build(sb):
    base = sb.slots({0: 'SPACE', 1: 'NEBULA', 2: 'METAL', 3: 'LIGHTS', 6: 'ICE', 7: 'RUST'})
    wreck = dict(base); wreck[6] = 'TENT'
    lair = sb.slots({0: 'SPACE', 1: 'NEBULA', 2: 'METAL', 3: 'LIGHTS', 6: 'K0', 7: 'K1'})
    sb.slots(wreck)

    cvA = PT.station_canvas(8 * 56, seed=11)
    PT.launch_bay(cvA)
    PT.stamp_mask(cvA, PT.nebula_stamp(72, 48, 10), 48, 24)
    sb.section('station0', cvA, base)

    cvB = PT.stars_canvas(160, 8 * 128, seed=41)
    nebs = [PT.nebula_stamp(72, 48, 10), PT.nebula_stamp(56, 40, 12)]
    for i, (x, y) in enumerate([(8, 880), (96, 680), (16, 480), (80, 280), (24, 64)]):
        PT.stamp_mask(cvB, nebs[i % 2], x, y)
    sb.section('open0', cvB, base)
    sb.section('ice0', PT.ice_canvas(8 * 136, seed=21), base)
    sb.section('gap1', PT.stars_canvas(160, 8 * 24, seed=51), base)
    sb.section('convoy0', PT.convoy_canvas(8 * 56, seed=81), wreck)
    sb.section('wreck0', PT.wreck_canvas(8 * 136, seed=31), wreck)
    sb.section('approach0', PT.stars_canvas(160, 8 * 40, seed=61, density=0.006), base)
    sb.boss_image(PT.kraken('open', 'shut'),
                  [('open', PT.kraken('open', 'shut')), ('angry', PT.kraken('angry', 'shut')),
                   ('closed', PT.kraken('closed', 'shut')), ('spit', PT.kraken('angry', 'open'))],
                  lair, stars_seed=71)
    sb.checkpoint(1, sb.sections['ice0'] - 18 + 98)
    sb.checkpoint(2, sb.sections['approach0'] + 6)

    sb.enemy_pals(['#000000', '#F8F8F8', '#FF7052', '#380810'],   # coral: reads against the purple nebula
                  ['#000000', '#F8E040', '#C03888', '#3A0C30'],     # krakenlings in the Kraken's colours
                  ['#000000', '#F0F8FF', '#70C8F0', '#183868'],
                  ['#000000', '#B8F050', '#3C9030', '#102008'])
    sb.sprite("KL_A", X.KL_A, 16, pal=1)
    sb.sprite("KL_B", X.KL_B, 16, pal=1)
    sb.sprite("JELLY_A", S.JELLY_A, 16, pal=2)
    sb.sprite("JELLY_B", S.JELLY_B, 16, pal=2)
    sb.sprite("SKIFF_A", S.SKIFF_A, 16, pal=3)
    sb.sprite("SKIFF_B", S.SKIFF_B, 16, pal=3)
    sb.sprite("ICE_A", S.ICE_A, 16, pal=2)
    sb.sprite("ICE_B", S.ICE_B, 16, pal=2)
    sb.sprite("GUN", S.GUNSHIP_A + ['.' * 32] * 8, 32, 32, pal=3)
    sb.sprite("TENT_SEG", S.TENT_SEG, 8, pal=1)
    sb.sprite("TENT_TIP", S.TENT_TIP, 8, pal=1)
    # boss hit-flash palette
    PT.BG_PALS['K0HIT'] = [PT.SPACE, '#702060', '#E060A8', '#FFC8E8']
    sb.lib.add('K0HIT')
    return 'K0HIT'
