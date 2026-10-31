#include "mod_runtime.h"
#include <stdio.h>
#include <string.h>
static SNESModActivationCallback activation, reset_callback;
static const char *plugin_id;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn; return 1; }
int snes_mod_register_activation_plugin(const char *id, SNESModActivationCallback fn) { plugin_id=id; activation=fn; return 1; }
#include <stdbool.h>
static bool rush;
bool BossRushActive(void) { return rush; }
#include "../src/com_level4_mod.c"
uint8 g_ram[0x20000];
#define CHECK(v) do { if (!(v)) { fprintf(stderr,"line %d\n",__LINE__); return 1; } } while(0)
int main(void) {
    CHECK(plugin_id && !strcmp(plugin_id,"superbomberman5.battle.com-level-4"));
    g_ram[0x6247]=3; ComLevel4BeforeFrame(); CHECK(g_ram[0x6247]==3); /* disabled: stock limit */
    activation(); CHECK(ComLevel4Enabled());
    ComLevel4BeforeFrame(); CHECK(g_ram[0x6247]==4);
    g_ram[0x6247]=0; ComLevel4BeforeFrame(); CHECK(g_ram[0x6247]==4);   /* cleared RAM at boot */
    g_ram[0x6247]=4; ComLevel4BeforeFrame(); CHECK(g_ram[0x6247]==4);   /* password save: unchanged */
    g_ram[0x6246]=0x11; g_ram[0x6248]=0x22; g_ram[0x625b]=0x33;
    g_ram[0x6247]=3; ComLevel4BeforeFrame();
    CHECK(g_ram[0x6246]==0x11 && g_ram[0x6248]==0x22 && g_ram[0x625b]==0x33); /* neighbours untouched */
    rush=true; g_ram[0x6247]=3; ComLevel4BeforeFrame(); CHECK(g_ram[0x6247]==3); /* Boss Rush: off */
    rush=false;
    reset_callback(); CHECK(!ComLevel4Enabled());
    g_ram[0x6247]=3; ComLevel4BeforeFrame(); CHECK(g_ram[0x6247]==3);
    puts("COM level 4 limit, password save, neighbours and reset passed."); return 0;
}
