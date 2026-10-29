/* Boss Rush ranking screens (see boss_rush_results.c). */
#ifndef SB5_BOSS_RUSH_RESULTS_H
#define SB5_BOSS_RUSH_RESULTS_H
void BossRushResultsBeforeFrame(void); /* first in the frame: may hide the pads while typing */
void BossRushResultsAfterFrame(void);  /* gives the pads back */
void BossRushResultsDraw(void);        /* over the finished frame */
void BossRushResultsReset(void);
#endif
