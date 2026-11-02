// ROM0 entry point: everything else lives in switchable banks.
#include <gb/gb.h>
#include "game.h"
void main(void) { game_main(); }
