#ifndef SOUND_H
#define SOUND_H
#include <stdint.h>
#include "songs.h"

enum { SFX_SHOT, SFX_SHOT2, SFX_TICK, SFX_EXPL_S, SFX_EXPL_L, SFX_HURT, SFX_PICKUP, SFX_POWER, SFX_BOMB, SFX_ALARM,
       SFX_START, SFX_BLIP, SFX_BOSSHIT, SFX_ONEUP, SFX_SELECT, SFX_ROAR, SFX_WHOOSH, SFX_SPIT, SFX_THUNDER };

void sound_init(void);
void sound_update(void);
void sfx_play(uint8_t id);
void music_play(uint8_t song);
void music_stop(void);
uint8_t music_playing(void);

#endif
