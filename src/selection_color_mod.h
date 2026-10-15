#pragma once
#include <stdbool.h>
#include <stdint.h>
struct Ppu;
struct SaveLoadInfo;
void SelectionColorBeforeFrame(void);
void SelectionColorAfterFrame(void);
void SelectionColorDrawPpuFrame(void);
bool SelectionColorEnabled(void);
void SelectionColorObserve(const struct Ppu *ppu, unsigned line, void *context);
void SelectionColorReset(void);
void SelectionColorSave(struct SaveLoadInfo *sli);
void SelectionColorLoad(struct SaveLoadInfo *sli, uint32_t version);
