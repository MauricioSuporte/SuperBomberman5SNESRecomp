/* Boss Rush ranking over a Discord webhook (no server).
 *
 * The webhook URL and the board message id come from boss_rush_ranking.cfg
 * in the game folder (written into release packages by CI from repository
 * secrets; never in Git). Without it, or without internet, every call here
 * is a no-op and the game never waits: requests run on a background thread
 * through the system's curl (Windows 10+ ships curl.exe), with a short
 * timeout, and the caller only polls results. */
#ifndef SB5_BOSS_RUSH_NET_H
#define SB5_BOSS_RUSH_NET_H
#include "boss_rush_ranking.h"
#include <stdbool.h>

bool BrNetConfigured(void);
/* Fetch the board in the background (at most once a minute unless forced). */
void BrNetRefresh(bool force);
/* 1: board available (copied to *out), 0: nothing yet / in flight, -1: last
 * fetch failed (offline, no config, bad answer). */
int BrNetBoard(BrBoard *out);

enum { BR_SUBMIT_IDLE, BR_SUBMIT_BUSY, BR_SUBMIT_RANKED, BR_SUBMIT_NOT_TOP, BR_SUBMIT_FAILED };
/* Read the current board, insert `e`, and when it enters the top 5 edit the
 * board message and post a notification. Once per entry. */
void BrNetSubmit(const BrEntry *e);
int BrNetSubmitState(unsigned *rank);
void BrNetSubmitClear(void);
#endif
