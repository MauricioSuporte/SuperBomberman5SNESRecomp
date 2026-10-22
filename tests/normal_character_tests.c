#include "mod_runtime.h"
#include "snes/snes.h"
#include "snes/saveload.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static SNESModActivationCallback activation, reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn;return 1; }
int snes_mod_register_activation_plugin(const char *id,SNESModActivationCallback fn) { (void)id;activation=fn;return 1; }
#include "../src/normal_character_mod.c"
uint8 g_ram[0x20000];
void (*g_interp_bridge_pc_hook)(uint32_t,int,int);
/* Honey/Kotetsu mod not linked: stock ten-cell picker arrays. */
uint8_t *PickerCellCharacters(void) { return g_ram+0x62c9; }
uint8_t *PickerCellColors(void) { return g_ram+0x62d3; }
unsigned PickerCellCount(void) { return 10; }
unsigned ExtraBaseChoice(unsigned choice) { return choice>=11 && choice<=EXTRA_CHOICE_LAST ? 1 : choice; }
unsigned hk_seat_choice[5];
void ExtraSetSeatChoice(unsigned seat,unsigned choice) { if(seat<5) hk_seat_choice[seat]=choice; }
unsigned ExtraCharacterSeatColor(unsigned seat) { return g_ram[0x1f82d+seat]; }
void ExtraSetSeatColor(unsigned seat,unsigned color) { g_ram[0x1f82d+seat]=(uint8_t)color; }
static uint8 rom[0x10000];
const uint8 *g_rom=rom;
Snes *g_snes;
static uint16_t pad;
static unsigned hook_calls;
static void other_hook(uint32_t pc,int m,int x) {(void)pc;(void)m;(void)x;++hook_calls;}
uint16 RtlGetPadState(int slot) { (void)slot;return pad; }
void RtlSetPadState(int slot,uint16 buttons) { (void)slot;(void)buttons; }
unsigned NormalCoopPlayerCount(void) {
    unsigned count=0;for(unsigned i=0;i<5;++i) count+=(g_ram[0x1d25]>>i)&1;
    return count?count:1;
}
size_t RtlSaveSnapshotToMemory(void *data,size_t capacity) {
    if(capacity<sizeof(g_ram)) return 0;
    memcpy(data,g_ram,sizeof(g_ram));return sizeof(g_ram);
}
bool RtlLoadSnapshotFromMemory(const void *data,size_t size) {
    if(size!=sizeof(g_ram)) return false;
    NormalCharacterReset(); /* Real snapshot loads also reset/load the mod extension. */
    memcpy(g_ram,data,size);return true;
}
void RtlSaveExecutionState(SaveLoadInfo *sli) { (void)sli; }
bool RtlLoadExecutionState(SaveLoadInfo *sli) { (void)sli;return true; }
void RtlApplyExecutionState(void) { }
static int failed;
void RtlStateStreamFail(SaveLoadInfo *sli) { (void)sli;failed=1; }
typedef struct { SaveLoadInfo base;unsigned char bytes[SNAP_CAP+64];size_t at;bool reading; } Stream;
static void transfer(SaveLoadInfo *sli,void *data,size_t size) {
    Stream *s=(Stream *)sli;
    if(s->at+size>sizeof(s->bytes)) { failed=1;return; }
    if(s->reading) memcpy(data,s->bytes+s->at,size);
    else memcpy(s->bytes+s->at,data,size);
    s->at+=size;
}
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static void file_ready(unsigned count) {
    memset(g_ram,0,sizeof(g_ram));
    g_ram[0x1031]=3;g_ram[0x1d25]=count==2?3:1;
    g_ram[0x73]=4;g_ram[0x10]=9;g_ram[0x0d08]=2;
    g_ram[0x2003]=0xff;g_ram[0x2004]=0x0b;g_ram[0x2005]=0xc7;
    g_ram[0x2006]=0xe6;g_ram[0x2007]=0x10;g_ram[0x2008]=0xc7;
    g_ram[0x625f]=1;g_ram[0x6260]=1;
    g_ram[0x4000]=0x72; /* campaign state must survive Battle's menu */
    set_picker_pc(0xc71257);
}
int main(void) {
    activation();players=2;chosen=true;
    g_ram[0x1031]=8;g_ram[0x625f]=2;g_ram[0x6260]=7;
    g_ram[0x6264]=1;g_ram[0x6265]=2;
    rom[0xbb4f]=8;rom[0xbb54]=48;
    g_interp_bridge_pc_hook=other_hook;
    NormalCharacterInstallHook();CHECK(g_interp_bridge_pc_hook==campaign_hook);
    NormalCharacterFrame();CHECK(chosen);
    campaign_hook(0xc428b4,1,1);
    CHECK(g_ram[0x16c2]==1 && g_ram[0x16c3]==6);
    CHECK(g_ram[0x16da]==9 && g_ram[0x16db]==50);
    g_ram[0x1d24]=3;g_ram[0x1d25]=2;
    g_ram[0x16c2]=0;campaign_hook(0xc21697,1,1);CHECK(g_ram[0x16c2]==1);
    CHECK(g_ram[0x1d25]==3 && !restarting);
    g_ram[0x1d25]=2;campaign_hook(0xc21697,1,1);CHECK(g_ram[0x1d25]==2);
    CHECK(hook_calls==3);
    for(unsigned count=1;count<=5;++count) {
        players=count;
        for(unsigned i=0;i<count;++i) {
            g_ram[0x625f+i]=(uint8_t)(i+1);g_ram[0x6264+i]=(uint8_t)i;
            rom[0xbb4e + i]=(uint8_t)(i*7);
        }
        campaign_hook(0xc21697,1,1);
        for(unsigned i=0;i<count;++i) {
            CHECK(g_ram[0x16c2+i]==i && g_ram[0x16da+i]==i*8);
        }
    }
    g_ram[0x1d1a]=1;g_ram[0x16c2]=0;campaign_hook(0xc21697,1,1);CHECK(!g_ram[0x16c2]);
    g_ram[0x1031]=2;NormalCharacterFrame();CHECK(!chosen);
    reset_callback();CHECK(g_interp_bridge_pc_hook==other_hook);g_interp_bridge_pc_hook=NULL;
    for(unsigned i=0;i<9;++i) rom[0xbb4e + i]=(uint8_t)(i*7+1);
    file_ready(1);NormalCharacterFrame();CHECK(!picking);
    activation();NormalCharacterFrame();CHECK(picking && players==1);
    CHECK(g_ram[0x1031]==4 && g_ram[0x626a]==2);
    g_ram[0x1038]=5;set_picker_pc(0xc68b10);NormalCharacterFrame();
    CHECK(picking && !picker_ready); /* Temporary initialization counter. */
    g_ram[0x1038]=0;set_picker_pc(0xc689cf);
    /* Save/load inside the picker retains the suspended campaign. */
    Stream *s=calloc(1,sizeof(Stream));CHECK(s);
    s->base.func=transfer;NormalCharacterSave(&s->base);
    size_t picker_length=s->at;
    NormalCharacterReset();s->at=0;s->reading=true;
    NormalCharacterLoad(&s->base,10);NormalCharacterLoaded(10);
    CHECK(!failed && picking && resume_size==sizeof(g_ram));
    g_ram[0x4000]=0xff;g_ram[0x625f]=9;g_ram[0x6264]=2;
    g_ram[0x1038]=1;set_picker_pc(0xc68e68);NormalCharacterFrame();
    CHECK(!picking && chosen && g_ram[0x4000]==0x72 && g_ram[0x1031]==3);
    CHECK(g_ram[0x16c2]==8 && g_ram[0x16da]==59);
    s->at=0;s->reading=false;NormalCharacterSave(&s->base);
    CHECK(s->at==picker_length); /* Rewind slots must fit every phase. */
    NormalCharacterReset();s->at=0;s->reading=true;
    NormalCharacterLoad(&s->base,10);NormalCharacterLoaded(10);
    CHECK(chosen && !picking && players==1); /* Continue authorization survives load. */
    restarting=true;s->at=0;s->reading=false;NormalCharacterSave(&s->base);
    CHECK(s->at==picker_length);
    NormalCharacterReset();s->at=0;s->reading=true;
    NormalCharacterLoad(&s->base,10);NormalCharacterLoaded(10);CHECK(chosen && restarting);
    /* Both players must confirm before the Normal machine resumes. */
    NormalCharacterReset();file_ready(2);NormalCharacterFrame();CHECK(players==2);
    set_picker_pc(0xc68e68);g_ram[0x1038]=1;NormalCharacterFrame();CHECK(picking);
    g_ram[0x1038]=2;g_ram[0x625f]=3;g_ram[0x6260]=7;
    set_picker_pc(0xc68e30);NormalCharacterFrame(); /* Last confirm advances VM. */
    CHECK(!picking && g_ram[0x16c2]==2 && g_ram[0x16c3]==6);
    for(unsigned count=3;count<=5;++count) {
        NormalCharacterReset();file_ready(2);g_ram[0x1d25]=(uint8_t)((1u<<count)-1);
        NormalCharacterFrame();CHECK(picking && players==count);
        for(unsigned i=0;i<count;++i) { CHECK(g_ram[0x6269+i]==1);g_ram[0x625f+i]=(uint8_t)(i+2);g_ram[0x6264+i]=(uint8_t)i; }
        set_picker_pc(0xc68e68);g_ram[0x1038]=(uint8_t)(count-1);NormalCharacterFrame();CHECK(picking);
        g_ram[0x1038]=(uint8_t)count;NormalCharacterFrame();CHECK(!picking && chosen);
        for(unsigned i=0;i<count;++i) { CHECK(g_ram[0x16c2+i]==i+1);CHECK(g_ram[0x16da+i]==rom[0xbb4f+i]+i); }
    }
    NormalCharacterReset();file_ready(1);NormalCharacterFrame();
    set_picker_pc(0xc68e68);pad=0x0001;NormalCharacterBeforeFrame();
    CHECK(!picking && !chosen && picker_pc()==0xc7100e && g_ram[0x4000]==0x72);
    CHECK(g_ram[0x0d08]==1 && g_ram[0x10]==6 && g_ram[0x2003]==0xff);
    /* Continue -> File confirms through its own fade return stack. */
    pad=0;NormalCharacterReset();file_ready(1);
    g_ram[0x2003]=0xea;g_ram[0x2006]=0x2b;g_ram[0x2007]=0x0e;
    NormalCharacterFrame();CHECK(picking && from_continue);
    s->at=0;s->reading=false;NormalCharacterSave(&s->base);
    NormalCharacterReset();s->at=0;s->reading=true;
    NormalCharacterLoad(&s->base,10);NormalCharacterLoaded(10);CHECK(!failed && picking && from_continue);
    g_ram[0x625f]=4;set_picker_pc(0xc68e68);g_ram[0x1038]=1;NormalCharacterFrame();
    CHECK(!picking && chosen && g_ram[0x16c2]==3);
    /* Character 10 (Subordinate Bomber mod) is cartridge index 9. */
    rom[0xbb4e + 9]=0x48;NormalCharacterReset();file_ready(1);NormalCharacterFrame();CHECK(picking);
    g_ram[0x625f]=10;g_ram[0x6264]=3;set_picker_pc(0xc68e68);g_ram[0x1038]=1;NormalCharacterFrame();
    CHECK(!picking && chosen && g_ram[0x625f]==10 && g_ram[0x16c2]==9 && g_ram[0x16da]==0x4b);
    NormalCharacterReset();file_ready(1);
    g_ram[0x2003]=0xea;g_ram[0x2006]=0x2b;g_ram[0x2007]=0x0e;NormalCharacterFrame();
    set_picker_pc(0xc68e68);pad=0x0001;NormalCharacterBeforeFrame();
    CHECK(!picking && !chosen && picker_pc()==0xc70d59 && from_continue && g_ram[0x2003]==0xea && g_ram[0x0d08]==1 && g_ram[0x10]==6);
    /* Back from the file list and other fades never open the picker. */
    pad=0;NormalCharacterReset();file_ready(1);g_ram[0x2006]=0x2b;NormalCharacterFrame();CHECK(!picking);
    pad=0;reset_callback();file_ready(1);NormalCharacterFrame();CHECK(!picking);
    activation();
    /* Snapshot restoration retains every extra choice and its independent color. */
    for(unsigned c=11;c<=EXTRA_CHOICE_LAST;++c) {
        NormalCharacterReset();file_ready(1);NormalCharacterFrame();
        g_ram[0x625f]=(uint8_t)c;g_ram[0x6264]=2;g_ram[0x1f82d]=3;
        set_picker_pc(0xc68e68);g_ram[0x1038]=1;NormalCharacterFrame();
        CHECK(chosen && !picking && hk_seat_choice[0]==c && g_ram[0x625f]==1);
        CHECK(g_ram[0x1f82d]==3);
    }
    /* Reject corrupt lengths without allocating an unbounded buffer. */
    memset(s,0,sizeof(*s));s->base.func=transfer;s->reading=true;
    uint32_t bad[]={0x32434e42,1,1,RESUME_CAP+1,RESUME_CAP};memcpy(s->bytes,bad,sizeof(bad));
    NormalCharacterLoad(&s->base,10);CHECK(failed && !resume);
    NormalCharacterReset();free(s);return 0;
}
