/* Bomber Kings: Super Bomberman 4's Bomberman, Hammer, Jet, Lady, Bazooka
 * and Great Bomber as playable characters.
 *
 * This plugin only switches the extra-character core (honey_kotetsu_mod.c)
 * to its three-row picker and enables the Super Bomberman 4 skins; their
 * frames and palettes are read at runtime from the player's own Super
 * Bomberman 4 image (sb4_rom.c). Without a verified image the picker keeps
 * the two-row Honey/Kotetsu layout and nothing else changes. Their special
 * abilities (B + direction) live in special_abilities.c. */
#include "bomber_kings_mod.h"
#include "sb4_rom.h"
#include "mod_runtime.h"
#include <stdio.h>

static bool wanted,reported;
bool BomberKingsWanted(void) { return wanted; }
bool BomberKingsReady(void) {
    if(!wanted) return false;
    bool ready=Sb4Rom()!=NULL;
    if(!reported) {
        reported=true;
        if(ready) fprintf(stderr,"[bomber-kings] Super Bomberman 4 image: %s\n",Sb4RomPath());
        else fprintf(stderr,"[bomber-kings] no verified Super Bomberman 4 image found; "
                            "place your own copy next to the Super Bomberman 5 ROM or in roms/\n");
    }
    return ready;
}
static void reset_plugin(void) { wanted=false; }
static void activate(void) { wanted=true; }
SNES_MOD_CONSTRUCTOR(register_bomber_kings) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.bomber-kings.characters",activate);
}
