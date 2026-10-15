#pragma once
#include <stdbool.h>
struct SaveLoadInfo;
bool GoldBattleHyper(void);
unsigned GoldBattleArena(void);
void GoldBattleReset(void);
void GoldBattleBeforeFrame(void);
void GoldBattleSave(struct SaveLoadInfo *sli);
void GoldBattleLoad(struct SaveLoadInfo *sli,unsigned version);
