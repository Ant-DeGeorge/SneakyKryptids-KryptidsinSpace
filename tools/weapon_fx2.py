"""Weapon flair, part 2: more effect sprites (all original art). Loaded into OBJ VRAM bank 1 at
tile FX2_BASE (80) in the scrolling stages and the gun cam, where both leave that range free.

Colours (OBJ palette indices): 1 light core, 2 body, 3 dark edge.
Most sprites draw with OBJ palette 2, the current weapon's colours (cool hues only, see weapon_fx.py),
so the player's side of the screen never borrows an enemy-bullet colour. Capsule spin frames and the
rune bomb glyphs use palette 7 (pickups / fx), like the capsules and bomb they belong to.
"""
E8 = ["........"] * 8

# ---- impacts per weapon (art in the top 8 rows; hit point at (4, 3))
IMP_STAR_A = [          # spread: a star bursts open
"...1....",
".1.1.1..",
"..121...",
"11232 11".replace(' ', '1'),
"..121...",
".1.1.1..",
"...1....",
"........",
] + E8
IMP_STAR_B = [          # ...and scatters
"1..2..1.",
"........",
"..3.3...",
"2..3..2.",
"..3.3...",
"........",
"1..2..1.",
"........",
] + E8
IMP_FROST_A = [         # wisp: a frost puff
"..222...",
".21112..",
"2111112.",
"2113112.",
"2111112.",
".21112..",
"..222...",
"........",
] + E8
IMP_FROST_B = [         # ...that breaks into crystals
"2.....2.",
".3.1.3..",
"...3....",
".13.31..",
"...3....",
".3.1.3..",
"2.....2.",
"........",
] + E8
SHARD = [               # kill pop: one hot shard, thrown four ways (flips make the others)
"..1.....",
".121....",
"..232...",
"...33...",
"........",
"........",
"........",
"........",
] + E8

# ---- buddy shots: each baby cryptid fires its own (weapon palette, themed by shape)
BUD_INK = [             # kraken spawn: an ink blob with a drip
"..33....",
".3223...",
"321123..",
"322223..",
".3223...",
"..33....",
"...3....",
"....3...",
] + E8
BUD_BOLT = [            # impundulu chick: a little lightning bolt
"....11..",
"...12...",
"..12....",
".1111...",
"...21...",
"..21....",
".12.....",
".1......",
] + E8
BUD_ARROW = [           # pukwudgie: a tiny arrow, fletched
"...1....",
"..121...",
"...2....",
"...2....",
"...3....",
"..3.3...",
"..3.3...",
"........",
] + E8
BUD_FROST = [           # tatzelwurm pup: a frost shard
"...1....",
"..121...",
"..121...",
".12221..",
"..232...",
"..232...",
"...3....",
"........",
] + E8

# ---- Forge's rune circle (bomb): two more glyphs to go with the rune-net glyph
RUNE_B = [
"..333...",
".31213..",
"3122213.",
"3211123.",
"3122213.",
".31213..",
"..333...",
"........",
] + E8
RUNE_C = [
"..333...",
".32123..",
"3211123.",
"3122213.",
"3211123.",
".32123..",
"..333...",
"........",
] + E8

# ---- capsule spin (16 wide, centred so an X-flip mirrors it in place)
CAP_34 = [
"......3333......",
".....322223.....",
"....32111123....",
"...3211111123...",
"...3211111123...",
"...3211111123...",
"...3211112123...",
"...3211121123...",
"...3211211123...",
"...3212111123...",
"...3211111123...",
"...3211111123...",
"....32111123....",
".....322223.....",
"......3333......",
"................",
]
CAP_EDGE = [
".......33.......",
"......3223......",
"......3213......",
"......3213......",
"......3213......",
"......3213......",
"......3213......",
"......3213......",
"......3213......",
"......3213......",
"......3213......",
"......3213......",
"......3213......",
"......3223......",
".......33.......",
"................",
]

# ---- max-level specials (Forge's weapon runes)
LANCE_A = [             # Lance Rune: the twin centre shot becomes a piercing lance
"...11...",
"..1111..",
"..1111..",
"..1221..",
"..1221..",
"..1221..",
"..1221..",
"..2332..",
"..2332..",
"..2332..",
"...33...",
"...22...",
"...33...",
"...2....",
"...3....",
"........",
]
LANCE_B = [             # second frame: the energy ripples down the shaft
"...11...",
"..1111..",
"..1221..",
"..1111..",
"..1221..",
"..1221..",
"..1111..",
"..2332..",
"..2332..",
"..2332..",
"...33...",
"...33...",
"...22...",
"....2...",
"....3...",
"........",
]
ZAP_A = [               # Chain Rune: the spark that jumps to the next target
"...1....",
"....1...",
"...12...",
"..12....",
"...21...",
"....12..",
"...21...",
"..12....",
"...1....",
"....1...",
"...12...",
"..21....",
"...1....",
"....1...",
"...1....",
"........",
]
ZAP_B = [
"....1...",
"...1....",
"..21....",
"...12...",
"....21..",
"...12...",
"..21....",
"...12...",
"....1...",
"...1....",
"..21....",
"...12...",
"....1...",
"...1....",
"....1...",
"........",
]

# ---- muzzle flashes for spread and wisp (twin keeps weapon_fx.MUZZLE)
MUZ_SPREAD = [          # a fan
"1..1..1.",
".1.1.1..",
"..121...",
".12321..",
"..222...",
"........",
"........",
"........",
] + E8
MUZ_WISP = [            # a cold ring
"..111...",
".1...1..",
"1..2..1.",
"1.232.1.",
"1..2..1.",
".1...1..",
"..111...",
"........",
] + E8

# (name, art, width) in VRAM order
SPRITES = [
    ('IMP_STAR_A', IMP_STAR_A, 8), ('IMP_STAR_B', IMP_STAR_B, 8),
    ('IMP_FROST_A', IMP_FROST_A, 8), ('IMP_FROST_B', IMP_FROST_B, 8),
    ('SHARD', SHARD, 8),
    ('BUD_INK', BUD_INK, 8), ('BUD_BOLT', BUD_BOLT, 8), ('BUD_ARROW', BUD_ARROW, 8), ('BUD_FROST', BUD_FROST, 8),
    ('RUNE_B', RUNE_B, 8), ('RUNE_C', RUNE_C, 8),
    ('CAP_34', CAP_34, 16), ('CAP_EDGE', CAP_EDGE, 16),
    ('LANCE_A', LANCE_A, 8), ('LANCE_B', LANCE_B, 8),
    ('ZAP_A', ZAP_A, 8), ('ZAP_B', ZAP_B, 8),
    ('MUZ_SPREAD', MUZ_SPREAD, 8), ('MUZ_WISP', MUZ_WISP, 8),
]
