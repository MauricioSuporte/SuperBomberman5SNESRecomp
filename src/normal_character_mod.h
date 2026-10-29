#pragma once
#include <stdint.h>
#include <stdbool.h>
struct SaveLoadInfo;
void NormalCharacterFrame(void);
void NormalCharacterInstallHook(void);
void NormalCharacterBeforeFrame(void);
void NormalCharacterReset(void);
void NormalCharacterSave(struct SaveLoadInfo *sli);
void NormalCharacterLoad(struct SaveLoadInfo *sli, uint32_t version);
void NormalCharacterLoaded(uint32_t version);
bool NormalCharacterEnabled(void);
/* Boss Rush (between frames): snapshot the title waiting on its start op,
 * then open the picker from the file-menu scene; it returns to the title. */
bool NormalCharacterSnapshotBossRush(void);
bool NormalCharacterOpenBossRush(void);
