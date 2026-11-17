// Arcade mode: ship select, the endless loop through the four scrolling missions, the results card,
// and its own small save block. Autobanked.
#pragma bank 255
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "assets.h"
#include "sound.h"
#include "game.h"
#include "objs.h"
#include "stage_def.h"
#include "alt_ships.h"
#include "arcade.h"

// ------------------------------------------------------------------ the ships
// spd: added to the pilot's base speed. hb: added to the hitbox half-size. sh: shield recharge in
// quarters of the normal time (4 = normal, 2 = twice as fast, 8 = twice as slow).
typedef struct {
    const char *name, *role, *d1, *d2;
    uint8_t wtype, wlevel, lives, bombs;
    int8_t spd, hb;
    uint8_t sh, bud;
} Ship;
static const Ship ships[N_ALT_SHIPS] = {
    {"SNEAKY KRYPTID", "ALL-ROUNDER", "TWIN GUN, 2 PIPS.",   "NO WEAK SPOTS.",      0, 1, 3, 3,   0,  0, 4, 0},
    {"INTERDICTOR",    "GUNSHIP",     "TWIN GUN AT FULL",    "POWER. 2 BOMBS.",     0, 2, 3, 2,   0,  0, 4, 0},
    {"GLYPH FANG",     "HEX CASTER",  "WISP GUN, 2 PIPS.",   "5 BOMBS.",            2, 1, 3, 5,   0,  0, 4, 0},
    {"RUST RUNNER",    "TANK",        "5 SHIPS. SPREAD GUN.", "SLOW TO TURN.",      1, 0, 5, 3, -32,  0, 4, 0},
    {"DEEP SPACE",     "WARDED",      "SHIELD RETURNS TWICE", "AS FAST. ONE BUDDY.", 0, 0, 3, 3,   0,  0, 2, 1},
    {"NIGHT HULL",     "GHOST",       "FAST. HARD TO HIT.",  "ONLY 2 SHIPS.",       1, 1, 2, 3,  48, -1, 4, 0},
    {"DRYDOCK",        "BRUISER",     "SPREAD GUN AT FULL",  "POWER. SLOW SHIELD.", 1, 2, 4, 4, -40,  0, 8, 0},
};
static const uint16_t sh_base[3] = {240, 420, 720};          // normal shield recharge by difficulty

// ------------------------------------------------------------------ run state, carried between stages
static uint8_t a_n;                                          // stages cleared this run
static uint8_t a_lives, a_bombs, a_wtype, a_wlevel;
static uint32_t a_score;
static uint8_t a_hearts;

void arc_begin(void) BANKED {
    const Ship *s = &ships[ship_sel];
    uint16_t t;
    if (a_n == 0) { lives = s->lives; bombs = s->bombs; wtype = s->wtype; wlevel = s->wlevel; }
    else { score = a_score; lives = a_lives; bombs = a_bombs; wtype = a_wtype; wlevel = a_wlevel; hearts = a_hearts; }
    next_extend = 0xFFFFFFFFUL;                              // no score extends in arcade: the ships you pick are the ships you get
    p_spd += s->spd; p_hb += s->hb;
    scr_n = 2; scr_dy = 0x80;                                // arcade scrolls at the old, faster pace
    arc_bspd = a_n > 6 ? 21 : a_n * 3;                       // enemy shots speed up every stage (tops out in loop 2)
    t = (sh_base[gmode] >> 2) * s->sh + (uint16_t)(a_n > 12 ? 12 : a_n) * 120;  // and the shield takes two seconds longer each stage
    arc_shield = t;
}

// ------------------------------------------------------------------ save block: best score per difficulty, last ship
#define ASAVE ((uint8_t *)0xA400)
#define ASAVE_LEN 17                                         // 'S' 'K' 'A' ver, ship, 3 x u32, sum
static uint32_t a_hi[3];
static uint8_t a_ship;
static uint8_t asum(const uint8_t *p) { uint8_t s = 0x3C, i; for (i = 0; i < ASAVE_LEN - 1; i++) s += p[i]; return s; }
static void asave_read(void) {
    uint8_t b[ASAVE_LEN];
    ENABLE_RAM; SWITCH_RAM(0);
    memcpy(b, ASAVE, ASAVE_LEN);
    DISABLE_RAM;
    memset(a_hi, 0, sizeof a_hi); a_ship = 0;
    if (b[0] != 'S' || b[1] != 'K' || b[2] != 'A' || b[3] != 1 || b[16] != asum(b)) return;
    a_ship = b[4] < N_ALT_SHIPS ? b[4] : 0;
    memcpy(a_hi, b + 5, 12);
}
static void asave_write(void) {
    uint8_t b[ASAVE_LEN];
    b[0] = 'S'; b[1] = 'K'; b[2] = 'A'; b[3] = 1; b[4] = a_ship;
    memcpy(b + 5, a_hi, 12);
    b[16] = asum(b);
    ENABLE_RAM; SWITCH_RAM(0);
    memcpy(ASAVE, b, ASAVE_LEN);
    DISABLE_RAM;
}

// ------------------------------------------------------------------ screens
static const uint16_t pal_text[4] = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(72, 224, 200), RGB8(248, 248, 248) };
static const uint16_t pal_gold[4] = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(232, 96, 24), RGB8(248, 208, 56) };
static const uint16_t pal_dim[4]  = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(40, 52, 70), RGB8(90, 100, 120) };
static void center(uint8_t y, const char *s, uint8_t attr) { txt_put((20 - (uint8_t)strlen(s)) >> 1, y, s, 0, attr); }
static void put_u32(uint8_t x, uint8_t y, uint32_t v, uint8_t attr) {
    uint32_t keep = score;
    uint8_t t[7], i;
    score = v; score_tiles(t); score = keep;
    for (i = 0; i < 7; i++) { VBK_REG = 1; set_bkg_tile_xy(x + i, y, attr); VBK_REG = 0; set_bkg_tile_xy(x + i, y, t[i]); }
}
static void put_n(uint8_t x, uint8_t y, uint8_t v, uint8_t attr) {
    char c[3]; c[0] = '0' + v / 10; c[1] = '0' + v % 10; c[2] = 0;
    txt_put(x, y, c[0] == '0' ? c + 1 : c, 0, attr);
}
static void screen_prep(void) {
    screen_off(); splits_off(); oam_clear();
    n_scx = n_scy = 0;
    VBK_REG = 0;
    clear_bkg(0, 7);
    ld_bkg_data(BANK_A2, 0, 0, N_COMMON, common_tiles);
    ld_spr_data(BANK_A2, 0, N_SPR_TILES, spr_tiles);
    ld_spr_pal(BANK_A2, 0, 8, obj_pals);
    set_bkg_palette(7, 1, pal_text);
    set_bkg_palette(6, 1, pal_gold);
    set_bkg_palette(5, 1, pal_dim);
    txt_fill(0, 0, 20, 18, txt_tile(' '), 7, 0);
}
static void wait_a(uint16_t max) {
    while (max--) { wait_vbl_done(); input_poll(); if (joy_new & (J_A | J_START)) return; }
}

static void ship_page(uint8_t n_ships) {
    const Ship *s = &ships[ship_sel];
    uint8_t open = ship_sel < n_ships;
    txt_fill(0, 2, 20, 12, txt_tile(' '), 7, 0);
    txt_put(6, 2, "SHIP", 0, 5); put_n(11, 2, ship_sel + 1, 7); txt_put(12, 2, "/7", 0, 5);
    if (open) {
        ship_gfx();
        center(9, s->name, 7); center(10, s->role, 6);
        center(12, s->d1, 7); center(13, s->d2, 7);
    } else {
        center(9, "LOCKED", 5);
        txt_put(2, 12, "CLEAR MISSION", 0, 5); put_n(16, 12, ship_sel, 6);
        center(13, "TO UNLOCK IT.", 5);
    }
}
static uint8_t ship_select(uint8_t n_ships) {
    uint8_t t = 0, open, y;
    screen_prep();
    center(0, "ARCADE: ENDLESS RUN", 6);
    txt_put(3, 14, "BEST", 0, 6); put_u32(9, 14, a_hi[gmode], 7);
    center(16, "PAD: CHANGE SHIP", 5);
    center(17, "A: FLY    B: BACK", 5);
    ship_sel = a_ship < n_ships ? a_ship : 0;
    ship_page(n_ships);
    screen_on(LCDC_SCREEN);
    music_play(MUS_MAP);
    while (1) {
        wait_vbl_done(); input_poll(); t++;
        open = ship_sel < n_ships;
        if (joy_new & J_LEFT) { ship_sel = ship_sel ? ship_sel - 1 : N_ALT_SHIPS - 1; sfx_play(SFX_SELECT); ship_page(n_ships); }
        if (joy_new & J_RIGHT) { ship_sel = ship_sel == N_ALT_SHIPS - 1 ? 0 : ship_sel + 1; sfx_play(SFX_SELECT); ship_page(n_ships); }
        y = 40 + 16 + ((t >> 4) & 1);
        if (open) {
            oam_fixed(0, 72 + 8, y, ST_SHIP_TOP, 0); oam_fixed(1, 80 + 8, y, ST_SHIP_TOP + 2, 0);
            oam_fixed(2, 72 + 8, y + 16, (t & 4) ? ST_SHIP_BOT : ST_SHIP_BOTB, 1);
            oam_fixed(3, 80 + 8, y + 16, ((t & 4) ? ST_SHIP_BOT : ST_SHIP_BOTB) + 2, 1);
        } else for (y = 0; y < 4; y++) oam_fixed(y, 0, 0, 0, 0);
        if ((joy_new & J_A) && open) { sfx_play(SFX_START); wait_frames(20); return 1; }
        if ((joy_new & J_A) && !open) sfx_play(SFX_HURT);
        if (joy_new & J_B) { ship_sel = 0; return 0; }
    }
}
static void stage_card(uint8_t sid) {
    screen_prep();
    txt_put(6, 6, "STAGE", 0, 6); put_n(12, 6, a_n + 1, 7);
    center(8, stage_names[sid], 7);
    if (a_n) center(11, "SHOTS GET FASTER", 5);
    if (a_n >= 4) { txt_put(7, 13, "LOOP", 0, 5); put_n(12, 13, (a_n >> 2) + 1, 6); }
    screen_on(LCDC_SCREEN);
    wait_a(110);
}
static void over_card(uint8_t quit) {
    uint8_t best = score > a_hi[gmode];
    if (best) a_hi[gmode] = score;
    a_ship = ship_sel;
    asave_write();
    screen_prep();
    center(2, quit ? "RUN ENDED" : "SHIP LOST", 6);
    center(4, ships[ship_sel].name, 7);
    txt_put(2, 7, "STAGES CLEARED", 0, 5); put_n(17, 7, a_n, 7);
    txt_put(2, 9, "SCORE", 0, 5); put_u32(11, 9, score, 7);
    txt_put(2, 11, "BEST", 0, 5); put_u32(11, 11, a_hi[gmode], 6);
    if (best) center(13, "NEW BEST RUN!", 6);
    center(17, "KRYPTIDS IN SPACE", 5);
    screen_on(LCDC_SCREEN);
    if (quit) music_stop(); else music_play(MUS_OVER);
    wait_frames(60);
    wait_a(900);
}

void arcade_run(uint8_t n_ships) BANKED {
    uint8_t res, sid;
    asave_read();
    while (ship_select(n_ships)) {
        g_arc = 1; a_n = 0; start_cp = 0;
        memset(g_gear, 0, N_GEAR);
        if (ships[ship_sel].bud) g_gear[GEAR_KENNEL] = 2;
        while (1) {
            sid = a_n & 3;
            stage_card(sid);
            res = stage_run(sid);
            if (res != RES_CLEAR) break;
            add_score(2000 + (uint16_t)a_n * 500);            // stage bonus, growing each stage
            a_hearts = hearts; a_score = score; a_lives = lives; a_bombs = bombs; a_wtype = wtype; a_wlevel = wlevel;
            if (a_n < 250) a_n++;
        }
        g_arc = 0; arc_bspd = 0;
        over_card(res == RES_QUIT);
        ship_sel = 0;
    }
    ship_sel = 0; g_arc = 0; arc_bspd = 0;
}
