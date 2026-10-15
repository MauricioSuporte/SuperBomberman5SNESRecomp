/* Native tests for the B + direction special abilities: input rule, remote
 * detonation, riding, debuff speed, damage rules (Normal, Free-for-All, Team
 * Battle) and blocking. WRAM only; no cartridge data. */
#include "common_rtl.h"
#include "snes/interp_bridge.h"
#include <stdio.h>
#include <string.h>
uint8 g_ram[0x20000];
static unsigned choices[5];
static bool installed=true;
unsigned ExtraCharacterSeatChoice(unsigned seat) { return seat<5?choices[seat]:0; }
bool HoneyKotetsuInstalled(void) { return installed; }
static unsigned emitted;
bool ExtraEmitPixels(int x,int y,const uint8_t *pixels,unsigned w,unsigned h,const uint16_t colors[16],unsigned priority) {
    (void)x;(void)y;(void)pixels;(void)w;(void)h;(void)colors;(void)priority;++emitted;return true;
}
static InterpPreOpcodeHook hooks[2];static uint32_t hook_pcs[2];static unsigned hook_count;
bool interp_bridge_add_pre_opcode_hook(uint32_t pc24,InterpPreOpcodeHook hook) {
    if(hook_count<2) { hook_pcs[hook_count]=pc24;hooks[hook_count++]=hook; }
    return true;
}
#include "../src/special_abilities.c"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static CpuState cpu;
/* One frame of the pad reader: held actions/directions and their new bits. */
static uint8 last_actions[5],last_dirs[5];
static void frame(unsigned seat,uint8 actions,uint8 dirs) {
    for(unsigned i=0;i<5;++i) {
        uint8 a=i==seat?actions:0,d=i==seat?dirs:0;
        g_ram[0x2c+i]=a;g_ram[0x36+i]=(uint8)(a&~last_actions[i]);
        g_ram[0x31+i]=d;g_ram[0x3b+i]=(uint8)(d&~last_dirs[i]);
        last_actions[i]=a;last_dirs[i]=d;
    }
    pad_hook(&cpu,PAD_HOOK);
}
static void arena(void) {
    memset(g_ram,0,sizeof(g_ram));memset(last_actions,0,5);memset(last_dirs,0,5);
    g_ram[0x1031]=0x74;g_ram[0x1d1a]=1;g_ram[0x0aee]=1;          /* Battle, playing */
    for(unsigned y=0;y<13;++y) for(unsigned x=0;x<16;++x)
        g_ram[0x1052+y*16+x]=(x==0 || y==0 || x>=14 || y==12)?0x80:0;
    memset(g_ram+0x17fa,0xff,24);
    for(unsigned i=0;i<5;++i) {                                      /* players in row 1 */
        g_ram[0x17fa+i]=(uint8)(i*2);g_ram[0x15a2+i]=0x18;
        g_ram[0x182a+i]=(uint8)(16+i*48);g_ram[0x1872+i]=16;
    }
}
static unsigned speed(unsigned seat,unsigned steps) {
    cpu.X=(uint16)seat;cpu.A=(uint16)(0x1200|steps);cpu._flag_Z=steps==0;
    speed_hook(&cpu,SPEED_HOOK);
    return cpu.A&0xff;
}
int main(void) {
    SpecialAbilitiesInstall();
    CHECK(hook_count==2 && hook_pcs[0]==PAD_HOOK && hook_pcs[1]==SPEED_HOOK);
    /* Honey (seat 0), Kotetsu (1), SB4 Bomberman (2, no ability), Bazooka (3), Great (4). */
    choices[0]=11;choices[1]=12;choices[2]=13;choices[3]=17;choices[4]=18;
    arena();
    /* B + direction in the same frame: ability, B consumed (no detonation). */
    frame(0,0x08,0x01);
    CHECK(seat_state(0)[4]==AB_SHOT && seat_state(0)[6]==0 && !(g_ram[0x36]&0x08));
    CHECK(SpecialAbilityPose(0)==0);
    /* While it recovers, B stays native. */
    for(unsigned k=0;k<60;++k) frame(0,0,0);
    CHECK(!seat_state(0)[4] && get16(seat_state(0))>0 && SpecialAbilityDebuffed(0));
    frame(0,0x08,0x01);CHECK((g_ram[0x36]&0x08) && !seat_state(0)[4]);
    /* Debuff: half the steps, alternating; zero clears Z for the BEQ. */
    unsigned total=0;
    for(unsigned k=0;k<10;++k) total+=speed(0,1);
    CHECK(total==5 && (cpu.A&0xff00)==0x1200);
    CHECK(speed(0,1)==0 && cpu._flag_Z==1 && speed(0,1)==1 && cpu._flag_Z==0);
    CHECK(speed(2,1)==1); /* no ability: untouched */
    for(unsigned k=0;k<DEBUFF_FRAMES;++k) frame(0,0,0);
    CHECK(!SpecialAbilityDebuffed(0) && speed(0,1)==1);
    /* B alone: held back, then released unchanged when no direction comes. */
    arena();
    frame(1,0x08,0);CHECK(!(g_ram[0x37]&0x08) && g_ram[PENDING+1]==B_WINDOW);
    for(unsigned k=1;k<B_WINDOW;++k) { frame(1,0x08,0);CHECK(!(g_ram[0x37]&0x08)); }
    frame(1,0x08,0);CHECK((g_ram[0x37]&0x08) && !seat_state(1)[4]);
    /* B released quickly: native B on the release frame. */
    arena();frame(1,0x08,0);frame(1,0,0);CHECK((g_ram[0x37]&0x08) && !seat_state(1)[4]);
    /* Hold B, then press a direction: ability, no detonation. */
    arena();frame(1,0x08,0);frame(1,0x08,0);frame(1,0x08,0x04);
    CHECK(seat_state(1)[4]==AB_SLASH && seat_state(1)[6]==1 && !(g_ram[0x37]&0x08) && !g_ram[PENDING+1]);
    /* Direction held, then B: ability in that direction. */
    arena();frame(1,0,0x02);frame(1,0x08,0x02);CHECK(seat_state(1)[4]==AB_SLASH && seat_state(1)[6]==2);
    /* Riding Louie: B + direction is native. */
    arena();g_ram[0x1782+1]=0x80;frame(1,0x08,0x02);CHECK(!seat_state(1)[4] && (g_ram[0x37]&0x08));
    /* No ability (SB4 Bomberman): native. */
    arena();frame(2,0x08,0x02);CHECK(!seat_state(2)[4] && (g_ram[0x38]&0x08));
    /* Paused or outside a stage: nothing. */
    arena();g_ram[0x1d1d]=1;frame(1,0x08,0x02);CHECK(!seat_state(1)[4]);
    arena();g_ram[0x1031]=4;frame(1,0x08,0x02);CHECK(!seat_state(1)[4]);
    /* Damage, Free-for-All: Bazooka (seat 3, x 160) fires left along row 1:
     * every other player in the row is hit, the shooter is not. */
    arena();frame(3,0x08,0x02);
    CHECK(seat_state(3)[4]==AB_FIRE);
    frame(3,0,0); /* effects run from the next frame */
    for(unsigned i=0;i<3;++i) CHECK(g_ram[0x15a2+i]&0x40);
    CHECK(!(g_ram[0x15a2+3]&0x40) && !(g_ram[0x15a2+4]&0x40));
    /* Invulnerable players ($15A2 bit 2) are spared. */
    arena();g_ram[0x15a2+2]|=0x04;frame(3,0x08,0x02);frame(3,0,0);CHECK(!(g_ram[0x15a2+2]&0x40) && (g_ram[0x15a2+1]&0x40));
    /* Hard blocks stop the flames. */
    arena();g_ram[0x1052+16+6]=0x80;frame(3,0x08,0x02);frame(3,0,0);
    CHECK(!(g_ram[0x15a2+1]&0x40) && !(g_ram[0x15a2]&0x40) && (g_ram[0x15a2+2]&0x40) && seat_state(3)[7]==3);
    /* Team Battle: teammates (same $15A2 bits 3-4) are spared. */
    arena();g_ram[0x625d]=1;
    g_ram[0x15a2]=0x10;g_ram[0x15a2+1]=0x08;g_ram[0x15a2+2]=0x10;g_ram[0x15a2+3]=0x10;g_ram[0x15a2+4]=0x08;
    frame(3,0x08,0x02);frame(3,0,0);
    CHECK(!(g_ram[0x15a2]&0x40) && (g_ram[0x15a2+1]&0x40) && !(g_ram[0x15a2+2]&0x40));
    /* Normal Game: campaign seats ($1D25) are never hit; enemies lose one
     * point and get 180 frames of invulnerability, or die on the last. */
    arena();g_ram[0x1d1a]=0;g_ram[0x1031]=0x0d;g_ram[0x1d25]=0x1f;
    g_ram[0x17fa+6]=0x20;g_ram[0x182a+6]=64;g_ram[0x1872+6]=16;g_ram[0x1662+6]=2;g_ram[0x15a2+6]=0;
    g_ram[0x17fa+7]=0x22;g_ram[0x182a+7]=96;g_ram[0x1872+7]=16;g_ram[0x1662+7]=1;g_ram[0x15a2+7]=0;
    frame(3,0x08,0x02);frame(3,0,0);
    for(unsigned i=0;i<5;++i) CHECK(!(g_ram[0x15a2+i]&0x40));
    CHECK(g_ram[0x1662+6]==1 && g_ram[0x85f+0x20]==0xb4 && !(g_ram[0x15a2+6]&0x40));
    CHECK(g_ram[0x1662+7]==0 && (g_ram[0x15a2+7]&0x40));
    frame(3,0,0);CHECK(g_ram[0x1662+6]==1); /* invulnerable for now */
    /* A rival bomber in a non-campaign seat slot is an enemy in Normal. */
    arena();g_ram[0x1d1a]=0;g_ram[0x1031]=0x0d;g_ram[0x1d25]=0x09;
    frame(3,0x08,0x02);frame(3,0,0);CHECK(g_ram[0x15a2+1]&0x40 && !(g_ram[0x15a2]&0x40) && g_ram[0x15a2+2]&0x40);
    /* Great: invulnerable in place (native $15A2 bit 2 + $170A), no walking. */
    arena();frame(4,0x08,0x08);
    CHECK(seat_state(4)[4]==AB_GUARD && (g_ram[0x15a2+4]&0x04) && g_ram[0x170a+4]==180 && speed(4,2)==0);
    /* Effects are drawn while active. */
    emitted=0;SpecialAbilityDraw();CHECK(emitted==1);
    /* Leaving the stage clears every seat. */
    g_ram[0x1031]=4;frame(0,0,0);
    for(unsigned b=STATE;b<STATE_END;++b) CHECK(!g_ram[b]);
    puts("special_abilities_tests: ok");
    return 0;
}
