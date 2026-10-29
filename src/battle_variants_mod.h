#pragma once
#include <stdbool.h>
struct SaveLoadInfo;
/* Battle stage variants picker (password layouts 4622/0413/0926 and the
 * 0110 item rule) between the arena carousel and the match. */
bool BattleVariantsEnabled(void);
bool BattleVariantsMenuOpen(void);
void BattleVariantsBeforeFrame(void);
void BattleVariantsAfterFrame(void);
void BattleVariantsDraw(void);
void BattleVariantsReset(void);
void BattleVariantsSave(struct SaveLoadInfo *sli);
void BattleVariantsLoad(struct SaveLoadInfo *sli, unsigned version);
