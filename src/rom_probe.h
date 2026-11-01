#ifndef SB5_ROM_PROBE_H
#define SB5_ROM_PROBE_H
/* A mod applies to a ROM only if every cartridge site it relies on holds what
 * that mod was written against (src/rom_probes.h, tools/gen-rom-probes.py). A
 * supported dump is never evaluated: all mods stay available, as before. Native
 * tests build without SB5_ROM_PROBES, so the gate compiles out of them. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mod_runtime.h"

#ifdef SB5_ROM_PROBES
void Sb5RomProbesEvaluate(const uint8_t *image, size_t len);
void Sb5RomProbesFailAll(void);
void Sb5RomSetSupported(bool supported);
bool Sb5RomProbeOk(const char *plugin_id);
bool Sb5RomIsSupported(void); /* false when running a dump other than rom_identity.txt */
SNESModActivationCallback Sb5RomGated(const char *plugin_id, SNESModActivationCallback callback);
#define SB5_ROM_GATED(id, callback) Sb5RomGated(id, callback)
#else
#define SB5_ROM_GATED(id, callback) (callback)
#endif
#endif
