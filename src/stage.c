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
#include "weapon_fx.h"
#include "weapon_fx2.h"

extern const uint8_t comm_buddy_hint, comm_buddy_got;

static uint16_t next_row;
static uint8_t checkpoint;
static uint8_t pxf, pyf;
static uint8_t p_inv, p_fire_cd, p_alive, p_intro;
static uint8_t bomb_t, flash_t;
static uint8_t pb_on[MAXPB], pb_x[MAXPB], pb_y[MAXPB], pb_type[MAXPB], pb_dmg[MAXPB], pb_tgt[MAXPB];
static int8_t pb_vx[MAXPB], pb_vy[MAXPB];
static uint8_t hud_row;

// ------------------------------------------------------------------ weapon flair
// Each weapon has its own colour-blind-safe OBJ palette 2 (shots, buddies, muzzle, hits):
// cool hues only (enemy bullets own the warm ones), told apart by brightness + shape too.
// Max level adds a slow, low-contrast shimmer. Bombs and power-ups get a ring burst;
// the bomb flash is a soft single fade instead of a hard white frame.
// Shot types: 0 twin, 1 spread, 2 wisp, 3 twin heavy lance (lv 2+), 4 buddy shot,
// 5 piercing lance (Lance Rune), 6 ricochet star (Ricochet Rune), 7 chained wisp (Chain Rune).
static uint8_t pop_t;                                    // HUD name pop frames left (weapon names over the score)
static const char *pop_s;
#define PB_BUDDY 4
#define PB_PIERCE 5
#define PB_RICO 6
#define PB_CHAIN 7
#define HOMING(t) ((t) == 2 || (t) == PB_CHAIN)
#define NHS 4
static uint8_t hs_t[NHS], hs_x[NHS], hs_y[NHS], hs_k[NHS], hs_i;   // hit sparks (ring buffer); kind 0 sparks, 1 star, 2 frost
static uint8_t pb_was[MAXPB];                            // shot drawn last frame, as type + 1 (a shot that vanished in the field hit something)
static uint8_t mz_t, mz_k;                               // muzzle flash frames left, and which weapon fired
static uint8_t ring_t, ring_in, ring_a;                  // bomb rune circle (out) / power-up ring (in), circle angle
static uint8_t bfx_t;                                    // bomb palette fade
static uint8_t wpal_cur, sw_t;                           // weapon palette shown; ship flash after a weapon change
static uint8_t zap_t, zap_x, zap_y;                      // Chain Rune spark between two targets
static uint8_t kp_t, kp_cx, kp_cy, kp_big;               // kill pop (shards)
static const uint8_t hit_kind[8] = {0, 1, 2, 0, 0, 0, 1, 2};
static void hit_spark(uint8_t x, uint8_t y, uint8_t k) { hs_i = (hs_i + 1) & (NHS - 1); hs_t[hs_i] = 8; hs_x[hs_i] = x; hs_y[hs_i] = y; hs_k[hs_i] = k; }
#define fx_add oam_add_fx
// a weapon's max-level rune (bought at Forge's armory) is live
static uint8_t rune_on(void) {
    return wlevel == 2 && g_gear[wtype == 0 ? GEAR_LANCE : wtype == 1 ? GEAR_RICO : GEAR_CHAIN];
}
static const uint16_t wpn_flash[4] = {0x0000, 0x7FFF, 0x7FFF, 0x6318};
static void name_pop(const char *s) { pop_s = s; pop_t = 90; hud_dirty = 1; }
static void wpal_update(void) {
    static const uint8_t step[4] = {0, 1, 2, 1};
    uint8_t w = (wtype > 2 ? 0 : wtype) * 3 + (wlevel >= 2 ? step[(frame_count >> 3) & 3] : 0);
    if (sw_t) {                                          // weapon change: the ship flashes white twice
        sw_t--;
        if (sw_t & 4) { set_sprite_palette(0, 1, wpn_flash); set_sprite_palette(1, 1, wpn_flash); }
        else { ld_spr_pal(BANK_A2, 0, 2, obj_pals); wpal_cur = 0xFF; }
    }
    if (w == wpal_cur) return;
    wpal_cur = w;
    set_sprite_palette(2, 1, wpn_pals + (uint16_t)w * 4);
    if (!(sw_t & 4)) set_sprite_palette_entry(1, 1, wpn_pals[(uint16_t)w * 4 + 2]);   // exhaust burns in the weapon's colour
}
// soft bomb flash: BG palette 0 eased from a pale blue tint back to normal (one fade, no strobe)
static void bomb_fade(uint8_t k) {
    uint16_t vp[4], c;
    uint8_t i, r, g, b;
    ld_copy(BANK_A2, vp, bg_pal_lib + (uint16_t)cur_bp[0] * 4, 8);
    for (i = 0; i < 4; i++) {
        c = vp[i]; r = c & 31; g = (c >> 5) & 31; b = (c >> 10) & 31;
        if (r < 22) r += (uint8_t)(((22 - r) * k) >> 3);        // toward pale blue #B0D0F8,
        if (g < 26) g += (uint8_t)(((26 - g) * k) >> 3);        // dimmer than the old white
        b += (uint8_t)(((31 - b) * k) >> 3);
        vp[i] = r | ((uint16_t)g << 5) | ((uint16_t)b << 10);
    }
    set_bkg_palette(0, 1, vp);
}

// buddies: friendly baby cryptids that trail the ship and shoot
uint8_t bud_n;
static uint8_t bud_cd, hist_x[32], hist_y[32], hist_i, bud_hint_done;

// ------------------------------------------------------------------ HUD (window)
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
    if (pop_t) for (i = 0; i < 7; i++) hud_tile(13 + i, txt_tile(pop_s[i]));
    else score_tiles(hud_buf + 13);
    set_win_tiles(0, hud_row, 20, 1, hud_buf);
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
        pb_spawn(x, y, 0, -6, rune_on() ? PB_PIERCE : wlevel ? 3 : 0, wlevel ? 3 : 2);
        if (wlevel == 2) { pb_spawn(x - 7, y + 6, 0, -6, 0, 2); pb_spawn(x + 7, y + 6, 0, -6, 0, 2); }
        p_fire_cd = wlevel == 1 ? 5 : 6;
        sfx_play(SFX_SHOT);
        mz_t = 4; mz_k = wtype;
        break;
    case 1:
        pb_spawn(x, y, 0, -5, 1, 2);
        if (wlevel == 0) { pb_spawn(x - 2, y, -2, -5, 1, 2); pb_spawn(x + 2, y, 2, -5, 1, 2); }
        else if (wlevel == 1) { pb_spawn(x - 2, y, -1, -5, 1, 2); pb_spawn(x + 2, y, 1, -5, 1, 2); pb_spawn(x - 4, y, -3, -4, 1, 2); pb_spawn(x + 4, y, 3, -4, 1, 2); }
        else {
            n = rune_on() ? PB_RICO : 1;                        // Ricochet Rune: the outer stars bounce off the walls
            pb_spawn(x - 2, y, -1, -5, 1, 2); pb_spawn(x + 2, y, 1, -5, 1, 2); pb_spawn(x - 4, y, -2, -5, n, 2); pb_spawn(x + 4, y, 2, -5, n, 2);
        }
        p_fire_cd = 9;
        sfx_play(SFX_SHOT2);
        mz_t = 4; mz_k = wtype;
        break;
    default:
        n = nearest_target(x + 4);
        pb_spawn(x, y, 0, -4, 2, 4);
        if (wlevel >= 1) pb_spawn(x - 8, y + 6, -1, -4, 2, 3);
        if (wlevel >= 2) pb_spawn(x + 8, y + 6, 1, -4, 2, 3);
        for (i = 0; i < MAXPB; i++) if (pb_on[i] && pb_type[i] == 2 && pb_tgt[i] == 0xFF) pb_tgt[i] = n;
        p_fire_cd = 10;
        sfx_play(SFX_SHOT);
        mz_t = 4; mz_k = wtype;
        break;
    }
}

static void use_bomb(void) {
    uint8_t i;
    if (!bombs || bomb_t) return;
    bombs--; hud_dirty = 1;
    bomb_t = 60; bfx_t = 9; ring_t = 28; ring_in = 0; ring_a = 0; k_shake = 8;
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
        pb_spawn(bx + 4, by - 2, 0, -5, PB_BUDDY, 1);
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

// the nearest live enemy to (x, y) other than `skip`, within about 72 px (Chain Rune)
static uint8_t chain_target(uint8_t x, uint8_t y, uint8_t skip) {
    uint8_t i, best = 0xFF, bd = 72, d, dy, ex, ey;
    for (i = 0; i < MAXE; i++) {
        uint8_t t = e_type[i];
        if (i == skip || t == E_NONE || t == E_BOOM || (e_tf[t] & TF_NOHIT) || (e_fl[i] & EF_HIDDEN)) continue;
        ex = (e_x[i] >> 8) + (e_style[t] == SY_32 ? 16 : 8); ey = (e_y[i] >> 8) + 8;
        if (ey < 24 || ey > 180) continue;
        d = ex > x ? ex - x : x - ex; dy = ey > y ? ey - y : y - ey;
        if (dy > d) d = dy;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}
// which enemy a shot at (bx, by) touches; chain segments report via *seg
static uint8_t shot_hits(uint8_t bx, uint8_t by, uint8_t *seg) {
    uint8_t j, t, s;
    *seg = 0;
    for (j = 0; j < MAXE; j++) {
        t = e_type[j];
        if (t == E_NONE || (e_tf[t] & TF_NOHIT) || (e_fl[j] & EF_HIDDEN)) continue;
        switch (e_style[t]) {
        case SY_32:
            if (NEAR(bx, (e_x[j] >> 8) + 16, 15) && NEAR(by, (e_y[j] >> 8) + 12, 11)) return j;
            break;
        case SY_EEL:
            if ((e_y[j] >> 8) > 20 && NEAR(bx, (e_x[j] >> 8) + 8, 9) && NEAR(by, (e_y[j] >> 8) + 8, 9)) return j;
            if (e_c[j] < NCH) for (s = 0; s < NCSEG; s++)
                if (NEAR(bx, ch_x[e_c[j]][s], 5) && NEAR(by, ch_y[e_c[j]][s], 5)) { *seg = 1; return j; }
            break;
        default:
            if ((e_y[j] >> 8) > 20 && NEAR(bx, (e_x[j] >> 8) + 8, 9) && NEAR(by, (e_y[j] >> 8) + 8, 9)) return j;
        }
    }
    return 0xFF;
}
static void pbullets_update(void) {
    uint8_t i, j, bx, by, tx, t, seg, ty;
    for (i = 0; i < MAXPB; i++) {
        if (!pb_on[i]) continue;
        t = pb_type[i];
        if (HOMING(t) && pb_tgt[i] != 0xFF) {
            j = pb_tgt[i];
            if (e_type[j] == E_NONE || e_type[j] == E_BOOM || (e_fl[j] & EF_HIDDEN)) pb_tgt[i] = 0xFF;
            else {
                tx = (e_x[j] >> 8) + (e_style[e_type[j]] == SY_32 ? 12 : 4);
                if (tx > pb_x[i] + 1 && pb_vx[i] < 3) pb_vx[i]++;
                else if (tx + 1 < pb_x[i] && pb_vx[i] > -3) pb_vx[i]--;
                if (t == PB_CHAIN) {                         // a chained wisp can dive or climb to its target
                    ty = (e_y[j] >> 8) + 4;
                    if (ty > pb_y[i] + 1 && pb_vy[i] < 3) pb_vy[i]++;
                    else if (ty + 1 < pb_y[i] && pb_vy[i] > -4) pb_vy[i]--;
                }
            }
        } else if (HOMING(t) && k_hpmax && k_state < 20) {
            tx = boss_cx() - 4;
            if (tx > pb_x[i] + 1 && pb_vx[i] < 2) pb_vx[i]++;
            else if (tx + 1 < pb_x[i] && pb_vx[i] > -2) pb_vx[i]--;
        }
        pb_x[i] += pb_vx[i]; pb_y[i] += pb_vy[i];
        if (t == PB_RICO && pb_tgt[i] == 0xFF && (pb_x[i] < 10 || pb_x[i] > 166)) {
            pb_vx[i] = -pb_vx[i]; pb_tgt[i] = 0;             // one bounce, with a flare on the wall
            pb_x[i] = pb_x[i] < 10 ? 10 : 166;
            hit_spark(pb_x[i], pb_y[i], 1);
            sfx_play(SFX_TICK);
        }
        if (pb_y[i] < 20 || pb_y[i] > 200 || pb_x[i] < 6 || pb_x[i] > 172) { pb_on[i] = 0; continue; }
        bx = pb_x[i] + 4; by = pb_y[i] + 3;
        j = shot_hits(bx, by, &seg);
        if (j != 0xFF) {
            if (t == PB_PIERCE) {                            // Lance Rune: through everything, each enemy once
                if (j != pb_tgt[i]) { pb_tgt[i] = j; e_damage(j, seg ? 1 : pb_dmg[i]); hit_spark(bx - 4, by - 3, 0); }
            } else {
                e_damage(j, seg ? 1 : pb_dmg[i]);
                pb_on[i] = 0;
                if (t == 2 && rune_on() && wtype == 2) {    // Chain Rune: the wisp jumps to the next enemy
                    uint8_t n = chain_target(bx, by, j);
                    if (n != 0xFF) {
                        pb_spawn(bx - 4, by - 3, 0, -2, PB_CHAIN, 3);
                        for (tx = 0; tx < MAXPB; tx++) if (pb_on[tx] && pb_type[tx] == PB_CHAIN && pb_tgt[tx] == 0xFF) pb_tgt[tx] = n;
                        zap_t = 8; zap_x = bx; zap_y = by;
                    }
                }
                continue;
            }
        }
        // shots knock enemy bullets out of the air; big orbs need rune rounds (2 hits)
        for (j = 0; j < MAXEB; j++) {
            if (eb_on[j] != 1) continue;
            tx = (eb_x[j] >> 8) + 3; ty = (eb_y[j] >> 8) + 3;
            if (NEAR(bx, tx, 5) && NEAR(by, ty, 7)) {
                if (eb_big[j]) {
                    pb_on[i] = 0;
                    if (!g_gear[GEAR_RUNES]) { sfx_play(SFX_TICK); break; }
                    if (eb_big[j] == 1) { eb_big[j] = 2; sfx_play(SFX_TICK); break; }
                } else if (t != PB_PIERCE) pb_on[i] = 0;    // the lance plows through small bullets
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
    uint8_t i, kind, cycle, n;
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
                n = cycle != wtype;                         // new weapon (name pop) or a power-up
                if (!n) { if (wlevel < 2) wlevel++; else add_score(1000); }
                else wtype = cycle;
                sfx_play(SFX_POWER);
                if (!ring_t) { ring_t = 16; ring_in = 1; }
                sw_t = 16;
                if (rune_on()) name_pop(wtype == 0 ? "LANCE! " : wtype == 1 ? "BOUNCE!" : "CHAIN! ");
                else if (n) name_pop(wtype == 0 ? "TWIN   " : wtype == 1 ? "SPREAD " : "WISP   ");
                else name_pop(wlevel == 2 ? "MAX!   " : "POWER! ");
            } else if (kind == PK_BOMB) { if (bombs < 9) bombs++; sfx_play(SFX_PICKUP); }
            else if (kind == PK_HEART) { if (hearts < MAXHEARTS) hearts++; else add_score(500); sfx_play(SFX_PICKUP); }
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

// CADET / PILOT: now and then a random on-screen enemy takes one aimed shot,
// so the run never plays itself (Hunter keeps its own hand-tuned patterns).
static uint8_t press_t;
static void pressure_fire(void) {
    uint8_t i, n, x, y;
    if (HUNTER || st != ST_PLAY || !p_alive || p_inv || mb_active) return;
    if (++press_t < (MIDMODE ? 75 : 150)) return;
    press_t = 0;
    n = rnd() & (MAXE - 1);
    for (i = 0; i < MAXE; i++, n = (n + 1) & (MAXE - 1)) {
        if (e_type[n] < E_KL || e_type[n] == E_ICE || (e_tf[e_type[n]] & (TF_MID | TF_NOHIT)) || (e_fl[n] & EF_HIDDEN)) continue;
        x = (e_x[n] >> 8) + 8; y = (e_y[n] >> 8) + 8;
        if (y < YO + 4 || y > py - 40 || x < XO + 4 || x > XO + 156) continue;
        eb_fire(x, y, aim(x, y, px + 8, py + 12), MIDMODE ? 22 : 16, 0);
        return;
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
// player shot tile / attributes by [type * 2 + animation phase]; type 3 = twin heavy lance
static const uint8_t shot_t[16] = {ST_SHOT_TWIN, ST_SHOT_TWIN, ST_SHOT_SPREAD, ST_SHOT_SPREADB,
                                   ST_SHOT_WISP, ST_SHOT_WISP, ST_SHOT_TWIN2, ST_SHOT_TWIN2,
                                   0, 0, FX_LANCE_A, FX_LANCE_B, ST_SHOT_SPREAD, ST_SHOT_SPREADB, ST_SHOT_WISP, ST_SHOT_WISP};
static const uint8_t shot_p[16] = {2, 2, 2, 2, 2, 2 | 0x20, 2, 2, 2 | 8, 2 | 8, 2 | 8, 2 | 8, 2, 2, 2, 2 | 0x20};
static const uint8_t bud_shot[4] = {FX_BUD_INK, FX_BUD_BOLT, FX_BUD_ARROW, FX_BUD_FROST};
static void draw_player(void) {
    uint8_t ox, oy, vis;
    vis = p_alive && !(p_inv && (p_inv & 4) && !p_intro);
    if (!vis) { oam_fixed(0, 0, 0, 0, 0); oam_fixed(1, 0, 0, 0, 0); oam_fixed(2, 0, 0, 0, 0); oam_fixed(3, 0, 0, 0, 0); return; }
    ox = px - 8; oy = py - 16;
    if (mz_t >= 3) oy++;                                 // recoil kick
    flame_t++;
    oam_fixed(0, ox, oy, ST_SHIP_TOP, 0);
    oam_fixed(1, ox + 8, oy, ST_SHIP_TOP + 2, 0);
    oam_fixed(2, ox, oy + 16, (flame_t & 4) ? ST_SHIP_BOT : ST_SHIP_BOTB, 1);
    oam_fixed(3, ox + 8, oy + 16, ((flame_t & 4) ? ST_SHIP_BOT : ST_SHIP_BOTB) + 2, 1);
}

static void draw_all(void) {
    uint8_t i, x, y, t, p, f, s, c;
    oam_begin();
    if (p_alive) { oam_band[(uint8_t)(py - 16) >> 4] += 2; oam_band[py >> 4] += 2; }   // the ship's fixed slots
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
        s = pk_t[i] & 63;
        if (s >= 52) {                               // a quick spin: 3/4, edge on, 3/4 mirrored
            t = (s < 56 || s >= 60) ? FX_CAP_34 : FX_CAP_EDGE;
            if (s >= 60) { oam_add(x, y, t + 2, 7 | 8 | 0x20); oam_add(x + 8, y, t, 7 | 8 | 0x20); }
            else { oam_add(x, y, t, 7 | 8); oam_add(x + 8, y, t + 2, 7 | 8); }
            continue;
        }
        switch (pk_kind[i]) {
            case PK_WEAP: { uint8_t cy = (pk_t[i] / (HUNTER ? 64 : 110)) % 3; t = cy == 0 ? ST_PK_T : cy == 1 ? ST_PK_S : ST_PK_H; } break;
            case PK_BOMB: t = ST_PK_B; break;
            case PK_HEART: t = ST_PK_HEART; break;
            default: t = ST_PK_1UP;
        }
        oam_add(x, y, t, 7); oam_add(x + 8, y, t + 2, 7);
        if (s >= 8 && s < 16 && (s & 2)) oam_add_fx(x + 9, y - 3, ST_SPARK, 7);   // a glint
    }
    for (i = 0; i < MAXEB; i++) {
        if (!eb_on[i]) continue;
        if (eb_on[i] > 1) { if (eb_on[i] & 2) oam_add((eb_x[i] >> 8) - 8, (eb_y[i] >> 8) - 16 - 2, ST_SPARK, 7); continue; }
        oam_add((eb_x[i] >> 8) - 8, (eb_y[i] >> 8) - 16, eb_big[i] ? ST_EB_INK : ST_EB_ORB, 3);
    }
    f = (frame_count >> 2) & 1;
    for (i = 0; i < MAXPB; i++) {
        if (!pb_on[i]) {
            if (pb_was[i]) {                 // gone since last frame without leaving the field: it hit
                if (pb_y[i] >= 20 && pb_y[i] <= 200 && pb_x[i] >= 6 && pb_x[i] <= 172) hit_spark(pb_x[i], pb_y[i], hit_kind[pb_was[i] - 1]);
                pb_was[i] = 0;
            }
            continue;
        }
        pb_was[i] = pb_type[i] + 1;
        c = (uint8_t)(pb_type[i] << 1) | ((f ^ i) & 1);          // spread stars spin, wisp tails wiggle, the lance ripples
        if (pb_type[i] == PB_BUDDY) oam_add(pb_x[i] - 8, pb_y[i] - 16, bud_shot[stage_id], (f ^ i) ? 2 | 8 : 2 | 8 | 0x20);
        else oam_add(pb_x[i] - 8, pb_y[i] - 16, shot_t[c], shot_p[c]);
    }
    // weapon flair: added last and only on lighter frames (a busy frame is the one that would
    // run long and slow the game down); each sprite also has to fit the scanline budget
    c = oam_n < 24;
    for (i = 0; i < NHS; i++) {                      // impacts: twin sparks, spread star bursts, wisp frost puffs
        if (!hs_t[i]) continue;
        hs_t[i]--;
        if (!c) continue;
        if (hs_k[i] == 0) fx_add(hs_x[i] - 8, hs_y[i] - 16 - 2, hs_t[i] >= 4 ? ST_HIT : ST_SPARK, (hs_t[i] & 2) ? 2 : 2 | 0x20);
        else fx_add(hs_x[i] - 8, hs_y[i] - 16 - 2, (hs_k[i] == 1 ? FX_IMP_STAR_A : FX_IMP_FROST_A) + (hs_t[i] >= 4 ? 0 : 2), 2 | 8);
    }
    if (kp_t) {                                      // kill pop: four shards fly off
        uint8_t d;
        kp_t--;
        d = (uint8_t)((10 - kp_t) << (kp_big ? 2 : 1));
        if (c) {
            x = kp_cx - 8 - 4; y = kp_cy - 16 - 4;
            fx_add(x - d, y - d, FX_SHARD, 7 | 8); fx_add(x + d, y - d, FX_SHARD, 7 | 8 | 0x20);
            fx_add(x - d, y + d, FX_SHARD, 7 | 8 | 0x40); fx_add(x + d, y + d, FX_SHARD, 7 | 8 | 0x60);
        }
    }
    if (zap_t) {                                     // Chain Rune: the spark jumping to its next target
        zap_t--;
        if (c) fx_add(zap_x - 8 - 4, zap_y - 16 - 12, (zap_t & 2) ? FX_ZAP_A : FX_ZAP_B, 2 | 8);
    }
    if (mz_t) {
        mz_t--;
        if (c && p_alive && mz_t) {
            t = mz_t < 2 ? ST_SPARK : mz_k == 0 ? ST_MUZZLE : mz_k == 1 ? FX_MUZ_SPREAD : FX_MUZ_WISP;
            fx_add(px + 4 - 8, py - 6 - 16 + 2, t, (t >= FX2_BASE && mz_t >= 2 && mz_k) ? 2 | 8 : 2);
        }
    }
    if (ring_t) {
        uint8_t r, cx = px + 8 - 4 - 8, cy = py + 10 - 4 - 16;
        ring_t--;
        if (ring_in) {                               // power-up: sparks pull in
            uint8_t d;
            r = (ring_t << 1) + 4; d = r - (r >> 2) - (r >> 4);
            t = (ring_t & 4) ? ST_HIT : ST_SPARK;
            if (p_alive) {
                fx_add(cx, cy - r, t, 2); fx_add(cx, cy + r, t, 2);
                fx_add(cx - r, cy, t, 2); fx_add(cx + r, cy, t, 2);
                fx_add(cx - d, cy - d, t, 2); fx_add(cx + d, cy - d, t, 2);
                fx_add(cx - d, cy + d, t, 2); fx_add(cx + d, cy + d, t, 2);
            }
        } else {                                     // bomb: Forge's rune circle spins outward (weapon colours, so it never reads as enemy fire)
            static const uint8_t glyph[3] = {ST_RUNE, FX_RUNE_B, FX_RUNE_C};
            uint8_t k, a, g;
            r = (uint8_t)((28 - ring_t) * 3);
            ring_a += 2;
            for (k = 0; k < 8; k++) {
                a = (uint8_t)(ring_a + (k << 3)) & 63;
                g = glyph[(k + (ring_t >> 3)) % 3];
                fx_add(cx + (int8_t)(((int16_t)sin64[a] * r) >> 7), cy + (int8_t)(((int16_t)COS64(a) * r) >> 7), g, g == ST_RUNE ? 2 : 2 | 8);
            }
        }
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
    bomb_t = 0; flash_t = 0; bfx_t = 0; ring_t = 0; mz_t = 0; hs_i = 0; sw_t = 0; pop_t = 0; zap_t = 0; kp_t = 0; kp_new = 0;
    memset(hs_t, 0, sizeof hs_t); memset(pb_was, 0, sizeof pb_was);
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
    set_sprite_data(ST_SHOT_TWIN, 6, fx_shot_tiles);          // weapon flair art (bank 1)
    set_sprite_data(FX_FIRST, 8, fx_new_tiles);
    VBK_REG = 1; ld_spr_data(BANK_FX2, FX2_BASE, FX2_NT, fx2_tiles); VBK_REG = 0;   // weapon flair part 2
    wpal_cur = 0xFF;
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
    comm_reset();                                   // drop any crew line so the prompt owns the panel
    txt_fill(0, 2, 20, 4, txt_tile(' '), 5 | 0x80, 1);
    n_lcdc = LCDC_COMM; n_scx = 0; n_scy = 16; n_s1_line = 32; n_s1_lcdc = LCDC_PLAY; n_s1_scx = 0; n_s1_scy = (uint8_t)(112 - pos); n_s2_line = 0;
    oam_clear();
    music_play(MUS_OVER);
    txt_put(5, 3, "GAME OVER", 1, 5 | 0x80);         // a beat to let the loss land
    for (f = 0; f < 100; f++) {
        wait_vbl_done();
        if (f == 50) { txt_put(5, 3, "         ", 1, 5 | 0x80); }
        if (f == 60) { txt_put(5, 3, "GAME OVER", 1, 5 | 0x80); }
    }
    f = 0;
    txt_fill(0, 2, 20, 4, txt_tile(' '), 5 | 0x80, 1);
    if (!HUNTER) {
        txt_put(5, 3, "TRY AGAIN!", 1, 5 | 0x80);
        txt_put(3, 4, "YOU CAN DO IT", 1, 5 | 0x80);
        wait_frames(100);
        return 1;
    }
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
    lives = 3 + g_gear[GEAR_HULL]; hearts = MAXHEARTS; bombs = (HUNTER ? 3 : 5) + g_gear[GEAR_BOMBS]; wtype = 0; wlevel = g_gear[GEAR_CANNON];
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
        } else if (bfx_t) {
            bfx_t--;
            if (bfx_t) bomb_fade(bfx_t > 5 ? 5 : bfx_t);
            else set_pal_slot(0, cur_bp[0]);
        }
        wpal_update();
        if (pop_t && !--pop_t) hud_dirty = 1;
        if (kp_new) { kp_new = 0; kp_t = 10; kp_cx = kp_x; kp_cy = kp_y; kp_big = kp_bigf; }
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
                    bstar_take((uint8_t)(112 - pos));          // clear the stars in the boss band so they don't sway with it
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
        pressure_fire();
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
                    hearts = MAXHEARTS;
                    if (HUNTER) { if (wlevel > g_gear[GEAR_CANNON]) wlevel--; }
                    if (bombs < 3 + g_gear[GEAR_BOMBS]) bombs = 3 + g_gear[GEAR_BOMBS];
                    p_alive = 1; p_inv = 150; px = 72 + XO; py = k_hpmax ? 100 + YO : 108 + YO; p_intro = 0;
                    st = k_hpmax ? ST_BOSS : ST_PLAY;
                    hud_dirty = 1;
                } else {
                    if (!continue_prompt()) return RES_GAMEOVER;
                    continues_used++;
                    if (HUNTER) { score = 0; bounty_pts = 0; } else bounty_pts >>= 1;   // continues cost the bounty
                    lives = 3 + g_gear[GEAR_HULL]; hearts = MAXHEARTS; bombs = (HUNTER ? 3 : 5) + g_gear[GEAR_BOMBS];
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
