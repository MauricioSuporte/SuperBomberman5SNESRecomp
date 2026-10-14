#pragma once
#include <stdint.h>
#include <stdbool.h>
struct SaveLoadInfo;
void NormalCoopBeforeFrame(void);
void NormalCoopAfterFrame(void);
void NormalCoopReset(void);
void NormalCoopSave(struct SaveLoadInfo *sli);
void NormalCoopLoad(struct SaveLoadInfo *sli, uint32_t version);
unsigned NormalCoopPlayerCount(void);
bool NormalCoopPlayerMenu(void);
