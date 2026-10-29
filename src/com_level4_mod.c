/* Battle COM level 4 (Super) without the 1103 password.
 *
 * The password handler $C5:68E0 stores 4 in $7E6247, the highest COM level
 * the Battle settings script offers ($C6:91F1 compares the chosen level
 * $625B against it before incrementing). Boot ($C0:319D) stores 3, then
 * $C0:486D raises it to 4 only when SRAM $3061E8 bit 4 is set. This plugin
 * keeps the WRAM limit at 4 while it is enabled; SRAM and the ROM are never
 * changed, so turning the mod off restores the stock 3 levels on the next
 * boot (a password-unlocked save keeps its own 4). */
#include "com_level4_mod.h"
#include "common_rtl.h"
#include "mod_runtime.h"
#include <stdbool.h>

enum { kComLevelLimit = 0x6247, kSuperLevel = 4 };

static bool enabled;

bool ComLevel4Enabled(void) { return enabled; }

void ComLevel4BeforeFrame(void) {
    /* WRAM only: save states and netplay snapshots already carry it. */
    if (enabled && g_ram[kComLevelLimit] < kSuperLevel)
        g_ram[kComLevelLimit] = kSuperLevel;
}

static void reset_plugin(void) { enabled = false; }
static void activate(void) { enabled = true; }
SNES_MOD_CONSTRUCTOR(register_com_level4) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.battle.com-level-4", activate);
}
