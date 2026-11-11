// ROM0 services shared by all banks: bank-safe loaders, raster splits, OAM, text.
#ifndef CORE_H
#define CORE_H
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

// ---- LCDC presets (BG tiles at 8800 signed, OBJ 8x16, window map 9C00)
#define LCDC_PLAY   (LCDCF_ON | LCDCF_WIN9C00 | LCDCF_WINON | LCDCF_BG8800 | LCDCF_BG9800 | LCDCF_OBJ16 | LCDCF_OBJON | LCDCF_BGON)
#define LCDC_COMM   (LCDC_PLAY | LCDCF_BG9C00)
#define LCDC_SCREEN (LCDCF_ON | LCDCF_WIN9C00 | LCDCF_WINOFF | LCDCF_BG8800 | LCDCF_BG9800 | LCDCF_OBJ16 | LCDCF_OBJON | LCDCF_BGON)

// ---- raster state: main loop writes n_*, VBL latches them for the next frame
extern volatile uint8_t n_lcdc, n_scx, n_scy;
extern volatile uint8_t n_s1_line, n_s1_lcdc, n_s1_scx, n_s1_scy;
extern volatile uint8_t n_s2_line, n_s2_lcdc, n_s2_scx, n_s2_scy;
extern volatile uint8_t frame_count;
void isr_install(void);
void splits_off(void);

// ---- bank-safe loaders (save/restore the caller's bank)
void ld_bkg_data(uint8_t bank, uint8_t vbk, uint8_t first, uint8_t n, const uint8_t *data);
void ld_spr_data(uint8_t bank, uint8_t first, uint8_t n, const uint8_t *data);
void ld_bkg_pal(uint8_t bank, uint8_t first, uint8_t n, const uint16_t *pal);
void ld_spr_pal(uint8_t bank, uint8_t first, uint8_t n, const uint16_t *pal);
void ld_copy(uint8_t bank, void *dst, const void *src, uint16_t n);
// draw a w x h image whose map indices are relative; adds tile_off, writes attrs (+ VRAM bank bit)
void ld_image(uint8_t bank, uint8_t x, uint8_t y, uint8_t w, uint8_t h,
              const uint8_t *map, const uint8_t *attr, uint8_t tile_off, uint8_t attr_or, uint8_t win);
void ld_level_row(uint16_t row, uint8_t maprow);
uint8_t ld_byte(uint8_t bank, const uint8_t *p);

// ---- text
void txt_put(uint8_t x, uint8_t y, const char *s, uint8_t win, uint8_t attr);
void txt_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t tile, uint8_t attr, uint8_t win);
uint8_t txt_tile(char c);
void score_tiles(uint8_t *o);             // 7 digit tiles of the score

// ---- OAM builder: slots 0-3 fixed (player), 4-39 rotated each frame for flicker sharing
extern uint8_t oam_n;
void oam_begin(void);
void oam_add(uint8_t x, uint8_t y, uint8_t tile, uint8_t prop);   // x,y = OAM coords (screen+8, screen+16)
void oam_end(void);
extern uint8_t oam_band[16];                 // sprites per 16-line band (by top edge) this frame
void oam_add_fx(uint8_t x, uint8_t y, uint8_t tile, uint8_t prop);   // optional effect: skipped if a line would pass 10 sprites
void oam_fixed(uint8_t slot, uint8_t x, uint8_t y, uint8_t tile, uint8_t prop);
void oam_clear(void);

// ---- input
extern uint8_t joy, joy_prev, joy_new;
void input_poll(void);

// ---- misc
extern uint8_t rnd_seed;
uint8_t rnd(void);
void wait_frames(uint8_t n);
void screen_off(void);
void screen_on(uint8_t lcdc);
void clear_bkg(uint8_t tile, uint8_t attr);
void fade_from_black(void);
#endif
