/* Native tests for the extra-character core (Honey, Kotetsu, Terrorin and
 * the Bomber Kings row). Synthetic cartridge bytes only:
 * the patched locations hold the expected stock instructions, and the
 * archive, frames and palettes are generated patterns, not cartridge data. */
#include "mod_runtime.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static SNESModActivationCallback activation,reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn;return 1; }
int snes_mod_register_activation_plugin(const char *id,SNESModActivationCallback fn) {
    if(strcmp(id,"superbomberman5.extra-characters.characters")) return 0;
    activation=fn;return 1;
}
int snes_frame_counter;
/* Collaborators of the core, stubbed: Bomber Kings switch, Super Bomberman 4
 * image (synthetic frames: pixel = (frame + character) % 15 + 1 inside a
 * 20 x 28 body at x 6..25, y 2..29), abilities and seat colors. */
static bool kings_wanted,kings_ready;
bool BomberKingsWanted(void) { return kings_wanted; }
bool BomberKingsReady(void) { return kings_ready; }
bool Sb4Frame(unsigned character,unsigned frame,uint8_t out[32][32]) {
    if(!kings_ready || character>=6 || frame>=64) return false;
    memset(out,0,32*32);
    for(unsigned y=2;y<30;++y) for(unsigned x=6;x<26;++x) out[y][x]=(uint8_t)((frame+character)%15+1);
    return true;
}
uint32_t Sb4FramePc(unsigned character,unsigned frame) { return character<6 && frame<64?0xd00000|character<<8|frame:0; }
bool Sb4FrameAt(uint32_t pc,uint8_t out[32][32]) { return Sb4Frame((pc>>8)&0xff,pc&0xff,out); }
uint16_t Sb4Color(unsigned character,unsigned palette,unsigned index) { return (uint16_t)(0x4000+character*0x100+palette*0x10+index); }
/* Portraits: a 32x32 square of color 9; colors 0x2000 + ... */
bool Sb4Portrait(unsigned character,uint8_t out[32][32]) {
    if(!kings_ready || character>=6) return false;
    memset(out,9,32*32);return true;
}
uint16_t Sb4PortraitColor(unsigned character,unsigned palette,unsigned index) { return (uint16_t)(0x2000+character*0x100+palette*0x10+index); }
static int ability_pose=-1;static uint32_t ability_frame;
int SpecialAbilityPose(unsigned seat,unsigned *step,uint32_t *sb4_frame) { (void)seat;*step=0;*sb4_frame=ability_frame;return ability_pose; }
uint32_t Sb4BadBomberPc(unsigned character,unsigned direction,bool throwing) {
    return character<6 && direction<4?0xd00000|character<<8|(40+direction+(throwing?4u:0u)):0;
}
void SpecialAbilitiesInstall(void) {}
static bool dazed;
bool SpecialAbilityDazed(unsigned seat) { (void)seat;return dazed; }
static unsigned subordinate_on;
void SubordinateActivate(void) { ++subordinate_on; }
void SpecialAbilityDraw(void) {}
int SelectionColorSeat(unsigned seat) { (void)seat;return -1; }
#include "../src/extra_characters_mod.c"
uint8 g_ram[0x20000];
static uint8 cartridge[0x400000];
const uint8 *g_rom=cartridge;
uint8_t *SnesRomPtr(uint32_t pc) { return cartridge+(pc&0x3fffff); }
static Cart cart;
static Snes machine;
Snes *g_snes=&machine;
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static uint8 stock_rom[0x400000];
static uint8 *at(uint32_t pc) { return cartridge+file(pc); }
static void put_bytes(uint32_t pc,const uint8_t *b,unsigned n) { memcpy(at(pc),b,n); }
static void put_word(uint32_t pc,unsigned v) { at(pc)[0]=(uint8)v;at(pc)[1]=(uint8)(v>>8); }
/* Expected archives: tile t has every pixel = (t+shift)%15+1. */
static uint8 expected[ARCHIVE_SIZE],expected_champion[CHAMPION_SIZE],expected_terrorin[TERRORIN_SIZE];
static void build_archive(uint32_t src,uint32_t end,uint8 *want,unsigned size,unsigned shift) {
    for(unsigned t=0;t<size/32;++t) {
        unsigned v=(t+shift)%15+1;uint8 *b=want+t*32;
        for(unsigned y=0;y<8;++y) for(unsigned p=0;p<4;++p) b[(p/2)*16+y*2+(p%2)]=(v>>p)&1?0xff:0;
    }
    /* Encode so the stream ends exactly at `end` like the verified image:
     * minimal runs first (repeat-previous where bytes repeat), then repeats
     * turned into literals and literals split to add input bytes. */
    enum { MAXSEG=8192 };
    static uint16 kind[MAXSEG],len[MAXSEG];unsigned segs=0;
    for(unsigned i=0;i<size;) {
        unsigned r=0;
        while(i && r<32 && i+r<size && want[i+r]==want[i-1]) ++r;
        if(r) { kind[segs]=1;len[segs++]=(uint16)r;i+=r;continue; }
        unsigned l=1;
        while(l<32 && i+l<size && want[i+l]!=want[i+l-1]) ++l;
        kind[segs]=0;len[segs++]=(uint16)l;i+=l;
    }
    unsigned need=end-src,total=1;
    for(unsigned k=0;k<segs;++k) total+=kind[k]?1:1+len[k];
    for(unsigned k=0;k<segs && total<need;++k) if(kind[k] && total+len[k]<=need) { kind[k]=0;total+=len[k]; }
    while(total<need) {
        unsigned k=0;while(k<segs && (kind[k] || len[k]<2)) ++k;
        if(k==segs) break;
        memmove(kind+k+1,kind+k,(segs-k)*sizeof(*kind));memmove(len+k+1,len+k,(segs-k)*sizeof(*len));
        ++segs;len[k]=1;len[k+1]--;kind[k+1]=0;++total;
    }
    uint8 *out=at(src);unsigned n=0,i=0;
    for(unsigned k=0;k<segs;++k) {
        out[n++]=(uint8)((kind[k]?0x40:0x00)|(len[k]-1));
        if(!kind[k]) { memcpy(out+n,want+i,len[k]);n+=len[k]; }
        i+=len[k];
    }
    out[n++]=0x40; /* control read after the last byte */
}
static void native_frame(uint32_t pc,unsigned n,const int *pieces) {
    at(pc)[0]=(uint8)n;
    for(unsigned k=0;k<n;++k) {
        put_word(pc+1+k*7,(unsigned)pieces[k*3]);put_word(pc+3+k*7,(unsigned)pieces[k*3+1]);
        put_word(pc+5+k*7,0);at(pc+7+k*7)[0]=(uint8)pieces[k*3+2];
    }
}
static void stock_cartridge(void) {
    memset(cartridge,0xff,sizeof(cartridge));
    static const uint8_t char_ref[]={0xc9,0x62,0x7e},color_ref[]={0xd3,0x62,0x7e};
    for(unsigned i=0;i<sizeof(kCellCharRefs)/sizeof(*kCellCharRefs);++i) put_bytes(kCellCharRefs[i],char_ref,3);
    for(unsigned i=0;i<sizeof(kCellColorRefs)/sizeof(*kCellColorRefs);++i) put_bytes(kCellColorRefs[i],color_ref,3);
    static const uint8_t right[]={0x7b,0x34,0x10,0x5d,0x34,0x10,0x86,0x06,0x49,0x05,0x5b,0x01,0x5f,0x34,0x10};
    static const uint8_t left[]={0x7f,0x34,0x10,0x5d,0x34,0x10,0x86,0x05,0x49,0x05,0x5b,0x0a,0x5f,0x34,0x10};
    static const uint8_t vert[]={0x5d,0x34,0x10,0x96,0x06,0x49,0x0a,0x5d,0x34,0x10,0x61};
    static const uint8_t portrait[]={0x3a,0x29,0x0f,0xaa},init[]={0xbf,0x5f,0x62,0x7e,0x3a};
    static const uint8_t cursor_x[]={0xbf,0x76,0xc9,0x80},cursor_y[]={0xbf,0x80,0xc9,0x80};
    static const uint8_t grid_x[]={5,10,15,20,25,5,10,15,20,25},set[]={0x9c,0xe7,0x0a,0x9d,0xc2,0x16};
    static const uint8_t grid_y[]={15,15,15,15,15,20,20,20,20,20};
    put_bytes(0xc68c02,right,sizeof(right));put_bytes(0xc68c78,left,sizeof(left));
    put_bytes(0xc68cee,vert,sizeof(vert));put_bytes(0xc68d62,vert,sizeof(vert));
    put_bytes(0xc213e7,portrait,4);put_bytes(0xc21356,portrait,4);put_bytes(0xc42a3c,init,5);
    put_bytes(0xc0c93a,cursor_x,4);put_bytes(0xc0c941,cursor_y,4);put_bytes(0xc212d2,grid_x,10);
    put_bytes(0xc212dc,grid_y,10);
    put_bytes(0xc43bdf,set,6);
    for(unsigned c=0;c<10;++c) at(0xc0c980)[c]=c<5?0x90:0xb8;
    at(0xc21470)[0]=0x14;at(0xc512fe)[0]=0x1e;
    build_archive(ARCHIVE,ARCHIVE_END,expected,ARCHIVE_SIZE,0);
    build_archive(CHAMPION,CHAMPION_END,expected_champion,CHAMPION_SIZE,7);
    build_archive(TERRORIN_ARCHIVE,TERRORIN_END,expected_terrorin,TERRORIN_SIZE,3);
    /* Object set $0E (Terrorin): animation table at $C7:8000, frame table at
     * $C7:8600. Animation a, step k shows frame (a-0x38)*20+k+1 (up to
     * 18 steps); each frame is one 16x16 piece at (-8,-30) and one at
     * (-8,-14) using tiles (f%8)*2 and (f%8)*2+0x20. */
    at(0xc0171d+TERRORIN_SET*3)[0]=0x00;at(0xc0171d+TERRORIN_SET*3)[1]=0x80;at(0xc0171d+TERRORIN_SET*3)[2]=0xc7;
    at(0xc016cc+TERRORIN_SET*3)[0]=0x00;at(0xc016cc+TERRORIN_SET*3)[1]=0x86;at(0xc016cc+TERRORIN_SET*3)[2]=0xc7;
    for(unsigned a=0x39;a<=0x54;++a) {
        unsigned script=0x8100+(a-0x39)*40;put_word(0xc78000+(a-1)*2,script);
        for(unsigned k=0;k<18;++k) { at(0xc70000|script)[k*2]=(uint8)((a-0x38)*20%190+k+1);at(0xc70000|script)[k*2+1]=6; }
    }
    for(unsigned f=1;f<=200;++f) {
        unsigned data=0x8800+f*16;put_word(0xc78600+(f-1)*2,data);
        uint8 *d=at(0xc70000|data);d[0]=2;
        put_word(0xc70000|(data+1),(unsigned)-8);put_word(0xc70000|(data+3),(unsigned)-30);put_word(0xc70000|(data+5),(f%8)*2);d[7]=0x21;
        put_word(0xc70000|(data+8),(unsigned)-8);put_word(0xc70000|(data+10),(unsigned)-14);put_word(0xc70000|(data+12),(f%8)*2+0x20);d[14]=0x21;
    }
    for(unsigned c=0;c<16;++c) put_word(TERRORIN_PALETTE+c*2,0x3000+c*5);
    /* Champion set 6: tables at $C4:C000/$C4:C100; animations $50 (Honey)
     * and $4E (Kotetsu) alternate frames 1/2 and 3/4 (durations 7 and 16);
     * frame f is one 16x16 piece at (0,-17) using tile f*2. */
    at(0xc0171d+CHAMPION_SET*3)[0]=0x00;at(0xc0171d+CHAMPION_SET*3)[1]=0xc0;at(0xc0171d+CHAMPION_SET*3)[2]=0xc4;
    at(0xc016cc+CHAMPION_SET*3)[0]=0x00;at(0xc016cc+CHAMPION_SET*3)[1]=0xc1;at(0xc016cc+CHAMPION_SET*3)[2]=0xc4;
    for(unsigned s=0;s<2;++s) {
        unsigned script=0xc200+s*16;put_word(0xc4c000+(kChampionAnim[s]-1)*2,script);
        uint8 *d=at(0xc40000|script);d[0]=(uint8)(1+s*2);d[1]=s?16:7;d[2]=(uint8)(2+s*2);d[3]=s?16:7;d[4]=0;d[5]=0;
    }
    for(unsigned f=1;f<=4;++f) {
        unsigned data=0xc300+f*16;put_word(0xc4c100+(f-1)*2,data);
        int piece[3]={0,-17,0x21};native_frame(0xc40000|data,1,piece);put_word(0xc40000|(data+5),f*2);
    }
    /* Object set $0D: animation table at $C4:9000, frame table at $C4:A000.
     * Animation a: frames (a*4+step), one piece per frame: a 16x16 block at
     * (-8,-22) using tile (a*2) and an 8x8 at (-8,-6) using tile 0x20+a. */
    at(0xc0171d+OBJECT_SET*3)[0]=0x00;at(0xc0171d+OBJECT_SET*3)[1]=0x90;at(0xc0171d+OBJECT_SET*3)[2]=0xc4;
    at(0xc016cc+OBJECT_SET*3)[0]=0x00;at(0xc016cc+OBJECT_SET*3)[1]=0xa0;at(0xc016cc+OBJECT_SET*3)[2]=0xc4;
    for(unsigned a=1;a<=0x40;++a) {
        unsigned script=0x9200+a*16;put_word(0xc49000+(a-1)*2,script);
        for(unsigned step=0;step<6;++step) { at(0xc40000|script)[step*2]=(uint8)((a%32)*4+step+1);at(0xc40000|script)[step*2+1]=8; }
    }
    for(unsigned f=1;f<=130;++f) {
        unsigned data=0xa400+f*16;put_word(0xc4a000+(f-1)*2,data);
        uint8 *d=at(0xc40000|data);d[0]=2;
        put_word(0xc40000|(data+1),(unsigned)-8);put_word(0xc40000|(data+3),(unsigned)-22);put_word(0xc40000|(data+5),(f%8)*2);d[7]=0x21;
        put_word(0xc40000|(data+8),(unsigned)-8);put_word(0xc40000|(data+10),(unsigned)-6);put_word(0xc40000|(data+12),0x20+f%8);d[14]=0x20;
    }
    for(unsigned s=0;s<2;++s) for(unsigned c=0;c<16;++c) put_word(kPalette[s]+c*2,0x1000*s+c*33);
}
static void hide_all(Ppu *ppu) { for(unsigned s=0;s<128;++s) ppu->oam[s*2]=0xe000; }
static void sprite(Ppu *ppu,unsigned s,unsigned x,unsigned y,unsigned tile,unsigned palette,bool big16) {
    ppu->oam[s*2]=(uint16_t)((y<<8)|(x&255));
    ppu->oam[s*2+1]=(uint16_t)(((0x20|(palette<<1)|(tile>>8))<<8)|(tile&255));
    ppu->highOam[s/4]&=(uint8_t)~(3u<<((s%4)*2));
    if(big16) ppu->highOam[s/4]|=(uint8_t)(2u<<((s%4)*2));
}
static unsigned pixel(const Ppu *ppu,unsigned tile,unsigned x,unsigned y) {
    unsigned base=tile_address(ppu,tile),c=0;
    for(unsigned p=0;p<4;++p) c|=((ppu->vram[(base+y+(p/2)*8)&0x7fff]>>((7-x)+(p%2)*8))&1u)<<p;
    return c;
}
typedef struct { uint16 vram[0x8000],oam[256],cgram[256];uint8 high[32]; } Snapshot;
static void snap(Snapshot *s,const Ppu *ppu) {
    memcpy(s->vram,ppu->vram,sizeof(s->vram));memcpy(s->oam,ppu->oam,sizeof(s->oam));
    memcpy(s->cgram,ppu->cgram,sizeof(s->cgram));memcpy(s->high,ppu->highOam,sizeof(s->high));
}
static bool same(const Snapshot *s,const Ppu *ppu) {
    return !memcmp(s->vram,ppu->vram,sizeof(s->vram)) && !memcmp(s->oam,ppu->oam,sizeof(s->oam)) &&
        !memcmp(s->cgram,ppu->cgram,sizeof(s->cgram)) && !memcmp(s->high,ppu->highOam,sizeof(s->high));
}
static unsigned visible_count(const Ppu *ppu) { unsigned n=0;for(unsigned s=0;s<128;++s) n+=visible(ppu,s);return n; }
int main(void) {
    CHECK(activation && reset_callback);
    stock_cartridge();
    activation();ExtraCharactersBeforeFrame();CHECK(!installed);reset_callback();
    machine.cart=&cart;memcpy(stock_rom,cartridge,sizeof(stock_rom));
    /* Disabled: nothing changes and the stock arrays are reported. */
    ExtraCharactersBeforeFrame();CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)) && !ExtraCharactersEnabled());
    CHECK(PickerCellCharacters()==g_ram+0x62c9 && PickerCellCount()==10);
    activation();ExtraCharactersBeforeFrame();
    CHECK(installed && patched && art_ready && champion_ready && terrorin_ready && ExtraCharactersInstalled() && subordinate_on>=1);
    CHECK(!memcmp(archive,expected,sizeof(archive)) && !memcmp(champion,expected_champion,sizeof(champion)));
    CHECK(!memcmp(terrorin,expected_terrorin,sizeof(terrorin)));
    CHECK(PickerCellCharacters()==g_ram+0x1f800 && PickerCellColors()==g_ram+0x1f814 && PickerCellCount()==19);
    for(unsigned i=0;i<sizeof(kCellCharRefs)/sizeof(*kCellCharRefs);++i)
        CHECK(at(kCellCharRefs[i])[0]==0x00 && at(kCellCharRefs[i])[1]==0xf8 && at(kCellCharRefs[i])[2]==0x7f);
    for(unsigned i=0;i<sizeof(kCellColorRefs)/sizeof(*kCellColorRefs);++i)
        CHECK(at(kCellColorRefs[i])[0]==0x14 && at(kCellColorRefs[i])[1]==0xf8 && at(kCellColorRefs[i])[2]==0x7f);
    /* Grid: seven columns, the five stock ones at tiles 3-19; Honey and
     * Kotetsu at 23, Terrorin at 27 (top row only). */
    static const uint8_t packed[]={3,7,11,15,19,3,7,11,15,19};
    static const uint8_t grid_rows_stock[]={15,15,15,15,15,20,20,20,20,20};
    CHECK(!memcmp(at(0xc212d2),packed,10));
    CHECK(at(TABLE_CURSOR_X)[4]==152 && at(TABLE_CURSOR_X)[10]==184 && at(TABLE_CURSOR_X)[11]==184 && at(TABLE_CURSOR_X)[18]==216);
    CHECK(at(TABLE_CURSOR_Y)[10]==0x90 && at(TABLE_CURSOR_Y)[11]==0xb8 && at(TABLE_CURSOR_Y)[18]==0x90);
    CHECK(!memcmp(at(0xc0c93a),"\xbf\x10\x61\xc2",4) && !memcmp(at(0xc0c941),"\xbf\x24\x61\xc2",4));
    CHECK(!memcmp(at(0xc212dc),grid_rows_stock,10)); /* two rows: rows unchanged */
    /* Movement: lookups replace the 5x2 arithmetic; every cell is reachable,
     * right/left are inverses, and the empty cell below Terrorin is skipped
     * ($C6:8CEE up, $C6:8D62 down). */
    CHECK(!memcmp(at(0xc68c02),"\x36\x34\x10\xc0\x60\xc2\x5f\x34\x10\x45\x10",11));
    CHECK(!memcmp(at(0xc68c78),"\x36\x34\x10\xd4\x60\xc2\x5f\x34\x10\x45\x10",11));
    CHECK(!memcmp(at(0xc68cee),"\x36\x34\x10\xfc\x60\xc2\x5f\x34\x10\x45\x0e",11));
    CHECK(!memcmp(at(0xc68d62),"\x36\x34\x10\xe8\x60\xc2\x5f\x34\x10\x45\x0e",11));
    {
        const uint8 *r=at(TABLE_RIGHT),*l=at(TABLE_LEFT),*d=at(TABLE_DOWN),*u=at(TABLE_UP);
        for(unsigned c=1;c<=12;++c) CHECK(l[r[c]]==c && r[l[c]]==c && u[d[c]]==c && d[u[c]]==c && d[c]!=c);
        CHECK(r[5]==11 && r[11]==19 && r[19]==1 && l[1]==19 && r[10]==12 && r[12]==6 && l[6]==12);
        CHECK(d[11]==12 && u[11]==12 && d[19]==19 && u[19]==19 && !r[13] && !d[13]);
    }
    /* Routines and calls. */
    CHECK(!memcmp(at(CLAMP),kClamp,sizeof(kClamp)) && !memcmp(at(STAGE_INIT),kStageInit,sizeof(kStageInit)));
    CHECK(at(STAGE_INIT)[5]==0x28 && at(STAGE_INIT)[6]==0xf8 && at(CLAMP)[2]==0x1f);
    CHECK(!memcmp(at(SET_CHARACTER),kSetCharacter,sizeof(kSetCharacter)));
    CHECK(!memcmp(at(0xc213e7),"\x22\x88\x60\xc2",4) && !memcmp(at(0xc21356),"\x22\x88\x60\xc2",4));
    CHECK(!memcmp(at(0xc42a3c),"\x22\x94\x60\xc2\xea",5) && !memcmp(at(0xc43bdf),"\x22\xa4\x60\xc2\xea\xea",6));
    unsigned count=patch_count;ExtraCharactersBeforeFrame();CHECK(patch_count==count); /* Installed once. */
    /* Cells 10/11/18 hold their choices, 12-17 none (no Bomber Kings); the
     * title clears remembered seats. */
    g_ram[0x1031]=4;g_ram[0x1f828]=11;ExtraCharactersBeforeFrame();
    CHECK(g_ram[0x1f80a]==11 && g_ram[0x1f80b]==12 && g_ram[0x1f812]==19 && !g_ram[0x1f80c] && g_ram[0x1f828]==11);
    g_ram[0x1031]=2;ExtraCharactersBeforeFrame();CHECK(!g_ram[0x1f828]);
    CHECK(ExtraBaseChoice(11)==1 && ExtraBaseChoice(12)==1 && ExtraBaseChoice(10)==10);
    CHECK(ExtraBaseChoice(18)==1 && ExtraBaseChoice(19)==1 && ExtraBaseChoice(20)==20);
    CHECK(ExtraCharacterColorCount(11)==1 && ExtraCharacterColorCount(19)==1 && ExtraCharacterColorCount(15)==5 && ExtraCharacterColorCount(3)==0);
    ExtraSetSeatChoice(3,12);CHECK(g_ram[0x1f82b]==12 && ExtraCharacterSeatChoice(3)==12);
    /* Reset restores every byte. */
    reset_callback();CHECK(!ExtraCharactersEnabled() && !installed && !memcmp(stock_rom,cartridge,sizeof(stock_rom)));
    /* A different image (unexpected bytes) is left untouched. */
    at(0xc68c02)[0]=0xea;memcpy(stock_rom,cartridge,sizeof(stock_rom));
    activation();ExtraCharactersBeforeFrame();CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)) && !patched && !patch_count);
    reset_callback();stock_cartridge();
    /* A corrupt archive stream (wrong end) disables the feature. */
    at(ARCHIVE)[0]^=0x40;memcpy(stock_rom,cartridge,sizeof(stock_rom));
    activation();ExtraCharactersBeforeFrame();CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)) && !patched);
    reset_callback();stock_cartridge();memcpy(stock_rom,cartridge,sizeof(stock_rom));
    activation();ExtraCharactersBeforeFrame();CHECK(patched);

    Ppu *ppu=calloc(1,sizeof(Ppu));CHECK(ppu);ppu->obsel=3;
    Snapshot *before=malloc(sizeof(Snapshot));CHECK(before);
    for(unsigned i=0;i<0x8000;++i) ppu->vram[i]=(uint16)(i*7);
    /* Stage: seat 0 is Honey (object 8, palette slot 1) walking right. Its
     * native frame ($C4:F000) has three pieces matching sprites 10-12. */
    memset(g_ram,0,sizeof(g_ram));g_ram[0x1031]=0x0d;g_ram[0x1f828]=11;
    memset(g_ram+0x17fa,0xff,5);g_ram[0x17fa]=8;g_ram[0x6df+8]=1;g_ram[0x55f+8]=100;g_ram[0x5bf+8]=100;
    g_ram[0x79f+8]=1;g_ram[0x7ff+8]=2; /* walk right, step A */
    static const int walk[]={-8,-1,0x20, 0,-1,0x20, -8,-17,0x21};
    native_frame(0xc4f000,3,walk);g_ram[0x2eea+8]=0x00;g_ram[0x2eeb+8]=0xf0;g_ram[0x2f4a+8]=0xc4;
    memset(g_ram+0x35c5,0xff,5);
    hide_all(ppu);
    sprite(ppu,10,92,99,0x23,1,false);sprite(ppu,11,100,99,0x25,1,false);sprite(ppu,12,92,83,0x20,1,true);
    sprite(ppu,13,40,40,0x40,1,false); /* same palette, far away: untouched */
    sprite(ppu,14,104,88,0x44,1,true); /* same palette, nearby (mount, egg): untouched */
    snap(before,ppu);
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn);
    /* Native pieces hidden or reused; the replacement blocks show the
     * frame of animation $1E step 1: frame (0x1e%32)*4+2 = 122, in the free
     * palette 7 (slot 1 keeps its colors for the mount and eggs). */
    unsigned expected_frame=(0x1e%32)*4+2;
    CHECK(pose_frame(0,8,-1,0)==expected_frame);
    unsigned big_rows=0;
    for(unsigned s=10;s<=12;++s) if(visible(ppu,s)) {
        CHECK(big(ppu,s) && sprite_palette(ppu,s)==7 && sprite_x(ppu,s)==92);
        unsigned tile=ppu->oam[s*2+1]&0x1ff,y=ppu->oam[s*2]>>8;
        /* Block rows start at the first drawn line: the 16x16 piece at
         * dy -22, composed 2 lines up (canvas 18), is the first block row. */
        if(y==(unsigned)(100-42+18)) {
            unsigned v=(expected_frame%8)*2,want=(2*v)%15+1; /* VRAM tile v = archive tile 2v */
            CHECK(pixel(ppu,tile,4,0)==want);
        }
        ++big_rows;
    }
    CHECK(big_rows==2);
    CHECK(visible(ppu,13) && (ppu->oam[13*2+1]&0x1ff)==0x40);
    CHECK(visible(ppu,14) && ppu->oam[14*2]==before->oam[14*2] && ppu->oam[14*2+1]==before->oam[14*2+1]);
    for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+7*16+c]==palette_color(0,c) && ppu->cgram[0x80+16+c]==before->cgram[0x80+16+c]);
    ExtraCharactersObserve(ppu,224,NULL);CHECK(!drawn && same(before,ppu));
    /* Every palette in use: slot 1 itself is recolored (fallback). */
    for(unsigned p=0;p<8;++p) if(p!=1) sprite(ppu,20+p,8,8+p*16,0x60,p,false);
    snap(before,ppu);
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn && big(ppu,10) && sprite_palette(ppu,10)==1);
    for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+16+c]==palette_color(0,c));
    ExtraCharactersObserve(ppu,224,NULL);CHECK(same(before,ppu));
    for(unsigned p=0;p<8;++p) if(p!=1) ppu->oam[(20+p)*2]=0xe000;
    /* Riding ($1782 bit 7): the native frame ends 17 lines above the
     * object; the figure (no shadow) stands with its feet there, and the
     * mount stays. */
    static const int ride[]={-16,-33,0x21, -16,-49,0x21};
    native_frame(0xc4f000,2,ride);g_ram[0x79f+8]=0x0e;g_ram[0x1782]=0x80;
    hide_all(ppu);sprite(ppu,10,84,67,0x23,1,true);sprite(ppu,11,84,51,0x25,1,true);
    sprite(ppu,14,96,80,0x44,1,true);
    snap(before,ppu);
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn);
    unsigned lowest=0;
    for(unsigned s=10;s<=11;++s) if(visible(ppu,s)) { unsigned y=ppu->oam[s*2]>>8;if(y>lowest) lowest=y; }
    CHECK(lowest==(unsigned)(100-42-16+34) && visible(ppu,14) && sprite_palette(ppu,14)==1);
    ExtraCharactersObserve(ppu,224,NULL);CHECK(same(before,ppu));
    /* Victory: the champion frames alternate with their native durations. */
    native_frame(0xc4f000,3,walk);g_ram[0x79f+8]=0x20;g_ram[0x1782]=0;
    hide_all(ppu);
    sprite(ppu,10,92,99,0x23,1,false);sprite(ppu,11,100,99,0x25,1,false);sprite(ppu,12,92,83,0x20,1,true);
    snap(before,ppu);
    unsigned shown[2];
    for(unsigned k=0;k<2;++k) {
        snes_frame_counter=k?7:0;
        ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn);
        int s=-1;for(unsigned i=10;i<=12;++i) if(visible(ppu,i)) s=(int)i;
        CHECK(s>=0 && sprite_x(ppu,(unsigned)s)==92);
        /* Champion frame f = tile 2f, pixel (4f+7)%15+1; the piece at canvas
         * (8, 25..40) is the only block row, at y 100-42+25. */
        CHECK((ppu->oam[s*2]>>8)==83);
        shown[k]=pixel(ppu,ppu->oam[s*2+1]&0x1ff,0,0);
        ExtraCharactersObserve(ppu,224,NULL);CHECK(same(before,ppu));
    }
    CHECK(shown[0]==(4*1+7)%15+1 && shown[1]==(4*2+7)%15+1); /* tile 2f = archive tile 4f */
    snes_frame_counter=0;
    /* Non-walking animation (outside the four-way groups) shows the spin. */
    int dir;
    CHECK(bomber_pose(0x16,&dir)==POSE_DEATH && bomber_pose(0x21,&dir)==POSE_STUN && bomber_pose(0x60,&dir)==POSE_SPIN);
    CHECK(bomber_pose(0x47,&dir)==POSE_STAND && dir==1 && bomber_pose(3,&dir)==POSE_WALK && dir==2);
    CHECK(bomber_pose(0x31,&dir)==POSE_CARRY_WALK && dir==2 && bomber_pose(0x28,&dir)==POSE_ACTION && dir==1);
    CHECK(bomber_pose(0x36,&dir)==POSE_ACTION && dir==3 && bomber_pose(0x20,&dir)==POSE_VICTORY);
    CHECK(bomber_pose(0x2c,&dir)==POSE_CARRY && dir==1 && bomber_pose(0x44,&dir)==POSE_STAND && dir==2);
    CHECK(bomber_pose(0x1f,&dir)==POSE_TIME_UP && bomber_pose(0x18,&dir)==POSE_AIR && dir==1);
    /* Picker: seat 0 hovers Honey; figures for cells 10/11 and the seat
     * portrait use borrowed blocks; all restored at line 224. */
    memset(g_ram,0,sizeof(g_ram));g_ram[0x1031]=4;g_ram[0xc90]=0x68;g_ram[0xcb8]=0x8e;g_ram[0xce0]=0xc6;
    g_ram[0x625f]=11;g_ram[0x62bf]=12;g_ram[0x6df+12]=0;g_ram[0x55f+12]=40;g_ram[0x5bf+12]=72;
    hide_all(ppu);
    sprite(ppu,4,48,47,0x106,0,true);sprite(ppu,5,32,47,0x104,0,true);
    sprite(ppu,6,48,63,0x102,0,true);sprite(ppu,7,32,63,0x100,0,true);
    snap(before,ppu);
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn);
    for(unsigned s=4;s<=7;++s) CHECK((ppu->oam[s*2+1]&0x1ff)!=(before->oam[s*2+1]&0x1ff));
    for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+c]==palette_color(0,c));
    /* Honey/Kotetsu in column six (tile 23), Terrorin in column seven (27). */
    unsigned figures=0,column6=0,column7=0;
    for(unsigned s=0;s<128;++s) if(s<4 || s>7) if(visible(ppu,s)) {
        unsigned x=sprite_x(ppu,s);CHECK(x==184 || (x>=208 && x<232));
        column6+=x==184;column7+=x>=208;++figures;
    }
    CHECK(column6>=2 && column7>=1 && visible_count(ppu)==4+figures);
    ExtraCharactersObserve(ppu,224,NULL);CHECK(same(before,ppu));
    memcpy(at(0xc4f000),stock_rom+file(0xc4f000),32); /* test-written native frames */
    reset_callback();CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)));

    /* Bomber Kings alone without a Super Bomberman 4 image: nothing to add. */
    kings_wanted=true;kings_ready=false;
    ExtraCharactersBeforeFrame();CHECK(!patched && ExtraCharactersEnabled() && PickerCellCount()==10);
    CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)));
    reset_callback();kings_wanted=false;
    /* Bomber Kings alone with the image: three rows of six (BG rows
     * 12/16/20); the sixth column only holds Great Bomber. */
    kings_wanted=kings_ready=true;
    ExtraCharactersBeforeFrame();CHECK(patched && rows==3 && cols==6 && PickerCellCount()==19);
    static const uint8_t three_rows[]={12,12,12,12,12,16,16,16,16,16},six[]={5,9,13,17,21,5,9,13,17,21};
    CHECK(!memcmp(at(0xc212dc),three_rows,10) && !memcmp(at(0xc212d2),six,10));
    {
        const uint8 *r=at(TABLE_RIGHT),*l=at(TABLE_LEFT),*d=at(TABLE_DOWN),*u=at(TABLE_UP);
        CHECK(r[5]==1 && l[1]==5 && r[10]==6 && d[1]==6 && d[6]==13 && d[13]==1 && u[1]==13);
        CHECK(d[18]==18 && u[18]==18 && r[17]==18 && r[18]==13 && !r[11] && !r[19]);
        CHECK(at(TABLE_CURSOR_X)[12]==40 && at(TABLE_CURSOR_X)[17]==200);
    }
    reset_callback();CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)));
    /* Both mods: three rows of seven, 19 cells. */
    activation();
    ExtraCharactersBeforeFrame();CHECK(patched && rows==3 && cols==7 && PickerCellCount()==19);
    CHECK(!memcmp(at(0xc212dc),three_rows,10));
    {
        const uint8 *r=at(TABLE_RIGHT),*l=at(TABLE_LEFT),*d=at(TABLE_DOWN),*u=at(TABLE_UP);
        for(unsigned c=1;c<=19;++c) CHECK(l[r[c]]==c && r[l[c]]==c && u[d[c]]==c && d[u[c]]==c);
        CHECK(d[1]==6 && d[6]==13 && d[13]==1 && u[1]==13 && d[11]==12 && d[12]==18 && d[18]==11 && r[17]==18 && r[18]==13);
        CHECK(d[19]==19 && r[11]==19 && r[19]==1);
        CHECK(at(TABLE_CURSOR_Y)[0]==12*8+24 && at(TABLE_CURSOR_Y)[5]==16*8+24 && at(TABLE_CURSOR_Y)[12]==20*8+24);
        CHECK(at(TABLE_CURSOR_X)[12]==24 && at(TABLE_CURSOR_X)[17]==184 && at(TABLE_CURSOR_X)[18]==216);
    }
    g_ram[0x1031]=4;ExtraCharactersBeforeFrame();
    for(unsigned c=10;c<19;++c) CHECK(g_ram[0x1f800+c]==c+1);
    /* Picker grid: Honey and Terrorin (row 1), Kotetsu (row 2) and six Super
     * Bomberman 4 figures (row 3), each row's palettes loaded on its first line. */
    memset(g_ram+0x1f814,0,18);g_ram[0x1f814+14]=3; /* Jet's cell shows color 3 */
    g_ram[0xc90]=0x68;g_ram[0xcb8]=0x8e;g_ram[0xce0]=0xc6;memset(g_ram+0x625f,1,5);
    hide_all(ppu);snap(before,ppu);
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn);
    unsigned row3=0,first_line=999;
    for(unsigned s=0;s<128;++s) if(visible(ppu,s)) {
        unsigned y=ppu->oam[s*2]>>8;
        if(y>=150 && y<200) { ++row3;if(y<first_line) first_line=y; }
    }
    CHECK(row3>=12); /* six 32-line figures: two 16x16 blocks each at least */
    unsigned loads_at_row3=0;
    for(unsigned k=0;k<line_load_count;++k) if(line_loads[k].line==first_line) ++loads_at_row3;
    CHECK(loads_at_row3==6);
    for(unsigned k=0;k<line_load_count;++k) if(line_loads[k].key==(2+2)*8+3) {
        unsigned p=line_loads[k].palette;
        CHECK(ppu->cgram[0x80+p*16+5]!=Sb4Color(2,3,5) || p==7);
        ExtraCharactersObserve(ppu,first_line,NULL);
        CHECK(ppu->cgram[0x80+p*16+5]==Sb4Color(2,3,5));
    }
    ExtraCharactersObserve(ppu,224,NULL);CHECK(!drawn && same(before,ppu));
    /* A full picker: only four idle OAM entries. The third row reuses the
     * entries of native sprites that end above it, written when the beam
     * reaches the row and restored on line 224. */
    hide_all(ppu);
    for(unsigned k=0;k<124;++k) sprite(ppu,k,(int)(k%32)*8,30+(int)(k/32)*8,0x01,0,false);
    snap(before,ppu);
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn && late_count>0);
    unsigned late_line0=(unsigned)late_places[0].line;
    CHECK(late_line0>100 && (ppu->oam[late_places[0].slot*2]>>8)<64);   /* still native */
    for(unsigned line=2;line<224;++line) ExtraCharactersObserve(ppu,line,NULL);
    row3=0;
    for(unsigned s=0;s<128;++s) if(visible(ppu,s) && (ppu->oam[s*2]>>8)>=150 && (ppu->oam[s*2]>>8)<200) ++row3;
    CHECK(row3>=12);
    ExtraCharactersObserve(ppu,224,NULL);CHECK(!drawn && same(before,ppu));
    /* Stage: seat 0 plays Hammer (choice 14) in color 2, drawn from frame
     * 3 (stand facing right) with the image's palette 2 in a free palette. */
    memset(g_ram,0,sizeof(g_ram));g_ram[0x1031]=0x74;g_ram[0x1f828]=14;g_ram[0x1f82d]=2;
    memset(g_ram+0x17fa,0xff,5);g_ram[0x17fa]=8;g_ram[0x6df+8]=1;g_ram[0x55f+8]=100;g_ram[0x5bf+8]=100;
    g_ram[0x79f+8]=0x46;g_ram[0x7ff+8]=0;
    native_frame(0xc4f000,3,walk);g_ram[0x2eea+8]=0x00;g_ram[0x2eeb+8]=0xf0;g_ram[0x2f4a+8]=0xc4;
    memset(g_ram+0x35c5,0xff,5);
    hide_all(ppu);
    sprite(ppu,10,92,99,0x23,1,false);sprite(ppu,11,100,99,0x25,1,false);sprite(ppu,12,92,83,0x20,1,true);
    snap(before,ppu);
    CHECK(sb4_pose_frame(8,false,false)==3 && sb4_pose_frame(8,true,false)==17 && sb4_pose_frame(8,true,true)==21);
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn);
    unsigned shown_blocks=0;int fig_top=256,fig_bottom=-1;
    for(unsigned s=10;s<=12;++s) if(visible(ppu,s)) {
        /* The seat's own slot, recolored only over the figure's lines. */
        CHECK(sprite_palette(ppu,s)==1);++shown_blocks;
        CHECK(pixel(ppu,ppu->oam[s*2+1]&0x1ff,0,4)==(3+1)%15+1); /* body column 0 of each block */
        int y=ppu->oam[s*2]>>8;if(y<fig_top) fig_top=y;if(y+15>fig_bottom) fig_bottom=y+15;
    }
    CHECK(shown_blocks>=2 && fig_top>1);
    uint16_t native1[16];memcpy(native1,&ppu->cgram[0x90],sizeof(native1));
    for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+16+c]==native1[c] && ppu->cgram[0x80+16+c]!=Sb4Color(1,2,c));
    for(int line=2;line<=fig_bottom+1;++line) {
        ExtraCharactersObserve(ppu,(unsigned)line,NULL);
        if(line<fig_top || line==fig_bottom+1)
            for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+16+c]==native1[c]);
        if(line==fig_top || line==fig_top+20)
            for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+16+c]==Sb4Color(1,2,c));
    }
    ExtraCharactersObserve(ppu,224,NULL);CHECK(same(before,ppu));
    /* Another sprite in the seat's slot on those lines (its mount): the
     * figure takes a free palette for the whole frame instead. */
    sprite(ppu,13,92,107,0x30,1,true);snap(before,ppu);
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn);
    for(unsigned s=10;s<=12;++s) if(visible(ppu,s)) CHECK(sprite_palette(ppu,s)==7);
    for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+7*16+c]==Sb4Color(1,2,c) && ppu->cgram[0x90+c]==native1[c]);
    ExtraCharactersObserve(ppu,224,NULL);CHECK(same(before,ppu));
    hide(ppu,13);snap(before,ppu);
    /* During an ability the frame the ability names is drawn instead. */
    ability_pose=1;ability_frame=Sb4FramePc(1,22);
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn);
    for(unsigned s=10;s<=12;++s) if(visible(ppu,s)) CHECK(pixel(ppu,ppu->oam[s*2+1]&0x1ff,0,4)==(22+1)%15+1);
    ability_pose=-1;ability_frame=0;
    ExtraCharactersObserve(ppu,224,NULL);CHECK(same(before,ppu));
    /* Bad Bomber ($15A2 bit 7, cart anim $43 facing down): the SB4 Bad
     * Bomber frame for that side; throwing ($3C) its throwing frame. */
    g_ram[0x15a2]=0x99;g_ram[0x79f+8]=0x43;
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn);
    for(unsigned s=10;s<=12;++s) if(visible(ppu,s)) CHECK(pixel(ppu,ppu->oam[s*2+1]&0x1ff,0,4)==(41+1)%15+1);
    ExtraCharactersObserve(ppu,224,NULL);CHECK(same(before,ppu));
    g_ram[0x79f+8]=0x3c;
    ExtraCharactersObserve(ppu,1,NULL);
    for(unsigned s=10;s<=12;++s) if(visible(ppu,s)) CHECK(pixel(ppu,ppu->oam[s*2+1]&0x1ff,0,4)==(45+1)%15+1);
    ExtraCharactersObserve(ppu,224,NULL);CHECK(same(before,ppu));
    g_ram[0x15a2]=0;g_ram[0x79f+8]=0x46;
    /* Picker portrait of a Super Bomberman 4 seat: its own portrait and
     * portrait palette (Hammer, color 2). */
    g_ram[0x1031]=4;g_ram[0xc90]=0x68;g_ram[0xcb8]=0x8e;g_ram[0xce0]=0xc6;
    g_ram[0x625f]=14;g_ram[0x62bf]=12;g_ram[0x6df+12]=0;g_ram[0x55f+12]=40;g_ram[0x5bf+12]=72;
    hide_all(ppu);
    sprite(ppu,4,48,47,0x106,0,true);sprite(ppu,5,32,47,0x104,0,true);
    sprite(ppu,6,48,63,0x102,0,true);sprite(ppu,7,32,63,0x100,0,true);
    snap(before,ppu);
    saved_count=0;borrowed_count=0;memcpy(saved_high,ppu->highOam,sizeof(saved_high));
    draw_portraits(ppu);drawn=true;
    for(unsigned s=4;s<=7;++s) CHECK(pixel(ppu,ppu->oam[s*2+1]&0x1ff,3,3)==9);
    for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+c]==Sb4PortraitColor(1,2,c));
    /* Stage: seat 0 plays Terrorin (choice 19), walking down: frames of his
     * set $0E walk ($3A), his palette in a free palette; the ability shows
     * his guarded spin ($52) and a Clock paralysis his dazed frames ($50). */
    memset(g_ram,0,sizeof(g_ram));g_ram[0x1031]=0x74;g_ram[0x1f828]=19;
    memset(g_ram+0x17fa,0xff,5);g_ram[0x17fa]=8;g_ram[0x6df+8]=1;g_ram[0x55f+8]=100;g_ram[0x5bf+8]=100;
    g_ram[0x79f+8]=2;g_ram[0x7ff+8]=10;
    native_frame(0xc4f000,3,walk);g_ram[0x2eea+8]=0x00;g_ram[0x2eeb+8]=0xf0;g_ram[0x2f4a+8]=0xc4;
    memset(g_ram+0x35c5,0xff,5);
    hide_all(ppu);
    sprite(ppu,10,92,99,0x23,1,false);sprite(ppu,11,100,99,0x25,1,false);sprite(ppu,12,92,83,0x20,1,true);
    sprite(ppu,13,92,107,0x30,1,true); /* the mount: a free palette for him */
    snap(before,ppu);
    CHECK(skin_of(19)==SKIN_TERRORIN && terrorin_pose_frame(0,8,-1,0)==terrorin_frame(T_WALK+1,4));
    CHECK(terrorin_frame(T_WALK+1,4)==(0x3a-0x38)*20+5);
    ExtraCharactersObserve(ppu,1,NULL);CHECK(drawn);
    unsigned tblocks=0;
    for(unsigned s2=10;s2<=12;++s2) if(visible(ppu,s2)) { CHECK(big(ppu,s2) && sprite_palette(ppu,s2)==7);++tblocks; }
    CHECK(tblocks>=2);
    for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+7*16+c]==(0x3000+c*5));
    ExtraCharactersObserve(ppu,224,NULL);CHECK(same(before,ppu));
    ability_pose=1;CHECK(terrorin_pose_frame(0,8,1,13)==terrorin_frame(T_SPIN_GUARD,2));ability_pose=-1;
    dazed=true;CHECK(terrorin_pose_frame(0,8,-1,0)>=terrorin_frame(T_DAZED,7));dazed=false;
    g_ram[0x79f+8]=0x20;CHECK(terrorin_pose_frame(0,8,-1,0)==terrorin_frame(T_CHEER,0) || terrorin_pose_frame(0,8,-1,0)==terrorin_frame(T_CHEER,1));
    g_ram[0x1031]=0x74;
    memcpy(at(0xc4f000),stock_rom+file(0xc4f000),32);
    reset_callback();kings_wanted=kings_ready=false;CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)));
    free(before);free(ppu);
    puts("extra_characters_tests: ok");
    return 0;
}
