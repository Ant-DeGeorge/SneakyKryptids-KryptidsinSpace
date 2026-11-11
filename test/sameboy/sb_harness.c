// Scripted SameBoy harness: accurate CGB emulation with an input script and screenshots.
// Script lines: "<frame> press|release <A|B|START|SELECT|LEFT|RIGHT|UP|DOWN>", "<frame> shot <name>",
// "<frame> poke <addr>=<val>", "<frame> wav <file.wav>" (start audio capture), "<frame> wavstop -", "<frame> end"
#include <Core/gb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t bitmap[256 * 224];
static volatile int vblank_hit;
static const char *boot_dir;
static int cc_mode;
// audio capture (48 kHz stereo 16-bit)
static int16_t *aud; static size_t aud_n, aud_cap; static int aud_on; static char aud_path[512];
static void sample_cb(GB_gameboy_t *gb, GB_sample_t *s) {
    if (!aud_on) return;
    if (aud_n + 2 > aud_cap) { aud_cap = aud_cap ? aud_cap * 2 : 1 << 20; aud = realloc(aud, aud_cap * 2); }
    aud[aud_n++] = s->left; aud[aud_n++] = s->right;
}
static void put32(FILE *f, uint32_t v) { fputc(v, f); fputc(v >> 8, f); fputc(v >> 16, f); fputc(v >> 24, f); }
static void put16(FILE *f, uint16_t v) { fputc(v, f); fputc(v >> 8, f); }
static void wav_write(void) {
    FILE *f = fopen(aud_path, "wb"); uint32_t bytes = aud_n * 2;
    fwrite("RIFF", 1, 4, f); put32(f, 36 + bytes); fwrite("WAVEfmt ", 1, 8, f);
    put32(f, 16); put16(f, 1); put16(f, 2); put32(f, 48000); put32(f, 48000 * 4); put16(f, 4); put16(f, 16);
    fwrite("data", 1, 4, f); put32(f, bytes); fwrite(aud, 2, aud_n, f); fclose(f);
    aud_on = 0; aud_n = 0;
}

static uint32_t rgb_encode(GB_gameboy_t *gb, uint8_t r, uint8_t g, uint8_t b) { return (r << 16) | (g << 8) | b; }
static void vblank(GB_gameboy_t *gb, GB_vblank_type_t type) { vblank_hit = 1; }
static void log_cb(GB_gameboy_t *gb, const char *s, GB_log_attributes_t a) { fprintf(stderr, "[sb] %s", s); }
// Minimal stand-in CGB boot ROM: enable CGB mode, set A=0x11, unmap at 0x00FE -> PC=0x0100.
static void load_boot(GB_gameboy_t *gb, GB_boot_rom_t type) {
    static uint8_t rom[0x900];
    memset(rom, 0, sizeof rom);
    rom[0] = 0x31; rom[1] = 0xFE; rom[2] = 0xFF;          // ld sp,$FFFE
    rom[3] = 0x3E; rom[4] = 0x80;                         // ld a,$80
    rom[5] = 0xE0; rom[6] = 0x4C;                         // ldh ($4C),a   KEY0: CGB mode
    rom[0xFC] = 0x3E; rom[0xFD] = 0x11;                   // ld a,$11
    rom[0xFE] = 0xE0; rom[0xFF] = 0x50;                   // ldh ($50),a   unmap boot ROM
    GB_load_boot_rom_from_buffer(gb, rom, sizeof rom);
}
static int key_of(const char *s) {
    if (!strcmp(s, "A")) return GB_KEY_A; if (!strcmp(s, "B")) return GB_KEY_B;
    if (!strcmp(s, "START")) return GB_KEY_START; if (!strcmp(s, "SELECT")) return GB_KEY_SELECT;
    if (!strcmp(s, "LEFT")) return GB_KEY_LEFT; if (!strcmp(s, "RIGHT")) return GB_KEY_RIGHT;
    if (!strcmp(s, "UP")) return GB_KEY_UP; return GB_KEY_DOWN;
}
static void write_ppm(const char *path) {
    FILE *f = fopen(path, "wb"); fprintf(f, "P6\n160 144\n255\n");
    for (int i = 0; i < 160 * 144; i++) { uint32_t c = bitmap[i]; fputc(c >> 16, f); fputc((c >> 8) & 0xFF, f); fputc(c & 0xFF, f); }
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: rom script outdir bootdir [cc]\n"); return 1; }
    boot_dir = argv[4]; cc_mode = argc > 5 ? atoi(argv[5]) : 0;
    static GB_gameboy_t gb;
    GB_init(&gb, GB_MODEL_CGB_E);
    GB_set_boot_rom_load_callback(&gb, load_boot);
    GB_set_vblank_callback(&gb, vblank);
    GB_set_pixels_output(&gb, bitmap);
    GB_set_rgb_encode_callback(&gb, rgb_encode);
    GB_set_log_callback(&gb, log_cb);
    GB_set_sample_rate(&gb, 48000);
    GB_apu_set_sample_callback(&gb, sample_cb);
    GB_set_highpass_filter_mode(&gb, GB_HIGHPASS_ACCURATE);
    GB_set_color_correction_mode(&gb, cc_mode ? GB_COLOR_CORRECTION_MODERN_BALANCED : GB_COLOR_CORRECTION_DISABLED);
    if (GB_load_rom(&gb, argv[1])) { perror("rom"); return 1; }
    FILE *sf = fopen(argv[2], "r");
    int ef[4096], ek[4096], ea[4096]; static char en[4096][512]; int n = 0;
    char act[32], arg[512];
    while (n < 4096 && fscanf(sf, "%d %31s", &ef[n], act) == 2) {
        arg[0] = 0;
        if (strcmp(act, "end")) fscanf(sf, "%511s", arg);
        ea[n] = !strcmp(act, "press") ? 1 : !strcmp(act, "release") ? 0 : !strcmp(act, "shot") ? 2 : !strcmp(act, "poke") ? 4 : !strcmp(act, "wav") ? 5 : !strcmp(act, "wavstop") ? 6 : 3;
        ek[n] = (ea[n] < 2) ? key_of(arg) : 0; strcpy(en[n], arg); n++;
    }
    int frame = 0, idx = 0;
    while (1) {
        vblank_hit = 0;
        while (!vblank_hit) GB_run(&gb);
        frame++;
        while (idx < n && ef[idx] <= frame) {
            if (ea[idx] == 0 || ea[idx] == 1) GB_set_key_state(&gb, ek[idx], ea[idx]);
            else if (ea[idx] == 4) { unsigned a, v; sscanf(en[idx], "%x=%x", &a, &v); GB_write_memory(&gb, (uint16_t)a, (uint8_t)v); }
            else if (ea[idx] == 2) { char p[512]; snprintf(p, sizeof p, "%s/%s.ppm", argv[3], en[idx]); write_ppm(p); }
            else if (ea[idx] == 5) { snprintf(aud_path, sizeof aud_path, "%s", en[idx]); aud_on = 1; aud_n = 0; }
            else if (ea[idx] == 6) wav_write();
            else { if (aud_on) wav_write(); printf("end at frame %d\n", frame); return 0; }
            idx++;
        }
    }
}
