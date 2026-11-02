"""Stage 4: Frostbite Moon (Beast of Bray Road mid-boss, Wendigo boss)."""
import paint as PT
import art_s4 as A
import art_dex as X

NAME = "FROSTBITE MOON"

def build(sb):
    base = sb.slots({0: 'SNOW', 1: 'SNOWD', 2: 'DOME', 3: 'DLIGHT', 6: 'CREV', 7: 'FROSTSKY'})
    lair = sb.slots({0: 'SNOW', 1: 'SNOWD', 2: 'DOME', 3: 'DLIGHT', 6: 'WEN0', 7: 'WEN1'})
    sb.lib.add('WEN0HIT'); sb.lib.add('WHITE4')
    # descent: open sky first (stars drawn in slot 7), then the frozen surface fills the screen
    orbit = PT.stars_canvas(160, 8 * 32, seed=401)
    orbit.pal[orbit.col > 0] = 7
    sb.section('orbit0', orbit, base)
    sb.section('plain0', A.moon_canvas(8 * 128, 402, domes=0.15, lakes=0.25, cracks=0.4), base)
    sb.section('mines0', A.moon_canvas(8 * 96, 403, domes=0.55, lakes=0.1, cracks=0.2), base)
    sb.section('rift0', A.moon_canvas(8 * 120, 404, domes=0.05, lakes=0.35, cracks=0.8, ridges=0.9), base)
    sb.section('approach0', PT.stars_canvas(160, 8 * 40, seed=405, density=0.006), base)
    wv = X.wendigo_boss()
    sb.boss_image(wv[0][1], wv, lair, stars_seed=406)
    sb.checkpoint(1, sb.sections['mines0'] - 18 + 20)
    sb.checkpoint(2, sb.sections['approach0'] + 6)

    sb.enemy_pals(['#000000', '#FFFFFF', '#F84898', '#401030'],
                  ['#000000', '#F8D040', '#6A7480', '#141820'],
                  ['#000000', '#C8F0FF', '#4890D0', '#102040'],
                  ['#000000', '#41EEF6', '#6A7C98', '#101018'])     # beast of bray road, claws
    sb.sprite("TURRET_A", A.TURRET_A, 16, pal=1)
    sb.sprite("TURRET_B", A.TURRET_B, 16, pal=1)
    sb.sprite("BAT_A", X.FBAT_A, 16, pal=2)
    sb.sprite("BAT_B", X.FBAT_B, 16, pal=2)
    sb.sprite("SNAKE_HEAD", X.tatz_head(), 16, pal=2)
    sb.sprite("SNAKE_SEG", X.TATZ_SEG, 8, pal=2)
    sb.sprite("CLAW_SEG", A.CLAW_SEG, 8, pal=3)
    sb.sprite("CLAW_TIP", A.CLAW_TIP, 8, pal=3)
    sb.sprite("BEAST", X.sheet32('bray_road', 'idle0', {(16, 16, 24): 3, (90, 106, 131): 2, (65, 238, 246): 1}), 32, 32, pal=3)
    return 'WEN0HIT'
