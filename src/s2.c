// Stage 2 logic (bank 10): Stormwall Nebula. Mothlings, storm eels,
// thunderclouds, Mothman (mid-boss) and the Thunderbird.
#pragma bank 10
#include <gb/gb.h>
#include <stdint.h>
#include "core.h"
#include "assets.h"
#include "sound.h"
#include "objs.h"
#include "script_ids.h"

enum { C_MOTHS = 1, C_MOTHSIDE, C_EEL, C_CLOUD, C_MID, C_COMM, C_PAL, C_WARN, C_SLEEP, C_STORMY, C_END };
#define H 0x80
typedef struct { uint16_t row; uint8_t cmd, a, b; } Cmd;
#define O S2_ROW_OPEN0
#define ST S2_ROW_STORM0
#define RL S2_ROW_RELAY0
#define EY S2_ROW_EYE0
#define AP S2_ROW_APPROACH0
static const Cmd script[] = {
    {6, C_COMM, CM2_FRONT, 0},
    {16, C_MOTHS, 40, 0}, {26, C_MOTHS, 104, 0}, {34, C_MOTHS | H, 72, 0},
    {RAT(ST, 4), C_COMM, CM2_MOTHS, 0},
    {RAT(ST, 8), C_MOTHSIDE, 0, 0}, {RAT(ST, 16), C_MOTHSIDE, 1, 0}, {RAT(ST, 20), C_MOTHS | H, 80, 0},
    {RAT(ST, 26), C_EEL, 0, 40}, {RAT(ST, 28), C_COMM, CM2_EELS, 0},
    {RAT(ST, 38), C_MOTHS, 56, 1}, {RAT(ST, 44), C_MOTHS, 104, 0},
    {RAT(ST, 52), C_EEL, 1, 56}, {RAT(ST, 54), C_EEL | H, 0, 24},
    {RAT(ST, 60), C_SLEEP, 80, 0},
    {RAT(ST, 66), C_CLOUD, 48, 0}, {RAT(ST, 67), C_COMM, CM2_CLOUDS, 0},
    {RAT(ST, 76), C_MOTHSIDE, 0, 0}, {RAT(ST, 78), C_MOTHSIDE | H, 1, 0},
    {RAT(ST, 86), C_CLOUD, 112, 0}, {RAT(ST, 88), C_CLOUD | H, 40, 0},
    {RAT(ST, 96), C_EEL, 0, 32}, {RAT(ST, 100), C_MOTHS, 80, 2},
    {RAT(ST, 108), C_MOTHS, 40, 0}, {RAT(ST, 110), C_MOTHS, 120, 0},
    {RAT(ST, 118), C_EEL, 1, 48}, {RAT(ST, 122), C_CLOUD | H, 80, 0},
    {RAT(RL, 2), C_COMM, CM2_PYLONS, 0},
    {RAT(RL, 8), C_MOTHS, 64, 0}, {RAT(RL, 14), C_MOTHS | H, 96, 0},
    {S2_CP1 + 2, C_COMM, CM2_MOTHMAN, 0}, {S2_CP1 + 4, C_MID, 0, 0},
    {RAT(RL, 34), C_MOTHSIDE, 1, 0}, {RAT(RL, 40), C_CLOUD, 64, 0},
    {RAT(RL, 48), C_EEL, 0, 40}, {RAT(RL, 52), C_MOTHS, 104, 1},
    {RAT(RL, 60), C_CLOUD, 104, 0}, {RAT(RL, 62), C_CLOUD | H, 40, 0},
    {RAT(RL, 70), C_MOTHSIDE, 0, 0}, {RAT(RL, 74), C_EEL | H, 1, 32},
    {RAT(RL, 80), C_SLEEP, 56, 0},
    {RAT(RL, 86), C_MOTHS, 72, 0}, {RAT(RL, 92), C_EEL, 1, 56},
    {RAT(EY, 0), C_STORMY, 1, 0}, {RAT(EY, 2), C_COMM, CM2_EYE, 0},
    {RAT(EY, 8), C_CLOUD, 56, 0}, {RAT(EY, 10), C_CLOUD, 104, 0},
    {RAT(EY, 18), C_MOTHS, 80, 0}, {RAT(EY, 22), C_EEL, 0, 32}, {RAT(EY, 24), C_EEL | H, 1, 64},
    {RAT(EY, 32), C_CLOUD, 80, 2}, {RAT(EY, 34), C_MOTHSIDE | H, 1, 0},
    {RAT(EY, 42), C_MOTHS, 40, 0}, {RAT(EY, 44), C_MOTHS, 120, 0},
    {RAT(EY, 52), C_EEL, 1, 40}, {RAT(EY, 56), C_CLOUD | H, 120, 0}, {RAT(EY, 58), C_CLOUD, 40, 0},
    {RAT(EY, 66), C_MOTHSIDE, 0, 0}, {RAT(EY, 70), C_MOTHSIDE, 1, 0},
    {RAT(EY, 78), C_EEL, 0, 48}, {RAT(EY, 80), C_MOTHS, 80, 1},
    {RAT(EY, 88), C_CLOUD, 64, 0}, {RAT(EY, 90), C_CLOUD | H, 96, 0},
    {RAT(EY, 98), C_MOTHS, 56, 0}, {RAT(EY, 100), C_MOTHS | H, 104, 0},
    {RAT(EY, 106), C_EEL, 1, 32}, {RAT(EY, 110), C_EEL | H, 0, 56},
    {RAT(AP, 0), C_STORMY, 0, 0},
    {S2_CP2 - 2, C_PAL, 6, BP_TB0}, {S2_CP2 - 2, C_PAL, 7, BP_TB1},
    {S2_CP2 + 2, C_WARN, 0, 0},
    {S2_CP2 + 10, C_COMM, CM2_TBIRD, 0},
    {0xFFFF, C_END, 0, 0}
};
static uint16_t script_i;
static uint8_t stormy, flash_left, thunder_t;

// ------------------------------------------------------------------ spawners
#define MAXSPN 4
static uint8_t sp_kind[MAXSPN], sp_left[MAXSPN], sp_t[MAXSPN], sp_a[MAXSPN], sp_i[MAXSPN], sp_drop[MAXSPN];

static void spawner_add(uint8_t kind, uint8_t a, uint8_t n, uint8_t drop) {
    uint8_t s;
    for (s = 0; s < MAXSPN; s++) if (!sp_left[s]) {
        sp_kind[s] = kind; sp_a[s] = a; sp_left[s] = n; sp_t[s] = 0; sp_i[s] = 0; sp_drop[s] = drop; return;
    }
}
static void spawners_update(void) {
    uint8_t s;
    int8_t i;
    for (s = 0; s < MAXSPN; s++) {
        if (!sp_left[s]) continue;
        if (sp_t[s]) { sp_t[s]--; continue; }
        if (sp_kind[s] == C_MOTHS) {
            i = e_new(E_MOTH, sp_a[s] + XO - 8 + (int8_t)((sp_i[s] & 1) ? 10 : -10), 8);
            if (i >= 0) { e_a[i] = (uint8_t)(e_x[i] >> 8); if (sp_i[s] == 0) e_b[i] = sp_drop[s] ? sp_drop[s] - 1 : 9; }
            sp_t[s] = 12;
        } else {
            i = e_new(E_MOTH, sp_a[s] ? 172 : 4, 20 + YO);
            if (i >= 0) { e_st[i] = 1; e_vx[i] = sp_a[s] ? -0x180 : 0x180; e_vy[i] = 0x30; }
            sp_t[s] = 12;
        }
        sp_i[s]++; sp_left[s]--;
    }
}

static void run_cmd(const Cmd *c, uint8_t live) {
    int8_t i;
    uint8_t cmd = c->cmd & 0x7F;
    if ((c->cmd & H) && !HUNTER) return;
    if (!live && cmd != C_PAL && cmd != C_STORMY) return;
    switch (cmd) {
        case C_MOTHS: spawner_add(C_MOTHS, c->a, HUNTER ? 6 : 4, c->b); break;
        case C_MOTHSIDE: spawner_add(C_MOTHSIDE, c->a, HUNTER ? 5 : 4, 0); break;
        case C_EEL: {
            uint8_t ch = ch_alloc();
            if (ch == 0xFF) break;
            i = e_new(E_EEL, c->a ? 180 : 0, c->b + YO);
            if (i < 0) { ch_used[ch] = 0; break; }
            e_c[i] = ch; e_a[i] = c->b + YO;
            e_vx[i] = c->a ? -(HUNTER ? 0x140 : 0xF0) : (HUNTER ? 0x140 : 0xF0);
            { uint8_t k; for (k = 0; k < NCSEG; k++) { ch_x[ch][k] = 0; ch_y[ch][k] = 0; } }
            break;
        }
        case C_CLOUD:
            i = e_new(E_CLOUD, c->a + XO - 8, 8);
            if (i >= 0) { e_a[i] = c->a + XO - 8; if (c->b) e_b[i] = c->b - 1; }
            break;
        case C_MID:
            if (!mb_done) {
                i = e_new(E_MOTHMAN, 80 + XO - 16, 0);
                if (i >= 0) { mb_slot = i; mb_active = 1; mb_timer = 0; }
                music_play(MUS_BOSS);
            }
            break;
        case C_SLEEP: i = e_new(E_SLEEPER, c->a + XO - 8, 8); if (i >= 0) e_a[i] = c->a + XO - 8; break;
        case C_COMM: comm_queue_add(c->a); break;
        case C_PAL: set_pal_slot(c->a, c->b); break;
        case C_WARN: music_play(MUS_WARN); sfx_play(SFX_ALARM); break;
        case C_STORMY: stormy = c->a; break;
    }
}

static int8_t hz_new(uint8_t x, uint8_t y0, uint8_t warn) {
    uint8_t i;
    for (i = 0; i < NHZ; i++) if (!hz_on[i]) {
        hz_on[i] = 1; hz_x[i] = x; hz_y0[i] = y0; hz_t[i] = warn;
        sfx_play(SFX_ALARM);
        return (int8_t)i;
    }
    return -1;
}

// ------------------------------------------------------------------ enemies
static void enemies_update(void) {
    uint8_t i, k, x, y, cx, cy, ch;
    uint8_t pcx = px + 8, pcy = py + 12;
    for (i = 0; i < MAXE; i++) {
        switch (e_type[i]) {
        case E_MOTH:
            e_t[i]++;
            if (e_st[i] == 0) {
                // flutter down, drifting toward the ship's engine glow
                e_y[i] += HUNTER ? 0x90 : 0x70;
                if ((e_t[i] & 3) == 0 && (e_y[i] >> 8) < py) {
                    x = e_a[i] + 8;
                    if (x + 4 < pcx) e_a[i]++; else if (x > pcx + 4) e_a[i]--;
                }
                e_x[i] = ((uint16_t)(e_a[i] + (sin64[(e_t[i] * 4) & 63] >> 4))) << 8;
            } else {
                e_x[i] += e_vx[i]; e_y[i] += e_vy[i];
                e_vy[i] += 4; if (e_vy[i] > 0x150) e_vy[i] = 0x150;
                if (e_vx[i] > 0) { if (e_vx[i] > 0x20) e_vx[i] -= 5; } else if (e_vx[i] < -0x20) e_vx[i] += 5;
            }
            if (HUNTER && (e_t[i] == 40 || (e_t[i] == 90 && (rnd() & 1)))) {
                cx = (e_x[i] >> 8) + 8; cy = (e_y[i] >> 8) + 8;
                eb_fire(cx, cy, aim(cx, cy, pcx, pcy), 24, 0);
            }
            break;
        case E_EEL:
            e_t[i]++;
            e_x[i] += e_vx[i];
            y = e_a[i] + (sin64[(e_t[i] * 3) & 63] >> 3);
            e_y[i] = (uint16_t)(y - 8) << 8;
            x = (e_x[i] >> 8) + 8;
            ch = e_c[i];
            if (ch < NCH) for (k = 0; k < NCSEG; k++) {
                ch_x[ch][k] = x - (e_vx[i] > 0 ? 1 : -1) * (int8_t)(9 * (k + 1));
                ch_y[ch][k] = e_a[i] + (sin64[(e_t[i] * 3 - (k + 1) * 9) & 63] >> 3);
            }
            if (HUNTER && (e_t[i] % 70) == 35) {
                cy = y;
                eb_fire(x, cy, aim(x, cy, pcx, pcy), 22, 0);
            }
            if (x > 200 || (e_vx[i] < 0 && x < 4) || (e_vx[i] > 0 && x > 190)) {
                if (e_t[i] > 30) { if (ch < NCH) ch_used[ch] = 0; e_type[i] = E_NONE; }
            }
            break;
        case E_CLOUD:
            e_t[i]++;
            e_y[i] += HUNTER ? 0x50 : 0x40;
            e_x[i] = ((uint16_t)(e_a[i] + (sin64[(e_t[i]) & 63] >> 5))) << 8;
            y = e_y[i] >> 8;
            if (y > 20 + YO && y < 90 + YO) {
                uint8_t period = HUNTER ? 110 : 200;
                if ((e_t[i] % period) == 60) hz_new((e_x[i] >> 8) + 8, y + 14, HUNTER ? 45 : 75);
            }
            break;
        case E_MOTHMAN:
            e_t[i]++;
            if (e_st[i] == 0) {
                e_y[i] += 0x80;
                if ((e_y[i] >> 8) >= 12 + YO) { e_st[i] = 1; e_t[i] = 0; }
            } else if (e_st[i] == 1) {
                // figure eight around the relay beacons
                e_x[i] = ((uint16_t)(80 + XO - 16 + (sin64[(uint8_t)(e_t[i] >> 1) & 63] * (HUNTER ? 46 : 40) >> 7))) << 8;
                e_y[i] = ((uint16_t)(14 + YO + (sin64[e_t[i] & 63] >> 4))) << 8;
                x = e_x[i] >> 8; y = e_y[i] >> 8;
                // red-eye glare
                if (HUNTER) {
                    if ((e_t[i] % 60) == 0) {
                        cx = x + 16; cy = y + 10;
                        k = aim(cx, cy, pcx, pcy);
                        eb_fire(cx - 4, cy, k, 30, 0); eb_fire(cx + 4, cy, k, 30, 0);
                        eb_fire(cx - 4, cy, k - 2, 26, 0); eb_fire(cx + 4, cy, k + 2, 26, 0);
                        sfx_play(SFX_SHOT2);
                    }
                    if ((e_t[i] % 160) == 120) { eb_ring(x + 16, y + 16, 10, (e_t[i] >> 3) & 3, 16, 1); sfx_play(SFX_SPIT); }
                } else {
                    if ((e_t[i] % 90) == 45) {
                        cx = x + 16; cy = y + 10;
                        eb_fire(cx, cy, aim(cx, cy, pcx, pcy), 15, 0);
                        sfx_play(SFX_SHOT2);
                    }
                    if ((e_t[i] % 240) == 200) {
                        eb_fire(x + 16, y + 20, 0, 12, 1); eb_fire(x + 16, y + 20, 4, 12, 1); eb_fire(x + 16, y + 20, 28, 12, 1);
                        sfx_play(SFX_SPIT);
                    }
                }
                if (++mb_timer >= 2700) e_st[i] = 2;
            } else {
                e_y[i] -= 0x100;
                if ((e_y[i] >> 8) < 4) {
                    e_type[i] = E_NONE; mb_active = 0; mb_done = 1;
                    mid_out = OUT_FLEE; comm_queue_add(CM2_MMGONE); music_play(MUS_STAGE2);
                }
            }
            break;
        }
    }
}

// ------------------------------------------------------------------ storm ambience
static void storm_update(void) {
    if (flash_left) {
        flash_left--;
        if (!flash_left) { set_pal_slot(2, BP_CLOUD); set_pal_slot(6, BP_ZAP); }
        return;
    }
    if (!stormy || k_hpmax) return;
    if (thunder_t) { thunder_t--; return; }
    set_pal_slot(2, BP_CLOUDF); set_pal_slot(6, BP_ZAPF);
    flash_left = 4;
    sfx_play(SFX_THUNDER);
    thunder_t = 150 + (rnd() & 127);
}

// ------------------------------------------------------------------ Thunderbird
static uint8_t k_phase, k_t, k_swayt, k_atk_i, gust_t, sweep_x;
enum { A_BOLTS = 1, A_FEATHER, A_GUST, A_EELS, A_CHARGE, A_SWEEP };
static const uint8_t atk_p1[] = {A_BOLTS, A_FEATHER, A_GUST, A_FEATHER};
static const uint8_t atk_p2[] = {A_BOLTS, A_FEATHER, A_EELS, A_CHARGE, A_GUST, A_FEATHER};
static const uint8_t atk_p3[] = {A_SWEEP, A_FEATHER, A_CHARGE, A_EELS, A_BOLTS, A_GUST};

void s2_boss_start(void) BANKED {
    k_hpmax = HUNTER ? 1150 : 560;
    k_hp = k_hpmax;
    k_phase = 1; k_state = 0; k_t = 90; k_swayt = 0; k_sway = 0; k_atk_i = 0;
    k_hdx = 0; k_hw = 20; k_hy0 = YO + 6; k_hy1 = YO + 58;
    boss_name = boss_label[1];
    boss_face(S2_BV_OPEN);
    bp_n = 0; gust_t = 0; wind = 0;
    stormy = 0;
    music_play(MUS_TBIRD);
    sfx_play(SFX_ROAR);
    k_shake = 30;
}

static void feathers(void) {
    uint8_t l = 96 + k_sway - 50, r = 96 + k_sway + 50, y = YO + 24, k;
    uint8_t dl = aim(l, y, px + 8, py + 12), dr = aim(r, y, px + 8, py + 12);
    if (HUNTER) {
        for (k = 0; k < 5; k++) { eb_fire(l, y, dl - 2 + k, 22, 0); eb_fire(r, y, dr - 2 + k, 22, 0); }
    } else {
        for (k = 0; k < 3; k++) { eb_fire(l, y, dl - 2 + k * 2, 13, 0); eb_fire(r, y, dr - 2 + k * 2, 13, 0); }
    }
    sfx_play(SFX_SPIT);
}

static void gust_parts(void) {
    uint8_t k, n = 6;
    for (k = 0; k < n; k++) {
        uint8_t x = (uint8_t)(k * 29 + gust_t * (wind > 0 ? 3 : -3));
        bp_x[k] = XO + 8 + (x % 150);
        bp_y[k] = YO + 70 + k * 12;
        bp_tile[k] = ST_SHOT_SPREAD; bp_pal[k] = 2; bp_hurt[k] = 0;
    }
    bp_n = n;
}

static void boss_update(void) {
    uint8_t i, amp;
    k_swayt++;
    amp = k_phase == 1 ? 10 : 16;
    if (k_state < 20) {
        k_sway = (int8_t)(((int16_t)sin64[(k_swayt >> 1) & 63] * amp) >> 7);
        k_bob = sin64[(k_swayt * 3) & 63] >> 5;
        uint8_t ph = k_hp * 3 <= k_hpmax ? 3 : (k_hp * 3 <= k_hpmax * 2 ? 2 : 1);
        if (ph > k_phase) {
            k_phase = ph; k_atk_i = 0; k_shake = 30; sfx_play(SFX_ROAR); boss_face(S2_BV_SCREECH);
            boss_comm = ph == 2 ? CM2_HURT : CM2_CHARGE;
        }
    }
    if (k_hitflash) k_hitflash--;
    if (gust_t) {
        gust_t--;
        gust_parts();
        if (!gust_t) { wind = 0; bp_n = 0; }
    }
    switch (k_state) {
    case 0:
        if (k_t) k_t--;
        if ((k_swayt & 127) == 90) boss_face(S2_BV_CLOSED);
        else if ((k_swayt & 127) == 98) boss_face(S2_BV_OPEN);
        if (HUNTER && (k_swayt % (k_phase == 1 ? 70 : 45)) == 0) {
            uint8_t cx = 96 + k_sway, cy = YO + 40;
            eb_fire(cx, cy, aim(cx, cy, px + 8, py + 12), 24, 0);
        }
        if (!k_t) {
            uint8_t a;
            if (k_phase == 1) a = atk_p1[k_atk_i % sizeof(atk_p1)];
            else if (k_phase == 2 || !HUNTER) a = atk_p2[k_atk_i % sizeof(atk_p2)];
            else a = atk_p3[k_atk_i % sizeof(atk_p3)];
            k_atk_i++;
            k_state = a; k_t = 0;
            boss_face(a == A_CHARGE ? S2_BV_CHARGED : (a == A_GUST ? S2_BV_SCREECH : S2_BV_ANGRY));
        }
        break;
    case A_BOLTS:
        k_t++;
        if (k_t == 10) {
            hz_new(px + 8, YO + 60, HUNTER ? 45 : 80);
            if (HUNTER) { hz_new(px + 8 > 70 ? px + 8 - 44 : px + 8 + 44, YO + 60, 45); }
            if (HUNTER && k_phase >= 2) hz_new(px + 8 > 90 ? px + 8 - 80 : px + 8 + 80, YO + 60, 55);
        }
        if (k_t >= (HUNTER ? 100 : 140)) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S2_BV_OPEN); }
        break;
    case A_FEATHER:
        k_t++;
        if (k_t == 20) feathers();
        if ((HUNTER && k_t == 44) || (k_phase >= 2 && k_t == 68)) feathers();
        if (k_t >= 90) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S2_BV_OPEN); }
        break;
    case A_GUST:
        k_t++;
        if (k_t == 20) {
            wind = (px + 8 < 96) ? -1 : 1;           // push toward the nearer edge
            if (HUNTER) wind *= 2;
            gust_t = HUNTER ? 150 : 120;
            boss_comm = CM2_GUST;
            sfx_play(SFX_WHOOSH);
        }
        if (HUNTER && k_t > 20 && (k_t % 30) == 0) feathers();
        if (k_t >= 160) { k_state = 0; k_t = HUNTER ? 30 : 60; boss_face(S2_BV_OPEN); }
        break;
    case A_EELS:
        k_t++;
        if (k_t == 15 || (HUNTER && k_t == 45)) {
            uint8_t ch = ch_alloc();
            if (ch != 0xFF) {
                int8_t e = e_new(E_EEL, (k_t == 15) ? 0 : 180, YO + 72 + ((k_t == 15) ? 0 : 24));
                if (e >= 0) {
                    e_c[e] = ch; e_a[e] = YO + 72 + ((k_t == 15) ? 0 : 24);
                    e_vx[e] = (k_t == 15) ? 0xE0 : -0xE0;
                } else ch_used[ch] = 0;
            }
            sfx_play(SFX_SPIT);
        }
        if (k_t >= 80) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S2_BV_OPEN); }
        break;
    case A_CHARGE:
        k_t++;
        if (k_t < 60 && (k_t & 7) == 0) k_hitflash = 2;
        if (k_t == 60) {
            uint8_t cx = 96 + k_sway, cy = YO + 36;
            if (HUNTER) { eb_ring(cx, cy, 12, k_t & 3, 18, 1); }
            else eb_ring(cx, cy, 6, 2, 11, 1);
            k_shake = 10;
            sfx_play(SFX_THUNDER);
        }
        if (HUNTER && k_t == 80) eb_ring(96 + k_sway, YO + 36, 12, 1, 14, 0);
        if (k_t >= 110) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S2_BV_OPEN); }
        break;
    case A_SWEEP:
        k_t++;
        if (k_t == 1) sweep_x = (px + 8 < 96) ? XO + 12 : XO + 148;
        if ((k_t % 18) == 0 && k_t <= 90) {
            hz_new(sweep_x, YO + 60, 40);
            sweep_x = (sweep_x < 96) ? sweep_x + 28 : sweep_x - 28;
        }
        if (k_t >= 150) { k_state = 0; k_t = 40; boss_face(S2_BV_OPEN); }
        break;
    case 20:
        k_t++;
        wind = 0; gust_t = 0; bp_n = 0;
        if ((k_t & 7) == 0) {
            boom_at(56 + (rnd() & 63) + k_sway, YO + 6 + (rnd() & 31));
            sfx_play((k_t & 15) ? SFX_EXPL_S : SFX_EXPL_L);
        }
        k_hitflash = (k_t & 4) ? 2 : 0;
        boss_face((k_t & 32) ? S2_BV_SCREECH : S2_BV_CLOSED);
        if (k_t >= 150) { k_state = 21; k_t = 0; sfx_play(SFX_ROAR); }
        break;
    case 21:
        if (boss_outro()) k_state = 22;
        break;
    }
    if (k_state < 20 && k_hp == 0) {
        k_state = 20; k_t = 0;
        for (i = 0; i < MAXEB; i++) eb_on[i] = 0;
        for (i = 0; i < NHZ; i++) hz_on[i] = 0;
        for (i = 0; i < MAXE; i++) if (e_type[i] == E_EEL) { if (e_c[i] < NCH) ch_used[e_c[i]] = 0; e_type[i] = E_BOOM; e_t[i] = 0; }
        music_stop();
        add_score(22000);
        if (hits_taken == 0) add_score(10000);
    }
}

// ------------------------------------------------------------------ API
void s2_reset(uint8_t cp) BANKED {
    uint16_t srow = stage_defs[1].cp[cp];
    uint8_t s;
    for (s = 0; s < MAXSPN; s++) sp_left[s] = 0;
    stormy = 0; flash_left = 0; thunder_t = 60;
    script_i = 0;
    while (script[script_i].row < srow) { run_cmd(&script[script_i], 0); script_i++; }
}

void s2_tick(void) BANKED {
    if (st != ST_BOSS && !k_hpmax) {
        uint16_t srow = pos >> 3;
        while (script[script_i].row <= srow) { run_cmd(&script[script_i], 1); script_i++; }
    }
    spawners_update();
    enemies_update();
    storm_update();
    if (k_hpmax) boss_update();
}
