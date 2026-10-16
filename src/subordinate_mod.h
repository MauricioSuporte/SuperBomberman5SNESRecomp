#pragma once
#include <stdbool.h>
#include <stdint.h>
struct Ppu;
/* Subordinate Bomber (cartridge character index 9) as a selectable bomber. */
bool SubordinateEnabled(void);
/* Called by the Super Bomberman 5 extra characters plugin. */
void SubordinateActivate(void);
void SubordinateBeforeFrame(void);
void SubordinateObserve(const struct Ppu *ppu,unsigned line,void *context);
