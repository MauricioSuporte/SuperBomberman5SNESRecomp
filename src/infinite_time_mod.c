/* Reuse the cartridge's unlimited timer and native HUD, without ROM patches. */
#include "infinite_time_mod.h"
#include "boss_rush_mod.h"
#include "common_rtl.h"
#include "mod_runtime.h"
#include "snes/interp_bridge.h"
#include <stdbool.h>

static bool enabled, installed;
static void (*previous_hook)(uint32_t, int, int);
/* Cheats stay off for a whole Boss Rush, so ranked times are played straight. */
static bool campaign(void) {
    unsigned mode = g_ram[0x1031];
    if (BossRushActive()) return false;
    return (mode == 8 || (mode >= 0x0d && mode < 0x74)) &&
           !(g_ram[0x1d1a] & 1);
}
static void timer_hook(uint32_t pc, int m, int x) {
    if (previous_hook) previous_hook(pc, m, x);
    /* Initialization has finished before $0400; $0460 precedes the
     * expiration test; $0BEF is the native HUD update (also after load). */
    if (enabled && campaign() &&
        (pc == 0xc50400 || pc == 0xc50460 || pc == 0xc50bef)) {
        g_ram[0x0e1d] = 99;
        g_ram[0x0e1e] = 9;
    }
}
void InfiniteTimeReset(void) {
    if (installed && g_interp_bridge_pc_hook == timer_hook)
        g_interp_bridge_pc_hook = previous_hook;
    installed = false;
    previous_hook = NULL;
}
void InfiniteTimeBeforeFrame(void) {
    if (!enabled || installed) return;
    previous_hook = g_interp_bridge_pc_hook;
    g_interp_bridge_pc_hook = timer_hook;
    installed = true;
}
static void reset_plugin(void) { InfiniteTimeReset(); enabled = false; }
static void activate(void) { enabled = true; }
SNES_MOD_CONSTRUCTOR(register_infinite_time) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.normal.infinite-time", activate);
}
