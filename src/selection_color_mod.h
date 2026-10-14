#pragma once
#include <stdint.h>
struct SaveLoadInfo;
void SelectionColorBeforeFrame(void);
void SelectionColorAfterFrame(void);
void SelectionColorDrawPpuFrame(void);
void SelectionColorReset(void);
void SelectionColorSave(struct SaveLoadInfo *sli);
void SelectionColorLoad(struct SaveLoadInfo *sli, uint32_t version);
