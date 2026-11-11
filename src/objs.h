// Shared game objects (ROM0): geometry, entity pools, comms, bosses, helpers.
// Used by the generic stage engine (bank 1) and every stage's logic bank.
#ifndef OBJS_H
#define OBJS_H
#include <gb/gb.h>
#include <stdint.h>
#include "game.h"
#include "stage_def.h"

// Object coords: X = screen x + 16, Y = screen y + 32
#define XO 16
#define YO 32
#define SCR_BOTTOM (136 + YO)
#define NEAR(a, b, r) ((uint8_t)((uint8_t)(a) - (uint8_t)(b) + (uint8_t)(r)) < (uint8_t)((r) * 2))
#define HUNTER (gmode == MODE_HUNTER)
#define MIDMODE (gmode == MODE_PILOT)
#define MAXHEARTS (MIDMODE ? 3 : 5)
#define COS64(a) sin64[((a) + 16) & 63]
#define DCOS(d) dsin[((d) + 8) & 31]
#define RAT(row0, n) ((row0) - 18 + (n))

#define MAXE  8
#define MAXEB 14
#define MAXPB 10
#define MAXPK 3

// ---- enemy types (global across stages)
enum { E_NONE, E_BOOM, E_SLEEPER,
       E_KL, E_JELLY, E_SKIFF, E_ICE, E_GUN,           // stage 1
       E_MOTH, E_EEL, E_CLOUD, E_MOTHMAN,              // stage 2
       E_IMP, E_CONE, E_WISP, E_GUN3,                  // stage 3
       E_TURRET, E_FBAT, E_SNAKE, E_BEAST,             // stage 4
       N_ETYPES };
// draw styles
enum { SY_NONE, SY_16, SY_16HP, SY_32, SY_BOOM, SY_EEL, SY_BLINK, SY_SLEEP };
// static type flags
#define TF_NOBODY 1     // body contact does not damage it (ice)
#define TF_BURST  2     // bursts into bullets when killed
#define TF_MID    4     // mid-boss
#define TF_NOHIT  8     // shots pass through (sleeper)
// runtime flags (e_fl)
#define EF_HIDDEN 1     // intangible + not drawn solid (wisp)
#define EF_FLIP   2     // 32x32 sprite drawn mirrored

enum { ST_PLAY, ST_DYING, ST_BOSS, ST_CLEAR };
enum { PK_WEAP, PK_BOMB = 3, PK_HEART, PK_1UP };

extern const int8_t sin64[64];
extern const int8_t dsin[32];

extern uint8_t stage_id;
extern const StageDef *SD;
extern uint8_t st, st_t, scroll_on;
extern uint16_t pos;
extern uint8_t px, py;
extern uint8_t hud_dirty;
extern uint32_t next_extend;
extern uint8_t kill_count;
extern uint8_t kp_new, kp_x, kp_y, kp_bigf;

extern uint8_t e_type[MAXE], e_hp[MAXE], e_t[MAXE], e_flash[MAXE], e_a[MAXE], e_b[MAXE], e_c[MAXE], e_st[MAXE], e_fl[MAXE];
extern uint16_t e_x[MAXE], e_y[MAXE];
extern int16_t e_vx[MAXE], e_vy[MAXE];
extern uint8_t eb_on[MAXEB], eb_big[MAXEB];
extern uint16_t eb_x[MAXEB], eb_y[MAXEB];
extern int16_t eb_vx[MAXEB], eb_vy[MAXEB];
extern uint8_t pk_on[MAXPK], pk_kind[MAXPK], pk_t[MAXPK];
extern uint16_t pk_x[MAXPK], pk_y[MAXPK];

extern const uint8_t e_tA[N_ETYPES], e_tB[N_ETYPES], e_pal[N_ETYPES], e_style[N_ETYPES], e_tf[N_ETYPES];

// chains (storm eels): segment centres
#define NCH 2
#define NCSEG 5
extern uint8_t ch_used[NCH], ch_x[NCH][NCSEG], ch_y[NCH][NCSEG];

// lightning hazards: vertical bolt columns
#define NHZ 3
extern uint8_t hz_on[NHZ], hz_x[NHZ], hz_t[NHZ], hz_y0[NHZ];   // hz_on: 1 warning, 2 striking

// mid-boss
extern uint8_t mb_active, mb_done, mb_slot;
extern uint16_t mb_timer;

// boss (generic big BG boss in the top band)
#define NBP 14
extern uint16_t k_hp, k_hpmax;
extern uint8_t k_state, k_hitflash, k_retreat, k_shake, boss_comm, k_face;
extern int8_t k_sway, k_bob;
extern int8_t k_hdx; extern uint8_t k_hw, k_hy0, k_hy1;      // weak spot relative to band centre
extern const char *boss_name;
extern const char * const boss_label[4];
extern uint8_t bp_n, bp_x[NBP], bp_y[NBP], bp_tile[NBP], bp_pal[NBP], bp_hurt[NBP];
extern uint8_t warn_x, warn_t;
extern int8_t wind;          // sideways push on the player (px per 4 frames)

uint8_t aim(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void eb_fire(uint8_t cx, uint8_t cy, uint8_t d, uint8_t spd, uint8_t big);
void eb_ring(uint8_t cx, uint8_t cy, uint8_t n, uint8_t phase, uint8_t spd, uint8_t big);
int8_t e_new(uint8_t type, uint8_t x, uint8_t y);
void boom_at(uint8_t x, uint8_t y);
void add_score(uint16_t pts);
void spawn_pickup(uint8_t x, uint8_t y, uint8_t kind);
void e_kill(uint8_t i);
void e_damage(uint8_t i, uint8_t dmg);
void set_pal_slot(uint8_t slot, uint8_t bp);
extern uint8_t cur_bp[8];
void comm_queue_add(uint8_t id);
void comm_update(void);
void comm_reset(void);
extern uint8_t comm_on;
void boss_face(uint8_t v);
void boss_load(void);
void boss_bar(void);
void boss_clear(void);
uint8_t boss_outro(void);
extern uint8_t outro_t;
uint8_t ch_alloc(void);

// stage logic banks
void s1_reset(uint8_t cp) BANKED; void s1_tick(void) BANKED; void s1_boss_start(void) BANKED;
void s2_reset(uint8_t cp) BANKED; void s2_tick(void) BANKED; void s2_boss_start(void) BANKED;
void s3_reset(uint8_t cp) BANKED; void s3_tick(void) BANKED; void s3_boss_start(void) BANKED;
void s4_reset(uint8_t cp) BANKED; void s4_tick(void) BANKED; void s4_boss_start(void) BANKED;
#endif
