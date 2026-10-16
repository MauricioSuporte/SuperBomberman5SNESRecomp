/* Bomber Kings: Super Bomberman 4's Bomberman, Hammer, Jet, Lady, Bazooka
 * and Great Bomber as playable characters.
 *
 * This plugin only switches the extra-character core (extra_characters_mod.c)
 * to its three-row picker and enables the Super Bomberman 4 skins; their
 * frames and palettes are read at runtime from the player's own Super
 * Bomberman 4 image (sb4_rom.c). Without a verified image the picker keeps
 * the layout of the Super Bomberman 5 extra characters mod (or the stock
 * one) and nothing else changes. Their special
 * abilities (B + direction) live in special_abilities.c. */
#include "bomber_kings_mod.h"
#include "sb4_rom.h"
#include "mod_runtime.h"
#include <stdio.h>

static bool wanted,reported;
bool BomberKingsWanted(void) { return wanted; }
bool BomberKingsReady(void) {
    if(!wanted) return false;
    /* The ROM the player selected for this mod on the launcher's Mods page
     * (manifest [[external_rom]] "sb4-rom", hash checked by the engine). */
    char path[1024];
    if(snes_mod_runtime_resource_path_c("superbomberman5.bomber-kings","bomber_kings","sb4-rom",path,sizeof(path)))
        Sb4RomLoad(path);
    bool ready=Sb4Rom()!=NULL;
    if(!reported) {
        reported=true;
        if(ready) fprintf(stderr,"[bomber-kings] Super Bomberman 4 image: %s\n",Sb4RomPath());
        else fprintf(stderr,"[bomber-kings] no verified Super Bomberman 4 image found; "
                            "select it for Bomber Kings on the launcher's Mods page\n");
    }
    return ready;
}
static void reset_plugin(void) { wanted=false; }
static void activate(void) { wanted=true; }
SNES_MOD_CONSTRUCTOR(register_bomber_kings) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.bomber-kings.characters",activate);
}
