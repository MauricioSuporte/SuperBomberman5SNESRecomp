#pragma once
#include <stdbool.h>
struct Ppu;
void NormalHudBeforeFrame(void);
void NormalHudReset(void);
bool NormalHudEnabled(void);
void NormalHudObserve(const struct Ppu *, unsigned, void *);
