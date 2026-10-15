/* Subordinate Bomber as a selectable character.
 *
 * The cartridge already carries everything a bomber needs for index 9:
 * every per-character table read through $16C2 has a tenth entry, its
 * sprite set ($C0:BB44 -> $19) and palettes $48-$4D (six colors) plus the
 * rival palette $4F. Zone 3-D creates it as rival type 9. What the cartridge
 * lacks is picker grid art, a picker portrait and a HUD face. The grid
 * figure and HUD face are built at render time from the bomber's own
 * standing-frame tiles read from the player's cartridge; the portraits are
 * authored art (below). No cartridge graphics or palettes are stored.
 *
 * In-memory cartridge patches (restored on reset; the ROM file and SRAM
 * layout are untouched):
 *  - $C6:8B20: the picker script stores character 0 (the X cell) in grid
 *    cell 9 when the secret slot is locked; store 10 instead. $C6:8B2E sets
 *    that cell's preview color to 6, the rival palette, like the villains.
 *    The unlocked branch also stores 10/color 6 ($C6:8B02/$8B10),
 *    replacing the redundant yellow Bomberman without changing unlock flags.
 *  - $C2:1276: the grid drawer ($C2:122A) draws arrangement $90 (the X) for
 *    any character above 9; skip it for character 10 (trampoline $C2:6000).
 *  - $C5:129B/$C5:1319/$C5:12EE: the HUD face tables $C2:1470/$C5:12FE have
 *    nine entries; read ten-entry copies at $C2:6020/$C2:6030 instead. */
#include "subordinate_mod.h"
#include "honey_kotetsu_mod.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "mod_runtime.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include <string.h>

enum { CHARACTER = 10, INDEX = 9, CELL = 9, FACE = 0x1d, FACE_ALT = 0x27 };
/* Standing frame, front view: three 8x8 rows of the left half; the right
 * half is the same tile mirrored (verified from Zone 3-D OAM/VRAM dumps). */
static const uint32_t kFrameRows[3]={0x095800,0x096340,0x096360};

static bool enabled,installed,patched;
typedef struct { uint32_t pc; unsigned size; uint8_t original[24]; } Patch;
static Patch patches[12];
static unsigned patch_count;

bool SubordinateEnabled(void) { return enabled; }
static uint8_t *rom(uint32_t pc) { return SnesRomPtr(pc); }
static void patch(uint32_t pc,const uint8_t *bytes,unsigned size) {
    if(patch_count>=sizeof(patches)/sizeof(*patches) || size>sizeof(patches[0].original)) return;
    Patch *p=&patches[patch_count++];
    p->pc=pc;p->size=size;memcpy(p->original,rom(pc),size);memcpy(rom(pc),bytes,size);
}
static void uninstall(void) {
    while(patch_count) { Patch *p=&patches[--patch_count];memcpy(rom(p->pc),p->original,p->size); }
    installed=patched=false;
}
static const uint8_t grid_skip[]={
    0x20,0x00,0x60,   /* 1276 JSR $6000                              */
    0x80,0x04,        /* 1279 BRA $127F   next cell                  */
    0xea,0xea,0xea,0xea,
};
/* A holds the cell character - 1 here ($C2:1243). Not reading the cell
 * array keeps this working when the Honey/Kotetsu mod relocates it. */
static const uint8_t grid_x[]={
    0xc9,INDEX,          /* 6000 CMP #9     character 10             */
    0xf0,0x09,           /* 6002 BEQ $600D  no X for this bomber     */
    0xce,0x28,0x1d,      /* 6004 DEC $1D28  stock X arrangement      */
    0xa9,0x90,           /* 6007 LDA #$90                            */
    0x22,0x09,0x00,0xc0, /* 6009 JSL $C00009                         */
    0x60,                /* 600D RTS                                 */
};
static void install(void) {
    if(installed || !g_rom || !g_snes || !g_snes->cart) return;
    /* Only patch the verified image's expected bytes. */
    static const uint8_t cell_store[]={0x4f,0x00,0x4a},unlocked_store[]={0x4f,0x01,0x4a},unlocked_color[]={0x4f,0x05,0x4a};
    static const uint8_t x_draw[]={0xce,0x28,0x1d,0xa9,0x90,0x22,0x09,0x00,0xc0};
    if(memcmp(rom(0xc68b1f),cell_store,3) || rom(0xc68b2e)[0]!=0 ||
       memcmp(rom(0xc68b01),unlocked_store,3) || memcmp(rom(0xc68b0f),unlocked_color,3) || memcmp(rom(0xc21276),x_draw,9) ||
       rom(0xc5129b)[0]!=0xbf || rom(0xc51319)[0]!=0xbf || rom(0xc512ee)[0]!=0xbf) { installed=true;return; }
    uint8_t cell=CHARACTER,color=6;
    patch(0xc68b20,&cell,1);patch(0xc68b2e,&color,1);
    patch(0xc68b02,&cell,1);patch(0xc68b10,&color,1);
    patch(0xc26000,grid_x,sizeof(grid_x));patch(0xc21276,grid_skip,sizeof(grid_skip));
    uint8_t face[10],alt[10];
    memcpy(face,rom(0xc21470),9);face[INDEX]=FACE;
    memcpy(alt,rom(0xc512fe),9);alt[INDEX]=FACE_ALT;
    patch(0xc26020,face,10);patch(0xc26030,alt,10);
    static const uint8_t face_table[]={0x20,0x60,0xc2},alt_table[]={0x30,0x60,0xc2};
    patch(0xc5129c,face_table,3);patch(0xc5131a,face_table,3);patch(0xc512ef,alt_table,3);
    installed=true;patched=patch_count==11;
}
static void reset_plugin(void) { uninstall();enabled=false; }
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_subordinate) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.subordinate.character",activate);
}
void SubordinateBeforeFrame(void) { if(enabled) install(); }

/* ---- Render-only art: every VRAM/OAM/CGRAM word is restored at line 224. */
typedef struct { uint16_t *word; uint16_t value; } Saved;
static Saved saved[2304];
static unsigned saved_count;
static uint8_t saved_high[0x20];
static bool drawn;
static void put(uint16_t *word,uint16_t value) {
    if(saved_count>=sizeof(saved)/sizeof(*saved)) return;
    saved[saved_count++]=(Saved){word,*word};*word=value;
}
/* 16x24 figure, 4bpp color indices, decoded from the cartridge. */
static uint8_t figure[24][16];
static bool load_figure(void) {
    for(unsigned row=0;row<3;++row) {
        const uint8_t *t=g_rom+kFrameRows[row];
        for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) {
            unsigned v=0;
            for(unsigned p=0;p<4;++p) v|=((t[(p/2)*16+y*2+(p%2)]>>(7-x))&1u)<<p;
            figure[row*8+y][x]=figure[row*8+y][15-x]=(uint8_t)v;
        }
    }
    for(unsigned y=0;y<24;++y) for(unsigned x=0;x<16;++x) if(figure[y][x]) return true;
    return false;
}
static unsigned tile_address(const Ppu *ppu,unsigned tile) {
    return ((tile&0x100)?PPU_objTileAdr2(ppu):PPU_objTileAdr1(ppu))+(tile&255)*16;
}
/* Pixel canvas for one overlay target, up to a 32x32 portrait. */
static uint8_t canvas[32][32];
static void write_tile(Ppu *ppu,unsigned tile,unsigned cx,unsigned cy) {
    unsigned base=tile_address(ppu,tile);
    for(unsigned y=0;y<8;++y) for(unsigned pair=0;pair<2;++pair) {
        uint16_t value=0;
        for(unsigned x=0;x<8;++x) {
            unsigned c=canvas[cy+y][cx+x];
            for(unsigned plane=0;plane<2;++plane)
                if(c&(1u<<(pair*2+plane))) value|=(uint16_t)(1u<<((7-x)+plane*8));
        }
        put(&ppu->vram[(base+y+pair*8)&0x7fff],value);
    }
}
static bool visible(const Ppu *ppu,unsigned s) {
    unsigned y=ppu->oam[s*2]>>8;
    return y<224 || y>=240;
}
static bool big(const Ppu *ppu,unsigned s) { return (ppu->highOam[s/4]>>((s%4)*2+1))&1; }
static unsigned sprite_x(const Ppu *ppu,unsigned s) {
    return (ppu->oam[s*2]&255)|(((ppu->highOam[s/4]>>((s%4)*2))&1u)<<8);
}
/* True when a visible sprite other than `owner` shows tile `tile`. */
static bool tile_shown(const Ppu *ppu,unsigned tile,int owner) {
    for(unsigned s=0;s<128;++s) if((int)s!=owner && visible(ppu,s)) {
        unsigned t=ppu->oam[s*2+1]&0x1ff;
        if(t==tile) return true;
        if(big(ppu,s) && PPU_objSize(ppu)==0 && (t+1==tile || t+16==tile || t+17==tile)) return true;
    }
    return false;
}
/* Borrowed OBJ tiles: 16x16 blocks (t, t+1, t+16, t+17) that no visible
 * sprite shows this frame. Native sprites of this bomber are pointed at
 * them, so tiles the game shares between objects (two seats on the same
 * portrait, or a Bomberman face using the same frame) stay untouched.
 * $1F0-$1FF stay reserved for the color mod's cursor hint. */
static uint16_t borrowed[8];
static unsigned borrowed_count;
static bool tile_free(const Ppu *ppu,unsigned tile) {
    if(tile>=0x1f0 || tile_shown(ppu,tile,-1)) return false;
    for(unsigned i=0;i<borrowed_count;++i) {
        unsigned b=borrowed[i];
        if(tile==b || tile==b+1 || tile==b+16 || tile==b+17) return false;
    }
    return true;
}
static int borrow_block(const Ppu *ppu) {
    if(borrowed_count>=sizeof(borrowed)/sizeof(*borrowed)) return -1;
    for(unsigned t=0x1de;t>=0x100;--t) {
        if((t&15)==15) continue;
        if(tile_free(ppu,t) && tile_free(ppu,t+1) && tile_free(ppu,t+16) && tile_free(ppu,t+17)) {
            borrowed[borrowed_count++]=(uint16_t)t;return (int)t;
        }
    }
    return -1;
}
static void paint_block(Ppu *ppu,unsigned t,unsigned cx,unsigned cy) {
    for(unsigned q=0;q<4;++q) write_tile(ppu,t+(q&1)+(q>>1)*16,cx+(q&1)*8,cy+(q>>1)*8);
}
static void point_sprite(Ppu *ppu,unsigned s,unsigned tile) {
    put(&ppu->oam[s*2+1],(uint16_t)((ppu->oam[s*2+1]&~0x1ffu)|tile));
}
static uint32_t picker_pc(void) {
    return g_ram[0xc90]|((uint32_t)g_ram[0xcb8]<<8)|((uint32_t)g_ram[0xce0]<<16);
}
static bool picker(void) {
    uint32_t pc=picker_pc();
    return g_ram[0x1031]==4 && pc>=0xc68b48 && pc<0xc68ee4 && PickerCellCharacters()[CELL]==CHARACTER;
}
static unsigned palette_source(unsigned id) {
    unsigned table=g_rom[0x379]|g_rom[0x37a]<<8;
    unsigned entry=0x10000+table+(id-1)*2;
    return 0x10000+(g_rom[entry]|g_rom[entry+1]<<8)+4;
}
static void draw_grid(Ppu *ppu) {
    /* Cell 9's figure goes where the drawer puts 16x24 figures: tile
     * ($C2:12D2[9], $C2:12DC[9]+1) of BG1, i.e. pixels (200,168). */
    unsigned px=g_rom[0x212d2 + CELL]*8,py=(g_rom[0x212dc + CELL]+1)*8;
    unsigned scroll_x=ppu->hScroll[0]&0x3ff,scroll_y=ppu->vScroll[0]&0x3ff;
    px=(px-scroll_x)&0x1ff;py=(py-scroll_y)&0xff;
    if(px>240 || py>200) return;
    /* A free OBJ palette, three free tiles and six idle OAM slots. */
    unsigned palette=8;
    for(unsigned p=7;p>=5 && palette==8;p-=2) {
        bool used=false;
        for(unsigned s=0;s<128;++s) if(visible(ppu,s) && ((ppu->oam[s*2+1]>>9)&7)==p) used=true;
        if(!used) palette=p;
    }
    unsigned slots[6],idle=0;
    for(int s=127;s>=0 && idle<6;--s) if(!visible(ppu,(unsigned)s)) slots[idle++]=(unsigned)s;
    if(palette==8 || idle<6) return;
    int block=borrow_block(ppu);
    if(block<0) return;
    unsigned tiles[3]={(unsigned)block,(unsigned)block+1,(unsigned)block+16};
    unsigned shade=PickerCellColors()[CELL];
    unsigned id=g_rom[0xbb4e + INDEX]+(shade>=6?7:shade);
    const uint8_t *colors=g_rom+palette_source(id);
    for(unsigned c=1;c<9;++c) put(&ppu->cgram[0x80+palette*16+c],(uint16_t)(colors[c*2]|colors[c*2+1]<<8));
    memset(canvas,0,sizeof(canvas));
    for(unsigned y=0;y<24;++y) for(unsigned x=0;x<8;++x) canvas[y][x]=figure[y][x];
    for(unsigned row=0;row<3;++row) {
        write_tile(ppu,tiles[row],0,row*8);
        for(unsigned side=0;side<2;++side) {
            unsigned s=slots[row*2+side],x=px+side*8,y=(py+row*8-1)&255;
            put(&ppu->oam[s*2],(uint16_t)((y<<8)|(x&255)));
            put(&ppu->oam[s*2+1],(uint16_t)(((side?0x40:0x00)|0x20|(palette<<1)|(tiles[row]>>8))<<8|(tiles[row]&255)));
            uint8_t high=ppu->highOam[s/4]&(uint8_t)~(3u<<((s%4)*2));
            if(x&0x100) high|=(uint8_t)(1u<<((s%4)*2));
            ppu->highOam[s/4]=high; /* Whole table restored from saved_high. */
        }
    }
}
/* Seat previews: four 16x16 sprites (32x32) around the seat's object. The
 * frame and the per-player background are BG; native faces are transparent
 * around the face and sit at (4..27, 4..27). The cartridge has no portrait
 * for this bomber, so these are authored 24x24 portraits supplied by the
 * maintainer: one shape (0 transparent, classes 1-9) and one nine-color set
 * per selectable color (white, black, red, blue, green, yellow), BGR555.
 * Colors are written render-only into the seat's own OBJ palette, which no
 * other visible sprite uses. */
static const char kPortrait[24][25]={
    "000000011111111110000000",
    "000001123111113321100000",
    "000012231888881332210000",
    "000122218818188133221000",
    "001222218888888133322100",
    "012222231188811333332210",
    "012222231818181333332210",
    "012222223111113333333210",
    "122222222333333333332221",
    "122222111111111111222221",
    "122111267777777776111221",
    "111666555555555555776111",
    "126665444444444444577621",
    "166654441444444144457761",
    "166654411144441114457771",
    "166654411144441114457771",
    "016654411144441114457710",
    "012654441444444144457610",
    "001265544444444445576100",
    "011126655555555556661110",
    "011111266677777766111110",
    "191111111111111111111491",
    "941221111111111111111149",
    "941222122123312313333149",
};
static const uint16_t kPortraitColors[6][9]={
    {0x1466,0x58cb,0x75b1,0x2b3f,0x1cdc,0x56f3,0x73df,0x7fff,0x3dbf},
    {0x1466,0x24e7,0x358c,0x2b3f,0x1cdc,0x24e7,0x358c,0x7fff,0x3dbf},
    {0x1466,0x00b7,0x01df,0x573f,0x10b0,0x14fa,0x153f,0x7fff,0x3dbf},
    {0x1466,0x5d87,0x7e6b,0x433f,0x1cdc,0x5d87,0x7e6b,0x7fff,0x3dbf},
    {0x1466,0x26c5,0x3bce,0x573f,0x1cdc,0x26c5,0x3bce,0x7fff,0x3dbf},
    {0x1466,0x1a5a,0x3bff,0x3f9f,0x1cdc,0x1a5a,0x3bff,0x7fff,0x3dbf},
};
static void draw_portraits(Ppu *ppu) {
    int quarter[5][4];
    unsigned slots[5];
    bool mine[128]={false};
    for(unsigned i=0;i<5;++i) {
        for(unsigned q=0;q<4;++q) quarter[i][q]=-1;
        slots[i]=8;
        if(g_ram[0x625f+i]!=CHARACTER) continue;
        unsigned object=g_ram[0x62bf+i];
        if(object>=0x60 || g_ram[0x6df+object]>=8) continue;
        slots[i]=g_ram[0x6df+object];
        unsigned left=(g_ram[0x55f+object]-8)&0x1ff,top=(g_ram[0x5bf+object]-25)&255;
        for(unsigned s=0;s<128;++s) {
            if(!visible(ppu,s) || !big(ppu,s) || ((ppu->oam[s*2+1]>>9)&7)!=slots[i] ||
               ((ppu->oam[s*2+1]>>8)&0xc0)) continue;
            unsigned dx=(sprite_x(ppu,s)-left)&0x1ff,dy=((ppu->oam[s*2]>>8)-top)&255;
            if((dx==0 || dx==16) && (dy==0 || dy==16)) { quarter[i][(dy/16)*2+dx/16]=(int)s;mine[s]=true; }
        }
    }
    int blocks[4]={-1,-1,-1,-1};
    for(unsigned i=0;i<5;++i) {
        if(slots[i]>=8 || quarter[i][0]<0 || quarter[i][1]<0 || quarter[i][2]<0 || quarter[i][3]<0) continue;
        /* The seat palette must not color anything else on screen. */
        bool usable=true;
        for(unsigned s=0;s<128;++s)
            if(visible(ppu,s) && ((ppu->oam[s*2+1]>>9)&7)==slots[i] && !mine[s]) usable=false;
        if(!usable) continue;
        if(blocks[0]<0) {
            /* One shared copy of the shape for every seat on this bomber. */
            for(unsigned q=0;q<4;++q) if((blocks[q]=borrow_block(ppu))<0) return;
            memset(canvas,0,sizeof(canvas));
            for(unsigned y=0;y<24;++y) for(unsigned x=0;x<24;++x) canvas[4+y][4+x]=(uint8_t)(kPortrait[y][x]-'0');
            for(unsigned q=0;q<4;++q) paint_block(ppu,(unsigned)blocks[q],(q&1)*16,(q>>1)*16);
        }
        unsigned color=g_ram[0x6264+i];
        if(color>=6) color=0;
        for(unsigned c=0;c<9;++c) put(&ppu->cgram[0x80+slots[i]*16+1+c],kPortraitColors[color][c]);
        for(unsigned q=0;q<4;++q) point_sprite(ppu,(unsigned)quarter[i][q],(unsigned)blocks[q]);
    }
}
/* HUD faces: one 16x16 sprite 17 lines above the face object ($7E35C5+i),
 * in the player's own palette. The alive face ($1D) is this bomber's head
 * from its standing frame; the defeated face ($27) is authored art supplied
 * by the maintainer, stored as indices into the bomber's nine-color palette
 * (drawn from the red set: 1/2 main, 3 secondary, 4 plate, 5 skin, 6/7
 * accent, 8 outline), so it takes the player's selected color. */
static const char kDefeatedFace[16][17]={
    "0000000880000000",
    "0000008778000000",
    "0000088888800000",
    "0008811111188000",
    "0081222222221800",
    "0812221111222180",
    "0812214444122180",
    "0812148448412180",
    "0811144444411180",
    "0861211551121680",
    "0861111111111680",
    "8386585555856838",
    "0838585555858380",
    "0833111111113380",
    "0888333333338880",
    "8684838338384868",
};
static void draw_faces(Ppu *ppu) {
    unsigned mode=g_ram[0x1031];
    if(mode==4 || mode<5) return;
    int blocks[2]={-1,-1};
    for(unsigned i=0;i<5;++i) {
        if(g_ram[0x16c2+i]!=INDEX) continue;
        unsigned object=g_ram[0x35c5+i];
        if(object>=0x60 || g_ram[0x4ff+object]!=6) continue;
        unsigned anim=g_ram[0x79f+object];
        if(anim!=FACE && anim!=FACE_ALT) continue;
        unsigned state=anim==FACE_ALT;
        unsigned x=g_ram[0x55f+object],y=(g_ram[0x5bf+object]-17)&255;
        for(unsigned s=0;s<128;++s) {
            if(!visible(ppu,s) || !big(ppu,s) || sprite_x(ppu,s)!=x || (ppu->oam[s*2]>>8)!=y ||
               ((ppu->oam[s*2+1]>>8)&0xc0)) continue;
            if(blocks[state]<0) {
                if((blocks[state]=borrow_block(ppu))<0) return;
                memset(canvas,0,sizeof(canvas));
                for(unsigned r=0;r<16;++r) for(unsigned c=0;c<16;++c)
                    canvas[r][c]=state?(uint8_t)(kDefeatedFace[r][c]-'0'):figure[r][c];
                paint_block(ppu,(unsigned)blocks[state],0,0);
            }
            point_sprite(ppu,s,(unsigned)blocks[state]);
        }
    }
}
void SubordinateObserve(const Ppu *view,unsigned line,void *context) {
    (void)context;Ppu *ppu=(Ppu *)view;
    if(line==224) {
        if(!drawn) return;
        while(saved_count) { Saved w=saved[--saved_count];*w.word=w.value; }
        memcpy(ppu->highOam,saved_high,sizeof(saved_high));
        ++ppu->vramWriteCount;++ppu->oamWriteCount;drawn=false;return;
    }
    if(line!=1 || !enabled || !patched || PPU_objSize(ppu)!=0 || !load_figure()) return;
    saved_count=0;borrowed_count=0;memcpy(saved_high,ppu->highOam,sizeof(saved_high));
    if(picker()) { draw_grid(ppu);draw_portraits(ppu); }
    else draw_faces(ppu);
    if(saved_count) { drawn=true;++ppu->vramWriteCount;++ppu->oamWriteCount; }
}
