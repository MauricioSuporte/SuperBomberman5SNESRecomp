#include "mod_runtime.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static SNESModActivationCallback activation,reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn;return 1; }
int snes_mod_register_activation_plugin(const char *id,SNESModActivationCallback fn) { (void)id;activation=fn;return 1; }
#include "../src/gold_battle_mod.c"
uint8 g_ram[0x20000];
static uint8 cartridge[0x400000];
uint8_t *SnesRomPtr(uint32_t pc) { return cartridge+(pc&0x3fffff); }

void (*g_interp_bridge_pc_hook)(uint32_t,int,int);
typedef struct { SaveLoadInfo base;uint8 bytes[128];size_t at,end;bool reading; } Stream;
static bool failed;
static void transfer(SaveLoadInfo *sli,void *data,size_t n) {
    Stream *s=(Stream *)sli;
    if(s->at+n>sizeof(s->bytes) || (s->reading && s->at+n>s->end)) { failed=true;return; }
    if(s->reading) memcpy(data,s->bytes+s->at,n);
    else { memcpy(s->bytes+s->at,data,n);s->end=s->at+n; }
    s->at+=n;
}
size_t RtlStateBytesRemaining(SaveLoadInfo *sli) { Stream *s=(Stream *)sli;return s->end-s->at; }
void RtlStateStreamFail(SaveLoadInfo *sli) { (void)sli;failed=true; }
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static unsigned chained;
static void other(uint32_t pc,int m,int x) { (void)pc;(void)m;(void)x;++chained; }
/* Execute the authored rule program with a small VM oracle. Runtime tests also
 * execute these exact bytes through the cartridge's own bytecode interpreter. */
static unsigned rule(unsigned value,bool right) {
    uint32_t pc=right?0xc73000:0xc73080;unsigned acc=0;bool eq=false;
    g_ram[0x625d]=(uint8_t)value;
    for(unsigned steps=0;steps<64;++steps) {
        uint8_t op=*rom(pc++);
        if(op==0x5d) { pc+=2;acc=g_ram[0x625d]; }
        else if(op==0x6d) acc&=*rom(pc++);
        else if(op==0x86) eq=acc==*rom(pc++);
        else if(op==0x49) { unsigned n=*rom(pc++);if(!eq) pc+=n; }
        else if(op==0) return g_ram[0x625d];
        else if(op==0x45) { int8_t n=(int8_t)*rom(pc++);pc+=n; }
        else if(op==0xff) {
            unsigned fn=*rom(pc++);
            if(fn==0x60) { unsigned mask=*rom(pc++),set=*rom(pc++);if(set) g_ram[0x625d]|=mask;else g_ram[0x625d]&=~mask; }
            else if(fn==0x8e) pc+=5;
            else return 255;
        } else if(op==0x42) pc+=2;
        else return 255;
    }
    return 255;
}
int main(void) {
    memset(cartridge,255,sizeof(cartridge));activation();GoldBattleBeforeFrame();CHECK(!installed);
    *rom(0xc048c4)=10;
    const uint8_t prefix[]={0x5d,0x5d,0x62,0x6d,0x10,0x8a},right[]={0x5d,0x5d,0x62,0x6d,0x18,0x86},drop[]={0x20,0xcd,0x37};
    memcpy(rom(0xc69175),prefix,6);memcpy(rom(0xc692d8),right,6);memcpy(rom(0xc436e8),drop,3);
    g_interp_bridge_pc_hook=other;GoldBattleBeforeFrame();CHECK(installed && *rom(0xc048c4)==13 && g_ram[0x6245]==13);
    g_ram[0x1031]=4;g_ram[0x625a]=11;g_ram[0xc90]=0x8d;g_ram[0xcb8]=0xa2;g_ram[0xce0]=0xc6;
    CHECK(GoldBattleArena()==11);g_ram[0x1031]=0x9c;CHECK(!GoldBattleArena());
    g_ram[0x1031]=4;g_ram[0xce0]=0;CHECK(!GoldBattleArena());
    const unsigned modes[]={0,8,16,80};
    for(unsigned i=0;i<4;++i) {
        CHECK(rule(modes[i]|0x27,true)==(modes[i<3?i+1:3]|0x27));
        CHECK(rule(modes[i]|0x27,false)==(modes[i?i-1:0]|0x27));
    }
    for(unsigned killer=0;killer<5;++killer) for(unsigned victim=0;victim<5;++victim) if(killer!=victim) {
        memset(victims,0,5);g_ram[0x625d]=80;g_ram[0x1d1a]=1;
        g_ram[0x8d]=victim;
        for(unsigned f=0;f<5;++f) g_ram[fields[f]+victim]=(uint8_t)(10+f);
        instruction(0xc4365a,1,1);g_ram[0xa1]=killer;instruction(0xc43791,1,1);
        CHECK(victims[victim]==killer+1);
        instruction(0xc436e6,1,1);CHECK(*rom(0xc436e8)==0xea);
        instruction(0xc436eb,1,1);CHECK(!memcmp(rom(0xc436e8),drop,3));
        g_ram[0x8d]=killer;instruction(0xc42b37,1,1);
        for(unsigned f=0;f<5;++f) CHECK(g_ram[fields[f]+killer]==10+f);
        CHECK(!victims[victim]);
    }
    g_ram[0x625d]=16;g_ram[0x8d]=2;g_ram[0xa1]=3;instruction(0xc43791,1,1);CHECK(!victims[2]);
    Stream stream={.base.func=transfer};victims[0]=2;GoldBattleSave(&stream.base);
    victims[0]=0;stream.reading=true;stream.at=0;GoldBattleLoad(&stream.base,0);CHECK(!failed && victims[0]==2);
    drop_suppressed=1;stream.reading=false;stream.at=0;GoldBattleSave(&stream.base);
    drop_suppressed=0;stream.reading=true;stream.at=0;GoldBattleLoad(&stream.base,0);
    CHECK(!failed && drop_suppressed==1 && *rom(0xc436e8)==0xea);
    instruction(0xc436eb,1,1);CHECK(!drop_suppressed && *rom(0xc436e8)==0x20);
    stream.bytes[4]=7;stream.at=0;GoldBattleLoad(&stream.base,0);CHECK(failed);
    failed=false;stream.at=stream.end=0;GoldBattleLoad(&stream.base,0);CHECK(!failed && !victims[0]);
    reset_callback();CHECK(g_ram[0x6245]==10);
    CHECK(!installed && !enabled && *rom(0xc048c4)==10 && *rom(0xc73000)==255);
    CHECK(!memcmp(rom(0xc69175),prefix,6) && !memcmp(rom(0xc692d8),right,6));
    CHECK(g_interp_bridge_pc_hook==other && chained);
    puts("Gold rules, all 20 Revenge pairs, equipment transfer, states and restoration passed.");return 0;
}
