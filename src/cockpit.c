// Mission 5 "Flatwoods Drift": first-person gun-cam stage (bank 25).
// The helm is jammed, so the ship flies itself and the player works the cannons:
// the D-pad moves a crosshair, shots are instant, and anything fired at the ship
// has to be shot down before it lands. Uses the same lives / hearts / bombs /
// gear / bounty rules as the scrolling missions, but none of their engine.
//
// Screen: lines 0-15 canopy arch (9C00 map rows 8-9 shown as BG), lines 16-95 the
// view (9800 map, scrolled), lines 96-143 the dashboard (window rows 0-5):
// row 0 rim / boss bar, row 1 HUD, rows 2-5 radar + bounty panel or the crew comm.
#pragma bank 25
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "assets.h"
#include "cockpit_data.h"
#include "script_ids.h"
#include "sound.h"
#include "game.h"
#include "objs.h"
#include "weapon_fx2.h"

extern uint8_t bud_n;
extern const uint8_t boss_out_comm[];

#define VCX 80
#define VCY 56
#define BOSS_CY 48                      // the boss floats a little above the view centre, clear of the planet
#define HA (5 | 0x80)                   // HUD text attribute
#define MAP98 ((uint8_t *)0x9800)
#define MAP9C ((uint8_t *)0x9C00)
#define CK_STAGE 4                      // mission index (0-based)

enum { O_NONE, O_UFO, O_ELITE, O_ROCK, O_ORB, O_BIGORB, O_CAPS, O_BOOM };
#define CK_MAXO 12
// View coordinates are 8-bit and biased by 48 (screen x + 48, screen y + 48), so objects can
// sit a little off-screen and every test stays in cheap unsigned bytes.
#define VB 48
#define VCXB (VCX + VB)
#define VCYB (VCY + VB)
// object pool (global so the test bot can read it)
uint8_t ck_type[CK_MAXO], ck_z[CK_MAXO], ck_hp[CK_MAXO], ck_t[CK_MAXO], ck_st[CK_MAXO], ck_a[CK_MAXO], ck_b[CK_MAXO], ck_flash[CK_MAXO];
int8_t ck_wx[CK_MAXO], ck_wy[CK_MAXO];           // saucers and rocks: sideways position in space
uint8_t ck_sx[CK_MAXO], ck_sy[CK_MAXO];          // centre, view coordinates
uint8_t ck_hx[CK_MAXO], ck_hy[CK_MAXO], ck_ft[CK_MAXO];   // saucers: hover point and frames to the next shot
uint16_t ck_fx[CK_MAXO], ck_fy[CK_MAXO];         // orbs and capsules: 8.8 position
int16_t ck_vx[CK_MAXO], ck_vy[CK_MAXO];
uint8_t ck_cx, ck_cy;                            // crosshair (screen px)
int8_t ck_bx, ck_by;                             // boss offset from the view centre
uint8_t ck_charge;                               // beam charge frames left (0 = none)
uint16_t ck_loops;                               // main-loop passes (tests compare it with frames to spot slowdown)
uint8_t ck_tick, ck_pc;                          // wave script clock (32-frame ticks) and position

static uint8_t ck_sub, ck_cp, cp_pc[4], cp_tick[4];        // checkpoints: start, debris, push, boss
static uint8_t p_inv, fire_cd, bolt_t, spark_t, hold_t, flash_t, shake_t, bomb_t, lock;
static uint8_t ck_pop, ck_wpal, spark_k, spark_x, spark_y;     // weapon flair: name pop, palette shown, impact kind + spot
static const char *ck_pop_s;
static uint8_t was_comm, panel_bar, panel_rate, alert, alert_last, bar_last, bud_cd;
static uint16_t star_x[6], star_y[6];            // 8.8, view coordinates
static int16_t star_vx[6], star_vy[6];
static uint8_t star_age[6];
static uint8_t b_pose, b_t, b_atk, b_at, b_n, b_ph, b_phase2, b_fade, b_seq, b_stag, b_hint;
static int8_t parx, pary;
static uint8_t b_dmg;
static uint8_t buf[32];

// perspective scale: 4096 / z, indexed by z >> 1 (64 = life size at z 64)
static const uint8_t sc_tab[128] = {
    255,255,255,255,255,255,255,255,240,215,195,178,163,151,141,132,124,117,110,105,99,95,91,87,83,80,77,74,71,69,67,65,
    63,61,59,57,56,54,53,51,50,49,48,47,46,45,44,43,42,41,40,39,39,38,37,36,36,35,35,34,33,33,32,32,
    31,31,30,30,29,29,29,28,28,27,27,27,26,26,26,25,25,25,24,24,24,23,23,23,23,22,22,22,22,21,21,21,
    21,21,20,20,20,20,19,19,19,19,19,19,18,18,18,18,18,18,17,17,17,17,17,17,16,16,16,16,16,16,16,16};

// bounty thresholds [0 cadet/pilot, 1 hunter]
static const uint32_t ck_cap_thr[2] = {46000UL, 58000UL};
static const uint32_t ck_kill_thr[2] = {36000UL, 46000UL};

// ------------------------------------------------------------------ wave script
// {tick, type, x, y, p}. Saucers: p bits 0-1 drop (1 weapon, 2 bomb, 3 heart / 1UP), bit 2 strafes.
// Rocks: p bits 0-1 speed. Any: bit 7 Hunter only, bit 6 not in Cadet.
#define W_COMM 0xF0
#define W_CP   0xF1
#define W_END  0xFF
#define HN 0x80
#define NC 0x40
typedef struct { uint8_t t, type; int8_t x, y; uint8_t p; } Wave;
static const Wave waves[] = {
    // ---- scouts
    {1, W_COMM, CM5_HELM, 0, 0},
    {4, O_UFO, -60, -20, 0}, {4, O_UFO, 60, -20, 0},
    {8, O_UFO, 0, -36, 1}, {8, O_UFO, -90, 10, 0}, {8, O_UFO, 90, 10, 0},
    {13, O_UFO, -40, 20, 4}, {13, O_UFO, 40, 20, 4}, {13, O_UFO, 0, -10, NC},
    {16, W_COMM, CM5_ORBS, 0, 0},
    {18, O_UFO, -100, -30, 0}, {18, O_UFO, -50, -36, 0}, {18, O_UFO, 50, -36, 0}, {18, O_UFO, 100, -30, 0},
    {23, O_UFO, 0, 24, 4}, {23, O_UFO, -80, 0, NC}, {23, O_UFO, 80, 0, HN},
    {27, W_COMM, CM5_ELITE, 0, 0},
    {28, O_ELITE, 0, -24, 2}, {29, O_UFO, -90, -20, 0}, {29, O_UFO, 90, -20, 0},
    {34, O_UFO, -60, 24, 4}, {34, O_UFO, 60, 24, 4}, {34, O_UFO, 0, -38, 0},
    {38, O_ELITE, -56, -10, NC}, {38, O_UFO, 40, -30, 0}, {38, O_UFO, 90, 10, HN},
    {44, W_CP, 0, 0, 0},
    // ---- debris field: rocks near the middle are on a collision course
    {45, W_COMM, CM5_DEBRIS, 0, 0},
    {46, O_ELITE, 0, -28, 0}, {48, O_ROCK, 0, 4, 1}, {49, O_ROCK, -8, 6, 1}, {50, O_ROCK, 8, -4, 1}, {51, O_ROCK, -56, -36, 2},
    {52, O_ROCK, -4, -8, 1}, {52, O_ROCK, 50, 30, 2}, {53, O_ROCK, 6, 6, 2},
    {54, O_ROCK, -12, 0, 1}, {54, O_ROCK, 70, -10, 1}, {56, O_ROCK, 12, 2, 2}, {56, O_ROCK, -10, -4, 2 | NC},
    {57, O_UFO, 80, -20, 1}, {58, O_ROCK, 0, 8, 1}, {59, O_ROCK, -64, 34, 2}, {59, O_ROCK, 8, -8, 2}, {60, O_ROCK, -6, 6, 2 | HN},
    {62, O_UFO, 0, -36, 4}, {62, O_UFO, -90, 10, 0}, {63, O_ROCK, 4, 0, 2}, {64, O_ROCK, -12, -6, 2}, {64, O_ROCK, 56, -24, 2},
    {66, O_ROCK, 10, 6, 2}, {67, O_ROCK, -2, -6, 2 | NC}, {67, O_ROCK, -60, 10, 1}, {68, O_ROCK, 12, -2, 2},
    {70, O_ELITE, -50, -20, 3}, {71, O_ROCK, 0, 4, 2}, {72, O_ROCK, -8, -8, 2 | HN}, {73, O_ROCK, 8, 8, 2}, {74, O_UFO, 70, 20, 0},
    {76, O_ROCK, -10, 2, 2}, {76, O_ROCK, 10, -4, 2}, {78, O_ROCK, 0, 0, 2},
    {84, W_CP, 0, 0, 0},
    // ---- the swarm
    {85, W_COMM, CM5_PUSH, 0, 0},
    {86, O_ELITE, 0, -24, 0}, {87, O_ROCK, 0, 4, 2}, {89, O_UFO, -80, -26, 4}, {89, O_UFO, 80, -26, 4}, {89, O_UFO, 0, 20, 0}, {89, O_UFO, -40, -36, NC},
    {90, O_ELITE, 50, -24, 0}, {90, O_ROCK, -6, 4, 2}, {90, O_UFO, -90, 14, 0},
    {94, O_UFO, -30, -36, 4}, {94, O_UFO, 30, -36, 4}, {94, O_UFO, -100, 0, 0}, {94, O_UFO, 100, 0, 0}, {95, O_ROCK, 8, -4, 2 | NC},
    {99, O_ELITE, -60, -10, 1}, {99, O_ELITE, 60, -10, NC}, {99, O_UFO, 0, 26, 0},
    {104, O_ROCK, -10, -6, 2}, {104, O_ROCK, 10, 6, 2}, {104, O_UFO, 90, 0, 4}, {104, O_UFO, -90, 0, 4},
    {108, O_ELITE, 0, -30, 2}, {108, O_UFO, -64, 22, 0}, {108, O_UFO, 64, 22, 0}, {109, O_UFO, -30, -10, HN}, {109, O_UFO, 30, -10, HN},
    {113, O_ROCK, 0, 0, 2}, {113, O_UFO, -40, -24, 0}, {113, O_UFO, 40, -24, 0}, {114, O_ELITE, 0, 20, NC},
    {122, W_END, 0, 0, 0},
};

// ------------------------------------------------------------------ small helpers
static void bg_rows(uint8_t *base, uint8_t x, uint8_t y, uint8_t w, const uint8_t *map, const uint8_t *attr, uint8_t off, uint8_t attr_or) {
    uint8_t i;
    ld_copy(BANK_CK, buf, map, w);
    for (i = 0; i < w; i++) buf[i] += off;
    set_tiles(x, y, w, 1, base, buf);
    ld_copy(BANK_CK, buf, attr, w);
    if (attr_or) for (i = 0; i < w; i++) buf[i] |= attr_or;
    VBK_REG = 1; set_tiles(x, y, w, 1, base, buf); VBK_REG = 0;
}
static int8_t o_new(uint8_t type) {
    uint8_t i;
    for (i = 0; i < CK_MAXO; i++) if (ck_type[i] == O_NONE) {
        ck_type[i] = type; ck_t[i] = 0; ck_st[i] = 0; ck_a[i] = 0; ck_b[i] = 0; ck_flash[i] = 0; ck_hp[i] = 1;
        ck_wx[i] = 0; ck_wy[i] = 0; ck_z[i] = 250;
        return (int8_t)i;
    }
    return -1;
}
static void ck_boom(uint8_t x, uint8_t y) {
    int8_t i = o_new(O_BOOM);
    if (i < 0) return;
    ck_sx[i] = x; ck_sy[i] = y;
}
static uint8_t clamp8(int16_t v) { return v < 0 ? 0 : v > 255 ? 255 : (uint8_t)v; }
static void project(uint8_t i) {
    uint8_t sc = sc_tab[ck_z[i] >> 1];
    ck_sx[i] = clamp8(VCXB + (((int16_t)ck_wx[i] * sc) >> 6));
    ck_sy[i] = clamp8(VCYB + (((int16_t)ck_wy[i] * sc) >> 6));
}
// 0 small (8 px), 1 medium (16 px), 2 large (32 px)
static uint8_t o_size(uint8_t i) {
    uint8_t t = ck_type[i];
    if (t == O_ORB || t == O_BIGORB) return ck_st[i];           // kept up to date as it flies
    if (t == O_CAPS || t == O_BOOM) return 1;
    return ck_z[i] < 90 ? 2 : ck_z[i] < 180 ? 1 : 0;
}
static uint8_t orb_count(void) {
    uint8_t i, n = 0;
    for (i = 0; i < CK_MAXO; i++) if (ck_type[i] == O_ORB || ck_type[i] == O_BIGORB) n++;
    return n;
}
static uint8_t foe_count(void) {
    uint8_t i, n = 0;
    for (i = 0; i < CK_MAXO; i++) if (ck_type[i] >= O_UFO && ck_type[i] <= O_ROCK) n++;
    return n;
}
// an orb leaves (x, y) and lands on the cockpit n frames later unless it is shot down
static void orb_fire(uint8_t x, uint8_t y, uint8_t big) {
    int8_t i;
    uint8_t n = HUNTER ? 76 : MIDMODE ? 94 : 120;
    int16_t dx, dy;
    if (orb_count() >= (HUNTER ? 6 : MIDMODE ? 4 : 3) + big) return;
    i = o_new(big ? O_BIGORB : O_ORB);
    if (i < 0) return;
    if (big) n += 44;
    dx = (int16_t)(VCXB - 28 + (rnd() % 57)) - x; dy = (int16_t)(VCYB - 12 + (rnd() % 29)) - y;
    ck_fx[i] = (uint16_t)x << 8; ck_fy[i] = (uint16_t)y << 8;
    ck_vx[i] = ((dx << 6) / (int16_t)n) << 2; ck_vy[i] = ((dy << 6) / (int16_t)n) << 2;
    ck_sx[i] = x; ck_sy[i] = y;
    ck_z[i] = n; ck_a[i] = n / 3; ck_b[i] = ck_a[i]; ck_st[i] = big ? 1 : 0;      // ck_a: frames per size step
    ck_hp[i] = big ? (g_gear[GEAR_RUNES] ? 1 : 3) : 1;
    sfx_play(SFX_SPIT);
}

// ------------------------------------------------------------------ HUD + dashboard panel
static uint8_t hud_buf[20];                              // the HUD row is built here and sent in one go (20 single-tile writes cost ~25 lines)
#define hud_tile(x, t) (hud_buf[x] = (t))
static void hud_draw(void) {
    uint8_t i;
    if (HUNTER) {
        hud_tile(0, T_HUD_SHIP); hud_tile(1, txt_tile('x')); hud_tile(2, txt_tile('0' + (lives > 9 ? 9 : lives)));
        hud_tile(3, txt_tile(' ')); hud_tile(4, txt_tile(' '));
    } else {
        for (i = 0; i < 5; i++) hud_tile(i, i >= MAXHEARTS ? txt_tile(' ') : i < hearts ? T_HUD_HEART : T_HUD_HEARTE);
    }
    hud_tile(5, HUNTER ? txt_tile(' ') : txt_tile('0' + lives));
    hud_tile(6, T_HUD_BOMB); hud_tile(7, txt_tile('0' + bombs));
    hud_tile(8, bud_n ? T_HUD_BUD : txt_tile(' '));
    hud_tile(9, txt_tile(wtype == 0 ? 'T' : wtype == 1 ? 'S' : 'H'));
    for (i = 0; i < 3; i++) hud_tile(10 + i, i <= wlevel ? T_HUD_PIP : T_HUD_PIPE);
    if (ck_pop) for (i = 0; i < 7; i++) hud_tile(13 + i, txt_tile(ck_pop_s[i]));
    else score_tiles(hud_buf + 13);
    set_win_tiles(0, 1, 20, 1, hud_buf);
    hud_dirty = 0;
}
static void rim_draw(void) {
    ld_copy(BANK_CK, buf, ck_rim_map, 20);
    set_win_tiles(0, 0, 20, 1, buf);
    ld_copy(BANK_CK, buf, ck_rim_attr, 20);
    VBK_REG = 1; set_win_tiles(0, 0, 20, 1, buf); VBK_REG = 0;
}
#define BOSS_AWARD 26000UL
// the outcome the run is heading for (the boss award is counted in until it has been paid)
static uint32_t proj_pts(void) { return k_state < 20 ? bounty_pts + BOSS_AWARD : bounty_pts; }
static uint8_t rating(void) {
    uint32_t p = proj_pts();
    return p >= ck_cap_thr[HUNTER] ? OUT_CAPTURE : p >= ck_kill_thr[HUNTER] ? OUT_KILL : OUT_FLEE;
}
static uint8_t bar_cells(void) {
    uint32_t p = proj_pts(), lo = BOSS_AWARD, cap = ck_cap_thr[HUNTER];
    if (p >= cap) return 8;
    if (p <= lo) return 0;
    return (uint8_t)(((p - lo) << 3) / (cap - lo));
}
static void panel_bar_draw(void) {
    uint8_t i, n = bar_cells();
    panel_bar = n;
    for (i = 0; i < 8; i++) set_win_tile_xy(12 + i, 3, i < n ? T_HUD_BAR : T_HUD_BARE);
}
static void panel_rate_draw(void) {
    panel_rate = rating();
    txt_put(5, 4, panel_rate == OUT_CAPTURE ? "RATING: CAPTURE" : panel_rate == OUT_KILL ? "RATING: KILL   " : "RATING: ESCAPE ", 1, HA);
}
static void alert_draw(void) {
    alert_last = alert;
    txt_put(5, 5, alert == 1 ? "BEAM! SHOOT IT!" : alert == 2 ? (HUNTER ? "PAD AIM  A FIRE" : "PAD AIM  B BOMB") : alert == 3 ? "HULL BREACH!   " : "               ", 1, HA);
}
static void panel_draw(void) {
    uint8_t i;
    txt_fill(0, 2, 20, 4, txt_tile(' '), HA, 1);
    for (i = 0; i < 4; i++) {
        ld_copy(BANK_CK, buf, ck_radar_map + i * 4, 4);
        set_win_tiles(0, 2 + i, 4, 1, buf);
        ld_copy(BANK_CK, buf, ck_radar_attr + i * 4, 4);
        VBK_REG = 1; set_win_tiles(0, 2 + i, 4, 1, buf); VBK_REG = 0;
        set_win_tile_xy(4, 2 + i, CK_T_DIV);
        VBK_REG = 1; set_win_tile_xy(4, 2 + i, 6 | 0x08 | 0x80); VBK_REG = 0;
    }
    txt_put(5, 2, "LAMP DRIFT", 1, HA);
    txt_put(5, 3, "BOUNTY", 1, HA);
    panel_bar_draw(); panel_rate_draw(); alert_draw();
}
static uint16_t bar_hp;
static void boss_bar_draw(void) {
    uint8_t n, i;
    if (bar_last == 0xFF) {
        txt_fill(0, 0, 20, 1, txt_tile(' '), HA, 1);
        txt_put(0, 0, "LAMPTALL", 1, HA);
    }
    if (k_hp == bar_hp && bar_last != 0xFF) return;
    bar_hp = k_hp;
    {                                                   // ceil(hp * 10 / max) without a 32-bit divide (~20 scanlines)
        uint16_t v = k_hp * 10, acc = 0;                // hp <= 900, so hp * 10 fits
        n = 0;
        while (acc < v && n < 10) { acc += k_hpmax; n++; }
    }
    if (n == bar_last) return;
    bar_last = n;
    for (i = 0; i < 10; i++) set_win_tile_xy(10 + i, 0, i < n ? T_HUD_BAR : T_HUD_BARE);
}

// ------------------------------------------------------------------ boss graphics
static const uint8_t * const b_maps[3] = {ck_boss_map0, ck_boss_map1, ck_boss_map2};
static const uint8_t * const b_attrs[3] = {ck_boss_attr0, ck_boss_attr1, ck_boss_attr2};
static void boss_row(uint8_t pose, uint8_t r) {
    bg_rows(MAP98, BOSS_COL, BOSS_ROW + r, 8, b_maps[pose] + r * 8, b_attrs[pose] + r * 8, 0, 0);
}
// a pose change is spread over four frames so it never costs a frame
static uint8_t pose_row = 8;
static void boss_pose(uint8_t pose) {
    if (pose == b_pose) return;
    b_pose = pose; pose_row = 0;
}
static void boss_pose_step(void) {
    uint8_t n = 2;                                      // two rows a frame (each row ~10 scanlines)
    while (pose_row < 8 && n--) boss_row(b_pose, pose_row++);
}
static void boss_fade(uint8_t k) { b_fade = k; ld_bkg_pal(BANK_CK, 2, 2, ck_boss_fade + k * 8); }
static uint8_t boss_sx(void) { return (uint8_t)(VCXB - 1 + ck_bx); }
static uint8_t boss_sy(void) { return (uint8_t)(BOSS_CY + VB - 4 + ck_by); }

// ------------------------------------------------------------------ player
static void clear_orbs(uint8_t pts) {
    uint8_t i;
    for (i = 0; i < CK_MAXO; i++) if (ck_type[i] == O_ORB || ck_type[i] == O_BIGORB) {
        ck_type[i] = O_BOOM; ck_t[i] = 8;
        if (pts) add_score(10);
    }
}
static void ck_hit(void) {
    if (p_inv || st == ST_DYING || st == ST_CLEAR) return;
    hits_taken++;
    shake_t = 14; flash_t = 3; hud_dirty = 1;
    if (!HUNTER && hearts > 1) {
        hearts--; p_inv = 70; sfx_play(SFX_HURT);
        return;
    }
    hearts = 0;
    st = ST_DYING; st_t = 0;
    alert = 3;
    clear_orbs(0);
    if (bud_n) { bud_n = 0; }
    sfx_play(SFX_EXPL_L);
}
static void boss_damage(uint8_t dmg) {
    static uint8_t hc;
    if (k_state != 2) return;
    k_hp = k_hp > dmg ? k_hp - dmg : 0;
    if (ck_charge) b_dmg++;
    if (++hc >= 3) { hc = 0; k_hitflash = 2; }
    sfx_play(SFX_BOSSHIT);
    add_score(10);
}
static void caps_take(uint8_t i) {
    uint8_t kind = ck_a[i];
    if (kind == 1) {
        uint8_t cy = (ck_t[i] >> 6) % 3;
        ck_pop = 90;
        if (cy == wtype) { if (wlevel < 2) wlevel++; else add_score(1000); ck_pop_s = wlevel == 2 ? "MAX!   " : "POWER! "; }
        else { wtype = cy; ck_pop_s = cy == 0 ? "TWIN   " : cy == 1 ? "SPREAD " : "WISP   "; }
        sfx_play(SFX_POWER);
    }
    else if (kind == 2) { if (bombs < 9) bombs++; sfx_play(SFX_PICKUP); }
    else if (HUNTER) { if (lives < 9) lives++; sfx_play(SFX_ONEUP); }
    else { if (hearts < MAXHEARTS) hearts++; else add_score(500); sfx_play(SFX_PICKUP); }
    add_score(100);
    ck_type[i] = O_NONE;
    hud_dirty = 1;
}
static void o_kill(uint8_t i) {
    uint8_t t = ck_type[i], drop = ck_b[i] & 3, big = o_size(i) == 2;
    uint8_t x = ck_sx[i], y = ck_sy[i];
    add_score(t == O_UFO ? 400 : t == O_ELITE ? 1000 : t == O_ROCK ? (ck_b[i] & 0x10 ? 300 : 200) : t == O_BIGORB ? 100 : 20);
    if (t == O_ORB || t == O_BIGORB) { ck_type[i] = O_BOOM; ck_t[i] = 8; sfx_play(SFX_TICK); return; }
    kill_count++;
    ck_type[i] = O_BOOM; ck_t[i] = 0;
    sfx_play(t == O_ELITE ? SFX_EXPL_L : SFX_EXPL_S);
    if (t == O_ELITE || big) ck_boom(x - 8 + (rnd() & 15), y - 6 + (rnd() & 7));
    if (drop && t != O_ROCK) {
        int8_t c = o_new(O_CAPS);
        if (c >= 0) {
            ck_a[c] = drop; ck_z[c] = 210;
            if (y < VB + 30) y = VB + 30;
            ck_fx[c] = (uint16_t)x << 8; ck_fy[c] = (uint16_t)y << 8; ck_vx[c] = x < VCXB ? 40 : -40; ck_vy[c] = 48;
            ck_sx[c] = x; ck_sy[c] = y;
        }
    }
}
static void o_damage(uint8_t i, uint8_t dmg) {
    if (ck_type[i] == O_CAPS) { caps_take(i); return; }
    if (ck_hp[i] > dmg) { ck_hp[i] -= dmg; ck_flash[i] = 4; sfx_play(SFX_TICK); }
    else o_kill(i);
}
// what is under the crosshair: object index, 0xFE boss, 0xFF nothing
static const uint8_t hit_w[3] = {5, 9, 15}, hit_h[3] = {5, 7, 11}, hit_orb[3] = {5, 7, 9};
static uint8_t target_at(void) {
    uint8_t i, t, s, hw, hh, key, best = 0xFF, bk = 255;
    uint8_t as = (HUNTER ? 0 : MIDMODE ? 1 : 3) + (wtype == 2 ? 8 : 0);     // wisp shots seek: a wider lock
    uint8_t cx = ck_cx + VB, cy = ck_cy + VB;
    for (i = 0; i < CK_MAXO; i++) {
        t = ck_type[i];
        if (t == O_NONE || t == O_BOOM) continue;
        s = o_size(i);
        if (t == O_ORB || t == O_BIGORB) { hw = hh = hit_orb[s]; key = ck_z[i] >> 3; }
        else if (t == O_CAPS) { hw = hh = 9; key = 22; }
        else { hw = hit_w[s]; hh = hit_h[s]; key = ck_z[i]; }
        if (key < bk && NEAR(ck_sx[i], cx, hw + as) && NEAR(ck_sy[i], cy, hh + as)) { bk = key; best = i; }
    }
    if (best == 0xFF && k_state == 2 && NEAR(boss_sx(), cx, 19) && NEAR(boss_sy(), cy, 26)) best = 0xFE;
    return best;
}
static void player_fire(void) {
    uint8_t dmg = 2 + wlevel;
    fire_cd = wlevel == 2 ? 7 : 8;
    bolt_t = 3;
    sfx_play(SFX_SHOT);
    if (lock == 0xFF) return;
    spark_t = 6; spark_k = wtype;
    if (lock == 0xFE) { boss_damage(dmg); spark_x = ck_cx + VB; spark_y = ck_cy + VB; return; }
    spark_x = ck_sx[lock]; spark_y = ck_sy[lock];
    if (wtype == 1) {                                   // spread: the burst splashes anything close by
        uint8_t i, t;
        for (i = 0; i < CK_MAXO; i++) {
            t = ck_type[i];
            if (i != lock && t >= O_UFO && t <= O_BIGORB && NEAR(ck_sx[i], spark_x, 20) && NEAR(ck_sy[i], spark_y, 16)) o_damage(i, 1);
        }
    }
    if (ck_type[lock] != O_NONE) o_damage(lock, dmg);
}
static void use_bomb(void) {
    uint8_t i, t;
    if (!bombs || bomb_t) return;
    bombs--; hud_dirty = 1;
    bomb_t = 60; flash_t = 8;
    if (p_inv < 90) p_inv = 90;
    sfx_play(SFX_BOMB);
    clear_orbs(1);
    for (i = 0; i < CK_MAXO; i++) {
        t = ck_type[i];
        if (t >= O_UFO && t <= O_ROCK) o_damage(i, 20);
    }
    if (k_state == 2) {
        uint16_t d = HUNTER ? 40 : 60;
        k_hp = k_hp > d ? k_hp - d : 1;
        if (ck_charge) b_dmg += 4;
        k_hitflash = 6;
    }
}
static void player_update(void) {
    uint8_t v;
    if (p_inv) p_inv--;
    if (joy & (J_LEFT | J_RIGHT | J_UP | J_DOWN)) { if (hold_t < 40) hold_t++; } else hold_t = 0;
    v = hold_t < 5 ? 1 : hold_t < 14 ? 2 : 3;
    if (joy & J_LEFT)  ck_cx = ck_cx > 8 + v ? ck_cx - v : 8;
    if (joy & J_RIGHT) ck_cx = ck_cx < 152 - v ? ck_cx + v : 152;
    if (joy & J_UP)    ck_cy = ck_cy > 12 + v ? ck_cy - v : 12;
    if (joy & J_DOWN)  ck_cy = ck_cy < 90 - v ? ck_cy + v : 90;
    lock = target_at();
    if (fire_cd) fire_cd--;
    else if (!HUNTER || (joy & J_A)) player_fire();
    if (joy_new & J_B) use_bomb();
    // buddy gunner (Buddy Bay level 2): picks off the closest orb now and then
    if (bud_n) {
        if (bud_cd) bud_cd--;
        else {
            uint8_t i, best = 0xFF, bz = 255;
            for (i = 0; i < CK_MAXO; i++) if (ck_type[i] == O_ORB && ck_z[i] < bz) { bz = ck_z[i]; best = i; }
            if (best != 0xFF && bz < 60) { o_kill(best); bud_cd = 70; }
        }
    }
}

// ------------------------------------------------------------------ objects
static void objects_update(void) {
    uint8_t i, t;
    for (i = 0; i < CK_MAXO; i++) {
        t = ck_type[i];
        if (t == O_NONE) continue;
        if (ck_flash[i]) ck_flash[i]--;
        ck_t[i]++;
        switch (t) {
        case O_BOOM:
            if (ck_t[i] >= 20) ck_type[i] = O_NONE;
            break;
        case O_UFO:
        case O_ELITE:
            if (ck_st[i] == 0) {                                     // fly in
                ck_z[i] -= 3;
                if ((i ^ ck_t[i]) & 1) project(i);                   // every other frame is plenty
                if (ck_z[i] <= (t == O_ELITE ? 80 : 104)) {
                    project(i);
                    ck_st[i] = 1; ck_t[i] = 0; ck_hx[i] = ck_sx[i]; ck_hy[i] = ck_sy[i];
                    ck_ft[i] = (HUNTER ? 30 : MIDMODE ? 60 : 90) + ((i * 13) & 31);
                }
            } else if (ck_st[i] == 1) {                              // hover, strafe, shoot
                ck_sx[i] = (ck_b[i] & 4) ? ck_hx[i] + (sin64[(ck_t[i] * 2) & 63] >> 3) : ck_hx[i];
                ck_sy[i] = ck_hy[i] + ((ck_t[i] >> 5) & 1);
                if (st != ST_DYING && --ck_ft[i] == 0) {
                    if (t == O_ELITE && !(ck_b[i] & 8)) { orb_fire(ck_sx[i], ck_sy[i] + 6, 0); ck_b[i] |= 8; ck_ft[i] = 18; }   // elites fire pairs
                    else {
                        orb_fire(ck_sx[i], ck_sy[i] + 6, t == O_ELITE && HUNTER);
                        ck_b[i] &= ~8;
                        ck_ft[i] = HUNTER ? 80 : MIDMODE ? 115 : 150;
                    }
                }
                if (ck_t[i] >= 250) ck_st[i] = 2;
            } else {                                                 // peel off over the canopy, growing as it passes
                if (ck_z[i] > 40) ck_z[i] -= 3;
                if (ck_sy[i] < 24) { ck_type[i] = O_NONE; continue; }
                ck_sy[i] -= 3;
                ck_sx[i] += (int8_t)(ck_sx[i] - VCXB) >> 4;
            }
            break;
        case O_ROCK:
            ck_z[i]--;
            if ((ck_b[i] & 3) >= 2 && (ck_t[i] & 1)) ck_z[i]--;                 // fast rock
            if (ck_z[i] <= 26) {
                if (ck_b[i] & 0x10) { ck_boom(ck_sx[i], ck_sy[i]); ck_type[i] = O_BOOM; ck_t[i] = 0; ck_hit(); }
                else ck_type[i] = O_NONE;
                continue;
            }
            if ((i ^ ck_t[i]) & 1) project(i);
            break;
        case O_ORB:
        case O_BIGORB:
            ck_fx[i] += ck_vx[i]; ck_fy[i] += ck_vy[i];
            ck_sx[i] = ck_fx[i] >> 8; ck_sy[i] = ck_fy[i] >> 8;
            if (--ck_z[i] == 0) { ck_type[i] = O_BOOM; ck_t[i] = 4; ck_hit(); break; }
            if (--ck_b[i] == 0) { ck_b[i] = ck_a[i]; if (ck_st[i] < 2) ck_st[i]++; }     // one size bigger each third of the way
            break;
        case O_CAPS:
            ck_fx[i] += ck_vx[i]; ck_fy[i] += ck_vy[i];
            ck_sx[i] = ck_fx[i] >> 8; ck_sy[i] = ck_fy[i] >> 8;
            if (--ck_z[i] == 0 || ck_sy[i] > VB + 92) ck_type[i] = O_NONE;
            break;
        }
    }
}
static void waves_update(void) {
    const Wave *w;
    int8_t i;
    if (st != ST_PLAY) return;
    if (++ck_sub >= 32) { ck_sub = 0; if (ck_tick < 255) ck_tick++; }
    while (1) {
        w = &waves[ck_pc];
        if (w->t > ck_tick) return;
        if (w->type == W_END) {
            if (foe_count()) return;                                 // let the last wave play out
            st = ST_BOSS; k_state = 1; b_t = 0; ck_cp = 3;
            return;
        }
        ck_pc++;
        if (w->type == W_COMM) { comm_queue_add((uint8_t)w->x); continue; }
        if (w->type == W_CP) { if (ck_cp < 2 && cp_tick[ck_cp + 1] == 0) { ck_cp++; cp_pc[ck_cp] = ck_pc; cp_tick[ck_cp] = ck_tick; } continue; }
        if ((w->p & HN) && !HUNTER) continue;
        if ((w->p & NC) && gmode == MODE_CADET) continue;
        i = o_new(w->type);
        if (i < 0) continue;
        ck_wx[i] = w->x; ck_wy[i] = w->y; ck_z[i] = 250;
        ck_b[i] = w->p & 7;
        if (w->type == O_ROCK) {
            ck_hp[i] = HUNTER ? 6 : 5;
            if (w->x > -16 && w->x < 16 && w->y > -12 && w->y < 12) ck_b[i] |= 0x10;     // on a collision course
        } else if (w->type == O_ELITE) ck_hp[i] = HUNTER ? 40 : MIDMODE ? 30 : 24;
        else ck_hp[i] = HUNTER ? 14 : MIDMODE ? 10 : 8;
        project(i);
    }
}

// ------------------------------------------------------------------ the Flatwoods Monster
enum { A_WAIT, A_VOLLEY, A_DRONES, A_BIG, A_CHARGE, A_STAGGER };
static const uint8_t atk_seq[8] = {A_VOLLEY, A_DRONES, A_CHARGE, A_VOLLEY, A_BIG, A_CHARGE, A_DRONES, A_BIG};
static void boss_spawn_ufo(int8_t x) {
    int8_t i = o_new(O_UFO);
    if (i < 0) return;
    ck_wx[i] = x; ck_wy[i] = 18; ck_z[i] = 250; ck_b[i] = 4; ck_hp[i] = HUNTER ? 14 : MIDMODE ? 10 : 8;
    project(i);
}
static void boss_update(void) {
    uint8_t bx = boss_sx(), by = boss_sy();
    uint8_t gap;
    if (k_state == 1) {                                              // phases in
        if (b_t == 0) {
            music_play(MUS_WARN);
            comm_queue_add(CM5_BOSS);
            b_pose = 0xFF; boss_fade(0);
            k_hpmax = HUNTER ? 720 : MIDMODE ? 900 : 800; k_hp = k_hpmax;
            ck_bx = 0; ck_by = 0; b_ph = 0; b_phase2 = 0; b_seq = 0; b_atk = A_WAIT; b_at = 0; b_hint = 0; ck_charge = 0;
        }
        if (b_t < 8) boss_row(0, b_t);
        if (b_t == 8) { b_pose = 0; pose_row = 8; }
        if (b_t == 40) boss_fade(1);
        if (b_t == 60) { boss_fade(2); sfx_play(SFX_ROAR); }
        if (b_t == 80) boss_fade(3);
        if (b_t == 150) { k_state = 2; music_play(MUS_FLATWOODS); bar_last = 0xFF; boss_bar_draw(); b_t = 0; return; }
        b_t++;
        return;
    }
    if (k_state == 2) {
        if (k_hp == 0) {
            k_state = 20; b_t = 0; ck_charge = 0; alert = 0;
            clear_orbs(0);
            add_score((uint16_t)BOSS_AWARD);
            if (hits_taken == 0) add_score(10000);
            boss_fade(3);
            boss_pose(2);
            music_stop();
            return;
        }
        if (!b_phase2 && k_hp < (k_hpmax >> 1)) { b_phase2 = 1; comm_queue_add(CM5_HURT); sfx_play(SFX_ROAR); }
        // drift in a figure of eight (holds still while charging or staggered)
        if (b_atk != A_CHARGE && b_atk != A_STAGGER) {
            b_ph += (b_phase2 && HUNTER) ? 2 : 1;
            if ((frame_count & 1) || b_phase2) {
                int8_t sn = sin64[(b_ph >> 2) & 63];
                ck_bx = (sn >> 2) + (sn >> 4);                       // +-39 px
                ck_by = sin64[(b_ph >> 1) & 63] >> 4;                // +-8 px
            }
        }
        b_at++;
        switch (b_atk) {
        case A_WAIT:
            gap = (HUNTER ? 58 : MIDMODE ? 72 : 90) - (b_phase2 ? 14 : 0);
            if (b_at >= gap) {
                b_atk = atk_seq[b_seq & 7]; b_seq++; b_at = 0; b_n = 0;
                if (b_atk == A_DRONES && foe_count() >= 2) b_atk = A_VOLLEY;
                if (b_atk == A_CHARGE) { ck_charge = HUNTER ? 112 : MIDMODE ? 128 : 155; b_dmg = 0; alert = 1; sfx_play(SFX_ALARM); if (!b_hint) { b_hint = 1; comm_queue_add(CM5_BEAM); } }
                if (b_atk != A_DRONES) boss_pose(1);
            }
            break;
        case A_VOLLEY:
            if (b_at >= 18) {
                b_at = 0;
                orb_fire(bx + ((b_n & 1) ? 20 : -20), by - 18, 0);
                if (++b_n >= (HUNTER ? 4 : MIDMODE ? 4 : 3) + b_phase2) { b_atk = A_WAIT; b_at = 0; boss_pose(0); }
            }
            break;
        case A_DRONES:
            if (b_at == 10) boss_spawn_ufo(-76);
            if (b_at == 30 && gmode != MODE_CADET) boss_spawn_ufo(76);
            if (b_at >= 40) { b_atk = A_WAIT; b_at = 0; }
            break;
        case A_BIG:
            if (b_at == 20) orb_fire(bx, by - 4, 1);
            if (b_at == 62 && HUNTER) orb_fire(bx, by - 4, 1);
            if (b_at >= 70) { b_atk = A_WAIT; b_at = 0; boss_pose(0); }
            break;
        case A_CHARGE:
            // enough damage while its eyes charge breaks the beam and staggers it
            if (b_dmg >= (HUNTER ? 9 : 8)) {             // hits landed during the charge
                ck_charge = 0; alert = 0;
                b_atk = A_STAGGER; b_at = 0;
                boss_pose(2);
                add_score(500);
                sfx_play(SFX_EXPL_L); shake_t = 6;
                if (!b_stag) { b_stag = 1; comm_queue_add(CM5_STAGGER); }
                break;
            }
            if ((ck_charge & 15) == 0) sfx_play(SFX_BLIP);
            if (--ck_charge == 0) {
                alert = 0;
                flash_t = 6; sfx_play(SFX_THUNDER);
                ck_hit();
                b_atk = A_WAIT; b_at = 0; boss_pose(0);
            }
            break;
        case A_STAGGER:
            if (b_at >= 70) { b_atk = A_WAIT; b_at = 20; boss_pose(0); }
            break;
        }
        boss_bar_draw();
        return;
    }
    if (k_state == 20) {                                             // beaten: a beat, then the bounty outcome
        b_t++;
        if ((b_t & 7) == 0) { ck_boom(bx - 16 + (rnd() & 31), by - 20 + (rnd() & 31)); sfx_play(SFX_EXPL_S); }
        shake_t = 3;
        if (b_t >= 80) { k_state = 21; outro_t = 0; boss_bar_draw(); }
        return;
    }
    if (k_state == 21) {
        if (outro_t == 0) {
            bounty_out = rating();
            k_hitflash = 0;
            sfx_play(bounty_out == OUT_FLEE ? SFX_WHOOSH : SFX_POWER);
        }
        outro_t++;
        if (outro_t == (bounty_out == OUT_CAPTURE ? 114 : bounty_out == OUT_KILL ? 74 : 66))
            comm_queue_add(boss_out_comm[CK_STAGE * 3 + bounty_out - 1]);
        switch (bounty_out) {
        case OUT_CAPTURE:                                            // Forge's rune net closes in
            if (outro_t > 20 && outro_t < 100 && (outro_t & 7) == 0) sfx_play(SFX_TICK);
            if (outro_t == 100) { ld_bkg_pal(BANK_CK, 2, 2, ck_boss_hit); sfx_play(SFX_ONEUP); shake_t = 10; }
            if (outro_t == 112) boss_fade(0);
            if (outro_t >= 200) k_state = 22;
            break;
        case OUT_KILL:
            if (outro_t < 70 && (outro_t & 3) == 0) {
                ck_boom(bx - 20 + (rnd() & 31) + (rnd() & 7), by - 24 + (rnd() & 31) + (rnd() & 15));
                sfx_play((outro_t & 7) ? SFX_EXPL_S : SFX_EXPL_L);
            }
            if (outro_t < 70) shake_t = 3;
            if (outro_t < 64) { if (outro_t & 8) ld_bkg_pal(BANK_CK, 2, 2, ck_boss_hit); else boss_fade(3); }
            if (outro_t == 70) { boss_fade(0); sfx_play(SFX_EXPL_L); flash_t = 4; }
            if (outro_t >= 170) k_state = 22;
            break;
        default:                                                     // it phases out and is gone
            if (outro_t < 60) boss_fade((outro_t & 4) ? 1 : outro_t < 30 ? 3 : 2);
            if (outro_t == 60) boss_fade(0);
            if (outro_t >= 140) k_state = 22;
        }
    }
}

// ------------------------------------------------------------------ drawing
// centre-based sprite helpers in view coordinates; anything that would poke into the dashboard is dropped
static void spr8(uint8_t cx, uint8_t cy, uint8_t tile, uint8_t prop) {
    if (cx < VB - 3 || cx > VB + 163 || cy < VB - 7 || cy > VB + 96) return;
    oam_add(cx - (VB - 4), cy - (VB - 8), tile, prop);
}
static void spr16(uint8_t cx, uint8_t cy, uint8_t tile, uint8_t prop) {
    if (cx < VB - 7 || cx > VB + 167 || cy < VB - 7 || cy > VB + 96) return;
    oam_add(cx - VB, cy - (VB - 8), tile, prop);
    oam_add(cx - (VB - 8), cy - (VB - 8), tile + 2, prop);
}
static void spr32(uint8_t cx, uint8_t cy, uint8_t tile, uint8_t prop) {
    uint8_t c, x;
    if (cx < VB - 15 || cx > VB + 175 || cy < VB - 15 || cy > VB + 104) return;
    for (c = 0; c < 4; c++) {
        x = cx - (VB + 8) + c * 8;                                  // OAM x; off-screen columns are skipped
        if (x == 0 || x >= 168) continue;
        if (cy >= VB) oam_add(x, cy - VB, tile + c * 2, prop);
        if (cy <= VB + 88) oam_add(x, cy - (VB - 16), tile + 8 + c * 2, prop);
    }
}
static int16_t qtr(int16_t d, uint8_t f) { return f == 1 ? d >> 2 : f == 2 ? d >> 1 : d - (d >> 2); }
static void star_new(uint8_t i) {
    uint8_t d = rnd() & 31;
    star_vx[i] = dsin[d] * 3; star_vy[i] = DCOS(d) * 3;
    star_x[i] = ((uint16_t)VCXB << 8) + star_vx[i] * 8; star_y[i] = ((uint16_t)VCYB << 8) + star_vy[i] * 8;
    star_age[i] = 0;
}
static const uint8_t t_small[4] = {0, CK_ST_UFO_S, CK_ST_UFO_S, CK_ST_ROCK_S};
static const uint8_t t_med[4] = {0, CK_ST_UFO_M, CK_ST_UFO_M, CK_ST_ROCK_M};
static const uint8_t t_large[4] = {0, CK_ST_UFO_L, CK_ST_UFO_L, CK_ST_ROCK_L};
static const uint8_t t_pal[4] = {0, 4 | 8, 6 | 8, 5 | 8};
static void draw_all(void) {
    uint8_t i, t, s, p, c, f, x, y;
    // crosshair (fixed slots 0-1)
    if (st == ST_DYING || st == ST_CLEAR || (p_inv && (p_inv & 4))) { oam_fixed(0, 0, 0, 0, 0); oam_fixed(1, 0, 0, 0, 0); }
    else {
        t = lock != 0xFF ? CK_ST_XLOCK : CK_ST_XHAIR; p = lock != 0xFF ? (7 | 8) : (0 | 8);
        oam_fixed(0, ck_cx, ck_cy + 8, t, p);
        oam_fixed(1, ck_cx + 8, ck_cy + 8, t + 2, p);
    }
    oam_fixed(2, 0, 0, 0, 0); oam_fixed(3, 0, 0, 0, 0);
    oam_begin();
    // cannon bolts converging on the crosshair
    if (bolt_t) {
        f = 4 - bolt_t;                                             // 1..3 quarters of the way
        y = (uint8_t)(VB + 92 + qtr((int16_t)ck_cy - 92, f));
        if (wtype == 0) { t = f == 1 ? CK_ST_BOLT : CK_ST_BOLT2; p = 1 | 8; }
        else { t = wtype == 1 ? ST_SHOT_SPREAD : ST_SHOT_WISP; p = 1; }
        spr8((uint8_t)(VB + 16 + qtr((int16_t)ck_cx - 16, f)), y, t, p);
        spr8((uint8_t)(VB + 144 + qtr((int16_t)ck_cx - 144, f)), y, t, p | 0x20);
        if (f == 1) {                                               // muzzle flashes at the gun ports
            t = wtype == 0 ? ST_SPARK : wtype == 1 ? FX_MUZ_SPREAD : FX_MUZ_WISP; p = wtype ? 1 | 8 : 7;
            spr8(VB + 16, VB + 88, t, p); spr8(VB + 144, VB + 88, t, p | 0x20);
        }
        bolt_t--;
    }
    if (spark_t) {                                                  // impact: twin sparks, spread star, wisp frost
        spark_t--;
        if (spark_k == 0) spr8(spark_x - 1 + (rnd() & 3), spark_y - 1 + (rnd() & 3), ST_SPARK, 7);
        else spr8(spark_x, spark_y, (spark_k == 1 ? FX_IMP_STAR_A : FX_IMP_FROST_A) + (spark_t >= 3 ? 0 : 2), 1 | 8);
    }
    // beam charge: the eyes flare
    if (ck_charge && (ck_charge & 2)) {
        spr8(boss_sx() - 5, boss_sy() - 1, CK_ST_GLOW, 6 | 8);
        spr8(boss_sx() + 7, boss_sy() - 1, CK_ST_GLOW, 6 | 8);
    }
    // rune net (capture)
    if (k_state == 21 && bounty_out == OUT_CAPTURE && outro_t < 112) {
        uint8_t r = outro_t < 90 ? 60 - (outro_t >> 1) - (outro_t >> 3) : 4;
        for (i = 0; i < 8; i++) {
            c = (uint8_t)(i * 8 + outro_t);
            spr8(boss_sx() + (int8_t)(((int16_t)sin64[c & 63] * r) >> 7), boss_sy() + (int8_t)(((int16_t)COS64(c & 63) * r) >> 8), ST_RUNE, 2);
        }
    }
    // orbs and capsules first (they are nearest), then saucers and rocks
    for (i = 0; i < CK_MAXO; i++) {
        t = ck_type[i];
        if (t < O_ORB) continue;
        x = ck_sx[i]; y = ck_sy[i];
        if (t == O_BOOM) { spr16(x, y, ST_EXPL0 + (ck_t[i] / 5) * 4, 7); continue; }
        if (t == O_CAPS) {
            if (ck_z[i] < 50 && (ck_z[i] & 4)) continue;
            c = (ck_t[i] >> 6) % 3;
            spr16(x, y, ck_a[i] == 1 ? (c == 0 ? ST_PK_T : c == 1 ? ST_PK_S : ST_PK_H) : ck_a[i] == 2 ? ST_PK_B : HUNTER ? ST_PK_1UP : ST_PK_HEART, 7);
            continue;
        }
        s = ck_st[i];
        p = (t == O_BIGORB ? 6 : 3) | 8;
        if (ck_flash[i] || (s == 2 && (ck_z[i] & 4))) p = 7 | 8;     // about to land: it blinks
        if (s == 0) spr8(x, y, CK_ST_ORB_S, p);
        else spr16(x, y, s == 1 ? CK_ST_ORB_M : CK_ST_ORB_L, p);
    }
    for (i = 0; i < CK_MAXO; i++) {
        t = ck_type[i];
        if (t == O_NONE || t > O_ROCK) continue;
        x = ck_sx[i]; y = ck_sy[i];
        s = ck_z[i] < 90 ? 2 : ck_z[i] < 180 ? 1 : 0;
        p = ck_flash[i] ? (7 | 8) : t_pal[t];
        if (t == O_ROCK && (ck_b[i] & 0x10) && ck_z[i] < 150 && (ck_t[i] & 8)) p = 6 | 8;    // collision course: flashes red
        if (s == 0) spr8(x, y, t_small[t], p);
        else if (s == 1) spr16(x, y, t_med[t], p);
        else spr32(x, y, t_large[t], p);
    }
    // speed streaks
    for (i = 0; i < 6; i++) {
        star_x[i] += star_vx[i]; star_y[i] += star_vy[i];
        if (star_age[i] > 24) { star_x[i] += star_vx[i]; star_y[i] += star_vy[i]; }      // they speed up as they pass
        star_age[i]++;
        x = star_x[i] >> 8; y = star_y[i] >> 8;
        if (x < VB + 2 || x > VB + 158 || y < VB + 18 || y > VB + 90) { star_new(i); continue; }
        spr8(x, y, star_age[i] < 20 ? CK_ST_STAR1 : star_age[i] < 40 ? CK_ST_STAR2 : CK_ST_STAR3, 2 | 8);
    }
    oam_end();
}
static void raster(void) {
    uint8_t shake = 0;
    int8_t tx = 0, ty = 0;
    if (shake_t) { shake_t--; shake = (shake_t & 2) ? 2 : 0; }
    // the view leans a little with the crosshair; it settles back when the boss arrives
    if (k_state == 0) { tx = ((int8_t)(ck_cx - 80)) / 8; ty = ((int8_t)(ck_cy - 52)) / 8; }
    if (parx < tx) parx++; else if (parx > tx) parx--;
    if (pary < ty) pary++; else if (pary > ty) pary--;
    n_lcdc = LCDC_COMM; n_scx = 0; n_scy = 64;
    n_s1_line = 16; n_s1_lcdc = LCDC_PLAY;
    n_s1_scx = (uint8_t)(48 - ck_bx + parx); n_s1_scy = (uint8_t)(16 - ck_by + pary + shake);
    n_s2_line = 0;
}

// ------------------------------------------------------------------ setup
static void view_load(void) {
    uint8_t r, i;
    for (r = 0; r < 32; r++) {
        if (r < CK_BG_H) bg_rows(MAP98, 0, r, 32, ck_bg_map + r * 32, ck_bg_attr + r * 32, N_COMMON, 0);
        else {
            memset(buf, N_COMMON, 32); set_tiles(0, r, 32, 1, MAP98, buf);
            memset(buf, 0, 32); VBK_REG = 1; set_tiles(0, r, 32, 1, MAP98, buf); VBK_REG = 0;
        }
    }
    for (i = 0; i < 2; i++) bg_rows(MAP9C, 0, 8 + i, 20, ck_canopy_map + i * 20, ck_canopy_attr + i * 20, 0, 0);
}
static void ck_load(void) {
    screen_off();
    splits_off();
    oam_clear();
    VBK_REG = 0;
    ld_spr_data(BANK_A2, 0, N_SPR_TILES, spr_tiles);
    VBK_REG = 1; ld_spr_data(BANK_CK, 0, CK_SPR_NT, ck_spr); ld_spr_data(BANK_FX2, FX2_BASE, FX2_NT, fx2_tiles); VBK_REG = 0;
    ck_wpal = 0xFF;
    ld_spr_pal(BANK_A2, 0, 3, obj_pals);
    ld_spr_pal(BANK_A2, 7, 1, obj_pals + 28);
    ld_spr_pal(BANK_CK, 3, 4, ck_objpal);
    ld_bkg_data(BANK_A2, 0, 0, N_COMMON, common_tiles);
    ld_bkg_data(BANK_CK, 0, N_COMMON, CK_BG_NT, ck_bg_tiles);
    ld_bkg_data(BANK_CK, 1, 0, CK_DASH_NT, ck_dash_tiles);
    ld_bkg_data(BANK_CK, 1, BOSS_BASE, CK_BOSS_NT, ck_boss_tiles);
    ld_bkg_pal(BANK_CK, 0, 4, ck_bgpal);
    ld_bkg_pal(BANK_CK, 6, 2, ck_dashpal);
    set_pal_slot(5, BP_HUD);
    move_win(7, 96);
}
static void restart_from(uint8_t cp) {
    uint8_t i;
    screen_off();
    splits_off();
    view_load();
    memset(ck_type, 0, sizeof ck_type);
    memset(e_type, 0, sizeof e_type);
    memset(eb_on, 0, sizeof eb_on);
    comm_reset(); was_comm = 0;
    boss_clear();
    ck_charge = 0; ck_bx = 0; ck_by = 0; b_pose = 0xFF; b_fade = 0; b_stag = 0;
    boss_fade(0);
    ck_cp = cp; ck_pc = cp_pc[cp]; ck_tick = cp_tick[cp]; ck_sub = 0;
    if (cp == 3) { ck_pc = 0; while (waves[ck_pc].type != W_END) ck_pc++; ck_tick = waves[ck_pc].t; }
    parx = 0; pary = 0;
    ck_cx = 80; ck_cy = 52; p_inv = 90; fire_cd = 20; bolt_t = 0; spark_t = 0; ck_pop = 0; flash_t = 0; shake_t = 0; bomb_t = 0; lock = 0xFF;
    alert = cp == 0 ? 2 : 0; bar_last = 0xFF; bud_cd = 60;
    for (i = 0; i < 6; i++) { star_new(i); star_x[i] += star_vx[i] * (int16_t)(i * 6); star_y[i] += star_vy[i] * (int16_t)(i * 6); }
    st = ST_PLAY; st_t = 0;
    txt_fill(0, 0, 20, 6, txt_tile(' '), HA, 1);
    rim_draw(); hud_draw(); panel_draw();
    raster();
    music_play(MUS_STAGE5);
    oam_clear();
    screen_on(LCDC_COMM);
}
static uint8_t continue_prompt(void) {
    uint8_t t = 9, f;
    comm_reset(); was_comm = 0;
    oam_clear();
    music_play(MUS_OVER);
    txt_fill(0, 2, 20, 4, txt_tile(' '), HA, 1);
    txt_put(5, 3, "GAME OVER", 1, HA);
    for (f = 0; f < 100; f++) {
        wait_vbl_done();
        if (f == 50) txt_put(5, 3, "         ", 1, HA);
        if (f == 60) txt_put(5, 3, "GAME OVER", 1, HA);
    }
    f = 0;
    txt_fill(0, 2, 20, 4, txt_tile(' '), HA, 1);
    if (!HUNTER) {
        txt_put(5, 3, "TRY AGAIN!", 1, HA);
        txt_put(3, 4, "YOU CAN DO IT", 1, HA);
        wait_frames(100);
        return 1;
    }
    txt_put(5, 3, "CONTINUE?", 1, HA);
    txt_put(2, 4, "A: YES  START: NO", 1, HA);
    while (1) {
        wait_vbl_done(); input_poll();
        set_win_tile_xy(15, 3, txt_tile('0' + t));
        if (++f >= 60) { f = 0; if (t == 0) return 0; t--; sfx_play(SFX_SELECT); }
        if (joy_new & J_A) return 1;
        if (joy_new & J_START) return 0;
    }
}

// ------------------------------------------------------------------ main loop
uint8_t cockpit_run(void) BANKED {
    uint8_t paused = 0, sec_t = 0, was_flash = 0, was_bflash = 0, r;
    stage_id = CK_STAGE;
    ck_load();
    score = 0; hits_taken = 0; continues_used = 0; play_seconds = 0; midboss_killed = 0; kill_count = 0;
    lives = 3 + g_gear[GEAR_HULL]; hearts = MAXHEARTS; bombs = (HUNTER ? 3 : 5) + g_gear[GEAR_BOMBS]; wtype = 0; wlevel = g_gear[GEAR_CANNON];
    bud_n = g_gear[GEAR_KENNEL] >= 2 ? 1 : 0;
    bounty_pts = 0; bounty_out = OUT_NONE; mid_out = OUT_NONE;
    if (start_cp == 2 && wlevel < 1) wlevel = 1;
    next_extend = 30000UL;
    memset(cp_pc, 0, sizeof cp_pc); memset(cp_tick, 0, sizeof cp_tick);
    restart_from(start_cp == 2 ? 3 : 0);
    while (1) {
        wait_vbl_done();
        if (flash_t) {
            flash_t--;
            if (flash_t) { ld_bkg_pal(BANK_A2, 0, 1, white_pal); was_flash = 1; }
        }
        if (!flash_t && was_flash) { ld_bkg_pal(BANK_CK, 0, 1, ck_bgpal); was_flash = 0; }
        if (k_state == 2) {                                         // boss palettes: hit flash and beam charge blink
            r = k_hitflash ? 1 : (ck_charge && (ck_charge & (ck_charge < 50 ? 4 : 8))) ? 2 : 0;
            if (k_hitflash) k_hitflash--;
            if (r != was_bflash) { was_bflash = r; ld_bkg_pal(BANK_CK, 2, 2, r == 1 ? ck_boss_hit : r == 2 ? ck_boss_charge : ck_boss_fade + 24); }
        } else was_bflash = 0;
        input_poll();
        if ((joy_new & J_START) && st != ST_DYING) {
            paused ^= 1;
            sfx_play(SFX_SELECT);
            if (paused) txt_put(13, 1, "PAUSE  ", 1, HA);
            else hud_dirty = 1;
        }
        if (paused) continue;
        ck_loops++;
        if (++sec_t >= 60) { sec_t = 0; play_seconds++; }
        if (alert == 2 && play_seconds >= 8) alert = 0;

        comm_update();
        if (st != ST_DYING && st != ST_CLEAR) player_update();
        waves_update();
        objects_update();
        if (k_state) { boss_update(); if (pose_row < 8) boss_pose_step(); }
        if (bomb_t) bomb_t--;

        if (st == ST_DYING) {
            st_t++;
            shake_t = 4;
            if ((st_t & 7) == 1) { ck_boom(VB + 16 + (rnd() & 127), VB + 24 + (rnd() & 63)); sfx_play(SFX_EXPL_S); }
            if (st_t == 70) {
                alert = 0;
                if (lives > 1) {
                    lives--;
                    hearts = MAXHEARTS;
                    if (HUNTER) { if (wlevel > g_gear[GEAR_CANNON]) wlevel--; }
                    if (bombs < 3 + g_gear[GEAR_BOMBS]) bombs = 3 + g_gear[GEAR_BOMBS];
                    p_inv = 150;
                    st = k_state ? ST_BOSS : ST_PLAY;
                    hud_dirty = 1;
                } else {
                    if (!continue_prompt()) return RES_GAMEOVER;
                    continues_used++;
                    if (HUNTER) { score = 0; bounty_pts = 0; } else bounty_pts >>= 1;
                    lives = 3 + g_gear[GEAR_HULL]; hearts = MAXHEARTS; bombs = (HUNTER ? 3 : 5) + g_gear[GEAR_BOMBS];
                    if (HUNTER) wlevel = g_gear[GEAR_CANNON];
                    restart_from(ck_cp);
                    continue;
                }
            }
        }
        if (k_state == 22 && st != ST_CLEAR) { st = ST_CLEAR; st_t = 0; music_play(MUS_CLEAR); }
        if (st == ST_CLEAR) {
            st_t++;
            if (st_t >= 150) { oam_clear(); return RES_CLEAR; }
        }
        // dashboard: HUD, then the bounty panel whenever the crew is not on the comm
        if (ck_wpal != wtype) { ck_wpal = wtype; ld_spr_pal(BANK_FX2, 1, 1, fx2_wpn_pals + wtype * 4); }
        if (ck_pop && !--ck_pop) hud_dirty = 1;
        if (hud_dirty) hud_draw();
        if (comm_on != was_comm) { was_comm = comm_on; if (!comm_on) panel_draw(); }
        if (!comm_on) {
            if (alert != alert_last) alert_draw();
            else if ((frame_count & 15) == 0) { if (rating() != panel_rate) panel_rate_draw(); }
            else if ((frame_count & 15) == 8) { if (bar_cells() != panel_bar) panel_bar_draw(); }
        }
        raster();
        draw_all();
    }
}
