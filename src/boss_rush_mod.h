#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
struct SaveLoadInfo;
struct Ppu;
/* Boss Rush: a fourth title-menu entry that starts the cartridge's own boss
 * rush (password 3879) after the player-count menu and, with the Normal
 * character mod, the character picker. */
bool BossRushEnabled(void);
/* A boss rush started from the title is running (until the title returns). */
bool BossRushActive(void);
/* The title's main menu is on screen with the four-entry layout. */
bool BossRushTitleMenu(void);
/* Pause-menu action: leave the rush for the title screen. */
void BossRushQuitToTitle(void);
/* Called by the Normal character mod after the picker it opened for the
 * rush has closed (snapshot already restored). */
void BossRushPickerDone(bool cancelled);
uint32_t BossRushFrames(void);
/* Terrorin beaten during the rush: BossRushFrames() is the final time. */
bool BossRushFinished(void);
/* Participation mask of the rush ($1D25 bits). */
uint8_t BossRushRoster(void);
/* 1-5 while the cursor of the Boss Rush player-count menu is on that count. */
unsigned BossRushPlayerMenuCategory(void);
/* Text over the finished frame (menu font, black outline), for the ranking
 * screens. Lowercase prints as uppercase, other bytes >= 0x80 as '?'. */
int BossRushTextWidth(const char *s);
void BossRushText(int x, int y, const char *s, uint32_t rgb);
void BossRushShade(int x0, int y0, int x1, int y1);
/* "M:SS.CC" for a frame count (60 frames per second). */
void BossRushFormat(uint32_t frames, char *out, size_t size);
/* The picker closed and the rush starts from the title in a moment: the
 * title must not forget the choices yet. */
bool BossRushKeepChoices(void);
void BossRushBeforeFrame(void);
void BossRushAfterFrame(void);
void BossRushDraw(void);
void BossRushReset(void);
void BossRushSave(struct SaveLoadInfo *sli);
void BossRushLoad(struct SaveLoadInfo *sli, unsigned version);
