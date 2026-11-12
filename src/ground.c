// Mission 7 "The Tree Line": the Sneaky Kryptid is down on the planet with her guns out, and
// Graul goes out on foot. Autobanked; own loop, like the gun-cam and escape missions.
//
// Part A: Graul walks up through the forest (the field scrolls as he pushes on). A swings the
//         chain-mace in a full circle (Cadet and Pilot swing by themselves), B is a howl that
//         clears the screen (the bomb stock). An ambush and a bruiser block the way.
// Part B: back at the wreck at night. Hold it while Forge repairs the guns: critters that reach
//         the hull slow the repair to a crawl. When the bar fills the ship's turret comes online.
// Boss:   the watcher from the ending looms over the tree line (BG tiles in the 9C00 map above
//         a raster split), with the eyes of the kin of the last bounty captured. Bat its orbs
//         back with the mace, smash the claw it slams down, and let the turret work.
// Screen: field (9800 map), two window rows at line 128: status line + bar, HUD.
// Accessibility: hazards are warm and bright, Graul and the ship's guns are cool; the claw is
// marked before it lands; nothing strobes the whole screen.
#pragma bank 255
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "assets.h"
#include "ground_data.h"
#include "script_ids.h"
#include "sound.h"
#include "game.h"
#include "objs.h"
#include "stage_def.h"
#include "ground_int.h"

#define BANK_GA BANK(ground_art)
#define HA (5 | 0x80)
#define TA (7 | 0x80)
#define MAP98 ((uint8_t *)0x9800)
#define MAP9C ((uint8_t *)0x9C00)
#define GM gmode

// course, in 8-pixel rows walked
#define RA1 190u                         // ambush clearing
#define RA2 400u                         // bruiser clearing
#define RW 610u                          // the wreck's bottom row
#define STOP1 (RA1 + 17)
#define STOP2 (RA2 + 17)
#define STOP3 (RW + 13)
#define WX (XO + 80)
#define WY (YO + 77)

// the test bot reads the gr_* names
uint16_t gr_top;                         // course row of the look-ahead map row (17 rows ahead of the bottom of the screen)
uint8_t gr_fscy, gr_phase, gr_vis;
uint8_t gr_pl[32], gr_pr[32], gr_ol[32], gr_or[32];      // per map row: open columns pl..pr, blocked columns ol..or
uint8_t gr_gt[MAXG], gr_gx[MAXG], gr_gy[MAXG], gr_ghp[MAXG], gr_gs[MAXG];
uint8_t gr_bh, gr_bmax, gr_bs, gr_ef;    // watcher: health, state, eye frame (0 open, 1 half, 2 shut)
uint8_t gr_swing, gr_warn, gr_warn_x;
uint16_t gr_rep, gr_rep_max, gr_loops;

static uint8_t gtm[MAXG], ghit[MAXG];
static int8_t gvx[MAXG], gvy[MAXG];
static uint8_t wkr[32], bk[32];          // per map row: wreck image row + 1, boulder half (1 top, 2 bottom)
static uint8_t g_c, g_bcd, g_seed;
static uint16_t stop_row, enc_t;
static uint8_t enc_left, enc_hits, cp, was_comm;
static uint8_t cool, swing_id, hitstop, tag_t, bar_last, vis_t, sway_t;
static uint8_t kin, bt_n, claw_n, eflash, end_t2, guns;
static uint16_t bt;

static int16_t tb_vx[MAXTB], tb_vy[MAXTB];
uint8_t grd_facing, grd_walk_t, grd_p_inv, grd_howl_t, grd_shake_t, grd_gfl[MAXG], grd_tb_on[MAXTB];
int8_t grd_sway;
uint16_t grd_t, grd_end_t, grd_tb_x[MAXTB], grd_tb_y[MAXTB];
static uint8_t buf[32], abuf[32], wbuf[10], hud_buf[20];
static uint16_t pbuf[12], wp[8];

// [cadet, pilot, hunter]
static const uint8_t bspd[3] = {14, 17, 21};
static const uint16_t rep_tab[3] = {1300, 1500, 1500};
static const uint8_t bhp_tab[3] = {130, 170, 220};
static const uint32_t cap_thr[3] = {33000UL, 40000UL, 56000UL};      // bounty before the final award
static const uint32_t kill_thr[3] = {24000UL, 30000UL, 42000UL};
static const uint8_t kin_shape[6] = {0, 1, 2, 3, 4, 2};
static const uint16_t kin_pal[6][3] = {
    { RGB8(96, 16, 72), RGB8(224, 64, 160), RGB8(255, 224, 240) }, { RGB8(104, 80, 0), RGB8(248, 208, 32), RGB8(255, 255, 224) },
    { RGB8(112, 8, 8), RGB8(240, 40, 24), RGB8(255, 224, 160) },  { RGB8(24, 56, 112), RGB8(120, 200, 255), RGB8(240, 248, 255) },
    { RGB8(8, 80, 24), RGB8(64, 240, 96), RGB8(224, 255, 208) },  { RGB8(64, 24, 104), RGB8(168, 80, 255), RGB8(240, 224, 255) },
};
extern uint8_t pf_out[N_PROFILES][N_MISSIONS];

static uint8_t grnd(void) { g_seed = g_seed * 5 + 113; return g_seed >> 2; }
#define MR(R) ((uint8_t)(16 - (uint8_t)(R)) & 31)

// ------------------------------------------------------------------ the field
static uint8_t cell_attr;
static uint8_t tile_at(uint8_t r, uint8_t c) {
    uint8_t h = r * 37 + c * 53; h ^= h >> 3;
    if (wkr[r] && c >= 5 && c < 15 && wbuf[c - 5]) { cell_attr = 2; return GR_T_WRECK + wbuf[c - 5]; }
    if (c < gr_pl[r] || c > gr_pr[r]) {
        cell_attr = 1;
        if (c + 1 == gr_pl[r]) return GR_T_EDGE + (h & 1);
        if (c == gr_pr[r] + 1) { cell_attr = 1 | 0x20; return GR_T_EDGE + (h & 1); }
        cell_attr = 1 | ((h & 4) ? 0x20 : 0) | ((h & 8) ? 0x40 : 0);
        return GR_T_TREE + (h & 3);
    }
    if (bk[r] && c >= gr_ol[r] && c <= gr_or[r]) { cell_attr = 2; return GR_T_BOULDER + (bk[r] == 1 ? 0 : 2) + (c - gr_ol[r]); }
    cell_attr = 0;
    h &= 63;
    return h < 7 ? GR_T_GRASS0 + h % 3 : GR_T_BLANK;
}
static void row_draw(uint8_t r) {
    uint8_t c;
    if (wkr[r]) ld_copy(BANK_GA, wbuf, gr_wreck_map + (wkr[r] - 1) * 10, 10);
    for (c = 0; c < 20; c++) { buf[c] = tile_at(r, c); abuf[c] = cell_attr; }
    set_tiles(0, r, 20, 1, MAP98, buf);
    VBK_REG = 1; set_tiles(0, r, 20, 1, MAP98, abuf); VBK_REG = 0;
}
static uint8_t clearing(uint16_t R) {
    return (R + 1 >= RA1 && R <= RA1 + 15) || (R + 1 >= RA2 && R <= RA2 + 15) || (R + 2 >= RW && R <= RW + 13);
}
static void gen_row(uint16_t R) {
    uint8_t r = MR(R), hw = R < RA2 ? 5 : 4, d;
    wkr[r] = 0; bk[r] = 0; gr_ol[r] = 1; gr_or[r] = 0;
    if (R > RW + 13) { gr_pl[r] = 20; gr_pr[r] = 19; return; }          // the tree line behind the wreck
    if (clearing(R)) {
        gr_pl[r] = 1; gr_pr[r] = 18; g_c = 10; g_bcd = 6;
        if (R >= RW && R <= RW + 4) { wkr[r] = 5 - (uint8_t)(R - RW); if (R <= RW + 3) { gr_ol[r] = 6; gr_or[r] = 13; } }
        return;
    }
    if (((uint8_t)R & 3) == 0) { d = grnd() % 3; if (d == 0 && g_c > hw + 2) g_c--; else if (d == 2 && g_c < 17 - hw) g_c++; }
    gr_pl[r] = g_c - hw; gr_pr[r] = g_c + hw;
    if (g_bcd == 0xFF) {                                     // top half of the boulder started on the row below
        bk[r] = 1; gr_ol[r] = gr_ol[MR(R - 1)]; gr_or[r] = gr_ol[r] + 1; g_bcd = 7 + (grnd() & 7);
    } else if (g_bcd) g_bcd--;
    else if (R > 12) {
        bk[r] = 2; gr_ol[r] = (grnd() & 1) ? gr_pl[r] + 1 : gr_pr[r] - 2; gr_or[r] = gr_ol[r] + 1; g_bcd = 0xFF;
    }
}
static uint8_t blocked(uint8_t x, uint8_t y) {              // object coords
    uint8_t r = ((uint8_t)(gr_fscy + y - YO) >> 3) & 31, c = (uint8_t)(x - XO) >> 3;
    return c < gr_pl[r] || c > gr_pr[r] || (c >= gr_ol[r] && c <= gr_or[r]);
}

// ------------------------------------------------------------------ HUD
static const char *tag_s;
#define TAG_IDLE "               "
static void tag(const char *s, uint8_t t) { tag_s = s; tag_t = t; txt_put(0, 0, s, 1, TA); }
static void bar_draw(void) {
    uint8_t n, i, b[4];
    if (gr_phase >= P_DARK) n = gr_bmax ? (uint8_t)(((uint16_t)gr_bh * 32 + gr_bmax - 1) / gr_bmax) : 0;
    else if (gr_phase >= P_WRECK) n = (uint8_t)(gr_rep / (gr_rep_max / 32));
    else n = (uint8_t)(gr_top / (STOP3 / 32));
    if (n > 32) n = 32;
    if (n == bar_last) return;
    bar_last = n;
    for (i = 0; i < 4; i++) { b[i] = GR_HUD_BASE + (n >= 8 ? 8 : n); n = n >= 8 ? n - 8 : 0; }
    set_win_tiles(15, 0, 4, 1, b);
}
static void hud_draw(void) {
    uint8_t i;
    if (HUNTER) {
        hud_buf[0] = T_HUD_SHIP; hud_buf[1] = txt_tile('x'); hud_buf[2] = txt_tile('0' + (lives > 9 ? 9 : lives));
        hud_buf[3] = txt_tile(' '); hud_buf[4] = txt_tile(' '); hud_buf[5] = txt_tile(' ');
    } else {
        for (i = 0; i < 5; i++) hud_buf[i] = i >= MAXHEARTS ? txt_tile(' ') : i < hearts ? T_HUD_HEART : T_HUD_HEARTE;
        hud_buf[5] = txt_tile('0' + lives);
    }
    hud_buf[6] = T_HUD_BOMB; hud_buf[7] = txt_tile('0' + bombs);
    for (i = 8; i < 13; i++) hud_buf[i] = txt_tile(' ');
    score_tiles(hud_buf + 13);
    set_win_tiles(0, 1, 20, 1, hud_buf);
    hud_dirty = 0;
}

// ------------------------------------------------------------------ the watcher's head and eyes

static void eyes_set(uint8_t f) {
    uint8_t row, col, l[6], r[6], base;
    gr_ef = f;
    for (row = 0; row < 3; row++) {
        base = GR_EYE_BASE + f * GR_EYE_FT + row * 6;
        for (col = 0; col < 6; col++) { l[col] = base + col; r[col] = base + 5 - col; }
        set_tiles(3, HEAD_ROW + 2 + row, 6, 1, MAP9C, l);
        set_tiles(11, HEAD_ROW + 2 + row, 6, 1, MAP9C, r);
    }
}
static void eye_pal(uint8_t lv) {                            // 0 dark .. 2 full, 3 hit flash
    const uint16_t *k = kin_pal[kin];
    wp[0] = (k[0] >> 1) & 0x3DEF;
    wp[1] = lv ? k[0] : wp[0]; wp[2] = lv == 3 ? k[2] : lv == 2 ? k[1] : lv ? k[0] : wp[0]; wp[3] = lv >= 2 ? k[2] : lv ? k[1] : wp[0];
    ld_bkg_pal(BANK_GA, 3, 1, wp);
}
static void head_pal(void) {
    const uint16_t *k = kin_pal[kin];
    wp[4] = RGB8(4, 8, 12); wp[5] = (k[0] >> 1) & 0x3DEF; wp[6] = k[0]; wp[7] = RGB8(200, 200, 184);
    ld_bkg_pal(BANK_GA, 6, 1, wp + 4);
}
static void field_pal(uint8_t night, uint8_t lv) {           // lv 8 = full, lower = toward black (the end fade)
    uint8_t i;
    ld_copy(BANK_GA, pbuf, night ? gr_bgpal_night : gr_bgpal, 24);
    if (lv < 8) for (i = 0; i < 12; i++) {
        uint16_t c = pbuf[i];
        uint8_t r = ((c & 31) * lv) >> 3, g = (((c >> 5) & 31) * lv) >> 3, b = (((c >> 10) & 31) * lv) >> 3;
        pbuf[i] = r | ((uint16_t)g << 5) | ((uint16_t)b << 10);
    }
    ld_bkg_pal(BANK_GA, 0, 3, pbuf);
}

// ------------------------------------------------------------------ enemies
static uint8_t g_count(void) { uint8_t i, n = 0; for (i = 0; i < MAXG; i++) if (gr_gt[i] && gr_gt[i] != G_CLAW) n++; return n; }
static uint8_t spawn(uint8_t t, uint8_t x, uint8_t y) {
    uint8_t i;
    static const uint8_t hp[7] = {0, 1, 2, 2, 3, 22, 2};
    for (i = 0; i < MAXG; i++) if (!gr_gt[i]) {
        gr_gt[i] = t; gr_gx[i] = x; gr_gy[i] = y; gr_ghp[i] = hp[t]; gr_gs[i] = 0; gtm[i] = 40 + (rnd() & 31); ghit[i] = 0; gfl[i] = 0;
        gvx[i] = (rnd() & 1) ? 1 : -1; gvy[i] = 0;
        if (t == G_BRUISE) gr_ghp[i] = 14 + GM * 5;
        return i;
    }
    return 0xFF;
}
static void spawn_edge(uint8_t t, uint8_t flag) {            // from a random edge of the clearing
    uint8_t e = rnd() & 3, i;
    if (e == 0) i = spawn(t, XO + 20 + rnd() % 120, band_bot() + YO + 2);
    else if (e == 1) i = spawn(t, XO + 20 + rnd() % 120, YO + FIELD_BOT - 6);
    else i = spawn(t, e == 2 ? XO + 6 : XO + 154, YO + 30 + rnd() % 80);
    if (i != 0xFF) gr_gs[i] = flag;
}
static void clear_shots(void) { memset(eb_on, 0, sizeof eb_on); }
static void hurt(void) {
    if (p_inv || st != ST_PLAY) return;
    hits_taken++; enc_hits++;
    shake_t = 10; hud_dirty = 1;
    if (!HUNTER && hearts > 1) { hearts--; p_inv = 80; sfx_play(SFX_HURT); return; }
    hearts = 0; st = ST_DYING; st_t = 0;
    clear_shots();
    sfx_play(SFX_EXPL_L);
}
static void boss_dmg(uint8_t d) {
    if (gr_phase != P_BOSS) return;
    if (gr_ef == 2) { sfx_play(SFX_TICK); return; }           // eyes shut: nothing gets through
    gr_bh = gr_bh > d ? gr_bh - d : 0;
    eflash = 5; eye_pal(3);
    add_score((uint16_t)d * 20);
    sfx_play(SFX_EXPL_S);
}
static void g_kill(uint8_t i) {
    static const uint16_t pts[7] = {0, 100, 200, 250, 300, 5000, 0};
    add_score(pts[gr_gt[i]]);
    boom_at(gr_gx[i], gr_gy[i]);
    if (gr_gt[i] == G_BRUISE) { boom_at(gr_gx[i] - 10, gr_gy[i] + 8); boom_at(gr_gx[i] + 10, gr_gy[i] - 8); sfx_play(SFX_EXPL_L); }
    else sfx_play(SFX_EXPL_S);
    gr_gt[i] = G_NONE; kill_count++;
}
static void g_dmg(uint8_t i, uint8_t d) {
    uint8_t t = gr_gt[i];
    if (t == G_CLAW) { boss_dmg(5); add_score(400); if (gr_ghp[i] > 1) gr_ghp[i]--; else gr_gs[i] = 2; gfl[i] = 6; return; }
    gfl[i] = 14;
    if (gr_ghp[i] > d) {
        gr_ghp[i] -= d;
        if (t != G_BRUISE && t != G_SPIT) {                  // knocked back
            if (gr_gx[i] < px) { if (gr_gx[i] > XO + 14) gr_gx[i] -= 8; } else if (gr_gx[i] < XO + 146) gr_gx[i] += 8;
            if (gr_gy[i] < py) { if (gr_gy[i] > YO + 12) gr_gy[i] -= 8; } else gr_gy[i] += 8;
        }
        sfx_play(SFX_TICK);
    } else g_kill(i);
}
static void toward(uint8_t i, uint8_t tx, uint8_t ty) {
    if (gr_gx[i] < tx) gr_gx[i]++; else if (gr_gx[i] > tx) gr_gx[i]--;
    if (gr_gy[i] < ty) gr_gy[i]++; else if (gr_gy[i] > ty) gr_gy[i]--;
}
static uint8_t chewers;
static void enemies_update(void) {
    uint8_t i, t, x, y, d, slow = gr_t & 1, bb = band_bot() + YO;
    chewers = 0;
    for (i = 0; i < MAXG; i++) {
        t = gr_gt[i];
        if (!t) continue;
        x = gr_gx[i]; y = gr_gy[i];
        if (gfl[i]) gfl[i]--;
        if (y > YO + FIELD_BOT + 14 && t != G_CLAW) { gr_gt[i] = G_NONE; continue; }
        switch (t) {
        case G_CRAWL:
            if (gr_gs[i] == 1 && gr_phase == P_WRECK) {      // goes for the hull
                if (NEAR(x, WX, 38) && NEAR(y, WY, 22)) { chewers++; break; }
                if (HUNTER || slow || (gr_t & 2)) toward(i, WX, WY);
            } else if (HUNTER ? (gr_t & 3) != 0 : GM == MODE_CADET ? slow : (gr_t % 3) != 0) toward(i, px, py);
            break;
        case G_SPIT:
            if (y > bb + 6 && !--gtm[i]) {
                gtm[i] = GM == MODE_CADET ? 120 : MIDMODE ? 96 : 70;
                eb_fire(x - 3, y - 3, aim(x, y, px, py), bspd[GM], 0);
                sfx_play(SFX_SPIT);
            }
            break;
        case G_TROOP:
            d = py > bb + 64 ? py - 54 : bb + 12;
            if (slow) { if (y < d) gr_gy[i]++; else if (y > d + 2) gr_gy[i]--; }
            if (slow || HUNTER) { gr_gx[i] += gvx[i]; if (gr_gx[i] < XO + 16) gvx[i] = 1; else if (gr_gx[i] > XO + 144) gvx[i] = -1; }
            if (!--gtm[i]) {
                gtm[i] = GM == MODE_CADET ? 110 : MIDMODE ? 84 : 60;
                d = aim(x, y, px, py);
                eb_fire(x - 3, y, d, bspd[GM] + 3, 0);
                if (HUNTER) { eb_fire(x - 3, y, d + 2, bspd[GM] + 3, 0); eb_fire(x - 3, y, d - 2, bspd[GM] + 3, 0); }
                sfx_play(SFX_SPIT);
            }
            break;
        case G_CHARGE:
            if (gr_gs[i] == 0) {
                if (slow) toward(i, px, py);
                if (!--gtm[i]) { gr_gs[i] = 2; gtm[i] = GM == MODE_CADET ? 40 : 30; d = aim(x, y, px, py); gvx[i] = dsin[d] / 22; gvy[i] = DCOS(d) / 22; }
            } else if (gr_gs[i] == 2) { if (!--gtm[i]) { gr_gs[i] = 3; gtm[i] = 24; sfx_play(SFX_WHOOSH); } }      // winding up (it flashes)
            else if (gr_gs[i] == 3) {
                x += gvx[i]; y += gvy[i];
                if (x < XO + 8 || x > XO + 152 || y < bb + 4 || y > YO + FIELD_BOT - 4) gtm[i] = 1; else { gr_gx[i] = x; gr_gy[i] = y; }
                if (!--gtm[i]) { gr_gs[i] = 0; gtm[i] = 60 + (rnd() & 31); }
            }
            break;
        case G_BRUISE:
            if ((gr_t % 3) == 0) toward(i, px, py);
            if (!--gtm[i]) {
                gtm[i] = GM == MODE_CADET ? 130 : MIDMODE ? 104 : 80;
                for (d = 0; d < 8; d++) if (GM != MODE_CADET || (d & 1)) eb_fire(x - 3, y - 3, d * 4 + ((gr_t >> 6) & 2), bspd[GM], 0);
                sfx_play(SFX_SPIT);
            }
            break;
        case G_CLAW:
            if (gr_gs[i] == 0) { gr_gy[i] += 6; if (gr_gy[i] >= gtm[i]) { gr_gy[i] = gtm[i]; gr_gs[i] = 1; gtm[i] = HUNTER ? 44 : 60; shake_t = 8; sfx_play(SFX_EXPL_L); } }
            else if (gr_gs[i] == 1) { if (!--gtm[i]) gr_gs[i] = 2; }
            else { if (gr_gy[i] < bb + 8) { gr_gt[i] = G_NONE; continue; } gr_gy[i] -= 4; }
            break;
        }
        if (!gr_gt[i]) continue;
        x = gr_gx[i]; y = gr_gy[i];
        // the mace
        if (gr_swing && ghit[i] != swing_id) {
            d = t == G_BRUISE ? 34 : 27;
            if (t == G_CLAW ? (NEAR(x, px, 24) && py + 26 > (uint8_t)(y - 28) && py < y + 26) : (NEAR(x, px, d) && NEAR(y, py, d))) {
                ghit[i] = swing_id; hitstop = 3;
                g_dmg(i, 1);
                continue;
            }
        }
        if (st != ST_PLAY || gfl[i]) continue;               // one that was just hit can't hit back
        if (t == G_SPIT || (t == G_CHARGE && gr_gs[i] != 3)) continue;      // spitters only spit; chargers only hurt in the dash
        if (t == G_CLAW) { if (NEAR(x, px, 10) && py < y + 4 && py + 26 > y) hurt(); }
        else if (t == G_BRUISE) { if (NEAR(x, px, 16) && NEAR(y, py, 16)) hurt(); }
        else if (NEAR(x, px, 8) && NEAR(y, py, 8)) hurt();
    }
}
static void shots_update(void) {
    uint8_t i, x, y, bb = band_bot() + YO;
    for (i = 0; i < MAXEB; i++) {
        if (!eb_on[i]) continue;
        eb_x[i] += eb_vx[i]; eb_y[i] += eb_vy[i];
        x = eb_x[i] >> 8; y = eb_y[i] >> 8;
        if (eb_on[i] == 2) {                                 // batted back at the watcher
            if (y < bb + 2 || y < YO + 4) { eb_on[i] = 0; boom_at(x + 4, y + 8); boss_dmg(6); }
            continue;
        }
        if (x < 10 || x > 180 || y < 22 || y > FIELD_BOT + YO - 6) { eb_on[i] = 0; continue; }
        if (gr_swing && NEAR(x + 3, px, 25) && NEAR(y + 3, py, 25)) {
            if (eb_big[i]) { eb_on[i] = 2; eb_vy[i] = -640; eb_vx[i] = ((int16_t)((x < WX ? XO + 48 : XO + 112) - x)) * 6; add_score(300); sfx_play(SFX_POWER); }
            else { eb_on[i] = 0; add_score(10); }
            continue;
        }
        if (st == ST_PLAY && NEAR(x + 3, px, 5) && NEAR(y + 3, py + 2, 6)) { eb_on[i] = 0; hurt(); }
    }
}
// the ship's turret, once the guns are back
static void bolts_update(void) {
    uint8_t i, j, x, y, d, best = 0xFF, bd = 255, k;
    for (i = 0; i < MAXTB; i++) {
        if (!tb_on[i]) continue;
        tb_x[i] += tb_vx[i]; tb_y[i] += tb_vy[i];
        x = tb_x[i] >> 8; y = tb_y[i] >> 8;
        if (y < band_bot() + YO + 2) { tb_on[i] = 0; boss_dmg(1); continue; }
        if (x < 12 || x > 176 || y > YO + FIELD_BOT - 4) { tb_on[i] = 0; continue; }
        for (j = 0; j < MAXG; j++) if (gr_gt[j] && gr_gt[j] != G_CLAW && NEAR(gr_gx[j], x, 9) && NEAR(gr_gy[j], y, 9)) { tb_on[i] = 0; g_dmg(j, 1); break; }
    }
    if (!guns || (gr_t & 31) || st != ST_PLAY) return;
    for (j = 0; j < MAXG; j++) if (gr_gt[j] && gr_gt[j] != G_CLAW) {
        k = (gr_gx[j] > WX ? gr_gx[j] - WX : WX - gr_gx[j]) + (gr_gy[j] > WY ? gr_gy[j] - WY : WY - gr_gy[j]);
        if (k < bd) { bd = k; best = j; }
    }
    if (best == 0xFF && gr_phase != P_BOSS) return;
    for (i = 0; i < MAXTB; i++) if (!tb_on[i]) {
        k = (gr_t & 32) ? WX - 20 : WX + 20;                 // alternate gun pods
        d = best == 0xFF ? 16 : aim(k, WY + 8, gr_gx[best], gr_gy[best]);
        tb_on[i] = 1; tb_x[i] = (uint16_t)k << 8; tb_y[i] = (uint16_t)(WY + 8) << 8;
        tb_vx[i] = (int16_t)dsin[d] * 10; tb_vy[i] = (int16_t)DCOS(d) * 10;
        sfx_play(SFX_TICK);
        return;
    }
}

// ------------------------------------------------------------------ scrolling and the course
static void walk_spawn(uint16_t R) {
    uint8_t r = MR(R), sec = R < RA1 ? 0 : R < RA2 ? 1 : 2, h, x, t;
    static const uint8_t every[3][3] = {{11, 9, 8}, {10, 8, 6}, {9, 7, 5}};
    if (clearing(R) || clearing(R + 6) || R < 24 || R > RW - 8) return;
    if (R % every[GM][sec]) return;
    if (g_count() >= (GM == MODE_CADET ? 4 : 6)) return;
    h = grnd();
    x = XO + (gr_pl[r] + 1 + h % (gr_pr[r] - gr_pl[r] - 1)) * 8 + 4;
    t = sec == 0 ? ((h & 8) ? G_SPIT : G_CRAWL) : (h & 24) == 0 ? G_SPIT : (h & 24) == 8 ? G_TROOP : (h & 24) == 16 ? G_CHARGE : G_CRAWL;
    spawn(t, x, YO - 10);
    if (sec == 2 && (h & 4)) spawn(G_CRAWL, XO + 80, YO + FIELD_BOT - 4);      // and one from behind
}
static void row_step(void) {
    gr_top++;
    gen_row(gr_top); row_draw(MR(gr_top));
    if ((gr_top & 7) == 0) add_score(40);
    walk_spawn(gr_top);
    if (gr_top == 60) comm_queue_add(CM7_HINT);
    if (gr_top == RA2 - 30) cp = 1;
    if (gr_top == RW - 24) { cp = 2; comm_queue_add(CM7_NEAR); }
}
static void scroll_by(uint8_t n) {
    uint8_t i;
    while (n--) {
        if (gr_top >= stop_row) return;
        gr_fscy--;
        if ((gr_fscy & 7) == 7) row_step();
        for (i = 0; i < MAXG; i++) if (gr_gt[i]) gr_gy[i]++;
        for (i = 0; i < MAXEB; i++) if (eb_on[i]) eb_y[i] += 256;
        for (i = 0; i < MAXE; i++) if (e_type[i]) e_y[i] += 256;
        py++;
    }
}

// ------------------------------------------------------------------ Graul
#define PX_MIN (XO + 7)
#define PX_MAX (XO + 153)
#define PY_MAX (YO + FIELD_BOT - 10)
#define SCROLL_Y (YO + 56)
static uint8_t free_at(uint8_t x, uint8_t y) {
    return !blocked(x - 5, y + 1) && !blocked(x + 5, y + 1) && !blocked(x - 5, y + 7) && !blocked(x + 5, y + 7);
}
static void player_update(void) {
    uint8_t ymin = band_bot() + YO + 10, i, n = 1 + (gr_t & 1), go = 0;
    int8_t dx = 0, dy = 0;
    if (joy & J_LEFT) dx = -1; else if (joy & J_RIGHT) dx = 1;
    if (joy & J_UP) dy = -1; else if (joy & J_DOWN) dy = 1;
    if (dy > 0) facing = 0; else if (dy < 0) facing = 1; else if (dx) facing = dx > 0 ? 2 : 3;
    while (n--) {
        if (dx && px + dx >= PX_MIN && px + dx <= PX_MAX && free_at(px + dx, py)) px += dx;
        if (dy && py + dy >= ymin && py + dy <= PY_MAX && free_at(px, py + dy)) py += dy;
    }
    if (py < ymin) py = ymin;
    if (dx || dy) walk_t++;
    if (py < SCROLL_Y) scroll_by(SCROLL_Y - py);
    if (cool) cool--;
    if (gr_swing) { gr_swing--; if (!gr_swing) cool = HUNTER ? 5 : 9; }
    else if (!cool) {
        go = (joy & J_A) != 0;
        if (!go && !HUNTER) {                                // Cadet and Pilot swing by themselves when something is in reach
            for (i = 0; i < MAXG; i++) if (gr_gt[i] && NEAR(gr_gx[i], px, gr_gt[i] == G_BRUISE ? 32 : 24) && NEAR(gr_gy[i], gr_gt[i] == G_CLAW ? py + 14 : py, gr_gt[i] == G_BRUISE ? 32 : 24)) go = 1;
            for (i = 0; i < MAXEB; i++) if (eb_on[i] == 1 && NEAR((uint8_t)(eb_x[i] >> 8) + 3, px, 22) && NEAR((uint8_t)(eb_y[i] >> 8) + 3, py, 22)) go = 1;
        }
        if (go) { gr_swing = 16; swing_id++; if (!swing_id) swing_id = 1; sfx_play(SFX_WHOOSH); }
    }
    if ((joy_new & J_B) && bombs && !howl_t) {               // the howl
        bombs--; howl_t = 36; if (p_inv < 50) p_inv = 50; hud_dirty = 1;
        for (i = 0; i < MAXEB; i++) if (eb_on[i]) { eb_on[i] = 0; add_score(10); }
        for (i = 0; i < MAXG; i++) if (gr_gt[i] && gr_gt[i] != G_CLAW) g_dmg(i, 3);
        boss_dmg(6);
        tag("HOWL!          ", 60);
        sfx_play(SFX_ROAR);
    }
}

// ------------------------------------------------------------------ encounters, the wreck, the watcher
static void watcher_in(void) {                               // night falls and the eyes open
    gr_phase = P_DARK; enc_t = 0; guns = 1;
    field_pal(1, 8); head_pal(); eye_pal(0);
    wait_vbl_done(); eyes_set(2);
    gr_bmax = bhp_tab[GM]; gr_bh = gr_bmax; bar_last = 0xFF;
    set_win_tile_xy(19, 0, GR_HUD_BASE + 10);
    music_play(MUS_WARN);
    tag("IT'S HERE...   ", 0);
    comm_queue_add(CM7_DARK);
}
static void encounter_start(void) {
    enc_t = 0; enc_hits = 0;
    if (stop_row == STOP1) { gr_phase = P_AMBUSH; enc_left = GM == MODE_CADET ? 8 : MIDMODE ? 11 : 14; tag("AMBUSH!        ", 120); comm_queue_add(CM7_AMBUSH); sfx_play(SFX_ALARM); }
    else if (stop_row == STOP2) { gr_phase = P_BRUISER; spawn(G_BRUISE, XO + 80, YO + 8); tag("BRUISER!       ", 120); comm_queue_add(CM7_BRUISER); music_play(MUS_BOSS); }
    else { gr_phase = P_WRECK; gr_rep = 0; tag("HOLD THE WRECK!", 150); comm_queue_add(CM7_WRECK); set_win_tile_xy(19, 0, GR_HUD_BASE + 9); bar_last = 0xFF; music_play(MUS_STAGE4); }
}
static void encounter_done(const char *s) {
    uint8_t i;
    for (i = 0; i < MAXG; i++) if (gr_gt[i]) g_kill(i);
    if (!enc_hits) { add_score(2500); tag("CLEAN! +2500   ", 120); sfx_play(SFX_PICKUP); } else tag(s, 120);
    gr_phase = P_WALK; stop_row = stop_row == STOP1 ? STOP2 : STOP3;
}
static void boss_next(uint8_t s) {
    gr_bs = s; bt = 0; bt_n = 0;
    wait_vbl_done();
    if (s == 0) { eyes_set(0); eye_pal(2); }
    else if (s == 1) { eyes_set(1); eye_pal(2); claw_n = HUNTER ? 3 : 2; gr_warn = 0; }
    else { eyes_set(2); eye_pal(1); tag("EYES SHUT: WAIT", 100); }
}
static void boss_update(void) {
    uint8_t i, ex, by = band_bot() + YO, fast = gr_bh < (gr_bmax >> 1), iv;
    bt++;
    if (eflash && !--eflash) eye_pal(gr_ef == 2 ? 1 : 2);
    if (!gr_bh) {                                            // down: the bounty decides how it ends
        gr_phase = P_END; end_t = 0; gr_warn = 0;
        bounty_out = bounty_pts >= cap_thr[GM] ? OUT_CAPTURE : bounty_pts >= kill_thr[GM] ? OUT_KILL : OUT_FLEE;
        clear_shots(); for (i = 0; i < MAXG; i++) if (gr_gt[i]) g_kill(i);
        add_score(bounty_out == OUT_FLEE ? 4000 : 10000);
        wait_vbl_done(); eyes_set(bounty_out == OUT_FLEE ? 1 : 2); eye_pal(2);
        tag(bounty_out == OUT_CAPTURE ? "SEALED! GOT IT " : bounty_out == OUT_KILL ? "THE GUNS END IT" : "IT SLIPS AWAY..", 0);
        comm_queue_add(bounty_out == OUT_CAPTURE ? CM7_BCAP : bounty_out == OUT_KILL ? CM7_BKILL : CM7_BFLEE);
        music_play(MUS_CLEAR);
        return;
    }
    switch (gr_bs) {
    case 0:                                                  // eyes open: orbs to bat back
        iv = (GM == MODE_CADET ? 84 : MIDMODE ? 66 : 50) - (fast ? 12 : 0);
        if (bt % iv == iv - 1) {
            ex = ((bt_n++ & 1) ? XO + 112 : XO + 48) + sway;
            eb_fire(ex - 4, by - 2, aim(ex, by, px, py), bspd[GM] - 3, 1);
            if (GM != MODE_CADET) { eb_fire(ex - 4, by, aim(ex, by, px, py) + 4, bspd[GM] + 2, 0); eb_fire(ex - 4, by, aim(ex, by, px, py) - 4, bspd[GM] + 2, 0); }
            sfx_play(SFX_SPIT);
        }
        if (bt > (fast ? 230u : 280u)) boss_next(bt_n & 1 ? 1 : 2);
        break;
    case 1:                                                  // eyes narrowed: the claw. A marker shows where, then it lands
        if (gr_warn) {
            if (!--gr_warn) { i = spawn(G_CLAW, gr_warn_x, by + 4); if (i != 0xFF) { gtm[i] = py < by + 40 ? by + 44 : py + 6; if (gtm[i] > YO + FIELD_BOT - 8) gtm[i] = YO + FIELD_BOT - 8; } claw_n--; bt = 0; }
        } else {
            for (i = 0; i < MAXG; i++) if (gr_gt[i] == G_CLAW) break;
            if (i == MAXG && bt > 30) {
                if (!claw_n) boss_next(0);
                else { gr_warn = GM == MODE_CADET ? 56 : MIDMODE ? 46 : 36; gr_warn_x = px; sfx_play(SFX_ALARM); }
            }
        }
        break;
    case 2:                                                  // eyes shut: it calls the brood
        if (bt == 10 || bt == 70) { spawn_edge(G_CRAWL, 0); spawn_edge(G_CRAWL, 0); if (GM != MODE_CADET) spawn_edge(fast ? G_CHARGE : G_CRAWL, 0); }
        if (bt > 150) boss_next(0);
        break;
    }
}
static void phase_update(void) {
    uint8_t n;
    enc_t++;
    switch (gr_phase) {
    case P_WALK:
        if (gr_top >= stop_row) encounter_start();
        break;
    case P_AMBUSH:
        n = g_count();
        if (enc_left && n < (GM == MODE_CADET ? 3 : 4) && (enc_t % 36) == 0) { enc_left--; spawn_edge((enc_left % 3) == 0 ? G_CHARGE : (enc_left & 1) ? G_TROOP : G_CRAWL, 0); }
        if (!enc_left && !n) { encounter_done("PATH CLEAR     "); comm_queue_add(CM7_CLEAR); }
        break;
    case P_BRUISER:
        for (n = 0; n < MAXG; n++) if (gr_gt[n] == G_BRUISE) break;
        if (n == MAXG) {
            midboss_killed = 1; mid_out = enc_hits ? OUT_KILL : OUT_CAPTURE;
            encounter_done("BRUISER DOWN   "); comm_queue_add(CM7_BRDOWN); music_play(MUS_STAGE3);
        } else if ((enc_t % (GM == MODE_CADET ? 240 : 170)) == 0 && g_count() < 3) spawn_edge(G_CRAWL, 0);
        break;
    case P_WRECK:
        if (!chewers) { gr_rep++; if (tag_s[0] == 'O' && tag_t > 2) tag_t = 2; }
        else if (!(enc_t & 3)) gr_rep++;                     // chewed wiring: the repair crawls, but never stops
        if (chewers && (enc_t & 63) == 0) { tag("OFF THE HULL!  ", 70); if (!(enc_t & 255)) comm_queue_add(CM7_HULL); }
        n = GM == MODE_CADET ? 100 : MIDMODE ? 80 : 60;
        if ((enc_t % n) == 0 && g_count() < (GM == MODE_CADET ? 4 : 6)) {
            n = rnd() & 7;
            spawn_edge(n < 5 ? G_CRAWL : n < 7 ? G_TROOP : G_CHARGE, n < 4 && chewers < 2);
        }
        if (gr_rep >= gr_rep_max) {
            gr_phase = P_GUNS; enc_t = 0; guns = 1;
            add_score(3000); if (!enc_hits) add_score(2500);
            tag("GUNS ONLINE!   ", 150); comm_queue_add(CM7_GUNS); sfx_play(SFX_POWER);
        }
        break;
    case P_GUNS:
        if ((!g_count() && enc_t > 90) || enc_t > 420) { for (n = 0; n < MAXG; n++) if (gr_gt[n]) g_kill(n); cp = 3; watcher_in(); }
        break;
    case P_DARK:
        if (enc_t == 100) { vis_t = 40; sfx_play(SFX_THUNDER); }
        if (enc_t == 190) { wait_vbl_done(); eyes_set(1); eye_pal(1); }
        if (enc_t == 215) { wait_vbl_done(); eyes_set(0); eye_pal(2); sfx_play(SFX_ROAR); comm_queue_add(CM7_EYES); }
        if (enc_t == 270) { gr_phase = P_BOSS; boss_next(0); tag("BAT ORBS BACK! ", 200); music_play(MUS_WENDIGO); }
        break;
    case P_BOSS:
        boss_update();
        break;
    case P_END:
        end_t++;
        if (bounty_out == OUT_KILL && end_t < 100 && (end_t & 7) == 0) { boom_at(XO + 20 + (rnd() & 127), YO + 4 + (rnd() & 31)); sfx_play(SFX_EXPL_S); }
        if (end_t == 110) { eye_pal(1); if (bounty_out != OUT_FLEE) { wait_vbl_done(); eyes_set(2); } }
        if (end_t == 130) { eye_pal(0); vis_t = 0; }
        if (end_t == 260) { st = ST_CLEAR; st_t = 0; }
        break;
    }
}

// ------------------------------------------------------------------ setup
static void gr_load(void) {
    uint8_t r, i;
    screen_off(); splits_off(); oam_clear();
    kin = 5;
    for (i = 0; i < 5; i++) if (pf_out[profile][i] == OUT_CAPTURE) kin = i;
    VBK_REG = 0;
    ld_spr_data(BANK_A2, 0, N_SPR_TILES, spr_tiles);
    VBK_REG = 1; ld_spr_data(BANK_GA, 0, GR_SPR_NT, gr_spr); VBK_REG = 0;
    ld_spr_pal(BANK_A2, 0, 3, obj_pals);
    ld_spr_pal(BANK_A2, 7, 1, obj_pals + 28);
    ld_spr_pal(BANK_GA, 3, 4, gr_objpal);
    ld_bkg_data(BANK_A2, 0, 0, N_COMMON, common_tiles);
    ld_bkg_data(BANK_GA, 0, N_COMMON, GR_FIELD_NT, gr_field_tiles);
    ld_bkg_data(BANK_GA, 1, 0, GR_HEAD_NT, gr_head_tiles);
    ld_bkg_data(BANK_GA, 1, GR_HUD_BASE, GR_HUD_NT, gr_hud_tiles);
    ld_bkg_data(BANK_GA, 1, GR_EYE_BASE, GR_EYE_SHAPE_NT, gr_eye_tiles + (uint16_t)kin_shape[kin] * (GR_EYE_SHAPE_NT * 16));
    set_pal_slot(5, BP_HUD);
    ld_bkg_pal(BANK_GA, 7, 1, gr_tagpal);
    memset(buf, GR_T_BLANK, 32); memset(abuf, 6, 32);
    for (r = 6; r < 32; r++) { set_tiles(0, r, 32, 1, MAP9C, buf); VBK_REG = 1; set_tiles(0, r, 32, 1, MAP9C, abuf); VBK_REG = 0; }
    for (r = 0; r < 7; r++) {
        ld_copy(BANK_GA, buf, gr_head_map + r * 20, 20); set_tiles(0, HEAD_ROW + r, 20, 1, MAP9C, buf);
        ld_copy(BANK_GA, buf, gr_head_attr + r * 20, 20); VBK_REG = 1; set_tiles(0, HEAD_ROW + r, 20, 1, MAP9C, buf); VBK_REG = 0;
    }
    memset(abuf, 3 | 0x08, 6); memset(abuf + 6, 3 | 0x08 | 0x20, 6);
    VBK_REG = 1;
    for (r = 0; r < 3; r++) { set_tiles(3, HEAD_ROW + 2 + r, 6, 1, MAP9C, abuf); set_tiles(11, HEAD_ROW + 2 + r, 6, 1, MAP9C, abuf + 6); }
    VBK_REG = 0;
    move_win(7, FIELD_BOT);
}
static void restart_from(uint8_t c) {
    uint16_t B = c == 0 ? 0 : c == 1 ? RA2 - 30 : c == 2 ? RW - 24 : STOP3 - 17, R;
    uint8_t r;
    screen_off(); splits_off();
    memset(e_type, 0, sizeof e_type); memset(eb_on, 0, sizeof eb_on);
    memset(gr_gt, 0, sizeof gr_gt); memset(tb_on, 0, sizeof tb_on);
    comm_reset(); was_comm = 0;
    boss_clear(); k_hpmax = 1;                                // crew lines never wait for a quiet moment here
    cp = c; gr_phase = P_WALK; guns = 0;
    stop_row = B + 17 < STOP1 ? STOP1 : B + 17 < STOP2 ? STOP2 : STOP3;
    gr_fscy = (uint8_t)(((uint8_t)(0 - (uint8_t)B)) & 31) << 3;
    g_seed = (uint8_t)B + 9; g_c = 10; g_bcd = 5;
    for (r = 0; r < 32; r++) { gr_pl[r] = 20; gr_pr[r] = 19; gr_ol[r] = 1; gr_or[r] = 0; wkr[r] = 0; bk[r] = 0; }
    for (R = B; R <= B + 17; R++) gen_row(R);
    for (r = 0; r < 32; r++) row_draw(r);
    gr_top = B + 17;
    field_pal(0, 8); head_pal(); eye_pal(0); eyes_set(2);
    gr_vis = 0; vis_t = 0; sway = 0; gr_bmax = 0; gr_bh = 0; gr_warn = 0; gr_rep = 0; gr_rep_max = rep_tab[GM]; gr_swing = 0;
    px = XO + 80; py = YO + 100; p_inv = 100; cool = 0; howl_t = 0; shake_t = 0; hitstop = 0; facing = 1; eflash = 0;
    if (c == 3) { px = XO + 40; py = YO + 108; }
    while (!free_at(px, py) && px < PX_MAX) px += 8;
    bar_last = 0xFF; gr_t = 0;
    st = ST_PLAY; st_t = 0;
    txt_fill(0, 0, 20, 2, txt_tile(' '), HA, 1);
    VBK_REG = 1; memset(buf, 5 | 0x08 | 0x80, 5); set_win_tiles(15, 0, 5, 1, buf); VBK_REG = 0;
    set_win_tile_xy(19, 0, txt_tile(' '));
    tag(TAG_IDLE, 0); bar_draw(); hud_draw();
    ground_draw();
    music_play(MUS_STAGE3);
    oam_clear();
    screen_on(n_lcdc);
    if (c == 0) comm_queue_add(CM7_START);
    if (c == 3) { gr_rep = gr_rep_max; watcher_in(); }
}
static uint8_t continue_prompt(void) {
    uint8_t t = 9, f = 0;
    comm_reset(); was_comm = 0;
    oam_clear();
    music_play(MUS_OVER);
    txt_fill(0, 0, 20, 2, txt_tile(' '), HA, 1);
    txt_put(5, 0, "GAME OVER", 1, HA);
    wait_frames(110);
    txt_fill(0, 0, 20, 2, txt_tile(' '), HA, 1);
    if (!HUNTER) {
        txt_put(5, 0, "TRY AGAIN!", 1, HA);
        txt_put(3, 1, "YOU CAN DO IT", 1, HA);
        wait_frames(100);
        return 1;
    }
    txt_put(5, 0, "CONTINUE?", 1, HA);
    txt_put(2, 1, "A: YES  START: NO", 1, HA);
    while (1) {
        wait_vbl_done(); input_poll();
        set_win_tile_xy(15, 0, txt_tile('0' + t));
        if (++f >= 60) { f = 0; if (t == 0) return 0; t--; sfx_play(SFX_SELECT); }
        if (joy_new & J_A) return 1;
        if (joy_new & J_START) return 0;
    }
}

// ------------------------------------------------------------------ main loop
uint8_t ground_run(void) BANKED {
    uint8_t paused = 0, sec_t = 0;
    stage_id = GRD_SID;
    gr_load();
    score = 0; hits_taken = 0; continues_used = 0; play_seconds = 0; midboss_killed = 0; kill_count = 0;
    lives = 3 + g_gear[GEAR_HULL]; hearts = MAXHEARTS; bombs = 3 + g_gear[GEAR_BOMBS]; wtype = 0; wlevel = 0;
    bounty_pts = 0; bounty_out = OUT_NONE; mid_out = OUT_NONE;
    next_extend = 30000UL;
    restart_from(start_cp == 2 ? 3 : 0);
    while (1) {
        wait_vbl_done();
        input_poll();
        if ((joy_new & J_START) && st == ST_PLAY) {
            paused ^= 1;
            sfx_play(SFX_SELECT);
            if (paused) txt_put(13, 1, "PAUSE  ", 1, HA); else hud_dirty = 1;
        }
        if (paused) continue;
        gr_loops++;
        if (++sec_t >= 60) { sec_t = 0; play_seconds++; }
        comm_update();
        if (hitstop) { hitstop--; ground_draw(); continue; }      // a beat of freeze when the mace lands
        gr_t++;
        if (!(gr_t & 1)) {
            if (gr_vis < vis_t) gr_vis = gr_vis < 8 ? 8 : gr_vis + 1;
            else if (gr_vis > vis_t) gr_vis = gr_vis <= 8 ? 0 : gr_vis - 1;
        }
        if (gr_vis && !(gr_t & 7)) { sway_t++; sway = sin64[sway_t & 63] >> 4; }

        if (st == ST_PLAY) { player_update(); phase_update(); }
        if (p_inv) p_inv--;
        enemies_update(); shots_update(); bolts_update();

        if (st == ST_DYING) {
            st_t++;
            shake_t = 4;
            if ((st_t & 7) == 1) { boom_at(px - 8 + (rnd() & 15), py - 8 + (rnd() & 15)); sfx_play(SFX_EXPL_S); }
            if (st_t == 70) {
                if (lives > 1) {
                    lives--; hearts = MAXHEARTS;
                    if (bombs < 3 + g_gear[GEAR_BOMBS]) bombs = 3 + g_gear[GEAR_BOMBS];
                    p_inv = 150; st = ST_PLAY; hud_dirty = 1;
                } else {
                    if (!continue_prompt()) { k_hpmax = 0; return RES_GAMEOVER; }
                    continues_used++;
                    if (HUNTER) { score = 0; bounty_pts = 0; } else bounty_pts >>= 1;
                    lives = 3 + g_gear[GEAR_HULL]; hearts = MAXHEARTS; bombs = 3 + g_gear[GEAR_BOMBS];
                    restart_from(cp);
                    continue;
                }
            }
        }
        if (st == ST_CLEAR) {                                // a slow fade to black; no flashes
            st_t++;
            if (st_t <= 64 && (st_t & 7) == 0) field_pal(1, 8 - (st_t >> 3));
            if (st_t >= 90) { oam_clear(); k_hpmax = 0; return RES_CLEAR; }
        }
        if (tag_t && !--tag_t) tag(TAG_IDLE, 0);
        if (hud_dirty) hud_draw();
        if ((gr_t & 15) == 0) bar_draw();
        ground_draw();
    }
}
