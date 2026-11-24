// Japanese build only (copied into build/ja_src by tools/ja/gen_ja.py).
// Kana are not in the normal font, so each one is loaded into a VRAM tile when it is first drawn.
// Capital letters are cached the same way (most screens show few or none), so the free tiles are
//   bank 0: font tiles of every capital and of punctuation the Japanese build never shows,
//   bank 1: 24 tiles no mode uses (224-255 minus a few). Bank-1 tiles need the attribute's bank bit,
//           so only callers that write attributes (txt_put, the comm typewriter) may get one (ja_allow1).
// When every slot is taken, the tile maps are scanned and every slot not on screen is freed (the window
// map only once this screen has drawn window text: until then it still holds the last screen's HUD).
// ja_reset() runs after each font load (the load overwrites bank-0 slots). After a reset every slot is
// "unknown" until a sweep finds it unused on both maps, so a tile some screen put there is never taken.
// ja_boss1(): the mission 2 boss fills bank-1 tiles 128-255, so the bank-1 slots move to the stage's
// unused scenery tiles (bank 1, n_scen1..111) until the next reset.
#pragma bank 255
#include <gb/gb.h>
#include <stdint.h>
#include <string.h>
#include "ja_font.inc"    // ja_font[], JA_NGLYPH, JA_LATIN0, ja_free0[], ja_free1x[], ja_map0[], JA_NFREE0/1, JA_QMARK

#define NS (JA_NFREE0 + JA_NFREE1)
#define RING 8
#define S_UNKNOWN 0xFF                  // slot_g: not yet seen free since the reset
#define S_BLOCKED 0xFE                  // slot_g: holds someone else's tile until the next reset
#define SCEN1_END 112                   // stage bank-1 scenery ends below the comm portrait (COMM_TILE)

uint8_t ja_allow1, ja_vb, ja_on;
uint8_t ja_win;                         // window text drawn since the reset (else its map is a stale screen)
uint8_t ja_ovf, ja_gcs;                 // test counters: glyphs that did not fit / map sweeps
uint8_t slot_g[NS];                     // glyph in the slot + 1 (0 = free); public for the tests
uint8_t slot_t1[JA_NFREE1];             // VRAM bank-1 tile of each bank-1 slot
static uint8_t g_s0[JA_NGLYPH], g_s1[JA_NGLYPH];   // glyph -> its bank 0 / bank 1 slot + 1
static uint8_t rev1[256];               // bank-1 tile -> slot (0xFF none)
static uint8_t ring[RING], ring_i;      // newest slots: never freed (may not be on the map yet)
static uint8_t used[NS];
static uint8_t row[32], rowa[32];

static void set_t1(uint8_t k, uint8_t t) {
    rev1[slot_t1[k]] = 0xFF;
    slot_t1[k] = t; rev1[t] = JA_NFREE0 + k;
}

void ja_reset(void) BANKED {
    uint8_t k;
    ja_on = 1; ja_win = 0;
    memset(slot_g, S_UNKNOWN, NS); memset(g_s0, 0, JA_NGLYPH); memset(g_s1, 0, JA_NGLYPH);
    memset(ring, 0xFF, RING); memset(rev1, 0xFF, 256);
    for (k = 0; k < JA_NFREE1; k++) { slot_t1[k] = 224 + ja_free1x[k]; rev1[slot_t1[k]] = JA_NFREE0 + k; }
}

static uint8_t slot_tile(uint8_t s) { return s < JA_NFREE0 ? ja_free0[s] : slot_t1[s - JA_NFREE0]; }

static void free_slot(uint8_t s) {
    uint8_t g = slot_g[s];
    if (g == S_BLOCKED) return;
    slot_g[s] = 0;
    if (!g || g == S_UNKNOWN) return;
    g--;
    if (g_s0[g] == s + 1) g_s0[g] = 0;
    if (g_s1[g] == s + 1) g_s1[g] = 0;
}

void ja_boss1(uint8_t first, uint8_t n, uint8_t scen1) BANKED {
    uint8_t k, s, g;
    if ((uint16_t)first + n <= 224) return;        // the cache's bank-1 tiles are not touched
    for (k = 0; k < JA_NFREE1; k++) {
        s = JA_NFREE0 + k;
        g = slot_g[s];
        if (g && g < S_BLOCKED) { g--; if (g_s1[g] == s + 1) g_s1[g] = 0; }
        if (scen1 + k < SCEN1_END) { set_t1(k, scen1 + k); slot_g[s] = S_UNKNOWN; }
        else { rev1[slot_t1[k]] = 0xFF; slot_g[s] = S_BLOCKED; }
    }
}

static void scan(uint8_t *map, uint8_t rows) {
    uint8_t y, x, t, s;
    for (y = 0; y < rows; y++) {
        get_tiles(0, y, 32, 1, map, row);
        VBK_REG = 1; get_tiles(0, y, 32, 1, map, rowa); VBK_REG = 0;
        for (x = 0; x < 32; x++) {
            t = row[x];
            s = (rowa[x] & 0x08) ? rev1[t] : (t < 64 ? ja_map0[t] : 0xFF);
            if (s != 0xFF) used[s] = 1;
        }
    }
}

static void sweep(void) {
    uint8_t s;
    ja_gcs++;
    memset(used, 0, NS);
    scan((uint8_t *)0x9800, 32);
    if (ja_win) scan((uint8_t *)0x9C00, 18);
    for (s = 0; s < RING; s++) if (ring[s] != 0xFF) used[ring[s]] = 1;
    for (s = 0; s < NS; s++) if (!used[s]) free_slot(s);
}

static uint8_t find(uint8_t allow1) {
    uint8_t s;
    if (allow1) for (s = JA_NFREE0; s < NS; s++) if (!slot_g[s]) return s;   // bank 1 first: keeps bank 0 for HUD text
    for (s = 0; s < JA_NFREE0; s++) if (!slot_g[s]) return s;
    return 0xFF;
}

uint8_t ja_tile(uint8_t c) BANKED {
    uint8_t g = c >= 0x80 ? c - 0x80 : c < 0x0A ? 127 + c : c < 0x20 ? 126 + c : JA_LATIN0 + c - 'A', s;
    uint8_t allow1 = ja_allow1;
    if (!ja_on) ja_reset();
    if (!allow1) ja_win = 1;                       // tile-only callers are the HUD rows (window)
    if (g >= JA_NGLYPH) { ja_ovf++; return JA_QMARK; }
    if (allow1 && g_s1[g]) { ja_vb = 1; return slot_tile(g_s1[g] - 1); }
    if (g_s0[g]) { ja_vb = 0; return slot_tile(g_s0[g] - 1); }
    s = find(allow1);
    if (s == 0xFF) { sweep(); s = find(allow1); }
    if (s == 0xFF) { ja_ovf++; ja_vb = 0; return JA_QMARK; }
    slot_g[s] = g + 1;
    ring[ring_i] = s; ring_i = (ring_i + 1) & (RING - 1);
    if (s < JA_NFREE0) {
        g_s0[g] = s + 1; ja_vb = 0;
        set_bkg_data(ja_free0[s], 1, ja_font + (uint16_t)g * 16);
    } else {
        g_s1[g] = s + 1; ja_vb = 1;
        VBK_REG = 1; set_bkg_data(slot_t1[s - JA_NFREE0], 1, ja_font + (uint16_t)g * 16); VBK_REG = 0;
    }
    return slot_tile(s);
}
