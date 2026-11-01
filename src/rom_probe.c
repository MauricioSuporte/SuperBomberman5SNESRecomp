#include "rom_probe.h"
#include "crc32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Sb5RomProbe {
    const char *plugin_id;
    const uint32_t *sites; /* file offsets of four-byte windows */
    unsigned count;
    uint32_t crc32;
} Sb5RomProbe;
#include "rom_probes.h"

enum { PROBE_COUNT = sizeof(kRomProbes) / sizeof(kRomProbes[0]) };
static bool s_evaluated, s_ok[PROBE_COUNT], s_unsupported;

static bool probe_matches(const Sb5RomProbe *probe, const uint8_t *image, size_t len) {
    uint8_t *joined = malloc((size_t)probe->count * 4u + 1u);
    bool ok = joined != NULL;
    for (unsigned i = 0; ok && i < probe->count; ++i) {
        if ((size_t)probe->sites[i] + 4u > len) ok = false;
        else memcpy(joined + i * 4u, image + probe->sites[i], 4);
    }
    ok = ok && crc32_compute(joined, (size_t)probe->count * 4u) == probe->crc32;
    free(joined);
    return ok;
}

void Sb5RomProbesEvaluate(const uint8_t *image, size_t len) {
    s_evaluated = true;
    for (unsigned i = 0; i < PROBE_COUNT; ++i) {
        s_ok[i] = probe_matches(&kRomProbes[i], image, len);
        if (!s_ok[i]) printf("[rom] mod %s is off: this ROM differs at its sites\n", kRomProbes[i].plugin_id);
    }
}

void Sb5RomProbesFailAll(void) {
    s_evaluated = true;
    memset(s_ok, 0, sizeof(s_ok));
    printf("[rom] this build's ROM was not found: mods that need the cartridge are off\n");
}

void Sb5RomSetSupported(bool supported) { s_unsupported = !supported; }
bool Sb5RomIsSupported(void) { return !s_unsupported; }

bool Sb5RomProbeOk(const char *plugin_id) {
    if (!s_evaluated) return true;
    for (unsigned i = 0; i < PROBE_COUNT; ++i)
        if (strcmp(kRomProbes[i].plugin_id, plugin_id) == 0) return s_ok[i];
    return true; /* Touches no cartridge site. */
}

/* Mods register in static constructors, before the ROM is known, so each gets a
 * stand-in that decides when activated. Registered either way: an enabled mod
 * that a ROM can't host must not stop Play with a conflict. It only does nothing. */
enum { MAX_GATES = 24 };
static struct { const char *plugin_id; SNESModActivationCallback callback; } s_gates[MAX_GATES];
static unsigned s_gate_count;
static void gate_fire(unsigned index) {
    if (Sb5RomProbeOk(s_gates[index].plugin_id)) s_gates[index].callback();
}
#define G(n) static void gate##n(void) { gate_fire(n); }
G(0) G(1) G(2) G(3) G(4) G(5) G(6) G(7) G(8) G(9) G(10) G(11)
G(12) G(13) G(14) G(15) G(16) G(17) G(18) G(19) G(20) G(21) G(22) G(23)
#undef G
static const SNESModActivationCallback kGates[MAX_GATES] = {
    gate0, gate1, gate2, gate3, gate4, gate5, gate6, gate7, gate8, gate9, gate10, gate11,
    gate12, gate13, gate14, gate15, gate16, gate17, gate18, gate19, gate20, gate21, gate22, gate23,
};
SNESModActivationCallback Sb5RomGated(const char *plugin_id, SNESModActivationCallback callback) {
    if (s_gate_count == MAX_GATES) {
        fprintf(stderr, "[rom] too many gated mods: %s is not gated\n", plugin_id);
        return callback;
    }
    s_gates[s_gate_count].plugin_id = plugin_id;
    s_gates[s_gate_count].callback = callback;
    return kGates[s_gate_count++];
}
