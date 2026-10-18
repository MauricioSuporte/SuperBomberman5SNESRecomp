#include "mod_runtime.h"
#include "snes/snes.h"
#include "snes/saveload.h"
#include <stdio.h>
#include <string.h>
static SNESModActivationCallback activation,reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn;return 1; }
int snes_mod_register_activation_plugin(const char *id,SNESModActivationCallback fn) { (void)id;activation=fn;return 1; }
static bool corner_spawns;
bool CoopCornerSpawnsEnabled(void) { return corner_spawns; }
#include "../src/normal_coop_mod.c"
static InterpPreOpcodeHook pre_hooks[4];static uint32_t pre_pcs[4];static unsigned pre_count;static uint32_t redirected;
bool interp_bridge_add_pre_opcode_hook(uint32_t pc,InterpPreOpcodeHook h){
    if(pre_count>=4)return false;pre_pcs[pre_count]=pc;pre_hooks[pre_count++]=h;return true;}
void interp_bridge_pre_opcode_redirect(uint32_t pc){redirected=pc;}
uint8 g_ram[0x20000];
static uint8 cartridge[0x400000];
const uint8 *g_rom=cartridge;
static Cart cart;
static Snes machine;
static Ppu video;
Snes *g_snes=&machine;
void (*g_interp_bridge_pc_hook)(uint32_t,int,int);
uint8_t *SnesRomPtr(uint32_t pc) { return cartridge+(pc&0x3fffff); }
static uint16_t pad;
uint16 RtlGetPadState(int slot) { return slot==0?pad:0; }
void RtlSetPadState(int slot,uint16 input) { if(slot==0) pad=input; }
typedef struct { SaveLoadInfo base;uint8_t bytes[256];size_t at,end;bool reading,failed; } Stream;
static void transfer(SaveLoadInfo *sli,void *data,size_t n) {
    Stream *s=(Stream *)sli;
    if(s->at+n>sizeof(s->bytes) || (s->reading && s->at+n>s->end)) { s->failed=true;return; }
    if(s->reading) memcpy(data,s->bytes+s->at,n);
    else { memcpy(s->bytes+s->at,data,n);s->end=s->at+n; }
    s->at+=n;
}
size_t RtlStateBytesRemaining(SaveLoadInfo *sli) { Stream *s=(Stream *)sli;return s->end-s->at; }
void RtlStateStreamFail(SaveLoadInfo *sli) { ((Stream *)sli)->failed=true; }
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static void map(void) {
    memset(g_ram,0,sizeof(g_ram));
    memset(g_ram+0x1052,0x80,256);memset(g_ram+0x17fa,0xff,5);
    for(int y=1;y<=11;++y) for(int x=1;x<=13;++x) g_ram[0x1052+y*16+x]=0;
}
int main(void) {
    machine.cart=&cart;cart.rom=cartridge;cart.romSize=sizeof(cartridge);
    machine.ppu=&video;video.bgmode=9;video.bgXsc[2]=0x41;
    memset(cartridge,0xff,sizeof(cartridge));
    /* Boss choreography names its rival by literal slot. */
    static const uint8_t choreography[]={0xa9,0x02,0x22,0x3a,0x2b,0xc4,0xa2,0x02,0xa9,0x08,0x22,0xde,0x3b,0xc4};
    memcpy(cartridge+0x526d7,choreography,sizeof(choreography));
    cartridge[0x526f3]=0xad;cartridge[0x526f4]=0xfd;cartridge[0x526f5]=0x17;
    /* Boss start $C4:3C75-$3CA4 as in the cartridge: LDA $B9AE,Y; STA $1CBF
     * ... LDA #2; JSR $2906 ... LDA #3 ... LDX #2 ... LDA #2; CLC; ADC $1CBF. */
    static const uint8_t boss_start[]={0xb9,0xae,0xb9,0x8d,0xbf,0x1c,0x22,0x3f,0x00,0xc4,0x20,0x30,0x3d,
        0xa9,0x02,0x20,0x06,0x29,0xc0,0x04,0xd0,0x05,0xa9,0x03,0x20,0x06,0x29,0xb9,0xb7,0xb9,0x30,0x15,
        0x85,0x9f,0xa2,0x02,0xa9,0x80,0x9d,0x82,0x17,0xa9,0x02,0x18,0x6d,0xbf,0x1c};
    memcpy(cartridge+0x43c75,boss_start,sizeof(boss_start));
    /* Enemy distance arrays: LDA $1FF6,Y and STA $1FFE,Y. */
    cartridge[0x31146]=0xb9;cartridge[0x31147]=0xf6;cartridge[0x31148]=0x1f;
    cartridge[0x34753]=0x99;cartridge[0x34754]=0xfe;cartridge[0x34755]=0x1f;
    NormalCoopBeforeFrame();CHECK(!installed);
    activation();NormalCoopBeforeFrame();CHECK(installed);
    g_ram[0x1031]=2;g_ram[0x2000]=0x17;g_ram[0x2001]=0xf7;g_ram[0x2002]=0xc6;
    video.vram[0x424b]=302;video.vram[0x424c]=261;
    video.vram[0x428b]=278;video.vram[0x428c]=261;
    /* Attract-mode return history must not hide this menu. */
    g_ram[0x2000]=0x3b;g_ram[0x2001]=0x1b;g_ram[0x2002]=0xc7;
    NormalCoopBeforeFrame();CHECK(NormalCoopPlayerMenu());NormalCoopAfterFrame();
    video.vram[0x428b]=0;
    NormalCoopBeforeFrame();CHECK(!NormalCoopPlayerMenu());NormalCoopAfterFrame();
    video.vram[0x428b]=278;
    for(unsigned n=2;n<=5;++n) {
        /* Up/Down reach the native menu so its cursor sound still plays. */
        pad=0x20;NormalCoopBeforeFrame();CHECK(count==n && pad==0x20);NormalCoopAfterFrame();CHECK(pad==0x20);
        pad=0;NormalCoopBeforeFrame();NormalCoopAfterFrame();
    }
    pad=0x20;NormalCoopBeforeFrame();CHECK(count==1);NormalCoopAfterFrame();
    pad=0x10;NormalCoopBeforeFrame();CHECK(count==5);NormalCoopAfterFrame();
    pad=8;NormalCoopBeforeFrame();CHECK(pad==8);NormalCoopAfterFrame();
    /* The confirmation fade (mode 3) keeps the five-row layout but no input. */
    pad=0;NormalCoopBeforeFrame();NormalCoopAfterFrame();
    g_ram[0x1031]=3;pad=0x20;NormalCoopBeforeFrame();CHECK(NormalCoopPlayerMenu() && count==5);NormalCoopAfterFrame();
    pad=0;g_ram[0x1031]=2;NormalCoopBeforeFrame();NormalCoopAfterFrame();
    for(unsigned n=3;n<=5;++n) {
        count=n;map();g_ram[0x1031]=3;instruction(0xc4289d,1,1);CHECK(g_ram[0x1d23]==n+2);
        instruction(0xc428b4,1,1);CHECK(g_ram[0x1d25]==(1u<<n)-1);
    }
    /* Corner spawn mod: P3 lower left, P4 lower right, P5 centre. */
    corner_spawns=true;
    CHECK(spawn_tile(2,false)==0xb1 && spawn_tile(3,false)==0xbd && spawn_tile(4,false)==0x67);
    g_ram[0x1052+0xb1]=0x30;CHECK(spawn_tile(2,false)==0xa1);
    g_ram[0x1052+0xbd]=0x80;CHECK(spawn_tile(3,false)==0xad);
    g_ram[0x1052+0x67]=0x80;CHECK(spawn_tile(4,false)==0x57);
    /* An empty but enclosed cell is not playable. */
    map();g_ram[0x1052+0x57]=g_ram[0x1052+0x77]=g_ram[0x1052+0x66]=g_ram[0x1052+0x68]=0x80;
    CHECK(spawn_tile(4,false)!=0x67);
    /* Boss pattern is derived from the two cartridge spawns. */
    map();g_ram[0x1d1a]=4;g_ram[0x4e8d]=5;g_ram[0x4e8e]=9;g_ram[0x4ea5]=9;
    CHECK(spawn_tile(2,false)==0x93 && spawn_tile(3,false)==0x9b && spawn_tile(4,false)==0x97);
    g_ram[0x1052+0x97]=0x30;CHECK(spawn_tile(4,false)==0x87);
    g_ram[0x17fa]=10;g_ram[0x16aa]=0x87;CHECK(spawn_tile(4,false)==0x96);
    /* No safe floor must never remove a block. */
    memset(g_ram+0x1052,0x80,256);CHECK(spawn_tile(4,false)==-1);
    for(unsigned i=0;i<256;++i) CHECK(g_ram[0x1052+i]==0x80);
    /* Rejoin runs native creation, reset and life charge, only with a survivor. */
    map();g_ram[0x1031]=3;g_ram[0x1d23]=1;g_ram[0x17fa]=10;g_ram[0x16aa]=0x11;
    rejoin_pressed=4;g_ram[0x4e8f]=1;g_ram[0x4ea7]=11;
    instruction(0xc5060e,1,1);CHECK(rom(0xc57000)[0]==0xa2 && rom(0xc57000)[1]==2);
    instruction(0xc57002,1,1);CHECK(g_ram[0x15a4]==1 && g_ram[0x1d28]==1 && g_ram[0x1d2a]==11);
    g_ram[0x17fa]=0xff;instruction(0xc5060e,1,1);CHECK(rom(0xc57000)[0]==0x4c);
    g_ram[0x17fa]=10;g_ram[0x1d23]=0;instruction(0xc5060e,1,1);CHECK(rom(0xc57000)[0]==0x4c);
    /* Continue equipment reset must preserve every chosen skin/palette. */
    g_ram[0x1031]=8;
    for(unsigned i=0;i<5;++i) { g_ram[0x16c2+i]=(uint8_t)i;g_ram[0x16da+i]=(uint8_t)(i*7+2); }
    instruction(0xc4286f,1,1);CHECK(preserve_characters);
    memset(g_ram+0x16c2,0,5);memset(g_ram+0x16da,0,5);instruction(0xc428b4,1,1);
    for(unsigned i=0;i<5;++i) { CHECK(g_ram[0x16c2+i]==i);CHECK(g_ram[0x16da+i]==i*7+2); }
    CHECK(g_ram[0x1d24]==3 && preserve_characters);
    g_ram[0x1031]=3;instruction(0xc4286f,1,1);
    memset(g_ram+0x16c2,0,5);memset(g_ram+0x16da,0,5);
    instruction(0xc21697,1,1);CHECK(!preserve_characters);
    for(unsigned i=0;i<5;++i) { CHECK(g_ram[0x16c2+i]==i);CHECK(g_ram[0x16da+i]==i*7+2); }
    /* Spawns precede the stage map; once $C5:3D3F built it, a seat inside a
     * wall moves to the nearest safe floor with its object and actor. */
    map();g_ram[0x1031]=13;count=5;
    for(unsigned y=1;y<=11;++y) for(unsigned x=1;x<=4;++x) g_ram[0x1052+y*16+x]=0x80;
    g_ram[0x17fc]=16;g_ram[0x16ac]=0xb1;g_ram[0x182c]=16;g_ram[0x1872+2]=176;g_ram[0x055f+16]=32;g_ram[0x05bf+16]=204;
    g_ram[0x17fd]=0xff;g_ram[0x17fe]=0xff;g_ram[0x17fa]=0xff;g_ram[0x17fb]=0xff;
    spawn_check=false;instruction(0xc53d5d,1,1);CHECK(g_ram[0x16ac]==0xb1);
    spawn_check=true;instruction(0xc53d5d,1,1);
    CHECK(!spawn_check && g_ram[0x16ac]==0xb5 && g_ram[0x4e8f]==5 && g_ram[0x4ea7]==11);
    CHECK(g_ram[0x182c]==80 && g_ram[0x1874]==176 && g_ram[0x055f+16]==96 && g_ram[0x05bf+16]==204);
    spawn_check=true;instruction(0xc53d5d,1,1);CHECK(g_ram[0x16ac]==0xb5); /* Safe seats stay. */
    /* Default layout: P3/P5 beside P1, P4 beside P2, stepping inward. */
    corner_spawns=false;
    map();g_ram[0x4e8d]=1;g_ram[0x4ea5]=1;g_ram[0x4e8e]=13;g_ram[0x4ea6]=11;
    CHECK(spawn_tile(2,false)==0x12 && spawn_tile(4,false)==0x21 && spawn_tile(3,false)==0xbc);
    map();g_ram[0x4e8d]=13;g_ram[0x4ea5]=11;g_ram[0x4e8e]=1;g_ram[0x4ea6]=1;
    CHECK(spawn_tile(2,false)==0xbc && spawn_tile(4,false)==0xad && spawn_tile(3,false)==0x12);
    /* A blocked neighbour falls back to safe floor, never onto P1. */
    map();g_ram[0x4e8d]=1;g_ram[0x4ea5]=1;g_ram[0x17fa]=10;g_ram[0x16aa]=0x11;
    g_ram[0x1052+0x12]=0x30;CHECK(spawn_tile(2,false)==0x13);
    /* Boss layout is the same with or without the corner mod. */
    map();g_ram[0x1d1a]=4;g_ram[0x4e8d]=5;g_ram[0x4e8e]=9;g_ram[0x4ea5]=9;
    CHECK(spawn_tile(2,false)==0x93 && spawn_tile(3,false)==0x9b && spawn_tile(4,false)==0x97);
    corner_spawns=true;
    /* Dead players stay out across stages, including a sole P5 participant.
     * The last participant remains eligible for the original retry flow. */
    map();g_ram[0x1031]=13;active_mask=31;
    for(unsigned i=0;i<4;++i) { g_ram[0x8d]=(uint8_t)i;instruction(0xc43794,1,1); }
    CHECK(active_mask==16);
    g_ram[0x8d]=4;instruction(0xc43794,1,1);CHECK(active_mask==16);
    instruction(0xc21697,1,1);CHECK(rom(0xc24300)[2]==0xa2 && rom(0xc24300)[3]==4);
    CHECK(g_ram[0x1cbf]==19 && rom(0xc42261)[0]==19 && rom(0xc43c76)[0]==19);
    /* The stock HUD selector stays live; sprite pages use the authored
     * allocator only while three or more seats are configured. */
    CHECK(rom(0xc5066c)[0]==0xff);
    CHECK(rom(0xc56af5)[0]==0x4c && rom(0xc56af5)[1]==0x00 && rom(0xc56af5)[2]==0x71);
    CHECK(!memcmp(rom(0xc57100),page_allocator,sizeof(page_allocator)));
    /* A request with no page left (a Louie with five seats) no longer
     * blocks the page queue: the service starts it without tiles. */
    CHECK(rom(0xc56ae3)[0]==0x4c && rom(0xc56ae3)[1]==0x40 && rom(0xc56ae3)[2]==0x71);
    CHECK(!memcmp(rom(0xc57140),page_service,sizeof(page_service)));
    count=2;configure();CHECK(rom(0xc56af5)[0]==0xff && g_ram[0x1cbf]==2 && rom(0xc43c75)[0]==0xb9);
    CHECK(rom(0xc56ae3)[0]==0xff);
    /* One/two players keep the boss stage's own mount offset ($C4:3C78:
     * rivals own slot 2), carried mounts included; only the five-seat
     * value is undone. */
    g_ram[0x1cbf]=3;g_ram[0x1031]=0x6c;configure();CHECK(g_ram[0x1cbf]==3);
    g_ram[0x1cbf]=4;g_ram[0x1031]=0x6a;configure();CHECK(g_ram[0x1cbf]==4);
    g_ram[0x1031]=13;g_ram[0x1cbf]=19;configure();CHECK(g_ram[0x1cbf]==2 && rom(0xc42261)[0]!=19 && rom(0xc0374d)[0]!=0x5c);
    count=5;configure();CHECK(rom(0xc56af5)[0]==0x4c);
    /* Every P1/P2-only loop covers the roster; point damage skips companions. */
    CHECK(rom(0xc20c9f)[1]==5 && rom(0xc56195)[1]==5 && rom(0xc561c6)[1]==5);
    CHECK(rom(0xc16ba1)[1]==5 && rom(0xc0373f)[1]==5);
    /* Point damage: JML to the range check; actors 5-18 are enemies. */
    CHECK(rom(0xc0374d)[0]==0x5c && rom(0xc0374e)[0]==0x50 && rom(0xc0374f)[0]==0x43 && rom(0xc03750)[0]==0xc2);
    CHECK(rom(0xc24350)[1]==5 && rom(0xc24355)[0]==19 && 0x4354+rom(0xc24353)[0]==0x435c && 0x4358+rom(0xc24357)[0]==0x435c);
    CHECK(rom(0xc24359)[0]==0x51 && rom(0xc2435a)[0]==0x37 && rom(0xc2435d)[0]==0x78);
    CHECK(rom(0xc216ee)[0]==0xff && rom(0xc526d8)[0]==2);
    /* Enemy contact sees every seat through five-entry distance arrays. */
    CHECK(rom(0xc31147)[0]==0x05 && rom(0xc31148)[0]==0x01 && rom(0xc34754)[0]==0x19 && rom(0xc34755)[0]==0x01 && rom(0xc34766)[0]==30);
    CHECK(rom(0xc347fb)[0]==5 && rom(0xc3119c)[0]==5 && rom(0xc34696)[0]==0xff);
    CHECK(rom(0xc34b8f)[0]==0x20 && rom(0xc34b8f)[1]==0x70 && rom(0xc34b8f)[2]==0xff);
    CHECK(rom(0xc3ff70)[0]==0x20 && rom(0xc3ff77)[0]==5 && rom(0xc3ff88)[0]==0x60);
    /* Boss rivals move past the players; five bomber slots trim the roster. */
    uint8_t saved_active=active_mask;active_mask=31;
    g_ram[0x1031]=0x67;configure();build_spawn();
    CHECK(seats()==4 && stage_mask()==15 && rom(0xc20c9f)[1]==4 && rom(0xc502ff)[1]==4);
    CHECK(rom(0xc216b8)[0]==0x20 && rom(0xc216ee)[0]==0x20 && rom(0xc24349)[0]==2);
    CHECK(rom(0xc526d8)[0]==4 && rom(0xc526de)[0]==4 && rom(0xc526f4)[0]==0xff);
    CHECK(rom(0xc24300)[34]==0x7a);
    CHECK(rom(0xc347fb)[0]==4 && rom(0xc3ff77)[0]==4);
    g_ram[0x1031]=0x6a;configure();CHECK(seats()==3 && rom(0xc526de)[0]==3 && rom(0xc526f4)[0]==0xfe);
    /* ZONE3 - D's rival rides its Black Louie from its moved slot, with the
     * five-seat mount offset; P3 keeps its own seat. */
    CHECK(rom(0xc43c83)[0]==3 && rom(0xc43c8c)[0]==4 && rom(0xc43c98)[0]==3 && rom(0xc43c9f)[0]==3);
    CHECK(rom(0xc43c75)[0]==0xa9 && rom(0xc43c76)[0]==19 && rom(0xc43c78)[0]==0x8d);
    active_mask=16;CHECK(stage_mask()==1);
    count=3;g_ram[0x1031]=0x67;configure();CHECK(seats()==3 && rom(0xc24349)[0]==1 && rom(0xc526de)[0]==3);
    count=5;g_ram[0x1031]=13;active_mask=saved_active;configure();build_spawn();
    CHECK(rom(0xc43c98)[0]==2 && rom(0xc43c9f)[0]==2 && rom(0xc43c83)[0]==2);
    CHECK(seats()==5 && rom(0xc216ee)[0]==0xff && rom(0xc526de)[0]==2 && rom(0xc526f4)[0]==0xfd);
    /* A seat benched for a boss keeps the character and items the rival's
     * creation overwrites, then returns with them on the next stage. */
    active_mask=31;preserve_characters=false;g_ram[0x1031]=0x67;g_ram[0x16c6]=3;g_ram[0x17ce]=7;g_ram[0x4ec1]=4;
    instruction(0xc21697,1,1);CHECK(benched_mask==16 && g_ram[0x1d25]==15);
    g_ram[0x16c6]=1;g_ram[0x17ce]=1;g_ram[0x4ec1]=1;instruction(0xc21697,1,1);CHECK(benched[4][0]==3);
    {
        Stream b={0};b.base.func=transfer;NormalCoopSave(&b.base);benched_mask=0;memset(benched,0,sizeof(benched));
        b.reading=true;b.at=0;NormalCoopLoad(&b.base,10);CHECK(!b.failed && benched_mask==16 && benched[4][7]==7);
    }
    g_ram[0x1031]=0x0d;instruction(0xc21697,1,1);
    CHECK(!benched_mask && g_ram[0x16c6]==3 && g_ram[0x17ce]==7 && g_ram[0x4ec1]==4 && g_ram[0x1d25]==31);
    g_ram[0x1031]=13;active_mask=saved_active;configure();build_spawn();
    /* Pause-menu map return restores a benched seat before the scene ends. */
    g_ram[0x1031]=0x67;benched_mask=16;benched[4][0]=3;benched[4][7]=7;
    g_ram[0x16c6]=9;g_ram[0x17ce]=1;NormalCoopPrepareWorldMapReturn();
    CHECK(!benched_mask && g_ram[0x16c6]==3 && g_ram[0x17ce]==7);
    g_ram[0x1031]=13;
    /* Seats 3-5 request OBJ palette slots 6, 5 and 4 (never 7: the $21-byte
     * palette copies at $C1:0015/$0035 overrun palette 15); P1/P2 keep theirs. */
    g_ram[0x8d]=1;g_ram[0x348]=4;instruction(0xc42adc,1,1);CHECK(g_ram[0x348]==4);
    g_ram[0x8d]=2;g_ram[0x348]=8;instruction(0xc42adc,1,1);CHECK(g_ram[0x348]==0x40);
    g_ram[0x8d]=3;g_ram[0x348]=0x10;instruction(0xc42b90,1,1);CHECK(g_ram[0x348]==0x20);
    g_ram[0x8d]=4;g_ram[0x348]=0x20;instruction(0xc42adc,1,1);CHECK(g_ram[0x348]==0x10);
    /* Palette arbitration: stage objects win slots held by seats 3-5. */
    {
        CpuState cpu={0};installed=true;enabled=true;count=5;active_mask=31;g_ram[0x1031]=13;
        static const uint8_t ids[8]={0x58,1,2,0x26,5,4,3,0},refs[8]={1,2,1,1,1,1,2,0};
        memcpy(g_ram+0x2b10,ids,8);memcpy(g_ram+0x349,refs,8);
        static const uint8_t seat_slot[5]={1,2,6,5,4};
        for(unsigned i=0;i<5;++i) { g_ram[0x17fa+i]=(uint8_t)(0x10+2*i);g_ram[0x6df+0x10+2*i]=seat_slot[i]; }
        g_ram[0x6df+0x20]=6; /* P3's mount shares P3's slot. */
        /* ZONE5 - 1's tower: two slots in mask $C0, slot 6 held by P3. */
        g_ram[0x346]=0x38;g_ram[0x347]=2;g_ram[0x348]=0xc0;pending_seat=-1;redirected=0;
        palette_full(&cpu,0xc00303);
        CHECK(redirected==0xc002e2 && !g_ram[0x2b16] && !g_ram[0x349+6]);
        CHECK(g_ram[0x6df+0x14]==1 && g_ram[0x6df+0x20]==1 && g_ram[0x349+1]==4);
        /* P4/P5 outside the mask keep their slots. */
        CHECK(g_ram[0x6df+0x16]==5 && g_ram[0x6df+0x18]==4 && g_ram[0x2b15]==4 && g_ram[0x2b14]==5);
        /* Enemy slots full of enemies: nothing to take, the failure stands. */
        g_ram[0x2b14]=0x40;g_ram[0x2b15]=0x41;g_ram[0x6df+0x16]=1;g_ram[0x6df+0x18]=1;
        g_ram[0x348]=0x38;g_ram[0x347]=1;redirected=0;
        palette_full(&cpu,0xc00303);CHECK(!redirected && g_ram[0x2b14]==0x40 && g_ram[0x2b15]==0x41);
        /* A seat whose own slot is taken shares P1's ($C0:033D counts it). */
        g_ram[0x2b16]=0x38;g_ram[0x349+6]=1;pending_seat=2;g_ram[0x348]=0x40;cpu.X=0;redirected=0;
        palette_full(&cpu,0xc00303);CHECK(redirected==0xc0033d && cpu.X==1);
        pending_seat=-1;
        /* A sharing seat never writes its colors into P1's slot. */
        cpu.Y=0x14;redirected=0;seat_colors(&cpu,0xc437b0);CHECK(redirected==0xc437cc);
        g_ram[0x6df+0x14]=6;redirected=0;seat_colors(&cpu,0xc437b0);CHECK(!redirected);
        cpu.Y=0x10;g_ram[0x6df+0x10]=1;redirected=0;seat_colors(&cpu,0xc437b0);CHECK(!redirected);
        /* Mounts take their rider's slot (C4:53F0 -> $C4:5B40). */
        CHECK(rom(0xc453f0)[0]==0x20 && rom(0xc453f1)[0]==0x40 && rom(0xc453f2)[0]==0x5b && rom(0xc453f3)[0]==0xea);
        CHECK(!memcmp(rom(0xc45b40),mount_slot,sizeof(mount_slot)));
        CHECK(0x5b4a+mount_slot[9]==0x5b55 && 0x5b52+mount_slot[17]==0x5b55 && mount_slot[19]==0x60 && mount_slot[26]==0x60);
        memset(g_ram+0x349,0,8);memset(g_ram+0x2b10,0,8);memset(g_ram+0x17fa,0xff,5);
    }
    /* A dead player's held Start rejoins without becoming a pause edge on
     * the following frame after their actor has been recreated. */
    memset(previous_seats,0,sizeof(previous_seats));pad=8;
    NormalCoopBeforeFrame();CHECK(pad==0 && (block_start_mask&1));NormalCoopAfterFrame();CHECK(pad==8);
    g_ram[0x17fa]=8;g_ram[0x15a2]=5;
    NormalCoopBeforeFrame();CHECK(pad==0);NormalCoopAfterFrame();
    pad=0;NormalCoopBeforeFrame();NormalCoopAfterFrame();CHECK(!(block_start_mask&1));
    /* Pending guest trampoline instructions survive save/load and rewind. */
    Stream s={0};s.base.func=transfer;count=5;active_mask=31;
    revive_mask=4;build_rejoin();NormalCoopSave(&s.base);size_t length=s.end;
    NormalCoopReset();rom(0xc57000)[0]=0xff;s.reading=true;s.at=0;
    NormalCoopLoad(&s.base,10);CHECK(!s.failed && count==5 && active_mask==31 && rom(0xc57000)[0]==0xa2);
    s.at=0;s.reading=false;NormalCoopSave(&s.base);CHECK(s.end==length);
    s.bytes[4]=6;s.at=0;s.reading=true;NormalCoopLoad(&s.base,10);CHECK(s.failed);
    unsigned saved_patch_count=patch_count;CHECK(patch_count<=sizeof(patches)/sizeof(*patches));
    reset_callback();CHECK(!enabled && !installed && !g_interp_bridge_pc_hook);
    for(unsigned i=0;i<saved_patch_count;++i) CHECK(!memcmp(rom(patches[i].pc),patches[i].original,patches[i].size));
    CHECK(cartridge[0x31147]==0xf6 && cartridge[0x31148]==0x1f && cartridge[0x34754]==0xfe && cartridge[0x34755]==0x1f);
    CHECK(!memcmp(cartridge+0x526d7,choreography,sizeof(choreography)) && cartridge[0x526f4]==0xfd);
    puts("Normal co-op: menu, lives, collision, bosses, boss slots, rejoin gating and snapshots passed.");return 0;
}
