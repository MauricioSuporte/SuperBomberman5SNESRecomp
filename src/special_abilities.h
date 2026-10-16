#pragma once
#include <stdbool.h>
/* B + direction special abilities of the extra characters (Honey, Kotetsu
 * and the Super Bomberman 4 Bomber Kings), with Super Bomberman 4's debuff. */
void SpecialAbilitiesInstall(void);
#include <stdint.h>
/* Pose a seat shows now for its ability: -1 for none, else the direction
 * (0 right, 1 down, 2 left, 3 up). Honey/Kotetsu: `step` of their attack
 * animation; Super Bomberman 4 characters: `sb4_frame`, the CPU address of
 * the 32x32 Super Bomberman 4 frame to draw (0 = the plain pose). */
int SpecialAbilityPose(unsigned seat,unsigned *step,uint32_t *sb4_frame);
bool SpecialAbilityDebuffed(unsigned seat);
/* Draws the active effects (called by the extra-character overlay). */
void SpecialAbilityDraw(void);
