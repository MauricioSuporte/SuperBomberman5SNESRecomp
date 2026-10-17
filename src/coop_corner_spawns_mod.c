/* Corner spawns for the five-player Normal Game (normal_coop_mod.c).
 *
 * This plugin only switches the spawn layout of regular campaign stages.
 * With it off, P3 and P5 start next to P1 and P4 next to P2; with it on,
 * P3 starts in the lower-left corner, P4 in the lower-right corner and P5 in
 * the centre. Boss layouts, the safe-floor search and the post-map settle
 * pass are shared and unchanged. Without the five-player mod it does nothing. */
#include "coop_corner_spawns_mod.h"
#include "mod_runtime.h"

static bool enabled;
bool CoopCornerSpawnsEnabled(void) { return enabled; }
static void reset_plugin(void) { enabled=false; }
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_coop_corner_spawns) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.normal.coop.corner-spawns",activate);
}
