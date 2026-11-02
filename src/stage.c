// Generic shooter engine (bank 1): scrolling, player, buddies, bullets, pickups,
// collisions, drawing, HUD and the stage state machine. Each stage's waves,
// enemy behaviours and boss live in its own logic bank (s1.c, s2.c, s3.c).
#pragma bank 1
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "core.h"
#include "assets.h"
#include "sound.h"
#include "game.h"
#include "objs.h"

extern const uint8_t comm_buddy_hint, comm_buddy_got;

static uint16_t next_row;
static uint8_t checkpoint;
static uint8_t pxf, pyf;
static uint8_t p_inv, p_fire_cd, p_alive, p_intro;
static uint8_t bomb_t, flash_t;
static uint8_t pb_on[MAXPB], pb_x[MAXPB], pb_y[MAXPB], pb_type[MAXPB], pb_dmg[MAXPB], pb_tgt[MAXPB];
static int8_t pb_vx[MAXPB], pb_vy[MAXPB];
static uint8_t hud_row;

// buddies: friendly baby cryptids that trail the ship and shoot
uint8_t bud_n;
static uint8_t bud_cd, hist_x[32], hist_y[32], hist_i, bud_hint_done;

// ------------------------------------------------------------------ HUD (window)
static void hud_tile(uint8_t x, uint8_t t) { set_win_tile_xy(x, hud_row, t); }
static const uint32_t pow10[7] = {1000000UL, 100000UL, 10000UL, 1000UL, 100UL, 10UL, 1UL};
static void hud_draw(void) {
    uint8_t i, d;
    uint32_t s;
    if (HUNTER) {
        hud_tile(0, T_HUD_SHIP); hud_tile(1, txt_tile('x')); hud_tile(2, txt_tile('0' + (lives > 9 ? 9 : lives)));
        hud_tile(3, txt_tile(' ')); hud_tile(4, txt_tile(' '));
    } else {
        for (i = 0; i < 5; i++) hud_tile(i, i < hearts ? T_HUD_HEART : T_HUD_HEARTE);
    }
    hud_tile(5, HUNTER ? txt_tile(' ') : txt_tile('0' + lives));
    hud_tile(6, T_HUD_BOMB); hud_tile(7, txt_tile('0' + bombs));
    hud_tile(8, bud_n ? T_HUD_BUD : txt_tile(' '));
    hud_tile(9, txt_tile(wtype == 0 ? 'T' : wtype == 1 ? 'S' : 'H'));
    for (i = 0; i < 3; i++) hud_tile(10 + i, i <= wlevel ? T_HUD_PIP : T_HUD_PIPE);
    s = score;
    for (i = 0; i < 7; i++) {
        d = 0;
        while (s >= pow10[i]) { s -= pow10[i]; d++; }
        hud_tile(13 + i, txt_tile('0' + d));
    }
    hud_dirty = 0;
}
static void hud_init(uint8_t boss) {
    hud_row = boss ? 1 : 0;
    txt_fill(0, 0, 20, 2, txt_tile(' '), 5 | 0x80, 1);
    move_win(7, boss ? 128 : 136);
    if (boss) boss_bar();
    hud_draw();
}

// ------------------------------------------------------------------ stage logic dispatch
static void logic_reset(uint8_t cp) {
    switch (stage_id) { case 0: s1_reset(cp); break; case 1: s2_reset(cp); break; case 2: s3_reset(cp); break; default: s4_reset(cp); }
}
static void logic_tick(void) {
    switch (stage_id) { case 0: s1_tick(); break; case 1: s2_tick(); break; case 2: s3_tick(); break; default: s4_tick(); }
}
static void logic_boss_start(void) {
    switch (stage_id) { case 0: s1_boss_start(); break; case 1: s2_boss_start(); break; case 2: s3_boss_start(); break; default: s4_boss_start(); }
}

// ------------------------------------------------------------------ scrolling
static void scroll_rows(void) {
    while (next_row < SD->n_rows && next_row <= (pos >> 3) + 19) {
        ld_level_row(next_row, (uint8_t)(31 - (next_row & 31)));
        next_row++;
    }
}

// ------------------------------------------------------------------ player
static void player_reset_pos(void) {
    px = 72 + XO; py = 112 + YO + 24; pxf = pyf = 0; p_intro = 48; p_inv = 150; p_alive = 1;
    for (hist_i = 0; hist_i < 32; hist_i++) { hist_x[hist_i] = px; hist_y[hist_i] = 168 + YO; }
    hist_i = 0;
}

static void pb_spawn(uint8_t x, uint8_t y, int8_t vx, int8_t vy, uint8_t type, uint8_t dmg) {
    uint8_t i;
    for (i = 0; i < MAXPB; i++) if (!pb_on[i]) {
        pb_on[i] = 1; pb_x[i] = x; pb_y[i] = y; pb_vx[i] = vx; pb_vy[i] = vy; pb_type[i] = type; pb_dmg[i] = dmg;
        pb_tgt[i] = 0xFF;
        return;
    }
}
static uint8_t nearest_target(uint8_t x) {
    uint8_t i, best = 0xFF, bd = 255, d, ex;
    for (i = 0; i < MAXE; i++) {
        uint8_t t = e_type[i];
        if (t == E_NONE || (e_tf[t] & TF_NOHIT) || (e_fl[i] & EF_HIDDEN)) continue;
        if ((e_y[i] >> 8) > py + 8 || (e_y[i] >> 8) < 20) continue;
        ex = (e_x[i] >> 8) + (e_style[t] == SY_32 ? 16 : 8);
        d = ex > x ? ex - x : x - ex;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
static void player_fire(void) {
    uint8_t x = px + 4, y = py - 6, n, i;
    if (p_fire_cd) { p_fire_cd--; return; }
    switch (wtype) {
    case 0:
        pb_spawn(x, y, 0, -6, 0, wlevel ? 3 : 2);
        if (wlevel == 2) { pb_spawn(x - 7, y + 6, 0, -6, 0, 2); pb_spawn(x + 7, y + 6, 0, -6, 0, 2); }
        p_fire_cd = wlevel == 1 ? 5 : 6;
        sfx_play(SFX_SHOT);
        break;
    case 1:
        pb_spawn(x, y, 0, -5, 1, 2);
        if (wlevel == 0) { pb_spawn(x - 2, y, -2, -5, 1, 2); pb_spawn(x + 2, y, 2, -5, 1, 2); }
        else if (wlevel == 1) { pb_spawn(x - 2, y, -1, -5, 1, 2); pb_spawn(x + 2, y, 1, -5, 1, 2); pb_spawn(x - 4, y, -3, -4, 1, 2); pb_spawn(x + 4, y, 3, -4, 1, 2); }
        else { pb_spawn(x - 2, y, -1, -5, 1, 2); pb_spawn(x + 2, y, 1, -5, 1, 2); pb_spawn(x - 4, y, -2, -5, 1, 2); pb_spawn(x + 4, y, 2, -5, 1, 2); }
        p_fire_cd = 9;
        sfx_play(SFX_SHOT2);
        break;
    default:
        n = nearest_target(x + 4);
        pb_spawn(x, y, 0, -4, 2, 4);
        if (wlevel >= 1) pb_spawn(x - 8, y + 6, -1, -4, 2, 3);
        if (wlevel >= 2) pb_spawn(x + 8, y + 6, 1, -4, 2, 3);
        for (i = 0; i < MAXPB; i++) if (pb_on[i] && pb_type[i] == 2 && pb_tgt[i] == 0xFF) pb_tgt[i] = n;
        p_fire_cd = 10;
        sfx_play(SFX_SHOT);
        break;
    }
}

static void use_bomb(void) {
    uint8_t i;
    if (!bombs || bomb_t) return;
    bombs--; hud_dirty = 1;
    bomb_t = 60; flash_t = 8;
    if (p_inv < 90) p_inv = 90;
    sfx_play(SFX_BOMB);
    for (i = 0; i < MAXEB; i++) eb_on[i] = 0;
    for (i = 0; i < NHZ; i++) hz_on[i] = 0;
    for (i = 0; i < MAXE; i++) {
        uint8_t t = e_type[i];
        if (t == E_NONE || (e_tf[t] & TF_NOHIT)) continue;
        if ((e_y[i] >> 8) < 16) continue;
        e_fl[i] &= ~EF_HIDDEN;
        e_damage(i, (e_tf[t] & TF_MID) ? (HUNTER ? 30 : 40) : 20);
    }
    if (k_hpmax && k_state < 20) {
        uint16_t d = HUNTER ? 40 : 60;
        k_hp = k_hp > d ? k_hp - d : 1;
        k_hitflash = 6;
    }
}

static void player_hit(void);
static void player_update(void) {
    uint8_t spd = HUNTER ? 0xC0 : 0xA0;
    uint8_t ymax = k_hpmax ? 100 + YO : 108 + YO;
    uint16_t t;
    if (!p_alive) return;
    if (p_inv) p_inv--;
    if (p_intro) { p_intro--; py--; return; }
    if (joy & J_LEFT)  { t = (uint16_t)pxf + spd; pxf = (uint8_t)t; px -= 1 + (t >> 8); }
    if (joy & J_RIGHT) { t = (uint16_t)pxf + spd; pxf = (uint8_t)t; px += 1 + (t >> 8); }
    if (joy & J_UP)    { t = (uint16_t)pyf + spd; pyf = (uint8_t)t; py -= 1 + (t >> 8); }
    if (joy & J_DOWN)  { t = (uint16_t)pyf + spd; pyf = (uint8_t)t; py += 1 + (t >> 8); }
    if (wind && (frame_count & 3) == 0) px += wind;
    if (px < XO || px > 200) px = XO;
    if (px > 144 + XO) px = 144 + XO;
    if (py < 8 + YO) py = 8 + YO;
    if (py > ymax) py = ymax;
    if (!HUNTER || (joy & J_A)) player_fire();
    else if (p_fire_cd) p_fire_cd--;
    if (joy_new & J_B) use_bomb();
}

// ------------------------------------------------------------------ buddies
static void buddies_update(void) {
    uint8_t k, h, bx, by;
    if (!p_alive) return;
    if ((frame_count & 1) == 0) {
        hist_i = (hist_i + 1) & 31;
        hist_x[hist_i] = px; hist_y[hist_i] = py;
    }
    if (!bud_n || p_intro) return;
    if (bud_cd) { bud_cd--; return; }
    if (HUNTER && !(joy & J_A)) return;
    for (k = 0; k < bud_n; k++) {
        h = (hist_i - 6 * (k + 1)) & 31;
        bx = hist_x[h]; by = hist_y[h];
        pb_spawn(bx + 4, by - 2, 0, -5, 0, 1);
    }
    bud_cd = 14;
}
static void buddy_lose(void) {
    uint8_t h;
    if (!bud_n) return;
    h = (hist_i - 6 * bud_n) & 31;
    boom_at(hist_x[h], hist_y[h]);
    bud_n--;
    hud_dirty = 1;
}

// ------------------------------------------------------------------ bullets
static void boss_hit(uint8_t dmg) {
    static uint8_t hc;
    k_hp = k_hp > dmg ? k_hp - dmg : 0;
    if (++hc >= 3) { hc = 0; k_hitflash = 2; }
    sfx_play(SFX_BOSSHIT);
    add_score(10);
}
static uint8_t boss_cx(void) { return (uint8_t)(96 + k_sway + k_hdx); }

static void pbullets_update(void) {
    uint8_t i, j, bx, by, tx, t, s;
    for (i = 0; i < MAXPB; i++) {
        if (!pb_on[i]) continue;
        if (pb_type[i] == 2 && pb_tgt[i] != 0xFF) {
            j = pb_tgt[i];
            if (e_type[j] == E_NONE || e_type[j] == E_BOOM || (e_fl[j] & EF_HIDDEN)) pb_tgt[i] = 0xFF;
            else {
                tx = (e_x[j] >> 8) + (e_style[e_type[j]] == SY_32 ? 12 : 4);
                if (tx > pb_x[i] + 1 && pb_vx[i] < 3) pb_vx[i]++;
                else if (tx + 1 < pb_x[i] && pb_vx[i] > -3) pb_vx[i]--;
            }
        } else if (pb_type[i] == 2 && k_hpmax && k_state < 20) {
            tx = boss_cx() - 4;
            if (tx > pb_x[i] + 1 && pb_vx[i] < 2) pb_vx[i]++;
            else if (tx + 1 < pb_x[i] && pb_vx[i] > -2) pb_vx[i]--;
        }
        pb_x[i] += pb_vx[i]; pb_y[i] += pb_vy[i];
        if (pb_y[i] < 20 || pb_y[i] > 200 || pb_x[i] < 6 || pb_x[i] > 172) { pb_on[i] = 0; continue; }
        bx = pb_x[i] + 4; by = pb_y[i] + 3;
        for (j = 0; j < MAXE; j++) {
            t = e_type[j];
            if (t == E_NONE || (e_tf[t] & TF_NOHIT) || (e_fl[j] & EF_HIDDEN)) continue;
            switch (e_style[t]) {
            case SY_32:
                if (NEAR(bx, (e_x[j] >> 8) + 16, 15) && NEAR(by, (e_y[j] >> 8) + 12, 11)) { e_damage(j, pb_dmg[i]); pb_on[i] = 0; }
                break;
            case SY_EEL:
                if ((e_y[j] >> 8) > 20 && NEAR(bx, (e_x[j] >> 8) + 8, 9) && NEAR(by, (e_y[j] >> 8) + 8, 9)) { e_damage(j, pb_dmg[i]); pb_on[i] = 0; break; }
                if (e_c[j] < NCH) for (s = 0; s < NCSEG; s++)
                    if (NEAR(bx, ch_x[e_c[j]][s], 5) && NEAR(by, ch_y[e_c[j]][s], 5)) { e_damage(j, 1); pb_on[i] = 0; break; }
                break;
            default:
                if ((e_y[j] >> 8) > 20 && NEAR(bx, (e_x[j] >> 8) + 8, 9) && NEAR(by, (e_y[j] >> 8) + 8, 9)) { e_damage(j, pb_dmg[i]); pb_on[i] = 0; }
            }
            if (!pb_on[i]) break;
        }
        if (!pb_on[i]) continue;
        // shots knock enemy bullets out of the air; big orbs need rune rounds (2 hits)
        for (j = 0; j < MAXEB; j++) {
            if (eb_on[j] != 1) continue;
            tx = (eb_x[j] >> 8) + 3; t = (eb_y[j] >> 8) + 3;
            if (NEAR(bx, tx, 5) && NEAR(by, t, 7)) {
                pb_on[i] = 0;
                if (eb_big[j]) {
                    if (!g_gear[GEAR_RUNES]) { sfx_play(SFX_TICK); break; }
                    if (eb_big[j] == 1) { eb_big[j] = 2; sfx_play(SFX_TICK); break; }
                }
                eb_on[j] = 9; eb_vx[j] = 0; eb_vy[j] = 0;      // spark, then gone
                add_score(10);
                break;
            }
        }
        if (!pb_on[i] || !k_hpmax || k_state >= 20) continue;
        if (NEAR(bx, boss_cx(), k_hw) && by > k_hy0 - k_retreat && by < k_hy1 - k_retreat) {
            pb_on[i] = 0;
            boss_hit(pb_dmg[i]);
        }
    }
}

static void ebullets_update(void) {
    uint8_t i, x, y, r;
    uint8_t pcx = px + 8, pcy = py + 12;
    uint8_t hb = HUNTER ? 2 : 3;
    for (i = 0; i < MAXEB; i++) {
        if (!eb_on[i]) continue;
        if (eb_on[i] > 1) { eb_on[i]--; if (eb_on[i] == 1) eb_on[i] = 0; continue; }
        eb_x[i] += eb_vx[i]; eb_y[i] += eb_vy[i];
        x = eb_x[i] >> 8; y = eb_y[i] >> 8;
        if (x < 4 || x > 172 || y < 12 || y > SCR_BOTTOM + 4) { eb_on[i] = 0; continue; }
        if (p_alive && !p_inv && !p_intro) {
            r = eb_big[i] ? 3 + hb : 2 + hb;
            if (NEAR(x + (eb_big[i] ? 4 : 3), pcx, r) && NEAR(y + (eb_big[i] ? 4 : 3), pcy, r)) {
                eb_on[i] = 0; player_hit();
            }
        }
    }
}

static void catch_buddy(uint8_t i) {
    uint8_t x = e_x[i] >> 8, y = e_y[i] >> 8;
    e_type[i] = E_NONE;
    if (bud_n < (g_gear[GEAR_KENNEL] ? 3 : 2)) { bud_n++; comm_queue_add(comm_buddy_got); }
    else add_score(2000);
    add_score(1000);
    sfx_play(SFX_ONEUP);
    boom_at(x, y);
    hud_dirty = 1;
}

static void collide_bodies(void) {
    uint8_t i, j, s, t, pcx = px + 8, pcy = py + 12, r;
    if (!p_alive || p_intro) return;
    for (i = 0; i < MAXE; i++) {
        t = e_type[i];
        if (t == E_NONE || t == E_BOOM) continue;
        if (t == E_SLEEPER) {
            if (NEAR(pcx, (e_x[i] >> 8) + 8, 12) && NEAR(pcy, (e_y[i] >> 8) + 8, 14)) catch_buddy(i);
            continue;
        }
        if (p_inv || (e_fl[i] & EF_HIDDEN)) continue;
        switch (e_style[t]) {
        case SY_32:
            if (NEAR(pcx, (e_x[i] >> 8) + 16, 16) && NEAR(pcy, (e_y[i] >> 8) + 12, 12)) { player_hit(); return; }
            break;
        case SY_EEL:
            if (e_c[i] < NCH) for (s = 0; s < NCSEG; s++)
                if (NEAR(pcx, ch_x[e_c[i]][s], 6) && NEAR(pcy, ch_y[e_c[i]][s], 7)) { player_hit(); return; }
            /* fall through: head */
        default:
            if (NEAR(pcx, (e_x[i] >> 8) + 8, 8) && NEAR(pcy, (e_y[i] >> 8) + 8, 8)) {
                if (!(e_tf[t] & TF_NOBODY)) e_damage(i, 6);
                player_hit(); return;
            }
        }
    }
    if (p_inv) return;
    for (j = 0; j < NHZ; j++)
        if (hz_on[j] == 2 && NEAR(pcx, hz_x[j], HUNTER ? 5 : 4) && pcy > hz_y0[j]) { player_hit(); return; }
    if (k_hpmax && k_state < 20) {
        r = HUNTER ? 5 : 4;
        for (j = 0; j < bp_n; j++)
            if (bp_hurt[j] && NEAR(pcx, bp_x[j], r) && NEAR(pcy, bp_y[j], r)) { player_hit(); return; }
        if (pcy < k_hy1 - 4 - k_retreat && NEAR(pcx, boss_cx(), k_hw - 4)) { player_hit(); return; }
    }
}

static void pickups_update(void) {
    uint8_t i, kind, cycle;
    for (i = 0; i < MAXPK; i++) {
        if (!pk_on[i]) continue;
        pk_t[i]++;
        pk_y[i] += 0x50;
        pk_x[i] += (int16_t)sin64[(pk_t[i] * 2) & 63] >> 2;
        if ((pk_y[i] >> 8) > SCR_BOTTOM) { pk_on[i] = 0; continue; }
        if (p_alive && !p_intro && NEAR((pk_x[i] >> 8) + 8, px + 8, 14) && NEAR((pk_y[i] >> 8) + 8, py + 12, 16)) {
            kind = pk_kind[i]; pk_on[i] = 0;
            if (kind == PK_WEAP) {
                cycle = (pk_t[i] / (HUNTER ? 64 : 110)) % 3;
                if (cycle == wtype) { if (wlevel < 2) wlevel++; else add_score(1000); }
                else wtype = cycle;
                sfx_play(SFX_POWER);
            } else if (kind == PK_BOMB) { if (bombs < 9) bombs++; sfx_play(SFX_PICKUP); }
            else if (kind == PK_HEART) { if (hearts < 5) hearts++; else add_score(500); sfx_play(SFX_PICKUP); }
            else if (kind == PK_1UP) { if (lives < 9) lives++; sfx_play(SFX_ONEUP); }
            add_score(100);
            hud_dirty = 1;
        }
    }
}

// lightning columns: warning, then a strike
static void hazards_update(void) {
    uint8_t i;
    for (i = 0; i < NHZ; i++) {
        if (!hz_on[i]) continue;
        if (hz_t[i]) { hz_t[i]--; continue; }
        if (hz_on[i] == 1) { hz_on[i] = 2; hz_t[i] = 26; sfx_play(SFX_BOMB); k_shake = 6; }
        else hz_on[i] = 0;
    }
}

// generic enemy upkeep: explosions, sleepers, flashes, culling
static void enemies_common(void) {
    uint8_t i, t, y;
    for (i = 0; i < MAXE; i++) {
        t = e_type[i];
        if (t == E_NONE) continue;
        if (e_flash[i]) e_flash[i]--;
        if (t == E_BOOM) {
            e_t[i]++;
            if (e_t[i] >= 20) e_type[i] = E_NONE;
            if (scroll_on) e_y[i] += 0x80;
        } else if (t == E_SLEEPER) {
            e_t[i]++;
            e_y[i] += 0x48;
            e_x[i] = ((uint16_t)(e_a[i] + (sin64[e_t[i] & 63] >> 4))) << 8;
            if (!bud_hint_done && e_t[i] == 30) { comm_queue_add(comm_buddy_hint); bud_hint_done = 1; }
        }
        y = e_y[i] >> 8;
        if (!(e_tf[t] & TF_MID) && !(e_fl[i] & EF_HIDDEN) && e_type[i] != E_NONE && y > SCR_BOTTOM + 4 && y < 240) {
            if ((t == E_EEL || t == E_SNAKE) && e_c[i] < NCH) ch_used[e_c[i]] = 0;
            e_type[i] = E_NONE;
        }
    }
}

// ------------------------------------------------------------------ dying
static void player_hit(void) {
    hits_taken++;
    if (!HUNTER) {
        if (hearts > 1) {
            hearts--; p_inv = 120; sfx_play(SFX_HURT); hud_dirty = 1; flash_t = 2;
            return;
        }
        hearts = 0;
    }
    p_alive = 0; st = ST_DYING; st_t = 0;
    boom_at(px, py + 4); boom_at(px + 4, py - 4);
    buddy_lose();
    sfx_play(SFX_EXPL_L);
    hud_dirty = 1;
}

// ------------------------------------------------------------------ rendering
static uint8_t flame_t;
static void draw_player(void) {
    uint8_t ox, oy, vis;
    vis = p_alive && !(p_inv && (p_inv & 4) && !p_intro);
    if (!vis) { oam_fixed(0, 0, 0, 0, 0); oam_fixed(1, 0, 0, 0, 0); oam_fixed(2, 0, 0, 0, 0); oam_fixed(3, 0, 0, 0, 0); return; }
    ox = px - 8; oy = py - 16;
    flame_t++;
    oam_fixed(0, ox, oy, ST_SHIP_TOP, 0);
    oam_fixed(1, ox + 8, oy, ST_SHIP_TOP + 2, 0);
    oam_fixed(2, ox, oy + 16, (flame_t & 4) ? ST_SHIP_BOT : ST_SHIP_BOTB, 1);
    oam_fixed(3, ox + 8, oy + 16, ((flame_t & 4) ? ST_SHIP_BOT : ST_SHIP_BOTB) + 2, 1);
}

static void draw_all(void) {
    uint8_t i, x, y, t, p, f, s, c;
    oam_begin();
    // boss parts (tentacles, tail)
    if (k_hpmax) {
        if (k_state == 21 && bounty_out == OUT_CAPTURE && outro_t < 112) {     // rune net closing in
            uint8_t r = outro_t < 90 ? 64 - (outro_t >> 1) - (outro_t >> 2) : 0;
            uint8_t cx = 96 + k_sway, cy = YO + 30 - k_retreat;
            if (r) for (i = 0; i < 8; i++) {
                c = (uint8_t)(i * 8 + outro_t);
                oam_add(cx + (int8_t)(((int16_t)sin64[c & 63] * r) >> 7) - 4 - 8, cy + (int8_t)(((int16_t)COS64(c & 63) * r) >> 8) - 8 - 16, ST_RUNE, 2);
            }
        }
        if (k_state < 21 || bounty_out == OUT_FLEE || outro_t < (bounty_out == OUT_KILL ? 70 : 112))
        for (i = 0; i < bp_n; i++) oam_add(bp_x[i] - 4 - 8, bp_y[i] - 4 - 16, bp_tile[i], bp_pal[i]);
        if (warn_t && (warn_t & 8)) oam_add(warn_x - 8, 128 + YO - 16 - 6, ST_WARN, 7);
    }
    // lightning columns
    for (i = 0; i < NHZ; i++) {
        if (!hz_on[i]) continue;
        x = hz_x[i] - 4 - 8;
        if (hz_on[i] == 1) {
            if (hz_t[i] & 4) { oam_add(x, SCR_BOTTOM - 16 - 10, ST_WARN, 7); oam_add(x, hz_y0[i] - 16, ST_WARN, 7); }
        } else {
            for (y = hz_y0[i]; y < SCR_BOTTOM; y += 16)
                oam_add(x + ((frame_count & 2) ? 1 : 0), y - 16, S2_ST_BOLT, 4 | 0x08);
        }
    }
    // enemies
    for (i = 0; i < MAXE; i++) {
        t = e_type[i];
        if (t == E_NONE) continue;
        x = (e_x[i] >> 8) - 8; y = (e_y[i] >> 8) - 16;
        f = (e_t[i] >> 3) & 1;
        p = e_flash[i] ? (7 | (e_pal[t] & 0x08)) : e_pal[t];
        if (e_b[i] < 9 && (e_t[i] & 8)) p = 7 | (e_pal[t] & 0x08);   // carriers glow
        if ((e_fl[i] & EF_HIDDEN) && e_style[t] != SY_BLINK) {   // lurking below: blink a marker
            if (frame_count & 4) oam_add(x + 4, SCR_BOTTOM - 16 - 10, ST_WARN, 7);
            continue;
        }
        switch (e_style[t]) {
        case SY_BOOM:
            t = ST_EXPL0 + (e_t[i] / 5) * 4; p = 7; break;
        case SY_SLEEP:
            t = ST_BUDZ0 + stage_id * 4; p = 2;
            y -= (e_t[i] >> 5) & 1;                                  // slow sleepy bob
            break;
        case SY_16HP:
            t = (e_hp[i] <= (HUNTER ? 7 : 4)) ? e_tB[t] : e_tA[t]; break;
        case SY_BLINK:                                   // lit = A (shootable), dim = B
            t = (e_fl[i] & EF_HIDDEN) ? e_tB[t] : e_tA[t]; break;
        case SY_32:
            for (c = 0; c < 4; c++) {
                s = (e_fl[i] & EF_FLIP) ? 3 - c : c;
                oam_add(x + c * 8, y, e_tA[t] + s * 2, p | ((e_fl[i] & EF_FLIP) ? 0x20 : 0));
                oam_add(x + c * 8, y + 16, e_tA[t] + 8 + s * 2, p | ((e_fl[i] & EF_FLIP) ? 0x20 : 0));
            }
            continue;
        case SY_EEL:
            if (e_c[i] < NCH) for (s = 0; s < NCSEG; s++)
                oam_add(ch_x[e_c[i]][s] - 4 - 8, ch_y[e_c[i]][s] - 4 - 16, e_tB[t], p);
            t = e_tA[t];
            if (e_vx[i] < 0) {                           // heads face their direction of travel
                if (y > 152 + 16 || y < 1) continue;
                oam_add(x, y, t + 2, p | 0x20); oam_add(x + 8, y, t, p | 0x20);
                continue;
            }
            break;
        default:
            t = f ? e_tB[t] : e_tA[t];
        }
        if (y > 152 + 16 || y < 1) continue;
        oam_add(x, y, t, p);
        oam_add(x + 8, y, t + 2, p);
    }
    // buddies
    if (p_alive) for (i = 0; i < bud_n; i++) {
        s = (hist_i - 6 * (i + 1)) & 31;
        x = hist_x[s] - 8; y = hist_y[s] - 16 + 6;
        t = ST_BUD0_A + stage_id * 8 + ((flame_t & 8) ? 4 : 0);
        oam_add(x, y, t, 2); oam_add(x + 8, y, t + 2, 2);
    }
    // pickups
    for (i = 0; i < MAXPK; i++) {
        if (!pk_on[i]) continue;
        x = (pk_x[i] >> 8) - 8; y = (pk_y[i] >> 8) - 16;
        switch (pk_kind[i]) {
            case PK_WEAP: { uint8_t cy = (pk_t[i] / (HUNTER ? 64 : 110)) % 3; t = cy == 0 ? ST_PK_T : cy == 1 ? ST_PK_S : ST_PK_H; } break;
            case PK_BOMB: t = ST_PK_B; break;
            case PK_HEART: t = ST_PK_HEART; break;
            default: t = ST_PK_1UP;
        }
        oam_add(x, y, t, 7); oam_add(x + 8, y, t + 2, 7);
    }
    for (i = 0; i < MAXEB; i++) {
        if (!eb_on[i]) continue;
        if (eb_on[i] > 1) { if (eb_on[i] & 2) oam_add((eb_x[i] >> 8) - 8, (eb_y[i] >> 8) - 16 - 2, ST_SPARK, 7); continue; }
        oam_add((eb_x[i] >> 8) - 8, (eb_y[i] >> 8) - 16, eb_big[i] ? ST_EB_INK : ST_EB_ORB, 3);
    }
    for (i = 0; i < MAXPB; i++) {
        if (!pb_on[i]) continue;
        t = pb_type[i] == 0 ? ST_SHOT_TWIN : pb_type[i] == 1 ? ST_SHOT_SPREAD : ST_SHOT_WISP;
        oam_add(pb_x[i] - 8, pb_y[i] - 16, t, 2);
    }
    oam_end();
}

// ------------------------------------------------------------------ raster
static void raster(void) {
    uint8_t scy = (uint8_t)(112 - pos);
    uint8_t shake = 0;
    if (k_shake) { k_shake--; shake = (k_shake & 2) ? 2 : 0; }
    if (k_hpmax) {
        uint8_t band_scy = scy + k_retreat + shake - k_bob;
        uint8_t band_scx = (uint8_t)(-k_sway);
        if (comm_on) {
            n_lcdc = LCDC_COMM; n_scx = 0; n_scy = 16;
            n_s1_line = 32; n_s1_lcdc = LCDC_PLAY; n_s1_scx = band_scx; n_s1_scy = band_scy;
            n_s2_line = 64; n_s2_lcdc = LCDC_PLAY; n_s2_scx = 0; n_s2_scy = scy + shake;
        } else {
            n_lcdc = LCDC_PLAY; n_scx = band_scx; n_scy = band_scy;
            n_s1_line = 64; n_s1_lcdc = LCDC_PLAY; n_s1_scx = 0; n_s1_scy = scy + shake;
            n_s2_line = 0;
        }
    } else if (comm_on) {
        n_lcdc = LCDC_COMM; n_scx = 0; n_scy = 16;
        n_s1_line = 32; n_s1_lcdc = LCDC_PLAY; n_s1_scx = 0; n_s1_scy = scy + shake;
        n_s2_line = 0;
    } else {
        n_lcdc = LCDC_PLAY; n_scx = 0; n_scy = scy + shake;
        n_s1_line = 0; n_s2_line = 0;
    }
}

// ------------------------------------------------------------------ setup
static void clear_entities(void) {
    memset(e_type, 0, sizeof e_type);
    memset(eb_on, 0, sizeof eb_on);
    memset(pb_on, 0, sizeof pb_on);
    memset(pk_on, 0, sizeof pk_on);
    memset(ch_used, 0, sizeof ch_used);
    comm_reset();
    mb_active = 0;
}

static void restart_from(uint8_t cp) {
    uint16_t srow = SD->cp[cp], r;
    uint8_t i, bp[8];
    screen_off();
    splits_off();
    clear_entities();
    pos = srow << 3;
    for (r = srow; r < srow + 20 && r < SD->n_rows; r++) ld_level_row(r, (uint8_t)(31 - (r & 31)));
    next_row = r;
    ld_copy(SD->dbank, bp, SD->initbp, 8);
    for (i = 0; i < 8; i++) if (i != 4) set_pal_slot(i, bp[i]);
    boss_clear();
    mb_done = cp >= 2;
    logic_reset(cp);
    scroll_on = 1;
    st = ST_PLAY; st_t = 0;
    player_reset_pos();
    bomb_t = 0; flash_t = 0;
    hud_init(0);
    raster();
    music_play(cp == 2 ? MUS_WARN : SD->mus_stage);
    checkpoint = cp;
    screen_on(LCDC_PLAY);
}

static void stage_load(void) {
    screen_off();
    VBK_REG = 0;
    ld_spr_data(BANK_A2, 0, N_SPR_TILES, spr_tiles);
    VBK_REG = 1; ld_spr_data(SD->dbank, 0, SD->n_spr, SD->spr); VBK_REG = 0;
    ld_spr_pal(BANK_A2, 0, 3, obj_pals);
    ld_spr_pal(BANK_A2, 7, 1, obj_pals + 28);
    ld_spr_pal(SD->dbank, 3, 4, SD->objpal);
    ld_bkg_data(BANK_A2, 0, 0, N_COMMON, common_tiles);
    ld_bkg_data(SD->dbank, 0, N_COMMON, SD->n_scen0, SD->scen0);
    if (SD->n_scen1) ld_bkg_data(SD->dbank, 1, 0, SD->n_scen1, SD->scen1);
    boss_load();
    txt_fill(0, 0, 20, 6, txt_tile(' '), 5 | 0x80, 1);
    oam_clear();
}

// ------------------------------------------------------------------ continue prompt
static uint8_t continue_prompt(void) {
    uint8_t t = 9, f = 0;
    txt_fill(0, 2, 20, 4, txt_tile(' '), 5 | 0x80, 1);
    n_lcdc = LCDC_COMM; n_scx = 0; n_scy = 16; n_s1_line = 32; n_s1_lcdc = LCDC_PLAY; n_s1_scx = 0; n_s1_scy = (uint8_t)(112 - pos); n_s2_line = 0;
    if (!HUNTER) {
        txt_put(5, 3, "TRY AGAIN!", 1, 5 | 0x80);
        txt_put(3, 4, "YOU CAN DO IT", 1, 5 | 0x80);
        wait_frames(120);
        return 1;
    }
    music_play(MUS_OVER);
    txt_put(5, 3, "CONTINUE?", 1, 5 | 0x80);
    txt_put(2, 4, "A: YES  START: NO", 1, 5 | 0x80);
    while (1) {
        wait_vbl_done(); input_poll();
        set_win_tile_xy(15, 3, txt_tile('0' + t));
        if (++f >= 60) { f = 0; if (t == 0) return 0; t--; sfx_play(SFX_SELECT); }
        if (joy_new & J_A) return 1;
        if (joy_new & J_START) return 0;
    }
}

// ------------------------------------------------------------------ main loop
uint8_t stage_run(uint8_t sid) BANKED {
    uint8_t paused = 0, sec_t = 0, was_flash = 0;
    stage_id = sid;
    SD = &stage_defs[sid];
    stage_load();
    score = 0; hits_taken = 0; continues_used = 0; play_seconds = 0; midboss_killed = 0; kill_count = 0;
    lives = 3 + g_gear[GEAR_HULL]; hearts = 5; bombs = (HUNTER ? 3 : 5) + g_gear[GEAR_BOMBS]; wtype = 0; wlevel = g_gear[GEAR_CANNON];
    bud_n = g_gear[GEAR_KENNEL] >= 2 ? 1 : 0; bud_hint_done = 0;
    bounty_pts = 0; bounty_out = OUT_NONE; mid_out = OUT_NONE;
    if (start_cp == 2 && wlevel < 1) wlevel = 1;              // boss practice: start powered up
    next_extend = 30000UL;
    restart_from(start_cp);
    while (1) {
        wait_vbl_done();
        if (flash_t) {
            flash_t--;
            if (flash_t) ld_bkg_pal(BANK_A2, 0, 1, white_pal);
            else set_pal_slot(0, cur_bp[0]);
        }
        if (k_hpmax) {
            if (k_hitflash) { if (!was_flash) set_pal_slot(6, SD->hit_bp); was_flash = 1; }
            else if (was_flash) { set_pal_slot(6, SD->boss_bp6); was_flash = 0; }
        }
        input_poll();
        if ((joy_new & J_START) && st != ST_DYING) {
            paused ^= 1;
            sfx_play(SFX_SELECT);
            if (paused) txt_put(13, hud_row, "PAUSE  ", 1, 5 | 0x80);
            else hud_dirty = 1;
        }
        if (paused) continue;
        if (++sec_t >= 60) { sec_t = 0; play_seconds++; }

        if (scroll_on && !mb_active && st != ST_BOSS) {
            if (frame_count & 1) {
                if (pos < (SD->n_rows - 18) * 8) { pos++; scroll_rows(); }
                else if (st == ST_PLAY) {
                    st = ST_BOSS; scroll_on = 0;
                    logic_boss_start();
                    hud_init(1);
                }
            }
        }
        if (checkpoint < 2 && (pos >> 3) >= SD->cp[checkpoint + 1] && SD->cp[checkpoint + 1]) checkpoint++;
        comm_update();
        player_update();
        buddies_update();
        pbullets_update();
        enemies_common();
        logic_tick();
        hazards_update();
        if (boss_comm) { comm_queue_add(boss_comm); boss_comm = 0; }
        if (k_hpmax) boss_bar();
        ebullets_update();
        collide_bodies();
        pickups_update();
        if (bomb_t) bomb_t--;

        if (st == ST_DYING) {
            st_t++;
            if (st_t == 70) {
                if (lives > 1) {
                    lives--;
                    hearts = 5;
                    if (HUNTER) { if (wlevel > g_gear[GEAR_CANNON]) wlevel--; }
                    if (bombs < 3 + g_gear[GEAR_BOMBS]) bombs = 3 + g_gear[GEAR_BOMBS];
                    p_alive = 1; p_inv = 150; px = 72 + XO; py = k_hpmax ? 100 + YO : 108 + YO; p_intro = 0;
                    st = k_hpmax ? ST_BOSS : ST_PLAY;
                    hud_dirty = 1;
                } else {
                    if (!continue_prompt()) return RES_GAMEOVER;
                    continues_used++;
                    if (HUNTER) { score = 0; bounty_pts = 0; } else bounty_pts >>= 1;   // continues cost the bounty
                    lives = 3 + g_gear[GEAR_HULL]; hearts = 5; bombs = (HUNTER ? 3 : 5) + g_gear[GEAR_BOMBS];
                    if (HUNTER) wlevel = g_gear[GEAR_CANNON];
                    restart_from(checkpoint);
                    continue;
                }
            }
        }
        if (k_hpmax && k_state == 22 && st != ST_CLEAR) {
            st = ST_CLEAR; st_t = 0;
            music_play(MUS_CLEAR);
        }
        if (st == ST_CLEAR) {
            st_t++;
            p_inv = 0;
            if (st_t > 60 && py > 8 + YO) py -= 2;
            if (st_t >= 150) { oam_clear(); return RES_CLEAR; }
        }
        if (hud_dirty) hud_draw();
        raster();
        draw_player();
        draw_all();
    }
}
