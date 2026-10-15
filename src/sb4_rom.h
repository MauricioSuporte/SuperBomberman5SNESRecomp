#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* The player's own Super Bomberman 4 image (verified by SHA-256), loaded
 * on first use; NULL when none was found. Never stored or redistributed. */
enum { SB4_CHARACTERS = 6, SB4_COLORS = 5 };
/* Character order: Bomberman, Hammer, Jet, Lady, Bazooka, Great. */
const uint8_t *Sb4Rom(void);
/* Loads a specific file (the launcher selection) when it is a verified image. */
bool Sb4RomLoad(const char *path);
const char *Sb4RomPath(void);
bool Sb4RomAccept(const uint8_t *data,size_t size);
/* Battle frame `frame` (0-63) of a character as 32x32 color indices. */
bool Sb4Frame(unsigned character,unsigned frame,uint8_t out[32][32]);
/* CPU address of battle frame `frame`, and any 32x32 frame of the same
 * layout (four tile rows $200 apart) by address: the ability poses. */
uint32_t Sb4FramePc(unsigned character,unsigned frame);
bool Sb4FrameAt(uint32_t pc,uint8_t out[32][32]);
/* Battle fire (explosion) 16x16 block `code` (tiles code, +1, +16, +17)
 * with its palette, and the effect OBJ block at OBJ tile `tile` ($80 mace,
 * $82/$84 laser), drawn with the player's palette. */
bool Sb4FireBlock(unsigned code,uint8_t out[16][16]);
uint16_t Sb4FireColor(unsigned index);
bool Sb4EffectBlock(unsigned tile,uint8_t out[16][16]);
/* Battle picker portrait (32x32) and its colors per player palette. */
bool Sb4Portrait(unsigned character,uint8_t out[32][32]);
uint16_t Sb4PortraitColor(unsigned character,unsigned palette,unsigned index);
/* BGR555 color `index` of player palette `palette` (0-4). */
uint16_t Sb4Color(unsigned character,unsigned palette,unsigned index);
void Sb4RomForTests(const uint8_t *data);
