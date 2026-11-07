#pragma once
#include <stdbool.h>
struct SaveLoadInfo;
void LouieBeforeFrame(void);
void LouieAfterFrame(void);
void LouieReset(void);
bool LouieOwnsBomb(unsigned bomb); /* a Yellow Louie kick moves this bomb: its chain is the Louie mod's */
void LouieSave(struct SaveLoadInfo *sli);
void LouieLoad(struct SaveLoadInfo *sli, unsigned version);
