#ifndef SB5_ROM_IDENTITY_RESOLVE_H
#define SB5_ROM_IDENTITY_RESOLVE_H
#include "host_main.h"
/* Points `game` at the ROM this run will use, when it is not rom_identity.txt's. */
void Sb5ResolveRomIdentity(SnesDesktopHostGame *game, int argc, char **argv);
#endif
