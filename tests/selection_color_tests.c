#include "mod_runtime.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static SNESModActivationCallback activation,reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn;return 1; }
int snes_mod_register_activation_plugin(const char *id,SNESModActivationCallback fn) { (void)id;activation=fn;return 1; }
static bool subordinate;
bool SubordinateEnabled(void) { return subordinate; }
#include "../src/selection_color_mod.c"
uint8 g_ram[0x20000];
/* Honey/Kotetsu mod not linked: stock ten-cell picker arrays. */
uint8_t *PickerCellCharacters(void) { return g_ram+0x62c9; }
uint8_t *PickerCellColors(void) { return g_ram+0x62d3; }
unsigned PickerCellCount(void) { return 10; }
unsigned HoneyKotetsuBaseChoice(unsigned choice) { return choice>=11 && choice<=18 ? 1 : choice; }
unsigned ExtraCharacterColorCount(unsigned choice) { return choice==11 || choice==12 ? 1 : choice>=13 && choice<=18 ? 5 : 0; }
unsigned hk_seat_choice[5];
void HoneyKotetsuSetSeatChoice(unsigned seat,unsigned choice) { if(seat<5) hk_seat_choice[seat]=choice; }
static uint8 rom[0x400000];
const uint8 *g_rom=rom;
static uint16 pads[5];
uint16 RtlGetPadState(int slot) { return pads[slot]; }
void PtbrDrawPpuFrameObserved(SnesBeamRasterObserver *observer,void *context) { (void)observer;(void)context; }
typedef struct { SaveLoadInfo base;uint8 bytes[64];size_t at,end;bool reading; } Stream;
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
static void set_pc(uint32_t pc) { g_ram[0xc90]=pc;g_ram[0xcb8]=pc>>8;g_ram[0xce0]=pc>>16; }
static void rom_word(unsigned a,unsigned v) { rom[a]=v;rom[a+1]=v>>8; }
static void fixture(void) {
    SelectionColorReset();memset(g_ram,0,sizeof(g_ram));memset(pads,0,sizeof(pads));
    g_ram[0x1031]=4;set_pc(0xc68e68);rom_word(0x379,0x8000);
    for(unsigned id=1;id<=0x4f;++id) {
        unsigned record=0x8100+id*32;rom_word(0x18000+(id-1)*2,record);
        for(unsigned c=0;c<9;++c) rom_word(0x10000+record+4+c*2,id*16+c);
    }
    for(unsigned character=0;character<9;++character) {
        rom[0xbb4e + character]=1+character*8;
        rom[0x21301+character]=0x30+character*8;
        g_ram[0x62c9+character]=character+1;
        for(unsigned c=0;c<6;++c) rom[0x2130b+character*7+c]=1+character*8+c;
    }
    for(unsigned player=0;player<5;++player) {
        g_ram[0x6269+player]=1;g_ram[0x625f+player]=player+1;
        g_ram[0x62ab+player]=player;g_ram[0x62b5+player]=1;
        unsigned object=10+player*2;
        g_ram[0x62bf+player]=object;g_ram[0x6df+object]=player;
        g_ram[0x349+player]=1;
    }
}
static void press(unsigned player,uint16 input) {
    pads[player]=input;SelectionColorBeforeFrame();SelectionColorAfterFrame();
    pads[player]=0;SelectionColorBeforeFrame();SelectionColorAfterFrame();
}
static void native_glyph(Ppu *ppu,unsigned tile,unsigned ink) {
    unsigned base=tile_address(ppu,tile);
    /* Synthetic test glyph, not cartridge font data. */
    for(unsigned y=1;y<7;++y) for(unsigned x=2;x<5;++x)
        if(x==2 || y==1 || y==3 || (x==4 && y<4))
            for(unsigned p=0;p<4;++p) if(ink&(1u<<p)) ppu->vram[base+y+(p/2)*8]|=1u<<((7-x)+(p%2)*8);
}
int main(void) {
    fixture();press(0,0x800);CHECK(!live && g_ram[0x6264]==0);
    activation();SelectionColorAfterFrame();CHECK(live && g_ram[0x6264]==0);
    for(unsigned i=0;i<5;++i) CHECK(colors[i]==i && g_ram[0x6264+i]==i);
    /* Upper strips must preserve the lower figure's shared index 8. */
    for(unsigned cell=0;cell<5;++cell) {
        rom[0x21301+cell]=0x38+cell*16;
        unsigned shared=0x2910+rom[0x21301+cell]*2;
        g_ram[shared]=0x66;g_ram[shared+1]=0x14;
        for(unsigned c=0;c<6;++c) {
            colors[cell]=c;SelectionColorAfterFrame();
            CHECK(g_ram[shared]==0x66 && g_ram[shared+1]==0x14);
        }
    }
    for(unsigned i=0;i<5;++i) colors[i]=0; /* Remaining cycle fixtures start at white. */
    for(unsigned c=1;c<=6;++c) { press(0,0x800);CHECK(g_ram[0x6264]==c%6); }
    press(0,0x400);CHECK(g_ram[0x6264]==5);
    press(0,0xc00);CHECK(g_ram[0x6264]==5); /* Both shoulders cancel. */
    pads[0]=0x800;SelectionColorBeforeFrame();SelectionColorAfterFrame();
    CHECK(g_ram[0x6264]==0);
    SelectionColorBeforeFrame();SelectionColorAfterFrame();CHECK(g_ram[0x6264]==0); /* Held key. */
    pads[0]=0;SelectionColorBeforeFrame();
    /* A Super Bomberman 4 character (choice 15) has five colors. */
    {
        unsigned saved=g_ram[0x625f];g_ram[0x625f]=15;colors[0]=0;
        for(unsigned c=1;c<=5;++c) { press(0,0x800);CHECK(colors[0]==c%5 && g_ram[0x6264]==c%5); }
        press(0,0x400);CHECK(colors[0]==4);
        CHECK(SelectionColorSeat(0)==4);
        g_ram[0x625f]=(uint8_t)saved;colors[0]=0;press(0,0x400);press(0,0x800);
    }
    /* The shared grid portrait has one palette; isolate P1 while checking it. */
    for(unsigned p=1;p<5;++p) g_ram[0x625f+p]=0;
    for(unsigned character=1;character<=9;++character) {
        g_ram[0x625f]=character;
        for(unsigned c=0;c<6;++c) {
            colors[0]=c;g_ram[0x6264]=4;SelectionColorAfterFrame();
            CHECK(g_ram[0x6264]==c && g_ram[0x62d3+character-1]==c);
            unsigned id=1+(character-1)*8+c;
            CHECK(g_ram[0x2b10]==id);
            unsigned dest=0x2910+0x80*2;
            CHECK((g_ram[dest]|g_ram[dest+1]<<8)==id*16);
        }
    }
    for(unsigned p=1;p<5;++p) { g_ram[0x625f+p]=p+1;g_ram[0x6269+p]=2; }
    memset(g_ram+0x62b5,0xff,5);
    memset(g_ram+0x62ab,0xff,5);
    /* Active CPUs are selected through controller 1; confirmed seats freeze. */
    for(unsigned seat=1;seat<5;++seat) {
        g_ram[0x1038]=seat;g_ram[0x62ab]=seat;g_ram[0x62b5+seat]=1;press(0,0x800);
        CHECK(colors[seat]==1 && colors[0]==5);
        CHECK(g_ram[0x6264+seat]==1);
        press(0,0x400);CHECK(colors[seat]==0 && colors[0]==5);
        g_ram[0x62b5+seat]=0xff;
    }
    g_ram[0x1038]=4;g_ram[0x62b5+4]=1;press(0,0x800);CHECK(colors[4]==1);
    for(unsigned seat=0;seat<5;++seat) { g_ram[0x6269+seat]=1;g_ram[0x62ab+seat]=seat;g_ram[0x62b5+seat]=1; }
    for(unsigned seat=0;seat<5;++seat) {
        g_ram[0x6269+seat]=1;g_ram[0x1038]=seat;
        uint8 before[5];memcpy(before,colors,sizeof(before));
        press(seat,0x800);CHECK(colors[seat]==(before[seat]+1)%6);
        for(unsigned other=0;other<5;++other) if(other!=seat) CHECK(colors[other]==before[other]);
        press(seat,0x400);CHECK(!memcmp(before,colors,sizeof(before)));
    }
    memset(g_ram+0x62b5,0xff,5);
    g_ram[0x1038]=5;press(0,0x800);CHECK(colors[0]==5 && colors[4]==1); /* Complete. */
    g_ram[0x1038]=0;g_ram[0x62b5]=1;press(0,0x900);CHECK(colors[0]==0); /* R plus A. */
    colors[0]=4;g_ram[0x1038]=5;set_pc(0xc68e30);SelectionColorAfterFrame();
    CHECK(g_ram[0x6264]==4); /* Last confirmation advances the script. */
    Stream state={.base={.func=transfer}};SelectionColorSave(&state.base);
    SelectionColorReset();state.reading=true;state.at=0;SelectionColorLoad(&state.base,10);
    CHECK(!failed && live && colors[0]==4 && colors[4]==1);
    state.at=state.end;SelectionColorLoad(&state.base,10);CHECK(!live); /* Old snapshot, no chunk. */
    state.at=0;state.bytes[5]=6;SelectionColorLoad(&state.base,10);CHECK(failed);
    fixture();activation();SelectionColorAfterFrame();
    memset(colors,0,sizeof(colors));
    memset(g_ram+0x62ab,0xff,5);memset(g_ram+0x62b5,0xff,5);
    g_ram[0x62ab]=3;g_ram[0x62ac]=2;g_ram[0x62b7]=g_ram[0x62b8]=1;
    press(1,0x800);CHECK(colors[2]==1 && colors[3]==0 && colors[0]==0);
    press(0,0x800);CHECK(colors[2]==1 && colors[3]==1 && colors[0]==0);
    g_ram[0x62b8]=0xff;g_ram[0x62ab]=4;g_ram[0x62b9]=1;
    press(0,0x800);CHECK(colors[4]==1 && colors[3]==1 && colors[2]==1);
    /* Same-character previews must not share the palette of a pending CPU. */
    g_ram[0x625f]=g_ram[0x6260]=2;colors[0]=4;colors[1]=2;
    g_ram[0x62b5]=1;g_ram[0x62b6]=0;
    g_ram[0x6df+10]=1;g_ram[0x349]=0;g_ram[0x34a]=2;
    SelectionColorAfterFrame();
    CHECK(g_ram[0x6df+10]==0 && g_ram[0x6df+12]==1);
    CHECK(g_ram[0x349]==1 && g_ram[0x34a]==1);
    CHECK(g_ram[0x2b10]==13 && g_ram[0x2b11]==11 && g_ram[0x62d4]==4);
    /* Final cleanup may reset CPU colors before the stage derives palettes. */
    g_ram[0x1038]=g_ram[0x103a]=5;set_pc(0xc68ef0);
    memset(g_ram+0x6264,4,5);SelectionColorAfterFrame();
    CHECK(pending && !live && !memcmp(g_ram+0x6264,colors,5));
    Stream committed={.base={.func=transfer}};SelectionColorSave(&committed.base);
    SelectionColorReset();committed.reading=true;committed.at=0;SelectionColorLoad(&committed.base,10);CHECK(pending && !live);
    memset(g_ram+0x6264,3,5);SelectionColorBeforeFrame();CHECK(!memcmp(g_ram+0x6264,colors,5));
    for(unsigned i=0;i<5;++i) {
        g_ram[0x16c2+i]=g_ram[0x625f+i]-1;
        g_ram[0x16da+i]=rom[0xbb4e + g_ram[0x16c2+i]]+colors[i];
    }
    SelectionColorAfterFrame();CHECK(!pending);
    fixture();activation();SelectionColorAfterFrame();
    Ppu *ppu=calloc(1,sizeof(Ppu));CHECK(ppu);ppu->obsel=3;
    for(unsigned s=0;s<128;++s) ppu->oam[s*2]=0xe000;
    for(unsigned p=0;p<5;++p) {
        unsigned y=p==4?175:111,x=40+p*40;
        ppu->oam[p*4]=(y<<8)|x;ppu->oam[p*4+1]=0x3c00|6+p*2;
        ppu->oam[p*4+2]=(y<<8)|(x+8);ppu->oam[p*4+3]=0x3c00|7+p*2;
        native_glyph(ppu,7+p*2,15-p);
    }
    uint16 *vram_copy=malloc(sizeof(ppu->vram));CHECK(vram_copy);
    uint16 oam_copy[256];uint8 high_copy[32];
    memcpy(vram_copy,ppu->vram,sizeof(ppu->vram));memcpy(oam_copy,ppu->oam,sizeof(oam_copy));memcpy(high_copy,ppu->highOam,sizeof(high_copy));
    SelectionColorObserve(ppu,1,NULL);CHECK(drawn && saved_vram_count==240);
    for(unsigned p=0;p<5;++p) CHECK((ppu->oam[p*4]>>8)==(p==4?215:151));
    CHECK(pixel(ppu,tile_address(ppu,0x1f0),2,6)==15);
    SelectionColorObserve(ppu,224,NULL);
    CHECK(!drawn && !memcmp(vram_copy,ppu->vram,sizeof(ppu->vram)) && !memcmp(oam_copy,ppu->oam,sizeof(oam_copy)) && !memcmp(high_copy,ppu->highOam,sizeof(high_copy)));
    ppu->oam[30*2]=100<<8;ppu->oam[30*2+1]=0x1f0;
    SelectionColorObserve(ppu,1,NULL);CHECK(!drawn && !saved_vram_count); /* Occupied spare tile. */
    /* Subordinate Bomber (character 10, palettes $48-$4D) with its mod on. */
    fixture();rom[0xbb4e + 9]=0x48;g_ram[0x62c9+9]=10;activation();SelectionColorAfterFrame();
    for(unsigned p=1;p<5;++p) g_ram[0x625f+p]=0;
    g_ram[0x625f]=10;subordinate=true;
    for(unsigned c=0;c<6;++c) {
        colors[0]=c;SelectionColorAfterFrame();
        CHECK(g_ram[0x6264]==c && g_ram[0x62d3+9]==c && g_ram[0x2b10]==0x48+c);
        unsigned dest=0x2910+0x80*2;CHECK((g_ram[dest]|g_ram[dest+1]<<8)==(0x48+c)*16);
    }
    subordinate=false;colors[0]=1;g_ram[0x2b10]=0;SelectionColorAfterFrame();CHECK(g_ram[0x2b10]==0);
    reset_callback();CHECK(!enabled);free(vram_copy);free(ppu);
    puts("Selection colors: six-color cycle, nine characters (plus Subordinate Bomber), independent seats, confirmation, state loading and cursor restoration passed.");
    return 0;
}
