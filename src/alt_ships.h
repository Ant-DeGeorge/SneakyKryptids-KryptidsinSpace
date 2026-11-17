// Alternate player ships. Same layout as ST_SHIP_TOP/BOT/BOTB (12 tiles, 8x16 OBJ mode):
//   tiles 0-3 top (OBJ pal 0), 4-7 bottom frame A, 8-11 bottom frame B (OBJ pal 1).
#ifndef ALT_SHIPS_H
#define ALT_SHIPS_H
#include <stdint.h>
#define N_ALT_SHIPS 7
#define ALT_SHIP_NT 12
#include <gb/gb.h>
BANKREF_EXTERN(alt_ships)  // use BANK(alt_ships) with ld_spr_data / ld_spr_pal
extern const uint8_t alt_ship_tiles[];  // N_ALT_SHIPS * 12 tiles * 16 bytes
extern const uint16_t alt_ship_pals[];  // N_ALT_SHIPS * 2 palettes * 4 colours (ship, engine)
#define ALT_SHIP_CURRENT 0
#define ALT_SHIP_INTERDICTOR 1
#define ALT_SHIP_GLYPH_FANG 2
#define ALT_SHIP_RUST_RUNNER 3
#define ALT_SHIP_DEEP_SPACE 4
#define ALT_SHIP_NIGHT_HULL 5
#define ALT_SHIP_DRYDOCK 6
#endif
