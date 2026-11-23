#ifndef ARCADE_ART_H
#define ARCADE_ART_H
#include <gb/gb.h>
#include <stdint.h>
BANKREF_EXTERN(arcade_art)
#define ARC_ART_NT 105
#define ARC_ART_NPAL 5
extern const uint8_t arc_art_tiles[], arc_art_map[], arc_art_attr[];
extern const uint16_t arc_art_pals[];
#endif
