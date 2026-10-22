#include "mod_runtime.h"
#include "snes/interp_bridge.h"
#include "snes/saveload.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static SNESModActivationCallback activation,reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback f) { reset_callback=f;return 1; }
int snes_mod_register_activation_plugin(const char *id,SNESModActivationCallback f) { (void)id;activation=f;return 1; }
static unsigned redirect_pc,registrations;
bool interp_bridge_add_pre_opcode_hook(uint32_t pc,InterpPreOpcodeHook f) { (void)pc;(void)f;++registrations;return true; }
void interp_bridge_pre_opcode_redirect(uint32_t pc) { redirect_pc=pc; }
#include "../src/louie_mod.c"
uint8 g_ram[0x20000];
static uint8 cartridge[0x400000];
uint8_t *SnesRomPtr(uint32_t pc) { return cartridge+(pc&0x3fffff); }
typedef struct { SaveLoadInfo base;uint8 bytes[512];size_t at,end;bool reading; } Stream;
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
static void actor(unsigned a,unsigned object,unsigned cell) {
    g_ram[0x17fa+a]=(uint8_t)object;g_ram[0x15a2+a]=0;
    set_position(a,(int)(cell%16)*16-8,(int)(cell/16)*16-8);
    g_ram[0x800+object]=1;
}
static void scene(unsigned count,unsigned type) {
    memset(g_ram,0,sizeof(g_ram));memset(&state,0,sizeof(state));
    memset(g_ram+0x17fa,255,24);
    /* Mounts: stock rider + 2; five-seat co-op rider + 19 (enemies 5-18). */
    unsigned base=count>2?19:count;
    g_ram[0x1031]=13;g_ram[0xaee]=1;g_ram[0x1cbf]=(uint8_t)(count>2?5:count);
    if(count>2) g_ram[0x11e]=19;
    g_ram[0x1d25]=(uint8_t)((1u<<count)-1);
    for(unsigned a=0;a<count;++a) {
        actor(a,a*2,0x33+a);actor(a+base,20+a*2,0x33+a);
        g_ram[0x16c2+a+base]=(uint8_t)type;g_ram[0x1782+a]=0x80;g_ram[0x1632+a]=1;
    }
}
int main(void) {
    memset(cartridge,255,sizeof(cartridge));*rom(limits[0])=*rom(limits[1])=5;
    LouieBeforeFrame();CHECK(!installed);
    activation();LouieBeforeFrame();CHECK(installed && registrations==11);
    CHECK(*rom(limits[0])==6 && *rom(limits[1])==6);
    CpuState cpu={0};
    for(unsigned count=2;count<=5;count+=3) for(unsigned a=0;a<count;++a) {
        scene(count,RED);g_ram[0x8d]=(uint8_t)a;g_ram[0x1782+a]|=4;
        unsigned enemy=5+count;actor(enemy,40,0x34+a);cpu.A=(uint16_t)(0x34+a);
        hook(&cpu,0xc45560);CHECK(state.freeze[enemy]==180 && !state.freeze[a]);
        redirect_pc=0;cpu._flag_C=1;hook(&cpu,0xc45563);CHECK(redirect_pc==0xc45576 && !red_hit);
        redirect_pc=0;hook(&cpu,0xc45563);CHECK(!redirect_pc);
        hook(&cpu,0xc45560);cpu._flag_C=0;hook(&cpu,0xc45563);CHECK(!redirect_pc && !red_hit);
        cpu.A=0xfa;hook(&cpu,0xc45560);cpu._flag_C=1;hook(&cpu,0xc45563);CHECK(!redirect_pc);
        cpu.A=(uint16_t)(0x34+a);
        g_ram[0x1d1a]=1;state.freeze[enemy]=0;hook(&cpu,0xc45560);CHECK(!state.freeze[enemy]);
        g_ram[0x1d1a]=0;hook(&cpu,0xc45560);g_ram[0x8d]=(uint8_t)enemy;
        g_ram[0xe20]=0x56;g_ram[0xe21]=0x12;redirect_pc=0;
        hook(&cpu,0xc4224f);CHECK(redirect_pc==0xc34c13 && clock_borrowed);
        hook(&cpu,0xc34c69);CHECK(!clock_borrowed && g_ram[0xe20]==0x56 && g_ram[0xe21]==0x12);
        g_ram[0x1d1d]=1;LouieAfterFrame();CHECK(state.freeze[enemy]==180);g_ram[0x1d1d]=0;
        g_ram[0x800+40]=0;for(unsigned frame=0;frame<180;++frame) LouieAfterFrame();
        CHECK(!state.freeze[enemy] && g_ram[0x800+40]==1);
    }
    scene(2,RED);g_ram[0x1031]=0x67;g_ram[0x1d1a]=0x0c;CHECK(stage());
    g_ram[0xaee]=0;CHECK(!stage());g_ram[0xaee]=1;
    g_ram[0x1031]=4;CHECK(!stage());
    scene(5,YELLOW);g_ram[0x8d]=4;g_ram[0xe40]=100;g_ram[0xe7c]=0x38;
    cpu.X=0x38;hook(&cpu,0xc45797);
    CHECK(state.bomb_owner[0]==5 && redirect_pc==0xc45b80 && *rom(0xc45b87)==0x37);
    g_ram[0xe90]=0x81;LouieAfterFrame();CHECK(state.bomb_owner[0]==5);
    g_ram[0xe90]=0;LouieAfterFrame();CHECK(!state.bomb_owner[0]);
    g_ram[0xe90]=0x41;redirect_pc=0;hook(&cpu,0xc45797);CHECK(!redirect_pc);
    scene(2,YELLOW);state.bomb_owner[0]=1;g_ram[0xe90]=0x11;
    g_ram[0x1052+0x39]=0x30;cpu.X=0;cpu.A=0x39;redirect_pc=0;
    hook(&cpu,0xc52182);CHECK(redirect_pc==0xc45be0 && collision_resuming);
    redirect_pc=0;hook(&cpu,0xc52182);CHECK(!redirect_pc && !collision_resuming);
    g_ram[0xdef]=40;hook(&cpu,0xc52182);CHECK(!redirect_pc);
    for(unsigned a=0;a<5;++a) {
        scene(5,BLACK);g_ram[0x36+a]=4;g_ram[0x1632+a]=2;
        redirect_pc=0;hook(&cpu,0xc00af9);CHECK(redirect_pc==0xc45c00);
        redirect_pc=0;hook(&cpu,0xc00af9);CHECK(!redirect_pc);
        CHECK(state.dash[a]==30 && (g_ram[0x15a2+a]&4) && g_ram[0x31+a]==4);
        cpu.X=(uint16_t)a;cpu.A=0;cpu._flag_Z=1;hook(&cpu,0xc43180);CHECK(cpu.A==1 && !cpu._flag_Z);
        unsigned enemy=10;actor(enemy,40,0x43+a);LouieBeforeFrame();CHECK(state.push[enemy]==16);
        int y=position(enemy,true);LouieBeforeFrame();CHECK(position(enemy,true)==y+2);
        g_ram[0x1052+g_ram[0x16aa+enemy]+16]=0x80;
        for(unsigned frame=0;frame<20;++frame) { LouieBeforeFrame();LouieAfterFrame(); }
        CHECK(position(enemy,true)<y+32);
    }
    scene(2,BLACK);g_ram[0x8d]=0;g_ram[0x1782]=2;g_ram[0x9f]=BLACK;
    redirect_pc=0;cpu.A=0x022e;
    hook(&cpu,0xc45168);CHECK(!redirect_pc && cpu.A==0x022e);
    CHECK(rom(0xc45c00)[7]==0x35 && rom(0xc45c00)[8]==2);
    CHECK(rom(0xc45c00)[21]==0xfa && rom(0xc45c00)[22]==0x7a && rom(0xc45c00)[23]==0x6b);
    scene(2,BLACK);cpu.X=0;unsigned distance=0;
    for(unsigned remaining=30;remaining;--remaining) {
        state.dash[0]=(uint8_t)remaining;cpu.A=4;hook(&cpu,0xc43180);
        CHECK(cpu.A>=1 && cpu.A<=2 && !cpu._flag_Z);distance+=cpu.A;
    }
    CHECK(distance==48);
    for(unsigned owner=0;owner<5;++owner) for(unsigned battle_mode=0;battle_mode<2;++battle_mode) {
        scene(5,BLACK);g_ram[0x1d1a]=(uint8_t)battle_mode;
        if(battle_mode) g_ram[0x1d25]=1; /* Battle HUD mask is not a player roster. */
        unsigned target=(owner+1)%5;
        actor(owner,owner*2,0x43);actor(target,target*2,0x44);
        state.dash[owner]=owner==4?2:30;state.direction[owner]=1;
        LouieBeforeFrame();CHECK(state.push[target]==16 && state.impact[owner]==16 && state.dash[owner]>=state.push[target]);
        unsigned cell=g_ram[0x16aa+target];g_ram[0x1152+cell]=8;
        CHECK(move(target,1,2));
    }
    scene(5,RED);actor(10,40,0x45);freeze(10,180);g_ram[0x800+40]=0;g_ram[0x15a2+10]=0x80;
    LouieBeforeFrame();CHECK(!state.freeze[10] && g_ram[0x800+40]==1);
    actor(10,40,0x45);freeze(10,1);g_ram[0x800+40]=0;g_ram[0x15a2+10]=0x80;
    LouieAfterFrame();CHECK(!state.freeze[10] && g_ram[0x800+40]==1);
    actor(10,40,0x45);freeze(10,180);g_ram[0x800+40]=0;
    Stream disabled_stream={.base.func=transfer};LouieSave(&disabled_stream.base);
    disabled_stream.reading=true;disabled_stream.at=0;enabled=false;
    LouieLoad(&disabled_stream.base,0);CHECK(!state.freeze[10] && g_ram[0x800+40]==1);enabled=true;
    Stream stream={.base.func=transfer};LouieSave(&stream.base);
    unsigned duration=state.dash[0];memset(&state,0,sizeof(state));
    stream.at=0;stream.reading=true;LouieLoad(&stream.base,0);CHECK(!failed && state.dash[0]==duration);
    stream.bytes[4]=255;stream.bytes[5]=255;stream.at=0;LouieLoad(&stream.base,0);CHECK(failed);
    failed=false;stream.end=stream.at=0;LouieLoad(&stream.base,0);CHECK(!failed && !state.freeze[0]);
    reset_callback();CHECK(!enabled && !installed && *rom(limits[0])==5);
    for(unsigned i=0;i<160;++i) CHECK(rom(0xc45b80)[i]==255);
    puts("Louie input, five seats, native Clock, bomb ownership, collision, states and restoration passed.");
    return 0;
}
