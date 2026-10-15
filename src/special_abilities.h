#pragma once
#include <stdbool.h>
/* B + direction special abilities of the extra characters (Honey, Kotetsu
 * and the Super Bomberman 4 Bomber Kings), with Super Bomberman 4's debuff. */
void SpecialAbilitiesInstall(void);
/* Direction (0 right, 1 down, 2 left, 3 up) of the ability pose a seat
 * shows now, or -1. */
int SpecialAbilityPose(unsigned seat);
bool SpecialAbilityDebuffed(unsigned seat);
/* Draws the active effects (called by the extra-character overlay). */
void SpecialAbilityDraw(void);
