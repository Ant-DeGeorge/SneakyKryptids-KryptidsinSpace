// Sneaky Kryptid sound test: every song and sound effect, through the game's own sound engine.
// UP/DOWN pick a song, A plays it, B stops. LEFT/RIGHT pick an effect, SELECT fires it over the music.
// A renderer can poke jb_req (a song number) to start a song without pressing anything.
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <gbdk/console.h>
#include <stdint.h>
#include "sound.h"

static const char * const names[] = {
    "TITLE  BOUNTY CALL", "STAGE 1 RIMEVEIL", "BOSS   KRAKEN", "BRIEFING", "WARNING", "CLEAR", "GAME OVER", "ENDING",
    "STAGE 2 STORMWALL", "BOSS   THUNDERBIRD", "STAGE 3 BARRENS", "BOSS   JERSEY DEVIL", "MAP    STAR CHART",
    "STAGE 4 FROSTBITE", "BOSS   WENDIGO", "STAGE 5 GUN CAM", "BOSS   FLATWOODS"};
static const char * const sfx_names[] = {"SHOT", "SHOT2", "TICK", "EXPL S", "EXPL L", "HURT", "PICKUP", "POWER", "BOMB",
    "ALARM", "START", "BLIP", "BOSSHIT", "ONEUP", "SELECT", "ROAR", "WHOOSH", "SPIT", "THUNDER"};
volatile uint8_t jb_req = 0xFF;          // poked by the renderer
volatile uint8_t jb_frames;
volatile uint8_t jb_nr51 = 0xFF;      // poked by the renderer: solo channels (0xFF = all)

static void vbl(void) { sound_update(); if (jb_nr51 != 0xFF) NR51_REG = jb_nr51; jb_frames++; }

static void pad(const char *s, uint8_t w) {
    while (*s && w) { putchar(*s++); w--; }
    while (w--) putchar(' ');
}
static void draw(uint8_t sel, uint8_t fx, uint8_t now) {
    uint8_t i;
    gotoxy(0, 0); printf("SOUND TEST         ");
    for (i = 0; i < MUS_COUNT; i++) {
        gotoxy(0, i + 1);
        putchar(i == sel ? '>' : ' '); putchar(i == now ? '*' : ' '); pad(names[i], 18);
    }
    gotoxy(0, 17); printf("SFX < "); pad(sfx_names[fx], 8); printf(" >");
}

void main(void) {
    uint8_t sel = 0, fx = 0, now = 0xFF, j, last = 0;
    static const uint16_t pal[] = {RGB(2, 2, 6), RGB(8, 10, 20), RGB(16, 24, 31), RGB(31, 31, 31)};
    cpu_fast();
    set_bkg_palette(0, 1, pal);
    sound_init();
    __critical { add_VBL(vbl); }
    set_interrupts(VBL_IFLAG);
    draw(sel, fx, now);
    while (1) {
        wait_vbl_done();
        if (jb_req != 0xFF) { now = sel = jb_req; jb_req = 0xFF; music_play(now); draw(sel, fx, now); }
        j = joypad();
        if (j != last) {
            uint8_t n = j & ~last;
            if (n & J_UP) sel = sel ? sel - 1 : MUS_COUNT - 1;
            if (n & J_DOWN) sel = sel + 1 < MUS_COUNT ? sel + 1 : 0;
            if (n & J_LEFT) fx = fx ? fx - 1 : SFX_THUNDER;
            if (n & J_RIGHT) fx = fx < SFX_THUNDER ? fx + 1 : 0;
            if (n & J_A) { now = sel; music_play(now); }
            if (n & J_B) { now = 0xFF; music_stop(); }
            if (n & J_SELECT) sfx_play(fx);
            if (n) draw(sel, fx, now);
            last = j;
        }
    }
}
