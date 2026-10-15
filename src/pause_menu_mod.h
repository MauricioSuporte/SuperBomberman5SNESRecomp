#pragma once
#include <stdbool.h>
#include "beam_frame_driver.h"
struct SaveLoadInfo;
void PauseMenuBeforeFrame(void);
void PauseMenuAfterFrame(void);
void PauseMenuReset(void);
bool PauseMenuEnabled(void);
void PauseMenuObserve(const struct Ppu *, unsigned, void *);
void PauseMenuSave(struct SaveLoadInfo *);
void PauseMenuLoad(struct SaveLoadInfo *, unsigned);
