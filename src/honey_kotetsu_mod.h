#pragma once
#include <stdbool.h>
#include <stdint.h>
struct Ppu;
/* Honey and Kotetsu (the secret-stage bosses of Zones 1-3) as selectable
 * characters in a six-column picker. They play as a stock bomber and are
 * drawn with their own cartridge frames at render time. This module is also
 * the extra-character core: with the Bomber Kings mod a third row holds the
 * Super Bomberman 4 characters (choices 13-18). */
enum { HK_CHOICE_HONEY = 11, HK_CHOICE_KOTETSU = 12, KINGS_CHOICE_FIRST = 13, EXTRA_CHOICE_LAST = 18 };
bool HoneyKotetsuEnabled(void);
/* True once the picker patches are installed: the grid has twelve (or
 * eighteen) cells and the cell arrays live at PickerCellCharacters()/
 * PickerCellColors(). */
bool HoneyKotetsuInstalled(void);
uint8_t *PickerCellCharacters(void);
uint8_t *PickerCellColors(void);
unsigned PickerCellCount(void);
/* Native choice ($625F, 1 based) an extra choice plays as (Bomberman).
 * Other choices are returned unchanged. */
unsigned HoneyKotetsuBaseChoice(unsigned choice);
/* Records the choice a campaign seat plays with (Normal picker hand-off). */
void HoneyKotetsuSetSeatChoice(unsigned seat,unsigned choice);
/* Extra choice (11-18) a seat plays with in the current stage, else 0. */
unsigned ExtraCharacterSeatChoice(unsigned seat);
/* Player palette (0-4) a seat's Super Bomberman 4 character uses. */
unsigned ExtraCharacterSeatColor(unsigned seat);
/* L/R colors of an extra choice: 1 (Honey/Kotetsu), 5 (Super Bomberman 4),
 * 0 for a native choice. */
unsigned ExtraCharacterColorCount(unsigned choice);
/* Stage overlay helper for ability effects (valid only while
 * SpecialAbilityDraw runs): draws w x h color indices at screen (x, y)
 * with its own palette. */
bool ExtraEmitPixels(int x,int y,const uint8_t *pixels,unsigned w,unsigned h,const uint16_t colors[16],unsigned priority);
void HoneyKotetsuBeforeFrame(void);
void HoneyKotetsuObserve(const struct Ppu *ppu,unsigned line,void *context);
