#ifndef ARCADE_H
#define ARCADE_H
#include <gb/gb.h>
#include <stdint.h>
// Arcade: an endless run through missions 1-4 in a loop, one ship of seven, no continues.
void arcade_run(uint8_t n_ships) BANKED;   // n_ships = how many ships are unlocked (1-7)
void arc_begin(void) BANKED;               // called by stage_run when g_arc is set
#endif
