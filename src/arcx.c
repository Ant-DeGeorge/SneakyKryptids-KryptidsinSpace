// Arcade extras, autobanked.
// 1. Arcade-only weapons, in the weapon capsule's cycle after the usual three:
//    PRISM  a rainbow fan of glass shards that bounce off the walls at full power
//    HELIX  two orbs that wind around each other and go through enemies
//    NOVA   a slow unstable orb that goes through enemies, then bursts into a ring of stars
//    Their colours cycle (OBJ palette 2), so they read as nothing else on screen.
// 2. Each map is played twice without a reload: at the end of lap 1 the scroll speeds up and the
//    map washes out (the warp), lap 1's last rows hand over to lap 2's first rows, and lap 2 runs
//    in new colours with enemies that fire three-shot fans and faster shots, and the boss at its end.
#pragma bank 255
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include "core.h"
#include "sound.h"
#include "game.h"
#include "objs.h"
#include "stage_def.h"
#include "arcx.h"
#include "weapon_fx3.h"

static uint8_t ax_t[MAXPB];                                   // nova orb fuse, per shot slot
static uint8_t ax_pal = 0xFF, warp_on;
static char pop_buf[8];                                       // HUD pop text in RAM: the HUD reads it from bank 1
static void pop(const char *s) { uint8_t i; for (i = 0; i < 7; i++) pop_buf[i] = s[i]; pop_buf[7] = 0; pop_s = pop_buf; pop_t = 100; hud_dirty = 1; }

void arcx_load(void) BANKED {
    VBK_REG = 1; ld_spr_data(BANK(weapon_fx3), FX3_BASE, FX3_NT, fx3_tiles); VBK_REG = 0;
    ax_pal = 0xFF; warp_on = 0;
}

static uint8_t spawn(uint8_t x, uint8_t y, int8_t vx, int8_t vy, uint8_t type, uint8_t dmg) {
    uint8_t i;
    for (i = 0; i < MAXPB; i++) if (!pb_on[i]) {
        pb_on[i] = 1; pb_x[i] = x; pb_y[i] = y; pb_vx[i] = vx; pb_vy[i] = vy; pb_type[i] = type; pb_dmg[i] = dmg;
        pb_tgt[i] = 0xFF; ax_t[i] = 0;
        return i;
    }
    return 0xFF;
}

void arcx_fire(void) BANKED {
    uint8_t x = px + 4, y = py - 6, i;
    switch (wtype) {
    case 3:                                                   // PRISM: 3, 5, then 5 that bounce
        spawn(x, y, 0, -6, PB_PRISM, 2);
        spawn(x - 3, y + 2, -2, -5, PB_PRISM, 2); spawn(x + 3, y + 2, 2, -5, PB_PRISM, 2);
        if (wlevel) { spawn(x - 5, y + 4, -4, -4, PB_PRISM, 2); spawn(x + 5, y + 4, 4, -4, PB_PRISM, 2); }
        p_fire_cd = 8;
        sfx_play(SFX_SHOT2);
        break;
    case 4:                                                   // HELIX: one pair, harder pairs, then two pairs
        spawn(x, y, 0, -4, PB_HELIXL, wlevel ? 3 : 2); spawn(x, y, 0, -4, PB_HELIXR, wlevel ? 3 : 2);
        if (wlevel == 2) { spawn(x, y + 12, 0, -4, PB_HELIXR, 3); spawn(x, y + 12, 0, -4, PB_HELIXL, 3); }
        p_fire_cd = wlevel == 2 ? 10 : 7;
        sfx_play(SFX_SHOT);
        break;
    default:                                                  // NOVA: one orb at a time
        for (i = 0; i < MAXPB; i++) if (pb_on[i] && pb_type[i] == PB_NOVA) return;
        i = spawn(x, y, 0, -3, PB_NOVA, wlevel ? 6 : 5);
        if (i != 0xFF) ax_t[i] = wlevel == 2 ? 14 : 20;
        p_fire_cd = 10;
        sfx_play(SFX_SPIT);
        break;
    }
    mz_t = 4; mz_k = 0;
}

static const int8_t ring_vx[8] = {0, 3, 4, 3, 0, -3, -4, -3};
static const int8_t ring_vy[8] = {-4, -3, 0, 3, 4, 3, 0, -3};
void arcx_move(uint8_t i) BANKED {
    uint8_t t = pb_type[i], k, x, y, d;
    int8_t s;
    if (t == PB_PRISM) {                                      // full power: shards bounce off the walls (as long as they live)
        if (wlevel == 2 && (pb_x[i] < 12 || pb_x[i] > 164)) { pb_vx[i] = -pb_vx[i]; pb_x[i] = pb_x[i] < 12 ? 12 : 164; }
    } else if (t == PB_HELIXL || t == PB_HELIXR) {            // wind around the line of fire
        s = sin64[(uint8_t)(pb_y[i] << 1) & 63] >> 5;
        pb_vx[i] = t == PB_HELIXL ? s : -s;
    } else if (t == PB_NOVA && ax_t[i] && !--ax_t[i]) {       // fuse out: burst into a ring
        x = pb_x[i]; y = pb_y[i]; d = pb_dmg[i] == 6 ? 3 : 2;
        pb_on[i] = 0;
        for (k = 0; k < 8; k++) spawn(x, y, ring_vx[k], ring_vy[k], PB_SPARK, d);
        sfx_play(SFX_EXPL_S);
    }
}

// colour cycles: PRISM runs the rainbow, HELIX flips magenta/cyan, NOVA pulses gold and pink
static const uint16_t ax_pals[10][4] = {
    {0, RGB8(255, 255, 255), RGB8(248, 64, 64),  RGB8(120, 16, 40)},
    {0, RGB8(255, 255, 255), RGB8(248, 168, 32), RGB8(136, 56, 8)},
    {0, RGB8(255, 255, 255), RGB8(240, 240, 48), RGB8(112, 104, 8)},
    {0, RGB8(255, 255, 255), RGB8(64, 240, 96),  RGB8(8, 104, 40)},
    {0, RGB8(255, 255, 255), RGB8(64, 200, 248), RGB8(16, 64, 168)},
    {0, RGB8(255, 255, 255), RGB8(192, 96, 248), RGB8(72, 24, 152)},
    {0, RGB8(255, 255, 255), RGB8(240, 64, 240), RGB8(96, 16, 160)},
    {0, RGB8(255, 255, 255), RGB8(64, 240, 240), RGB8(16, 96, 160)},
    {0, RGB8(255, 255, 255), RGB8(248, 224, 64), RGB8(240, 64, 128)},
    {0, RGB8(255, 248, 224), RGB8(248, 120, 176), RGB8(176, 32, 96)},
};
void arcx_wpal(void) BANKED {
    uint8_t f = frame_count, k;
    k = wtype == 3 ? (uint8_t)((f >> 2) % 6) : wtype == 4 ? 6 + ((f >> 3) & 1) : 8 + ((f >> 2) & 1);
    if (k == ax_pal) return;
    ax_pal = k;
    set_sprite_palette(2, 1, ax_pals[k]);
    set_sprite_palette_entry(1, 1, ax_pals[k][2]);            // exhaust burns in the weapon's colour
}

// ------------------------------------------------------------------ laps
void arcx_tick(void) BANKED {
    uint8_t i;
    if (arc_lap || warp_on || pos < (SD->n_rows - 18) * 8) return;
    warp_on = 1;                                              // the map has run out: warp
    pal_tint = 4;
    for (i = 0; i < 8; i++) if (i != 4) set_pal_slot(i, cur_bp[i]);
    scr_n = 1;                                                // a pixel a frame
    pop("WARP!  ");
    sfx_play(SFX_WHOOSH);
}
void arcx_lap(void) BANKED {
    uint8_t i, bp[8];
    pos = 0;                                                  // lap 2 starts where lap 1's extra rows ended
    lap_base += SD->n_rows; lap8 = (uint8_t)(lap_base << 3);
    next_row -= SD->n_rows;
    scroll_end = (SD->n_rows - 18) * 8; row_end = SD->n_rows;
    arc_lap = 1; warp_on = 0;
    scr_n = 2;
    arc_fan = 1;                                              // lap 2: three-shot fans
    arc_bspd += 4;                                            // and faster shots
    pal_tint = arc_tint2;
    ld_copy(SD->dbank, bp, SD->initbp, 8);
    for (i = 0; i < 8; i++) if (i != 4) set_pal_slot(i, bp[i]);
    flash_t = 4;
    pop("LAP 2! ");
    add_score(1000);
    sfx_play(SFX_ALARM);
}
