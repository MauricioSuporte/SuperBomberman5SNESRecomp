#pragma once
#include <stdbool.h>
struct SaveLoadInfo;
/* Battle rules page 2: round-start countdown and shuffled spawn points. */
bool BattleRulesEnabled(void);
bool BattleRulesCountdownActive(void);
bool BattleRulesAbilitiesOff(void); /* rules page 2: extra characters' B + direction abilities disabled */
void BattleRulesBeforeFrame(void);
void BattleRulesAfterFrame(void);
void BattleRulesPrepareDraw(void); /* before the PPU frame: blank the hidden rows */
void BattleRulesFinishDraw(void);  /* after the PPU frame: put VRAM/OAM back */
struct Ppu;
bool BattleRulesPageOpen(void);
void BattleRulesApplyText(struct Ppu *ppu); /* inside the PPU frame: page 2 labels in the native font */
void BattleRulesDraw(void);        /* over the finished frame */
void BattleRulesReset(void);
void BattleRulesSave(struct SaveLoadInfo *sli);
void BattleRulesLoad(struct SaveLoadInfo *sli, unsigned version);
