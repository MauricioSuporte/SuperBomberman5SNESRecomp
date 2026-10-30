/* The Clock item ($C5:42D3) freezes actors 5-23: in the cartridge's Normal
 * Game those are enemies only, since the two mounts are actors 2-3 (rider +
 * $1CBF). Five-seat co-op moves the mounts to actors 19-23 and Battle keeps
 * them at 5-9, so a Clock (co-op Normal, or Terrorin's in Battle) froze the
 * Louies too: they stood still under their riders until the Clock ran out.
 *
 * $C5:42E9 (STX $8D) is reached for each living actor about to be frozen;
 * a mount is sent on to the loop's next actor ($C5:42F4 INX). Installed by
 * the mods that put mounts in that range (normal_coop_mod.c) or bring the
 * Clock into Battle (special_abilities.c); the stock game never needs it. */
#include "clock_mounts.h"
#include "actor_layout.h"
#include "common_cpu_infra.h"
#include "common_rtl.h"
#include "snes/interp_bridge.h"

enum { FREEZE_ONE = 0xc542e9, NEXT_ACTOR = 0xc542f4 };

bool Sb5ClockIsMount(const uint8_t *ram, unsigned actor) {
    unsigned offset = Sb5MountOffset(ram);
    return offset >= 5 && actor >= offset && actor < offset + 5;
}

static void freeze_one(CpuState *cpu, uint32_t pc) {
    (void)pc;
    if (Sb5ClockIsMount(g_ram, cpu->X & 0xff)) interp_bridge_pre_opcode_redirect(NEXT_ACTOR);
}

void Sb5ClockSparesMountsInstall(void) {
    static bool installed;
    if (installed || !g_rom) return;
    /* Only on the expected instruction: STX $8D, then LDA #$43. */
    const uint8_t *p = g_rom + (((FREEZE_ONE >> 16) & 0x3f) << 16 | (FREEZE_ONE & 0xffff));
    if (p[0] != 0x86 || p[1] != 0x8d || p[2] != 0xa9 || p[3] != 0x43) return;
    installed = interp_bridge_add_pre_opcode_hook(FREEZE_ONE, freeze_one);
}
