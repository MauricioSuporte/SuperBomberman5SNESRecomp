/* The Clock item spares mounts (see clock_mounts.c). */
#ifndef SB5_CLOCK_MOUNTS_H
#define SB5_CLOCK_MOUNTS_H
#include <stdbool.h>
#include <stdint.h>
bool Sb5ClockIsMount(const uint8_t *ram, unsigned actor);
void Sb5ClockSparesMountsInstall(void);
#endif
