#ifndef GAME_H
#define GAME_H
#include <stdint.h>

#define MODE_CADET  0               // CADET: forgiving, auto-fire, a few slow aimed shots
#define MODE_PILOT  1               // PILOT: 3 hearts, aimed shot ~1.25 s
#define MODE_HUNTER 2               // HUNTER: the original hard mode

// shared game state (WRAM)
extern uint8_t gmode;
extern uint32_t score;
extern uint8_t lives, hearts, bombs;
extern uint8_t wtype, wlevel;            // weapon: 0 twin, 1 spread, 2 wisp; level 0-2
extern uint16_t hits_taken;               // times hit (hearts lost / lives lost)
extern uint8_t continues_used;
extern uint16_t play_seconds;
extern uint8_t midboss_killed;
extern uint8_t start_cp;                   // 0 normal, 2 = Kraken practice (hold SELECT on title)
#define N_PROFILES 3                        // CADET (easy), PILOT (medium), HUNTER (hard); profile == gmode
extern uint8_t profile;
extern uint8_t pf_unlock[N_PROFILES], pf_done[N_PROFILES];

// bounty outcome (set when the boss goes down) and contract pay
#define OUT_NONE    0
#define OUT_FLEE    1
#define OUT_KILL    2
#define OUT_CAPTURE 3
extern uint8_t bounty_out, mid_out;
extern uint32_t bounty_pts;                // points scored this mission (decides the outcome)
// Forge's armory: per-pilot gear levels, copied into g_gear[] for the current run
#define GEAR_HULL   0                       // +1 life per level
#define GEAR_CANNON 1                       // starting weapon level
#define GEAR_BOMBS  2                       // +1 bomb per level
#define GEAR_KENNEL 3                       // 1: buddy cap 3, 2: start with a buddy
#define GEAR_RUNES  4                       // 1: shots break big orbs
#define GEAR_LANCE  5                       // Lance Rune: max-level twin pierces
#define GEAR_RICO   6                       // Ricochet Rune: max-level spread bounces off the walls
#define GEAR_CHAIN  7                       // Chain Rune: max-level wisp jumps to the next enemy
#define N_GEAR 8
extern uint8_t g_gear[N_GEAR];
extern uint16_t pf_credits[N_PROFILES];
extern uint8_t pf_gear[N_PROFILES][N_GEAR];

// stage results
#define RES_CLEAR    1
#define RES_GAMEOVER 2
#define RES_QUIT     3

uint8_t stage_run(uint8_t sid) BANKED;     // bank 1
uint8_t cockpit_run(void) BANKED;          // bank 25: mission 5, the gun-cam stage
void game_main(void) BANKED;               // bank 7
void save_write(void);
void save_read(void);
uint8_t calc_rank(void);
#endif
