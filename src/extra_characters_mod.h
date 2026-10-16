#pragma once
#include <stdbool.h>
#include <stdint.h>
struct Ppu;
/* Extra-character core of the character picker: the Super Bomberman 5 extra
 * characters (Honey, Kotetsu and Emperor Terrorin, plus Subordinate Bomber
 * in the stock X cell) and, with the Bomber Kings mod, a third row with the
 * Super Bomberman 4 characters (choices 13-18). Extra characters play as a
 * stock bomber and are drawn with their own frames at render time. */
enum { HK_CHOICE_HONEY = 11, HK_CHOICE_KOTETSU = 12, KINGS_CHOICE_FIRST = 13, KINGS_CHOICE_LAST = 18,
       TERRORIN_CHOICE = 19, EXTRA_CHOICE_LAST = 19 };
/* The core runs (either plugin enabled). */
bool ExtraCharactersEnabled(void);
/* The Super Bomberman 5 Extra Characters plugin is enabled. */
bool Sb5ExtrasEnabled(void);
/* True once the picker patches are installed: the grid has up to nineteen
 * cells and the cell arrays live at PickerCellCharacters()/PickerCellColors(). */
bool ExtraCharactersInstalled(void);
uint8_t *PickerCellCharacters(void);
uint8_t *PickerCellColors(void);
unsigned PickerCellCount(void);
/* Native choice ($625F, 1 based) an extra choice plays as (Bomberman).
 * Other choices are returned unchanged. */
unsigned ExtraBaseChoice(unsigned choice);
/* Records the choice a campaign seat plays with (Normal picker hand-off). */
void ExtraSetSeatChoice(unsigned seat,unsigned choice);
void ExtraSetSeatColor(unsigned seat,unsigned color);
/* Extra choice (11-19) a seat plays with in the current stage, else 0. */
unsigned ExtraCharacterSeatChoice(unsigned seat);
/* Player palette (0-4) a seat's Super Bomberman 4 character uses. */
unsigned ExtraCharacterSeatColor(unsigned seat);
/* L/R colors of an extra choice: 1 (Honey/Kotetsu/Terrorin), 5 (Super
 * Bomberman 4), 0 for a native choice. */
unsigned ExtraCharacterColorCount(unsigned choice);
/* Stage overlay helper for ability effects (valid only while
 * SpecialAbilityDraw runs): draws w x h color indices at screen (x, y)
 * with its own palette. */
bool ExtraEmitPixels(int x,int y,const uint8_t *pixels,unsigned w,unsigned h,const uint16_t colors[16],unsigned priority);
/* Same, for a Honey/Kotetsu animation frame (skin 0/1) with its origin at
 * screen (x, y). */
bool ExtraEmitSkinFrame(unsigned skin,unsigned anim,unsigned step,int x,int y);
/* Stage overlay helper (valid only while SpecialAbilityDraw runs): tints the
 * screen box left..right x top..bottom (inclusive, screen pixels) with the
 * fixed color `color` (BGR555) through the color window, half-added to the
 * BG1/BG2/OBJ layers like Terrorin's barrier in ZONE5 - J. At most two
 * boxes a frame; nothing happens when the frame already uses color math or
 * the color window. */
bool ExtraTintBox(int left,int top,int right,int bottom,uint16_t color);
void ExtraCharactersBeforeFrame(void);
void ExtraCharactersObserve(const struct Ppu *ppu,unsigned line,void *context);
