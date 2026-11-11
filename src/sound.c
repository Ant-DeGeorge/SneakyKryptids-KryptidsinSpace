// Sound engine (ROM0). Music: hUGEDriver (all four channels, songs from hUGETracker, see
// tools/music). Sound effects run on ch1 (square + sweep) and ch4 (noise): while one plays, the
// driver is told to leave that channel alone, and it gets the channel back when the effect ends.
#include <gb/gb.h>
#include <gb/hardware.h>
#include "sound.h"
#include "songs.h"
#include "hUGE/hUGEDriver.h"

extern const hUGESong_t * const music_songs[];
extern const uint8_t music_banks[];

static uint8_t cur_song = 0xFF, cur_bank, playing;
static uint8_t sfx_ch_busy;                 // bit 0: ch1 taken by an effect, bit 3: ch4

typedef struct { uint8_t dur, ch, r0, r1, r2, r3, r4; } Step;
#define LO(f) ((uint8_t)((f) & 0xFF))
#define HI(f) ((uint8_t)(0x80 | ((f) >> 8)))
#define HIL(f) ((uint8_t)(0xC0 | ((f) >> 8)))
#define END {0,0,0,0,0,0,0}

static const Step s_shot[]    = {{3,1,0x1A,0x80|0x3A,0x51,LO(1900),HIL(1900)}, END};
static const Step s_shot2[]   = {{4,1,0x24,0x40|0x3C,0x51,LO(1750),HIL(1750)}, END};
static const Step s_tick[]    = {{2,4,0,0x3E,0x61,0x21,0xC0}, END};
static const Step s_expl_s[]  = {{4,4,0,0x00,0xC1,0x55,0x80}, {10,4,0,0x00,0x92,0x67,0x80}, END};
static const Step s_expl_l[]  = {{6,4,0,0x00,0xF2,0x56,0x80}, {20,4,0,0x00,0xC4,0x77,0x80}, {24,4,0,0x00,0x84,0x7F,0x80}, END};
static const Step s_hurt[]    = {{6,1,0x7F,0x80,0xF2,LO(1700),HI(1700)}, {4,4,0,0,0xD2,0x44,0x80}, {16,1,0x7E,0x80,0xC3,LO(1400),HI(1400)}, END};
static const Step s_pickup[]  = {{4,1,0,0x80,0xC1,LO(1798),HI(1798)}, {4,1,0,0x80,0xC1,LO(1899),HI(1899)}, {10,1,0,0x80,0xC2,LO(1985),HI(1985)}, END};
static const Step s_power[]   = {{3,1,0,0x80,0xD1,LO(1650),HI(1650)}, {3,1,0,0x80,0xD1,LO(1750),HI(1750)}, {3,1,0,0x80,0xD1,LO(1850),HI(1850)},
                                 {3,1,0,0x80,0xD1,LO(1923),HI(1923)}, {3,1,0,0x80,0xD1,LO(1985),HI(1985)}, {14,1,0,0x80,0xD3,LO(2016),HI(2016)}, END};
static const Step s_bomb[]    = {{8,1,0x77,0x80,0xF7,LO(1200),HI(1200)}, {40,4,0,0x00,0xF6,0x71,0x80}, {30,4,0,0x00,0xA5,0x7F,0x80}, END};
static const Step s_alarm[]   = {{10,1,0,0x80,0xD1,LO(1923),HI(1923)}, {10,1,0,0x80,0xD1,LO(1849),HI(1849)},
                                 {10,1,0,0x80,0xD1,LO(1923),HI(1923)}, {10,1,0,0x80,0xD1,LO(1849),HI(1849)}, END};
static const Step s_start[]   = {{5,1,0,0x80,0xC1,LO(1650),HI(1650)}, {5,1,0,0x80,0xC1,LO(1798),HI(1798)},
                                 {5,1,0,0x80,0xC1,LO(1849),HI(1849)}, {16,1,0,0x80,0xC3,LO(1923),HI(1923)}, END};
static const Step s_blip[]    = {{2,1,0,0x40|0x3E,0x41,LO(1960),HIL(1960)}, END};
static const Step s_bosshit[] = {{2,4,0,0x3D,0x81,0x31,0xC0}, END};
static const Step s_oneup[]   = {{4,1,0,0x80,0xC1,LO(1849),HI(1849)}, {4,1,0,0x80,0xC1,LO(1923),HI(1923)}, {4,1,0,0x80,0xC1,LO(1985),HI(1985)},
                                 {4,1,0,0x80,0xC1,LO(1923),HI(1923)}, {4,1,0,0x80,0xC1,LO(1985),HI(1985)}, {16,1,0,0x80,0xC3,LO(2016),HI(2016)}, END};
static const Step s_select[]  = {{4,1,0,0x80,0x91,LO(1800),HI(1800)}, END};
static const Step s_roar[]    = {{40,4,0,0x00,0xF7,0x6D,0x80}, {30,1,0x1F,0x00,0xF3,LO(900),HI(900)}, END};
static const Step s_whoosh[]  = {{20,4,0,0x00,0x73,0x34,0x80}, {20,4,0,0x00,0x53,0x23,0x80}, END};
static const Step s_spit[]    = {{6,4,0,0x00,0xA2,0x45,0x80}, {8,1,0x2E,0x80,0x92,LO(1500),HI(1500)}, END};
static const Step s_thunder[] = {{4,4,0,0x00,0xF1,0x37,0x80}, {50,4,0,0x00,0xD7,0x6F,0x80}, END};

static const Step * const sfx_table[] = {
    s_shot, s_shot2, s_tick, s_expl_s, s_expl_l, s_hurt, s_pickup, s_power, s_bomb, s_alarm,
    s_start, s_blip, s_bosshit, s_oneup, s_select, s_roar, s_whoosh, s_spit, s_thunder
};
// priority: 0 = can be cut by anything, 1 = normal, 2 = important
static const uint8_t sfx_prio_tab[] = {0,0,0,1,2,2,1,2,2,2, 2,0,0,2,1,2,1,1,1};

static const Step *sfx_pos;
static uint8_t sfx_timer, sfx_prio;

static void sfx_take(uint8_t ch) {
    uint8_t bit = ch == 1 ? 0x01 : 0x08;
    if (sfx_ch_busy & bit) return;
    sfx_ch_busy |= bit;
    if (playing) hUGE_mute_channel(ch == 1 ? HT_CH1 : HT_CH4, HT_CH_MUTE);
}
static void sfx_release(void) {
    if (sfx_ch_busy & 0x01) { NR12_REG = 0; NR14_REG = 0x80; if (playing) hUGE_mute_channel(HT_CH1, HT_CH_PLAY); }
    if (sfx_ch_busy & 0x08) { NR42_REG = 0; NR44_REG = 0x80; if (playing) hUGE_mute_channel(HT_CH4, HT_CH_PLAY); }
    sfx_ch_busy = 0;
}
static void write_step(const Step *s) {
    sfx_take(s->ch);
    if (s->ch == 1) { NR10_REG = s->r0; NR11_REG = s->r1; NR12_REG = s->r2; NR13_REG = s->r3; NR14_REG = s->r4; }
    else { NR41_REG = s->r1; NR42_REG = s->r2; NR43_REG = s->r3; NR44_REG = s->r4; }
}

void sfx_play(uint8_t id) {
    uint8_t prio = sfx_prio_tab[id];
    if (sfx_pos && prio < sfx_prio) return;
    sfx_pos = sfx_table[id]; sfx_prio = prio; sfx_timer = 0;
}

void sound_init(void) {
    NR52_REG = 0x80; NR51_REG = 0xFF; NR50_REG = 0x77;
    playing = 0; sfx_pos = 0; sfx_ch_busy = 0;
}

void music_play(uint8_t song) {
    uint8_t b;
    if (song == cur_song && playing) return;
    __critical {
        playing = 0;
        cur_song = song; cur_bank = music_banks[song];
        b = _current_bank; SWITCH_ROM(cur_bank);
        hUGE_init(music_songs[song]);
        SWITCH_ROM(b);
        NR50_REG = 0x77; NR51_REG = 0xFF;                  // songs may change volume and panning
        if (sfx_ch_busy & 0x01) hUGE_mute_channel(HT_CH1, HT_CH_MUTE);
        if (sfx_ch_busy & 0x08) hUGE_mute_channel(HT_CH4, HT_CH_MUTE);
        playing = 1;
    }
}

void music_stop(void) {
    __critical {
        playing = 0; cur_song = 0xFF;
        if (!(sfx_ch_busy & 0x01)) { NR12_REG = 0; NR14_REG = 0x80; }
        NR22_REG = 0; NR24_REG = 0x80;
        NR30_REG = 0;
        if (!(sfx_ch_busy & 0x08)) { NR42_REG = 0; NR44_REG = 0x80; }
        NR30_REG = 0x80;
    }
}

uint8_t music_playing(void) { return playing; }

// called from the VBlank interrupt: song data is banked, so switch in and restore
void sound_update(void) {
    if (playing) {
        uint8_t b = _current_bank;
        SWITCH_ROM(cur_bank);
        hUGE_dosound();
        SWITCH_ROM(b);
    }
    if (sfx_pos) {
        if (sfx_timer) sfx_timer--;
        if (!sfx_timer) {
            if (sfx_pos->dur == 0) { sfx_pos = 0; sfx_release(); }
            else { write_step(sfx_pos); sfx_timer = sfx_pos->dur; sfx_pos++; }
        }
    }
}
