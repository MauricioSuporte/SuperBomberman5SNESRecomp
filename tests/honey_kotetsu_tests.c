/* Native tests for the Honey and Kotetsu mod. Synthetic cartridge bytes only:
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
    if(strcmp(id,"superbomberman5.honey-kotetsu.characters")) return 0;
    activation=fn;return 1;
}
int snes_frame_counter;
#include "../src/honey_kotetsu_mod.c"
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
static uint8 expected[ARCHIVE_SIZE],expected_champion[CHAMPION_SIZE];
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
    put_bytes(0xc68c02,right,sizeof(right));put_bytes(0xc68c78,left,sizeof(left));
    put_bytes(0xc68cee,vert,sizeof(vert));put_bytes(0xc68d62,vert,sizeof(vert));
    put_bytes(0xc213e7,portrait,4);put_bytes(0xc21356,portrait,4);put_bytes(0xc42a3c,init,5);
    put_bytes(0xc0c93a,cursor_x,4);put_bytes(0xc0c941,cursor_y,4);put_bytes(0xc212d2,grid_x,10);
    put_bytes(0xc43bdf,set,6);
    for(unsigned c=0;c<10;++c) at(0xc0c980)[c]=c<5?0x90:0xb8;
    at(0xc212dc)[0]=0x0f;at(0xc212dc)[5]=0x14;
    at(0xc21470)[0]=0x14;at(0xc512fe)[0]=0x1e;
    build_archive(ARCHIVE,ARCHIVE_END,expected,ARCHIVE_SIZE,0);
    build_archive(CHAMPION,CHAMPION_END,expected_champion,CHAMPION_SIZE,7);
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
    activation();HoneyKotetsuBeforeFrame();CHECK(!installed);reset_callback();
    machine.cart=&cart;memcpy(stock_rom,cartridge,sizeof(stock_rom));
    /* Disabled: nothing changes and the stock arrays are reported. */
    HoneyKotetsuBeforeFrame();CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)) && !HoneyKotetsuEnabled());
    CHECK(PickerCellCharacters()==g_ram+0x62c9 && PickerCellCount()==10);
    activation();HoneyKotetsuBeforeFrame();
    CHECK(installed && patched && art_ready && champion_ready && HoneyKotetsuInstalled());
    CHECK(!memcmp(archive,expected,sizeof(archive)) && !memcmp(champion,expected_champion,sizeof(champion)));
    CHECK(PickerCellCharacters()==g_ram+0x1f800 && PickerCellColors()==g_ram+0x1f80c && PickerCellCount()==12);
    for(unsigned i=0;i<sizeof(kCellCharRefs)/sizeof(*kCellCharRefs);++i)
        CHECK(at(kCellCharRefs[i])[0]==0x00 && at(kCellCharRefs[i])[1]==0xf8 && at(kCellCharRefs[i])[2]==0x7f);
    for(unsigned i=0;i<sizeof(kCellColorRefs)/sizeof(*kCellColorRefs);++i)
        CHECK(at(kCellColorRefs[i])[0]==0x0c && at(kCellColorRefs[i])[1]==0xf8 && at(kCellColorRefs[i])[2]==0x7f);
    /* Grid: five packed columns, column six at tile 25 for both rows. */
    static const uint8_t packed[]={5,9,13,17,21,5,9,13,17,21};
    CHECK(!memcmp(at(0xc212d2),packed,10));
    CHECK(at(TABLE_CURSOR_X)[4]==168 && at(TABLE_CURSOR_X)[10]==200 && at(TABLE_CURSOR_X)[11]==200);
    CHECK(at(TABLE_CURSOR_Y)[10]==0x90 && at(TABLE_CURSOR_Y)[11]==0xb8);
    CHECK(!memcmp(at(0xc0c93a),"\xbf\x70\x60\xc2",4) && !memcmp(at(0xc0c941),"\xbf\x7c\x60\xc2",4));
    /* Movement: lookups replace the 5x2 arithmetic; every cell is reachable
     * and right/left, up/down are inverses. */
    CHECK(!memcmp(at(0xc68c02),"\x36\x34\x10\x40\x60\xc2\x5f\x34\x10\x45\x10",11));
    CHECK(!memcmp(at(0xc68c78),"\x36\x34\x10\x50\x60\xc2\x5f\x34\x10\x45\x10",11));
    CHECK(!memcmp(at(0xc68cee),"\x36\x34\x10\x60\x60\xc2\x5f\x34\x10\x45\x0e",11));
    CHECK(!memcmp(at(0xc68d62),"\x36\x34\x10\x60\x60\xc2\x5f\x34\x10\x45\x0e",11));
    for(unsigned c=1;c<=12;++c) {
        CHECK(kLeft[kRight[c]]==c && kRight[kLeft[c]]==c && kVert[kVert[c]]==c && kVert[c]!=c);
    }
    CHECK(kRight[5]==11 && kRight[10]==12 && kVert[11]==12 && kLeft[1]==11 && kLeft[6]==12);
    /* Routines and calls. */
    CHECK(!memcmp(at(CLAMP),kClamp,sizeof(kClamp)) && !memcmp(at(STAGE_INIT),kStageInit,sizeof(kStageInit)));
    CHECK(!memcmp(at(SET_CHARACTER),kSetCharacter,sizeof(kSetCharacter)));
    CHECK(!memcmp(at(0xc213e7),"\x22\x88\x60\xc2",4) && !memcmp(at(0xc21356),"\x22\x88\x60\xc2",4));
    CHECK(!memcmp(at(0xc42a3c),"\x22\x94\x60\xc2\xea",5) && !memcmp(at(0xc43bdf),"\x22\xa4\x60\xc2\xea\xea",6));
    unsigned count=patch_count;HoneyKotetsuBeforeFrame();CHECK(patch_count==count); /* Installed once. */
    /* Cells 10/11 hold the choices; the title clears remembered seats. */
    g_ram[0x1031]=4;g_ram[0x1f818]=11;HoneyKotetsuBeforeFrame();
    CHECK(g_ram[0x1f80a]==11 && g_ram[0x1f80b]==12 && g_ram[0x1f818]==11);
    g_ram[0x1031]=2;HoneyKotetsuBeforeFrame();CHECK(!g_ram[0x1f818]);
    CHECK(HoneyKotetsuBaseChoice(11)==1 && HoneyKotetsuBaseChoice(12)==1 && HoneyKotetsuBaseChoice(10)==10);
    HoneyKotetsuSetSeatChoice(3,12);CHECK(g_ram[0x1f81b]==12);
    /* Reset restores every byte. */
    reset_callback();CHECK(!HoneyKotetsuEnabled() && !installed && !memcmp(stock_rom,cartridge,sizeof(stock_rom)));
    /* A different image (unexpected bytes) is left untouched. */
    at(0xc68c02)[0]=0xea;memcpy(stock_rom,cartridge,sizeof(stock_rom));
    activation();HoneyKotetsuBeforeFrame();CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)) && !patched && !patch_count);
    reset_callback();stock_cartridge();
    /* A corrupt archive stream (wrong end) disables the feature. */
    at(ARCHIVE)[0]^=0x40;memcpy(stock_rom,cartridge,sizeof(stock_rom));
    activation();HoneyKotetsuBeforeFrame();CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)) && !patched);
    reset_callback();stock_cartridge();memcpy(stock_rom,cartridge,sizeof(stock_rom));
    activation();HoneyKotetsuBeforeFrame();CHECK(patched);

    Ppu *ppu=calloc(1,sizeof(Ppu));CHECK(ppu);ppu->obsel=3;
    Snapshot *before=malloc(sizeof(Snapshot));CHECK(before);
    for(unsigned i=0;i<0x8000;++i) ppu->vram[i]=(uint16)(i*7);
    /* Stage: seat 0 is Honey (object 8, palette slot 1) walking right. Its
     * native frame ($C4:F000) has three pieces matching sprites 10-12. */
    memset(g_ram,0,sizeof(g_ram));g_ram[0x1031]=0x0d;g_ram[0x1f818]=11;
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
    HoneyKotetsuObserve(ppu,1,NULL);CHECK(drawn);
    /* Native pieces hidden or reused; the replacement blocks show the
     * frame of animation $1E step 1: frame (0x1e%32)*4+2 = 122, in the free
     * palette 7 (slot 1 keeps its colors for the mount and eggs). */
    unsigned expected_frame=(0x1e%32)*4+2;
    CHECK(pose_frame(0,8)==expected_frame);
    unsigned big_rows=0;
    for(unsigned s=10;s<=12;++s) if(visible(ppu,s)) {
        CHECK(big(ppu,s) && sprite_palette(ppu,s)==7 && sprite_x(ppu,s)==92);
        unsigned tile=ppu->oam[s*2+1]&0x1ff,y=ppu->oam[s*2]>>8;
        /* Block rows start at y-42 (+2 lift): the head block is row 1. */
        if(y==(unsigned)(100-42+16)) {
            unsigned v=(expected_frame%8)*2,want=(2*v)%15+1; /* VRAM tile v = archive tile 2v */
            CHECK(pixel(ppu,tile,4,6)==want); /* row 6 of the block = frame dy -22 + lift */
        }
        ++big_rows;
    }
    CHECK(big_rows==2);
    CHECK(visible(ppu,13) && (ppu->oam[13*2+1]&0x1ff)==0x40);
    CHECK(visible(ppu,14) && ppu->oam[14*2]==before->oam[14*2] && ppu->oam[14*2+1]==before->oam[14*2+1]);
    for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+7*16+c]==palette_color(0,c) && ppu->cgram[0x80+16+c]==before->cgram[0x80+16+c]);
    HoneyKotetsuObserve(ppu,224,NULL);CHECK(!drawn && same(before,ppu));
    /* Every palette in use: slot 1 itself is recolored (fallback). */
    for(unsigned p=0;p<8;++p) if(p!=1) sprite(ppu,20+p,8,8+p*16,0x60,p,false);
    snap(before,ppu);
    HoneyKotetsuObserve(ppu,1,NULL);CHECK(drawn && big(ppu,10) && sprite_palette(ppu,10)==1);
    for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+16+c]==palette_color(0,c));
    HoneyKotetsuObserve(ppu,224,NULL);CHECK(same(before,ppu));
    for(unsigned p=0;p<8;++p) if(p!=1) ppu->oam[(20+p)*2]=0xe000;
    /* Riding ($1782 bit 7): the native frame ends 17 lines above the
     * object; the figure (no shadow) stands with its feet there, and the
     * mount stays. */
    static const int ride[]={-16,-33,0x21, -16,-49,0x21};
    native_frame(0xc4f000,2,ride);g_ram[0x79f+8]=0x0e;g_ram[0x1782]=0x80;
    hide_all(ppu);sprite(ppu,10,84,67,0x23,1,true);sprite(ppu,11,84,51,0x25,1,true);
    sprite(ppu,14,96,80,0x44,1,true);
    snap(before,ppu);
    HoneyKotetsuObserve(ppu,1,NULL);CHECK(drawn);
    unsigned lowest=0;
    for(unsigned s=10;s<=11;++s) if(visible(ppu,s)) { unsigned y=ppu->oam[s*2]>>8;if(y>lowest) lowest=y; }
    CHECK(lowest==(unsigned)(100-42+32-16) && visible(ppu,14) && sprite_palette(ppu,14)==1);
    HoneyKotetsuObserve(ppu,224,NULL);CHECK(same(before,ppu));
    /* Victory: the champion frames alternate with their native durations. */
    native_frame(0xc4f000,3,walk);g_ram[0x79f+8]=0x20;g_ram[0x1782]=0;
    hide_all(ppu);
    sprite(ppu,10,92,99,0x23,1,false);sprite(ppu,11,100,99,0x25,1,false);sprite(ppu,12,92,83,0x20,1,true);
    snap(before,ppu);
    unsigned shown[2];
    for(unsigned k=0;k<2;++k) {
        snes_frame_counter=k?7:0;
        HoneyKotetsuObserve(ppu,1,NULL);CHECK(drawn);
        int s=-1;for(unsigned i=10;i<=12;++i) if(visible(ppu,i)) s=(int)i;
        CHECK(s>=0 && sprite_x(ppu,(unsigned)s)==92);
        /* Champion frame f = tile 2f, pixel (4f+7)%15+1; piece at canvas
         * (8, 25..40) inside the block row 1 (16..31) at y 74. */
        unsigned tile=ppu->oam[s*2+1]&0x1ff;
        CHECK((ppu->oam[s*2]>>8)==74 || (ppu->oam[s*2]>>8)==90);
        shown[k]=0;
        for(unsigned i=10;i<=12;++i) if(visible(ppu,i) && (ppu->oam[i*2]>>8)==74) shown[k]=pixel(ppu,(ppu->oam[i*2+1]&0x1ff)+16,0,7);
        (void)tile;
        HoneyKotetsuObserve(ppu,224,NULL);CHECK(same(before,ppu));
    }
    CHECK(shown[0]==(4*1+7)%15+1 && shown[1]==(4*2+7)%15+1); /* tile 2f = archive tile 4f */
    snes_frame_counter=0;
    /* Non-walking animation (outside the four-way groups) shows the spin. */
    int dir;
    CHECK(bomber_pose(0x16,&dir)==POSE_SPIN && bomber_pose(0x21,&dir)==POSE_SPIN);
    CHECK(bomber_pose(0x47,&dir)==POSE_STAND && dir==1 && bomber_pose(3,&dir)==POSE_WALK && dir==2);
    CHECK(bomber_pose(0x31,&dir)==POSE_WALK && dir==2 && bomber_pose(0x28,&dir)==POSE_ACTION && dir==1);
    CHECK(bomber_pose(0x36,&dir)==POSE_ACTION && dir==3 && bomber_pose(0x20,&dir)==POSE_VICTORY);
    CHECK(bomber_pose(0x2c,&dir)==POSE_STAND && dir==1 && bomber_pose(0x44,&dir)==POSE_STAND && dir==2);
    /* Picker: seat 0 hovers Honey; figures for cells 10/11 and the seat
     * portrait use borrowed blocks; all restored at line 224. */
    memset(g_ram,0,sizeof(g_ram));g_ram[0x1031]=4;g_ram[0xc90]=0x68;g_ram[0xcb8]=0x8e;g_ram[0xce0]=0xc6;
    g_ram[0x625f]=11;g_ram[0x62bf]=12;g_ram[0x6df+12]=0;g_ram[0x55f+12]=40;g_ram[0x5bf+12]=72;
    hide_all(ppu);
    sprite(ppu,4,48,47,0x106,0,true);sprite(ppu,5,32,47,0x104,0,true);
    sprite(ppu,6,48,63,0x102,0,true);sprite(ppu,7,32,63,0x100,0,true);
    snap(before,ppu);
    HoneyKotetsuObserve(ppu,1,NULL);CHECK(drawn);
    for(unsigned s=4;s<=7;++s) CHECK((ppu->oam[s*2+1]&0x1ff)!=(before->oam[s*2+1]&0x1ff));
    for(unsigned c=1;c<16;++c) CHECK(ppu->cgram[0x80+c]==palette_color(0,c));
    unsigned figures=0;
    for(unsigned s=0;s<128;++s) if(s<4 || s>7) if(visible(ppu,s)) { CHECK(sprite_x(ppu,s)==200);++figures; }
    CHECK(figures>=2 && visible_count(ppu)==4+figures);
    HoneyKotetsuObserve(ppu,224,NULL);CHECK(same(before,ppu));
    memcpy(at(0xc4f000),stock_rom+file(0xc4f000),32); /* test-written native frames */
    reset_callback();CHECK(!memcmp(stock_rom,cartridge,sizeof(stock_rom)));
    free(before);free(ppu);
    puts("honey_kotetsu_tests: ok");
    return 0;
}
