#pragma once
#include <stdbool.h>
struct SaveLoadInfo;
void LouieBeforeFrame(void);
void LouieAfterFrame(void);
void LouieReset(void);
void LouieSave(struct SaveLoadInfo *sli);
void LouieLoad(struct SaveLoadInfo *sli, unsigned version);
