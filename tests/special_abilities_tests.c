/* Native tests for the B + direction special abilities: input rule, remote
 * detonation, riding, timelines (poses, freezing, Jet's flight), debuff
 * speed, damage rules (Normal, Free-for-All, Team Battle), stun and item
 * loss. WRAM only; Super Bomberman 4 graphics are stubbed. */
#include "common_rtl.h"
#include "snes/interp_bridge.h"
#include <stdio.h>
#include <string.h>
uint8 g_ram[0x20000];
static uint8 rom_image[0x400000];
const uint8 *g_rom=rom_image;
static unsigned choices[5];
static bool installed=true;
unsigned ExtraCharacterSeatChoice(unsigned seat) { return seat<5?choices[seat]:0; }
unsigned ExtraCharacterSeatColor(unsigned seat) { (void)seat;return 0; }
bool ExtraCharactersInstalled(void) { return installed; }
bool Sb5ExtrasEnabled(void) { return true; }
static unsigned stage_sounds;static uint32_t stage_sound;
void Sb5StageSoundPlay(unsigned set,uint16_t effect) { ++stage_sounds;stage_sound=set<<16|effect; }
void Sb5StageSoundPrepare(unsigned set,uint16_t effect) { (void)set;(void)effect; }
int snes_frame_counter;
static unsigned tints;static uint16_t tint_color;static int tint_box[4];
bool ExtraTintBox(int left,int top,int right,int bottom,uint16_t color) {
    tint_box[0]=left;tint_box[1]=top;tint_box[2]=right;tint_box[3]=bottom;tint_color=color;++tints;return true;
}
static unsigned emitted;
bool ExtraEmitPixels(int x,int y,const uint8_t *pixels,unsigned w,unsigned h,const uint16_t colors[16],unsigned priority) {
    (void)x;(void)y;(void)pixels;(void)w;(void)h;(void)colors;(void)priority;++emitted;return true;
}
bool Sb4FireBlock(unsigned code,uint8_t out[16][16]) { memset(out,code&15,256);return true; }
uint16_t Sb4FireColor(unsigned index) { return (uint16_t)index; }
bool Sb4EffectBlock(unsigned tile,uint8_t out[16][16]) { memset(out,1,256);return tile>=0x80; }
uint16_t Sb4Color(unsigned character,unsigned palette,unsigned index) { return (uint16_t)(character+palette+index); }
uint32_t Sb4FramePc(unsigned character,unsigned frame) { return 0xd00000|character<<8|frame; }
static unsigned sounds[128],skin_frames;
bool ExtraEmitSkinFrame(unsigned skin,unsigned anim,unsigned step,int x,int y) { (void)skin;(void)anim;(void)step;(void)x;(void)y;++skin_frames;return true; }
void Sb4SoundPlay(uint8_t id) { ++sounds[id&0x7f]; }
static InterpPreOpcodeHook hooks[32];static uint32_t hook_pcs[32];static unsigned hook_count;
bool interp_bridge_add_pre_opcode_hook(uint32_t pc24,InterpPreOpcodeHook hook) {
    if(hook_count<32) { hook_pcs[hook_count]=pc24;hooks[hook_count++]=hook; }
    return true;
}
static uint32_t redirected;
void interp_bridge_pre_opcode_redirect(uint32_t pc24) { redirected=pc24; }
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
static void idle(unsigned frames) { for(unsigned k=0;k<frames;++k) frame(0,0,0); }
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
    cpu.S=0x1ff;cpu.A=0x1234;cpu.X=0x05;cpu.Y=0x07;
}
static unsigned speed(unsigned seat,unsigned steps) {
    cpu.X=(uint16)seat;cpu.A=(uint16)(0x1200|steps);cpu._flag_Z=steps==0;
    speed_hook(&cpu,SPEED_HOOK);
    return cpu.A&0xff;
}
static unsigned timer(unsigned seat) { return get16(seat_state(seat)+S_TIMER); }
int main(void) {
    /* The kill stores are hooked only where the image holds STA $15A2,X/Y. */
    for(unsigned k=0;k<sizeof(kKillStores)/sizeof(*kKillStores);++k) {
        unsigned at=((kKillStores[k]>>16)&0x3f)<<16|(kKillStores[k]&0xffff);
        if(k) { rom_image[at]=kKillStores[k]>=0xc02000 && k>=9?0x9d:0x99;rom_image[at+1]=0xa2;rom_image[at+2]=0x15; }
    }
    SpecialAbilitiesInstall();
    CHECK(hook_count==2+21 && hook_pcs[0]==PAD_HOOK && hook_pcs[1]==SPEED_HOOK && hook_pcs[2]==kKillStores[1]);
    unsigned step;uint32_t pc;
    /* Honey (seat 0), Kotetsu (1), SB4 Bomberman (2, no ability), Bazooka (3), Great (4). */
    choices[0]=11;choices[1]=12;choices[2]=13;choices[3]=17;choices[4]=18;
    arena();
    /* B + direction in the same frame: ability, B consumed (no detonation). */
    frame(0,0x08,0x01);
    CHECK(seat_state(0)[S_KIND]==AB_SHOT && seat_state(0)[S_DIR]==0 && !(g_ram[0x36]&0x08));
    CHECK(SpecialAbilityPose(0,&step,&pc)==0 && step==0 && !pc);
    /* Honey stands for her shooting animation (flash at frame 32); the
     * bullet flies 4 px per frame and kills the first target in line. */
    CHECK(speed(0,2)==0);
    idle(SHOT_AT);CHECK(!(seat_state(0)[S_FLAGS]&4) && SpecialAbilityPose(0,&step,&pc)==0 && step==3);
    frame(0,0,0);CHECK((seat_state(0)[S_FLAGS]&4) && !(g_ram[0x15a2+1]&0x40));
    skin_frames=0;SpecialAbilityDraw();CHECK(skin_frames==1);
    idle(10);CHECK(g_ram[0x15a2+1]&0x40 && !(g_ram[0x15a2+2]&0x40) && !(seat_state(0)[S_FLAGS]&4));
    idle(SHOT_FRAMES);
    CHECK(!seat_state(0)[S_KIND] && SpecialAbilityDebuffed(0) && SpecialAbilityPose(0,&step,&pc)<0);
    /* While it recovers, B stays native. */
    frame(0,0x08,0x01);CHECK((g_ram[0x36]&0x08) && !seat_state(0)[S_KIND]);
    /* Debuff: half the steps, alternating; zero clears Z for the BEQ. */
    unsigned total=0;
    for(unsigned k=0;k<10;++k) total+=speed(0,1);
    CHECK(total==5 && (cpu.A&0xff00)==0x1200);
    CHECK(speed(0,1)==0 && cpu._flag_Z==1 && speed(0,1)==1 && cpu._flag_Z==0);
    CHECK(speed(2,1)==1); /* no ability: untouched */
    idle(DEBUFF_FRAMES);
    CHECK(!SpecialAbilityDebuffed(0) && speed(0,1)==1);
    /* B alone: held back, then released unchanged when no direction comes. */
    arena();
    frame(1,0x08,0);CHECK(!(g_ram[0x37]&0x08) && g_ram[PENDING+1]==B_WINDOW);
    for(unsigned k=1;k<B_WINDOW;++k) { frame(1,0x08,0);CHECK(!(g_ram[0x37]&0x08)); }
    frame(1,0x08,0);CHECK((g_ram[0x37]&0x08) && !seat_state(1)[S_KIND]);
    /* B released quickly: native B on the release frame. */
    arena();frame(1,0x08,0);frame(1,0,0);CHECK((g_ram[0x37]&0x08) && !seat_state(1)[S_KIND]);
    /* Hold B, then press a direction: ability, no detonation. */
    arena();frame(1,0x08,0);frame(1,0x08,0);frame(1,0x08,0x04);
    CHECK(seat_state(1)[S_KIND]==AB_SLASH && seat_state(1)[S_DIR]==1 && !(g_ram[0x37]&0x08) && !g_ram[PENDING+1]);
    /* Direction held, then B: ability in that direction. Kotetsu's cut hits
     * the tile in front (seat 2 stands to his left). */
    arena();frame(1,0,0x02);frame(1,0x08,0x02);CHECK(seat_state(1)[S_KIND]==AB_SLASH && seat_state(1)[S_DIR]==2);
    for(unsigned k=0;k<SLASH_FROM+2;++k) frame(1,0,0);
    CHECK((g_ram[0x15a2]&0x40)==0); /* seat 0 is three tiles away */
    arena();g_ram[0x182a]=48;frame(1,0,0x02);frame(1,0x08,0x02);
    for(unsigned k=0;k<SLASH_FROM;++k) frame(1,0,0);
    CHECK(!(g_ram[0x15a2]&0x40));                     /* the thrust comes out at 13 */
    frame(1,0,0);
    CHECK(g_ram[0x15a2]&0x40);
    /* Riding Louie: B + direction is native. */
    arena();g_ram[0x1782+1]=0x80;frame(1,0x08,0x02);CHECK(!seat_state(1)[S_KIND] && (g_ram[0x37]&0x08));
    /* No ability (SB4 Bomberman): native. */
    arena();frame(2,0x08,0x02);CHECK(!seat_state(2)[S_KIND] && (g_ram[0x38]&0x08));
    /* Paused or outside a stage: nothing. */
    arena();g_ram[0x1d1d]=1;frame(1,0x08,0x02);CHECK(!seat_state(1)[S_KIND]);
    arena();g_ram[0x1031]=4;frame(1,0x08,0x02);CHECK(!seat_state(1)[S_KIND]);
    /* Every arena's mode ($74+4*(arena-1)+variant) is a stage. */
    arena();g_ram[0x1031]=0x84;frame(1,0x08,0x02);CHECK(seat_state(1)[S_KIND]!=0);
    arena();g_ram[0x1031]=0xa4;frame(1,0x08,0x02);CHECK(seat_state(1)[S_KIND]!=0);
    arena();g_ram[0x1031]=0xa8;frame(1,0x08,0x02);CHECK(!seat_state(1)[S_KIND]);

    /* Bazooka (seat 3, x 160) fires left along row 1. He stands still for
     * 98 frames, the aiming pose turns every 8 frames, then holds. */
    arena();frame(3,0x08,0x02);
    CHECK(seat_state(3)[S_KIND]==AB_BAZOOKA);
    CHECK(SpecialAbilityPose(3,&step,&pc)==2 && pc==0xca98b0+3*0x80);   /* left */
    for(unsigned k=0;k<8;++k) frame(3,0,0x01);
    CHECK(SpecialAbilityPose(3,&step,&pc)==2 && pc==0xca98b0);            /* up */
    CHECK(!(g_ram[0x31+3]) && speed(3,2)==0);                              /* frozen */
    while(timer(3)<BEAM_FROM) frame(3,0,0);
    CHECK(!(g_ram[0x15a2]&0x40));
    frame(3,0,0);
    /* Every other player in the beam's row is hit, the shooter is not. */
    for(unsigned i=0;i<3;++i) CHECK(g_ram[0x15a2+i]&0x40);
    CHECK(!(g_ram[0x15a2+3]&0x40) && !(g_ram[0x15a2+4]&0x40) && seat_state(3)[S_BEAM]==9);
    emitted=0;SpecialAbilityDraw();CHECK(emitted==9);
    while(seat_state(3)[S_KIND]) frame(3,0,0);
    CHECK(timer(3)==BAZOOKA_FRAMES && SpecialAbilityDebuffed(3));
    /* Invulnerable players ($15A2 bit 2) are spared; blocks stop the beam. */
    arena();g_ram[0x15a2+2]|=0x04;g_ram[0x1052+16+3]=0x80;frame(3,0x08,0x02);
    while(timer(3)<=BEAM_FROM) frame(3,0,0);
    CHECK(!(g_ram[0x15a2+2]&0x40) && (g_ram[0x15a2+1]&0x40) && !(g_ram[0x15a2]&0x40) && seat_state(3)[S_BEAM]==6);
    /* A soft block is the beam's last tile and burns with the cartridge's
     * $C5:441D (A = cell, called through a pushed RTL frame). */
    arena();g_ram[0x1052+16+3]=0x30;redirected=0;
    frame(3,0x08,0x02);while(timer(3)<=BEAM_FROM && !redirected) frame(3,0,0);
    CHECK(seat_state(3)[S_BEAM]==7 && redirected==BURN_BLOCK && injecting && (cpu.A&0xff)==0x13 && cpu.S==0x1ff-3);
    CHECK(g_ram[0x1ff]==0xc0 && g_ram[0x1fe]==0x0a && g_ram[0x1fd]==0xf8);
    cpu.S=0x1ff;redirected=0;pad_hook(&cpu,PAD_HOOK);CHECK(!injecting && cpu.A==0x1234);
    /* A bomb stops it and explodes at once (fuse $0E40 set to 1). */
    arena();g_ram[0x1052+16+5]=0x50;g_ram[0xe2a]=0;g_ram[0xe7c]=0x15;g_ram[0xe40]=100;g_ram[0xea4]=0x14;
    frame(3,0x08,0x02);while(timer(3)<=BEAM_FROM) frame(3,0,0);
    CHECK(seat_state(3)[S_BEAM]==4 && g_ram[0xe40]==1 && !(g_ram[0x15a2]&0x40) && (g_ram[0x15a2+2]&0x40));
    /* Team Battle: teammates (same $15A2 bits 3-4) are spared. */
    arena();g_ram[0x625d]=1;
    g_ram[0x15a2]=0x10;g_ram[0x15a2+1]=0x08;g_ram[0x15a2+2]=0x10;g_ram[0x15a2+3]=0x10;g_ram[0x15a2+4]=0x08;
    frame(3,0x08,0x02);while(timer(3)<=BEAM_FROM) frame(3,0,0);
    CHECK(!(g_ram[0x15a2]&0x40) && (g_ram[0x15a2+1]&0x40) && !(g_ram[0x15a2+2]&0x40));
    /* Normal Game: campaign seats ($1D25) are never hit; enemies lose one
     * point and get 180 frames of invulnerability, or die on the last. */
    arena();g_ram[0x1d1a]=0;g_ram[0x1031]=0x0d;g_ram[0x1d25]=0x1f;
    g_ram[0x17fa+6]=0x20;g_ram[0x182a+6]=64;g_ram[0x1872+6]=16;g_ram[0x1662+6]=2;g_ram[0x15a2+6]=0;
    g_ram[0x17fa+7]=0x22;g_ram[0x182a+7]=96;g_ram[0x1872+7]=16;g_ram[0x1662+7]=1;g_ram[0x15a2+7]=0;
    frame(3,0x08,0x02);while(timer(3)<=BEAM_FROM) frame(3,0,0);
    for(unsigned i=0;i<5;++i) CHECK(!(g_ram[0x15a2+i]&0x40));
    CHECK(g_ram[0x1662+6]==1 && g_ram[0x85f+0x20]==0xb4 && !(g_ram[0x15a2+6]&0x40));
    CHECK(g_ram[0x1662+7]==0 && (g_ram[0x15a2+7]&0x40));
    frame(3,0,0);CHECK(g_ram[0x1662+6]==1); /* invulnerable for now */
    /* A rival bomber in a non-campaign seat slot is an enemy in Normal. */
    arena();g_ram[0x1d1a]=0;g_ram[0x1031]=0x0d;g_ram[0x1d25]=0x09;
    frame(3,0x08,0x02);while(timer(3)<=BEAM_FROM) frame(3,0,0);
    CHECK(g_ram[0x15a2+1]&0x40 && !(g_ram[0x15a2]&0x40) && g_ram[0x15a2+2]&0x40);

    /* Great: invulnerable in place for 160 frames facing down, then debuff. */
    arena();memset(sounds,0,sizeof(sounds));frame(4,0x08,0x08);
    CHECK(sounds[SND_GUARD]==1 && seat_state(4)[S_KIND]==AB_GREAT && (g_ram[0x15a2+4]&0x04) && g_ram[0x170a+4]==GREAT_FRAMES && speed(4,2)==0);
    CHECK(SpecialAbilityPose(4,&step,&pc)==1 && pc==Sb4FramePc(5,6));
    while(seat_state(4)[S_KIND]) frame(4,0,0);
    CHECK(timer(4)==GREAT_FRAMES && SpecialAbilityDebuffed(4));

    /* Jet (seat 1, x 64): wind-up, then 4 px per frame to the right with a
     * fire trail until stopped, blinking invulnerable; after the crash he
     * keeps the flying pose for 20 frames. Debuff from the trigger. */
    choices[1]=15;arena();g_ram[0x182a]=200;memset(sounds,0,sizeof(sounds));frame(1,0x08,0x01);
    CHECK(seat_state(1)[S_KIND]==AB_JET && (g_ram[0x15a2+1]&0x04) && SpecialAbilityDebuffed(1));
    CHECK(SpecialAbilityPose(1,&step,&pc)==0 && pc==0xcad0b0+1*0x800);
    while(timer(1)<JET_WINDUP) { frame(1,0,0);CHECK(speed(1,2)==0); }
    CHECK(SpecialAbilityPose(1,&step,&pc)==0 && pc==0xcad0b0+1*0x800+3*0x80);
    unsigned x=64;
    for(unsigned k=0;k<20;++k) {
        frame(1,0,0);
        CHECK(g_ram[0x31+1]==0x01 && speed(1,1)==JET_SPEED && (g_ram[0x15a2+1]&0x04));
        x+=JET_SPEED;g_ram[0x182a+1]=(uint8)x;
    }
    CHECK(g_ram[0x15a2+2]&0x40); /* seat 2 (x 112, tile 7) was in the trail */
    /* Enemy contact and blasts do not kill him: the cartridge's kill store
     * loses bit 6 for his seat only (STA $15A2,Y at $C3:1197, STA $15A2,X at
     * $C4:42E5). */
    cpu.Y=1;cpu.A=0x45;hooks[2](&cpu,kKillStores[1]);CHECK(cpu.A==0x05);
    cpu.Y=3;cpu.A=0x45;hooks[2](&cpu,kKillStores[1]);CHECK(cpu.A==0x45);
    cpu.X=1;cpu.A=0x4c;hooks[22](&cpu,kKillStores[21]);CHECK(cpu.A==0x0c);
    /* His body hits an enemy next to him (actor 10, 8 px ahead). */
    g_ram[0x17fa+10]=0x20;g_ram[0x182a+10]=(uint8)(x+8);g_ram[0x1872+10]=16;g_ram[0x1662+10]=1;
    frame(1,0,0);x+=JET_SPEED;g_ram[0x182a+1]=(uint8)x;
    CHECK(g_ram[0x1662+10]==0 || (g_ram[0x15a2+10]&0x40));
    g_ram[0x17fa+10]=0xff;
    CHECK(sounds[SND_EXPLOSION]==3);                        /* a segment every 8 frames */
    /* The invulnerability countdown runs (it blinks) and is never frozen. */
    { unsigned a=g_ram[0x170a+1];frame(1,0,0);CHECK(g_ram[0x170a+1]!=a || a>=8);x+=JET_SPEED;g_ram[0x182a+1]=(uint8)x; }
    emitted=0;SpecialAbilityDraw();CHECK(emitted>=3);
    for(unsigned k=0;k<3;++k) frame(1,0,0);                 /* a wall: no movement */
    CHECK((seat_state(1)[S_FLAGS]&1) && g_ram[0x170a+1]>=JET_LANDING+JET_GUARD_AFTER-4);
    for(unsigned k=0;k<JET_LANDING-4;++k) { frame(1,0,0);CHECK(SpecialAbilityPose(1,&step,&pc)==0 && speed(1,2)==0); }
    for(unsigned k=0;k<TRAIL_LIFE+8;++k) frame(1,0,0);
    CHECK(!seat_state(1)[S_KIND] && SpecialAbilityDebuffed(1) && SpecialAbilityPose(1,&step,&pc)<0);

    /* Hammer (seat 2, x 112): stands still for 125 frames; the mace stuns
     * players it touches once and takes an item; no debuff. */
    choices[2]=14;arena();memset(sounds,0,sizeof(sounds));frame(2,0x08,0x04);
    CHECK(seat_state(2)[S_KIND]==AB_HAMMER && SpecialAbilityPose(2,&step,&pc)==1 && pc==0xcd0900);
    while(timer(2)<MACE_FROM+4) frame(2,0,0);
    g_ram[0x182a+3]=112+16;g_ram[0x1872+3]=16+16;            /* seat 3 next to him */
    redirected=0;
    while(seat_state(2)[S_KIND] && !(g_ram[0x15ba+3]&0x20)) frame(2,0,0);
    /* The cartridge's own reaction ($15BA bit 5) dazes and takes the item. */
    CHECK(g_ram[0x15ba+3]==0x20 && !(g_ram[0x15a2+3]&0x40) && seat_state(3)[S_DROPS]==0 && !injecting);
    g_ram[0x15ba+3]=0;
    while(seat_state(2)[S_KIND]) frame(2,0,0);
    CHECK(g_ram[0x15ba+3]==0 && seat_state(3)[S_DROPS]==0);     /* once per swing */
    CHECK(timer(2)==HAMMER_FRAMES && !SpecialAbilityDebuffed(2));
    CHECK(sounds[SND_MACE]==5);

    /* Lady (seat 0, x 16): turns the gun, laser to the right from frame 48
     * at 2 px per frame, turns the gun again and walks from frame 109; the
     * first player hit is stunned and loses two items. */
    choices[0]=16;arena();memset(sounds,0,sizeof(sounds));frame(0,0x08,0x01);
    CHECK(seat_state(0)[S_KIND]==AB_LASER && SpecialAbilityPose(0,&step,&pc)==0 && pc==0xcaa0b0+2*0x800+1*0x80);
    while(seat_state(0)[S_KIND] && !g_ram[0x15ba+1]) frame(0,0,0);
    CHECK(sounds[SND_LASER]==1 && SpecialAbilityPose(0,&step,&pc)==0 && pc==0xcaa0b0+5*0x800+1*0x80);
    /* One item with the daze, the second through the cartridge's item
     * routine $C4:37CD called this frame through pushed return frames; the
     * hook restores the registers when it comes back. */
    CHECK(g_ram[0x15ba+1]==0x20 && seat_state(1)[S_DROPS]==0 && !g_ram[0x15ba+2]);
    CHECK(seat_state(0)[S_KIND] && !SpecialAbilityDebuffed(0));
    CHECK(redirected==LOSE_ITEM && injecting && cpu.X==1 && (cpu.A&0xff)==0 && cpu.S==0x1ff-5);
    CHECK(g_ram[0x1ff]==0xc0 && g_ram[0x1fe]==0x0a && g_ram[0x1fd]==0xf8 && g_ram[0x1fc]==0x36 && g_ram[0x1fb]==0xd0);
    cpu.S=0x1ff;redirected=0;
    unsigned before=timer(0);
    pad_hook(&cpu,PAD_HOOK);                                      /* back from the routine */
    CHECK(!injecting && !redirected && cpu.A==0x1234 && cpu.X==0x05 && cpu.Y==0x07 && timer(0)==before);
    while(seat_state(0)[S_KIND]) { CHECK(speed(0,1)==0);frame(0,0,0); }
    CHECK(timer(0)==LADY_FRAMES && speed(0,1)==1 && SpecialAbilityPose(0,&step,&pc)<0);
    /* Normal Game: no items are taken. */
    arena();g_ram[0x1d1a]=0;g_ram[0x1031]=0x0d;g_ram[0x1d25]=0x1f;frame(0,0x08,0x01);
    for(unsigned k=0;k<120;++k) frame(0,0,0);
    CHECK(!g_ram[0x15ba+1] && !injecting);

    /* Terrorin (seat 0, cell 3,3): B + down spins him in place, untouchable
     * (kill stores and the fire-cell test), for 64 frames; then the barrier
     * rises on the 21 cells of the 5 x 5 square ($C3:4091), tinted blue;
     * a Clock is placed on a free cell outside it at frame 174 and the
     * barrier drops at 192. */
    choices[0]=19;choices[1]=11;
    arena();
    g_ram[0x182a]=48;g_ram[0x1872]=48;g_ram[0x16aa]=0x33;
    g_ram[0x182b]=80;g_ram[0x1873]=48;g_ram[0x16ab]=0x35;          /* seat 1 two cells right */
    for(unsigned i=2;i<5;++i) g_ram[0x16aa+i]=(uint8)(0x1b+i);
    g_ram[0x1052+0x55]=0x30;                                       /* a soft block inside */
    frame(0,0x08,0x04);
    CHECK(seat_state(0)[S_KIND]==AB_BARRIER && SpecialAbilityPose(0,&step,&pc)==1 && speed(0,2)==0 && shielded(0));
    idle(WALL_UP);CHECK(g_ram[0x1052+0x33]==0);
    frame(0,0,0);
    unsigned raised=0;
    for(unsigned c=0;c<13*16;++c) raised+=g_ram[0x1052+c]==0x0c;
    CHECK(raised==20 && g_ram[0x1052+0x55]==0x30 && g_ram[0x1052+0x33]==0x0c && g_ram[0x1052+0x34]==0x0c);
    CHECK(g_ram[0x1052+0x22]==0 && g_ram[0x1052+0x11]==0x0c && g_ram[0x1052+0x24]==0);   /* pillar positions free */
    /* The fire-cell test ($C4:2CFE, A = $1CCA): Terrorin and his allies are
     * spared, other players are not. */
    cpu.X=0;cpu.A=0x0004;death_hook(&cpu,DEATH_HOOK);CHECK(cpu.A==0);
    cpu.X=1;cpu.A=0x0004;death_hook(&cpu,DEATH_HOOK);CHECK(cpu.A==4);
    g_ram[0x625d]=1;cpu.X=1;cpu.A=0x0004;death_hook(&cpu,DEATH_HOOK);CHECK(cpu.A==0);   /* teammate */
    g_ram[0x15a2+1]=0x08;cpu.X=1;cpu.A=0x0004;death_hook(&cpu,DEATH_HOOK);CHECK(cpu.A==4); /* other team */
    g_ram[0x625d]=0;g_ram[0x15a2+1]=0x18;
    tints=0;SpecialAbilityDraw();CHECK(tints==1 && tint_color==(2u<<10) && tint_box[2]-tint_box[0]==79);
    CHECK(stage_sounds==1 && stage_sound==((0x18u<<16)|0x0288));          /* barrier sound, set $18 */
    idle(WALL_CLOCK-WALL_UP-1);
    /* The Clock flies off like a lost item: $C4:398C (RTS in bank C4) with
     * A = 8, $8D = seat and $1CC7 = 0, both restored afterwards. */
    g_ram[0x8d]=0x44;g_ram[0x1cc7]=0x55;
    frame(0,0,0);
    CHECK(injecting && redirected==THROW_ITEM && (cpu.A&0xff)==ITEM_CLOCK && g_ram[0x8d]==0 && !g_ram[0x1cc7]);
    CHECK(g_ram[0x1fd]==0xf8 && g_ram[0x1fc]==0x36 && g_ram[0x1fb]==0xd0 && g_ram[CLOCK_OWNER]==1);
    cpu.S=0x1ff;redirected=0;pad_hook(&cpu,PAD_HOOK);
    CHECK(!injecting && g_ram[0x8d]==0x44 && g_ram[0x1cc7]==0x55);
    while(seat_state(0)[S_KIND]) frame(0,0,0);
    CHECK(timer(0)==WALL_FRAMES && g_ram[0x1052+0x33]==0 && g_ram[0x1052+0x55]==0x30);
    for(unsigned c=0;c<13*16;++c) CHECK(g_ram[0x1052+c]!=0x0c);
    CHECK(SpecialAbilityDebuffed(0) && speed(0,2)==1);
    /* Seat 1 takes Terrorin's Clock ($C5:42D3, picker in $8D): Terrorin is
     * paralyzed on the next frame ($C4:4503, X = 0) and the Battle clock
     * keeps running. */
    g_ram[0x8d]=1;g_ram[0xe20]=0xc0;g_ram[0xe21]=3;
    clock_hook(&cpu,CLOCK_HOOK);CHECK(!g_ram[CLOCK_OWNER]);
    frame(0,0,0);
    CHECK(injecting && redirected==FREEZE_PLAYER && cpu.X==0 && SpecialAbilityDazed(0) && !SpecialAbilityDazed(1));
    CHECK(!g_ram[0xe20] && !g_ram[0xe21]);
    cpu.S=0x1ff;redirected=0;pad_hook(&cpu,PAD_HOOK);CHECK(!injecting);
    /* His own Clock paralyzes every other player (Free-for-All). */
    g_ram[CLOCK_OWNER]=1;g_ram[0x8d]=0;clock_hook(&cpu,CLOCK_HOOK);
    frame(0,0,0);
    unsigned frozen_seats=0;
    for(unsigned k=0;k<8 && injecting;++k) { if(redirected==FREEZE_PLAYER) frozen_seats|=1u<<cpu.X;cpu.S=0x1ff;redirected=0;pad_hook(&cpu,PAD_HOOK); }
    CHECK(frozen_seats==0x1e && SpecialAbilityDazed(4));
    /* No Clock of his out: nothing; Normal Game keeps the stock effect. */
    clock_hook(&cpu,CLOCK_HOOK);CHECK(!g_ram[CLOCK_EVENT]);
    g_ram[0x1d1a]=0;g_ram[0x1031]=0x0d;g_ram[CLOCK_OWNER]=1;
    clock_hook(&cpu,CLOCK_HOOK);CHECK(g_ram[CLOCK_OWNER]==1 && !g_ram[CLOCK_EVENT]);
    choices[0]=11;choices[1]=12;

    /* Leaving the stage clears every seat. */
    arena();frame(3,0x08,0x02);
    g_ram[0x1031]=4;frame(0,0,0);
    for(unsigned b=STATE;b<STATE_END;++b) CHECK(!g_ram[b]);
    puts("special_abilities_tests: ok");
    return 0;
}
