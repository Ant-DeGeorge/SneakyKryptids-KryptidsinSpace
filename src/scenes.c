// Title, pilot select, mission map, story cutscenes, results, ending,
// save data and the top-level game flow (bank 7).
#pragma bank 7
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "assets.h"
#include "cockpit_data.h"
#include "sound.h"
#include "game.h"
#include "stage_def.h"
#include "studio_card/studio_card.h"

#define BANK_A6 6
#define BANK_SC 19

extern const uint8_t * const intro_meta[];
extern const uint8_t * const outro_meta[];
extern const char * const * const intro_text[];
extern const char * const * const outro_text[];
extern const uint8_t intro_count[], outro_count[];
extern const char * const crew_names[];

// shared state
uint8_t gmode;
uint32_t score;
uint8_t lives, hearts, bombs, wtype, wlevel;
uint16_t hits_taken;
uint8_t continues_used;
uint16_t play_seconds;
uint8_t midboss_killed;
uint8_t start_cp;
uint8_t profile;
uint8_t pf_unlock[N_PROFILES], pf_done[N_PROFILES], pf_best[N_PROFILES][N_MISSIONS];
uint32_t pf_hi[N_PROFILES][N_MISSIONS];
uint8_t pf_out[N_PROFILES][N_MISSIONS];       // best bounty outcome per mission
uint16_t pf_credits[N_PROFILES];
uint8_t pf_gear[N_PROFILES][N_GEAR];
uint8_t g_gear[N_GEAR];
uint8_t bounty_out, mid_out;
uint32_t bounty_pts;
static const uint16_t pay_base[N_MISSIONS] = {100, 150, 200, 250, 300};
static const char * const out_names[4] = {"", "ESCAPED", "KILLED", "CAPTURED"};

static const char * const pilot_names[N_PROFILES] = {"CADET", "PILOT", "HUNTER"};
static const char * const boss_names[N_MISSIONS] = {"SPACE KRAKEN", "THUNDERBIRD", "JERSEY DEVIL", "WENDIGO", "FLATWOODS MONSTER"};
static const char * const mission_names[N_MISSIONS] = {"RIMEVEIL LANE", "STORMWALL NEBULA", "BARRENS BELT", "FROSTBITE MOON", "FLATWOODS DRIFT"};

static const uint16_t pal_text[4] = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(72, 224, 200), RGB8(248, 248, 248) };
static const uint16_t pal_panel[4] = { RGB8(12, 14, 24), RGB8(44, 52, 72), RGB8(232, 48, 72), RGB8(248, 248, 248) };
static const uint16_t pal_teal[4] = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(32, 120, 110), RGB8(72, 224, 200) };
static const uint16_t pal_star[4] = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(232, 96, 24), RGB8(248, 208, 56) };
// lock icon: the HUD lock glyph draws its body in colour 0, so give it a light colour 0
static const uint16_t pal_lock[4] = { RGB8(150, 160, 180), RGB8(6, 8, 24), RGB8(90, 100, 120), RGB8(6, 8, 24) };
static const uint16_t pal_dim[4] = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(40, 52, 70), RGB8(90, 100, 120) };

// ------------------------------------------------------------------ save (SRAM 0xA200)
// The demo keeps its own save block with its own signature ('S','K','D'), away from the
// family build's block at 0xA000 ('K','S'), so a cart that already holds family saves
// starts the demo clean and the family saves survive if that ROM is flashed back.
// FLATWOODS_DEMO: the standalone gun-cam demo (make flatwoods). Its own block and signature
// ('S','K','F' at 0xA400), so it never touches a Sneaky Kryptid save on the same cart.
#ifdef FLATWOODS_DEMO
#define SAVE ((uint8_t *)0xA400)
#define SAVE_SIG 'F'
#define SAVE_VER 2
#else
#define SAVE ((uint8_t *)0xA200)
#define SAVE_SIG 'D'
#define SAVE_VER 3
#endif
#ifdef FLATWOODS_DEMO
#define DEMO_VERSION "GUN CAM DEMO V1.0"
#elif defined(UNLOCK_ALL)
#define DEMO_VERSION "DEMO V1.1 UNLOCKED"
#else
#define DEMO_VERSION "DEMO V1.1"
#endif
#define PF_LEN(n, g) (2 + (n) + 4 * (n) + (n) + 2 + (g))
#define SAVE_LEN (4 + N_PROFILES * PF_LEN(N_MISSIONS, N_GEAR) + 1)
#define OLD_GEAR 5                                           // saves before the weapon runes held five gear slots
#define SAVE_LEN_OLD (4 + N_PROFILES * PF_LEN(N_MISSIONS, OLD_GEAR) + 1)
#define SAVE_LEN_V1 (4 + N_PROFILES * PF_LEN(4, OLD_GEAR) + 1)   // version 1: four missions
static uint8_t sum8(uint8_t *p, uint8_t n) { uint8_t s = 0x5A; while (n--) s += *p++; return s; }
static void save_clear(void) {
    memset(pf_unlock, 0, sizeof pf_unlock); memset(pf_done, 0, sizeof pf_done);
    memset(pf_best, 0, sizeof pf_best); memset(pf_hi, 0, sizeof pf_hi);
    memset(pf_out, 0, sizeof pf_out); memset(pf_credits, 0, sizeof pf_credits); memset(pf_gear, 0, sizeof pf_gear);
}
static void save_parse(uint8_t *q, uint8_t n, uint8_t g) {   // n = missions, g = gear slots stored per pilot
    uint8_t p;
    for (p = 0; p < N_PROFILES; p++) {
        pf_unlock[p] = *q++; pf_done[p] = *q++;
        memcpy(pf_best[p], q, n); q += n;
        memcpy(pf_hi[p], q, 4 * n); q += 4 * n;
        memcpy(pf_out[p], q, n); q += n;
        pf_credits[p] = q[0] | ((uint16_t)q[1] << 8); q += 2;
        memcpy(pf_gear[p], q, g); q += g;
    }
}
void save_read(void) {
    uint8_t buf[SAVE_LEN];
    ENABLE_RAM; SWITCH_RAM(0);
    memcpy(buf, SAVE, SAVE_LEN);
    DISABLE_RAM;
    save_clear();
    if (buf[0] != 'S' || buf[1] != 'K' || buf[2] != SAVE_SIG) return;
    if (buf[3] == SAVE_VER && buf[SAVE_LEN - 1] == sum8(buf, SAVE_LEN - 1)) save_parse(buf + 4, N_MISSIONS, N_GEAR);
    else if (buf[3] == SAVE_VER - 1 && buf[SAVE_LEN_OLD - 1] == sum8(buf, SAVE_LEN_OLD - 1)) {
        save_parse(buf + 4, N_MISSIONS, OLD_GEAR);           // save from before the weapon runes: keep it, runes start unbought
        save_write();
    } else if (SAVE_SIG == 'D' && buf[3] == 1 && buf[SAVE_LEN_V1 - 1] == sum8(buf, SAVE_LEN_V1 - 1)) {
        save_parse(buf + 4, 4, OLD_GEAR);                    // v1.0 save: keep it, mission 5 starts empty
        save_write();
    }
}
void save_write(void) {
    uint8_t buf[SAVE_LEN], p, *q;
    buf[0] = 'S'; buf[1] = 'K'; buf[2] = SAVE_SIG; buf[3] = SAVE_VER;
    q = buf + 4;
    for (p = 0; p < N_PROFILES; p++) {
        *q++ = pf_unlock[p]; *q++ = pf_done[p];
        memcpy(q, pf_best[p], N_MISSIONS); q += N_MISSIONS;
        memcpy(q, pf_hi[p], 4 * N_MISSIONS); q += 4 * N_MISSIONS;
        memcpy(q, pf_out[p], N_MISSIONS); q += N_MISSIONS;
        q[0] = (uint8_t)pf_credits[p]; q[1] = pf_credits[p] >> 8; q += 2;
        memcpy(q, pf_gear[p], N_GEAR); q += N_GEAR;
    }
    buf[SAVE_LEN - 1] = sum8(buf, SAVE_LEN - 1);
    ENABLE_RAM; SWITCH_RAM(0);
    memcpy(SAVE, buf, SAVE_LEN);
    DISABLE_RAM;
}

// cadet: 1-5 stars from hits; hunter: 1 (D) .. 5 (S) from score
uint8_t calc_rank(void) {
    uint8_t r;
    if ((gmode != MODE_HUNTER)) {
        r = hits_taken <= 2 ? 5 : hits_taken <= 5 ? 4 : hits_taken <= 9 ? 3 : hits_taken <= 14 ? 2 : 1;
        if (continues_used && r > 1) r--;
        return r;
    }
    if (continues_used) return score >= 40000UL ? 2 : 1;
    if (score >= 78000UL) return 5;
    if (score >= 68000UL) return 4;
    if (score >= 55000UL) return 3;
    if (score >= 40000UL) return 2;
    return 1;
}

// ------------------------------------------------------------------ helpers
static void center(uint8_t y, const char *s, uint8_t attr) {
    uint8_t n = (uint8_t)strlen(s);
    txt_put((20 - n) >> 1, y, s, 0, attr);
}
static void put_num(uint8_t x, uint8_t y, uint32_t v, uint8_t digits, uint8_t attr) {
    static const uint32_t p10[7] = {1000000UL, 100000UL, 10000UL, 1000UL, 100UL, 10UL, 1UL};
    uint8_t i, d;
    char c[2] = {0, 0};
    for (i = 7 - digits; i < 7; i++) {
        d = 0;
        while (v >= p10[i]) { v -= p10[i]; d++; }
        c[0] = '0' + d;
        txt_put(x++, y, c, 0, attr);
    }
}
static void put_tile(uint8_t x, uint8_t y, uint8_t t, uint8_t attr) {
    set_bkg_tile_xy(x, y, t); VBK_REG = 1; set_bkg_tile_xy(x, y, attr); VBK_REG = 0;
}
static void screen_prep(void) {
    screen_off();
    splits_off();
    oam_clear();
    n_scx = n_scy = 0;
    VBK_REG = 0;
    clear_bkg(0, 7);
    ld_bkg_data(BANK_A2, 0, 0, N_COMMON, common_tiles);
    ld_spr_data(BANK_A2, 0, N_SPR_TILES, spr_tiles);
    ld_spr_pal(BANK_A2, 0, 8, obj_pals);
    set_bkg_palette(7, 1, pal_text);
}
static uint8_t wait_button(uint16_t max) {
    while (max--) {
        wait_vbl_done(); input_poll();
        if (joy_new & (J_A | J_START | J_B)) return joy_new;
    }
    return 0;
}
static void arrow_at(uint8_t col, uint8_t row, uint8_t t) {
    oam_fixed(0, col * 8 + 8 - 2 + ((t >> 3) & 1), row * 8 + 16, ST_ARROW, 7);
}
static void put_rating(uint8_t x, uint8_t y, uint8_t prof, uint8_t sid) {
    uint8_t r = pf_best[prof][sid], i;
    if (prof < 2) {
        for (i = 0; i < 5; i++) put_tile(x + i, y, i < r ? T_HUD_STAR : T_HUD_STARE, 5);
    } else {
        static const char rk[] = "-DCBAS";
        char c[2]; c[0] = rk[r]; c[1] = 0;
        txt_put(x + 2, y, c, 0, 6);
    }
}

// ------------------------------------------------------------------ title
static uint16_t fade_buf[4 * 8];
static uint8_t title_seen;
// scale palettes to level lv (0 black .. 8 full) and load them
static void pal_fade(uint8_t first, uint8_t n, const uint16_t *src, uint8_t lv) {
    uint8_t i, r, g, b;
    for (i = 0; i < n * 4; i++) {
        uint16_t c = src[i];
        r = ((c & 31) * lv) >> 3; g = (((c >> 5) & 31) * lv) >> 3; b = (((c >> 10) & 31) * lv) >> 3;
        fade_buf[i] = r | ((uint16_t)g << 5) | ((uint16_t)b << 10);
    }
    wait_vbl_done();
    set_bkg_palette(first, n, fade_buf);
}
static uint8_t title_wait(uint8_t n) {        // A/START skips the intro
    while (n--) { wait_vbl_done(); input_poll(); if (joy_new & (J_A | J_START)) return 1; }
    return 0;
}
static void title_screen(void) {
    uint8_t t = 0, buf[40], lv, skip = 0;
    static uint16_t art_pals[4 * 7];
    music_play(MUS_TITLE);
    if (!title_seen) {
        // AGD PRESENTS on its own, fading up out of black
        title_seen = 1;
        screen_prep();
        txt_fill(0, 0, 20, 18, txt_tile(' '), 7, 0);
        pal_fade(7, 1, pal_text, 0);
        center(8, "AGD PRESENTS", 7);
        screen_on(LCDC_SCREEN);
        for (lv = 1; lv <= 8 && !skip; lv++) skip = title_wait(6);
        pal_fade(7, 1, pal_text, 8);
        if (!skip) skip = title_wait(80);
        for (lv = 8; lv && !skip; lv--) { pal_fade(7, 1, pal_text, lv - 1); skip = title_wait(3); }
    }
    screen_prep();
    ld_bkg_data(BANK_A6, 0, N_COMMON, TITLE_ART_NT, title_art_tiles);
    ld_copy(BANK_A6, art_pals, title_art_pals, 6 * 8);
    ld_copy(BANK_A6, art_pals + 24, logo_pal, 8);
    pal_fade(0, 7, art_pals, skip ? 8 : 0);
    ld_image(BANK_A6, 0, 2, 20, 10, title_art_map, title_art_attr, N_COMMON, 0, 0);
    ld_bkg_data(BANK_A6, 1, 0, LOGO_NT, logo_tiles);
    ld_copy(BANK_A6, buf, logo_map, LOGO_W * 2);
    set_bkg_tiles(0, 12, LOGO_W, 2, buf);
    VBK_REG = 1; fill_bkg_rect(0, 12, 20, 2, 6 | 0x08); VBK_REG = 0;
    txt_fill(0, 0, 20, 2, txt_tile(' '), 7, 0);
    txt_fill(0, 14, 20, 4, txt_tile(' '), 7, 0);
    center(0, "AGD PRESENTS", 7);
    screen_on(LCDC_SCREEN);
    // the art and logo fade in under AGD PRESENTS
    for (lv = 1; lv <= 8 && !skip; lv++) { pal_fade(0, 7, art_pals, lv); skip = title_wait(6); }
    pal_fade(0, 7, art_pals, 8);
#ifdef FLATWOODS_DEMO
    center(14, "FLATWOODS DRIFT", 7);
#else
    center(14, "KRYPTIDS IN SPACE", 7);
#endif
    center(17, DEMO_VERSION, 7);
    if (skip) title_wait(15);                  // don't let the skip press also start the game
    while (1) {
        wait_vbl_done(); input_poll();
        t++;
        if ((t & 31) == 0) center(16, "PRESS START", 7);
        if ((t & 31) == 20) txt_fill(4, 16, 12, 1, txt_tile(' '), 7, 0);
        if ((joy & J_SELECT) && (joy_new & J_B)) {          // SELECT + B: erase all save slots
            txt_fill(0, 16, 20, 2, txt_tile(' '), 7, 0);
            center(16, "ERASE ALL SAVES?", 7);
            center(17, "A: YES   B: NO", 7);
            sfx_play(SFX_ALARM);
            while (1) {
                wait_vbl_done(); input_poll();
                if (joy_new & J_A) { save_clear(); save_write(); sfx_play(SFX_BOMB); txt_fill(0, 16, 20, 2, txt_tile(' '), 7, 0); center(16, "SAVES ERASED", 7); wait_frames(90); break; }
                if (joy_new & J_B) { sfx_play(SFX_SELECT); break; }
            }
            txt_fill(0, 16, 20, 2, txt_tile(' '), 7, 0);
            center(17, DEMO_VERSION, 7);
            t = 31;
            continue;
        }
        if (joy_new & (J_A | J_START)) { sfx_play(SFX_START); wait_frames(20); return; }
    }
}

// ------------------------------------------------------------------ pilot select
static uint8_t total_stars(uint8_t p) {
    uint8_t s = 0, i;
    for (i = 0; i < N_MISSIONS; i++) s += pf_best[p][i];
    return s;
}
static uint8_t pilot_select(void) {
    uint8_t sel = profile, t = 0, p, y;
    screen_prep();
    set_bkg_palette(6, 1, pal_teal);
    set_bkg_palette(5, 1, pal_star);
    txt_fill(0, 0, 20, 18, txt_tile(' '), 7, 0);
    center(1, "WHO'S FLYING?", 7);
    for (p = 0; p < N_PROFILES; p++) {
        y = 4 + p * 4;
        txt_put(3, y, pilot_names[p], 0, 7);
        txt_put(12, y, p == 0 ? "EASY" : p == 1 ? "MEDIUM" : "HARD", 0, 6);
#ifdef FLATWOODS_DEMO
        if (p < 2) {
            uint8_t i;
            for (i = 0; i < 5; i++) put_tile(3 + i, y + 1, i < pf_best[p][N_MISSIONS - 1] ? T_HUD_STAR : T_HUD_STARE, 5);
        } else {
            txt_put(3, y + 1, "HI", 0, 6);
            put_num(6, y + 1, pf_hi[2][N_MISSIONS - 1], 7, 7);
        }
        if (pf_out[p][N_MISSIONS - 1]) txt_put(10, y + 1, out_names[pf_out[p][N_MISSIONS - 1]], 0, 6);   // best outcome
        continue;
#endif
        if (p < 2) {
            put_tile(3, y + 1, T_HUD_STAR, 5);
            put_num(5, y + 1, total_stars(p), 2, 7);
            txt_put(7, y + 1, "/25", 0, 7);
        } else {
            txt_put(3, y + 1, "HI", 0, 6);
            put_num(6, y + 1, pf_hi[2][0] + pf_hi[2][1] + pf_hi[2][2] + pf_hi[2][3] + pf_hi[2][4], 7, 7);
        }
    }
#ifdef FLATWOODS_DEMO
    center(15, "A: GO   B: BACK", 6);
    center(17, "SELECT+A: BOSS", 6);
#else
    center(16, "A: GO   B: BACK", 6);
#endif
    screen_on(LCDC_SCREEN);
    while (1) {
        wait_vbl_done(); input_poll(); t++;
        if (joy_new & J_UP) { sel = sel ? sel - 1 : N_PROFILES - 1; sfx_play(SFX_SELECT); }
        if (joy_new & J_DOWN) { sel = sel + 1 < N_PROFILES ? sel + 1 : 0; sfx_play(SFX_SELECT); }
        arrow_at(1, 4 + sel * 4, t);
        if (joy_new & (J_A | J_START)) {
            sfx_play(SFX_START);
            profile = sel;
            gmode = sel;
#ifdef FLATWOODS_DEMO
            start_cp = (joy & J_SELECT) ? 2 : 0;
            wait_frames(20);
#endif
            return 1;
        }
        if (joy_new & J_B) return 0;
    }
}

// ------------------------------------------------------------------ mission map
static uint8_t mission_map(void) {
    uint8_t sel, t = 0, s, y, top = pf_unlock[profile];
    if (top >= N_MISSIONS) top = N_MISSIONS - 1;
#ifdef UNLOCK_ALL
    top = N_MISSIONS - 1;                 // test build (make unlocked): every mission selectable
#endif
    sel = top;
    screen_prep();
    set_bkg_palette(6, 1, pal_teal);
    set_bkg_palette(5, 1, pal_star);
    set_bkg_palette(4, 1, pal_dim);
    set_bkg_palette(3, 1, pal_lock);
    txt_fill(0, 0, 20, 18, txt_tile(' '), 7, 0);
    center(0, "MISSION SELECT", 7);
    txt_put(1, 1, pilot_names[profile], 0, 7);
    txt_put(15, 1, "$", 0, 6); put_num(16, 1, pf_credits[profile], 4, 7);
    for (s = 0; s < N_MISSIONS; s++) {
        y = 3 + s * 2;
        if (s <= top) {
            char c[2]; c[0] = '1' + s; c[1] = 0;
            txt_put(2, y, c, 0, 6);
            txt_put(4, y, mission_names[s], 0, 7);
            put_rating(4, y + 1, profile, s);
            if (pf_hi[profile][s]) put_num(12, y + 1, pf_hi[profile][s], 7, 4);
            if (pf_out[profile][s]) { char c[2]; c[0] = " EKC"[pf_out[profile][s]]; c[1] = 0; txt_put(10, y + 1, c, 0, pf_out[profile][s] == OUT_CAPTURE ? 5 : 6); }
        } else {
            put_tile(2, y, T_HUD_LOCK, 3);            // light lock icon (pal_lock)
            txt_put(4, y, mission_names[s], 0, 4);            // greyed out until unlocked
        }
    }
    center(14, "BOUNTY:", 6);
    center(17, "A:GO  START:ARMORY", 6);
    screen_on(LCDC_SCREEN);
    music_play(MUS_MAP);
    s = 0xFF;
    while (1) {
        wait_vbl_done(); input_poll(); t++;
        if ((joy_new & J_UP) && sel) { sel--; sfx_play(SFX_SELECT); }
        if ((joy_new & J_DOWN) && sel < top) { sel++; sfx_play(SFX_SELECT); }
        if (s != sel) {
            s = sel;
            txt_fill(0, 15, 20, 1, txt_tile(' '), 7, 0);
            center(15, boss_names[sel], 7);
        }
        // the Sneaky Kryptid parks next to the chosen mission
        oam_fixed(1, 0 * 8 + 8, (3 + sel * 2) * 8 + 16 - 4 + ((t >> 4) & 1), ST_SHIP_TOP, 0);
        oam_fixed(2, 1 * 8 + 8, (3 + sel * 2) * 8 + 16 - 4 + ((t >> 4) & 1), ST_SHIP_TOP + 2, 0);
        if (joy_new & J_START) { sfx_play(SFX_SELECT); return 0xFE; }
        if (joy_new & J_A) {
            sfx_play(SFX_START);
            start_cp = (joy & J_SELECT) ? 2 : 0;
            wait_frames(20);
            return sel;
        }
        if (joy_new & J_B) return 0xFF;
    }
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
static void armory(void) {
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
                save_write();
                sfx_play(SFX_POWER);
                quip = t & 3;
            } else sfx_play(SFX_TICK);
            redraw = 1;
        }
        if (joy_new & (J_B | J_START)) { sfx_play(SFX_SELECT); oam_clear(); return; }
    }
}

// ------------------------------------------------------------------ cutscenes
extern const uint8_t por_cap_a_tiles[], por_cap_b_tiles[], por_static_a_tiles[], por_static_b_tiles[], por_forge_a_tiles[], por_forge_b_tiles[],
    por_doc_a_tiles[], por_doc_b_tiles[], por_vex_a_tiles[], por_vex_b_tiles[];
extern const uint16_t por_cap_a_pals[], por_cap_b_pals[], por_static_a_pals[], por_static_b_pals[], por_forge_a_pals[], por_forge_b_pals[],
    por_doc_a_pals[], por_doc_b_pals[], por_vex_a_pals[], por_vex_b_pals[];
extern const uint8_t por_cap_a_map[], por_cap_b_map[], por_static_a_map[], por_static_b_map[], por_forge_a_map[], por_forge_b_map[],
    por_doc_a_map[], por_doc_b_map[], por_vex_a_map[], por_vex_b_map[];
extern const uint8_t por_cap_a_attr[], por_cap_b_attr[], por_static_a_attr[], por_static_b_attr[], por_forge_a_attr[], por_forge_b_attr[],
    por_doc_a_attr[], por_doc_b_attr[], por_vex_a_attr[], por_vex_b_attr[];
static const uint8_t * const por_tiles[10] = {por_cap_a_tiles, por_cap_b_tiles, por_static_a_tiles, por_static_b_tiles, por_forge_a_tiles, por_forge_b_tiles, por_doc_a_tiles, por_doc_b_tiles, por_vex_a_tiles, por_vex_b_tiles};
static const uint16_t * const por_pals[10] = {por_cap_a_pals, por_cap_b_pals, por_static_a_pals, por_static_b_pals, por_forge_a_pals, por_forge_b_pals, por_doc_a_pals, por_doc_b_pals, por_vex_a_pals, por_vex_b_pals};
static const uint8_t * const por_map[10] = {por_cap_a_map, por_cap_b_map, por_static_a_map, por_static_b_map, por_forge_a_map, por_forge_b_map, por_doc_a_map, por_doc_b_map, por_vex_a_map, por_vex_b_map};
static const uint8_t * const por_attr[10] = {por_cap_a_attr, por_cap_b_attr, por_static_a_attr, por_static_b_attr, por_forge_a_attr, por_forge_b_attr, por_doc_a_attr, por_doc_b_attr, por_vex_a_attr, por_vex_b_attr};
static const uint8_t por_nt[10] = {POR_CAP_A_NT, POR_CAP_B_NT, POR_STATIC_A_NT, POR_STATIC_B_NT, POR_FORGE_A_NT, POR_FORGE_B_NT, POR_DOC_A_NT, POR_DOC_B_NT, POR_VEX_A_NT, POR_VEX_B_NT};

typedef struct { uint8_t bank, nt; const uint8_t *tiles; const uint16_t *pals; const uint8_t *map, *attr; } Scene;
static const Scene scenes[10] = {
    {BANK_A6, SCENE_SHIP_NT, scene_ship_tiles, scene_ship_pals, scene_ship_map, scene_ship_attr},
    {BANK_A6, SCENE_STATION_NT, scene_station_tiles, scene_station_pals, scene_station_map, scene_station_attr},
    {BANK_A6, SCENE_KRAKEN_NT, scene_kraken_tiles, scene_kraken_pals, scene_kraken_map, scene_kraken_attr},
    {BANK_SC, SCENE_TBIRD_NT, scene_tbird_tiles, scene_tbird_pals, scene_tbird_map, scene_tbird_attr},
    {BANK_SC, SCENE_JDEVIL_NT, scene_jdevil_tiles, scene_jdevil_pals, scene_jdevil_map, scene_jdevil_attr},
    {BANK_SC, SCENE_STORM_NT, scene_storm_tiles, scene_storm_pals, scene_storm_map, scene_storm_attr},
    {BANK_SC, SCENE_BARRENS_NT, scene_barrens_tiles, scene_barrens_pals, scene_barrens_map, scene_barrens_attr},
    {24, SCENE_WENDIGO_NT, scene_wendigo_tiles, scene_wendigo_pals, scene_wendigo_map, scene_wendigo_attr},
    {24, SCENE_FROST_NT, scene_frost_tiles, scene_frost_pals, scene_frost_map, scene_frost_attr},
    {BANK_CK2, SCENE_FLAT_NT, scene_flat_tiles, scene_flat_pals, scene_flat_map, scene_flat_attr},
};
static void load_scene(uint8_t sc) {
    const Scene *s = &scenes[sc];
    ld_bkg_data(s->bank, 0, N_COMMON, s->nt, s->tiles);
    ld_bkg_pal(s->bank, 0, 3, s->pals);
    ld_image(s->bank, 0, 0, 20, 9, s->map, s->attr, N_COMMON, 0, 0);
}

static void cutscene(const uint8_t *meta, const char * const *text, uint8_t count) {
    uint8_t i, cur_scene = 0xFF, k, x, y, b, skip = 0, n;
    screen_prep();
    set_bkg_palette(7, 1, pal_panel);
    txt_fill(0, 9, 20, 9, txt_tile(' '), 7, 0);
    fill_bkg_rect(0, 9, 20, 1, T_HUD_FRAMET);
    music_play(MUS_BRIEF);
    for (i = 0; i < count && !skip; i++) {
        uint8_t sc = meta[i * 2], por = meta[i * 2 + 1];
        const char *s = text[i];
        if (sc != cur_scene) {
            if (cur_scene != 0xFF) screen_off();
            load_scene(sc); cur_scene = sc;
        }
        ld_bkg_data(BANK_A3, 1, 0, por_nt[por], por_tiles[por]);
        ld_bkg_pal(BANK_A3, 3, 4, por_pals[por]);
        ld_image(BANK_A3, 1, 11, 6, 6, por_map[por], por_attr[por], 0, 0x08, 0);
        txt_fill(8, 10, 12, 7, txt_tile(' '), 7, 0);
        txt_fill(1, 10, 7, 1, txt_tile(' '), 7, 0);
        txt_put(1, 10, crew_names[por >> 1], 0, 7);
        screen_on(LCDC_SCREEN);
        x = 0; y = 0; n = 0;
        for (k = 0; s[k]; k++) {
            if (s[k] == '\n') { x = 0; y++; continue; }
            { char c[2]; c[0] = s[k]; c[1] = 0; txt_put(8 + x, 11 + y, c, 0, 7); }
            x++;
            if (!(n++ & 3)) sfx_play(SFX_BLIP);
            wait_vbl_done(); input_poll();
            b = joy_new;
            if (b & (J_START | J_B)) { skip = 1; break; }
            if (!(b & J_A)) { wait_vbl_done(); }
            else {
                for (k++; s[k]; k++) {
                    if (s[k] == '\n') { x = 0; y++; continue; }
                    { char c[2]; c[0] = s[k]; c[1] = 0; txt_put(8 + x, 11 + y, c, 0, 7); }
                    x++;
                }
                break;
            }
        }
        if (skip) break;
        {
            uint8_t t = 0;
            while (1) {
                wait_vbl_done(); input_poll(); t++;
                oam_fixed(0, 19 * 8 + 8 - 2, 16 * 8 + 16 + 4, ST_ARROW, 7);
                shadow_OAM[0].y = (t & 16) ? 0 : 16 * 8 + 16 + 4;
                if (joy_new & J_A) { sfx_play(SFX_SELECT); break; }
                if (joy_new & (J_START | J_B)) { skip = 1; break; }
            }
            shadow_OAM[0].y = 0;
        }
    }
}

// ------------------------------------------------------------------ results
static void results(uint8_t sid) {
    uint8_t r, i, newhi = 0, newbest = 0;
    uint16_t pay, bonus;
    screen_prep();
    set_bkg_palette(6, 1, pal_teal);
    set_bkg_palette(5, 1, pal_star);
    txt_fill(0, 0, 20, 18, txt_tile(' '), 7, 0);
    r = calc_rank();
    if (score > pf_hi[profile][sid]) { pf_hi[profile][sid] = score; newhi = 1; }
    if (r > pf_best[profile][sid]) { pf_best[profile][sid] = r; newbest = 1; }
    if (bounty_out > pf_out[profile][sid]) pf_out[profile][sid] = bounty_out;
    pf_done[profile] |= 1 << sid;
    if (pf_unlock[profile] < sid + 1) pf_unlock[profile] = sid + 1;
    pay = pay_base[sid] * (bounty_out ? bounty_out : 1);
    bonus = (mid_out == OUT_CAPTURE ? 50 : mid_out == OUT_KILL ? 25 : 0) + (hits_taken == 0 ? 50 : 0);
    pf_credits[profile] = pf_credits[profile] + pay + bonus > 9999 ? 9999 : pf_credits[profile] + pay + bonus;
    save_write();
#ifdef FLATWOODS_DEMO
    center(0, "MISSION COMPLETE", 7);
#else
    { char c[20]; strcpy(c, "MISSION 1 COMPLETE"); c[8] = '1' + sid; center(0, c, 7); }
#endif
    center(1, mission_names[sid], 6);
    { const char *nm = sid == N_MISSIONS - 1 ? "FLATWOODS" : boss_names[sid];
      txt_put(1, 3, "TARGET", 0, 6); txt_put(19 - strlen(nm), 3, nm, 0, 7); }
    txt_put(1, 4, "STATUS", 0, 6); txt_put(19 - strlen(out_names[bounty_out]), 4, out_names[bounty_out], 0, bounty_out == OUT_CAPTURE ? 6 : 7);
    txt_put(1, 6, "SCORE", 0, 7); put_num(12, 6, score, 7, 7);
    txt_put(1, 7, "HITS", 0, 7); put_num(16, 7, hits_taken, 3, 7);
    txt_put(1, 8, "TIME", 0, 7);
    put_num(14, 8, play_seconds / 60, 2, 7);
    txt_put(16, 8, ":", 0, 7);
    put_num(17, 8, play_seconds % 60, 2, 7);
    txt_put(1, 10, "CONTRACT PAY", 0, 6); txt_put(14, 10, "$", 0, 6); put_num(15, 10, pay, 4, 7);
    txt_put(1, 11, "BONUSES", 0, 6); txt_put(14, 11, "$", 0, 6); put_num(15, 11, bonus, 4, 7);
    txt_put(1, 12, "CREDITS", 0, 7); txt_put(14, 12, "$", 0, 6); put_num(15, 12, pf_credits[profile], 4, 7);
    if ((gmode != MODE_HUNTER)) {
        for (i = 0; i < 5; i++) put_tile(5 + i * 2, 14, i < r ? T_HUD_STAR : T_HUD_STARE, 5);
    } else {
        static const char rk[] = "DCBAS";
        char c[2]; c[0] = rk[r - 1]; c[1] = 0;
        txt_put(1, 14, "RANK", 0, 7);
        txt_put(18, 14, c, 0, 7);
    }
    if (newhi) center(16, "NEW HIGH SCORE!", 6);
    else if (newbest) center(16, "NEW BEST!", 6);
#ifdef FLATWOODS_DEMO
    center(17, "FULL GAME: GITHUB", 7);
#else
    center(17, "SPEND IT AT FORGE'S", 7);
#endif
    screen_on(LCDC_SCREEN);
    music_play(MUS_CLEAR);
    wait_frames(60);
    wait_button(1800);
}

static void ending(void) {
    ending_run();
    screen_prep();
    set_bkg_palette(6, 1, pal_teal);
    txt_fill(0, 0, 20, 18, txt_tile(' '), 7, 0);
    center(3, "KRYPTIDS IN SPACE", 6);
    center(5, "THE SNEAKY KRYPTID", 7);
    center(8, "A GAME BY", 7);
    center(10, "ANTHONY DEGEORGE", 6);
    center(13, "REJECTED BY", 7);
    center(14, "THE MACHINES", 7);
    screen_on(LCDC_SCREEN);
    wait_frames(60);
    wait_button(1200);
}

static void game_over(uint8_t sid) {
    screen_prep();
    set_bkg_palette(6, 1, pal_teal);
    txt_fill(0, 0, 20, 18, txt_tile(' '), 7, 0);
    center(6, "GAME OVER", 7);
    txt_put(3, 9, "SCORE", 0, 7); put_num(10, 9, score, 7, 7);
    if (score > pf_hi[profile][sid]) { pf_hi[profile][sid] = score; save_write(); center(12, "NEW HIGH SCORE!", 6); }
    screen_on(LCDC_SCREEN);
    music_play(MUS_OVER);
    wait_frames(60);
    wait_button(600);
}

// ------------------------------------------------------------------ top level
void game_main(void) BANKED {
    uint8_t res, sid;
    // Switch to double speed once, with the LCD and APU off, before the studio card:
    // the card then stays in double speed instead of switching back and forth.
    DISPLAY_OFF;
    NR52_REG = 0x00;
    cpu_fast();
    studio_card_play();                 // Rejected By The Machines studio card, before anything else
    DISPLAY_OFF;
    LCDC_REG = LCDC_SCREEN & 0x7F;
    n_lcdc = LCDC_REG;
    sound_init();
    isr_install();
    SPRITES_8x16;
    save_read();
    profile = 0;
    while (1) {
        title_screen();
#ifdef TEST_ENDING
        ending();                       // test ROM only: poke pf_out/pf_best/profile/gmode at the title
        continue;
#endif
#ifdef FLATWOODS_DEMO
        // standalone gun-cam demo: pilot select -> briefing -> Flatwoods Drift -> debrief, and again
        while (pilot_select()) {
            sid = N_MISSIONS - 1;
            if (!start_cp) cutscene(intro_meta[sid], intro_text[sid], intro_count[sid]);
            res = cockpit_run();
            if (res == RES_CLEAR) {
                { uint8_t o = (bounty_out ? bounty_out : 1) - 1;
                  cutscene(outro_meta[sid * 3 + o], outro_text[sid * 3 + o], outro_count[sid * 3 + o]); }
                results(sid);
            } else game_over(sid);
        }
        continue;
#endif
        while (pilot_select()) {
            while ((sid = mission_map()) != 0xFF) {
                if (sid == 0xFE) { armory(); continue; }
                memcpy(g_gear, pf_gear[profile], N_GEAR);
                if (!start_cp) cutscene(intro_meta[sid], intro_text[sid], intro_count[sid]);
                res = sid == N_MISSIONS - 1 ? cockpit_run() : stage_run(sid);
                if (res == RES_CLEAR) {
                    { uint8_t o = (bounty_out ? bounty_out : 1) - 1;
                      cutscene(outro_meta[sid * 3 + o], outro_text[sid * 3 + o], outro_count[sid * 3 + o]); }
                    results(sid);
                    if (sid == N_MISSIONS - 1) ending();
                } else game_over(sid);
            }
        }
    }
}
