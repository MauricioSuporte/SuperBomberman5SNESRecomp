/* Native tests for the Subordinate Bomber mod. Synthetic cartridge bytes only:
 * the patched locations hold the expected stock instructions, and the
 * "sprite" and palettes are generated patterns, not cartridge data. */
#include "mod_runtime.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static SNESModActivationCallback activation,reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn;return 1; }
int snes_mod_register_activation_plugin(const char *id,SNESModActivationCallback fn) { (void)id;(void)fn;return 0; }
#include "../src/subordinate_mod.c"
/* The Super Bomberman 5 Extra Characters plugin turns the module on. */
static void extras_plugin(void) { SubordinateActivate(); }
uint8 g_ram[0x20000];
/* Extra-character core not linked: stock ten-cell picker arrays. */
uint8_t *PickerCellCharacters(void) { return g_ram+0x62c9; }
uint8_t *PickerCellColors(void) { return g_ram+0x62d3; }
unsigned PickerCellCount(void) { return 10; }
unsigned ExtraBaseChoice(unsigned choice) { return choice>=11 && choice<=19 ? 1 : choice; }
unsigned hk_seat_choice[5];
void ExtraSetSeatChoice(unsigned seat,unsigned choice) { if(seat<5) hk_seat_choice[seat]=choice; }
static uint8 cartridge[0x400000];
const uint8 *g_rom=cartridge;
uint8_t *SnesRomPtr(uint32_t pc) { return cartridge+(pc&0x3fffff); }
static Cart cart;
static Snes machine;
Snes *g_snes=&machine;
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static uint8 stock[0x400000];
static void put_bytes(uint32_t pc,const uint8_t *b,unsigned n) { memcpy(SnesRomPtr(pc),b,n); }
static void stock_cartridge(void) {
    memset(cartridge,0xff,sizeof(cartridge));
    static const uint8_t cell[]={0x4f,0x00,0x4a},color[]={0x00};
    static const uint8_t unlocked[]={0x4f,0x01,0x4a},unlocked_color[]={0x4f,0x05,0x4a};
    static const uint8_t x_draw[]={0xce,0x28,0x1d,0xa9,0x90,0x22,0x09,0x00,0xc0};
    static const uint8_t lookup_face[]={0xbf,0x70,0x14,0xc2},lookup_alt[]={0xbf,0xfe,0x12,0xc5};
    put_bytes(0xc68b01,unlocked,3);put_bytes(0xc68b0f,unlocked_color,3);
    put_bytes(0xc68b1f,cell,3);put_bytes(0xc68b2e,color,1);put_bytes(0xc21276,x_draw,9);
    put_bytes(0xc5129b,lookup_face,4);put_bytes(0xc51319,lookup_face,4);put_bytes(0xc512ee,lookup_alt,4);
    for(unsigned i=0;i<9;++i) { cartridge[0x21470+i]=(uint8)(0x14+i);cartridge[0x512fe + i]=(uint8)(0x1e + i); }
    /* Grid geometry, palette base and a palette table ($C1:8000). */
    cartridge[0x212d2+9]=0x19;cartridge[0x212dc+9]=0x14;cartridge[0xbb4e + 9]=0x48;
    cartridge[0x379]=0x00;cartridge[0x37a]=0x80;
    for(unsigned id=1;id<=0x4f;++id) {
        unsigned record=0x9000+id*32;
        cartridge[0x18000+(id-1)*2]=(uint8)record;cartridge[0x18000+(id-1)*2+1]=(uint8)(record>>8);
        for(unsigned c=0;c<9;++c) {
            unsigned v=id*16+c;cartridge[0x10000+record+4+c*2]=(uint8)v;cartridge[0x10000+record+5+c*2]=(uint8)(v>>8);
        }
    }
    /* Synthetic standing frame: color = row*3+col%3+1 in each 8x8 tile. */
    for(unsigned row=0;row<3;++row) {
        uint8 *t=cartridge+kFrameRows[row];memset(t,0,32);
        for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) {
            unsigned v=(row*3+x%3+1)&15;
            for(unsigned p=0;p<4;++p) if(v&(1u<<p)) t[(p/2)*16+y*2+(p%2)]|=(uint8)(1u<<(7-x));
        }
    }
}
static unsigned pixel(const Ppu *ppu,unsigned tile,unsigned x,unsigned y) {
    unsigned base=tile_address(ppu,tile),c=0;
    for(unsigned p=0;p<4;++p) c|=((ppu->vram[(base+y+(p/2)*8)&0x7fff]>>((7-x)+(p%2)*8))&1u)<<p;
    return c;
}
static unsigned tile_of(const Ppu *ppu,unsigned s) { return ppu->oam[s*2+1]&0x1ff; }
static void set_pc(uint32_t pc) { g_ram[0xc90]=(uint8)pc;g_ram[0xcb8]=(uint8)(pc>>8);g_ram[0xce0]=(uint8)(pc>>16); }
static void hide_all(Ppu *ppu) { for(unsigned s=0;s<128;++s) ppu->oam[s*2]=0xe000; }
static void sprite(Ppu *ppu,unsigned s,unsigned x,unsigned y,unsigned tile,unsigned palette,bool big16) {
    ppu->oam[s*2]=(uint16_t)((y<<8)|(x&255));
    ppu->oam[s*2+1]=(uint16_t)(((0x20|(palette<<1)|(tile>>8))<<8)|(tile&255));
    ppu->highOam[s/4]&=(uint8_t)~(3u<<((s%4)*2));
    if(big16) ppu->highOam[s/4]|=(uint8_t)(2u<<((s%4)*2));
}
typedef struct { uint16 vram[0x8000],oam[256],cgram[256];uint8 high[32]; } Snapshot;
static void snap(Snapshot *s,const Ppu *ppu) {
    memcpy(s->vram,ppu->vram,sizeof(s->vram));memcpy(s->oam,ppu->oam,sizeof(s->oam));
    memcpy(s->cgram,ppu->cgram,sizeof(s->cgram));memcpy(s->high,ppu->highOam,sizeof(s->high));
}
/* Quarter sprites TL, TR, BL, BR show the authored portrait shape. */
static bool portrait_ok(const Ppu *ppu,const unsigned quarter[4]) {
    for(unsigned y=0;y<32;++y) for(unsigned x=0;x<32;++x) {
        unsigned t=tile_of(ppu,quarter[(y/16)*2+x/16])+((x&15)>>3)+(((y&15)>>3)<<4);
        unsigned want=(x>=4 && x<28 && y>=4 && y<28)?(unsigned)(kPortrait[y-4][x-4]-'0'):0;
        if(pixel(ppu,t,x&7,y&7)!=want) return false;
    }
    return true;
}
static bool same(const Snapshot *s,const Ppu *ppu) {
    return !memcmp(s->vram,ppu->vram,sizeof(s->vram)) && !memcmp(s->oam,ppu->oam,sizeof(s->oam)) &&
        !memcmp(s->cgram,ppu->cgram,sizeof(s->cgram)) && !memcmp(s->high,ppu->highOam,sizeof(s->high));
}
int main(void) {
    activation=extras_plugin;CHECK(reset_callback);
    stock_cartridge();
    /* No cartridge yet: nothing is installed. */
    activation();SubordinateBeforeFrame();CHECK(!installed);reset_callback();
    machine.cart=&cart;memcpy(stock,cartridge,sizeof(stock));
    /* Disabled: no cartridge byte changes. */
    SubordinateBeforeFrame();CHECK(!memcmp(stock,cartridge,sizeof(stock)) && !SubordinateEnabled());
    activation();SubordinateBeforeFrame();CHECK(SubordinateEnabled() && installed && patched);
    CHECK(cartridge[0x68b20]==10 && cartridge[0x68b2e]==6);
    CHECK(cartridge[0x68b02]==10 && cartridge[0x68b10]==6);
    CHECK(cartridge[0x21276]==0x20 && cartridge[0x21277]==0x00 && cartridge[0x21278]==0x60 && cartridge[0x21279]==0x80);
    CHECK(!memcmp(cartridge+0x26000,grid_x,sizeof(grid_x)));
    for(unsigned i=0;i<9;++i) CHECK(cartridge[0x26020+i]==0x14+i && cartridge[0x26030+i]==0x1e + i);
    CHECK(cartridge[0x26029]==FACE && cartridge[0x26039]==FACE_ALT);
    CHECK(cartridge[0x5129c]==0x20 && cartridge[0x5129d]==0x60 && cartridge[0x5129e]==0xc2);
    CHECK(cartridge[0x5131a]==0x20 && cartridge[0x512ef]==0x30);
    SubordinateBeforeFrame();CHECK(patch_count==11); /* Installed once. */
    reset_callback();CHECK(!SubordinateEnabled() && !installed && !memcmp(stock,cartridge,sizeof(stock)));
    /* A different image (unexpected bytes) is left untouched. */
    cartridge[0x21276]=0xea;memcpy(stock,cartridge,sizeof(stock));
    activation();SubordinateBeforeFrame();CHECK(!memcmp(stock,cartridge,sizeof(stock)) && !patch_count && !patched);
    reset_callback();stock_cartridge();memcpy(stock,cartridge,sizeof(stock));
    /* Reject an unexpected unlocked branch without installing any patch. */
    cartridge[0x68b02]=2;memcpy(stock,cartridge,sizeof(stock));
    activation();SubordinateBeforeFrame();CHECK(!patched && !patch_count && !memcmp(stock,cartridge,sizeof(stock)));
    reset_callback();stock_cartridge();memcpy(stock,cartridge,sizeof(stock));
    activation();SubordinateBeforeFrame();

    Ppu *ppu=calloc(1,sizeof(Ppu));CHECK(ppu);ppu->obsel=3;
    Snapshot *before=malloc(sizeof(Snapshot));CHECK(before);
    /* Picker: cell 9 holds character 10 in its rival color; seat 2 hovers it. */
    memset(g_ram,0,sizeof(g_ram));g_ram[0x1031]=4;set_pc(0xc68e68);
    g_ram[0x62c9+9]=10;g_ram[0x62d3+9]=6;
    g_ram[0x625f+1]=10;g_ram[0x62bf+1]=12;g_ram[0x6df+12]=1;g_ram[0x55f+12]=88;g_ram[0x5bf+12]=72;
    hide_all(ppu);
    sprite(ppu,4,96,47,0x106,1,true);sprite(ppu,5,80,47,0x104,1,true);
    sprite(ppu,6,96,63,0x102,1,true);sprite(ppu,7,80,63,0x100,1,true);
    for(unsigned i=0;i<0x8000;++i) ppu->vram[i]=(uint16)(i*7);
    snap(before,ppu);
    SubordinateObserve(ppu,1,NULL);CHECK(drawn);
    /* Grid figure: six 8x8 sprites at (200,167..183), mirrored right column,
     * OBJ palette 7 loaded with palette $4F (rival). */
    unsigned shown=0;
    for(unsigned s=0;s<128;++s) if((ppu->oam[s*2]>>8)<224 && s>=8) {
        unsigned x=ppu->oam[s*2]&255,y=ppu->oam[s*2]>>8,attr=ppu->oam[s*2+1]>>8,tile=ppu->oam[s*2+1]&0x1ff;
        CHECK((x==200 && !(attr&0x40)) || (x==208 && (attr&0x40)));
        CHECK(y==167 || y==175 || y==183);
        CHECK(((attr>>1)&7)==7 && tile>=0x1c0 && tile<0x1f0);
        if(x==200) CHECK(pixel(ppu,tile,0,0)==(y-167)/8*3+1 && pixel(ppu,tile,2,5)==(y-167)/8*3+3);
        ++shown;
    }
    CHECK(shown==6);
    for(unsigned c=1;c<9;++c) CHECK(ppu->cgram[0xf0+c]==0x4f*16+c);
    /* Portrait: the seat's quarter sprites now show borrowed tiles holding
     * the authored 24x24 shape at (4..27, 4..27), transparent around it
     * (frame and per-player background stay BG); slot 1 gets the white set.
     * Native portrait tiles are untouched. */
    const unsigned seat2[4]={5,4,7,6}; /* Quarters TL, TR, BL, BR. */
    CHECK(portrait_ok(ppu,seat2));
    for(unsigned q=0;q<4;++q) CHECK(tile_of(ppu,seat2[q])>=0x100 && tile_of(ppu,seat2[q])<0x1f0 &&
                                     tile_of(ppu,seat2[q])!=(before->oam[seat2[q]*2+1]&0x1ffu));
    CHECK(!memcmp(ppu->vram+tile_address(ppu,0x100),before->vram+tile_address(ppu,0x100),0x80*2));
    for(unsigned c=0;c<9;++c) CHECK(ppu->cgram[0x90+1+c]==kPortraitColors[0][c]);
    CHECK(ppu->cgram[0x90]==before->cgram[0x90] && ppu->cgram[0x90+10]==before->cgram[0x90+10]);
    CHECK(kPortrait[0][0]=='0' && kPortrait[0][7]=='1' && kPortrait[23][0]=='9');
    SubordinateObserve(ppu,224,NULL);CHECK(!drawn && same(before,ppu));
    /* A preview color from L/R. */
    g_ram[0x6264+1]=3;snap(before,ppu);SubordinateObserve(ppu,1,NULL);
    for(unsigned c=0;c<9;++c) CHECK(ppu->cgram[0x90+1+c]==kPortraitColors[3][c]);
    SubordinateObserve(ppu,224,NULL);CHECK(same(before,ppu));
    /* Another visible sprite on the seat palette: portrait left alone. */
    sprite(ppu,43,10,100,0x30,1,false);snap(before,ppu);SubordinateObserve(ppu,1,NULL);
    CHECK(ppu->cgram[0x91]==before->cgram[0x91] && tile_of(ppu,5)==0x104);
    SubordinateObserve(ppu,224,NULL);CHECK(same(before,ppu));ppu->oam[43*2]=0xe000;
    /* Bomberman's portrait shows the same native tiles in seat 1 (palette 0):
     * both stay correct, since this seat now shows borrowed tiles. */
    g_ram[0x625f]=1;
    sprite(ppu,0,64,47,0x106,0,true);sprite(ppu,1,48,47,0x104,0,true);
    sprite(ppu,2,64,63,0x102,0,true);sprite(ppu,3,48,63,0x100,0,true);
    snap(before,ppu);SubordinateObserve(ppu,1,NULL);
    CHECK(portrait_ok(ppu,seat2));
    for(unsigned q=0;q<4;++q) CHECK(tile_of(ppu,q)==(before->oam[q*2+1]&0x1ffu));
    CHECK(!memcmp(ppu->vram+tile_address(ppu,0x100),before->vram+tile_address(ppu,0x100),0x80*2));
    CHECK(ppu->cgram[0x81]==before->cgram[0x81]);
    SubordinateObserve(ppu,224,NULL);CHECK(same(before,ppu));
    /* Two seats on this bomber (white confirmed, black hovering): one shared
     * copy of the shape, each seat with its own palette. */
    g_ram[0x625f]=10;g_ram[0x6264]=0;g_ram[0x6264+1]=1;
    g_ram[0x62bf]=10;g_ram[0x6df+10]=0;g_ram[0x55f+10]=56;g_ram[0x5bf+10]=72;
    snap(before,ppu);SubordinateObserve(ppu,1,NULL);
    const unsigned seat1[4]={1,0,3,2};
    CHECK(portrait_ok(ppu,seat1) && portrait_ok(ppu,seat2));
    for(unsigned q=0;q<4;++q) CHECK(tile_of(ppu,seat1[q])==tile_of(ppu,seat2[q]));
    for(unsigned c=0;c<9;++c) CHECK(ppu->cgram[0x80+1+c]==kPortraitColors[0][c] && ppu->cgram[0x90+1+c]==kPortraitColors[1][c]);
    SubordinateObserve(ppu,224,NULL);CHECK(same(before,ppu));
    for(unsigned q=0;q<4;++q) ppu->oam[q*2]=0xe000;
    g_ram[0x625f]=0;
    g_ram[0x62d3+9]=3;snap(before,ppu);
    SubordinateObserve(ppu,1,NULL);
    for(unsigned c=1;c<9;++c) CHECK(ppu->cgram[0xf0+c]==0x4b*16+c);
    SubordinateObserve(ppu,224,NULL);CHECK(same(before,ppu));
    /* Palettes 7 and 5 both in use: no grid figure, nothing else changes. */
    sprite(ppu,41,0,0,0x10,7,false);sprite(ppu,42,0,0,0x11,5,false);
    g_ram[0x625f+1]=2;snap(before,ppu);
    SubordinateObserve(ppu,1,NULL);CHECK(!drawn && same(before,ppu));
    /* No free 16x16 block of tiles: nothing is drawn. */
    ppu->oam[41*2]=ppu->oam[42*2]=0xe000;g_ram[0x625f+1]=10;
    for(unsigned s=8;s<120;++s) sprite(ppu,s,0,230,0x100+((s-8)*2)%0xf0,3,s%2==0);
    for(unsigned s=8;s<120;++s) { ppu->oam[s*2]=(uint16_t)((200<<8)|0);ppu->oam[s*2+1]=(uint16_t)((ppu->oam[s*2+1]&0xfe00)|(0x100+(s-8)*2)); }
    snap(before,ppu);SubordinateObserve(ppu,1,NULL);CHECK(same(before,ppu));
    SubordinateObserve(ppu,224,NULL);
    for(unsigned s=8;s<120;++s) ppu->oam[s*2]=0xe000;
    /* Outside the picker or with a stock X cell nothing is drawn. */
    g_ram[0x62c9+9]=0;snap(before,ppu);
    SubordinateObserve(ppu,1,NULL);CHECK(!drawn && same(before,ppu));
    /* Stage HUD: seats 2 and 3 are index 9, faces at (116,20) and (148,20);
     * seat 1's face shows the same native tiles (a Bomberman face frame). */
    memset(g_ram,0,sizeof(g_ram));g_ram[0x1031]=0x74;hide_all(ppu);
    for(unsigned i=1;i<3;++i) {
        unsigned o=12+(i-1)*2;
        g_ram[0x16c2+i]=9;g_ram[0x35c5+i]=(uint8)o;g_ram[0x4ff+o]=6;g_ram[0x79f+o]=FACE;
        g_ram[0x55f+o]=(uint8)(116+(i-1)*32);g_ram[0x5bf+o]=20;
    }
    g_ram[0x79f+14]=FACE_ALT;
    sprite(ppu,1,116,3,0x160,2,true);sprite(ppu,2,148,3,0x160,3,true);sprite(ppu,0,84,3,0x160,1,true);snap(before,ppu);
    SubordinateObserve(ppu,1,NULL);CHECK(drawn);
    unsigned alive=tile_of(ppu,1),dead=tile_of(ppu,2);
    CHECK(alive!=0x160 && dead!=0x160 && alive!=dead && tile_of(ppu,0)==0x160);
    CHECK(pixel(ppu,alive,0,0)==1 && pixel(ppu,alive+1,7,0)==1 && pixel(ppu,alive+16,1,0)==5);
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<16;++x)
        CHECK(pixel(ppu,dead+(x>>3)+((y>>3)<<4),x&7,y&7)==(unsigned)(kDefeatedFace[y][x]-'0'));
    CHECK(!memcmp(ppu->vram+tile_address(ppu,0x160),before->vram+tile_address(ppu,0x160),0x20*2));
    SubordinateObserve(ppu,224,NULL);CHECK(same(before,ppu));
    g_ram[0x79f+12]=g_ram[0x79f+14]=0x15;SubordinateObserve(ppu,1,NULL);CHECK(!drawn); /* Not our faces. */
    g_ram[0x79f+12]=FACE_ALT;g_ram[0x16c2+1]=g_ram[0x16c2+2]=3;SubordinateObserve(ppu,1,NULL);CHECK(!drawn);
    /* Disabled: overlay is inert and patches are gone. */
    g_ram[0x16c2+1]=9;reset_callback();SubordinateObserve(ppu,1,NULL);CHECK(!drawn && !memcmp(stock,cartridge,sizeof(stock)));
    free(before);free(ppu);
    puts("Subordinate Bomber: in-memory patches, restore on reset, picker figure, six authored portraits, HUD alive/defeated face overlays restored passed.");
    return 0;
}
