// Mission 7 "The Tree Line": sprites and the raster split (see ground.c). Autobanked.
#pragma bank 255
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include "core.h"
#include "assets.h"
#include "ground_data.h"
#include "game.h"
#include "objs.h"
#include "ground_int.h"

// ------------------------------------------------------------------ drawing
static void spr16(uint8_t x, uint8_t y, uint8_t tile, uint8_t prop) {
    if (prop & 0x20) { oam_add(x - 16, y - 24, tile + 2, prop); oam_add(x - 8, y - 24, tile, prop); }
    else { oam_add(x - 16, y - 24, tile, prop); oam_add(x - 8, y - 24, tile + 2, prop); }
}
static const uint8_t g_tile[4] = {GR_ST_G_DOWN, GR_ST_G_UP, GR_ST_G_SIDE, GR_ST_G_SIDE};
static void draw_all(void) {
    uint8_t i, k, x, y, t, a, bb = band_bot() + YO;
    if (st == ST_DYING || (p_inv && (p_inv & 4)) || st == ST_CLEAR) { oam_fixed(0, 0, 0, 0, 0); oam_fixed(1, 0, 0, 0, 0); }
    else {
        t = g_tile[facing] + ((walk_t & 8) ? 4 : 0);
        if (facing == 3) { oam_fixed(0, px - 16, py - 24, t + 2, 6 | 8 | 0x20); oam_fixed(1, px - 8, py - 24, t, 6 | 8 | 0x20); }
        else { oam_fixed(0, px - 16, py - 24, t, 6 | 8); oam_fixed(1, px - 8, py - 24, t + 2, 6 | 8); }
    }
    oam_fixed(2, 0, 0, 0, 0); oam_fixed(3, 0, 0, 0, 0);
    oam_begin();
    if (gr_swing) {                                          // the chain-mace goes round once
        a = (uint8_t)((16 - gr_swing) * 4 + facing * 16) & 63;
        for (k = 0; k < 3; k++) {
            i = k == 0 ? 22 : k == 1 ? 14 : 7;
            x = px + (int8_t)(((int16_t)sin64[a] * i) >> 6); y = py + (int8_t)(((int16_t)COS64(a) * i) >> 6);
            if (y > bb) oam_add(x - 12, y - 24, k ? GR_ST_LINK : GR_ST_BALL, 6 | 8);
        }
    }
    if (howl_t) {
        howl_t--; k = (36 - howl_t) * 2 + 8;
        for (i = 0; i < 8; i++) {
            x = px + (int8_t)(((int16_t)sin64[i * 8] * k) >> 6); y = py + (int8_t)(((int16_t)COS64(i * 8) * k) >> 6);
            if (y > bb && y < FIELD_BOT + YO - 4 && x > 12 && x < 172) oam_add(x - 12, y - 24, GR_ST_HOWL, 6 | 8);
        }
    }
    if (gr_warn && (gr_t & 4)) { oam_add(gr_warn_x - 12, bb - 14, ST_WARN, 7); oam_add(gr_warn_x - 12, (py < bb + 40 ? bb + 44 : py + 6) - 24, ST_WARN, 7); }
    for (i = 0; i < MAXTB; i++) if (tb_on[i]) oam_add((tb_x[i] >> 8) - 12, (tb_y[i] >> 8) - 24, GR_ST_BOLT, 6 | 8);
    for (i = 0; i < MAXG; i++) {
        t = gr_gt[i];
        if (!t) continue;
        x = gr_gx[i]; y = gr_gy[i];
        if ((gfl[i] & 3) == 3) continue;                     // hit blink
        if (t != G_CLAW && (y < bb + 6 || y > YO + FIELD_BOT - 2)) continue;
        a = (gr_t >> 3) & 1;
        switch (t) {
        case G_CRAWL: spr16(x, y, a ? GR_ST_CRAWL2 : GR_ST_CRAWL, 4 | 8); break;
        case G_SPIT: spr16(x, y, GR_ST_SPIT, 4 | 8); break;
        case G_TROOP: spr16(x, y, a ? GR_ST_TROOP2 : GR_ST_TROOP, 5 | 8); break;
        case G_CHARGE: if (gr_gs[i] != 2 || (gr_t & 2)) spr16(x, y, GR_ST_CHARGE, (gr_gs[i] == 2 ? 3 : 4) | 8); break;
        case G_BRUISE:
            for (k = 0; k < 8; k++) oam_add(x - 24 + (k & 3) * 8, y - 32 + (k >> 2) * 16, GR_ST_BRUISER + k * 2, 5 | 8);
            break;
        case G_CLAW:
            for (k = y - 32; k > bb + 8 && k > y - 96; k -= 16) { oam_add(x - 16, k - 32, GR_ST_CLAW, 4 | 8); oam_add(x - 8, k - 32, GR_ST_CLAW + 2, 4 | 8); }     // the arm
            oam_add(x - 16, y - 48, GR_ST_CLAW, 4 | 8); oam_add(x - 8, y - 48, GR_ST_CLAW + 2, 4 | 8);
            oam_add(x - 16, y - 32, GR_ST_CLAW + 4, 4 | 8); oam_add(x - 8, y - 32, GR_ST_CLAW + 6, 4 | 8);
            break;
        }
    }
    for (i = 0; i < MAXEB; i++) if (eb_on[i]) oam_add((eb_x[i] >> 8) - 8, (eb_y[i] >> 8) - 16, eb_big[i] ? ST_EB_INK : ST_EB_ORB, eb_on[i] == 2 ? (6 | 0) : 3);
    for (i = 0; i < MAXE; i++) if (e_type[i] == E_BOOM) {
        if (++e_t[i] >= 20) { e_type[i] = E_NONE; continue; }
        x = e_x[i] >> 8; y = e_y[i] >> 8;
        oam_add(x - 16, y - 24, ST_EXPL0 + (e_t[i] / 5) * 4, 7); oam_add(x - 8, y - 24, ST_EXPL0 + (e_t[i] / 5) * 4 + 2, 7);
    }
    if (gr_phase == P_END && bounty_out == OUT_CAPTURE && end_t < 120) {      // Forge's rune ring closes on the eyes
        k = end_t < 90 ? 60 - (end_t >> 1) : 15;
        for (i = 0; i < 8; i++) {
            x = XO + 80 + (int8_t)(((int16_t)sin64[(i * 8 + end_t) & 63] * k) >> 6); y = YO + 14 + (int8_t)(((int16_t)COS64((i * 8 + end_t) & 63) * (k >> 1)) >> 6);
            oam_add(x - 12, y - 24, GR_ST_HOWL, 6 | 8);
        }
    }
    oam_end();
}
static void raster(void) {
    uint8_t c0 = comm_on ? 32 : 0, v = comm_on && gr_vis > 24 ? 24 : gr_vis, sh = 0;
    if (shake_t) { shake_t--; sh = shake_t & 2; }
    if (comm_on) {
        n_lcdc = LCDC_COMM; n_scx = 0; n_scy = 16;
        if (v >= 8) {
            n_s1_line = 32; n_s1_lcdc = LCDC_COMM; n_s1_scx = (uint8_t)(-sway); n_s1_scy = (uint8_t)(HEAD_ROW * 8 + (v == 24 ? 16 : 56 - v) - c0);      // under the crew box: show the eyes, not the jaw
            n_s2_line = 32 + v; n_s2_lcdc = LCDC_PLAY; n_s2_scx = 0; n_s2_scy = gr_fscy + sh;
        } else { n_s1_line = 32; n_s1_lcdc = LCDC_PLAY; n_s1_scx = 0; n_s1_scy = gr_fscy + sh; n_s2_line = 0; }
    } else if (v >= 8) {
        n_lcdc = LCDC_COMM; n_scx = (uint8_t)(-sway); n_scy = (uint8_t)(HEAD_ROW * 8 + 56 - v);
        n_s1_line = v; n_s1_lcdc = LCDC_PLAY; n_s1_scx = 0; n_s1_scy = gr_fscy + sh; n_s2_line = 0;
    } else { n_lcdc = LCDC_PLAY; n_scx = 0; n_scy = gr_fscy + sh; n_s1_line = 0; n_s2_line = 0; }
}

void ground_draw(void) BANKED { raster(); draw_all(); }
