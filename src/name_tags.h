#pragma once
#include <stdint.h>

/* Netplay name tags: where the nameplate of seat `seat` goes this frame, in
 * frame pixels (x = centre, y = bottom of the tag). Returns 0 when the seat
 * shows none: not in a stage, no character, dead, CPU or a boss rival. Pure
 * over a WRAM image so it can be unit-tested. */
int NameTagPlace(const uint8_t *ram, int seat, int *x, int *y);

/* After each simulated frame: hands the visible tags to the host overlay.
 * Inert outside a netplay match (the host names no seats then). */
void NameTagsAfterFrame(void);
