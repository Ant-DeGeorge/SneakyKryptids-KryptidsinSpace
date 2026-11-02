"""Stage 3: the Barrens Belt (Nubs Mk II mid-boss, Jersey Devil boss)."""
import paint as PT
import art_s3 as A
import sprites as S
import art_dex as X

NAME = "BARRENS BELT"

def build(sb):
    base = sb.slots({0: 'SPACE3', 1: 'FOG', 2: 'ROCK', 3: 'PINE', 6: 'CABIN', 7: 'WISPBG'})
    lair = sb.slots({0: 'SPACE3', 1: 'FOG', 2: 'ROCK', 3: 'PINE', 6: 'JD0', 7: 'JD1'})
    sb.lib.add('JD0HIT'); sb.lib.add('VOID3')

    sb.section('edge0', A.barrens_canvas(8 * 48, seed=301, density=0.5), base)
    sb.section('forest0', A.barrens_canvas(8 * 136, seed=302, density=1.0), base)
    sb.section('cabins0', A.barrens_canvas(8 * 96, seed=303, cabins=True, density=0.9), base)
    bog = A.barrens_canvas(8 * 120, seed=304, density=0.6)
    fogs = [A.fog_stamp(64, 48, 40), A.fog_stamp(56, 40, 41)]
    for k in range(6):
        PT.stamp_mask(bog, fogs[k % 2], [16, 88, 40, 96, 8, 64][k], 40 + k * 152)
    sb.section('bog0', bog, base)
    sb.section('approach0', PT.stars_canvas(160, 8 * 40, seed=305, density=0.006), base)
    jd = X.jersey_devil_boss()
    jd[3] = ('fire', jd[3][1])
    sb.boss_image(jd[0][1], jd, lair, stars_seed=306)
    sb.checkpoint(1, sb.sections['cabins0'] - 18 + 20)
    sb.checkpoint(2, sb.sections['approach0'] + 6)

    sb.enemy_pals(['#000000', '#FFF8C0', '#F89020', '#801808'],
                  ['#000000', '#A8A8B4', '#5A6448', '#0A0A08'],     # pukwudgie, Nubs Mk II
                  ['#000000', '#41EEF6', '#D5C5AC', '#3A1E36'],     # devil's eggs, devil's tail
                  ['#000000', '#D5FFF6', '#41EEF6', '#081010'])     # will-o'-the-wisp
    sb.sprite("IMP_A", X.PUK_A, 16, pal=1)
    sb.sprite("IMP_B", X.PUK_B, 16, pal=1)
    sb.sprite("CONE_A", X.EGG_A, 16, pal=2)
    sb.sprite("CONE_B", X.EGG_B, 16, pal=2)
    sb.sprite("WISP_A", X.wisp(True), 16, pal=3)
    sb.sprite("WISP_B", X.wisp(False), 16, pal=3)
    sb.sprite("GUN", S.GUNSHIP_A + ['.' * 32] * 8, 32, 32, pal=1)
    sb.sprite("TAIL_SEG", A.TAIL_SEG, 8, pal=1)
    sb.sprite("TAIL_TIP", A.TAIL_TIP, 8, pal=1)
    return 'JD0HIT'
