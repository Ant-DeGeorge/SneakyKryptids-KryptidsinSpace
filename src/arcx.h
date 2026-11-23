// Arcade extras (arcx.c, autobanked): the arcade-only weapons and the seamless two-lap maps.
#ifndef ARCX_H
#define ARCX_H
#include <gb/gb.h>
#include <stdint.h>
// shot types added for the arcade weapons (stage.c draws them from its shot tables)
#define PB_PRISM 8
#define PB_SPARK 9
#define PB_HELIXL 10
#define PB_HELIXR 11
#define PB_NOVA 12
#define N_PBTYPES 13
#define PIERCES(t) ((t) == 5 || (t) >= PB_HELIXL)       // the lance, helix orbs and the nova orb go through enemies
// stage.c state the arcade code reads and writes
extern uint8_t pb_on[], pb_x[], pb_y[], pb_type[], pb_dmg[], pb_tgt[];
extern int8_t pb_vx[], pb_vy[];
extern uint8_t p_fire_cd, mz_t, mz_k, pop_t, flash_t;
extern const char *pop_s;
extern uint16_t next_row;
void arcx_load(void) BANKED;          // shot and capsule sprites into OBJ VRAM bank 1
void arcx_fire(void) BANKED;          // fire the current arcade weapon (wtype 3-5)
void arcx_move(uint8_t i) BANKED;     // steer one arcade shot before it moves
void arcx_wpal(void) BANKED;          // cycle the weapon palette (shots, exhaust)
void arcx_tick(void) BANKED;          // lap 1: start the warp when the map runs out
void arcx_lap(void) BANKED;           // lap 1 -> lap 2, the map keeps scrolling
#endif
