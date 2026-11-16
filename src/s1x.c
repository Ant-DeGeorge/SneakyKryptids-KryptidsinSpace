// Extra Rimemaw attacks (mission 1 boss). Autobanked: bank 9 (s1.c) is full.
//   HAIL: ice shakes loose above the fight. The rocks hang and blink, then drop straight down
//         their columns, the rule the mission teaches on the way in. One lane is always left open.
//   RAIN: rows of ink fall the full width of the screen with a two-lane gap; the gap moves each row.
#pragma bank 255
#include <gb/gb.h>
#include <stdint.h>
#include "core.h"
#include "sound.h"
#include "game.h"
#include "objs.h"
#include "s1x.h"

static uint8_t gap;

uint8_t s1x_attack(uint8_t a, uint8_t t) BANKED {
    uint8_t k, n;
    int8_t e;
    if (a == S1X_HAIL) {
        if (t == 1) { sfx_play(SFX_ROAR); k_shake = 16; gap = rnd() % 5; }
        if (t == 24) {
            n = HUNTER ? 0xFF : (uint8_t)((gap + 2 + (rnd() & 1)) % 5);          // Cadet and Pilot get a second open lane
            for (k = 0; k < 5; k++) {
                if (k == gap || k == n) continue;
                e = e_new(E_ICE, 16 + k * 30 + XO - 8, 8);
                if (e >= 0) e_vx[e] = 0;
            }
        }
        return t >= 120;
    }
    // RAIN
    if (t == 1) { sfx_play(SFX_SPIT); gap = 1 + rnd() % 4; }
    if (t == 24 || t == 64 || (HUNTER && t == 104)) {
        for (k = 0; k < 7; k++) {
            if (k == gap || k == gap + 1) continue;
            eb_fire(XO + 14 + k * 22, YO + 58, 0, HUNTER ? 18 : 12, 1);
        }
        sfx_play(SFX_SPIT);
        gap = gap >= 3 ? gap - 2 : gap + 2;
    }
    return t >= (HUNTER ? 150 : 120);
}
