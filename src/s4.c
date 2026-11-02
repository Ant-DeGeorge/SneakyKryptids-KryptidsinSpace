// Stage 4 logic (bank 23): Frostbite Moon. Mine turrets on the ground, frost
// bats, snow snakes, ice storms, the Beast of Bray Road and the Wendigo.
#pragma bank 23
#include <gb/gb.h>
#include <stdint.h>
#include "core.h"
#include "assets.h"
#include "sound.h"
#include "objs.h"
#include "script_ids.h"

enum { C_TURRET = 1, C_BATS, C_SNAKE, C_GUST, C_MID, C_COMM, C_PAL, C_WARN, C_SLEEP, C_END };
#define H 0x80
typedef struct { uint16_t row; uint8_t cmd, a, b; } Cmd;
#define PL S4_ROW_PLAIN0
#define MI S4_ROW_MINES0
#define RF S4_ROW_RIFT0
#define AP S4_ROW_APPROACH0
static const Cmd script[] = {
    {4, C_COMM, CM4_DESCENT, 0},
    {14, C_BATS, 56, 0}, {22, C_BATS | H, 104, 0},
    {RAT(PL, 2), C_TURRET, 40, 0}, {RAT(PL, 3), C_COMM, CM4_TURRETS, 0}, {RAT(PL, 6), C_TURRET, 120, 0},
    {RAT(PL, 14), C_BATS, 80, 1}, {RAT(PL, 20), C_TURRET | H, 80, 0},
    {RAT(PL, 26), C_SNAKE, 0, 40}, {RAT(PL, 27), C_COMM, CM4_SNAKES, 0},
    {RAT(PL, 36), C_TURRET, 56, 0}, {RAT(PL, 38), C_TURRET, 104, 0},
    {RAT(PL, 44), C_BATS, 40, 0}, {RAT(PL, 46), C_BATS | H, 120, 0},
    {RAT(PL, 54), C_SLEEP, 80, 0},
    {RAT(PL, 60), C_SNAKE, 1, 56}, {RAT(PL, 64), C_TURRET, 80, 2},
    {RAT(PL, 72), C_BATS, 104, 0}, {RAT(PL, 76), C_TURRET | H, 40, 0},
    {RAT(PL, 84), C_SNAKE, 0, 32}, {RAT(PL, 86), C_SNAKE | H, 1, 72},
    {RAT(PL, 94), C_TURRET, 48, 0}, {RAT(PL, 96), C_TURRET, 112, 0},
    {RAT(PL, 104), C_BATS, 72, 0}, {RAT(PL, 110), C_BATS, 56, 1},
    {RAT(PL, 118), C_TURRET, 80, 0},
    {RAT(MI, 0), C_COMM, CM4_MINES, 0},
    {RAT(MI, 6), C_TURRET, 40, 0}, {RAT(MI, 8), C_TURRET, 120, 0},
    {S4_CP1 + 1, C_COMM, CM4_BEAST, 0}, {S4_CP1 + 4, C_MID, 0, 0},
    {RAT(MI, 36), C_BATS, 80, 0}, {RAT(MI, 40), C_TURRET, 56, 0}, {RAT(MI, 42), C_TURRET | H, 104, 0},
    {RAT(MI, 50), C_SNAKE, 1, 40}, {RAT(MI, 56), C_BATS, 48, 0},
    {RAT(MI, 64), C_TURRET, 80, 1}, {RAT(MI, 66), C_SLEEP, 56, 0},
    {RAT(MI, 74), C_BATS, 112, 0}, {RAT(MI, 78), C_SNAKE | H, 0, 64},
    {RAT(MI, 84), C_TURRET, 40, 0}, {RAT(MI, 86), C_TURRET, 120, 0},
    {RAT(RF, 2), C_GUST, 0, 0}, {RAT(RF, 3), C_COMM, CM4_WIND, 0},
    {RAT(RF, 10), C_BATS, 80, 0}, {RAT(RF, 14), C_TURRET, 104, 0},
    {RAT(RF, 22), C_SNAKE, 0, 48}, {RAT(RF, 24), C_SNAKE | H, 1, 24},
    {RAT(RF, 32), C_GUST, 1, 0}, {RAT(RF, 34), C_BATS, 56, 2},
    {RAT(RF, 42), C_TURRET, 48, 0}, {RAT(RF, 44), C_TURRET, 112, 0}, {RAT(RF, 46), C_TURRET | H, 80, 0},
    {RAT(RF, 54), C_SNAKE, 1, 40}, {RAT(RF, 58), C_BATS, 104, 0},
    {RAT(RF, 66), C_GUST, 0, 0}, {RAT(RF, 70), C_BATS, 40, 0}, {RAT(RF, 72), C_BATS | H, 120, 0},
    {RAT(RF, 80), C_TURRET, 72, 0}, {RAT(RF, 84), C_SNAKE, 0, 56},
    {RAT(RF, 92), C_BATS, 80, 1}, {RAT(RF, 96), C_GUST, 1, 0},
    {RAT(RF, 104), C_TURRET, 40, 0}, {RAT(RF, 106), C_TURRET, 120, 0},
    {RAT(RF, 112), C_SNAKE | H, 1, 48},
    {S4_CP2 + 14, C_PAL, 6, BP_WEN0}, {S4_CP2 + 14, C_PAL, 7, BP_WEN1},
    {S4_CP2 + 16, C_WARN, 0, 0},
    {S4_CP2 + 22, C_COMM, CM4_WENDIGO, 0},
    {0xFFFF, C_END, 0, 0}
};
static uint16_t script_i;

#define MAXSPN 4
static uint8_t sp_left[MAXSPN], sp_t[MAXSPN], sp_a[MAXSPN], sp_i[MAXSPN], sp_drop[MAXSPN];
static void spawner_add(uint8_t a, uint8_t n, uint8_t drop) {
    uint8_t s;
    for (s = 0; s < MAXSPN; s++) if (!sp_left[s]) {
        sp_a[s] = a; sp_left[s] = n; sp_t[s] = 0; sp_i[s] = 0; sp_drop[s] = drop; return;
    }
}
static void bat_new(uint8_t x, uint8_t drop) {
    int8_t i = e_new(E_FBAT, x, 8);
    if (i >= 0) { e_a[i] = x; if (drop) e_b[i] = drop - 1; }
}
static void spawners_update(void) {
    uint8_t s;
    for (s = 0; s < MAXSPN; s++) {
        if (!sp_left[s]) continue;
        if (sp_t[s]) { sp_t[s]--; continue; }
        bat_new(sp_a[s] + XO - 8 + (int8_t)((sp_i[s] & 1) ? 18 : -18), sp_i[s] == 0 ? sp_drop[s] : 0);
        sp_t[s] = 16;
        sp_i[s]++; sp_left[s]--;
    }
}

static void snake_new(uint8_t right, uint8_t y) {
    uint8_t ch = ch_alloc(), k;
    int8_t i;
    if (ch == 0xFF) return;
    i = e_new(E_SNAKE, right ? 180 : 0, y + YO);
    if (i < 0) { ch_used[ch] = 0; return; }
    e_c[i] = ch; e_a[i] = y + YO;
    e_vx[i] = right ? -(HUNTER ? 0x130 : 0xE0) : (HUNTER ? 0x130 : 0xE0);
    for (k = 0; k < NCSEG; k++) { ch_x[ch][k] = 0; ch_y[ch][k] = 0; }
}

static uint8_t wind_t, white_t;
static void gust(uint8_t right, uint8_t len) {
    wind = right ? 1 : -1;
    if (HUNTER && k_hpmax) wind *= 2;
    wind_t = len;
    sfx_play(SFX_WHOOSH);
}
static void gust_update(void) {                   // blowing snow (boss uses parts 12-13 only)
    uint8_t k, n, b;
    if (!wind_t) return;
    if (!--wind_t) { wind = 0; if (!k_hpmax) bp_n = 0; return; }
    if (k_hpmax) return;
    n = 6;
    for (k = 0; k < n; k++) {
        b = (uint8_t)(k * 29 + wind_t * (wind > 0 ? 3 : -3));
        bp_x[k] = XO + 8 + (b % 150);
        bp_y[k] = YO + 20 + k * 20 + (uint8_t)(wind_t * 2 + k * 7) % 12;
        bp_tile[k] = ST_SHOT_SPREAD; bp_pal[k] = 2; bp_hurt[k] = 0;
    }
    bp_n = n;
}

static void run_cmd(const Cmd *c, uint8_t live) {
    int8_t i;
    uint8_t cmd = c->cmd & 0x7F;
    if ((c->cmd & H) && !HUNTER) return;
    if (!live && cmd != C_PAL) return;
    switch (cmd) {
        case C_TURRET:
            i = e_new(E_TURRET, c->a + XO - 8, 8);
            if (i >= 0 && c->b) e_b[i] = c->b - 1;
            break;
        case C_BATS: spawner_add(c->a, HUNTER ? 4 : 3, c->b); break;
        case C_SNAKE: snake_new(c->a, c->b); break;
        case C_GUST: gust(c->a, HUNTER ? 150 : 110); break;
        case C_MID:
            if (!mb_done) {
                i = e_new(E_BEAST, 80 + XO - 16, 0);
                if (i >= 0) { mb_slot = i; mb_active = 1; mb_timer = 0; }
                music_play(MUS_BOSS);
            }
            break;
        case C_SLEEP: i = e_new(E_SLEEPER, c->a + XO - 8, 8); if (i >= 0) e_a[i] = c->a + XO - 8; break;
        case C_COMM: comm_queue_add(c->a); break;
        case C_PAL: set_pal_slot(c->a, c->b); break;
        case C_WARN: music_play(MUS_WARN); sfx_play(SFX_ALARM); break;
    }
}

// ------------------------------------------------------------------ enemies
static const uint8_t perch_x[4] = {24 + XO, 112 + XO, 40 + XO, 96 + XO};
static const uint8_t perch_y[4] = {10 + YO, 18 + YO, 26 + YO, 8 + YO};

static void enemies_update(void) {
    uint8_t i, k, x, y, cx, cy, ch;
    uint8_t pcx = px + 8, pcy = py + 12;
    for (i = 0; i < MAXE; i++) {
        switch (e_type[i]) {
        case E_TURRET:
            e_t[i]++;
            e_y[i] += 0x80;                                // fixed to the ground
            y = e_y[i] >> 8;
            if (y > 20 + YO && y < 116 + YO) {
                uint8_t per = HUNTER ? 56 : 120;
                if ((e_t[i] % per) == per - 12) e_st[i] = 1;  // barrel glows before firing
                if ((e_t[i] % per) == 0) {
                    cx = (e_x[i] >> 8) + 8; cy = y + 8;
                    k = aim(cx, cy, pcx, pcy);
                    eb_fire(cx, cy, k, HUNTER ? 24 : 13, 0);
                    if (HUNTER) eb_fire(cx, cy, k + 2, 22, 0);
                    e_st[i] = 0;
                    sfx_play(SFX_SHOT2);
                }
            }
            break;
        case E_FBAT:
            e_t[i]++;
            if (e_st[i] == 0) {                           // dive, steering toward the ship
                e_y[i] += HUNTER ? 0x150 : 0xF0;
                if ((e_t[i] & 1) && e_t[i] < (HUNTER ? 40 : 30)) {
                    x = (e_x[i] >> 8) + 8;
                    if (x + 3 < pcx) e_x[i] += 0x180; else if (x > pcx + 3) e_x[i] -= 0x180;
                }
                if ((e_y[i] >> 8) > py - 10) { e_st[i] = 1; e_vx[i] = ((e_x[i] >> 8) + 8 < pcx) ? -0x140 : 0x140; }
                if (HUNTER && e_t[i] == 20) { cx = (e_x[i] >> 8) + 8; cy = (e_y[i] >> 8) + 8; eb_fire(cx, cy, aim(cx, cy, pcx, pcy), 24, 0); }
            } else {                                      // peel away sideways
                e_x[i] += e_vx[i];
                e_y[i] += 0x80;
                if ((e_x[i] >> 8) < 2 || (e_x[i] >> 8) > 180) e_type[i] = E_NONE;
            }
            break;
        case E_SNAKE:
            e_t[i]++;
            e_x[i] += e_vx[i];
            e_a[i] += (e_t[i] & 1);                       // drifts down with the ground
            y = e_a[i] + (sin64[(e_t[i] * 4) & 63] >> 4);
            e_y[i] = (uint16_t)(y - 8) << 8;
            x = (e_x[i] >> 8) + 8;
            ch = e_c[i];
            if (ch < NCH) for (k = 0; k < NCSEG; k++) {
                ch_x[ch][k] = x - (e_vx[i] > 0 ? 1 : -1) * (int8_t)(8 * (k + 1));
                ch_y[ch][k] = e_a[i] + (sin64[(e_t[i] * 4 - (k + 1) * 10) & 63] >> 4);
            }
            if (HUNTER && (e_t[i] % 80) == 40) eb_fire(x, y, aim(x, y, pcx, pcy), 22, 0);
            if (e_t[i] >= 250 || e_a[i] > SCR_BOTTOM) { if (ch < NCH) ch_used[ch] = 0; e_type[i] = E_NONE; }
            break;
        case E_BEAST:
            e_t[i]++;
            if (e_st[i] == 0) {
                e_y[i] += 0x80;
                if ((e_y[i] >> 8) >= 10 + YO) { e_st[i] = 1; e_t[i] = 0; e_c[i] = 0; comm_queue_add(CM4_BEASTNAME); }
            } else if (e_st[i] == 1) {                    // crouched on a perch: slash or howl
                x = e_x[i] >> 8; y = e_y[i] >> 8;
                cx = x + 16; cy = y + 20;
                if (pcx < cx) e_fl[i] |= EF_FLIP; else e_fl[i] &= ~EF_FLIP;   // face the ship
                if (e_t[i] == (HUNTER ? 20 : 35)) {
                    k = aim(cx, cy, pcx, pcy);
                    if (HUNTER) { eb_fire(cx, cy, k, 26, 0); eb_fire(cx, cy, k - 2, 26, 0); eb_fire(cx, cy, k + 2, 26, 0);
                                  eb_fire(cx, cy, k - 4, 22, 0); eb_fire(cx, cy, k + 4, 22, 0); }
                    else { eb_fire(cx, cy, k, 14, 0); eb_fire(cx, cy, k - 3, 14, 0); eb_fire(cx, cy, k + 3, 14, 0); }
                    sfx_play(SFX_SHOT2);
                }
                if (e_t[i] == (HUNTER ? 50 : 80) && (e_c[i] & 1)) {
                    eb_ring(cx, cy - 6, HUNTER ? 12 : 6, e_c[i] & 2, HUNTER ? 16 : 10, 1);
                    sfx_play(SFX_ROAR);
                }
                if (e_t[i] >= (HUNTER ? 70 : 110)) {
                    e_st[i] = 2; e_t[i] = 0; e_c[i]++;
                    e_a[i] = perch_x[e_c[i] & 3] - 16; e_b[i] = 9;
                    e_vx[i] = (int16_t)(((int16_t)e_a[i] - (int16_t)(e_x[i] >> 8)) * 256 / 30);
                    e_vy[i] = (int16_t)(((int16_t)perch_y[e_c[i] & 3] - (int16_t)(e_y[i] >> 8)) * 256 / 30);
                    sfx_play(SFX_WHOOSH);
                }
                if (++mb_timer >= 2700) e_st[i] = 3;
            } else if (e_st[i] == 2) {                    // leap to the next perch (arc)
                e_x[i] += e_vx[i];
                e_y[i] += e_vy[i] + (e_t[i] < 15 ? -0x60 : 0x60);
                if (e_t[i] >= 30) { e_st[i] = 1; e_t[i] = 0; }
                mb_timer++;
            } else {
                e_y[i] -= 0x100;
                if ((e_y[i] >> 8) < 4) {
                    e_type[i] = E_NONE; mb_active = 0; mb_done = 1;
                    mid_out = OUT_FLEE; comm_queue_add(CM4_BEASTGONE); music_play(MUS_STAGE4);
                }
            }
            break;
        }
    }
}

// ------------------------------------------------------------------ Wendigo
static uint8_t k_phase, k_t, k_swayt, k_atk_i;
static uint8_t arm_mode[2], arm_t[2], arm_len[2];
static int8_t arm_base[2], arm_target[2];
enum { AM_IDLE, AM_RAISE, AM_GRAB, AM_BACK };
enum { A_SHARDS = 1, A_GRAB, A_HOWL, A_BATS, A_BLIZZARD };
static const uint8_t atk_p1[] = {A_SHARDS, A_GRAB, A_HOWL, A_SHARDS};
static const uint8_t atk_p2[] = {A_GRAB, A_SHARDS, A_BATS, A_HOWL, A_BLIZZARD, A_GRAB};
static const uint8_t atk_p3[] = {A_BLIZZARD, A_GRAB, A_HOWL, A_BATS, A_SHARDS, A_GRAB};

void s4_boss_start(void) BANKED {
    k_hpmax = HUNTER ? 1350 : 680;
    k_hp = k_hpmax;
    k_phase = 1; k_state = 0; k_t = 90; k_swayt = 0; k_sway = 0; k_atk_i = 0;
    k_hdx = 2; k_hw = 14; k_hy0 = YO + 4; k_hy1 = YO + 62;
    boss_name = boss_label[3];
    boss_face(S4_BV_OPEN);
    arm_mode[0] = arm_mode[1] = AM_IDLE; arm_len[0] = arm_len[1] = 5;
    white_t = 0;
    music_play(MUS_WENDIGO);
    sfx_play(SFX_ROAR);
    k_shake = 30;
}

static void arm_update(uint8_t n) {
    uint8_t k, a, L;
    int8_t side = n ? 1 : -1;
    int16_t x = (int16_t)(96 + k_sway + side * 20) << 7, y = (int16_t)(YO + 32 - k_retreat) << 7;
    uint8_t ph = k_swayt * 2 + n * 24;
    arm_t[n]++;
    switch (arm_mode[n]) {
    case AM_IDLE:
        if (arm_len[n] < 6 && (arm_t[n] & 3) == 0) arm_len[n]++;
        arm_base[n] = side * -3 + (sin64[ph & 63] >> 5);
        break;
    case AM_RAISE:                                   // telegraph: arm pulls back and shakes
        if (arm_len[n] > 3 && (arm_t[n] & 3) == 0) arm_len[n]--;
        arm_base[n] = arm_target[n] + ((arm_t[n] & 4) ? 2 : -2);
        if (arm_t[n] >= (HUNTER ? 45 : 75)) { arm_mode[n] = AM_GRAB; arm_t[n] = 0; sfx_play(SFX_WHOOSH); warn_t = 0; }
        break;
    case AM_GRAB:
        arm_base[n] = arm_target[n];
        if (arm_len[n] < (HUNTER ? 12 : 10) && (HUNTER || (arm_t[n] & 1))) arm_len[n]++;
        if (arm_t[n] >= 36) { arm_mode[n] = AM_BACK; arm_t[n] = 0; }
        break;
    case AM_BACK:
        if (arm_len[n] > 6) arm_len[n]--; else { arm_mode[n] = AM_IDLE; arm_t[n] = 0; }
        break;
    }
    if (k_state >= 20 && arm_len[n] > 1 && (arm_t[n] & 1)) arm_len[n]--;
    L = arm_len[n];
    a = (uint8_t)arm_base[n];
    for (k = 0; k < 6; k++) {
        uint8_t j = n * 6 + k;
        x += (int16_t)sin64[a & 63] * L;
        y += (int16_t)COS64(a & 63) * L;
        bp_x[j] = (uint8_t)(x >> 7); bp_y[j] = (uint8_t)(y >> 7);
        bp_tile[j] = k == 5 ? S4_ST_CLAW_TIP : S4_ST_CLAW_SEG;
        bp_pal[j] = 6 | 0x08;
        bp_hurt[j] = L >= 2;
        if (arm_mode[n] == AM_IDLE) a += (uint8_t)(side * (int8_t)(sin64[(ph + k * 8) & 63] >> 6));
    }
}

static void shards(uint8_t n, uint8_t spd) {
    uint8_t cx = 99 + k_sway, cy = YO + 26, d = aim(cx, cy, px + 8, py + 12), k;
    for (k = 0; k < n; k++) eb_fire(cx, cy, d - (n >> 1) + k, spd, 0);
    sfx_play(SFX_SPIT);
}

static void whiteout(void) {
    white_t = HUNTER ? 10 : 6;
    set_pal_slot(0, BP_WHITE4); set_pal_slot(1, BP_WHITE4);
    sfx_play(SFX_THUNDER);
}

static void boss_update(void) {
    uint8_t i, amp, ph;
    k_swayt++;
    amp = k_phase == 1 ? 12 : 18;
    if (white_t && !--white_t) { set_pal_slot(0, BP_SNOW); set_pal_slot(1, BP_SNOWD); }
    if (k_state < 20) {
        k_sway = (int8_t)(((int16_t)sin64[(k_swayt >> 1) & 63] * amp) >> 7);
        k_bob = sin64[(k_swayt * 2) & 63] >> 6;
        ph = k_hp * 3 <= k_hpmax ? 3 : (k_hp * 3 <= k_hpmax * 2 ? 2 : 1);
        if (ph > k_phase) {
            k_phase = ph; k_atk_i = 0; k_shake = 30; sfx_play(SFX_ROAR); boss_face(S4_BV_HOWL);
            boss_comm = ph == 2 ? CM4_HURT : CM4_CLAWS;
        }
    }
    if (k_hitflash) k_hitflash--;
    switch (k_state) {
    case 0:
        if (k_t) k_t--;
        if ((k_swayt & 127) == 70) boss_face(S4_BV_CLOSED);
        else if ((k_swayt & 127) == 78) boss_face(S4_BV_OPEN);
        if (HUNTER && (k_swayt % (k_phase == 1 ? 60 : 40)) == 0) shards(1, 24);
        if (!k_t) {
            uint8_t a;
            if (k_phase == 1) a = atk_p1[k_atk_i % sizeof(atk_p1)];
            else if (k_phase == 2 || !HUNTER) a = atk_p2[k_atk_i % sizeof(atk_p2)];
            else a = atk_p3[k_atk_i % sizeof(atk_p3)];
            k_atk_i++;
            k_state = a; k_t = 0;
            boss_face(a == A_HOWL || a == A_SHARDS ? S4_BV_HOWL : S4_BV_ANGRY);
            if (a == A_GRAB) {
                uint8_t n = (px + 8 < 96 + k_sway) ? 0 : 1;
                uint8_t rx = 96 + k_sway + (n ? 20 : -20);
                int8_t ang = (int8_t)(aim(rx, YO + 32, px + 8, py + 12) << 1);
                if (ang > 32) ang -= 64;
                if (ang > 22) ang = 22;
                if (ang < -22) ang = -22;
                arm_target[n] = ang; arm_mode[n] = AM_RAISE; arm_t[n] = 0;
                warn_x = px + 4; warn_t = HUNTER ? 45 : 75;
                sfx_play(SFX_ALARM);
                if (HUNTER && k_phase >= 2) {
                    rx = 96 + k_sway + (n ? -20 : 20);
                    ang = (int8_t)(aim(rx, YO + 32, px + 8, py + 12) << 1);
                    if (ang > 32) ang -= 64;
                    if (ang > 22) ang = 22;
                    if (ang < -22) ang = -22;
                    arm_target[n ^ 1] = ang; arm_mode[n ^ 1] = AM_RAISE; arm_t[n ^ 1] = 0;
                }
            }
        }
        break;
    case A_SHARDS:
        k_t++;
        if (k_t == 18) shards(HUNTER ? 7 : 3, HUNTER ? 22 : 13);
        if (k_t == 40) shards(HUNTER ? 6 : 2, HUNTER ? 22 : 13);
        if (k_phase >= 2 && k_t == 62) shards(HUNTER ? 7 : 3, HUNTER ? 22 : 13);
        if (k_t >= 85) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S4_BV_OPEN); }
        break;
    case A_GRAB:
        k_t++;
        if (k_t >= (HUNTER ? 120 : 160)) { k_state = 0; k_t = HUNTER ? 30 : 70; boss_face(S4_BV_OPEN); }
        break;
    case A_HOWL:
        k_t++;
        if (k_t == 15) { whiteout(); eb_ring(99 + k_sway, YO + 26, HUNTER ? 12 : 6, 0, HUNTER ? 16 : 10, 1); k_shake = 12; }
        if (k_t == 45) eb_ring(99 + k_sway, YO + 26, HUNTER ? 12 : 6, HUNTER ? 1 : 3, HUNTER ? 16 : 10, 0);
        if (k_t >= 85) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S4_BV_OPEN); }
        break;
    case A_BATS:
        k_t++;
        if (k_t == 10) { bat_new(XO + 24, 0); bat_new(XO + 120, 0); }
        if (HUNTER && k_t == 34) { bat_new(XO + 56, 0); bat_new(XO + 88, 0); }
        if (k_t >= 70) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S4_BV_OPEN); }
        break;
    case A_BLIZZARD:
        k_t++;
        if (k_t == 5) gust(px + 8 >= 96, HUNTER ? 140 : 110);
        if ((k_t % (HUNTER ? 5 : 11)) == 0 && k_t < 120)
            eb_fire(XO + 8 + (rnd() % 144), YO + 66, 0, HUNTER ? 20 : 12, 0);
        if (k_t >= 140) { k_state = 0; k_t = HUNTER ? 30 : 60; boss_face(S4_BV_OPEN); }
        break;
    case 20:
        k_t++;
        if ((k_t & 7) == 0) {
            boom_at(60 + (rnd() & 63) + k_sway, YO + 8 + (rnd() & 31));
            sfx_play((k_t & 15) ? SFX_EXPL_S : SFX_EXPL_L);
        }
        k_hitflash = (k_t & 4) ? 2 : 0;
        boss_face((k_t & 32) ? S4_BV_HOWL : S4_BV_CLOSED);
        if (k_t >= 150) { k_state = 21; k_t = 0; sfx_play(SFX_ROAR); }
        break;
    case 21:
        if (boss_outro()) k_state = 22;
        break;
    }
    if (k_state < 20 && k_hp == 0) {
        k_state = 20; k_t = 0;
        wind = 0; wind_t = 0;
        if (white_t) { white_t = 0; set_pal_slot(0, BP_SNOW); set_pal_slot(1, BP_SNOWD); }
        for (i = 0; i < MAXEB; i++) eb_on[i] = 0;
        for (i = 0; i < MAXE; i++) if (e_type[i] == E_FBAT) { e_type[i] = E_BOOM; e_t[i] = 0; }
        music_stop();
        add_score(26000);
        if (hits_taken == 0) add_score(10000);
        arm_mode[0] = arm_mode[1] = AM_BACK;
    }
    arm_update(0); arm_update(1);
    bp_n = 12;
    if (warn_t) warn_t--;
}

// ------------------------------------------------------------------ API
void s4_reset(uint8_t cp) BANKED {
    uint16_t srow = stage_defs[3].cp[cp];
    uint8_t s;
    for (s = 0; s < MAXSPN; s++) sp_left[s] = 0;
    wind = 0; wind_t = 0; white_t = 0;
    arm_mode[0] = arm_mode[1] = 0;
    set_pal_slot(6, BP_CREV); set_pal_slot(7, BP_FROSTSKY);
    script_i = 0;
    while (script[script_i].row < srow) { run_cmd(&script[script_i], 0); script_i++; }
}

void s4_tick(void) BANKED {
    if (st != ST_BOSS && !k_hpmax) {
        uint16_t srow = pos >> 3;
        while (script[script_i].row <= srow) { run_cmd(&script[script_i], 1); script_i++; }
    }
    spawners_update();
    gust_update();
    enemies_update();
    if (k_hpmax) boss_update();
}
