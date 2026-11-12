// Forge's armory (START on the mission map). Autobanked: moved out of scenes.c to keep bank 7
// inside 16 KB. The caller writes the save when it returns.
#pragma bank 255
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "assets.h"
#include "sound.h"
#include "game.h"

static const uint16_t pal_text[4] = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(72, 224, 200), RGB8(248, 248, 248) };
static const uint16_t pal_teal[4] = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(32, 120, 110), RGB8(72, 224, 200) };
static const uint16_t pal_star[4] = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(232, 96, 24), RGB8(248, 208, 56) };
static const uint16_t pal_dim[4] = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(40, 52, 70), RGB8(90, 100, 120) };
static void center(uint8_t y, const char *s, uint8_t attr) { txt_put((20 - (uint8_t)strlen(s)) >> 1, y, s, 0, attr); }
static void put_num(uint8_t x, uint8_t y, uint16_t v, uint8_t digits, uint8_t attr) {
    static const uint16_t p10[4] = {1000, 100, 10, 1};
    uint8_t i, d;
    char c[2] = {0, 0};
    for (i = 4 - digits; i < 4; i++) {
        d = 0;
        while (v >= p10[i]) { v -= p10[i]; d++; }
        c[0] = '0' + d;
        txt_put(x++, y, c, 0, attr);
    }
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
}
static void arrow_at(uint8_t col, uint8_t row, uint8_t t) {
    oam_fixed(0, col * 8 + 8 - 2 + ((t >> 3) & 1), row * 8 + 16, ST_ARROW, 7);
}
// ------------------------------------------------------------------ Forge's armory
static const char * const gear_name[N_GEAR] = {"HULL PLATING", "CANNON TUNE", "BOMB RACK", "BUDDY BAY", "RUNE ROUNDS",
    "LANCE RUNE", "RICOCHET RUNE", "CHAIN RUNE"};
static const uint8_t gear_max[N_GEAR] = {2, 2, 2, 2, 1, 1, 1, 1};
static const uint16_t gear_cost[N_GEAR][2] = {{150, 350}, {120, 300}, {80, 200}, {200, 400}, {300, 0}, {250, 0}, {250, 0}, {300, 0}};
static const char * const gear_desc[N_GEAR][2] = {
    {"+1 LIFE EVERY\nMISSION", "ANOTHER +1 LIFE.\nTANK MODE."},
    {"START WITH\nWEAPON LEVEL 2", "START WITH\nWEAPON LEVEL 3"},
    {"+1 BOMB EVERY\nMISSION", "ANOTHER +1 BOMB"},
    {"ROOM FOR 3\nBUDDIES", "START EVERY\nMISSION WITH ONE"},
    {"SHOTS BREAK\nBIG ORBS (2 HITS)", ""},
    {"MAX TWIN: CENTRE\nLANCE PIERCES ALL", ""},
    {"MAX SPREAD: OUTER\nSTARS BOUNCE", ""},
    {"MAX WISP: HITS\nJUMP TO ANOTHER", ""}};
#define GEAR_PAGE1 GEAR_LANCE                        // page 2 of the armory: the weapon runes
static const char * const forge_quip[4] = {"FORGE: NICE.\nSHE PURRS NOW.", "FORGE: RUNES\nCARVED. DON'T LICK.",
    "FORGE: PAID IN\nFULL. I LOVE YOU.", "FORGE: DONE. TRY\nNOT TO SCRATCH IT."};
static void put_lines(uint8_t x, uint8_t y, const char *s) {
    uint8_t cx = x; char c[2]; c[1] = 0;
    for (; *s; s++) { if (*s == '\n') { y++; cx = x; continue; } c[0] = *s; txt_put(cx++, y, c, 0, 7); }
}
void armory_run(void) BANKED {
    uint8_t sel = 0, t = 0, i, lv, redraw = 1, quip = 0xFF, pg = 0, g0, gn, r;
    screen_prep();
    set_bkg_palette(6, 1, pal_teal);
    set_bkg_palette(5, 1, pal_star);
    set_bkg_palette(4, 1, pal_dim);
    txt_fill(0, 0, 20, 18, txt_tile(' '), 7, 0);
    center(0, "FORGE'S ARMORY", 7);
    center(17, "A:BUY B:BACK PAD:TAB", 6);
    screen_on(LCDC_SCREEN);
    while (1) {
        g0 = pg ? GEAR_PAGE1 : 0; gn = pg ? N_GEAR - GEAR_PAGE1 : GEAR_PAGE1;
        if (redraw) {
            redraw = 0;
            txt_fill(0, 1, 20, 16, txt_tile(' '), 7, 0);
            txt_put(2, 1, "CREDITS", 0, 6); put_num(13, 1, pf_credits[profile], 4, 7);
            txt_put(1, 2, "GEAR", 0, pg ? 4 : 7); txt_put(8, 2, "WEAPON RUNES", 0, pg ? 7 : 4);
            for (i = 0; i < gn; i++) {
                r = g0 + i; lv = pf_gear[profile][r];
                txt_put(2, 3 + i * 2 + pg, gear_name[r], 0, lv ? 7 : 6);
                { char c[7]; strcpy(c, "LV 0/0"); c[3] = '0' + lv; c[5] = '0' + gear_max[r]; txt_put(3, 4 + i * 2 + pg, c, 0, lv ? 7 : 4); }
                if (lv >= gear_max[r]) txt_put(15, 4 + i * 2 + pg, "MAX", 0, 6);
                else { txt_put(14, 4 + i * 2 + pg, "$", 0, 6); put_num(15, 4 + i * 2 + pg, gear_cost[r][lv], 3, pf_credits[profile] >= gear_cost[r][lv] ? 7 : 4); }
            }
            if (quip != 0xFF) put_lines(1, 14, forge_quip[quip]);
            else {
                r = g0 + sel; lv = pf_gear[profile][r];
                if (lv < gear_max[r]) put_lines(1, 14, gear_desc[r][lv]);
                else if (pg) put_lines(1, 14, "FORGE: CARVED.\nMAX IT TO WAKE IT.");
                else put_lines(1, 14, "FORGE: MAXED OUT.\nSHE'S PERFECT.");
            }
        }
        wait_vbl_done(); input_poll(); t++;
        arrow_at(0, 3 + sel * 2 + pg, t);
        if (joy_new & (J_LEFT | J_RIGHT)) {
            txt_put(0, 3 + sel * 2 + pg, " ", 0, 7);
            pg ^= 1; sel = 0; sfx_play(SFX_SELECT); redraw = 1; quip = 0xFF;
            continue;
        }
        if (joy_new & J_UP) { txt_put(0, 3 + sel * 2 + pg, " ", 0, 7); sel = sel ? sel - 1 : gn - 1; sfx_play(SFX_SELECT); redraw = 1; quip = 0xFF; }
        if (joy_new & J_DOWN) { txt_put(0, 3 + sel * 2 + pg, " ", 0, 7); sel = sel + 1 < gn ? sel + 1 : 0; sfx_play(SFX_SELECT); redraw = 1; quip = 0xFF; }
        if (joy_new & J_A) {
            r = g0 + sel; lv = pf_gear[profile][r];
            if (lv < gear_max[r] && pf_credits[profile] >= gear_cost[r][lv]) {
                pf_credits[profile] -= gear_cost[r][lv];
                pf_gear[profile][r]++;
                sfx_play(SFX_POWER);
                quip = t & 3;
            } else sfx_play(SFX_TICK);
            redraw = 1;
        }
        if (joy_new & (J_B | J_START)) { sfx_play(SFX_SELECT); oam_clear(); return; }
    }
}

