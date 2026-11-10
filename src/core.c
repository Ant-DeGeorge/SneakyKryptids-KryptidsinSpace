// ROM0: interrupt handlers, bank-safe loaders, OAM builder, text, input.
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "core.h"
#include "assets.h"
#include "sound.h"
#include "objs.h"
#include "game.h"

// The studio card (src/studio_card) owns the STAT vector at 0x48; our split
// handler runs through its hook instead of GBDK's add_LCD dispatcher.
extern void (* volatile sc_stat_hook)(void);

volatile uint8_t n_lcdc = LCDC_SCREEN, n_scx, n_scy;
volatile uint8_t n_s1_line, n_s1_lcdc, n_s1_scx, n_s1_scy;
volatile uint8_t n_s2_line, n_s2_lcdc, n_s2_scx, n_s2_scy;
static uint8_t c_s1_lcdc, c_s1_scx, c_s1_scy, c_s2_line, c_s2_lcdc, c_s2_scx, c_s2_scy, c_stage;
volatile uint8_t frame_count;

static void vbl_isr(void) {
    LCDC_REG = n_lcdc; SCX_REG = n_scx; SCY_REG = n_scy;
    c_s1_lcdc = n_s1_lcdc; c_s1_scx = n_s1_scx; c_s1_scy = n_s1_scy;
    c_s2_line = n_s2_line; c_s2_lcdc = n_s2_lcdc; c_s2_scx = n_s2_scx; c_s2_scy = n_s2_scy;
    if (n_s1_line) { LYC_REG = n_s1_line - 1; c_stage = 1; }
    else { LYC_REG = 200; c_stage = 0; }
    frame_count++;
    sound_update();
}

static void lcd_isr(void) {
    while (STAT_REG & 3) ;            // wait for HBlank of the line before the split
    if (c_stage == 1) {
        LCDC_REG = c_s1_lcdc; SCX_REG = c_s1_scx; SCY_REG = c_s1_scy;
        if (c_s2_line) { LYC_REG = c_s2_line - 1; c_stage = 2; } else { LYC_REG = 200; c_stage = 0; }
    } else if (c_stage == 2) {
        LCDC_REG = c_s2_lcdc; SCX_REG = c_s2_scx; SCY_REG = c_s2_scy;
        LYC_REG = 200; c_stage = 0;
    }
}

void isr_install(void) {
    CRITICAL {
        add_VBL(vbl_isr);
        sc_stat_hook = lcd_isr;
        STAT_REG = STATF_LYC;
        LYC_REG = 200;
    }
    set_interrupts(VBL_IFLAG | LCD_IFLAG);
}

void splits_off(void) { n_s1_line = 0; n_s2_line = 0; }

// ------------------------------------------------------------------ loaders
void ld_bkg_data(uint8_t bank, uint8_t vbk, uint8_t first, uint8_t n, const uint8_t *data) {
    uint8_t b = _current_bank; SWITCH_ROM(bank);
    VBK_REG = vbk; set_bkg_data(first, n, data); VBK_REG = 0;
    SWITCH_ROM(b);
}
void ld_spr_data(uint8_t bank, uint8_t first, uint8_t n, const uint8_t *data) {
    uint8_t b = _current_bank; SWITCH_ROM(bank); set_sprite_data(first, n, data); SWITCH_ROM(b);
}
// Palette RAM is locked during mode 3 on real hardware, so write one colour
// at a time inside HBlank/VBlank (interrupts held off for the two bytes).
static void pal_write(uint8_t obj, uint8_t first, uint8_t n, const uint16_t *pal) {
    uint8_t i, c = n << 2;
    if (obj) OCPS_REG = 0x80 | (first << 3); else BCPS_REG = 0x80 | (first << 3);
    for (i = 0; i < c; i++) {
        uint16_t v = pal[i];
        __critical {
            while (STAT_REG & 2) ;          // wait for mode 0 or 1
            if (obj) { OCPD_REG = (uint8_t)v; OCPD_REG = (uint8_t)(v >> 8); }
            else { BCPD_REG = (uint8_t)v; BCPD_REG = (uint8_t)(v >> 8); }
        }
    }
}
void ld_bkg_pal(uint8_t bank, uint8_t first, uint8_t n, const uint16_t *pal) {
    uint8_t b = _current_bank; SWITCH_ROM(bank); pal_write(0, first, n, pal); SWITCH_ROM(b);
}
void ld_spr_pal(uint8_t bank, uint8_t first, uint8_t n, const uint16_t *pal) {
    uint8_t b = _current_bank; SWITCH_ROM(bank); pal_write(1, first, n, pal); SWITCH_ROM(b);
}
void ld_copy(uint8_t bank, void *dst, const void *src, uint16_t n) {
    uint8_t b = _current_bank; SWITCH_ROM(bank); memcpy(dst, src, n); SWITCH_ROM(b);
}
uint8_t ld_byte(uint8_t bank, const uint8_t *p) {
    uint8_t v, b = _current_bank; SWITCH_ROM(bank); v = *p; SWITCH_ROM(b); return v;
}

static uint8_t rowbuf[40];
void ld_image(uint8_t bank, uint8_t x, uint8_t y, uint8_t w, uint8_t h,
              const uint8_t *map, const uint8_t *attr, uint8_t tile_off, uint8_t attr_or, uint8_t win) {
    uint8_t r, i, b = _current_bank;
    for (r = 0; r < h; r++) {
        SWITCH_ROM(bank);
        for (i = 0; i < w; i++) { rowbuf[i] = map[r * w + i] + tile_off; rowbuf[20 + i] = attr[r * w + i] | attr_or; }
        SWITCH_ROM(b);
        if (win) {
            set_win_tiles(x, y + r, w, 1, rowbuf);
            VBK_REG = 1; set_win_tiles(x, y + r, w, 1, rowbuf + 20); VBK_REG = 0;
        } else {
            set_bkg_tiles(x, y + r, w, 1, rowbuf);
            VBK_REG = 1; set_bkg_tiles(x, y + r, w, 1, rowbuf + 20); VBK_REG = 0;
        }
    }
}

void ld_level_row(uint16_t row, uint8_t maprow) {
    uint8_t b = _current_bank, k = 0;
    while (row >= ROWS_PER_BANK) { row -= ROWS_PER_BANK; k++; }
    SWITCH_ROM(SD->rbank[k]); memcpy(rowbuf, SD->rptr[k] + row * 40, 40);
    SWITCH_ROM(b);
    // explicit 9800 address: LCDC.3 flips mid-frame for the comm box split
    set_tiles(0, maprow, 20, 1, (uint8_t *)0x9800, rowbuf);
    VBK_REG = 1; set_tiles(0, maprow, 20, 1, (uint8_t *)0x9800, rowbuf + 20); VBK_REG = 0;
    // off-screen columns 20-31 stay blank space so the Kraken's sway never shows garbage
    memset(rowbuf, N_COMMON, 12);
    set_tiles(20, maprow, 12, 1, (uint8_t *)0x9800, rowbuf);
    memset(rowbuf, 0, 12);
    VBK_REG = 1; set_tiles(20, maprow, 12, 1, (uint8_t *)0x9800, rowbuf); VBK_REG = 0;
}

// ------------------------------------------------------------------ text
uint8_t txt_tile(char c) {
    uint8_t t, b = _current_bank;
    SWITCH_ROM(BANK_A2); t = ascii_map[(uint8_t)c & 0x7F]; SWITCH_ROM(b);
    return t;
}
// the score as 7 digit tiles; constant compares and 16-bit math for the low digits keep it cheap
// (the old 32-bit table loop cost ~20 scanlines in a busy frame)
void score_tiles(uint8_t *o) {
    uint32_t s = score;
    uint16_t v;
    uint8_t d, z = txt_tile('0');
    d = z; while (s >= 1000000UL) { s -= 1000000UL; d++; } o[0] = d;
    d = z; while (s >= 100000UL) { s -= 100000UL; d++; } o[1] = d;
    d = z; while (s >= 10000UL) { s -= 10000UL; d++; } o[2] = d;
    v = (uint16_t)s;
    d = z; while (v >= 1000) { v -= 1000; d++; } o[3] = d;
    d = z; while (v >= 100) { v -= 100; d++; } o[4] = d;
    d = z; while (v >= 10) { v -= 10; d++; } o[5] = d;
    o[6] = z + (uint8_t)v;
}
void txt_put(uint8_t x, uint8_t y, const char *s, uint8_t win, uint8_t attr) {
    uint8_t n = 0, t;
    while (s[n] && n < 20) {
        t = txt_tile(s[n]);
        if (win) { set_win_tile_xy(x + n, y, t); VBK_REG = 1; set_win_tile_xy(x + n, y, attr); VBK_REG = 0; }
        else { set_bkg_tile_xy(x + n, y, t); VBK_REG = 1; set_bkg_tile_xy(x + n, y, attr); VBK_REG = 0; }
        n++;
    }
}
void txt_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t tile, uint8_t attr, uint8_t win) {
    if (win) { fill_win_rect(x, y, w, h, tile); VBK_REG = 1; fill_win_rect(x, y, w, h, attr); VBK_REG = 0; }
    else { fill_bkg_rect(x, y, w, h, tile); VBK_REG = 1; fill_bkg_rect(x, y, w, h, attr); VBK_REG = 0; }
}

// ------------------------------------------------------------------ OAM
uint8_t oam_n;
static uint8_t oam_rot;
static uint8_t obuf[36 * 4], *ow;            // staged entries: y, x, tile, prop (OAM layout)
// Sprites per 16-line band, by the band their top edge is in (fixed slots 0-3 are added by the
// caller). A line in band B is covered only by sprites starting in band B or B-1, so
// cnt[B-1] + cnt[B] bounds every line in band B. oam_add_fx uses it to draw an optional effect
// (muzzle flash, hit spark, ring) only where no scanline would go past the hardware's 10.
uint8_t oam_band[16];
void oam_begin(void) { oam_n = 0; ow = obuf; memset(oam_band, 0, sizeof oam_band); }
void oam_add(uint8_t x, uint8_t y, uint8_t tile, uint8_t prop) {
    uint8_t *w;
    if (oam_n >= 36) return;
    oam_band[y >> 4]++;
    w = ow; *w++ = y; *w++ = x; *w++ = tile; *w++ = prop; ow = w;
    oam_n++;
}
void oam_add_fx(uint8_t x, uint8_t y, uint8_t tile, uint8_t prop) {
    uint8_t b = y >> 4, c = oam_band[b];
    if (b && (uint8_t)(oam_band[b - 1] + c) > 9) return;
    if (b < 15 && (uint8_t)(c + oam_band[b + 1]) > 9) return;
    oam_add(x, y, tile, prop);
}
// Same result as before (entry i lands in rotating slot (rot + i) % 36, the rest hidden),
// done with two block copies instead of per-field stores.
void oam_end(void) {
    uint8_t i, s, first;
    uint8_t *p;
    if (oam_n) {
        oam_rot += 7;
        if (oam_rot >= 36) oam_rot -= 36;
    }
    s = oam_rot;
    p = &shadow_OAM[4].y;
    for (i = 36; i; i--) { *p = 0; p += 4; }
    first = 36 - s;
    if (oam_n <= first) memcpy(&shadow_OAM[4 + s], obuf, (uint16_t)oam_n << 2);
    else {
        memcpy(&shadow_OAM[4 + s], obuf, (uint16_t)first << 2);
        memcpy(&shadow_OAM[4], obuf + ((uint16_t)first << 2), (uint16_t)(oam_n - first) << 2);
    }
}
void oam_fixed(uint8_t slot, uint8_t x, uint8_t y, uint8_t tile, uint8_t prop) {
    OAM_item_t *o = &shadow_OAM[slot];
    o->y = y; o->x = x; o->tile = tile; o->prop = prop;
}
void oam_clear(void) { uint8_t i; for (i = 0; i < 40; i++) shadow_OAM[i].y = 0; oam_n = 0; ow = obuf; }

// ------------------------------------------------------------------ input / misc
uint8_t joy, joy_prev, joy_new;
void input_poll(void) { joy_prev = joy; joy = joypad(); joy_new = joy & ~joy_prev; }

uint8_t rnd_seed = 0x5A;
uint8_t rnd(void) {
    // 8-bit xorshift mixed with the frame counter
    uint8_t x = rnd_seed;
    x ^= x << 3; x ^= x >> 5; x ^= x << 1;
    if (!x) x = 0x5A;
    rnd_seed = x;
    return x ^ frame_count;
}
void wait_frames(uint8_t n) { while (n--) wait_vbl_done(); }
// Also drops the BG-map-select bit: a stage can end with the 9C00 map showing at the top of the
// frame (comm box, gun-cam canopy), and GBDK's set_bkg_* writers follow that bit.
void screen_off(void) { wait_vbl_done(); DISPLAY_OFF; LCDC_REG &= ~LCDCF_BG9C00; n_lcdc = LCDC_REG; }
void screen_on(uint8_t lcdc) { n_lcdc = lcdc; LCDC_REG = lcdc; }
void clear_bkg(uint8_t tile, uint8_t attr) {
    fill_bkg_rect(0, 0, 32, 32, tile);
    VBK_REG = 1; fill_bkg_rect(0, 0, 32, 32, attr); VBK_REG = 0;
}
