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
/* Honey/Kotetsu mod not linked: stock ten-cell picker arrays. */
uint8_t *PickerCellCharacters(void) { return g_ram+0x62c9; }
uint8_t *PickerCellColors(void) { return g_ram+0x62d3; }
unsigned PickerCellCount(void) { return 10; }
unsigned HoneyKotetsuBaseChoice(unsigned choice) { return choice>=11 && choice<=18 ? 1 : choice; }
unsigned hk_seat_choice[5];
void HoneyKotetsuSetSeatChoice(unsigned seat,unsigned choice) { if(seat<5) hk_seat_choice[seat]=choice; }
static uint8 rom[0x10000];
const uint8 *g_rom=rom;
Snes *g_snes;
static uint16_t pad;
uint16 RtlGetPadState(int slot) { (void)slot;return pad; }
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
    g_ram[0x2003]=0xff;g_ram[0x2004]=0x0b;g_ram[0x2005]=0xc7;
    g_ram[0x2006]=0xe6;g_ram[0x2007]=0x10;g_ram[0x2008]=0xc7;
    g_ram[0x625f]=1;g_ram[0x6260]=1;
    g_ram[0x4000]=0x72; /* campaign state must survive Battle's menu */
    set_picker_pc(0xc71257);
}
int main(void) {
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
    CHECK(!picking && !chosen && picker_pc()==0xc71014 && g_ram[0x4000]==0x72);
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
    CHECK(!picking && !chosen && picker_pc()==0xc70d39 && g_ram[0x2003]==0xe4 && g_ram[0x2006]==0x37);
    /* Back from the file list and other fades never open the picker. */
    pad=0;NormalCharacterReset();file_ready(1);g_ram[0x2006]=0x2b;NormalCharacterFrame();CHECK(!picking);
    pad=0;reset_callback();file_ready(1);NormalCharacterFrame();CHECK(!picking);
    /* Reject corrupt lengths without allocating an unbounded buffer. */
    memset(s,0,sizeof(*s));s->base.func=transfer;s->reading=true;
    uint32_t bad[]={0x32434e42,1,1,RESUME_CAP+1,RESUME_CAP};memcpy(s->bytes,bad,sizeof(bad));
    NormalCharacterLoad(&s->base,10);CHECK(failed && !resume);
    NormalCharacterReset();free(s);return 0;
}
