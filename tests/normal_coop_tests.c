#include "mod_runtime.h"
#include "snes/snes.h"
#include "snes/saveload.h"
#include <stdio.h>
#include <string.h>
static SNESModActivationCallback activation,reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn;return 1; }
int snes_mod_register_activation_plugin(const char *id,SNESModActivationCallback fn) { (void)id;activation=fn;return 1; }
#include "../src/normal_coop_mod.c"
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
    /* Dead players stay out across stages, including a sole P5 participant.
     * The last participant remains eligible for the original retry flow. */
    map();g_ram[0x1031]=13;active_mask=31;
    for(unsigned i=0;i<4;++i) { g_ram[0x8d]=(uint8_t)i;instruction(0xc43794,1,1); }
    CHECK(active_mask==16);
    g_ram[0x8d]=4;instruction(0xc43794,1,1);CHECK(active_mask==16);
    instruction(0xc21697,1,1);CHECK(rom(0xc24300)[2]==0xa2 && rom(0xc24300)[3]==4);
    CHECK(g_ram[0x1cbf]==5 && rom(0xc30458)[1]==10);
    /* The stock HUD selector stays live; sprite pages use the authored
     * allocator only while three or more seats are configured. */
    CHECK(rom(0xc5066c)[0]==0xff);
    CHECK(rom(0xc56af5)[0]==0x4c && rom(0xc56af5)[1]==0x00 && rom(0xc56af5)[2]==0x71);
    CHECK(!memcmp(rom(0xc57100),page_allocator,sizeof(page_allocator)));
    count=2;configure();CHECK(rom(0xc56af5)[0]==0xff);
    count=5;configure();CHECK(rom(0xc56af5)[0]==0x4c);
    /* Every P1/P2-only loop covers the roster; point damage skips companions. */
    CHECK(rom(0xc20c9f)[1]==5 && rom(0xc56195)[1]==5 && rom(0xc561c6)[1]==5);
    CHECK(rom(0xc16ba1)[1]==5 && rom(0xc0373f)[1]==5 && rom(0xc0374d)[1]==10);
    CHECK(rom(0xc216ee)[0]==0xff && rom(0xc526d8)[0]==2);
    /* Enemy contact sees every seat through five-entry distance arrays. */
    CHECK(rom(0xc31147)[0]==0xc5 && rom(0xc34754)[0]==0xd9 && rom(0xc34766)[0]==30);
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
    active_mask=16;CHECK(stage_mask()==1);
    count=3;g_ram[0x1031]=0x67;configure();CHECK(seats()==3 && rom(0xc24349)[0]==1 && rom(0xc526de)[0]==3);
    count=5;g_ram[0x1031]=13;active_mask=saved_active;configure();build_spawn();
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
    /* Seats 3-5 request idle OBJ palette slots 7, 6 and 5; P1/P2 keep theirs. */
    g_ram[0x8d]=1;g_ram[0x348]=4;instruction(0xc42adc,1,1);CHECK(g_ram[0x348]==4);
    g_ram[0x8d]=2;g_ram[0x348]=8;instruction(0xc42adc,1,1);CHECK(g_ram[0x348]==0x80);
    g_ram[0x8d]=3;g_ram[0x348]=0x10;instruction(0xc42b90,1,1);CHECK(g_ram[0x348]==0x40);
    g_ram[0x8d]=4;g_ram[0x348]=0x20;instruction(0xc42adc,1,1);CHECK(g_ram[0x348]==0x20);
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
    unsigned saved_patch_count=patch_count;
    reset_callback();CHECK(!enabled && !installed && !g_interp_bridge_pc_hook);
    for(unsigned i=0;i<saved_patch_count;++i) CHECK(!memcmp(rom(patches[i].pc),patches[i].original,patches[i].size));
    CHECK(cartridge[0x31147]==0xf6 && cartridge[0x34754]==0xfe);
    CHECK(!memcmp(cartridge+0x526d7,choreography,sizeof(choreography)) && cartridge[0x526f4]==0xfd);
    puts("Normal co-op: menu, lives, collision, bosses, boss slots, rejoin gating and snapshots passed.");return 0;
}
