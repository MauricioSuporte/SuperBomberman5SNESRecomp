#include "mod_runtime.h"
#include <stdio.h>
static SNESModActivationCallback activation, reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn; return 1; }
int snes_mod_register_activation_plugin(const char *id, SNESModActivationCallback fn) { (void)id; activation=fn; return 1; }
#include <stdbool.h>
static bool rush;
bool BossRushActive(void) { return rush; }
#include "../src/infinite_time_mod.c"
uint8 g_ram[0x20000];
void (*g_interp_bridge_pc_hook)(uint32_t,int,int);
static unsigned calls;
static void other(uint32_t pc,int m,int x) { (void)pc; (void)m; (void)x; ++calls; }
#define CHECK(v) do { if (!(v)) { fprintf(stderr,"line %d\n",__LINE__); return 1; } } while(0)
int main(void) {
    g_interp_bridge_pc_hook=other;
    InfiniteTimeBeforeFrame(); CHECK(g_interp_bridge_pc_hook==other);
    activation(); InfiniteTimeBeforeFrame();
    for (unsigned mode=0; mode<256; ++mode) {
        g_ram[0x1031]=mode; g_ram[0xe1d]=1; g_ram[0xe1e]=0;
        timer_hook(0xc50460,1,1);
        CHECK((g_ram[0xe1d]==99)==(mode==8 || (mode>=13 && mode<116)));
    }
    g_ram[0x1031]=13; g_ram[0x1d1a]=1; g_ram[0xe1d]=1;
    timer_hook(0xc50bef,1,1); CHECK(g_ram[0xe1d]==1);
    g_ram[0x1d1a]=0; timer_hook(0xc50000,1,1); CHECK(g_ram[0xe1d]==1);
    timer_hook(0xc50400,1,1); CHECK(g_ram[0xe1d]==99 && g_ram[0xe1e]==9);
    rush=true; g_ram[0xe1d]=1; timer_hook(0xc50400,1,1); CHECK(g_ram[0xe1d]==1); /* Boss Rush: off */
    rush=false; calls-=1;
    CHECK(calls==259);
    InfiniteTimeReset(); CHECK(g_interp_bridge_pc_hook==other);
    InfiniteTimeBeforeFrame(); CHECK(g_interp_bridge_pc_hook==timer_hook);
    reset_callback(); CHECK(g_interp_bridge_pc_hook==other);
    InfiniteTimeBeforeFrame(); CHECK(g_interp_bridge_pc_hook==other);
    puts("Infinite time guards, chaining and reset passed."); return 0;
}
