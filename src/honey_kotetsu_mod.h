#pragma once
#include <stdbool.h>
#include <stdint.h>
struct Ppu;
/* Honey and Kotetsu (the secret-stage bosses of Zones 1-3) as selectable
 * characters in a six-column picker. They play as a stock bomber and are
 * drawn with their own cartridge frames at render time. */
enum { HK_CHOICE_HONEY = 11, HK_CHOICE_KOTETSU = 12 };
bool HoneyKotetsuEnabled(void);
/* True once the picker patches are installed: the grid has twelve cells
 * and the cell arrays live at PickerCellCharacters()/PickerCellColors(). */
bool HoneyKotetsuInstalled(void);
uint8_t *PickerCellCharacters(void);
uint8_t *PickerCellColors(void);
unsigned PickerCellCount(void);
/* Native choice ($625F, 1 based) a Honey/Kotetsu choice plays as. Other
 * choices are returned unchanged. */
unsigned HoneyKotetsuBaseChoice(unsigned choice);
/* Records the choice a campaign seat plays with (Normal picker hand-off). */
void HoneyKotetsuSetSeatChoice(unsigned seat,unsigned choice);
void HoneyKotetsuBeforeFrame(void);
void HoneyKotetsuObserve(const struct Ppu *ppu,unsigned line,void *context);
