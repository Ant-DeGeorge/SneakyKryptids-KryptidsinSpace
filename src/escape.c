// Mission 6 "Escape Run": Nubs's whole gang jumps the Sneaky Kryptid, an EMP kills the guns,
// and the only way out is down, into the atmosphere of the planet below. No shooting: the
// whole mission is dodging. Autobanked; own loop, like the gun-cam mission.
//
// The ship faces DOWN the screen and the field scrolls the other way. Nubs's flagship chases
// from the top (BG tiles in the 9C00 map, shown above a raster split, so it costs no sprites).
// Asteroid walls and laser columns are BG tiles in the scrolling 9800 map; bullets, missiles
// and loose rocks are sprites and share the usual flicker rotation.
//   A  dash when the BOOST meter is full (grazing shots fills it). Cadet also dashes on its own.
//   B  shield pulse (the bomb stock): clears nearby shots and missiles.
// Screen: lines 0..vis-1 the flagship, then the field, then two window rows at line 128:
// row 0 status line + distance bar, row 1 HUD.
// Accessibility: hazards are warm and bright, the player side is cool; lasers warn with a
// blinking line and a marker before they fire; nothing strobes the whole screen.
#pragma bank 255
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "assets.h"
#include "escape_data.h"
#include "script_ids.h"
#include "sound.h"
#include "game.h"
#include "objs.h"

#define BANK_ER BANK(escape_art)
#define HA (5 | 0x80)
#define TA (7 | 0x80)
#define MAP98 ((uint8_t *)0x9800)
#define MAP9C ((uint8_t *)0x9C00)
#define ER_STAGE 5                       // mission index (0-based)
#define FIELD_BOT 128                    // first screen line of the HUD
#define PUR_ROW 8                        // flagship rows in the 9C00 map

// course, in 8-pixel rows
#define ROW_GAUNT 1100u
#define ROW_SPRINT 2000u
#define ROW_POD 2900u
#define ROW_BREAK 3040u                  // the flagship pulls up
#define ROW_END 3200u
uint16_t er_row;                         // rows flown (the test bot reads the er_* names)
const uint16_t er_total = ROW_END;
uint8_t er_fscy;                         // field scroll (map y shown on screen line 0)
uint8_t er_wall[32], er_wl[32], er_wr[32];   // per map row: wall present, first and last open column
uint8_t er_las_state, er_las_n, er_las_col[4];   // 0 idle, 1 placing, 2 warning, 3 firing, 4 clearing
uint8_t er_vis;                          // flagship lines on screen
uint8_t er_boost;
#define BOOST_MAX 12
#define MAXMIS 3
uint8_t er_mis_on[MAXMIS], er_mis_d[MAXMIS], er_mis_t[MAXMIS];
uint16_t er_mis_x[MAXMIS], er_mis_y[MAXMIS];
#define MAXRK 4
uint8_t er_rk_on[MAXRK], er_rk_x[MAXRK], er_rk_t[MAXRK];     // on: 1 warned at the bottom edge, 2 flying
uint16_t er_rk_y[MAXRK];
uint8_t er_pod, er_pod_x, er_pod_y;      // 0 none, 1 drifting, 2 taken, 3 missed
uint8_t er_next_c, er_next_warn;         // next wall's gap centre column and whether to mark it
uint16_t er_loops;

static uint8_t rseed[32], rkind[32];     // per map row: scenery seed, 0 space / 1 atmosphere / 2 haze
static uint8_t acc, phase, vis_t, sway_t, p_inv, dash_t, pulse_t, pulse_x, pulse_y, shake_t, graze_t, graze_x, graze_y;
static int8_t sway, dash_dx, dash_dy, last_dx;
static uint8_t tag_t, sec_clean, atmo_k, glow_k, las_t, las_i, atk_i, burst_n, burst_t, burst_k, flame_t;
static uint16_t atk_cd, er_t;
static uint8_t g_cd, g_c, g_corr, g_seed, g_thick;
static uint8_t cp, pod_ok, was_comm, bar_last, wob;
static uint8_t muz[5];
static uint16_t palbuf[4], bg0;
static uint8_t buf[32], abuf[32];
static uint8_t hud_buf[20];
static uint8_t trail_x[3], trail_y[3];
// The flagship band (9C00 map) cannot scroll with the field, so its empty sky would sit still
// while the field's stars stream past below it. These sprite stars keep the sky moving there:
// they rise at the field's speed and pass behind the hull (OBJ priority).
#define NBST 5
static uint8_t bst_x[NBST], bst_y[NBST], last_step;

// [cadet, pilot, hunter]
static const uint8_t spd_tab[3][3] = {{32, 24, 40}, {32, 28, 44}, {36, 32, 48}};      // scroll, 1/16 px per frame, per phase
static const uint8_t cd_tab[3][3] = {{120, 220, 95}, {104, 190, 78}, {84, 160, 62}};   // frames between attacks, per phase
static const uint8_t bspd_tab[3] = {17, 22, 24};                                      // bullet speed, 1/16 px
// Hunter flies the escape with two hits per ship: no guns here, so one-hit ships ended runs in the first minute
#define ER_HEARTS (HUNTER ? 2 : MAXHEARTS)
static const uint8_t gain_tab[3] = {3, 2, 2};                                         // boost per graze
static const uint32_t cap_thr[3] = {40000UL, 40000UL, 50000UL};                       // bounty needed for the brig pod
#define GM gmode

static uint8_t grnd(void) { g_seed = g_seed * 5 + 113; return g_seed >> 2; }

// ------------------------------------------------------------------ palettes
static void pal_set(uint8_t slot, const uint16_t *src) {      // src in the art bank; colour 0 follows the sky
    ld_copy(BANK_ER, palbuf, src, 8);
    palbuf[0] = bg0;
    ld_bkg_pal(BANK_ER, slot, 1, palbuf);                     // RAM source; the loader waits for HBlank per colour
}
static void laser_pal(uint8_t k) {                            // 0 warn bright, 1 warn dim, 2 firing
    ld_copy(BANK_ER, palbuf, er_laserpal + k * 4, 8);
    palbuf[0] = bg0;
    if (k < 2) { palbuf[2] = bg0; palbuf[3] = bg0; }
    ld_bkg_pal(BANK_ER, 6, 1, palbuf);
}
static void sky_set(const uint16_t *src) {                    // the space palette, and everyone's colour 0 with it
    ld_copy(BANK_ER, palbuf, src, 8);
    bg0 = palbuf[0];
    ld_bkg_pal(BANK_ER, 0, 1, palbuf);
    pal_set(1, er_bgpal + 4);
    pal_set(2, er_pur_pals); pal_set(3, er_pur_pals + 4);
    laser_pal(er_las_state == 3 ? 2 : 0);
}

// ------------------------------------------------------------------ the field (9800 map)
static uint8_t cell_attr;
static uint8_t tile_at(uint8_t r, uint8_t c) {
    uint8_t h, i;
    if (er_las_state && er_las_state != 4) for (i = 0; i < er_las_n; i++) if (er_las_col[i] == c && (er_las_state != 1 || i < las_i)) { cell_attr = 6; return ER_T_LASER; }
    h = rseed[r] + c * 53; h ^= h >> 3;
    if (er_wall[r] && (c < er_wl[r] || c > er_wr[r])) { cell_attr = 1 | ((h & 4) ? 0x20 : 0) | ((h & 8) ? 0x40 : 0); return ER_T_ROCK + (h & 3); }
    cell_attr = 0;
    if (rkind[r] == 2) return ER_T_HAZE;
    h &= 127;
    if (rkind[r] == 1 && h >= 100 && h < 116) return ER_T_WISP + (h & 3) % 3;
    if (h < 9) return ER_T_STAR0 + h % 3;
    return ER_T_BLANK;
}
static void row_draw(uint8_t r) {
    uint8_t c;
    for (c = 0; c < 20; c++) { buf[c] = tile_at(r, c); abuf[c] = cell_attr; }
    set_tiles(0, r, 20, 1, MAP98, buf);
    VBK_REG = 1; set_tiles(0, r, 20, 1, MAP98, abuf); VBK_REG = 0;
}
static void col_draw(uint8_t c) {
    uint8_t r;
    for (r = 0; r < 32; r++) { buf[r] = tile_at(r, c); abuf[r] = cell_attr; }
    set_tiles(c, 0, 1, 32, MAP98, buf);
    VBK_REG = 1; set_tiles(c, 0, 1, 32, MAP98, abuf); VBK_REG = 0;
}
// the course generator: called once per new row, decides whether it carries a wall
static void gen_reset(void) {
    g_seed = (uint8_t)(er_row >> 3) + 7; g_cd = 40; g_c = 10; g_corr = 0; g_thick = 0; er_next_warn = 0; er_next_c = 10;
}
static void gen_row(uint8_t r) {
    uint8_t hw = GM == MODE_CADET ? 4 : 3, d;
    rseed[r] = (uint8_t)er_row * 29 + (uint8_t)(er_row >> 8) * 7;
    rkind[r] = er_row >= ROW_END - 40 ? 2 : er_row >= ROW_SPRINT + 60 ? 1 : 0;
    er_wall[r] = 0;
    if (er_row < ROW_GAUNT + 30 || er_row >= ROW_SPRINT - 40) { er_next_warn = 0; return; }
    if (g_thick) {                                           // second row of a wall
        g_thick--; er_wall[r] = 1; er_wl[r] = g_c - hw; er_wr[r] = g_c + hw;
        return;
    }
    if (g_corr) {                                            // corridor: the gap wanders a column at a time
        g_corr--;
        if (!(g_corr & 1)) { d = grnd() % 3; if (d == 0 && g_c > hw + 2) g_c--; else if (d == 2 && g_c < 17 - hw) g_c++; }
        er_wall[r] = 1; er_wl[r] = g_c - hw - 1; er_wr[r] = g_c + hw;
        if (!g_corr) g_cd = HUNTER ? 10 : 13;
        return;
    }
    if (g_cd) {
        g_cd--;
        if (g_cd == 7) {                                     // choose the next gap early, so it can be marked
            d = grnd() % 11;                                 // shift of -5..5 columns
            g_c = g_c + d < 5 + hw + 1 ? hw + 1 : g_c + d - 5;
            if (g_c > 18 - hw) g_c = 18 - hw;
            er_next_c = g_c; er_next_warn = 1;
        }
        return;
    }
    er_next_warn = 0;
    er_wall[r] = 1; er_wl[r] = g_c - hw; er_wr[r] = g_c + hw;
    if ((grnd() & 7) < 2 && er_row < ROW_SPRINT - 80) g_corr = 10 + (grnd() & 3) * 2;
    else { g_thick = 1; g_cd = GM == MODE_CADET ? 13 : MIDMODE ? 11 : 10; }
}
static void field_fill(void) {
    uint8_t r;
    for (r = 0; r < 32; r++) { rseed[r] = r * 37 + 11; rkind[r] = er_row >= ROW_SPRINT + 60 ? 1 : 0; er_wall[r] = 0; row_draw(r); }
}

// ------------------------------------------------------------------ HUD
static const char *tag_s;
static void tag_draw(void) {
    txt_put(0, 0, tag_s, 1, TA);
}
static void tag(const char *s, uint8_t t) { tag_s = s; tag_t = t; tag_draw(); }
#define TAG_IDLE "WEAPONS OFFLINE"
static void bar_draw(void) {
    uint8_t n = (uint8_t)((er_row > ROW_END ? ROW_END : er_row) / (ROW_END / 32)), i, b[4];
    if (n == bar_last) return;
    bar_last = n;
    for (i = 0; i < 4; i++) { b[i] = ER_HUD_BASE + (n >= 8 ? 8 : n); n = n >= 8 ? n - 8 : 0; }
    set_win_tiles(15, 0, 4, 1, b);
}
static void hud_draw(void) {
    uint8_t i;
    if (HUNTER) {
        hud_buf[0] = T_HUD_SHIP; hud_buf[1] = txt_tile('x'); hud_buf[2] = txt_tile('0' + (lives > 9 ? 9 : lives));
        hud_buf[3] = txt_tile(' '); hud_buf[4] = hearts > 0 ? T_HUD_HEART : T_HUD_HEARTE; hud_buf[5] = hearts > 1 ? T_HUD_HEART : T_HUD_HEARTE;
    } else {
        for (i = 0; i < 5; i++) hud_buf[i] = i >= MAXHEARTS ? txt_tile(' ') : i < hearts ? T_HUD_HEART : T_HUD_HEARTE;
        hud_buf[5] = txt_tile('0' + lives);
    }
    hud_buf[6] = T_HUD_BOMB; hud_buf[7] = txt_tile('0' + bombs);
    hud_buf[8] = txt_tile(er_boost >= BOOST_MAX ? 'A' : ' ');
    for (i = 0; i < 4; i++) hud_buf[9 + i] = er_boost >= (i + 1) * 3 ? T_HUD_PIP : T_HUD_PIPE;
    score_tiles(hud_buf + 13);
    set_win_tiles(0, 1, 20, 1, hud_buf);
    hud_dirty = 0;
}

// ------------------------------------------------------------------ hazards
static uint8_t band_bot(void) { return (comm_on ? 32 : 0) + (comm_on && er_vis > 24 ? 24 : er_vis); }
static uint8_t muz_x(uint8_t i) { return (uint8_t)(muz[i] + sway + XO); }
static uint8_t muz_y(void) { uint8_t b = band_bot(); return (b < 12 ? 12 : b) + YO - 4; }
static void clear_shots(uint8_t pts) {
    uint8_t i;
    for (i = 0; i < MAXEB; i++) if (eb_on[i]) { eb_on[i] = 0; if (pts) add_score(10); }
    for (i = 0; i < MAXMIS; i++) if (er_mis_on[i]) { er_mis_on[i] = 0; if (pts) add_score(50); }
}
static void boom(uint8_t x, uint8_t y) { boom_at(x, y); }
static void er_hit(void) {
    if (p_inv || dash_t || st != ST_PLAY) return;
    hits_taken++; sec_clean = 0;
    shake_t = 12; hud_dirty = 1;
    if (hearts > 1) { hearts--; p_inv = 80; sfx_play(SFX_HURT); hud_dirty = 1; return; }
    hearts = 0; st = ST_DYING; st_t = 0;
    clear_shots(0);
    sfx_play(SFX_EXPL_L);
}
static void graze(uint8_t x, uint8_t y) {
    add_score(50);
    graze_t = 8; graze_x = x; graze_y = y;
    if (er_boost < BOOST_MAX) {
        er_boost += gain_tab[GM];
        if (er_boost >= BOOST_MAX) { er_boost = BOOST_MAX; tag("BOOST READY: A ", 90); sfx_play(SFX_POWER); }
    }
    sfx_play(SFX_TICK);
}
static void shots_update(void) {
    uint8_t i, x, y, hb = HUNTER ? 2 : 3;
    for (i = 0; i < MAXEB; i++) {
        if (!eb_on[i]) continue;
        eb_x[i] += eb_vx[i]; eb_y[i] += eb_vy[i];
        x = eb_x[i] >> 8; y = eb_y[i] >> 8;
        if (x < 10 || x > 180 || y < 22 || y > FIELD_BOT + YO - 6) { eb_on[i] = 0; continue; }
        if (pulse_t && NEAR(x, pulse_x, 44) && NEAR(y, pulse_y, 44)) { eb_on[i] = 0; add_score(10); continue; }
        if (st != ST_PLAY) continue;
        if (NEAR(x + 3, px, 2 + hb) && NEAR(y + 3, py, 2 + hb)) { eb_on[i] = 0; er_hit(); continue; }
        if (eb_on[i] == 1 && !p_inv && NEAR(x + 3, px, 12) && NEAR(y + 3, py, 12)) { eb_on[i] = 3; graze(x, y); }
    }
}
static void mis_fire(uint8_t x, uint8_t y) {
    uint8_t i;
    for (i = 0; i < (HUNTER ? MAXMIS : 2); i++) if (!er_mis_on[i]) {
        er_mis_on[i] = 1; er_mis_d[i] = 0; er_mis_t[i] = 0;
        er_mis_x[i] = (uint16_t)x << 8; er_mis_y[i] = (uint16_t)y << 8;
        sfx_play(SFX_WHOOSH);
        return;
    }
}
static void mis_update(void) {
    uint8_t i, x, y, d, a, spd = GM == MODE_CADET ? 20 : MIDMODE ? 24 : 29;
    for (i = 0; i < MAXMIS; i++) {
        if (!er_mis_on[i]) continue;
        x = er_mis_x[i] >> 8; y = er_mis_y[i] >> 8;
        d = er_mis_d[i];
        if (++er_mis_t[i] < (HUNTER ? 150 : 110) && (er_mis_t[i] & (HUNTER ? 3 : 7)) == 0) {   // steer for a while, then fly straight
            a = (aim(x, y, px, py) - d) & 31;
            if (a && a < 16) d = (d + 1) & 31; else if (a) d = (d - 1) & 31;
            er_mis_d[i] = d;
        }
        er_mis_x[i] += ((int16_t)dsin[d] * spd) >> 2; er_mis_y[i] += ((int16_t)DCOS(d) * spd) >> 2;
        x = er_mis_x[i] >> 8; y = er_mis_y[i] >> 8;
        if (x < 8 || x > 184 || y < 20 || y > FIELD_BOT + YO || er_mis_t[i] > 250) { er_mis_on[i] = 0; continue; }
        if (pulse_t && NEAR(x, pulse_x, 48) && NEAR(y, pulse_y, 48)) { er_mis_on[i] = 0; boom(x, y); add_score(50); continue; }
        if (st != ST_PLAY) continue;
        if (NEAR(x, px, 6) && NEAR(y, py, 7)) { er_mis_on[i] = 0; boom(x, y); er_hit(); continue; }
        if (er_mis_on[i] == 1 && !p_inv && NEAR(x, px, 15) && NEAR(y, py, 15)) { er_mis_on[i] = 3; graze(x, y); }
    }
}
static void rock_spawn(uint8_t x) {
    uint8_t i;
    for (i = 0; i < MAXRK; i++) if (!er_rk_on[i]) {
        er_rk_on[i] = 1; er_rk_x[i] = x; er_rk_t[i] = GM == MODE_CADET ? 50 : MIDMODE ? 44 : 36; er_rk_y[i] = (uint16_t)(FIELD_BOT + YO + 6) << 8;
        return;
    }
}
static void rock_update(uint8_t step) {
    uint8_t i, y;
    for (i = 0; i < MAXRK; i++) {
        if (!er_rk_on[i]) continue;
        if (er_rk_on[i] == 1) { if (!--er_rk_t[i]) er_rk_on[i] = 2; continue; }
        er_rk_y[i] -= ((uint16_t)step << 8) + 96;            // they hang in space, so they come up at scroll speed and a bit
        y = er_rk_y[i] >> 8;
        if (y < band_bot() + YO - 4) { er_rk_on[i] = 0; continue; }
        if (st == ST_PLAY && NEAR(er_rk_x[i], px, 8) && NEAR(y, py, 8)) er_hit();
    }
}
// lasers: BG columns. Placing and clearing take a frame per column; warning and firing are one palette.
// A laser never takes the middle of a gap: the centre lane of every wall on the map stays free,
// so walls and lasers together always leave a way through.
static uint8_t lane_free(uint8_t c) {
    uint8_t r, m;
    if (er_next_warn && NEAR(c, er_next_c, 2)) return 0;
    for (r = 0; r < 32; r++) if (er_wall[r]) {
        m = (er_wl[r] + er_wr[r] + 1) >> 1;
        if (NEAR(c, m, 2)) return 0;
    }
    return 1;
}
static void laser_start(void) {
    uint8_t n = GM == MODE_CADET ? 2 : MIDMODE ? 3 : 4, i, c, j, ok, tries = 0;
    if (er_las_state || er_row >= ROW_BREAK - 30) return;
    c = (uint8_t)(px - XO) >> 3; if (c > 19) c = 19;         // one tracks the ship, unless the ship sits in a gap lane
    er_las_n = 0;
    if (lane_free(c)) er_las_col[er_las_n++] = c;
    for (i = 1; i < n; i++) {
        do {
            c = 1 + rnd() % 18; ok = lane_free(c);
            for (j = 0; j < er_las_n; j++) if (NEAR(c, er_las_col[j], 2)) ok = 0;
        } while (!ok && ++tries < 24);
        if (ok) er_las_col[er_las_n++] = c;
    }
    if (!er_las_n) return;
    er_las_state = 1; las_i = 0; las_t = 0;
    laser_pal(0);
    vis_t = 48;
    tag("LASERS! MOVE!  ", 100);
    sfx_play(SFX_ALARM);
}
static void laser_update(void) {
    uint8_t i;
    switch (er_las_state) {
    case 1:
        las_i++; col_draw(er_las_col[las_i - 1]);
        if (las_i >= er_las_n) { er_las_state = 2; las_t = GM == MODE_CADET ? 80 : MIDMODE ? 68 : 58; }
        break;
    case 2:
        if ((las_t & 7) == 0) laser_pal((las_t >> 3) & 1);
        if (!--las_t) { er_las_state = 3; las_t = 36; laser_pal(2); sfx_play(SFX_THUNDER); }
        break;
    case 3:
        if (st == ST_PLAY) for (i = 0; i < er_las_n; i++) if (NEAR(er_las_col[i] * 8 + 4 + XO, px, 6)) er_hit();
        if (!--las_t) { er_las_state = 4; las_i = 0; }
        break;
    case 4:
        col_draw(er_las_col[las_i]);
        if (++las_i >= er_las_n) { er_las_state = 0; vis_t = phase == 1 ? 16 : 40; }
        break;
    }
}
// attack patterns
enum { A_FAN, A_CURTAIN, A_ROCK, A_MISSILE, A_RAIN, A_RING, A_LASER, A_END };
static const uint8_t atk_chase[] = {A_FAN, A_CURTAIN, A_ROCK, A_FAN, A_MISSILE, A_CURTAIN, A_RAIN, A_ROCK, A_MISSILE, A_RING, A_END};
static const uint8_t atk_gaunt[] = {A_LASER, A_FAN, A_LASER, A_MISSILE, A_END};
static const uint8_t atk_sprint[] = {A_CURTAIN, A_MISSILE, A_LASER, A_RAIN, A_ROCK, A_FAN, A_RING, A_ROCK, A_LASER, A_CURTAIN, A_END};
static const uint8_t * const atk_tab[3] = {atk_chase, atk_gaunt, atk_sprint};
static void attack(uint8_t a) {
    uint8_t i, m, d, sp = bspd_tab[GM], gap, x;
    switch (a) {
    case A_FAN:
        m = px < XO + 80 ? (rnd() & 1) : 2 + (rnd() & 1);
        d = aim(muz_x(m), muz_y(), px, py);
        eb_fire(muz_x(m), muz_y(), d, sp + 3, 0);
        if (GM != MODE_CADET) { eb_fire(muz_x(m), muz_y(), d + 3, sp + 3, 0); eb_fire(muz_x(m), muz_y(), d - 3, sp + 3, 0); }
        if (HUNTER) eb_fire(muz_x(4), muz_y() + 6, aim(muz_x(4), muz_y() + 6, px, py), sp, 1);
        sfx_play(SFX_SPIT);
        break;
    case A_CURTAIN:                                          // a row of shots with one gap, falling together
        gap = 1 + rnd() % 6;
        for (i = 0; i < 8; i++) {
            if (i == gap || i == gap + 1 || (!HUNTER && i + 1 == gap)) continue;
            x = XO + 10 + i * 20;
            eb_fire(x, muz_y(), 0, sp - 4, 0);
        }
        sfx_play(SFX_SPIT);
        break;
    case A_ROCK:
        if (GM == MODE_CADET) rock_spawn(XO + 16 + rnd() % 128);
        else rock_spawn((uint8_t)(px + 40) > XO + 150 ? px - 40 : px + 40);
        if (HUNTER) rock_spawn(px);
        break;
    case A_MISSILE:
        mis_fire(muz_x(4), muz_y() + 4);
        if (HUNTER) mis_fire(muz_x(0), muz_y());
        break;
    case A_RAIN:                                             // crossing diagonals from the outer cannons
        burst_n = GM == MODE_CADET ? 4 : MIDMODE ? 6 : 8; burst_t = 0; burst_k = 0;
        break;
    case A_RING:
        if (GM == MODE_CADET) { attack(A_FAN); break; }
        for (i = 0; i < 7; i++) if (HUNTER || (i & 1) == 0) eb_fire(muz_x(4), muz_y() + 4, (uint8_t)(26 + i * 2), sp, 0);
        sfx_play(SFX_SPIT);
        break;
    case A_LASER:
        laser_start();
        break;
    }
}
static void attacks_update(void) {
    const uint8_t *tab;
    if (burst_n && !burst_t--) {
        burst_t = HUNTER ? 9 : 13; burst_n--;
        if (burst_k ^= 1) eb_fire(muz_x(0), muz_y(), 3 + (burst_n & 1), bspd_tab[GM], 0);
        else eb_fire(muz_x(3), muz_y(), 29 - (burst_n & 1), bspd_tab[GM], 0);
        sfx_play(SFX_SPIT);
    }
    if (er_row >= ROW_BREAK) {                                // the flagship is gone: only burning debris on the way down
        if (atk_cd) atk_cd--;
        else if (er_row < ROW_END - 50) { attack(A_ROCK); atk_cd = GM == MODE_CADET ? 70 : MIDMODE ? 55 : 40; }
        return;
    }
    if (er_row >= ROW_BREAK - 20 || er_vis < 12) return;      // about to pull up, or not in yet
    if (atk_cd) { atk_cd--; return; }
    tab = atk_tab[phase];
    if (tab[atk_i] == A_END) atk_i = 0;
    attack(tab[atk_i++]);
    atk_cd = cd_tab[GM][phase];
}

// ------------------------------------------------------------------ player
#define PX_MIN (XO + 8)
#define PX_MAX (XO + 152)
#define PY_MAX (YO + FIELD_BOT - 12)
static void wall_check(void) {
    uint8_t r = ((uint8_t)(er_fscy + py - YO) >> 3) & 31, c0, c1;
    if (!er_wall[r]) return;
    c0 = (uint8_t)(px - XO - 4) >> 3; c1 = (uint8_t)(px - XO + 3) >> 3;
    if (c0 < er_wl[r] || c1 > er_wr[r]) er_hit();
}
static void player_update(void) {
    uint8_t ymin = band_bot() + YO + 10, sp = 2;
    int8_t dx = 0, dy = 0;
    if (joy & J_LEFT) dx = -1; else if (joy & J_RIGHT) dx = 1;
    if (joy & J_UP) dy = -1; else if (joy & J_DOWN) dy = 1;
    if (dx) last_dx = dx;
    if (dash_t) {
        dash_t--;
        trail_x[2] = trail_x[1]; trail_y[2] = trail_y[1]; trail_x[1] = trail_x[0]; trail_y[1] = trail_y[0]; trail_x[0] = px; trail_y[0] = py;
        dx = dash_dx; dy = dash_dy; sp = 1;
    } else if ((joy_new & J_A) && er_boost >= BOOST_MAX) {
        er_boost = 0; dash_t = 12; p_inv = 30; hud_dirty = 1;
        dash_dx = dx * 4; dash_dy = dy * 4;
        if (!dx && !dy) dash_dx = last_dx * 4;
        trail_x[0] = trail_x[1] = trail_x[2] = px; trail_y[0] = trail_y[1] = trail_y[2] = py;
        add_score(100);
        sfx_play(SFX_WHOOSH);
    }
    if ((joy_new & J_B) && bombs && !pulse_t) {               // shield pulse
        bombs--; pulse_t = 36; pulse_x = px; pulse_y = py; if (p_inv < 50) p_inv = 50; hud_dirty = 1;
        tag("SHIELD PULSE   ", 60);
        sfx_play(SFX_BOMB);
    }
    if (dx < 0) px = px > PX_MIN - dx * sp ? px + dx * sp : PX_MIN;
    else if (dx > 0) px = px < PX_MAX - dx * sp ? px + dx * sp : PX_MAX;
    if (dy < 0) py = py > ymin - dy * sp ? py + dy * sp : ymin;
    else if (dy > 0) py = py < PY_MAX - dy * sp ? py + dy * sp : PY_MAX;
    if (py < ymin) py = ymin;                                // the flagship slid in on top of us
    wall_check();
    if (er_pod == 1 && NEAR(er_pod_x, px, 13) && NEAR(er_pod_y, py, 13)) {
        er_pod = 2; add_score(4000);
        tag("BRIG POD ABOARD", 150);
        comm_queue_add(CM6_GOTPOD);
        sfx_play(SFX_PICKUP);
    }
}

// ------------------------------------------------------------------ drawing
static void spr16(uint8_t x, uint8_t y, uint8_t tile, uint8_t prop) {     // centre, object coords
    oam_add(x - 16, y - 24, tile, prop); oam_add(x - 8, y - 24, tile + 2, prop);
}
static const uint8_t mis_tile[8] = {ER_ST_MISSILE, ER_ST_MISSILE_D, ER_ST_MISSILE_S, ER_ST_MISSILE_D, ER_ST_MISSILE, ER_ST_MISSILE_D, ER_ST_MISSILE_S, ER_ST_MISSILE_D};
static const uint8_t mis_flip[8] = {0, 0, 0, 0x40, 0x40, 0x60, 0x20, 0x20};
static void draw_all(void) {
    uint8_t i, k, x, y, ox = px - 16, oy = py - 24, bb = band_bot();
    flame_t++;
    if (st == ST_DYING || (p_inv && !dash_t && (p_inv & 4)) || (st == ST_CLEAR && st_t > 150)) { for (i = 0; i < 4; i++) oam_fixed(i, 0, 0, 0, 0); }
    else {
        // the ship points down: body flipped, flame above it
        oam_fixed(0, ox, oy, ST_SHIP_TOP, 0x40); oam_fixed(1, ox + 8, oy, ST_SHIP_TOP + 2, 0x40);
        k = (flame_t & 4) ? ST_SHIP_BOT : ST_SHIP_BOTB;
        oam_fixed(2, ox, oy - 16, k, 1 | 0x40); oam_fixed(3, ox + 8, oy - 16, k + 2, 1 | 0x40);
    }
    oam_begin();
    if (dash_t) for (i = 0; i < 3; i++) if ((flame_t + i) & 1) { oam_add(trail_x[i] - 16, trail_y[i] - 24, ST_SHIP_TOP, 2 | 0x40); oam_add(trail_x[i] - 8, trail_y[i] - 24, ST_SHIP_TOP + 2, 2 | 0x40); }
    if (pulse_t) {                                           // shield pulse: a ring of cool sparks opening out
        k = (36 - pulse_t) + 6;
        for (i = 0; i < 8; i++) {
            x = pulse_x + (int8_t)(((int16_t)sin64[i * 8] * k) >> 6); y = pulse_y + (int8_t)(((int16_t)COS64(i * 8) * k) >> 6);
            if (y > bb + YO && y < FIELD_BOT + YO - 4) oam_add(x - 12, y - 24, ER_ST_PULSE, 6 | 8);
        }
    }
    if (graze_t) { graze_t--; oam_add(graze_x - 8, graze_y - 20, ER_ST_GRAZE, 6 | 8); }
    if (er_pod == 1) spr16(er_pod_x, er_pod_y, ER_ST_POD, 6 | 8);
    // markers: next gap, incoming rocks, laser columns
    if (er_next_warn && (flame_t & 8)) oam_add(er_next_c * 8 + 8, FIELD_BOT - 2, ER_ST_CHEV, 6 | 8);
    for (i = 0; i < MAXRK; i++) {
        if (er_rk_on[i] == 1) { if (flame_t & 4) oam_add(er_rk_x[i] - 12, FIELD_BOT - 2, ST_WARN, 7); }
        else if (er_rk_on[i]) spr16(er_rk_x[i], er_rk_y[i] >> 8, (i & 1) ? ER_ST_ROCK_B : ER_ST_ROCK_A, 5 | 8);
    }
    if (er_las_state == 2 && (flame_t & 4)) for (i = 0; i < er_las_n; i++) oam_add(er_las_col[i] * 8 + 8, bb + 18, ST_WARN, 7);
    for (i = 0; i < MAXMIS; i++) if (er_mis_on[i]) {
        k = ((er_mis_d[i] + 2) >> 2) & 7;
        oam_add((er_mis_x[i] >> 8) - 12, (er_mis_y[i] >> 8) - 24 - ((mis_flip[k] & 0x40) && (k & 3) ? 8 : 0) + ((k & 3) && !(mis_flip[k] & 0x40) ? 4 : 0),
                k ? mis_tile[k] : ((flame_t & 4) ? ER_ST_MISSILE : ER_ST_MISSILE2), 3 | 8 | mis_flip[k]);
    }
    for (i = 0; i < MAXEB; i++) if (eb_on[i]) oam_add((eb_x[i] >> 8) - 8, (eb_y[i] >> 8) - 16, eb_big[i] ? ST_EB_INK : ST_EB_ORB, 3);
    for (i = 0; i < MAXE; i++) if (e_type[i] == E_BOOM) {     // boom_at() explosions
        if (++e_t[i] >= 20) { e_type[i] = E_NONE; continue; }
        spr16(e_x[i] >> 8, e_y[i] >> 8, ST_EXPL0 + (e_t[i] / 5) * 4, 7);
    }
    // speed streaks (and heat in the atmosphere): cosmetic, dropped first when a line is full
    k = phase == 2 ? 5 : 3;
    for (i = 0; i < k; i++) {
        y = (uint8_t)(200 - (uint8_t)(er_t * (5 + i) + i * 61)) % (FIELD_BOT - 8);
        if (y < bb) continue;                                 // the band has its own stars below
        x = (uint8_t)(i * 53 + ((er_t * (5 + i) + i * 61) >> 8) * 37) % 152;
        if (st == ST_CLEAR || (atmo_k >= 2 && (i & 1))) oam_add_fx(px - 24 + i * 6, py - 40 - (flame_t & 7) * 2, ER_ST_HEAT, 6 | 8);
        else oam_add_fx(x + 8, y + 8, ER_ST_STREAK, 6 | 8 | 0x80);
    }
    k = comm_on ? 32 : 0;                                     // first sky line of the band
    if (bb >= k + 16 && st != ST_CLEAR) {
        for (i = 0; i < NBST; i++) {
            y = bst_y[i] - last_step;
            if (y < k + 8 || y >= bb + 8) { y = bb + (i & 7); bst_x[i] = (rnd() & 127) + (rnd() & 15) + 4; }
            bst_y[i] = y;
            if (y >= k + 8) oam_add_fx(bst_x[i] + 8, y + 8, ER_ST_STREAK, 6 | 8 | 0x80);
        }
    }
    oam_end();
}
static void raster(void) {
    uint8_t c0 = comm_on ? 32 : 0, v = comm_on && er_vis > 24 ? 24 : er_vis, sh = 0;
    if (shake_t) { shake_t--; sh = shake_t & 2; }
    if (comm_on) {
        n_lcdc = LCDC_COMM; n_scx = 0; n_scy = 16;
        if (v >= 8) {
            n_s1_line = 32; n_s1_lcdc = LCDC_COMM; n_s1_scx = (uint8_t)(-sway); n_s1_scy = (uint8_t)(PUR_ROW * 8 + 56 - v - c0);
            n_s2_line = 32 + v; n_s2_lcdc = LCDC_PLAY; n_s2_scx = wob; n_s2_scy = er_fscy + sh;
        } else { n_s1_line = 32; n_s1_lcdc = LCDC_PLAY; n_s1_scx = wob; n_s1_scy = er_fscy + sh; n_s2_line = 0; }
    } else if (v >= 8) {
        n_lcdc = LCDC_COMM; n_scx = (uint8_t)(-sway); n_scy = (uint8_t)(PUR_ROW * 8 + 56 - v);
        n_s1_line = v; n_s1_lcdc = LCDC_PLAY; n_s1_scx = wob; n_s1_scy = er_fscy + sh; n_s2_line = 0;
    } else { n_lcdc = LCDC_PLAY; n_scx = wob; n_scy = er_fscy + sh; n_s1_line = 0; n_s2_line = 0; }
}

// ------------------------------------------------------------------ setup
static void er_load(void) {
    uint8_t r;
    screen_off(); splits_off(); oam_clear();
    VBK_REG = 0;
    ld_spr_data(BANK_A2, 0, N_SPR_TILES, spr_tiles);
    VBK_REG = 1; ld_spr_data(BANK_ER, 0, ER_SPR_NT, er_spr); VBK_REG = 0;
    ld_spr_pal(BANK_A2, 0, 3, obj_pals);
    ld_spr_pal(BANK_A2, 7, 1, obj_pals + 28);
    ld_spr_pal(BANK_ER, 3, 4, er_objpal);
    ld_bkg_data(BANK_A2, 0, 0, N_COMMON, common_tiles);
    ld_bkg_data(BANK_ER, 0, N_COMMON, ER_FIELD_NT, er_field_tiles);
    ld_bkg_data(BANK_ER, 1, 0, ER_PUR_NT, er_pur_tiles);
    ld_bkg_data(BANK_ER, 1, ER_HUD_BASE, ER_HUD_NT, er_hud_tiles);
    ld_copy(BANK_ER, muz, er_muz_x, 5);
    set_pal_slot(5, BP_HUD);
    ld_bkg_pal(BANK_ER, 7, 1, er_tagpal);
    // 9C00 map: rows 0-1 HUD, 2-5 comm box (engine), 8-14 the flagship, the rest empty space
    memset(buf, ER_T_BLANK, 32); memset(abuf, 0, 32);
    for (r = 6; r < 32; r++) { set_tiles(0, r, 32, 1, MAP9C, buf); VBK_REG = 1; set_tiles(0, r, 32, 1, MAP9C, abuf); VBK_REG = 0; }
    for (r = 0; r < ER_PUR_H; r++) {
        ld_copy(BANK_ER, buf, er_pur_map + r * 20, 20); set_tiles(0, PUR_ROW + r, 20, 1, MAP9C, buf);
        ld_copy(BANK_ER, buf, er_pur_attr + r * 20, 20); VBK_REG = 1; set_tiles(0, PUR_ROW + r, 20, 1, MAP9C, buf); VBK_REG = 0;
    }
    move_win(7, FIELD_BOT);
}
#define ROW_CP3 (ROW_SPRINT + 560)       // halfway down the dive: a continue starts here, not at the top of it
static const uint16_t cp_row[4] = {0, ROW_GAUNT, ROW_SPRINT, ROW_CP3};
static void restart_from(uint8_t c) {
    screen_off(); splits_off();
    memset(e_type, 0, sizeof e_type); memset(eb_on, 0, sizeof eb_on);
    memset(er_mis_on, 0, sizeof er_mis_on); memset(er_rk_on, 0, sizeof er_rk_on);
    comm_reset(); was_comm = 0;
    boss_clear(); k_hpmax = 1;                                // crew lines never wait for a quiet moment here
    cp = c; er_row = cp_row[c]; phase = c > 2 ? 2 : c;
    er_las_state = 0; er_las_n = 0; er_pod = 0; pod_ok = 0;
    er_fscy = 0; acc = 0; er_t = 0; wob = 0;
    atmo_k = c == 3 ? 2 : c == 2 ? 1 : 0; glow_k = 0;
    sky_set(er_atmo + atmo_k * 4);
    gen_reset(); field_fill();
    er_vis = 0; vis_t = c == 1 ? 16 : 40; sway_t = 0; sway = 0;
    atk_i = 0; atk_cd = 150; burst_n = 0;
    px = XO + 80; py = YO + 84; p_inv = 100; dash_t = 0; pulse_t = 0; shake_t = 0; graze_t = 0; last_dx = 1;
    er_boost = 0; sec_clean = 1; bar_last = 0xFF;
    { uint8_t i; for (i = 0; i < NBST; i++) { bst_x[i] = rnd() % 156; bst_y[i] = 16 + i * 7; } }
    st = ST_PLAY; st_t = 0;
    txt_fill(0, 0, 20, 2, txt_tile(' '), HA, 1);
    VBK_REG = 1; memset(buf, 5 | 0x08 | 0x80, 5); set_win_tiles(15, 0, 5, 1, buf); VBK_REG = 0;
    set_win_tile_xy(19, 0, ER_HUD_BASE + 9);
    tag(TAG_IDLE, 0); bar_draw(); hud_draw();
    raster();
    music_play(c >= 2 ? MUS_FLATWOODS : MUS_BOSS);
    oam_clear();
    screen_on(n_lcdc);
    if (c == 0) { comm_queue_add(CM6_EMP); comm_queue_add(CM6_GRAZE); }
    else if (c == 1) comm_queue_add(CM6_ROCKS);
    else comm_queue_add(CM6_DIVE);
}
static uint8_t continue_prompt(void) {
    uint8_t t = 9, f;
    comm_reset(); was_comm = 0;
    oam_clear();
    music_play(MUS_OVER);
    txt_fill(0, 0, 20, 2, txt_tile(' '), HA, 1);
    txt_put(5, 0, "GAME OVER", 1, HA);
    wait_frames(110);
    f = 0;
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
// one row further down the course
static void row_step(void) {
    uint8_t r = ((uint8_t)(er_fscy + FIELD_BOT) >> 3) & 31;
    er_row++;
    gen_row(r); row_draw(r);
    if ((er_row & 7) == 0) add_score(80);
    if ((er_row & 255) == 0) {                               // a section without a scratch pays
        if (sec_clean) { add_score(3000); tag("CLEAN RUN +3000", 90); sfx_play(SFX_PICKUP); }
        sec_clean = 1;
    }
    if (er_row == ROW_GAUNT) { phase = 1; cp = 1; atk_i = 0; atk_cd = 200; vis_t = 16; comm_queue_add(CM6_ROCKS); music_play(MUS_STAGE); }
    if (er_row == ROW_GAUNT + 120) comm_queue_add(CM6_LASER);
    if (er_row == 180) comm_queue_add(CM6_MISSILE);
    if (er_row == ROW_SPRINT) { phase = 2; cp = 2; atk_i = 0; atk_cd = 160; vis_t = 40; comm_queue_add(CM6_DIVE); music_play(MUS_FLATWOODS); }
    if (er_row == ROW_SPRINT + 60) { atmo_k = 1; sky_set(er_atmo + 4); }
    if (er_row == ROW_CP3) cp = 3;
    if (er_row == ROW_SPRINT + 420) { atmo_k = 2; sky_set(er_atmo + 8); comm_queue_add(CM6_HEAT); }
    if (er_row == ROW_SPRINT + 800) { atmo_k = 3; sky_set(er_atmo + 12); }
    if (er_row == ROW_POD) {
        er_pod = 1; pod_ok = 1;
        er_pod_x = (px < XO + 80) ? (uint8_t)(XO + 136) : (uint8_t)(XO + 24);
        er_pod_y = (uint8_t)(muz_y() + 8);
        comm_queue_add(CM6_POD); tag("GRAB THE POD!  ", 200);
    }
    if (er_row == ROW_BREAK) { vis_t = 0; comm_queue_add(CM6_CLEAR); }
}

// ------------------------------------------------------------------ main loop
uint8_t escape_run(void) BANKED {
    uint8_t paused = 0, sec_t = 0, step, spd;
    stage_id = ER_STAGE;
    er_load();
    score = 0; hits_taken = 0; continues_used = 0; play_seconds = 0; midboss_killed = 0; kill_count = 0;
    lives = 3 + g_gear[GEAR_HULL]; hearts = ER_HEARTS; bombs = (HUNTER ? 3 : 5) + g_gear[GEAR_BOMBS]; wtype = 0; wlevel = 0;
    bounty_pts = 0; bounty_out = OUT_NONE; mid_out = OUT_NONE;
    next_extend = 30000UL;
    restart_from(start_cp == 2 ? 2 : 0);
    while (1) {
        wait_vbl_done();
        input_poll();
        if ((joy_new & J_START) && st == ST_PLAY) {
            paused ^= 1;
            sfx_play(SFX_SELECT);
            if (paused) txt_put(13, 1, "PAUSE  ", 1, HA); else hud_dirty = 1;
        }
        if (paused) continue;
        er_loops++; er_t++;
        if (++sec_t >= 60) { sec_t = 0; play_seconds++; }
        comm_update();

        // scroll
        spd = spd_tab[GM][phase];
        if (st == ST_CLEAR) spd = st_t < 60 ? spd + (st_t >> 1) : 80;
        acc += spd; step = acc >> 4; acc &= 15; last_step = step;
        { uint8_t n = step; while (n--) { er_fscy++; if ((er_fscy & 7) == 0 && er_row < ROW_END + 400) row_step(); } }

        // the flagship slides and sways
        if (!(er_t & 1)) {
            if (er_vis < vis_t) er_vis = er_vis < 8 ? 8 : er_vis + 1;
            else if (er_vis > vis_t) er_vis = er_vis <= 8 ? 0 : er_vis - 1;
        }
        if (!(er_t & 3)) { sway_t++; sway = sin64[sway_t & 63] >> 3; }

        if (st == ST_PLAY) {
            player_update();
            if (er_row < ROW_END) attacks_update();
        }
        if (p_inv) p_inv--;
        if (pulse_t) pulse_t--;
        laser_update();
        shots_update(); mis_update(); rock_update(step);
        if (er_pod == 1) {
            if (er_t & 1) er_pod_y++;
            if ((er_t & 7) == 0) {                          // drifts off the safe line, not toward the ship
                if (er_pod_x < XO + 80) { if (er_pod_x > XO + 12) er_pod_x--; }
                else if (er_pod_x < XO + 148) er_pod_x++;
            }
            if (er_pod_y > FIELD_BOT + YO - 8) { er_pod = 3; tag("POD LOST...    ", 120); }
        }

        if (st == ST_DYING) {
            st_t++;
            shake_t = 4;
            if ((st_t & 7) == 1) { boom(px - 8 + (rnd() & 15), py - 8 + (rnd() & 15)); sfx_play(SFX_EXPL_S); }
            if (st_t == 70) {
                if (lives > 1) {
                    lives--; hearts = ER_HEARTS;
                    if (bombs < 3 + g_gear[GEAR_BOMBS]) bombs = 3 + g_gear[GEAR_BOMBS];
                    p_inv = 150; st = ST_PLAY; hud_dirty = 1;
                } else {
                    if (!continue_prompt()) { k_hpmax = 0; return RES_GAMEOVER; }
                    continues_used++;
                    if (HUNTER) { score = 0; bounty_pts = 0; } else bounty_pts >>= 1;
                    lives = 3 + g_gear[GEAR_HULL]; hearts = ER_HEARTS; bombs = (HUNTER ? 3 : 5) + g_gear[GEAR_BOMBS];
                    restart_from(cp);
                    continue;
                }
            }
        }
        if (er_row >= ROW_END && st == ST_PLAY) {             // through the cloud tops: we made it
            st = ST_CLEAR; st_t = 0;
            bounty_out = er_pod == 2 ? OUT_CAPTURE : OUT_FLEE;
            clear_shots(1); memset(er_rk_on, 0, sizeof er_rk_on);
            tag("ENTERING ATMO  ", 0);
            music_play(MUS_CLEAR);
        }
        if (st == ST_CLEAR) {
            // the dive: the ship steadies, the air shakes it, and a soft haze rises and fades (no flashes)
            st_t++;
            if (px < XO + 80) px++; else if (px > XO + 80) px--;
            if (py < YO + 84) py++;
            wob = st_t < 150 ? (uint8_t)(sin64[(st_t * 5) & 63] >> 5) : 0;
            if (st_t >= 60 && st_t < 60 + ER_GLOW_N * 20 && (st_t - 60) % 20 == 0) {
                glow_k = (st_t - 60) / 20;
                sky_set(er_glow + glow_k * 4);
            }
            if (st_t >= 60 + ER_GLOW_N * 20 + 10) { oam_clear(); k_hpmax = 0; return RES_CLEAR; }
        }

        if (tag_t && !--tag_t) { tag_s = TAG_IDLE; tag_draw(); }
        if (hud_dirty) hud_draw();
        if ((er_t & 15) == 0) bar_draw();
        raster();
        draw_all();
    }
}
