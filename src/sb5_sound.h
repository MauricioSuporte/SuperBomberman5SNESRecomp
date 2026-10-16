#pragma once
#include <stdint.h>
/* Plays Super Bomberman 5 sound effect `effect` (the value the cartridge
 * passes to $C0:0CED, group 2 only) as sound set `set` ($C0:0E9C) has it,
 * rendered on first use from the player's cartridge in a private APU:
 * Terrorin's barrier is effect $0288 of set $0F (ZONE5 - J). */
void Sb5StageSoundPlay(unsigned set,uint16_t effect);
