#pragma once
#include <stdint.h>
/* Actor layout shared by the mods. Actors below $1CBF are bombers (players
 * and boss rivals); a bomber's mount is actor rider + offset. The cartridge
 * uses offset = $1CBF (campaign 2, Battle 5). Five-seat co-op keeps $1CBF = 5
 * and puts the mounts at rider + 19, storing 19 at $011E (see
 * normal_coop_mod.c, MOUNT_OFFSET_RAM). */
static inline unsigned Sb5MountOffset(const uint8_t *ram) {
    return ram[0x1cbf]==5 && ram[0x11e]==19 ? 19u : ram[0x1cbf];
}
