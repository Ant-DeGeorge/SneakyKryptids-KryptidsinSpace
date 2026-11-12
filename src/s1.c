// Stage 1 logic (bank 9): The Rimeveil Lane. Krakenlings, jellies, gremlin
// skiffs, ice, Nubs' gunship and the Space Kraken.
#pragma bank 9
#include <gb/gb.h>
#include <stdint.h>
#include "core.h"
#include "assets.h"
#include "sound.h"
#include "objs.h"
#include "script_ids.h"

enum { C_KLLINE = 1, C_KLV, C_KLSWOOP, C_JELLY, C_SKIFF, C_ICE, C_MID, C_COMM, C_PAL, C_WARN, C_SLEEP, C_END };
#define H 0x80
typedef struct { uint16_t row; uint8_t cmd, a, b; } Cmd;
#define R S1_ROW_
#define PAL_TENT_ROW (S1_ROW_GAP1 + 2)
#define PAL_K_ROW (S1_ROW_APPROACH0 + 4)
static const Cmd script[] = {
    {8, C_COMM, CM1_SWARM, 0},
    {22, C_KLLINE, 48, 0}, {30, C_KLLINE, 112, 0}, {38, C_KLV, 80, 0}, {44, C_KLLINE | H, 40, 0},
    {50, C_SKIFF, 0, 0}, {54, C_COMM, CM1_PODS, 0},
    {RAT(S1_ROW_OPEN0, 24), C_KLSWOOP, 0, 0}, {RAT(S1_ROW_OPEN0, 32), C_KLSWOOP, 1, 0}, {RAT(S1_ROW_OPEN0, 38), C_KLSWOOP | H, 0, 0},
    {RAT(S1_ROW_OPEN0, 42), C_KLV, 52, 0}, {RAT(S1_ROW_OPEN0, 46), C_KLV | H, 108, 0},
    {RAT(S1_ROW_OPEN0, 52), C_SKIFF, 1, 9}, {RAT(S1_ROW_OPEN0, 54), C_SKIFF | H, 0, 9},
    {RAT(S1_ROW_OPEN0, 58), C_SLEEP, 80, 0},
    {RAT(S1_ROW_OPEN0, 64), C_JELLY, 80, 0}, {RAT(S1_ROW_OPEN0, 68), C_COMM, CM1_CANCEL, 0},
    {RAT(S1_ROW_OPEN0, 70), C_KLLINE, 32, 0}, {RAT(S1_ROW_OPEN0, 72), C_KLLINE, 128, 0},
    {RAT(S1_ROW_OPEN0, 78), C_SKIFF, 0, 1},
    {RAT(S1_ROW_OPEN0, 86), C_JELLY, 44, 0}, {RAT(S1_ROW_OPEN0, 88), C_JELLY | H, 116, 0},
    {RAT(S1_ROW_OPEN0, 96), C_KLV, 80, 0}, {RAT(S1_ROW_OPEN0, 98), C_KLSWOOP | H, 1, 0},
    {RAT(S1_ROW_OPEN0, 104), C_SKIFF, 1, 0},
    {RAT(S1_ROW_OPEN0, 112), C_KLLINE, 64, 0}, {RAT(S1_ROW_OPEN0, 114), C_KLLINE | H, 96, 0},
    {RAT(S1_ROW_OPEN0, 120), C_JELLY, 120, 0}, {RAT(S1_ROW_OPEN0, 122), C_JELLY | H, 40, 0},
    {RAT(S1_ROW_ICE0, -2), C_COMM, CM1_ICE, 0},
    {RAT(S1_ROW_ICE0, 4), C_ICE, 64, 0}, {RAT(S1_ROW_ICE0, 6), C_KLSWOOP, 1, 0}, {RAT(S1_ROW_ICE0, 12), C_ICE, 104, 0}, {RAT(S1_ROW_ICE0, 14), C_ICE | H, 40, 0},
    {RAT(S1_ROW_ICE0, 20), C_KLV, 80, 0}, {RAT(S1_ROW_ICE0, 26), C_JELLY, 116, 0}, {RAT(S1_ROW_ICE0, 28), C_ICE, 36, 0},
    {RAT(S1_ROW_ICE0, 34), C_SKIFF, 1, 0}, {RAT(S1_ROW_ICE0, 42), C_ICE, 72, 0}, {RAT(S1_ROW_ICE0, 44), C_ICE | H, 112, 0},
    {RAT(S1_ROW_ICE0, 50), C_KLLINE, 52, 0}, {RAT(S1_ROW_ICE0, 52), C_KLLINE, 108, 0},
    {RAT(S1_ROW_ICE0, 58), C_JELLY, 80, 0}, {RAT(S1_ROW_ICE0, 60), C_KLSWOOP | H, 0, 0},
    {RAT(S1_ROW_ICE0, 66), C_ICE, 40, 0}, {RAT(S1_ROW_ICE0, 68), C_ICE, 120, 0},
    {RAT(S1_ROW_ICE0, 74), C_SKIFF, 0, 2},
    {RAT(S1_ROW_ICE0, 82), C_KLV, 80, 0}, {RAT(S1_ROW_ICE0, 84), C_KLV | H, 40, 0}, {RAT(S1_ROW_ICE0, 90), C_ICE, 92, 0},
    {S1_CP1 + 1, C_COMM, CM1_NUBS, 0}, {S1_CP1 + 4, C_MID, 0, 0}, {S1_CP1 + 8, C_ICE, 48, 0}, {S1_CP1 + 11, C_ICE, 112, 0}, {S1_CP1 + 14, C_ICE | H, 80, 0},
    {RAT(S1_ROW_ICE0, 112), C_KLSWOOP, 0, 0}, {RAT(S1_ROW_ICE0, 116), C_KLSWOOP, 1, 0}, {RAT(S1_ROW_ICE0, 124), C_ICE, 60, 0}, {RAT(S1_ROW_ICE0, 126), C_ICE | H, 100, 0},
    {PAL_TENT_ROW, C_PAL, 6, BP_TENT},
    {RAT(S1_ROW_CONVOY0, 4), C_COMM, CM1_TANKER, 0},
    {RAT(S1_ROW_CONVOY0, 8), C_KLLINE, 40, 0}, {RAT(S1_ROW_CONVOY0, 10), C_KLLINE, 120, 0},
    {RAT(S1_ROW_CONVOY0, 16), C_SKIFF, 1, 0}, {RAT(S1_ROW_CONVOY0, 18), C_SKIFF | H, 0, 9},
    {RAT(S1_ROW_CONVOY0, 24), C_JELLY, 60, 0}, {RAT(S1_ROW_CONVOY0, 26), C_JELLY | H, 100, 0},
    {RAT(S1_ROW_CONVOY0, 32), C_KLV, 80, 0}, {RAT(S1_ROW_CONVOY0, 34), C_KLSWOOP | H, 1, 0},
    {RAT(S1_ROW_CONVOY0, 38), C_SLEEP, 56, 0},
    {RAT(S1_ROW_CONVOY0, 40), C_KLSWOOP, 0, 0}, {RAT(S1_ROW_CONVOY0, 44), C_KLSWOOP, 1, 0},
    {RAT(S1_ROW_CONVOY0, 50), C_SKIFF, 0, 1},
    {RAT(S1_ROW_WRECK0, 2), C_COMM, CM1_MARKS, 0},
    {RAT(S1_ROW_WRECK0, 6), C_JELLY, 60, 0}, {RAT(S1_ROW_WRECK0, 8), C_JELLY | H, 100, 0},
    {RAT(S1_ROW_WRECK0, 14), C_ICE, 40, 0}, {RAT(S1_ROW_WRECK0, 16), C_ICE, 120, 0},
    {RAT(S1_ROW_WRECK0, 22), C_SKIFF, 1, 0}, {RAT(S1_ROW_WRECK0, 26), C_SKIFF | H, 0, 9},
    {RAT(S1_ROW_WRECK0, 30), C_KLV, 80, 0}, {RAT(S1_ROW_WRECK0, 32), C_KLV | H, 32, 0},
    {RAT(S1_ROW_WRECK0, 38), C_JELLY, 120, 0}, {RAT(S1_ROW_WRECK0, 40), C_KLSWOOP, 0, 0},
    {RAT(S1_ROW_WRECK0, 46), C_ICE, 72, 0}, {RAT(S1_ROW_WRECK0, 48), C_ICE | H, 104, 0},
    {RAT(S1_ROW_WRECK0, 54), C_SKIFF, 0, 1},
    {RAT(S1_ROW_WRECK0, 62), C_JELLY, 40, 0}, {RAT(S1_ROW_WRECK0, 64), C_JELLY | H, 120, 0},
    {RAT(S1_ROW_WRECK0, 70), C_KLSWOOP, 1, 0}, {RAT(S1_ROW_WRECK0, 74), C_KLSWOOP, 0, 0},
    {RAT(S1_ROW_WRECK0, 80), C_KLV, 60, 0}, {RAT(S1_ROW_WRECK0, 82), C_KLV, 108, 0},
    {RAT(S1_ROW_WRECK0, 88), C_SKIFF, 1, 2}, {RAT(S1_ROW_WRECK0, 90), C_SKIFF | H, 0, 0},
    {RAT(S1_ROW_WRECK0, 96), C_JELLY, 80, 0},
    {RAT(S1_ROW_WRECK0, 104), C_KLLINE, 32, 0}, {RAT(S1_ROW_WRECK0, 106), C_KLLINE, 80, 0}, {RAT(S1_ROW_WRECK0, 108), C_KLLINE | H, 128, 0},
    {RAT(S1_ROW_WRECK0, 114), C_KLSWOOP, 0, 0}, {RAT(S1_ROW_WRECK0, 120), C_KLV, 80, 0}, {RAT(S1_ROW_WRECK0, 124), C_KLSWOOP | H, 1, 0},
    {RAT(S1_ROW_APPROACH0, 4), C_COMM, CM1_HUGE, 0},
    {PAL_K_ROW, C_PAL, 6, BP_K0}, {PAL_K_ROW, C_PAL, 7, BP_K1},
    {S1_CP2 + 2, C_WARN, 0, 0},
    {S1_CP2 + 10, C_COMM, CM1_KRAKEN, 0},
    {0xFFFF, C_END, 0, 0}
};
static uint16_t script_i;

// ------------------------------------------------------------------ spawners
#define MAXSPN 4
static uint8_t sp_kind[MAXSPN], sp_left[MAXSPN], sp_t[MAXSPN], sp_a[MAXSPN], sp_i[MAXSPN];

static void spawn_kl(uint8_t x, uint8_t swoop_side) {
    int8_t i;
    if (swoop_side) {
        i = e_new(E_KL, swoop_side == 1 ? 4 : 172, 16 + YO);
        if (i < 0) return;
        e_st[i] = 1;
        e_vx[i] = swoop_side == 1 ? 0x1C0 : -0x1C0;
        e_vy[i] = 0x20;
    } else e_new(E_KL, x, 8);
}
static void spawners_update(void) {
    uint8_t s;
    static const int8_t voff[5] = {0, -18, 18, -36, 36};
    for (s = 0; s < MAXSPN; s++) {
        if (!sp_left[s]) continue;
        if (sp_t[s]) { sp_t[s]--; continue; }
        switch (sp_kind[s]) {
            case C_KLLINE: spawn_kl(sp_a[s] + XO - 8, 0); sp_t[s] = 16; break;
            case C_KLV:    spawn_kl((uint8_t)(sp_a[s] + XO - 8 + voff[sp_i[s]]), 0); sp_t[s] = (sp_i[s] & 1) ? 0 : 10; break;
            case C_KLSWOOP: spawn_kl(0, sp_a[s] + 1); sp_t[s] = 14; break;
        }
        sp_i[s]++; sp_left[s]--;
    }
}
static void spawner_add(uint8_t kind, uint8_t a, uint8_t n) {
    uint8_t s;
    for (s = 0; s < MAXSPN; s++) if (!sp_left[s]) {
        sp_kind[s] = kind; sp_a[s] = a; sp_left[s] = n; sp_t[s] = 0; sp_i[s] = 0; return;
    }
}

static void run_cmd(const Cmd *c, uint8_t live) {
    int8_t i;
    uint8_t cmd = c->cmd & 0x7F;
    if ((c->cmd & H) && !HUNTER) return;
    if (!live && cmd != C_PAL) return;
    switch (cmd) {
        case C_KLLINE: spawner_add(C_KLLINE, c->a, HUNTER ? 5 : 4); break;
        case C_KLV: spawner_add(C_KLV, c->a, 5); break;
        case C_KLSWOOP: spawner_add(C_KLSWOOP, c->a, HUNTER ? 5 : 4); break;
        case C_JELLY: i = e_new(E_JELLY, c->a + XO - 8, 8); if (i >= 0) e_a[i] = c->a + XO - 8; break;
        case C_SKIFF:
            i = e_new(E_SKIFF, c->a ? 172 : 4, 22 + YO);
            if (i >= 0) { e_vx[i] = c->a ? -0x150 : 0x150; e_b[i] = c->b; }
            break;
        case C_ICE:
            i = e_new(E_ICE, c->a + XO - 8, 8);
            if (i >= 0) e_vx[i] = (rnd() & 1) ? 0x18 : -0x18;
            break;
        case C_MID:
            if (!mb_done) {
                i = e_new(E_GUN, 80 + XO - 16, 0);
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
static void enemies_update(void) {
    uint8_t i, x, y, cx, cy, d;
    uint8_t pcx = px + 8, pcy = py + 12;
    for (i = 0; i < MAXE; i++) {
        switch (e_type[i]) {
        case E_KL:
            e_t[i]++;
            if (e_st[i] == 0) {
                e_y[i] += HUNTER ? 0xC0 : 0xA0;
                e_x[i] = ((uint16_t)(e_a[i] + (sin64[(e_t[i] * 3) & 63] >> 3))) << 8;
            } else {
                e_x[i] += e_vx[i]; e_y[i] += e_vy[i];
                e_vy[i] += 3;
                if (e_vy[i] > 0x140) e_vy[i] = 0x140;
                if (e_vx[i] > 0) { if (e_vx[i] > 8) e_vx[i] -= 4; } else if (e_vx[i] < -8) e_vx[i] += 4;
            }
            if (HUNTER && ((e_t[i] == 50 && (rnd() & 1)) || (e_t[i] == 100 && (rnd() & 3) == 0))) {
                cx = (e_x[i] >> 8) + 8; cy = (e_y[i] >> 8) + 8;
                eb_fire(cx, cy, aim(cx, cy, pcx, pcy), 26, 0);
            }
            break;
        case E_JELLY:
            e_t[i]++;
            if (e_st[i] == 0) { e_y[i] += 0x60; if ((e_y[i] >> 8) >= 64 + YO - 24) { e_st[i] = 1; e_t[i] = 0; } }
            else if (e_st[i] == 1) {
                e_x[i] = ((uint16_t)(e_a[i] + (sin64[(e_t[i] * 2) & 63] >> 4))) << 8;
                cx = (e_x[i] >> 8) + 8; cy = (e_y[i] >> 8) + 8;
                if (HUNTER) {
                    if (e_t[i] == 30 || e_t[i] == 80 || e_t[i] == 130) {
                        for (d = (e_t[i] == 80 ? 2 : 0); d < 32; d += 4) eb_fire(cx, cy, d, 20, 0);
                        sfx_play(SFX_SPIT);
                    }
                } else if (e_t[i] == 60) {
                    eb_fire(cx, cy, 0, 16, 0); eb_fire(cx, cy, 3, 16, 0); eb_fire(cx, cy, 29, 16, 0);
                    sfx_play(SFX_SPIT);
                }
                if (e_t[i] >= 150) e_st[i] = 2;
            } else e_y[i] += 0x80;
            break;
        case E_SKIFF:
            e_t[i]++;
            e_x[i] += e_vx[i];
            e_y[i] += (int16_t)sin64[(e_t[i] * 2) & 63] >> 1;
            cx = (e_x[i] >> 8) + 8; cy = (e_y[i] >> 8) + 8;
            if (e_t[i] == 45 || (HUNTER && (e_t[i] == 25 || e_t[i] == 70))) {
                if (HUNTER || (rnd() & 1)) eb_fire(cx, cy, aim(cx, cy, pcx, pcy), HUNTER ? 30 : 16, 0);
            }
            if ((e_x[i] >> 8) < 2 || (e_x[i] >> 8) > 178) e_type[i] = E_NONE;
            break;
        case E_ICE:
            e_t[i]++;
            if (e_st[i] == 0) {                 // hangs and blinks, then drops straight down that column
                if ((e_t[i] & 8) == 0) e_fl[i] |= EF_HIDDEN; else e_fl[i] &= (uint8_t)~EF_HIDDEN;
                if (e_t[i] >= (HUNTER ? 40 : 56)) { e_st[i] = 1; e_t[i] = 0; e_fl[i] &= (uint8_t)~EF_HIDDEN; }
            } else {
                e_y[i] += 0x90; e_x[i] += e_vx[i];
            }
            break;
        case E_GUN:
            e_t[i]++;
            if (e_st[i] == 0) {
                e_y[i] += 0x80;
                if ((e_y[i] >> 8) >= 14 + YO) { e_st[i] = 1; e_t[i] = 0; comm_queue_add(CM1_OWES); }
            } else if (e_st[i] == 1) {
                e_x[i] = ((uint16_t)(80 + XO - 16 + (sin64[(uint8_t)(e_t[i] >> 1) & 63] * (HUNTER ? 48 : 40) >> 7))) << 8;
                e_y[i] = ((uint16_t)(14 + YO + (sin64[(e_t[i] * 3) & 63] >> 5))) << 8;
                x = e_x[i] >> 8; y = e_y[i] >> 8;
                if (HUNTER) {
                    if ((e_t[i] % 48) < 15 && (e_t[i] % 5) == 0) {
                        eb_fire(x + 4, y + 20, aim(x + 4, y + 20, pcx, pcy), 28, 0);
                        eb_fire(x + 28, y + 20, aim(x + 28, y + 20, pcx, pcy), 28, 0);
                        sfx_play(SFX_SHOT2);
                    }
                    if ((e_t[i] % 144) == 100) {
                        for (d = 30; d != 3; d = (d + 1) & 31) eb_fire(x + 16, y + 22, d, 20, 1);
                        eb_fire(x + 16, y + 22, 3, 20, 1);
                        sfx_play(SFX_SPIT);
                    }
                } else {
                    if ((e_t[i] % 80) == 40) { eb_fire(x + 16, y + 22, aim(x + 16, y + 22, pcx, pcy), 16, 0); sfx_play(SFX_SHOT2); }
                    if ((e_t[i] % 240) == 200) {
                        eb_fire(x + 16, y + 22, 0, 14, 1); eb_fire(x + 16, y + 22, 3, 14, 1); eb_fire(x + 16, y + 22, 29, 14, 1);
                        sfx_play(SFX_SPIT);
                    }
                }
                if (++mb_timer >= 2700) e_st[i] = 2;
            } else {
                e_y[i] -= 0x100;
                if ((e_y[i] >> 8) < 4) { e_type[i] = E_NONE; mb_active = 0; mb_done = 1; mid_out = OUT_FLEE; comm_queue_add(CM1_GOTAWAY); music_play(MUS_STAGE); }
            }
            break;
        }
    }
}

// ------------------------------------------------------------------ Space Kraken
static uint8_t k_phase, k_t, k_swayt, k_atk_i;
static uint8_t tn_len[2], tn_mode[2], tn_t[2], tn_target[2];
static int8_t tn_base[2], tn_curl[2];
enum { TM_IDLE, TM_WIND, TM_SWIPE, TM_WARN, TM_SLAM, TM_BACK };
enum { A_INK = 1, A_SWIPE, A_SLAM, A_BROOD, A_SPIRAL };
static const uint8_t atk_p1[] = {A_INK, A_SWIPE, A_INK, A_SLAM};
static const uint8_t atk_p2[] = {A_INK, A_SLAM, A_BROOD, A_SWIPE, A_INK, A_SLAM};
static const uint8_t atk_p3[] = {A_SPIRAL, A_SLAM, A_INK, A_BROOD, A_SWIPE, A_SLAM};

void s1_boss_start(void) BANKED {
    uint8_t i;
    k_hpmax = HUNTER ? 820 : 420;
    k_hp = k_hpmax;
    k_phase = 1; k_state = 0; k_t = 90; k_swayt = 0; k_sway = 0; k_hitflash = 0; k_atk_i = 0; k_retreat = 0;
    k_hdx = 0; k_hw = 28; k_hy0 = YO + 10; k_hy1 = YO + 56;
    boss_name = boss_label[0];
    boss_face(S1_BV_OPEN);
    for (i = 0; i < 2; i++) { tn_mode[i] = 0; tn_t[i] = 0; tn_len[i] = 2; tn_curl[i] = 0; }
    warn_t = 0;
    music_play(MUS_BOSS);
    sfx_play(SFX_ROAR);
    k_shake = 30;
}

static void tentacle_update(uint8_t n) {
    uint8_t i, a, L, rx, ry, mt = tn_mode[n];
    int8_t side = n ? 1 : -1;
    int16_t x, y;
    uint8_t ph = k_swayt * 2 + n * 20;
    uint8_t idle_len = HUNTER ? 7 : 6;
    tn_t[n]++;
    switch (mt) {
    case TM_IDLE:
        if (tn_len[n] < idle_len && (tn_t[n] & 3) == 0) tn_len[n]++;
        if (tn_len[n] > idle_len) tn_len[n]--;
        tn_base[n] = side * 5 + (sin64[ph & 63] >> 5);
        tn_curl[n] = sin64[(ph + 16) & 63] >> 4;
        break;
    case TM_WIND:
        if (tn_len[n] > 4 && (tn_t[n] & 3) == 0) tn_len[n]--;
        tn_base[n] = side * 12 + ((tn_t[n] & 4) ? 1 : -1);
        tn_curl[n] = side * 4;
        if (tn_t[n] >= (HUNTER ? 40 : 70)) { tn_mode[n] = TM_SWIPE; tn_t[n] = 0; sfx_play(SFX_WHOOSH); }
        break;
    case TM_SWIPE:
        tn_len[n] = HUNTER ? 9 : 8;
        tn_base[n] = (int8_t)((int16_t)side * sin64[(16 + tn_t[n]) & 63] / 9);
        tn_curl[n] = -side;
        if (tn_t[n] >= 64) { tn_mode[n] = TM_IDLE; tn_t[n] = 0; }
        break;
    case TM_WARN:
        tn_base[n] = (int8_t)tn_target[n]; tn_curl[n] = 0;
        if (tn_len[n] > 4) tn_len[n]--;
        if (tn_t[n] >= (HUNTER ? 50 : 80)) { tn_mode[n] = TM_SLAM; tn_t[n] = 0; sfx_play(SFX_WHOOSH); warn_t = 0; }
        break;
    case TM_SLAM:
        tn_base[n] = (int8_t)tn_target[n]; tn_curl[n] = 0;
        if (tn_len[n] < (HUNTER ? 16 : 13)) tn_len[n] += 2;
        if (tn_t[n] >= 40) { tn_mode[n] = TM_BACK; tn_t[n] = 0; }
        break;
    case TM_BACK:
        if (tn_len[n] > idle_len) tn_len[n]--;
        else { tn_mode[n] = TM_IDLE; tn_t[n] = 0; }
        break;
    }
    if (k_state >= 20) { if (tn_len[n] > 1 && (tn_t[n] & 1)) tn_len[n]--; }
    rx = 48 + XO + (n ? 56 : 40) + k_sway - 16;
    ry = YO + 52 - k_retreat;
    x = (int16_t)rx << 7; y = (int16_t)ry << 7;
    L = tn_len[n];
    a = (uint8_t)tn_base[n];
    for (i = 0; i < 6; i++) {
        uint8_t k = n * 6 + i;
        x += (int16_t)sin64[a & 63] * L;
        y += (int16_t)COS64(a & 63) * L;
        bp_x[k] = (uint8_t)(x >> 7); bp_y[k] = (uint8_t)(y >> 7);
        bp_tile[k] = i == 5 ? S1_ST_TENT_TIP : S1_ST_TENT_SEG;
        bp_pal[k] = 4 | 0x08;
        bp_hurt[k] = L >= 2;
        a += (uint8_t)(tn_curl[n] + (mt == TM_IDLE ? (sin64[(ph + i * 10) & 63] >> 6) : 0));
    }
}

static void kraken_ink(void) {
    uint8_t cx = 96 + k_sway, cy = YO + 56;
    uint8_t d = aim(cx, cy, px + 8, py + 12);
    if (HUNTER) {
        uint8_t k, n = k_phase == 1 ? 7 : 9;
        for (k = 0; k < n; k++) eb_fire(cx, cy, d - (n >> 1) + k, 22, 1);
    } else {
        eb_fire(cx, cy, d, 14, 1); eb_fire(cx, cy, d - 3, 14, 1); eb_fire(cx, cy, d + 3, 14, 1);
    }
    sfx_play(SFX_SPIT);
}

static void boss_update(void) {
    uint8_t i, amp;
    k_swayt++;
    amp = k_phase == 1 ? 14 : 22;
    if (k_state < 20) k_sway = (int8_t)(((int16_t)sin64[(k_swayt >> 1) & 63] * amp) >> 7);
    if (k_state < 20) {
        uint8_t ph = k_hp * 3 <= k_hpmax ? 3 : (k_hp * 3 <= k_hpmax * 2 ? 2 : 1);
        if (ph > k_phase) {
            k_phase = ph; k_atk_i = 0; k_shake = 30; sfx_play(SFX_ROAR); boss_face(S1_BV_ANGRY);
            boss_comm = ph == 2 ? CM1_HURT : CM1_TENT;
        }
    }
    if (k_hitflash) k_hitflash--;
    switch (k_state) {
    case 0:
        if (k_t) k_t--;
        if ((k_swayt & 127) == 100) boss_face(S1_BV_CLOSED);
        else if ((k_swayt & 127) == 108) boss_face(S1_BV_OPEN);
        if (HUNTER && (k_swayt % (k_phase == 1 ? 60 : 40)) == 0) {
            uint8_t cx = 96 + k_sway, cy = YO + 56;
            eb_fire(cx, cy, aim(cx, cy, px + 8, py + 12), 24, 0);
        }
        if (!k_t) {
            uint8_t a;
            if (k_phase == 1) a = atk_p1[k_atk_i % sizeof(atk_p1)];
            else if (k_phase == 2 || !HUNTER) a = atk_p2[k_atk_i % sizeof(atk_p2)];
            else a = atk_p3[k_atk_i % sizeof(atk_p3)];
            k_atk_i++;
            k_state = a; k_t = 0;
            boss_face(a == A_INK || a == A_SPIRAL ? S1_BV_SPIT : S1_BV_ANGRY);
            if (a == A_SWIPE) {
                uint8_t n = (px + 8 < 96 + k_sway) ? 0 : 1;
                tn_mode[n] = TM_WIND; tn_t[n] = 0;
                if (HUNTER && k_phase >= 2) { tn_mode[n ^ 1] = TM_WIND; tn_t[n ^ 1] = 0; }
            }
            if (a == A_SLAM) {
                uint8_t n = (px + 8 < 96 + k_sway) ? 0 : 1, rx = 48 + XO + (n ? 56 : 40) + k_sway - 16;
                uint8_t d = aim(rx, YO + 52, px + 8, py + 12);
                int8_t ang = (int8_t)(d << 1);
                if (ang > 32) ang -= 64;
                if (ang > 20) ang = 20;
                if (ang < -20) ang = -20;
                tn_target[n] = (uint8_t)ang; tn_mode[n] = TM_WARN; tn_t[n] = 0;
                warn_x = px + 4; warn_t = HUNTER ? 50 : 80;
                sfx_play(SFX_ALARM);
                if (k_phase >= 2) {                 // the other arm marks a second column
                    uint8_t m = n ^ 1;
                    rx = 48 + XO + (m ? 56 : 40) + k_sway - 16;
                    d = aim(rx, YO + 52, px + 8, py + 12);
                    ang = (int8_t)(d << 1);
                    if (ang > 32) ang -= 64;
                    if (ang > 20) ang = 20;
                    if (ang < -20) ang = -20;
                    tn_target[m] = (uint8_t)ang; tn_mode[m] = TM_WARN; tn_t[m] = 16;
                }
            }
        }
        break;
    case A_INK:
        k_t++;
        if (k_t == 20) kraken_ink();
        if (HUNTER && (k_t == 38 || (k_phase >= 2 && k_t == 56))) kraken_ink();
        if (!HUNTER && k_phase >= 2 && k_t == 60) kraken_ink();
        if (k_t >= 80) { k_state = 0; k_t = HUNTER ? 50 : 90; boss_face(S1_BV_OPEN); }
        break;
    case A_SWIPE:
        k_t++;
        if (k_t >= (HUNTER ? 110 : 140)) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S1_BV_OPEN); }
        break;
    case A_SLAM:
        k_t++;
        if (k_t >= (HUNTER ? 130 : 170)) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S1_BV_OPEN); }
        break;
    case A_BROOD:
        k_t++;
        if (k_t == 20 || (HUNTER && k_t == 40)) {
            int8_t e = e_new(E_KL, 88 + k_sway, YO + 44);
            if (e >= 0) e_a[e] = 88 + k_sway + ((k_t == 20) ? -20 : 20);
            e = HUNTER ? e_new(E_KL, 88 + k_sway, YO + 44) : -1;
            if (e >= 0) e_a[e] = 88 + k_sway + ((k_t == 20) ? 20 : -20);
            sfx_play(SFX_SPIT);
        }
        if (k_t >= 70) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S1_BV_OPEN); }
        break;
    case A_SPIRAL:
        k_t++;
        if ((k_t & 3) == 0 && k_t < 140) {
            uint8_t cx = 96 + k_sway, cy = YO + 56, d = k_t >> 2;
            eb_fire(cx, cy, d * 3, 18, 0);
            eb_fire(cx, cy, d * 3 + 16, 18, 0);
        }
        if (k_t >= 160) { k_state = 0; k_t = 50; boss_face(S1_BV_OPEN); }
        break;
    case 20:
        k_t++;
        if ((k_t & 7) == 0) {
            boom_at(64 + (rnd() & 63) + k_sway, YO + 10 + (rnd() & 31));
            sfx_play((k_t & 15) ? SFX_EXPL_S : SFX_EXPL_L);
        }
        k_hitflash = (k_t & 4) ? 2 : 0;
        boss_face((k_t & 32) ? S1_BV_ANGRY : S1_BV_CLOSED);
        if (k_t >= 150) { k_state = 21; k_t = 0; sfx_play(SFX_ROAR); }
        break;
    case 21:
        if (boss_outro()) k_state = 22;
        break;
    }
    if (k_state < 20 && k_hp == 0) {
        k_state = 20; k_t = 0;
        for (i = 0; i < MAXEB; i++) eb_on[i] = 0;
        for (i = 0; i < MAXE; i++) if (e_type[i] == E_KL) { e_type[i] = E_BOOM; e_t[i] = 0; }
        music_stop();
        add_score(20000);
        if (hits_taken == 0) add_score(10000);
        tn_mode[0] = tn_mode[1] = TM_BACK;
    }
    for (i = 0; i < 2; i++) tentacle_update(i);
    bp_n = 12;
    if (warn_t) warn_t--;
}

// ------------------------------------------------------------------ API
void s1_reset(uint8_t cp) BANKED {
    uint16_t srow = stage_defs[0].cp[cp];
    uint8_t s;
    for (s = 0; s < MAXSPN; s++) sp_left[s] = 0;
    script_i = 0;
    while (script[script_i].row < srow) { run_cmd(&script[script_i], 0); script_i++; }
}

void s1_tick(void) BANKED {
    if (st != ST_BOSS && !k_hpmax) {
        uint16_t srow = pos >> 3;
        while (script[script_i].row <= srow) { run_cmd(&script[script_i], 1); script_i++; }
    }
    spawners_update();
    enemies_update();
    if (k_hpmax) boss_update();
}
