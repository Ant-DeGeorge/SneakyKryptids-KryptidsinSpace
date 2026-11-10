"""Player weapon colours and effect sprites (weapon flair).

Colour-vision-deficiency-safe scheme: enemy bullets own the warm hues (coral, yellow,
orange, pink) in every stage, so player weapons stay on the cool side and are told
apart by BRIGHTNESS as well as hue, plus shape and animation:
  TWIN   ice cyan   - mid-bright, twin needles / heavy lance
  SPREAD violet    - darkest body, spinning 4-point stars
  WISP   pale frost - brightest, near-white comet with a wiggling tail
OBJ palette order: 0 transparent, 1 light (core), 2 body, 3 dark edge.
"""
WEAPON_NAMES = ['TWIN', 'SPREAD', 'WISP']
WEAPON_PALS = [
    ['#000000', '#F8F8F8', '#48D0F8', '#1850C0'],   # TWIN   ice cyan
    ['#000000', '#F0E0F8', '#A070F8', '#401890'],   # SPREAD violet
    ['#000000', '#FFFFFF', '#E8F0F0', '#203C88'],   # WISP   pale frost
]
# max-level "charged" shimmer: colour 1 steps to this tint and back (slow, low contrast)
WEAPON_GLOW = ['#B8F0FF', '#D8C0FF', '#C0D8FF']

# ------------------------------------------------------------------ effect sprites (8x16, OBJ bank 0)
# Original art. Hit point of every shot stays at pixel (4, 3) like the old shots;
# rows below that are trail only (no extra sprites: 8x16 OBJs are 16 tall anyway).
SHOT_TWIN = [          # twin ice needles with a dotted trail (replaces the old twin art)
".1...1..",
"121.121.",
"121.121.",
".1...1..",
".2...2..",
".2...2..",
".3...3..",
".3...3..",
"........",
".3...3..",
"........",
"........",
] + ["........"] * 4
SHOT_SPREAD = [        # spread frame A: 4-point star
"...1....",
"..121...",
".12321..",
"..121...",
"...1....",
"...3....",
"........",
"........",
] + ["........"] * 8
SHOT_WISP = [          # wisp comet: white head with a navy rim (reads on dark space AND the bright moon),
"..3333..",            # zig-zag tail (drawn X-flipped every 4 frames = wiggle)
".311113.",
".312213.",
".311113.",
"..3113..",
"...22...",
"....3...",
"...3....",
"....3...",
"...3....",
"....3...",
"........",
] + ["........"] * 4
SHOT_TWIN2 = [         # heavy lance: twin level 1+ centre shot
"...11...",
"..1111..",
"..1221..",
"..1221..",
"..1221..",
"..2332..",
"..2332..",
"...33...",
"...22...",
"...33...",
"........",
"...3....",
"........",
"...3....",
"........",
"........",
]
SHOT_SPREAD_B = [      # spread frame B: the star turned 45 degrees (spin)
".2...2..",
"..121...",
"..131...",
"..121...",
".2...2..",
"........",
"........",
"........",
] + ["........"] * 8
MUZZLE = [             # muzzle flash at the gun, in the weapon's colours
"...11...",
".1.11.1.",
"..1111..",
"11111111",
"..1221..",
".2.22.2.",
"...33...",
"........",
] + ["........"] * 8
HIT = [                # impact burst where a shot lands, in the weapon's colours
"1..2..1.",
".1.2.1..",
"..131...",
"2213122.",
"..131...",
".1.2.1..",
"1..2..1.",
"........",
] + ["........"] * 8
