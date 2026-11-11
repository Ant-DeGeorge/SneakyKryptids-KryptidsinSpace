// Ending, after the last mission: a clean tally screen, the crew roasting your flying
// (rank + bounty outcomes decide the lines), then the watcher: glowing eyes of the kin of
// a boss you sealed in the hold, promising your time will come. Autobanked.
#pragma bank 255
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "assets.h"
#include "sound.h"
#include "game.h"
#include "stage_def.h"
#include "ending_eyes.h"

#define BANK_A6 6
#define N_BOUNTY 5                        // missions with a bounty to capture (the escape run has none)

extern uint8_t pf_best[N_PROFILES][N_MISSIONS];
extern uint32_t pf_hi[N_PROFILES][N_MISSIONS];
extern uint8_t pf_out[N_PROFILES][N_MISSIONS];
extern const uint8_t por_cap_a_tiles[], por_cap_b_tiles[], por_static_a_tiles[], por_static_b_tiles[], por_forge_a_tiles[], por_forge_b_tiles[],
    por_doc_a_tiles[], por_doc_b_tiles[], por_vex_a_tiles[], por_vex_b_tiles[];
extern const uint16_t por_cap_a_pals[], por_cap_b_pals[], por_static_a_pals[], por_static_b_pals[], por_forge_a_pals[], por_forge_b_pals[],
    por_doc_a_pals[], por_doc_b_pals[], por_vex_a_pals[], por_vex_b_pals[];
extern const uint8_t por_cap_a_map[], por_cap_b_map[], por_static_a_map[], por_static_b_map[], por_forge_a_map[], por_forge_b_map[],
    por_doc_a_map[], por_doc_b_map[], por_vex_a_map[], por_vex_b_map[];
extern const uint8_t por_cap_a_attr[], por_cap_b_attr[], por_static_a_attr[], por_static_b_attr[], por_forge_a_attr[], por_forge_b_attr[],
    por_doc_a_attr[], por_doc_b_attr[], por_vex_a_attr[], por_vex_b_attr[];

static const uint8_t * const e_por_tiles[10] = {por_cap_a_tiles, por_cap_b_tiles, por_static_a_tiles, por_static_b_tiles, por_forge_a_tiles, por_forge_b_tiles, por_doc_a_tiles, por_doc_b_tiles, por_vex_a_tiles, por_vex_b_tiles};
static const uint16_t * const e_por_pals[10] = {por_cap_a_pals, por_cap_b_pals, por_static_a_pals, por_static_b_pals, por_forge_a_pals, por_forge_b_pals, por_doc_a_pals, por_doc_b_pals, por_vex_a_pals, por_vex_b_pals};
static const uint8_t * const e_por_map[10] = {por_cap_a_map, por_cap_b_map, por_static_a_map, por_static_b_map, por_forge_a_map, por_forge_b_map, por_doc_a_map, por_doc_b_map, por_vex_a_map, por_vex_b_map};
static const uint8_t * const e_por_attr[10] = {por_cap_a_attr, por_cap_b_attr, por_static_a_attr, por_static_b_attr, por_forge_a_attr, por_forge_b_attr, por_doc_a_attr, por_doc_b_attr, por_vex_a_attr, por_vex_b_attr};
static const uint8_t e_por_nt[10] = {POR_CAP_A_NT, POR_CAP_B_NT, POR_STATIC_A_NT, POR_STATIC_B_NT, POR_FORGE_A_NT, POR_FORGE_B_NT, POR_DOC_A_NT, POR_DOC_B_NT, POR_VEX_A_NT, POR_VEX_B_NT};
static const char * const e_crew[5] = {"YETI", "STATIC", "FORGE", "SUTURE", "ZANNAH"};
#define P_YETI 0
#define P_STATIC 2
#define P_FORGE 4
#define P_ZANNAH 8

static const uint16_t e_pal_text[4]  = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(72, 224, 200), RGB8(248, 248, 248) };
static const uint16_t e_pal_panel[4] = { RGB8(12, 14, 24), RGB8(44, 52, 72), RGB8(232, 48, 72), RGB8(248, 248, 248) };
static const uint16_t e_pal_teal[4]  = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(32, 120, 110), RGB8(72, 224, 200) };
static const uint16_t e_pal_star[4]  = { RGB8(0, 0, 8), RGB8(6, 8, 24), RGB8(232, 96, 24), RGB8(248, 208, 56) };
static const uint16_t e_pal_sky[4]   = { RGB8(0, 0, 0), RGB8(40, 44, 70), RGB8(90, 96, 130), RGB8(190, 196, 230) };
static const uint16_t e_pal_dim[4]   = { RGB8(0, 0, 0), RGB8(0, 0, 0), RGB8(60, 64, 90), RGB8(120, 126, 160) };

// ------------------------------------------------------------------ crew banter (12 cols x 6 rows)
// tier: 3 top flying .. 0 rough (average best rating over all missions)
static const char * const yeti_line[4] = {
    "WHAT THE\n$&!? WAS\nTHAT FLYING?\nSTATIC, IS\nTHIS ON YOU?",
    "WE LIVED.\nTHAT'S WHAT\nI'M PUTTING\nON THE\nREPORT.",
    "SOLID WORK.\nTHE HULL\nONLY HAS A\nFEW NEW\nHOLES.",
    "NOT BAD.\nI'D HIRE YOU\nAGAIN. DON'T\nLET IT GO TO\nYOUR HEAD.",
};
static const char * const static_line[4] = {
    "DON'T LOOK\nAT ME. I WAS\nSCREAMING\nTHE WHOLE\nTIME.",
    "NEXT TIME,\nOPEN YOUR\n$&!? EYES\nWHEN YOU\nFLY.",
    "I COUNTED\nTHE NEAR\nMISSES. I\nRAN OUT OF\nFINGERS.",
    "I TAUGHT\nTHEM ALL OF\nTHAT. YOU'RE\nWELCOME.",
};
// outcome: 0 all captured, 1 mixed, 2 mostly killed, 3 one or more escaped
static const char * const zannah_line[4] = {
    "ALL OF THEM\nIN THE HOLD.\nFULL PAY.\nI'D KISS YOU\n...I WON'T.",
    "SOME CAGED,\nSOME DEAD.\nCLIENTS PAY\nDOUBLE FOR\nBREATHING.",
    "DEAD ONES\nPAY HALF,\nGENIUS. DO\nYOU HATE\n$&!? MONEY?",
    "ONE GOT\nAWAY. THAT'S\nMY $&!?\nMONEY OUT\nTHERE.",
};
static const char * const forge_line[2] = {
    "MY RUNES ARE\nHUMMING.\nSOMETHING'S\nCALLING TO\nWHAT'S IN\nTHE HOLD...",
    "MY RUNES ARE\nHUMMING.\nSOMETHING'S\nOUT THERE.\nWATCHING\nUS...",
};

// ------------------------------------------------------------------ watcher: kin of the captured
// index = boss sid of the latest capture (0 Kraken .. 4 Flatwoods), 5 = nobody captured
static const uint8_t kin_shape[6] = {0, 1, 2, 3, 4, 2};
static const uint16_t kin_pal[6][4] = {
    { RGB8(0, 0, 0), RGB8(96, 16, 72), RGB8(224, 64, 160), RGB8(255, 224, 240) },   // brood of the Kraken
    { RGB8(0, 0, 0), RGB8(104, 80, 0), RGB8(248, 208, 32), RGB8(255, 255, 224) },   // storm-chick of the Thunderbird
    { RGB8(0, 0, 0), RGB8(112, 8, 8), RGB8(240, 40, 24), RGB8(255, 224, 160) },     // the Devil's twelve siblings
    { RGB8(0, 0, 0), RGB8(24, 56, 112), RGB8(120, 200, 255), RGB8(240, 248, 255) }, // another Wendigo
    { RGB8(0, 0, 0), RGB8(8, 80, 24), RGB8(64, 240, 96), RGB8(224, 255, 208) },     // the Flatwoods crew
    { RGB8(0, 0, 0), RGB8(64, 24, 104), RGB8(168, 80, 255), RGB8(240, 224, 255) },  // unknown
};
static const char * const kin_line[6][2] = {
    {"YOU TOOK MOTHER.", "THE BROOD REMEMBERS."},
    {"YOU CAGED THE SKY.", "NOW THE STORM HUNTS."},
    {"TWELVE OF US", "ARE STILL OUT HERE."},
    {"THE HUNGER SPREADS.", "YOUR TIME WILL COME."},
    {"WE CAME FOR HIM.", "NOW WE COME FOR YOU."},
    {"NO CAGE HOLDS ME.", "YOUR TIME WILL COME."},
};

// ------------------------------------------------------------------ helpers
static uint16_t fbuf[4];
static void pal_lv(uint8_t slot, const uint16_t *src, uint8_t lv) {      // lv 0 (black) .. 8 (full)
    uint8_t i;
    for (i = 0; i < 4; i++) {
        uint16_t c = src[i];
        uint8_t r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
        r = (r * lv) >> 3; g = (g * lv) >> 3; b = (b * lv) >> 3;
        fbuf[i] = r | ((uint16_t)g << 5) | ((uint16_t)b << 10);
    }
    ld_bkg_pal(BANK_A2, slot, 1, fbuf);          // RAM source; HBlank-safe writer (the LCD is on)
}
static void e_center(uint8_t y, const char *s, uint8_t attr) {
    txt_put((20 - (uint8_t)strlen(s)) >> 1, y, s, 0, attr);
}
static void e_num(uint8_t x, uint8_t y, uint32_t v, uint8_t digits, uint8_t attr) {
    static const uint32_t p10[7] = {1000000UL, 100000UL, 10000UL, 1000UL, 100UL, 10UL, 1UL};
    uint8_t i, d; char c[2] = {0, 0};
    for (i = 7 - digits; i < 7; i++) {
        d = 0; while (v >= p10[i]) { v -= p10[i]; d++; }
        c[0] = '0' + d; txt_put(x++, y, c, 0, attr);
    }
}
static void e_prep(void) {
    screen_off(); splits_off(); oam_clear();
    n_scx = n_scy = 0;
    VBK_REG = 0;
    clear_bkg(T_SPACE, 0);
    ld_bkg_data(BANK_A2, 0, 0, N_COMMON, common_tiles);
}
static uint8_t skip_all;
static uint8_t pause(uint8_t n) {           // wait n frames; any button skips the rest of the sequence
    while (n--) {
        wait_vbl_done(); input_poll();
        if (joy_new & (J_A | J_B | J_START)) { skip_all = 1; return 1; }
    }
    return 0;
}
static uint8_t wait_btn(uint16_t max) {
    while (max--) { wait_vbl_done(); input_poll(); if (joy_new & (J_A | J_START | J_B)) return joy_new; }
    return 0;
}

// ------------------------------------------------------------------ 1. tally
static uint8_t n_cap, n_kill, n_flee, tier, outc, kin;
static void tally(void) {
    uint8_t i, sum = 0, r;
    uint32_t tot = 0;
    n_cap = n_kill = n_flee = 0;
    kin = 5;
    for (i = 0; i < N_BOUNTY; i++) {
        uint8_t o = pf_out[profile][i];
        if (o == OUT_CAPTURE) { n_cap++; kin = i; }
        else if (o == OUT_KILL) n_kill++;
        else n_flee++;
    }
    for (i = 0; i < N_MISSIONS; i++) { sum += pf_best[profile][i]; tot += pf_hi[profile][i]; }
    r = (sum + (N_MISSIONS >> 1)) / N_MISSIONS;        // 1..5
    if (r < 1) r = 1;
    tier = r >= 5 ? 3 : r >= 4 ? 2 : r >= 3 ? 1 : 0;
    outc = n_flee ? 3 : n_cap == N_BOUNTY ? 0 : n_kill > n_cap ? 2 : 1;

    e_prep();
    set_bkg_palette(7, 1, e_pal_text);
    set_bkg_palette(6, 1, e_pal_teal);
    set_bkg_palette(5, 1, e_pal_star);
    txt_fill(0, 0, 20, 18, txt_tile(' '), 7, 0);
    e_center(1, "CONTRACTS CLOSED", 6);
    {
        static const char * const pn[N_PROFILES] = {"CADET", "PILOT", "HUNTER"};
        e_center(3, pn[profile], 7);
    }
    if (gmode != MODE_HUNTER) {
        for (i = 0; i < 5; i++) {
            set_bkg_tile_xy(5 + i * 2, 5, i < r ? T_HUD_STAR : T_HUD_STARE);
            VBK_REG = 1; set_bkg_tile_xy(5 + i * 2, 5, 5); VBK_REG = 0;
        }
    } else {
        static const char rk[] = "DCBAS";
        char c[2]; c[0] = rk[r - 1]; c[1] = 0;
        txt_put(4, 5, "CAREER RANK", 0, 7);
        txt_put(16, 5, c, 0, 6);
    }
    txt_put(3, 8, "CAPTURED", 0, 6);  e_num(16, 8, n_cap, 1, 6);
    txt_put(3, 10, "KILLED", 0, 7);   e_num(16, 10, n_kill, 1, 7);
    txt_put(3, 12, "ESCAPED", 0, 7);  e_num(16, 12, n_flee, 1, 7);
    txt_put(3, 14, "BEST TOTAL", 0, 7); e_num(10, 15, tot, 7, 7);
    screen_on(LCDC_SCREEN);
    music_play(MUS_END);
    wait_frames(45);
    {
        uint16_t t = 0;
        while (t < 1500) {
            wait_vbl_done(); input_poll(); t++;
            if ((t & 31) == 0) e_center(17, "A: DEBRIEF", 6);
            if ((t & 31) == 20) txt_fill(0, 17, 20, 1, txt_tile(' '), 7, 0);
            if (joy_new & (J_A | J_START)) break;
        }
    }
    sfx_play(SFX_SELECT);
}

// ------------------------------------------------------------------ 2. crew banter
static uint8_t say(uint8_t por, const char *s) {           // returns 1 if START/B skipped the rest
    uint8_t k, x = 0, y = 0, n = 0, b, fast = 0;
    ld_bkg_data(BANK_A3, 1, 0, e_por_nt[por], e_por_tiles[por]);
    ld_bkg_pal(BANK_A3, 3, 4, e_por_pals[por]);
    ld_image(BANK_A3, 1, 11, 6, 6, e_por_map[por], e_por_attr[por], 0, 0x08, 0);
    txt_fill(8, 10, 12, 7, txt_tile(' '), 7, 0);
    txt_fill(1, 10, 7, 1, txt_tile(' '), 7, 0);
    txt_put(1, 10, e_crew[por >> 1], 0, 7);
    for (k = 0; s[k]; k++) {
        if (s[k] == '\n') { x = 0; y++; continue; }
        { char c[2]; c[0] = s[k]; c[1] = 0; txt_put(8 + x, 11 + y, c, 0, 7); }
        x++;
        if (fast) continue;
        if (!(n++ & 3)) sfx_play(SFX_BLIP);
        wait_vbl_done(); input_poll();
        b = joy_new;
        if (b & (J_START | J_B)) return 1;
        if (b & J_A) fast = 1; else wait_vbl_done();
    }
    {
        uint8_t t = 0;
        while (1) {
            wait_vbl_done(); input_poll(); t++;
            oam_fixed(0, 19 * 8 + 8 - 2, 16 * 8 + 16 + 4, ST_ARROW, 7);
            shadow_OAM[0].y = (t & 16) ? 0 : 16 * 8 + 16 + 4;
            if (joy_new & J_A) { sfx_play(SFX_SELECT); break; }
            if (joy_new & (J_START | J_B)) { shadow_OAM[0].y = 0; return 1; }
        }
        shadow_OAM[0].y = 0;
    }
    return 0;
}
static void banter(void) {
    e_prep();
    ld_spr_data(BANK_A2, 0, N_SPR_TILES, spr_tiles);
    ld_spr_pal(BANK_A2, 0, 8, obj_pals);
    set_bkg_palette(7, 1, e_pal_panel);
    ld_bkg_data(BANK_A6, 0, N_COMMON, SCENE_SHIP_NT, scene_ship_tiles);
    ld_bkg_pal(BANK_A6, 0, 3, scene_ship_pals);
    ld_image(BANK_A6, 0, 0, 20, 9, scene_ship_map, scene_ship_attr, N_COMMON, 0, 0);
    txt_fill(0, 9, 20, 9, txt_tile(' '), 7, 0);
    fill_bkg_rect(0, 9, 20, 1, T_HUD_FRAMET);
    screen_on(LCDC_SCREEN);
    if (say(P_YETI, yeti_line[tier])) return;
    if (say(outc ? P_ZANNAH + 1 : P_ZANNAH, zannah_line[outc])) return;
    if (say(P_STATIC + 1, static_line[tier])) return;
    say(P_FORGE, forge_line[kin == 5]);
}

// ------------------------------------------------------------------ 3. the watcher
#define EYE_Y 5
#define EYE_LX 3
#define EYE_RX 11
#define T_STAR (N_COMMON + EYE_SHAPE_NT)
static void eye_frame(uint8_t f) {                         // f: 0 open, 1 half, 2 shut, 3 hidden
    uint8_t l[EYE_FT], r[EYE_FT], i, row, col, base = N_COMMON + (f > 2 ? 2 : f) * EYE_FT;
    for (i = 0; i < EYE_FT; i++) {
        row = i / EYE_W; col = i - row * EYE_W;
        l[i] = f > 2 ? T_SPACE : base + i;
        r[i] = f > 2 ? T_SPACE : base + row * EYE_W + (EYE_W - 1 - col);   // mirrored column order
    }
    wait_vbl_done();
    set_bkg_tiles(EYE_LX, EYE_Y, EYE_W, EYE_H, l);
    set_bkg_tiles(EYE_RX, EYE_Y, EYE_W, EYE_H, r);
}
static void eye_flash(const uint16_t *p) {
    static uint16_t w[4];
    w[0] = p[0]; w[1] = p[3]; w[2] = p[3]; w[3] = p[3];
    wait_vbl_done(); ld_bkg_pal(BANK_A2, 2, 1, w);
    wait_frames(3);
    w[1] = p[1]; w[2] = p[2];                    // back to the eye colours (copied to RAM: the loader switches banks)
    wait_vbl_done(); ld_bkg_pal(BANK_A2, 2, 1, w);
    wait_frames(4);
}
static uint8_t type_line(uint8_t y, const char *s) {
    uint8_t k, x = (20 - (uint8_t)strlen(s)) >> 1;
    for (k = 0; s[k]; k++) {
        char c[2]; c[0] = s[k]; c[1] = 0;
        txt_put(x + k, y, c, 0, 7);
        if (s[k] != ' ' && !(k & 1)) sfx_play(SFX_TICK);
        if (pause(4)) return 1;
    }
    return 0;
}
static void watcher(void) {
    const uint16_t *p = kin_pal[kin];
    static uint16_t tp[4];
    uint8_t i, lv;
    music_stop();
    e_prep();
    VBK_REG = 0; set_bkg_data(N_COMMON, EYE_SHAPE_NT, eye_tiles + (uint16_t)kin_shape[kin] * (EYE_SHAPE_NT * 16));
    set_bkg_data(T_STAR, 2, star_tiles);
    set_bkg_palette(0, 1, e_pal_sky);          // blank tile = colour 0 = black
    set_bkg_palette(1, 1, e_pal_sky);
    pal_lv(2, p, 0);
    tp[0] = 0; tp[1] = 0; tp[2] = p[1]; tp[3] = p[2];
    set_bkg_palette(7, 1, tp);
    set_bkg_palette(6, 1, e_pal_dim);
    // starfield (attr 1), away from the eyes and text rows
    rnd_seed = 0x5D;
    for (i = 0; i < 40; i++) {
        uint8_t x = rnd() % 20, y = rnd() % 18;
        if (y >= EYE_Y - 1 && y <= EYE_Y + EYE_H) continue;
        if (y >= 11 && y <= 14) continue;
        set_bkg_tile_xy(x, y, T_STAR + (i & 1));
        VBK_REG = 1; set_bkg_tile_xy(x, y, 1); VBK_REG = 0;
    }
    VBK_REG = 1;
    fill_bkg_rect(EYE_LX, EYE_Y, EYE_W, EYE_H, 2);
    fill_bkg_rect(EYE_RX, EYE_Y, EYE_W, EYE_H, 2 | 0x20);              // X-flip: right eye
    VBK_REG = 0;
    eye_frame(2);
    screen_on(LCDC_SCREEN);
    skip_all = 0;
    if (pause(90)) goto done;
    // the seam glows, then the eyes open
    for (lv = 1; lv <= 8; lv++) { wait_vbl_done(); pal_lv(2, p, lv); if (pause(5)) goto done; }
    if (pause(30)) goto done;
    eye_frame(1); if (pause(10)) goto done;
    eye_frame(0); sfx_play(SFX_ROAR);
    if (pause(70)) goto done;
    // blink
    eye_frame(1); if (pause(4)) goto done;
    eye_frame(2); if (pause(7)) goto done;
    eye_frame(1); if (pause(4)) goto done;
    eye_frame(0); if (pause(40)) goto done;
    // flash
    sfx_play(SFX_THUNDER);
    eye_flash(p); eye_flash(p);
    if (pause(30)) goto done;
    if (type_line(12, kin_line[kin][0])) goto done;
    if (pause(25)) goto done;
    if (type_line(14, kin_line[kin][1])) goto done;
    if (pause(100)) goto done;
    eye_flash(p);
    if (pause(60)) goto done;
    // narrow, close, fade out
    eye_frame(1); if (pause(20)) goto done;
    eye_frame(2);
    for (lv = 8; lv; lv--) {
        wait_vbl_done(); pal_lv(2, p, lv - 1);
        tp[2] = fbuf[2]; tp[3] = fbuf[3];
        ld_bkg_pal(BANK_A2, 7, 1, tp);
        if (pause(6)) goto done;
    }
done:
    wait_vbl_done();
    pal_lv(2, p, 0);
    txt_fill(0, 12, 20, 3, T_SPACE, 0, 0);
    eye_frame(3);
    pause(skip_all ? 10 : 50);
    e_center(16, "TO BE CONTINUED...", 6);
    wait_frames(40);
    wait_btn(900);
}

void ending_run(void) BANKED {
    tally();
    banter();
    watcher();
}
