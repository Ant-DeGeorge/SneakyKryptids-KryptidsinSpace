// ROM0: tables, entity pools, comms, generic big-boss support and helpers.
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "assets.h"
#include "sound.h"
#include "objs.h"

const int8_t sin64[64] = {
    0,12,25,37,49,60,71,81,90,98,106,112,117,122,125,126,127,126,125,122,117,112,106,98,90,81,71,60,49,37,25,12,
    0,-12,-25,-37,-49,-60,-71,-81,-90,-98,-106,-112,-117,-122,-125,-126,-127,-126,-125,-122,-117,-112,-106,-98,-90,-81,-71,-60,-49,-37,-25,-12};
// 32 directions in 1/64 px; d=0 down, 8 right, 16 up, 24 left
const int8_t dsin[32] = {0,12,24,36,45,53,59,63,64,63,59,53,45,36,24,12,0,-12,-24,-36,-45,-53,-59,-63,-64,-63,-59,-53,-45,-36,-24,-12};
static const uint8_t atan_tab[17] = {0,0,1,1,1,2,2,2,2,3,3,3,3,3,4,4,4};

// ------------------------------------------------------------------ type tables
#define B1 0x08
const uint8_t e_tA[N_ETYPES] = {0, ST_EXPL0, ST_BUD0_A,
    S1_ST_KL_A, S1_ST_JELLY_A, S1_ST_SKIFF_A, S1_ST_ICE_A, S1_ST_GUN,
    S2_ST_MOTH_A, S2_ST_EEL_HEAD, S2_ST_CLOUD_A, S2_ST_MOTHMAN,
    S3_ST_IMP_A, S3_ST_CONE_A, S3_ST_WISP_A, S3_ST_GUN,
    S4_ST_TURRET_A, S4_ST_BAT_A, S4_ST_SNAKE_HEAD, S4_ST_BEAST};
const uint8_t e_tB[N_ETYPES] = {0, ST_EXPL0, ST_BUD0_B,
    S1_ST_KL_B, S1_ST_JELLY_B, S1_ST_SKIFF_B, S1_ST_ICE_B, S1_ST_GUN,
    S2_ST_MOTH_B, S2_ST_EEL_SEG, S2_ST_CLOUD_B, S2_ST_MOTHMAN,
    S3_ST_IMP_B, S3_ST_CONE_B, S3_ST_WISP_B, S3_ST_GUN,
    S4_ST_TURRET_B, S4_ST_BAT_B, S4_ST_SNAKE_SEG, S4_ST_BEAST};
const uint8_t e_pal[N_ETYPES] = {0, 7, 2,
    4 | B1, 5 | B1, 6 | B1, 5 | B1, 6 | B1,
    4 | B1, 5 | B1, 4 | B1, 6 | B1,
    4 | B1, 5 | B1, 6 | B1, 4 | B1,
    4 | B1, 5 | B1, 5 | B1, 6 | B1};
const uint8_t e_style[N_ETYPES] = {SY_NONE, SY_BOOM, SY_SLEEP,
    SY_16, SY_16, SY_16, SY_16HP, SY_32,
    SY_16, SY_EEL, SY_16, SY_32,
    SY_16, SY_16, SY_BLINK, SY_32,
    SY_16HP, SY_16, SY_EEL, SY_32};
const uint8_t e_tf[N_ETYPES] = {0, TF_NOHIT | TF_NOBODY, TF_NOHIT | TF_NOBODY,
    0, 0, 0, TF_NOBODY, TF_MID,
    0, 0, TF_NOBODY, TF_MID,
    0, TF_BURST, 0, TF_MID,
    TF_NOBODY, 0, 0, TF_MID};
static const uint8_t hp_c[N_ETYPES] = {1, 1, 1,  2, 8, 4, 8, 100,   2, 8, 10, 110,   2, 4, 5, 120,   6, 2, 9, 130};
static const uint8_t hp_h[N_ETYPES] = {1, 1, 1,  3, 14, 6, 14, 230, 3, 14, 16, 240,  3, 6, 8, 255,   10, 3, 15, 255};
static const uint16_t pts[N_ETYPES] = {0, 0, 0,  100, 300, 300, 150, 5000,  100, 400, 250, 6000,  120, 150, 300, 6000,   250, 120, 450, 7000};

// ------------------------------------------------------------------ state
uint8_t stage_id;
const StageDef *SD;
uint8_t st, st_t, scroll_on;
uint16_t pos;
uint8_t px, py;
uint8_t hud_dirty;
uint32_t next_extend;
uint8_t kill_count;

uint8_t e_type[MAXE], e_hp[MAXE], e_t[MAXE], e_flash[MAXE], e_a[MAXE], e_b[MAXE], e_c[MAXE], e_st[MAXE], e_fl[MAXE];
uint16_t e_x[MAXE], e_y[MAXE];
int16_t e_vx[MAXE], e_vy[MAXE];
uint8_t eb_on[MAXEB], eb_big[MAXEB];
uint16_t eb_x[MAXEB], eb_y[MAXEB];
int16_t eb_vx[MAXEB], eb_vy[MAXEB];
uint8_t pk_on[MAXPK], pk_kind[MAXPK], pk_t[MAXPK];
uint16_t pk_x[MAXPK], pk_y[MAXPK];
uint8_t ch_used[NCH], ch_x[NCH][NCSEG], ch_y[NCH][NCSEG];
uint8_t hz_on[NHZ], hz_x[NHZ], hz_t[NHZ], hz_y0[NHZ];
uint8_t mb_active, mb_done, mb_slot;
uint16_t mb_timer;

uint16_t k_hp, k_hpmax;
uint8_t k_state, k_hitflash, k_retreat, k_shake, boss_comm, k_face;
int8_t k_sway, k_bob;
int8_t k_hdx; uint8_t k_hw, k_hy0, k_hy1;
const char *boss_name;
const char * const boss_label[4] = {"KRAKEN", "THUNDERBIRD", "JERSEY DEVIL", "WENDIGO"};
uint8_t bp_n, bp_x[NBP], bp_y[NBP], bp_tile[NBP], bp_pal[NBP], bp_hurt[NBP];
uint8_t warn_x, warn_t;
int8_t wind;

// mid-boss capture threshold: points scored in the mission so far [mode][stage]
static const uint16_t mid_cap_thr[2][N_STAGES] = {{10000, 10000, 10000, 10000}, {15000, 16000, 16000, 16000}};
extern const uint8_t mid_comm[], boss_out_comm[];
// boss outcome thresholds (points this mission, boss bonus included) [mode][stage]
static const uint32_t cap_thr[2][N_STAGES] = {{45000, 45000, 45000, 45000}, {66000, 72000, 74000, 72000}};
static const uint32_t kill_thr[2][N_STAGES] = {{30000, 30000, 30000, 30000}, {48000, 52000, 54000, 52000}};
uint8_t outro_t;

// boss slots 6/7 -> sky colour (gone) or white on sky (flash), keeping colour 0 = sky
static void boss_pal(uint8_t white) {
    uint16_t vp[4];
    ld_copy(BANK_A2, vp, bg_pal_lib + (uint16_t)cur_bp[0] * 4, 2);
    vp[1] = vp[2] = vp[3] = white ? 0x7FFF : vp[0];
    ld_bkg_pal(BANK_A2, 6, 1, vp); ld_bkg_pal(BANK_A2, 7, 1, vp);
}
#define boss_vanish() boss_pal(0)

// After the boss is beaten: capture it (rune net), finish it, or watch it flee.
// Called by each stage's boss logic every frame of k_state 21; returns 1 when done.
uint8_t boss_outro(void) {
    uint8_t cx = 96 + k_sway;
    if (outro_t == 0) {
        bounty_out = bounty_pts >= cap_thr[HUNTER][stage_id] ? OUT_CAPTURE :
                     bounty_pts >= kill_thr[HUNTER][stage_id] ? OUT_KILL : OUT_FLEE;
        k_hitflash = 0;
        sfx_play(bounty_out == OUT_FLEE ? SFX_ROAR : SFX_POWER);
    }
    outro_t++;
    // the crew calls it once the moment has played out (the comm box would hide the boss)
    if (outro_t == (bounty_out == OUT_CAPTURE ? 114 : bounty_out == OUT_KILL ? 74 : 60))
        comm_queue_add(boss_out_comm[stage_id * 3 + bounty_out - 1]);
    switch (bounty_out) {
    case OUT_CAPTURE:                       // runes close in, the boss is sealed and pulled into the hold
        if (outro_t > 20 && outro_t < 100 && (outro_t & 7) == 0) sfx_play(SFX_TICK);
        if (outro_t == 100) { boss_pal(1); sfx_play(SFX_ONEUP); k_shake = 10; }
        if (outro_t == 112) boss_vanish();
        return outro_t >= 200;
    case OUT_KILL:                          // final barrage, then it is gone
        if (outro_t < 70 && (outro_t & 3) == 0) {
            boom_at(cx - 40 + (rnd() & 63) + 8, YO + (rnd() & 47));
            sfx_play((outro_t & 7) ? SFX_EXPL_S : SFX_EXPL_L);
        }
        if (outro_t < 70) k_shake = 4;
        if (outro_t < 64 && (outro_t & 8)) boss_pal(1);
        else if (outro_t < 64) { set_pal_slot(6, cur_bp[6]); set_pal_slot(7, cur_bp[7]); }
        if (outro_t == 70) { boss_vanish(); sfx_play(SFX_EXPL_L); }
        return outro_t >= 170;
    default:                                // it breaks away and escapes
        if (k_retreat < 70) k_retreat++;
        return outro_t >= 140;
    }
}

// ------------------------------------------------------------------ helpers
uint8_t aim(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    int16_t dx = (int16_t)x1 - x0, dy = (int16_t)y1 - y0;
    uint8_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy, a;
    if (ay >= ax) a = ay ? atan_tab[((uint16_t)ax << 4) / ay] : 0;
    else a = 8 - atan_tab[((uint16_t)ay << 4) / ax];
    if (dy >= 0) return dx >= 0 ? a : (uint8_t)(32 - a) & 31;
    return dx >= 0 ? (uint8_t)(16 - a) : (uint8_t)(16 + a);
}

// fire an enemy bullet from centre (cx, cy) in direction d (0-31), speed in 1/16 px/frame
void eb_fire(uint8_t cx, uint8_t cy, uint8_t d, uint8_t spd, uint8_t big) {
    uint8_t i;
    if (cy < 20 || cy > SCR_BOTTOM) return;
    for (i = 0; i < MAXEB; i++) if (!eb_on[i]) {
        eb_on[i] = 1; eb_big[i] = big;
        eb_x[i] = (uint16_t)(cx - (big ? 4 : 3)) << 8;
        eb_y[i] = (uint16_t)(cy - (big ? 4 : 3)) << 8;
        d &= 31;
        eb_vx[i] = ((int16_t)dsin[d] * spd) >> 2;
        eb_vy[i] = ((int16_t)DCOS(d) * spd) >> 2;
        return;
    }
}
void eb_ring(uint8_t cx, uint8_t cy, uint8_t n, uint8_t phase, uint8_t spd, uint8_t big) {
    uint8_t k, step = 32 / n;
    for (k = 0; k < n; k++) eb_fire(cx, cy, phase + k * step, spd, big);
}

void add_score(uint16_t p) {
    score += p;
    bounty_pts += p;
    if (HUNTER && score >= next_extend) {
        if (lives < 9) lives++;
        sfx_play(SFX_ONEUP);
        next_extend = next_extend == 30000UL ? 80000UL : next_extend + 100000UL;
    }
    hud_dirty = 1;
}

int8_t e_new(uint8_t type, uint8_t x, uint8_t y) {
    uint8_t i;
    for (i = 0; i < MAXE; i++) if (e_type[i] == E_NONE) {
        e_type[i] = type; e_x[i] = (uint16_t)x << 8; e_y[i] = (uint16_t)y << 8;
        e_vx[i] = 0; e_vy[i] = 0; e_t[i] = 0; e_flash[i] = 0; e_st[i] = 0; e_a[i] = x; e_b[i] = 9; e_c[i] = 0; e_fl[i] = 0;
        e_hp[i] = HUNTER ? hp_h[type] : hp_c[type];
        return (int8_t)i;
    }
    return -1;
}
void boom_at(uint8_t x, uint8_t y) {
    int8_t i = e_new(E_BOOM, x, y);
    if (i >= 0) e_hp[i] = 0;
}
uint8_t ch_alloc(void) {
    uint8_t c;
    for (c = 0; c < NCH; c++) if (!ch_used[c]) { ch_used[c] = 1; return c; }
    return 0xFF;
}

void spawn_pickup(uint8_t x, uint8_t y, uint8_t kind) {
    uint8_t i;
    for (i = 0; i < MAXPK; i++) if (!pk_on[i]) {
        pk_on[i] = 1; pk_kind[i] = kind; pk_t[i] = 0;
        pk_x[i] = (uint16_t)x << 8; pk_y[i] = (uint16_t)y << 8;
        return;
    }
}

void e_kill(uint8_t i) {
    uint8_t x = e_x[i] >> 8, y = e_y[i] >> 8, t = e_type[i], tf = e_tf[t], drop = e_b[i];
    kill_count++;
    add_score(pts[t]);
    if (tf & TF_MID) {
        e_type[i] = E_NONE;
        boom_at(x, y); boom_at(x + 16, y + 4); boom_at(x + 8, y + 12);
        sfx_play(SFX_EXPL_L);
        spawn_pickup(x + 8, y + 8, PK_WEAP);
        spawn_pickup(x + 20, y + 4, HUNTER ? PK_BOMB : PK_HEART);
        mb_active = 0; mb_done = 1; midboss_killed = 1;
        // a clean, high-scoring run bags the mid-boss alive
        mid_out = bounty_pts >= mid_cap_thr[HUNTER][stage_id] ? OUT_CAPTURE : OUT_KILL;
        comm_queue_add(mid_comm[stage_id * 3 + mid_out - 1]);
        if (mid_out == OUT_CAPTURE) { add_score(3000); sfx_play(SFX_ONEUP); }
        return;
    }
    if ((t == E_EEL || t == E_SNAKE) && e_c[i] < NCH) ch_used[e_c[i]] = 0;
    e_type[i] = E_BOOM; e_t[i] = 0; e_hp[i] = 0;
    sfx_play(SFX_EXPL_S);
    if (tf & TF_BURST) {
        if (HUNTER) eb_ring(x + 8, y + 8, 8, 2, 16, 0);
        else eb_ring(x + 8, y + 8, 4, 4, 9, 0);
    }
    if (drop < 9) {
        if (drop == 0) spawn_pickup(x, y, PK_WEAP);
        else if (drop == 1) spawn_pickup(x, y, PK_BOMB);
        else if (drop == 2) spawn_pickup(x, y, HUNTER ? PK_1UP : PK_HEART);
    }
    if (!HUNTER && (kill_count % 30) == 0 && hearts < 3) spawn_pickup(x, y, PK_HEART);
}

void e_damage(uint8_t i, uint8_t dmg) {
    uint8_t t = e_type[i];
    if (t == E_NONE || (e_tf[t] & TF_NOHIT) || (e_fl[i] & EF_HIDDEN)) return;
    e_flash[i] = 3;
    if (e_hp[i] > dmg) { e_hp[i] -= dmg; sfx_play(SFX_TICK); }
    else e_kill(i);
}

uint8_t cur_bp[8];
void set_pal_slot(uint8_t slot, uint8_t bp) {
    cur_bp[slot] = bp;
    ld_bkg_pal(BANK_A2, slot, 1, bg_pal_lib + (uint16_t)bp * 4);
}

// ------------------------------------------------------------------ comms (window rows 2-5, shown at the top by a raster split)
extern const uint8_t comm_who[];
extern const char * const comm_text[];
extern const char * const crew_names[];
extern const uint8_t mini_cap_tiles[], mini_static_tiles[], mini_forge_tiles[], mini_doc_tiles[], mini_vex_tiles[];
extern const uint16_t mini_cap_pals[], mini_static_pals[], mini_forge_pals[], mini_doc_pals[], mini_vex_pals[];
static const uint8_t * const mini_tiles_tab[5] = {mini_cap_tiles, mini_static_tiles, mini_forge_tiles, mini_doc_tiles, mini_vex_tiles};
static const uint16_t * const mini_pals_tab[5] = {mini_cap_pals, mini_static_pals, mini_forge_pals, mini_doc_pals, mini_vex_pals};
uint8_t comm_on;
static uint8_t comm_t, comm_len, comm_shown, comm_cx, comm_cy;
static const char *comm_str;
static uint8_t comm_queue[4], comm_qn;

static void comm_start(uint8_t id) {
    uint8_t who = comm_who[id], i;
    ld_bkg_data(BANK_A3, 1, COMM_TILE, 16, mini_tiles_tab[who]);
    ld_bkg_pal(BANK_A3, 4, 1, mini_pals_tab[who]);
    txt_fill(0, 2, 20, 4, txt_tile(' '), 5 | 0x80, 1);
    for (i = 0; i < 16; i++) {
        set_win_tile_xy(i & 3, 2 + (i >> 2), COMM_TILE + i);
        VBK_REG = 1; set_win_tile_xy(i & 3, 2 + (i >> 2), 4 | 0x08 | 0x80); VBK_REG = 0;
    }
    txt_put(5, 2, crew_names[who], 1, 5 | 0x80);
    comm_str = comm_text[id];
    comm_len = (uint8_t)strlen(comm_str);
    comm_shown = 0; comm_t = 0; comm_on = 1; comm_cx = 0; comm_cy = 0;
    sfx_play(SFX_BLIP);
}
// The crew panel covers the top 32 px of the playfield, so mid-wave lines wait
// until nothing is entering or fighting up there (or ~4 s pass). Boss fights,
// the stage intro and outro show at once as before.
static uint8_t comm_wait;
static uint8_t comm_area_busy(void) {
    uint8_t i, y;
    if (k_hpmax || st != ST_PLAY) return 0;
    for (i = 0; i < MAXE; i++) {
        if (e_type[i] < E_KL) continue;
        y = e_y[i] >> 8;
        if (y < YO + 40 || y > 232) return 1;
    }
    for (i = 0; i < MAXEB; i++) if (eb_on[i] == 1 && (eb_y[i] >> 8) < YO + 36) return 1;
    return 0;
}
void comm_queue_add(uint8_t id) {
    if (!comm_on && !comm_qn && !comm_area_busy()) comm_start(id);
    else if (comm_qn < 4) { comm_queue[comm_qn++] = id; if (comm_qn == 1) comm_wait = 0; }
}
void comm_reset(void) { comm_on = 0; comm_qn = 0; }
static void comm_next(void) {
    uint8_t id = comm_queue[0];
    comm_queue[0] = comm_queue[1]; comm_queue[1] = comm_queue[2]; comm_queue[2] = comm_queue[3];
    comm_qn--;
    comm_wait = 0;
    comm_start(id);
}
void comm_update(void) {
    char c;
    if (!comm_on) {
        if (comm_qn && (++comm_wait > 240 || !comm_area_busy())) comm_next();
        return;
    }
    comm_t++;
    if (comm_shown < comm_len && (comm_t & 1)) {
        c = comm_str[comm_shown];
        if (c == '\n') { comm_cx = 0; comm_cy++; }
        else {
            set_win_tile_xy(5 + comm_cx, 3 + comm_cy, txt_tile(c));
            comm_cx++;
            if ((comm_shown & 3) == 0) sfx_play(SFX_BLIP);
        }
        comm_shown++;
        comm_t = 0;
    } else if (comm_shown >= comm_len && comm_t > (HUNTER ? 100 : MIDMODE ? 125 : 150)) {
        comm_on = 0;
        comm_wait = 0;
    }
}

// ------------------------------------------------------------------ generic big boss
static uint8_t k_swap_pos[64];
static uint8_t bar_x0, bar_len, bar_last;
void boss_load(void) {
    ld_bkg_data(SD->dbank, 1, BOSS_BASE, SD->n_btiles, SD->btiles);
    ld_copy(SD->dbank, k_swap_pos, SD->bswap, SD->n_swap);
    k_face = 0xFF;
}
void boss_face(uint8_t v) {
    uint8_t i, n = SD->n_swap;
    const uint8_t *src;
    if (v == k_face) return;
    k_face = v;
    src = SD->bvar + (uint16_t)v * n * 16;
    for (i = 0; i < n; i++) ld_bkg_data(SD->dbank, 1, k_swap_pos[i], 1, src + (uint16_t)i * 16);
}
void boss_bar(void) {
    uint8_t n, i;
    if (!k_hpmax) return;
    if (bar_last == 0xFF) {
        uint8_t l = (uint8_t)strlen(boss_name);
        txt_put(0, 0, boss_name, 1, 5 | 0x80);
        bar_x0 = l + 1; bar_len = 20 - bar_x0;
    }
    n = (uint8_t)(((uint32_t)k_hp * bar_len + k_hpmax - 1) / k_hpmax);
    if (n == bar_last) return;
    bar_last = n;
    for (i = 0; i < bar_len; i++) set_win_tile_xy(bar_x0 + i, 0, i < n ? T_HUD_BAR : T_HUD_BARE);
}
void boss_clear(void) {
    k_hpmax = 0; k_hp = 0; k_retreat = 0; k_sway = 0; k_bob = 0; k_shake = 0; k_state = 0; k_hitflash = 0;
    boss_comm = 0; warn_t = 0; bp_n = 0; wind = 0; bar_last = 0xFF; outro_t = 0;
    memset(hz_on, 0, sizeof hz_on);
}
