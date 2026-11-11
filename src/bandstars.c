// Boss-band stars. In the scrolling stages the boss is part of the background map, and the top
// 64 lines are a raster band with their own SCX/SCY so the boss can sway, bob and retreat. Any
// star in those map rows slid along with the boss. When the boss arrives (the scroll has
// stopped by then), the stars in the band's map rows are cleared to empty sky, so only the boss
// moves. Drawing them as sprites instead was tried: it cost 10-20% of the frames in boss fights
// that already run close to the limit. Autobanked; runs once per boss.
#pragma bank 255
#include <gb/gb.h>
#include <stdint.h>
#include "core.h"

#define ROWS 9                   // map rows from one above the band to its last row

void bstar_take(uint8_t scy) BANKED {
    uint8_t r, c, mr, i, k, bt, ba, nc = 0;
    uint8_t rt[32], ra[32];
    uint8_t cnt_t[8], cnt_a[8], cnt_n[8];
    // the most common background (bank 0) cell in these rows is the empty sky
    for (r = 0; r < ROWS; r++) {
        mr = (((uint8_t)(scy - 8) >> 3) + r) & 31;
        get_tiles(0, mr, 20, 1, (uint8_t *)0x9800, rt);
        VBK_REG = 1; get_tiles(0, mr, 20, 1, (uint8_t *)0x9800, ra); VBK_REG = 0;
        for (c = 0; c < 20; c++) {
            if (ra[c] & 0x08) continue;                       // boss tiles live in VRAM bank 1
            for (i = 0; i < nc; i++) if (cnt_t[i] == rt[c] && cnt_a[i] == ra[c]) { cnt_n[i]++; break; }
            if (i == nc && nc < 8) { cnt_t[nc] = rt[c]; cnt_a[nc] = ra[c]; cnt_n[nc++] = 1; }
        }
    }
    if (!nc) return;
    for (i = 1, k = 0; i < nc; i++) if (cnt_n[i] > cnt_n[k]) k = i;
    bt = cnt_t[k]; ba = cnt_a[k];
    for (r = 0; r < ROWS; r++) {
        mr = (((uint8_t)(scy - 8) >> 3) + r) & 31;
        get_tiles(0, mr, 32, 1, (uint8_t *)0x9800, rt);
        VBK_REG = 1; get_tiles(0, mr, 32, 1, (uint8_t *)0x9800, ra); VBK_REG = 0;
        for (c = 0; c < 32; c++) if (!(ra[c] & 0x08)) { rt[c] = bt; ra[c] = ba; }
        set_tiles(0, mr, 32, 1, (uint8_t *)0x9800, rt);
        VBK_REG = 1; set_tiles(0, mr, 32, 1, (uint8_t *)0x9800, ra); VBK_REG = 0;
    }
}
