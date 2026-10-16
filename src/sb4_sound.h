#pragma once
#include <stdint.h>
/* Plays Super Bomberman 4 sound effect `id` (the number the cartridge sends
 * to its sound driver), rendered on first use from the player's own SB4
 * image. Nothing happens without the image. Effects used by the abilities:
 * $39 explosion (Bazooka beam, Jet trail), $4E mace swing, $5A laser,
 * $5F Great's guard. */
void Sb4SoundPlay(uint8_t id);
/* Drops the rendered clips (a different image was loaded). */
void Sb4SoundReset(void);
