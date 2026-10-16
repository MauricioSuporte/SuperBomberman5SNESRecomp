#pragma once
#include <stdint.h>
struct SaveLoadInfo;
void NormalCharacterFrame(void);
void NormalCharacterInstallHook(void);
void NormalCharacterBeforeFrame(void);
void NormalCharacterReset(void);
void NormalCharacterSave(struct SaveLoadInfo *sli);
void NormalCharacterLoad(struct SaveLoadInfo *sli, uint32_t version);
void NormalCharacterLoaded(uint32_t version);
