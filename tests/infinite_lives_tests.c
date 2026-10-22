#include "mod_runtime.h"
#include <stdio.h>
static SNESModActivationCallback activation, reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn; return 1; }
int snes_mod_register_activation_plugin(const char *id, SNESModActivationCallback fn) { (void)id; activation=fn; return 1; }
#include "../src/infinite_lives_mod.c"
uint8 g_ram[0x20000];
void (*g_interp_bridge_pc_hook)(uint32_t,int,int);
static unsigned calls;
/* Stands in for the co-op hook (wrapped by the lives hook): it charges two
 * extra seats at the stage-retry entry, like normal_coop_mod.c does. */
static void coop(uint32_t pc,int m,int x) { (void)m; (void)x; ++calls; if (pc==0xc53b1c) g_ram[0x1d23]-=2; }
#define CHECK(v) do { if (!(v)) { fprintf(stderr,"line %d\n",__LINE__); return 1; } } while(0)
/* Emulates the native retry: hook per instruction, the DECs between them. */
static void retry(unsigned participants) {
    g_interp_bridge_pc_hook(0xc53b1c,1,1);
    g_interp_bridge_pc_hook(0xc53b23,1,1); if (participants&1) --g_ram[0x1d23];
    g_interp_bridge_pc_hook(0xc53b2a,1,1); if (participants&2) --g_ram[0x1d23];
    g_interp_bridge_pc_hook(0xc53b2d,1,1);
}
static void respawn(void) {
    g_interp_bridge_pc_hook(0xc50659,1,1); --g_ram[0x1d23];
    g_interp_bridge_pc_hook(0xc5065c,1,1);
}
int main(void) {
    g_interp_bridge_pc_hook=coop;
    InfiniteLivesBeforeFrame(); CHECK(g_interp_bridge_pc_hook==coop);
    g_ram[0x1031]=0x0d; g_ram[0x1d23]=5;
    retry(3); CHECK(g_ram[0x1d23]==1);           /* disabled: stock charges */
    activation(); InfiniteLivesBeforeFrame(); CHECK(g_interp_bridge_pc_hook==lives_hook);
    CHECK(InfiniteLivesEnabled());
    g_ram[0x1d23]=5; retry(3); CHECK(g_ram[0x1d23]==5);
    g_ram[0x1d23]=0; retry(1); CHECK(g_ram[0x1d23]==0); /* no Game Over underflow */
    g_ram[0x1d23]=3; respawn(); CHECK(g_ram[0x1d23]==3);
    for (unsigned mode=0; mode<256; ++mode) {
        g_ram[0x1031]=(uint8_t)mode; g_ram[0x1d1a]=0; g_ram[0x1d23]=4; respawn();
        CHECK((g_ram[0x1d23]==4)==(mode==8 || (mode>=13 && mode<116)));
    }
    g_ram[0x1031]=0x0d; g_ram[0x1d1a]=1; g_ram[0x1d23]=4; respawn(); CHECK(g_ram[0x1d23]==3); /* Battle */
    g_ram[0x1d1a]=0; g_ram[0x1d23]=7; g_interp_bridge_pc_hook(0xc5065c,1,1); CHECK(g_ram[0x1d23]==7); /* no stale restore */
    CHECK(calls>0);
    InfiniteLivesReset(); CHECK(g_interp_bridge_pc_hook==coop);
    InfiniteLivesBeforeFrame(); CHECK(g_interp_bridge_pc_hook==lives_hook);
    reset_callback(); CHECK(g_interp_bridge_pc_hook==coop && !InfiniteLivesEnabled());
    InfiniteLivesBeforeFrame(); CHECK(g_interp_bridge_pc_hook==coop);
    puts("Infinite lives retry, respawn, co-op charge, guards and reset passed."); return 0;
}
