// Stage 3 logic (bank 11): the Barrens Belt. Leeds imps (some sneak up from
// behind), pine-cone mines, will-o'-wisps, Nubs Mk II and the Jersey Devil.
#pragma bank 11
#include <gb/gb.h>
#include <stdint.h>
#include "core.h"
#include "assets.h"
#include "sound.h"
#include "objs.h"
#include "script_ids.h"
#define MOUTH_X (uint8_t)(96 + 22 + k_sway)   // the horse-skull mouth (sheet art faces right)

enum { C_IMPS = 1, C_AMBUSH, C_CONES, C_WISP, C_MID, C_COMM, C_PAL, C_WARN, C_SLEEP, C_END, C_GAP };
#define H 0x80
typedef struct { uint16_t row; uint8_t cmd, a, b; } Cmd;
#define FO S3_ROW_FOREST0
#define CA S3_ROW_CABINS0
#define BG S3_ROW_BOG0
#define AP S3_ROW_APPROACH0
static const Cmd script[] = {
    {6, C_COMM, CM3_WELCOME, 0},
    {16, C_IMPS, 48, 0}, {26, C_IMPS, 112, 0}, {36, C_CONES, 80, 0}, {42, C_IMPS | H, 80, 0},
    {RAT(FO, 4), C_AMBUSH, 40, 0}, {RAT(FO, 5), C_COMM, CM3_BEHIND, 0}, {RAT(FO, 10), C_AMBUSH | H, 120, 0},
    {RAT(FO, 18), C_CONES, 56, 1}, {RAT(FO, 20), C_COMM, CM3_CONES, 0}, {RAT(FO, 24), C_CONES, 104, 0},
    {RAT(FO, 32), C_IMPS, 80, 0}, {RAT(FO, 36), C_IMPS | H, 40, 0},
    {RAT(FO, 44), C_AMBUSH, 112, 0}, {RAT(FO, 48), C_AMBUSH, 48, 0},
    {RAT(FO, 56), C_SLEEP, 80, 0},
    {RAT(FO, 64), C_CONES, 40, 0}, {RAT(FO, 66), C_CONES, 120, 0}, {RAT(FO, 68), C_CONES | H, 80, 0},
    {RAT(FO, 76), C_IMPS, 64, 2}, {RAT(FO, 80), C_AMBUSH | H, 80, 0},
    {RAT(FO, 88), C_IMPS, 104, 0}, {RAT(FO, 92), C_CONES, 72, 0},
    {RAT(FO, 100), C_AMBUSH, 72, 0}, {RAT(FO, 102), C_AMBUSH | H, 104, 0},
    {RAT(FO, 110), C_IMPS, 40, 1}, {RAT(FO, 114), C_IMPS, 120, 0},
    {RAT(FO, 122), C_CONES, 96, 0}, {RAT(FO, 126), C_AMBUSH, 56, 0},
    {S3_CP1 + 1, C_COMM, CM3_GAP, 0}, {S3_CP1 + 3, C_GAP, 0, 0}, {S3_CP1 + 8, C_COMM, CM3_NUBS, 0}, {S3_CP1 + 10, C_MID, 0, 0}, {S3_CP1 + 13, C_GAP, 1, 0},
    {RAT(CA, 34), C_IMPS, 72, 0}, {RAT(CA, 40), C_AMBUSH, 104, 0},
    {RAT(CA, 48), C_CONES, 48, 0}, {RAT(CA, 50), C_CONES | H, 112, 0},
    {RAT(CA, 58), C_IMPS, 96, 0}, {RAT(CA, 62), C_AMBUSH | H, 48, 0},
    {RAT(CA, 70), C_SLEEP, 64, 0},
    {RAT(CA, 76), C_IMPS, 56, 0}, {RAT(CA, 80), C_IMPS | H, 104, 0}, {RAT(CA, 88), C_CONES, 80, 2},
    {RAT(BG, 0), C_COMM, CM3_FOG, 0},
    {RAT(BG, 8), C_WISP, 64, 0}, {RAT(BG, 10), C_COMM, CM3_WISPS, 0}, {RAT(BG, 14), C_WISP | H, 104, 0},
    {RAT(BG, 24), C_IMPS, 40, 0}, {RAT(BG, 28), C_AMBUSH, 120, 0},
    {RAT(BG, 36), C_WISP, 104, 0}, {RAT(BG, 40), C_CONES, 56, 0},
    {RAT(BG, 48), C_WISP, 48, 0}, {RAT(BG, 50), C_WISP | H, 120, 0},
    {RAT(BG, 60), C_AMBUSH, 80, 0}, {RAT(BG, 62), C_IMPS, 104, 0},
    {RAT(BG, 72), C_WISP, 80, 0}, {RAT(BG, 76), C_CONES, 40, 0}, {RAT(BG, 78), C_CONES | H, 120, 0},
    {RAT(BG, 88), C_IMPS, 64, 1}, {RAT(BG, 90), C_AMBUSH | H, 40, 0},
    {RAT(BG, 98), C_WISP, 112, 0}, {RAT(BG, 100), C_WISP, 48, 0},
    {RAT(BG, 110), C_AMBUSH, 96, 0}, {RAT(BG, 114), C_IMPS | H, 80, 0},
    {S3_CP2 - 2, C_PAL, 6, BP_JD0}, {S3_CP2 - 2, C_PAL, 7, BP_JD1},
    {S3_CP2 + 2, C_WARN, 0, 0},
    {S3_CP2 + 10, C_COMM, CM3_DEVIL, 0},
    {0xFFFF, C_END, 0, 0}
};
static uint16_t script_i;

#define MAXSPN 4
static uint8_t sp_kind[MAXSPN], sp_left[MAXSPN], sp_t[MAXSPN], sp_a[MAXSPN], sp_i[MAXSPN], sp_drop[MAXSPN];
static void spawner_add(uint8_t kind, uint8_t a, uint8_t n, uint8_t drop) {
    uint8_t s;
    for (s = 0; s < MAXSPN; s++) if (!sp_left[s]) {
        sp_kind[s] = kind; sp_a[s] = a; sp_left[s] = n; sp_t[s] = 0; sp_i[s] = 0; sp_drop[s] = drop; return;
    }
}

// imp from below: waits off-screen while a marker blinks, then rises
static void ambush_imp(uint8_t x) {
    int8_t i = e_new(E_IMP, x, SCR_BOTTOM + 10);
    if (i >= 0) { e_st[i] = 2; e_a[i] = x; e_fl[i] = EF_HIDDEN; }
}

static void spawners_update(void) {
    uint8_t s;
    int8_t i;
    for (s = 0; s < MAXSPN; s++) {
        if (!sp_left[s]) continue;
        if (sp_t[s]) { sp_t[s]--; continue; }
        if (sp_kind[s] == C_GAP) {
            // one safe column, marked, then the row. The Ashmare's stomp is this.
            if (!sp_i[s]) {
                warn_x = sp_a[s] ? (uint8_t)(XO + 36) : (uint8_t)(XO + 124);
                warn_t = 48; sfx_play(SFX_ALARM);
                sp_i[s] = 1; sp_t[s] = 48;
            } else {
                uint8_t x, gap = warn_x, gw = HUNTER ? 28 : 44;
                for (x = XO + 4; x < XO + 160; x += 15) {
                    if (x + 8 > gap - gw / 2 && x < gap + gw / 2) continue;
                    eb_fire(x, YO + 8, 0, HUNTER ? 14 : 10, 1);
                }
                sfx_play(SFX_BOMB);
                sp_left[s] = 0;
            }
            continue;
        }
        if (sp_kind[s] == C_IMPS) {
            // imps dive from the top in a staggered arc
            i = e_new(E_IMP, sp_a[s] + XO - 8 + (int8_t)((sp_i[s] & 1) ? 16 : -16), 8);
            if (i >= 0) { e_a[i] = (uint8_t)(e_x[i] >> 8); e_c[i] = (sp_i[s] & 1); if (sp_i[s] == 0 && sp_drop[s]) e_b[i] = sp_drop[s] - 1; }
            sp_t[s] = 14;
        } else {
            ambush_imp(sp_a[s] + XO - 8 + (int8_t)(sp_i[s] * 20) - 20);
            sp_t[s] = 26;
        }
        sp_i[s]++; sp_left[s]--;
    }
}

static void run_cmd(const Cmd *c, uint8_t live) {
    int8_t i;
    uint8_t cmd = c->cmd & 0x7F;
    if ((c->cmd & H) && !HUNTER) return;
    if (!live && cmd != C_PAL) return;
    switch (cmd) {
        case C_IMPS: spawner_add(C_IMPS, c->a, HUNTER ? 5 : 4, c->b); break;
        case C_AMBUSH: spawner_add(C_AMBUSH, c->a, HUNTER ? 3 : 2, 0); break;
        case C_CONES: {
            uint8_t k, n = HUNTER ? 3 : 2;
            for (k = 0; k < n; k++) {
                i = e_new(E_CONE, c->a + XO - 8 + k * 24 - (n - 1) * 12, 8 - k * 12);
                if (i >= 0) { e_a[i] = (uint8_t)(e_x[i] >> 8); if (k == 0 && c->b) e_b[i] = c->b - 1; }
            }
            break;
        }
        case C_WISP:
            i = e_new(E_WISP, c->a + XO - 8, 8);
            if (i >= 0) e_a[i] = c->a + XO - 8;
            break;
        case C_MID:
            if (!mb_done) {
                i = e_new(E_GUN3, 80 + XO - 16, 0);
                if (i >= 0) { mb_slot = i; mb_active = 1; mb_timer = 0; }
                music_play(MUS_BOSS);
            }
            break;
        case C_GAP: spawner_add(C_GAP, c->a, 1, 0); break;
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
        case E_IMP:
            e_t[i]++;
            if (e_st[i] == 0) {                    // diving arc from the top
                e_y[i] += HUNTER ? 0xD0 : 0xA0;
                e_x[i] = ((uint16_t)(e_a[i] + ((e_c[i] ? 1 : -1) * (int8_t)(sin64[(e_t[i] * 2) & 63] >> 3)))) << 8;
                if (HUNTER && (e_t[i] == 30 || e_t[i] == 60)) {
                    cx = (e_x[i] >> 8) + 8; cy = (e_y[i] >> 8) + 8;
                    eb_fire(cx, cy, aim(cx, cy, pcx, pcy), 26, 0);
                }
            } else if (e_st[i] == 2) {             // waiting below: marker blinks
                if (e_t[i] >= (HUNTER ? 40 : 70)) { e_st[i] = 3; e_t[i] = 0; e_fl[i] = 0; sfx_play(SFX_WHOOSH); }
            } else {                                // rising from behind, curling away
                e_y[i] -= HUNTER ? 0x140 : 0xD0;
                if (e_t[i] > 40) {
                    if (e_a[i] < 96) e_a[i]--; else e_a[i]++;
                }
                e_x[i] = (uint16_t)e_a[i] << 8;
                if ((e_y[i] >> 8) < 8 || e_a[i] < 2 || e_a[i] > 180) e_type[i] = E_NONE;
            }
            break;
        case E_CONE:
            e_t[i]++;
            e_y[i] += HUNTER ? 0x60 : 0x48;
            e_x[i] = ((uint16_t)(e_a[i] + (sin64[e_t[i] & 63] >> 5))) << 8;
            break;
        case E_WISP:
            e_t[i]++;
            if (e_st[i] == 0) {
                e_y[i] += 0x70;
                if ((e_y[i] >> 8) >= 30 + YO + (e_a[i] & 31)) { e_st[i] = 1; e_t[i] = 0; }
            } else if (e_st[i] < 8) {
                // glow for a while, then fade into the fog and drift
                uint8_t cyc = e_t[i];
                if (cyc == 1) e_fl[i] = 0;
                if (cyc == (HUNTER ? 30 : 50)) {
                    cx = (e_x[i] >> 8) + 8; cy = (e_y[i] >> 8) + 8;
                    d = aim(cx, cy, pcx, pcy);
                    if (HUNTER) { eb_fire(cx, cy, d, 22, 0); eb_fire(cx, cy, d - 2, 22, 0); eb_fire(cx, cy, d + 2, 22, 0); }
                    else eb_fire(cx, cy, d, 13, 0);
                    sfx_play(SFX_SPIT);
                }
                if (cyc == 80) e_fl[i] = EF_HIDDEN;
                if (e_fl[i] & EF_HIDDEN) {
                    x = e_x[i] >> 8;
                    if (x + 8 < pcx) e_x[i] += 0x60; else e_x[i] -= 0x60;
                }
                if (cyc >= 130) { e_t[i] = 0; e_st[i]++; }
            } else {
                e_fl[i] = 0;
                e_y[i] += 0x100;
            }
            break;
        case E_GUN3:
            e_t[i]++;
            if (e_st[i] == 0) {
                e_y[i] += 0x80;
                if ((e_y[i] >> 8) >= 14 + YO) { e_st[i] = 1; e_t[i] = 0; comm_queue_add(CM3_OWES); }
            } else if (e_st[i] == 1) {
                e_x[i] = ((uint16_t)(80 + XO - 16 + (sin64[(uint8_t)(e_t[i] >> 1) & 63] * (HUNTER ? 48 : 40) >> 7))) << 8;
                e_y[i] = ((uint16_t)(16 + YO + (sin64[(e_t[i] * 2) & 63] >> 4))) << 8;
                x = e_x[i] >> 8; y = e_y[i] >> 8;
                if (HUNTER) {
                    if ((e_t[i] % 40) == 0) { eb_fire(x + 16, y + 22, aim(x + 16, y + 22, pcx, pcy), 26, 1); sfx_play(SFX_SHOT2); }
                    if ((e_t[i] % 40) == 20) { eb_fire(x + 4, y + 20, 0, 22, 0); eb_fire(x + 28, y + 20, 0, 22, 0);
                                               eb_fire(x + 4, y + 20, 3, 22, 0); eb_fire(x + 28, y + 20, 29, 22, 0); }
                } else if ((e_t[i] % 90) == 45) { eb_fire(x + 16, y + 22, aim(x + 16, y + 22, pcx, pcy), 14, 1); sfx_play(SFX_SHOT2); }
                if ((e_t[i] % (HUNTER ? 120 : 200)) == 100) {   // drops pine-cone mines
                    int8_t c = e_new(E_CONE, x + 8, y + 16);
                    if (c >= 0) e_a[c] = x + 8;
                    sfx_play(SFX_SPIT);
                }
                if (++mb_timer >= 2700) e_st[i] = 2;
            } else {
                e_y[i] -= 0x100;
                if ((e_y[i] >> 8) < 4) {
                    e_type[i] = E_NONE; mb_active = 0; mb_done = 1;
                    mid_out = OUT_FLEE; comm_queue_add(CM3_NUBSGONE); music_play(MUS_STAGE3);
                }
            }
            break;
        }
    }
}

// ------------------------------------------------------------------ Jersey Devil
static uint8_t k_phase, k_t, k_swayt, k_atk_i, tail_ph, gap_x, rows_left, row_t;
static int8_t sway_off;
enum { A_FIRE = 1, A_STOMP, A_SCREECH, A_IMPS, A_VANISH };
static const uint8_t atk_p1[] = {A_STOMP, A_VANISH, A_FIRE, A_STOMP};
static const uint8_t atk_p2[] = {A_VANISH, A_STOMP, A_IMPS, A_FIRE, A_STOMP, A_SCREECH};
static const uint8_t atk_p3[] = {A_VANISH, A_FIRE, A_STOMP, A_IMPS, A_SCREECH, A_FIRE};

void s3_boss_start(void) BANKED {
    k_hpmax = HUNTER ? 1250 : 620;
    k_hp = k_hpmax;
    k_phase = 1; k_state = 0; k_t = 90; k_swayt = 0; k_sway = 0; k_atk_i = 0; sway_off = 0;
    k_hdx = 6; k_hw = 22; k_hy0 = YO + 8; k_hy1 = YO + 60;
    boss_name = boss_label[2];
    boss_face(S3_BV_OPEN);
    bp_n = 0; rows_left = 0;
    music_play(MUS_JDEVIL);
    sfx_play(SFX_ROAR);
    k_shake = 30;
}

static void tail_update(void) {
    // tail hangs from the body and swings like a pendulum
    uint8_t k, n = HUNTER ? 7 : 6, a;
    int16_t x = (int16_t)(96 - 20 + k_sway) << 7, y = (int16_t)(YO + 54 - k_retreat) << 7;
    tail_ph += (k_phase == 1) ? 2 : 3;
    for (k = 0; k < n; k++) {
        a = (uint8_t)((int8_t)(sin64[(tail_ph - k * 5) & 63] >> 4) * (k + 3) / 4);
        x += (int16_t)sin64[a & 63] * 7;
        y += (int16_t)COS64(a & 63) * 7;
        bp_x[k] = (uint8_t)(x >> 7); bp_y[k] = (uint8_t)(y >> 7);
        bp_tile[k] = (k == n - 1) ? S3_ST_TAIL_TIP : S3_ST_TAIL_SEG;
        bp_pal[k] = 5 | 0x08;
        bp_hurt[k] = k_state < 20;
    }
    bp_n = n;
}

static void stomp_row(void) {
    uint8_t x, gw = HUNTER ? 30 : 46;
    for (x = XO + 4; x < XO + 160; x += 15) {
        if (x + 8 > gap_x - gw / 2 && x < gap_x + gw / 2) continue;
        eb_fire(x, YO + 62, 0, HUNTER ? 16 : 11, 1);
    }
    sfx_play(SFX_BOMB);
    k_shake = 8;
}

static void boss_update(void) {
    uint8_t i, amp, ph;
    k_swayt++;
    amp = k_phase == 1 ? 14 : 20;
    if (k_state < 20) {
        k_sway = (int8_t)(((int16_t)sin64[(k_swayt >> 1) & 63] * amp) >> 7) + sway_off;
        k_bob = sin64[(k_swayt * 2) & 63] >> 5;
        ph = k_hp * 3 <= k_hpmax ? 3 : (k_hp * 3 <= k_hpmax * 2 ? 2 : 1);
        if (ph > k_phase) {
            k_phase = ph; k_atk_i = 0; k_shake = 30; sfx_play(SFX_ROAR); boss_face(S3_BV_ANGRY);
            boss_comm = ph == 2 ? CM3_HURT : CM3_FIRE;
        }
    }
    if (k_hitflash) k_hitflash--;
    if (rows_left) {
        if (row_t) row_t--;
        else {
            stomp_row(); rows_left--;
            row_t = HUNTER ? 40 : 60;
            gap_x = (uint8_t)(gap_x + ((rnd() & 1) ? 22 : -22));
            if (gap_x < XO + 24) gap_x = XO + 24;
            if (gap_x > XO + 136) gap_x = XO + 136;
        }
    }
    switch (k_state) {
    case 0:
        if (k_t) k_t--;
        if ((k_swayt & 127) == 80) boss_face(S3_BV_CLOSED);
        else if ((k_swayt & 127) == 88) boss_face(S3_BV_OPEN);
        if (HUNTER && (k_swayt % (k_phase == 1 ? 65 : 42)) == 0) {
            uint8_t cx = MOUTH_X, cy = YO + 30;
            eb_fire(cx, cy, aim(cx, cy, px + 8, py + 12), 24, 0);
        }
        if (!k_t) {
            uint8_t a;
            if (k_phase == 1) a = atk_p1[k_atk_i % sizeof(atk_p1)];
            else if (k_phase == 2 || !HUNTER) a = atk_p2[k_atk_i % sizeof(atk_p2)];
            else a = atk_p3[k_atk_i % sizeof(atk_p3)];
            k_atk_i++;
            k_state = a; k_t = 0;
            boss_face(a == A_FIRE || a == A_SCREECH ? S3_BV_FIRE : S3_BV_ANGRY);
        }
        break;
    case A_FIRE: {
        // a sweeping stream of fire from the mouth
        uint8_t step = HUNTER ? 3 : 6, span = HUNTER ? 20 : 12;
        k_t++;
        if (k_t > 16 && (k_t % step) == 0 && k_t < 16 + span * 3) {
            uint8_t d = (uint8_t)(((k_swayt & 64) ? (span / 2 - (k_t - 16) / 3) : ((k_t - 16) / 3 - span / 2)) & 31);
            eb_fire(MOUTH_X, YO + 30, d, HUNTER ? 22 : 13, 1);
            if ((k_t & 7) == 0) sfx_play(SFX_SPIT);
        }
        if (k_t >= 16 + span * 3 + 20) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S3_BV_OPEN); }
        break;
    }
    case A_STOMP:
        k_t++;
        if (k_t == 20) {
            gap_x = px + 8;
            if (gap_x < XO + 24) gap_x = XO + 24;
            if (gap_x > XO + 136) gap_x = XO + 136;
            rows_left = HUNTER ? (k_phase >= 2 ? 3 : 2) : 1;
            row_t = 0;
        }
        if (k_t >= (HUNTER ? 120 : 110)) { k_state = 0; k_t = HUNTER ? 30 : 70; boss_face(S3_BV_OPEN); }
        break;
    case A_SCREECH:
        k_t++;
        if (k_t == 20) { eb_ring(MOUTH_X, YO + 30, HUNTER ? 12 : 6, 0, HUNTER ? 18 : 11, 0); sfx_play(SFX_ROAR); k_shake = 10; }
        if (k_t == 50) eb_ring(MOUTH_X, YO + 30, HUNTER ? 12 : 6, HUNTER ? 1 : 3, HUNTER ? 18 : 11, 0);
        if (k_t >= 90) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S3_BV_OPEN); }
        break;
    case A_IMPS:
        k_t++;
        if (k_t == 10) { ambush_imp(px > 90 ? px - 40 : px + 40); if (HUNTER) ambush_imp(px > 90 ? px - 70 : px + 70); }
        if (k_t >= 70) { k_state = 0; k_t = HUNTER ? 40 : 80; boss_face(S3_BV_OPEN); }
        break;
    case A_VANISH:
        // vanish in smoke, reappear on the other side
        k_t++;
        if (k_t == 1) { set_pal_slot(6, BP_VOID3); set_pal_slot(7, BP_VOID3); sfx_play(SFX_WHOOSH); boom_at(88 + k_sway, YO + 20); }
        if (k_t == 40) { sway_off = (sway_off > 0) ? -22 : 22; }
        if (k_t == 60) { set_pal_slot(6, BP_JD0); set_pal_slot(7, BP_JD1); sfx_play(SFX_ROAR); boss_face(S3_BV_FIRE);
                         eb_ring(MOUTH_X, YO + 30, HUNTER ? 10 : 5, 0, HUNTER ? 16 : 10, 1); }
        if (k_t >= 90) { k_state = 0; k_t = HUNTER ? 30 : 60; boss_face(S3_BV_OPEN); }
        break;
    case 20:
        k_t++;
        if ((k_t & 7) == 0) {
            boom_at(60 + (rnd() & 63) + k_sway, YO + 8 + (rnd() & 31));
            sfx_play((k_t & 15) ? SFX_EXPL_S : SFX_EXPL_L);
        }
        k_hitflash = (k_t & 4) ? 2 : 0;
        boss_face((k_t & 32) ? S3_BV_ANGRY : S3_BV_CLOSED);
        if (k_t >= 150) { k_state = 21; k_t = 0; sfx_play(SFX_ROAR); }
        break;
    case 21:
        if (boss_outro()) k_state = 22;
        break;
    }
    if (k_state < 20 && k_hp == 0) {
        k_state = 20; k_t = 0; rows_left = 0;
        set_pal_slot(6, BP_JD0); set_pal_slot(7, BP_JD1);
        for (i = 0; i < MAXEB; i++) eb_on[i] = 0;
        for (i = 0; i < MAXE; i++) if (e_type[i] == E_IMP) { e_type[i] = E_BOOM; e_t[i] = 0; }
        music_stop();
        add_score(24000);
        if (hits_taken == 0) add_score(10000);
    }
    if (k_state < 21) tail_update(); else bp_n = 0;
}

// ------------------------------------------------------------------ API
void s3_reset(uint8_t cp) BANKED {
    uint16_t srow = stage_defs[2].cp[cp];
    uint8_t s;
    for (s = 0; s < MAXSPN; s++) sp_left[s] = 0;
    script_i = 0;
    while (script[script_i].row < srow) { run_cmd(&script[script_i], 0); script_i++; }
}

void s3_tick(void) BANKED {
    if (st != ST_BOSS && !k_hpmax) {
        uint16_t srow = pos >> 3;
        while (script[script_i].row <= srow) { run_cmd(&script[script_i], 1); script_i++; }
    }
    spawners_update();
    enemies_update();
    if (k_hpmax) boss_update();
}
