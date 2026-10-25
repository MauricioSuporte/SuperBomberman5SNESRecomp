/* Keep the Normal Game life counter ($1D23) from ever going down.
 *
 * The cartridge spends lives in two places, both interpreted:
 *   $C5:0659  DEC $1D23   after a two-player respawn ($C5:0640-$065C)
 *   $C5:3B1C-$3B2C        stage retry: one DEC per participant (P1/P2),
 *                         then $C5:3B2D tests the sign for Game Over.
 * The five-player co-op mod charges seats 3-5 in its own hook at $C5:3B1C.
 * This hook captures the counter before anything charges it (it runs before
 * the hook it wraps) and puts it back once the charges are done, so the
 * native Game Over test never sees a negative value. No ROM bytes change.
 * While enabled in a campaign stage the counter is also pinned to
 * INFINITE_LIVES_MAX every frame, so a five-player Start rejoin (which spends
 * a shared life outside the hooked instructions) never lowers it and a seat
 * can always come back. */
#include "infinite_lives_mod.h"
#include "common_rtl.h"
#include "mod_runtime.h"
#include "snes/interp_bridge.h"
#include <stdbool.h>

static bool enabled, installed, holding;
static uint8_t held;
#define INFINITE_LIVES_MAX 9
static void (*previous_hook)(uint32_t, int, int);

static bool campaign(void) {
    unsigned mode = g_ram[0x1031];
    return (mode == 8 || (mode >= 0x0d && mode < 0x74)) &&
           !(g_ram[0x1d1a] & 1);
}
static void lives_hook(uint32_t pc, int m, int x) {
    bool active = enabled && campaign();
    if (active && (pc == 0xc53b1c || pc == 0xc50659)) {
        held = g_ram[0x1d23];
        holding = true;
    }
    if (previous_hook) previous_hook(pc, m, x);
    if (holding && (pc == 0xc53b2d || pc == 0xc5065c)) {
        holding = false;
        if (active) g_ram[0x1d23] = held < INFINITE_LIVES_MAX ? INFINITE_LIVES_MAX : held;
    }
}
bool InfiniteLivesEnabled(void) { return enabled; }
void InfiniteLivesReset(void) {
    if (installed && g_interp_bridge_pc_hook == lives_hook)
        g_interp_bridge_pc_hook = previous_hook;
    installed = holding = false;
    previous_hook = NULL;
}
static void pin(void) {
    if (enabled && campaign() && g_ram[0x1d23] < INFINITE_LIVES_MAX)
        g_ram[0x1d23] = INFINITE_LIVES_MAX;
}
void InfiniteLivesAfterFrame(void) { pin(); }
void InfiniteLivesBeforeFrame(void) {
    pin();
    if (!enabled || installed) return;
    previous_hook = g_interp_bridge_pc_hook;
    g_interp_bridge_pc_hook = lives_hook;
    installed = true;
}
static void reset_plugin(void) { InfiniteLivesReset(); enabled = false; }
static void activate(void) { enabled = true; }
SNES_MOD_CONSTRUCTOR(register_infinite_lives) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.normal.infinite-lives", activate);
}
