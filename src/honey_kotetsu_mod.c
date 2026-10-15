/* Honey and Kotetsu as selectable characters.
 *
 * Both are boss actors of the secret stages ZONE1/2/3 - S (stage ids
 * $71-$73), not bombers: they have no entry in the per-character tables
 * read through $16C2. Their frames live in object set $0D (animations
 * $1E-$22 for Honey, $2B-$2F for Kotetsu, right/down/left/up/spin, the same
 * direction order as the bombers' walk animations 1-4), drawn from one
 * compressed graphics archive ($CC:289D, $1E00 bytes, format of $C0:1CCC)
 * with the palettes at $C1:8FC6 (Honey) and $C1:8FA6 (Kotetsu).
 *
 * Gameplay: a Honey/Kotetsu seat plays as Bomberman (native choice 1).
 * Rendering: the bomber's sprites are replaced, each frame, by the matching
 * Honey/Kotetsu frame decoded from the player's cartridge at runtime. No
 * cartridge graphics or palettes are stored in this source.
 *
 * In-memory cartridge patches (restored on reset; ROM file and SRAM untouched):
 *  - Picker cell arrays ($62C9 character, $62D3 color; 10 cells) move to
 *    twelve-cell arrays at $7F:F800/$7F:F80C: every script operand and the
 *    native readers $C2:1240/$C2:12B5/$C2:12C3.
 *  - Grid layout: $C2:12D2 (figure tile x per cell) packs the five stock
 *    columns into tiles 5/9/13/17/21; column six (tile 25) holds Honey
 *    (top, choice 11) and Kotetsu (bottom, choice 12). Cursor position
 *    tables $C0:C976/$C0:C980 are read from twelve-entry copies.
 *  - Cursor movement ($C6:8C02, $C6:8C78, $C6:8CEE, $C6:8D62) uses lookup
 *    tables for the 6x2 grid instead of the 5x2 arithmetic.
 *  - Seat portrait helpers FF.66/FF.67 ($C2:1356/$C2:13E7) clamp character
 *    indices above the ten-entry tables to Bomberman.
 *  - Battle stage init ($C4:2A3C) maps choices 11/12 to Bomberman and
 *    records each seat's choice at $7F:F818+seat for the renderer.
 *  - The score board sets $16C2 = choice - 1 through $C4:3BDE (script
 *    $C6:7736, FF.59); indices above the ten-entry tables become Bomberman.
 *
 * Victory/cheer poses alternate the two frames Honey and Kotetsu use on the
 * Battle champion screen (object set 6, animations $50 and $4E), decoded from
 * that screen's archive ($D6:D871, $0D40 bytes) at runtime.
 *
 * This file is also the extra-character core used by the Bomber Kings mod
 * (bomber_kings_mod.c): with it, a third grid row holds Super Bomberman 4's
 * Bomberman and the five Bomber Kings (choices 13-18), drawn from the
 * player's own Super Bomberman 4 image (sb4_rom.c). The grid then has three
 * rows (BG tile rows 12/16/20 instead of 15/20) and 18 cells. */
#include "honey_kotetsu_mod.h"
#include "bomber_kings_mod.h"
#include "sb4_rom.h"
#include "special_abilities.h"
#include "selection_color_mod.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "mod_runtime.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include <string.h>

enum {
    MAX_CELLS = 18, HK_CELLS = 12, STOCK_CELLS = 10, SKINS = 2+SB4_CHARACTERS,
    CELL_CHARS = 0x1f800, CELL_COLORS = 0x1f812, SEAT_CHOICE = 0x1f824, SEAT_COLOR = 0x1f829,
    ARCHIVE = 0xcc289d, ARCHIVE_SIZE = 0x1e00, ARCHIVE_END = 0xcc40e4, OBJECT_SET = 0x0d,
    CHAMPION = 0xd6d871, CHAMPION_SIZE = 0xd40, CHAMPION_END = 0xd6e0bb, CHAMPION_SET = 0x06,
};
static const uint32_t kPalette[2]={0xc18fc6,0xc18fa6};  /* Honey, Kotetsu */
static const uint8_t kAnimBase[2]={0x1e,0x2b};         /* right, down, left, up, spin */
static const uint8_t kAttackBase[2]={0x23,0x30},kAttackStep[2]={2,2},kVictoryStep[2]={4,1};
/* Champion screen (Battle winner): Honey $A2/$A3, Kotetsu $9E/$9F. */
static const uint8_t kChampionAnim[2]={0x50,0x4e};

static bool hk_enabled,enabled,installed,patched,art_ready,champion_ready;
/* Grid rows (2 without the Bomber Kings, 3 with them) and cells. */
static unsigned rows=2,cells=HK_CELLS;
static const uint8_t kRowTiles[2][3]={{15,20,20},{12,16,20}};
/* Each object's last distinct frame pointers ($7E2EEA/$7E2F4A), newest
 * first, sampled at the start of every frame. */
enum { FRAME_HISTORY = 3 };
static uint32_t frame_history[0x30][FRAME_HISTORY];
static uint32_t frame_pointer(unsigned object) {
    return g_ram[0x2eea+object]|(uint32_t)g_ram[0x2eeb+object]<<8|(uint32_t)g_ram[0x2f4a+object]<<16;
}
typedef struct { uint32_t pc; unsigned size; uint8_t original[24]; } Patch;
static Patch patches[64];
static unsigned patch_count;

bool HoneyKotetsuEnabled(void) { return enabled; }
bool HoneyKotetsuInstalled(void) { return enabled && patched; }
uint8_t *PickerCellCharacters(void) { return g_ram+(HoneyKotetsuInstalled()?CELL_CHARS:0x62c9); }
uint8_t *PickerCellColors(void) { return g_ram+(HoneyKotetsuInstalled()?CELL_COLORS:0x62d3); }
unsigned PickerCellCount(void) { return HoneyKotetsuInstalled()?cells:STOCK_CELLS; }
unsigned HoneyKotetsuBaseChoice(unsigned choice) {
    return choice>=HK_CHOICE_HONEY && choice<=EXTRA_CHOICE_LAST ? 1 : choice;
}
void HoneyKotetsuSetSeatChoice(unsigned seat,unsigned choice) {
    if(seat<5 && HoneyKotetsuInstalled()) g_ram[SEAT_CHOICE+seat]=(uint8_t)choice;
}
unsigned ExtraCharacterSeatChoice(unsigned seat) {
    return seat<5 && HoneyKotetsuInstalled()?g_ram[SEAT_CHOICE+seat]:0;
}
unsigned ExtraCharacterSeatColor(unsigned seat) {
    return seat<5 && HoneyKotetsuInstalled()?g_ram[SEAT_COLOR+seat]:0;
}
unsigned ExtraCharacterColorCount(unsigned choice) {
    if(choice==HK_CHOICE_HONEY || choice==HK_CHOICE_KOTETSU) return 1;
    if(choice>=KINGS_CHOICE_FIRST && choice<=EXTRA_CHOICE_LAST) return SB4_COLORS;
    return 0;
}

static uint8_t *rom(uint32_t pc) { return SnesRomPtr(pc); }
static unsigned file(uint32_t pc) { return ((pc>>16)&0x3f)<<16|(pc&0xffff); }
static uint8_t rb(uint32_t pc) { return g_rom[file(pc)]; }
static unsigned rw(uint32_t pc) { return rb(pc)|rb(pc+1)<<8; }
static void patch(uint32_t pc,const uint8_t *bytes,unsigned size) {
    if(patch_count>=sizeof(patches)/sizeof(*patches) || size>sizeof(patches[0].original)) return;
    Patch *p=&patches[patch_count++];
    p->pc=pc;p->size=size;memcpy(p->original,rom(pc),size);memcpy(rom(pc),bytes,size);
}
static void uninstall(void) {
    while(patch_count) { Patch *p=&patches[--patch_count];memcpy(rom(p->pc),p->original,p->size); }
    installed=patched=false;
}

/* Script operands and native long reads of the two cell arrays. */
static const uint32_t kCellCharRefs[]={
    0xc21240,0xc212b5,0xc68ad5,0xc68afe,0xc68b1c,0xc68c2f,0xc68ca5,0xc68d19,0xc68d8d,0xc68de9,
    0xc69ace,0xc69bda,0xc69c43,0xc69cb6,0xc69d1d,0xc69d6c,0xc69e1c,0xc6a560,0xc6a671,
};
static const uint32_t kCellColorRefs[]={
    0xc212c3,0xc68ae5,0xc68b0c,0xc68b2a,0xc68c49,0xc68cbf,0xc68d33,0xc68da7,
    0xc69adf,0xc69bf4,0xc69c5d,0xc69cd0,0xc69d37,0xc69e3d,0xc6a684,
};
/* Grid (choice = cursor cell + 1), two or three rows of six ($C6:8CEE
 * moves up, $C6:8D62 down):
 *    1  2  3  4  5 11
 *    6  7  8  9 10 12
 *   13 14 15 16 17 18   (Bomber Kings)
 * Movement wraps inside a row and between rows. */
static const uint8_t kGrid[3][6]={{1,2,3,4,5,11},{6,7,8,9,10,12},{13,14,15,16,17,18}};
static const uint8_t kGridX[6]={5,9,13,17,21,25};
static void cell_place(unsigned choice,unsigned *row,unsigned *col) {
    for(unsigned r=0;r<3;++r) for(unsigned c=0;c<6;++c) if(kGrid[r][c]==choice) { *row=r;*col=c;return; }
    *row=*col=0;
}
enum { TABLE_RIGHT=0xc260c0, TABLE_LEFT=0xc260d3, TABLE_DOWN=0xc260e6, TABLE_UP=0xc260f9,
       TABLE_CURSOR_X=0xc2610c, TABLE_CURSOR_Y=0xc2611e, CLAMP=0xc26088, STAGE_INIT=0xc26094,
       SET_CHARACTER=0xc260a4, PATCH_START=0xc26040, PATCH_END=0xc26130 };
static const uint8_t kClamp[]={
    0x3a,             /* DEC A                                     */
    0x29,0x0f,        /* AND #$0F                                  */
    0xc9,0x0a,        /* CMP #$0A   ten-entry portrait tables       */
    0x90,0x02,        /* BCC +2                                    */
    0xa9,0x00,        /* LDA #$00   Bomberman                      */
    0xaa,             /* TAX                                       */
    0x6b,             /* RTL                                       */
};
static const uint8_t kStageInit[]={
    0xbf,0x5f,0x62,0x7e, /* LDA $7E625F,X  seat choice                */
    0x9f,0x24,0xf8,0x7f, /* STA $7FF824,X  remembered for rendering   */
    0xc9,0x0b,           /* CMP #11                                   */
    0x90,0x02,           /* BCC +2                                    */
    0xa9,0x01,           /* LDA #1         extra characters play as 1 */
    0x3a,                /* DEC A                                     */
    0x6b,                /* RTL                                       */
};
static const uint8_t kSetCharacter[]={
    0x9c,0xe7,0x0a,      /* STZ $0AE7      (moved from $C4:3BDF)      */
    0xc9,0x0a,           /* CMP #$0A                                  */
    0x90,0x02,           /* BCC +2                                    */
    0xa9,0x00,           /* LDA #$00       extra characters play as 0 */
    0x9d,0xc2,0x16,      /* STA $16C2,X                               */
    0x6b,                /* RTL                                       */
};
static bool stock(uint32_t pc,const uint8_t *bytes,unsigned size) { return !memcmp(rom(pc),bytes,size); }
static void move_patch(uint32_t pc,uint32_t table,uint8_t rel) {
    uint8_t code[]={0x36,0x34,0x10,(uint8_t)table,(uint8_t)(table>>8),(uint8_t)(table>>16), /* acc=table[$1034] */
                    0x5f,0x34,0x10,                                                       /* $1034=acc       */
                    0x45,rel};                                                            /* BRA portrait    */
    patch(pc,code,sizeof(code));
}

/* ---- Cartridge art: archive tiles, frame lists and palettes. ---- */
static uint8_t archive[ARCHIVE_SIZE],champion[CHAMPION_SIZE];
static bool decompress(uint32_t src,uint8_t *out,unsigned size,uint32_t end) {
    /* $C0:1CB0/$C0:1CCC: control byte = mode (bits 5-7) + count-1 (bits 0-4);
     * modes literal, previous+1, previous, then copies 2/4/16/32/64 back in a
     * 256-byte ring. */
    static const uint8_t back[8]={0,0,0,2,4,16,32,64};
    uint8_t ring[256]={0};unsigned index=0;
    unsigned control=rb(src++),mode=control>>5,count=(control&0x1f)+1;
    for(unsigned i=0;i<size;++i) {
        uint8_t v;
        if(mode==0) v=rb(src++);
        else if(mode==1) v=(uint8_t)(ring[(index-1)&255]+1);
        else if(mode==2) v=ring[(index-1)&255];
        else v=ring[(index-back[mode])&255];
        ring[index]=v;index=(index+1)&255;out[i]=v;
        if(!--count) { control=rb(src++);mode=control>>5;count=(control&0x1f)+1; }
    }
    return src==end; /* Exact end of the verified stream. */
}
static uint32_t long_ptr(uint32_t at) { return rw(at)|(uint32_t)rb(at+2)<<16; }
static uint32_t anim_script(unsigned set,unsigned anim) {
    uint32_t table=long_ptr(0xc0171d+set*3);
    return (table&0xff0000)|rw(table+(anim-1)*2);
}
/* Frame number of step `step` of object-set-$0D animation `anim`. */
static unsigned anim_frame(unsigned anim,unsigned step) { return rb(anim_script(OBJECT_SET,anim)+step*2); }
static uint32_t set_frame_data(unsigned set,unsigned frame) {
    uint32_t table=long_ptr(0xc016cc+set*3);
    return (table&0xff0000)|rw(table+(frame-1)*2);
}
static uint32_t frame_data(unsigned frame) { return set_frame_data(OBJECT_SET,frame); }
static bool check_art(void) {
    static const uint8_t base[2][2]={{0x1e,0x23},{0x2b,0x30}};
    for(unsigned s=0;s<2;++s) for(unsigned g=0;g<2;++g) for(unsigned d=0;d<(g?4u:5u);++d) for(unsigned step=0;step<(g?5u:4u);++step) {
        if(g && step>(s?2u:4u)) continue;
        unsigned f=anim_frame(base[s][g]+d,step);
        if(!f || f>150) return false;
        uint32_t data=frame_data(f);unsigned n=rb(data);
        if(!n || n>16) return false;
        for(unsigned k=0;k<n;++k) if(rw(data+1+k*7+4)>=0x100) return false;
    }
    return true;
}
/* Champion frames: every step of both animations (frame, duration pairs
 * up to a zero frame) must hold 1-16 pieces with tiles inside the archive. */
static bool check_champion(void) {
    for(unsigned s=0;s<2;++s) {
        uint32_t script=anim_script(CHAMPION_SET,kChampionAnim[s]);unsigned steps=0;
        for(;steps<8 && rb(script+steps*2);++steps) {
            if(!rb(script+steps*2+1)) return false;
            uint32_t data=set_frame_data(CHAMPION_SET,rb(script+steps*2));unsigned n=rb(data);
            if(!n || n>16) return false;
            for(unsigned k=0;k<n;++k) if(rw(data+1+k*7+4)+17>=CHAMPION_SIZE/32) return false;
        }
        if(steps<2 || steps==8) return false;
    }
    return true;
}
/* Step of the champion animation shown now (the native durations). */
static unsigned champion_frame(unsigned skin) {
    uint32_t script=anim_script(CHAMPION_SET,kChampionAnim[skin]);unsigned total=0;
    for(unsigned k=0;k<8 && rb(script+k*2);++k) total+=rb(script+k*2+1);
    unsigned t=(unsigned)snes_frame_counter%(total?total:1);
    for(unsigned k=0;k<8 && rb(script+k*2);++k) {
        if(t<rb(script+k*2+1)) return rb(script+k*2);
        t-=rb(script+k*2+1);
    }
    return rb(script);
}
/* Palette keys: skin*8 + color (Honey/Kotetsu have one, Super Bomberman 4
 * characters five). */
static unsigned palette_color(unsigned key,unsigned index) {
    unsigned skin=key/8;
    if(skin<2) return rw(kPalette[skin]+index*2)&0x7fff;
    return Sb4Color(skin-2,key%8,index);
}

/* Pose canvas: 48 x 64 pixels, origin (16, 42) is the object's position. */
enum { CANVAS_W = 48, CANVAS_H = 64, ORIGIN_X = 16, ORIGIN_Y = 42 };
static uint8_t canvas[CANVAS_H][CANVAS_W];
/* Both archives upload in 16x16 blocks (TL, BL, TR, BR) to a 16-tile-wide
 * VRAM area: frame tile t is in block (t%16)/2 + 8*(t/32). */
static void archive_pixels(const uint8_t *art,unsigned tile,uint8_t out[8][8]) {
    unsigned block=(tile%16)/2+8*(tile/32),sub=(tile%2)*2+(tile/16)%2;
    const uint8_t *t=art+block*128+sub*32;
    for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) {
        unsigned v=0;
        for(unsigned p=0;p<4;++p) v|=((t[(p/2)*16+y*2+(p%2)]>>(7-x))&1u)<<p;
        out[y][x]=(uint8_t)v;
    }
}
/* Draws frame `data` (count + 7-byte pieces) from `art` with its origin at
 * canvas (ox, oy). Without `shadow`, the 8x8 pieces below the feet (dy >= 2,
 * the ground shadow of set $0D frames) are left out. */
static void compose_data(uint32_t data,const uint8_t *art,unsigned art_size,int ox,int oy,bool shadow) {
    unsigned n=rb(data);
    for(unsigned k=n;k-->0;) { /* First piece on top. */
        uint32_t piece=data+1+k*7;
        int dx=(int16_t)rw(piece),dy=(int16_t)rw(piece+2);
        unsigned tile=rw(piece+4),attr=rb(piece+6),size=(attr&1)?2:1;
        if(!shadow && size==1 && dy>=2) continue;
        for(unsigned ty=0;ty<size;++ty) for(unsigned tx=0;tx<size;++tx) {
            /* Tiles $F0-$FF (a few weapon pieces) are not in the archive. */
            if(tile+ty*16+tx>=art_size/32) continue;
            uint8_t px[8][8];archive_pixels(art,tile+ty*16+tx,px);
            unsigned cx=(attr&0x40)?size-1-tx:tx,cy=(attr&0x80)?size-1-ty:ty;
            for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) {
                uint8_t c=px[(attr&0x80)?7-y:y][(attr&0x40)?7-x:x];
                int X=ox+dx+(int)(cx*8+x),Y=oy+dy+(int)(cy*8+y);
                if(c && X>=0 && X<CANVAS_W && Y>=0 && Y<CANVAS_H) canvas[Y][X]=c;
            }
        }
    }
}
static void compose(unsigned frame,int ox,int oy) { compose_data(frame_data(frame),archive,ARCHIVE_SIZE,ox,oy,true); }

static void reset_plugin(void) { uninstall();enabled=hk_enabled=false; }
static void activate(void) { hk_enabled=enabled=true; }
SNES_MOD_CONSTRUCTOR(register_honey_kotetsu) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.honey-kotetsu.characters",activate);
}

static void bomber_anchor(void);
static void install(void) {
    if(installed || !g_rom || !g_snes || !g_snes->cart) return;
    installed=true;
    /* Only patch the verified image's expected bytes. */
    static const uint8_t char_ref[]={0xc9,0x62,0x7e},color_ref[]={0xd3,0x62,0x7e};
    for(unsigned i=0;i<sizeof(kCellCharRefs)/sizeof(*kCellCharRefs);++i) if(!stock(kCellCharRefs[i],char_ref,3)) return;
    for(unsigned i=0;i<sizeof(kCellColorRefs)/sizeof(*kCellColorRefs);++i) if(!stock(kCellColorRefs[i],color_ref,3)) return;
    static const uint8_t right[]={0x7b,0x34,0x10,0x5d,0x34,0x10,0x86,0x06,0x49,0x05,0x5b,0x01,0x5f,0x34,0x10};
    static const uint8_t left[]={0x7f,0x34,0x10,0x5d,0x34,0x10,0x86,0x05,0x49,0x05,0x5b,0x0a,0x5f,0x34,0x10};
    static const uint8_t vert[]={0x5d,0x34,0x10,0x96,0x06,0x49,0x0a,0x5d,0x34,0x10,0x61};
    static const uint8_t portrait[]={0x3a,0x29,0x0f,0xaa},init[]={0xbf,0x5f,0x62,0x7e,0x3a};
    static const uint8_t cursor_x[]={0xbf,0x76,0xc9,0x80},cursor_y[]={0xbf,0x80,0xc9,0x80};
    static const uint8_t grid_x[]={5,10,15,20,25,5,10,15,20,25},grid_y[]={15,15,15,15,15,20,20,20,20,20};
    static const uint8_t set_character[]={0x9c,0xe7,0x0a,0x9d,0xc2,0x16};
    if(!stock(0xc68c02,right,sizeof(right)) || !stock(0xc68c78,left,sizeof(left)) ||
       !stock(0xc68cee,vert,sizeof(vert)) || !stock(0xc68d62,vert,sizeof(vert)) ||
       !stock(0xc213e7,portrait,4) || !stock(0xc21356,portrait,4) || !stock(0xc42a3c,init,5) ||
       !stock(0xc0c93a,cursor_x,4) || !stock(0xc0c941,cursor_y,4) || !stock(0xc212d2,grid_x,10) || !stock(0xc212dc,grid_y,10) ||
       !stock(0xc43bdf,set_character,6)) return;
    for(uint32_t pc=PATCH_START;pc<PATCH_END;++pc) if(*rom(pc)!=0xff) return;
    art_ready=decompress(ARCHIVE,archive,ARCHIVE_SIZE,ARCHIVE_END) && check_art();
    if(!art_ready) return;
    /* Optional: without it, victory poses use a still attack frame. */
    champion_ready=decompress(CHAMPION,champion,CHAMPION_SIZE,CHAMPION_END) && check_champion();
    rows=BomberKingsReady()?3:2;cells=rows==3?MAX_CELLS:HK_CELLS;

    uint8_t chars[]={(uint8_t)CELL_CHARS,(uint8_t)(CELL_CHARS>>8),0x7f};
    uint8_t colors[]={(uint8_t)CELL_COLORS,(uint8_t)(CELL_COLORS>>8),0x7f};
    for(unsigned i=0;i<sizeof(kCellCharRefs)/sizeof(*kCellCharRefs);++i) patch(kCellCharRefs[i],chars,3);
    for(unsigned i=0;i<sizeof(kCellColorRefs)/sizeof(*kCellColorRefs);++i) patch(kCellColorRefs[i],colors,3);
    uint8_t to_right[MAX_CELLS+1]={0},to_left[MAX_CELLS+1]={0},down[MAX_CELLS+1]={0},up[MAX_CELLS+1]={0};
    uint8_t cx[MAX_CELLS],cy[MAX_CELLS];
    for(unsigned r=0;r<rows;++r) for(unsigned c=0;c<6;++c) {
        unsigned choice=kGrid[r][c];
        to_right[choice]=kGrid[r][(c+1)%6];to_left[choice]=kGrid[r][(c+5)%6];
        down[choice]=kGrid[(r+1)%rows][c];up[choice]=kGrid[(r+rows-1)%rows][c];
        cx[choice-1]=(uint8_t)(kGridX[c]*8);cy[choice-1]=(uint8_t)(kRowTiles[rows-2][r]*8+24);
    }
    patch(TABLE_RIGHT,to_right,cells+1);patch(TABLE_LEFT,to_left,cells+1);
    patch(TABLE_DOWN,down,cells+1);patch(TABLE_UP,up,cells+1);
    patch(TABLE_CURSOR_X,cx,cells);patch(TABLE_CURSOR_Y,cy,cells);
    patch(CLAMP,kClamp,sizeof(kClamp));patch(STAGE_INIT,kStageInit,sizeof(kStageInit));
    uint8_t new_x[STOCK_CELLS],new_y[STOCK_CELLS];
    for(unsigned c=0;c<STOCK_CELLS;++c) { new_x[c]=kGridX[c%5];new_y[c]=kRowTiles[rows-2][c/5]; }
    patch(0xc212d2,new_x,STOCK_CELLS);
    if(rows==3) patch(0xc212dc,new_y,STOCK_CELLS);
    uint8_t read_x[]={0xbf,(uint8_t)TABLE_CURSOR_X,(uint8_t)(TABLE_CURSOR_X>>8),0xc2};
    uint8_t read_y[]={0xbf,(uint8_t)TABLE_CURSOR_Y,(uint8_t)(TABLE_CURSOR_Y>>8),0xc2};
    patch(0xc0c93a,read_x,4);patch(0xc0c941,read_y,4);
    move_patch(0xc68c02,TABLE_RIGHT,0x10);move_patch(0xc68c78,TABLE_LEFT,0x10);
    move_patch(0xc68cee,TABLE_UP,0x0e);move_patch(0xc68d62,TABLE_DOWN,0x0e);
    uint8_t clamp[]={0x22,(uint8_t)CLAMP,(uint8_t)(CLAMP>>8),0xc2};
    patch(0xc213e7,clamp,4);patch(0xc21356,clamp,4);
    uint8_t stage[]={0x22,(uint8_t)STAGE_INIT,(uint8_t)(STAGE_INIT>>8),0xc2,0xea};
    patch(0xc42a3c,stage,5);
    patch(SET_CHARACTER,kSetCharacter,sizeof(kSetCharacter));
    uint8_t set_call[]={0x22,(uint8_t)SET_CHARACTER,(uint8_t)(SET_CHARACTER>>8),0xc2,0xea,0xea};
    patch(0xc43bdf,set_call,6);
    patched=patch_count==sizeof(kCellCharRefs)/sizeof(*kCellCharRefs)+sizeof(kCellColorRefs)/sizeof(*kCellColorRefs)+20+(rows==3);
    if(!patched) uninstall(),installed=true;
    else bomber_anchor();
}

void HoneyKotetsuBeforeFrame(void) {
    if(!enabled && BomberKingsWanted()) enabled=true;
    if(!enabled) return;
    install();
    if(!patched) return;
    SpecialAbilitiesInstall();
    /* The picker script fills cells 0-9; cells 10/11 are Honey/Kotetsu and
     * 12-17 the Super Bomberman 4 characters. Its exit code copies the cell
     * character into $625F ($C6:A575), and the portrait helpers show
     * Bomberman for choices above 10 (clamp patch). */
    for(unsigned c=STOCK_CELLS;c<cells;++c) {
        g_ram[CELL_CHARS+c]=(uint8_t)(c+1);
        unsigned count=ExtraCharacterColorCount(c+1);
        if(g_ram[CELL_COLORS+c]>=(count>1?count:7)) g_ram[CELL_COLORS+c]=0;
    }
    /* A new session (title / player menu) forgets the previous choices. */
    if(g_ram[0x1031]==2) memset(g_ram+SEAT_CHOICE,0,10);
    /* Seat colors of extra characters: the L/R choice with the color mod,
     * otherwise one color per seat (as in Super Bomberman 4). */
    if(g_ram[0x1031]==4) for(unsigned i=0;i<5;++i) {
        unsigned choice=g_ram[0x625f+i],count=ExtraCharacterColorCount(choice);
        if(!count) continue;
        int color=SelectionColorSeat(i);
        g_ram[SEAT_COLOR+i]=(uint8_t)(count<2?0:(color>=0?(unsigned)color:i)%count);
    }
    for(unsigned o=0;o<0x60;o+=2) {
        uint32_t *h=frame_history[o/2],now=frame_pointer(o);
        if(h[0]!=now) { memmove(h+1,h,(FRAME_HISTORY-1)*sizeof(*h));h[0]=now; }
    }
}

/* ---- Render-only art: every VRAM/OAM/CGRAM word is restored at line 224. */
typedef struct { uint16_t *word; uint16_t value; } Saved;
static Saved saved[3072];
static unsigned saved_count;
static uint8_t saved_high[0x20];
static bool drawn;
static void put(uint16_t *word,uint16_t value) {
    if(saved_count>=sizeof(saved)/sizeof(*saved)) return;
    saved[saved_count++]=(Saved){word,*word};*word=value;
}
static unsigned tile_address(const Ppu *ppu,unsigned tile) {
    return ((tile&0x100)?PPU_objTileAdr2(ppu):PPU_objTileAdr1(ppu))+(tile&255)*16;
}
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
static bool visible(const Ppu *ppu,unsigned s) { unsigned y=ppu->oam[s*2]>>8; return y<224 || y>=240; }
static bool big(const Ppu *ppu,unsigned s) { return (ppu->highOam[s/4]>>((s%4)*2+1))&1; }
static unsigned sprite_x(const Ppu *ppu,unsigned s) {
    return (ppu->oam[s*2]&255)|(((ppu->highOam[s/4]>>((s%4)*2))&1u)<<8);
}
static unsigned sprite_palette(const Ppu *ppu,unsigned s) { return (ppu->oam[s*2+1]>>9)&7; }
static bool tile_shown(const Ppu *ppu,unsigned tile) {
    for(unsigned s=0;s<128;++s) if(visible(ppu,s)) {
        unsigned t=ppu->oam[s*2+1]&0x1ff;
        if(t==tile) return true;
        if(big(ppu,s) && PPU_objSize(ppu)==0 && (t+1==tile || t+16==tile || t+17==tile)) return true;
    }
    return false;
}
/* Borrowed 16x16 OBJ blocks no visible sprite shows this frame. $1F0-$1FF
 * stay reserved for the color mod's cursor hint. */
static uint16_t borrowed[128];
static unsigned borrowed_count;
static bool tile_free(const Ppu *ppu,unsigned tile) {
    if(tile>=0x1f0 || tile_shown(ppu,tile)) return false;
    for(unsigned i=0;i<borrowed_count;++i) {
        unsigned b=borrowed[i];
        if(tile==b || tile==b+1 || tile==b+16 || tile==b+17) return false;
    }
    return true;
}
static int borrow_block(const Ppu *ppu) {
    if(borrowed_count>=sizeof(borrowed)/sizeof(*borrowed)) return -1;
    for(int t=0x1de;t>=0;--t) {
        if((t&15)==15) continue;
        if(tile_free(ppu,(unsigned)t) && tile_free(ppu,(unsigned)t+1) &&
           tile_free(ppu,(unsigned)t+16) && tile_free(ppu,(unsigned)t+17)) {
            borrowed[borrowed_count++]=(uint16_t)t;return t;
        }
    }
    return -1;
}
static void paint_block(Ppu *ppu,unsigned t,unsigned cx,unsigned cy) {
    for(unsigned q=0;q<4;++q) write_tile(ppu,t+(q&1)+(q>>1)*16,cx+(q&1)*8,cy+(q>>1)*8);
}
static bool block_empty(unsigned cx,unsigned cy) {
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<16;++x) if(canvas[cy+y][cx+x]) return false;
    return true;
}
static void place(Ppu *ppu,unsigned s,unsigned x,unsigned y,unsigned tile,unsigned palette,unsigned priority) {
    put(&ppu->oam[s*2],(uint16_t)(((y&255)<<8)|(x&255)));
    put(&ppu->oam[s*2+1],(uint16_t)((((priority&3)<<4)|(palette<<1)|(tile>>8))<<8|(tile&255)));
    uint8_t high=ppu->highOam[s/4]&(uint8_t)~(3u<<((s%4)*2));
    high|=(uint8_t)(2u<<((s%4)*2));
    if(x&0x100) high|=(uint8_t)(1u<<((s%4)*2));
    ppu->highOam[s/4]=high; /* Whole table restored from saved_high. */
}
static void hide(Ppu *ppu,unsigned s) { put(&ppu->oam[s*2],(uint16_t)((ppu->oam[s*2]&0xff)|(224u<<8))); }
static void load_palette(Ppu *ppu,unsigned palette,unsigned key) {
    for(unsigned c=1;c<16;++c) put(&ppu->cgram[0x80+palette*16+c],(uint16_t)palette_color(key,c));
}
static int idle_slot(const Ppu *ppu,bool *taken) {
    for(int s=127;s>=0;--s) if(!visible(ppu,(unsigned)s) && !taken[s]) { taken[s]=true;return s; }
    return -1;
}
/* Draws the canvas as 16x16 sprites with screen (x, y) at its top-left:
 * one column of blocks starting at the leftmost drawn pixel, a second one
 * for wide frames (weapons). Blocks are borrowed first: when that fails
 * nothing changes, so the native sprites stay visible. The `hide` sprites
 * (the native ones being replaced) are then hidden and reused first,
 * keeping their place in the OAM priority order. */
static bool row_empty(unsigned cy) {
    for(unsigned x=0;x<CANVAS_W;++x) if(canvas[cy][x]) return false;
    return true;
}
static bool column_empty(unsigned cx) {
    for(unsigned y=0;y<CANVAS_H;++y) if(canvas[y][cx]) return false;
    return true;
}
static bool emit(Ppu *ppu,int x,int y,unsigned palette,unsigned priority,
                 const unsigned *hide_slots,unsigned hide_count,bool *taken) {
    enum { MAX = (CANVAS_H/16)*2 };
    unsigned left=0;
    while(left<CANVAS_W-16 && column_empty(left)) ++left;
    /* Rows of blocks start at the first drawn line, so a 32-line figure
     * takes two rows of blocks wherever it sits on the canvas. */
    unsigned top=0,bottom=CANVAS_H-1;
    while(top<CANVAS_H-1 && row_empty(top)) ++top;
    while(bottom>top && row_empty(bottom)) --bottom;
    unsigned block_rows=(bottom-top+16)/16;
    if(top+block_rows*16>CANVAS_H) top=CANVAS_H-block_rows*16;
    unsigned cols[MAX],rows[MAX],count=0,first=borrowed_count;
    for(unsigned c=0;c<2;++c) {
        unsigned cx=left+c*16;
        if(cx+16>CANVAS_W) break;
        for(unsigned row=0;row<block_rows;++row) if(!block_empty(cx,top+row*16)) { cols[count]=cx;rows[count++]=top+row*16; }
    }
    int blocks[MAX],slots[MAX];
    for(unsigned k=0;k<count;++k) if((blocks[k]=borrow_block(ppu))<0) { borrowed_count=first;return false; }
    for(unsigned k=0;k<hide_count;++k) taken[hide_slots[k]]=true;
    for(unsigned k=0;k<count;++k) {
        slots[k]=k<hide_count?(int)hide_slots[k]:idle_slot(ppu,taken);
        if(slots[k]<0) { borrowed_count=first;return false; }
    }
    for(unsigned k=0;k<hide_count;++k) hide(ppu,hide_slots[k]);
    for(unsigned k=0;k<count;++k) {
        paint_block(ppu,(unsigned)blocks[k],cols[k],rows[k]);
        place(ppu,(unsigned)slots[k],(unsigned)((x+(int)cols[k])&0x1ff),(unsigned)((y+(int)rows[k])&255),
              (unsigned)blocks[k],palette,priority);
    }
    return true;
}

/* ---- Stage: replace each extra character seat's bomber sprites. ---- */
static int skin_of(unsigned choice) {
    if(choice==HK_CHOICE_HONEY) return 0;
    if(choice==HK_CHOICE_KOTETSU) return 1;
    if(choice>=KINGS_CHOICE_FIRST && choice<=EXTRA_CHOICE_LAST && rows==3) return 2+(int)(choice-KINGS_CHOICE_FIRST);
    return -1;
}
/* Bomber animations (object set $10, four-way groups in the order right,
 * down, left, up; verified by scripted routes):
 *   1-4 walk, $2F-$32 walk carrying a bomb (power glove);
 *   $27-$2A punch (boxing glove, Y), $33-$36 throw (power glove),
 *   $3B-$3E second action group: shown with the attack frames;
 *   $15/$20 cheer and victory: shown with the victory frame;
 *   $2B-$2E lift and $37-$3A hold: standing while carrying;
 *   other four-way groups (stand $46-$49/$4A-$4D, Bad Bomber cart $42-$45,
 *   ...) stand facing that way; death $16, stun $21, Time Up $1F and
 *   airborne $17-$1A have their own poses; anything else spins. */
enum { POSE_STAND, POSE_WALK, POSE_ACTION, POSE_VICTORY, POSE_SPIN, POSE_CARRY_WALK, POSE_CARRY,
       POSE_DEATH, POSE_STUN, POSE_TIME_UP, POSE_AIR };
static int bomber_pose(unsigned anim,int *direction) {
    static const uint8_t actions[]={0x27,0x33,0x3b};
    static const uint8_t stands[]={0x05,0x09,0x0d,0x11,0x1b,0x23,0x42,0x46,0x4a};
    if(anim>=0x01 && anim<0x05) { *direction=(int)(anim-0x01);return POSE_WALK; }
    if(anim>=0x2f && anim<0x33) { *direction=(int)(anim-0x2f);return POSE_CARRY_WALK; }
    if(anim>=0x2b && anim<0x2f) { *direction=(int)(anim-0x2b);return POSE_CARRY; }
    if(anim>=0x37 && anim<0x3b) { *direction=(int)(anim-0x37);return POSE_CARRY; }
    if(anim>=0x17 && anim<0x1b) { *direction=(int)(anim-0x17);return POSE_AIR; }
    for(unsigned i=0;i<sizeof(actions);++i) if(anim>=actions[i] && anim<actions[i]+4u) { *direction=(int)(anim-actions[i]);return POSE_ACTION; }
    for(unsigned i=0;i<sizeof(stands);++i) if(anim>=stands[i] && anim<stands[i]+4u) { *direction=(int)(anim-stands[i]);return POSE_STAND; }
    *direction=1;
    if(anim==0x15 || anim==0x20) return POSE_VICTORY;
    if(anim==0x16) return POSE_DEATH;
    if(anim==0x21) return POSE_STUN;
    if(anim==0x1f) return POSE_TIME_UP;
    return POSE_SPIN;
}
static unsigned walk_step(unsigned object) {
    unsigned position=g_ram[0x7ff+object]/2;
    return position>=1 && position<=3?1:position>=6 && position<=8?3:0;
}
/* Honey/Kotetsu: attack animations ($23-$26 Honey shooting, $30-$33
 * Kotetsu thrusting) and the victory frame (Honey's gun twirl, $24 step 4;
 * Kotetsu's raised weapon, $31 step 1). Carrying poses use the plain ones. */
static unsigned pose_frame(unsigned skin,unsigned object,int ability,unsigned step) {
    int direction;int pose=bomber_pose(g_ram[0x79f+object],&direction);
    if(ability>=0) return anim_frame(kAttackBase[skin]+(unsigned)ability,step<kAttackStep[skin]?step:kAttackStep[skin]);
    switch(pose) {
    case POSE_WALK: case POSE_CARRY_WALK: return anim_frame(kAnimBase[skin]+(unsigned)direction,walk_step(object));
    case POSE_ACTION: return anim_frame(kAttackBase[skin]+(unsigned)direction,kAttackStep[skin]);
    case POSE_VICTORY: return anim_frame(kAttackBase[skin]+1,kVictoryStep[skin]);
    case POSE_STAND: case POSE_CARRY: case POSE_AIR: return anim_frame(kAnimBase[skin]+(unsigned)direction,0);
    default: return anim_frame(kAnimBase[skin]+4,(snes_frame_counter/6)&3);
    }
}
/* Super Bomberman 4 battle frames (all six characters share the layout;
 * verified by the cartridge's own walking uploads): directions up, right,
 * down, left; 0-11 stand/walk (stand, step, step per direction), 20-23
 * special-ability lunge, 24-27 defeat, 32-43 walking with a bomb overhead,
 * 45-47 and 53-54 cheering, 52 surprised (Time Up), 55 electrocuted,
 * 48-51 crouch (airborne), 56-59 riding, 60-63 kick/throw. SB5 directions
 * are right, down, left, up. */
static unsigned sb4_direction(int direction) { return (unsigned)(direction+1)&3; }
static unsigned sb4_pose_frame(unsigned object,bool rider) {
    int direction;int pose=bomber_pose(g_ram[0x79f+object],&direction);
    unsigned d=sb4_direction(direction),step=walk_step(object);
    unsigned walk=step==1?1:step==3?2:0;
    if(rider) return 56+d;
    switch(pose) {
    case POSE_WALK: return d*3+walk;
    case POSE_CARRY_WALK: return 32+d*3+walk;
    case POSE_CARRY: return 32+d*3;
    case POSE_ACTION: return 60+d;
    case POSE_AIR: return 48+d;
    case POSE_VICTORY: return (snes_frame_counter/16)&1?53:54;
    case POSE_DEATH: { unsigned k=g_ram[0x7ff+object]/2;return 24+(k>3?3:k); }
    case POSE_STUN: return 55;
    case POSE_TIME_UP: return 52;
    case POSE_STAND: return d*3;
    default: return ((snes_frame_counter/6)&3)*3;
    }
}
/* Draws Super Bomberman 4 frame `frame` with the character's standing
 * anchor (centre of the opaque columns, lowest opaque line of the front
 * standing frame) at canvas (cx, bottom); `own_anchor` uses this frame's
 * own box instead (riding frames). Returns false without the image. */
static bool sb4_box(const uint8_t px[32][32],int *cx,int *bottom) {
    int l=32,r=-1,b=-1;
    for(int y=0;y<32;++y) for(int x=0;x<32;++x) if(px[y][x]) { if(x<l) l=x;if(x>r) r=x;if(y>b) b=y; }
    if(r<0) return false;
    *cx=(l+r+1)/2;*bottom=b;return true;
}
static bool compose_sb4_at(unsigned character,uint32_t frame_pc,int cx,int bottom,bool own_anchor) {
    uint8_t px[32][32],ref[32][32];int ax=16,ay=31;
    if(!Sb4FrameAt(frame_pc,px)) return false;
    if(own_anchor) { if(!sb4_box(px,&ax,&ay)) return false; }
    else if(!Sb4Frame(character,6,ref) || !sb4_box(ref,&ax,&ay)) return false;
    for(int y=0;y<32;++y) for(int x=0;x<32;++x) if(px[y][x]) {
        int X=cx-ax+x,Y=bottom-ay+y;
        if(X>=0 && X<CANVAS_W && Y>=0 && Y<CANVAS_H) canvas[Y][X]=px[y][x];
    }
    return true;
}
static bool compose_sb4(unsigned character,unsigned frame,int cx,int bottom,bool own_anchor) {
    return compose_sb4_at(character,Sb4FramePc(character,frame),cx,bottom,own_anchor);
}
static bool stage(void) {
    unsigned mode=g_ram[0x1031];
    return mode>=5 && mode!=4;
}
static bool palette_unused(const Ppu *ppu,unsigned palette) {
    for(unsigned s=0;s<128;++s) if(visible(ppu,s) && sprite_palette(ppu,s)==palette) return false;
    return true;
}
/* One OBJ palette per palette key and frame. A seat's palette slot is
 * shared with its mount (Louie) and the eggs it hatches, so a palette no
 * visible sprite uses is taken first; only when none is free is `fallback`
 * recolored. */
static int key_palette[SKINS*8];
static bool palette_given[8];
static int choose_palette(Ppu *ppu,unsigned key,int fallback) {
    if(key_palette[key]>=0) return key_palette[key];
    int chosen=-1;
    for(int p=7;p>=0 && chosen<0;--p) if(!palette_given[p] && palette_unused(ppu,(unsigned)p)) chosen=p;
    if(chosen<0 && fallback>=0 && fallback<8 && !palette_given[fallback]) chosen=fallback;
    if(chosen<0) return -1;
    load_palette(ppu,(unsigned)chosen,key);palette_given[chosen]=true;
    return key_palette[key]=chosen;
}
/* Bounding box of a native frame's pieces relative to the object. */
static void frame_box(uint32_t data,int *left,int *right,int *bottom) {
    unsigned n=rb(data);*left=127;*right=-128;*bottom=-128;
    for(unsigned k=0;k<n && k<16;++k) {
        uint32_t piece=data+1+k*7;int size=(rb(piece+6)&1)?16:8;
        int dx=(int16_t)rw(piece),dy=(int16_t)rw(piece+2);
        if(dx<*left) *left=dx;if(dx+size>*right) *right=dx+size;if(dy+size>*bottom) *bottom=dy+size;
    }
}
/* The bomber's own sprites: the pieces of its current frame (24-bit pointer
 * $7E2EEA/$7E2F4A + object) at object position + piece offset, in its
 * palette slot. The screen offset is searched (0 first) so a scrolled view
 * still matches; most pieces must be found. Mount, eggs and other sprites
 * of the same slot stay untouched. Returns the matched count. */
static unsigned match_frame(const Ppu *ppu,uint32_t data,unsigned object,unsigned slot,unsigned *mine,int *ox,int *oy,int *bottom) {
    if((data>>16)==0x7e || (data>>16)==0x7f || ((data>>16)<0xc0 && !(data&0x8000))) return 0; /* not ROM */
    unsigned n=rb(data);
    if(!n || n>16) return 0;
    int x=g_ram[0x55f+object],y=g_ram[0x5bf+object],px[16],py[16],low=-128;bool large[16];
    for(unsigned k=0;k<n;++k) {
        uint32_t piece=data+1+k*7;
        px[k]=x+(int16_t)rw(piece);py[k]=y+(int16_t)rw(piece+2);large[k]=rb(piece+6)&1;
        int end=(int16_t)rw(piece+2)+(large[k]?16:8);
        if(end>low) low=end;
    }
    *bottom=low;
    unsigned best=0;
    for(int c=-1;c<128 && best<n;++c) {
        int dx=0,dy=0;
        if(c>=0) {
            if(!visible(ppu,(unsigned)c) || sprite_palette(ppu,(unsigned)c)!=slot || big(ppu,(unsigned)c)!=large[0]) continue;
            dx=(int)((sprite_x(ppu,(unsigned)c)-(unsigned)px[0])&0x1ff);dy=(int)(((ppu->oam[c*2]>>8)-(unsigned)py[0])&255);
            if(!dx && !dy) continue;
        }
        unsigned got[16],m=0;bool used[128]={false};
        for(unsigned k=0;k<n;++k) for(unsigned s=0;s<128;++s) {
            if(used[s] || !visible(ppu,s) || sprite_palette(ppu,s)!=slot || big(ppu,s)!=large[k]) continue;
            if(sprite_x(ppu,s)!=((unsigned)(px[k]+dx)&0x1ff) || (ppu->oam[s*2]>>8)!=((unsigned)(py[k]+dy)&255)) continue;
            used[s]=true;got[m++]=s;break;
        }
        if(m>best) { best=m;memcpy(mine,got,m*sizeof(*got));*ox=dx;*oy=dy; }
    }
    return best*2>=n?best:0;
}
/* The OAM shown can lag the pointer by a step or two (it is built and
 * uploaded on the game's own schedule), so recent pointers are tried too. */
static unsigned match_native(const Ppu *ppu,unsigned object,unsigned slot,unsigned *mine,int *ox,int *oy,int *bottom) {
    uint32_t tried[FRAME_HISTORY+1];unsigned tries=0,best=0;
    tried[tries++]=frame_pointer(object);
    for(unsigned k=0;k<FRAME_HISTORY;++k) {
        uint32_t p=frame_history[object/2][k];bool seen=false;
        for(unsigned j=0;j<tries;++j) seen|=tried[j]==p;
        if(!seen && p) tried[tries++]=p;
    }
    for(unsigned k=0;k<tries;++k) {
        unsigned got[16];int x=0,y=0,low=0;
        unsigned m=match_frame(ppu,tried[k],object,slot,got,&x,&y,&low);
        if(m>best) { best=m;memcpy(mine,got,m*sizeof(*got));*ox=x;*oy=y;*bottom=low; }
        if(m && m==rb(tried[k])) break;
    }
    return best;
}
/* Native standing frame of the bomber (set $10, animation $47 = stand
 * facing down): its centre and lowest line anchor the Super Bomberman 4
 * frames, so every pose keeps the same feet line as the native bomber. */
static int bomber_cx=0,bomber_bottom=11;
static void bomber_anchor(void) {
    uint32_t data=set_frame_data(0x10,rb(anim_script(0x10,0x47)));
    if(rb(data)<1 || rb(data)>16) return;
    int l,r,b;frame_box(data,&l,&r,&b);
    if(r>l && b>-64 && b<64) { bomber_cx=(l+r)/2;bomber_bottom=b; }
}
static void draw_players(Ppu *ppu,bool *taken) {
    for(unsigned i=0;i<5;++i) {
        int skin=skin_of(g_ram[SEAT_CHOICE+i]);
        unsigned object=g_ram[0x17fa+i];
        if(skin<0 || object>=0x60 || g_ram[0x16c2+i]!=0) continue;
        unsigned slot=g_ram[0x6df+object];
        if(slot>=8) continue;
        unsigned mine[16];int ox=0,oy=0,bottom=0;
        unsigned count=match_native(ppu,object,slot,mine,&ox,&oy,&bottom);
        /* Bomber not drawn (blinking, dead); Jet's flight keeps the native
         * invulnerability timer fixed, so he is drawn at his object. */
        bool forced=!count && SpecialAbilityShown(i);
        if(!count && !forced) continue;
        /* Death ($16): the last step is the burnt-out debris; leave it native. */
        if(g_ram[0x79f+object]==0x16 && g_ram[0x7ff+object]/2>=4) continue;
        unsigned priority=forced?2:(ppu->oam[mine[0]*2+1]>>12)&3;
        /* $1782+seat: bit 7 riding a mount, bit 1 jumping onto it. A rider
         * stands with its feet where the native rider frame ends; neither
         * keeps the ground shadow (the jump has its own native one). */
        bool rider=g_ram[0x1782+i]&0x80,shadow=!(g_ram[0x1782+i]&0x82);
        unsigned step;uint32_t sb4_frame;
        int ability=SpecialAbilityPose(i,&step,&sb4_frame);
        int direction;unsigned key=(unsigned)skin*8;
        memset(canvas,0,sizeof(canvas));
        int lift=0;
        if(skin>=2) {
            key+=g_ram[SEAT_COLOR+i]%SB4_COLORS;
            uint32_t frame_pc=ability>=0 && sb4_frame?sb4_frame:Sb4FramePc((unsigned)skin-2,sb4_pose_frame(object,rider));
            if(!compose_sb4_at((unsigned)skin-2,frame_pc,ORIGIN_X+bomber_cx,
                               rider?ORIGIN_Y+bottom-1:ORIGIN_Y+bomber_bottom-1,rider)) continue;
        } else if(ability<0 && bomber_pose(g_ram[0x79f+object],&direction)==POSE_VICTORY && champion_ready && !rider) {
            /* Champion frames are anchored at their left edge with the feet
             * one line above the origin. */
            compose_data(set_frame_data(CHAMPION_SET,champion_frame((unsigned)skin)),champion,CHAMPION_SIZE,
                         ORIGIN_X-8,ORIGIN_Y,true);
        } else {
            compose_data(frame_data(pose_frame((unsigned)skin,object,ability,step)),archive,ARCHIVE_SIZE,ORIGIN_X,ORIGIN_Y-2,shadow);
            lift=rider?bottom+1:0;
        }
        int palette=choose_palette(ppu,key,(int)slot);
        if(palette<0) continue;
        emit(ppu,g_ram[0x55f+object]+ox-ORIGIN_X,g_ram[0x5bf+object]+oy-ORIGIN_Y+lift,(unsigned)palette,priority,mine,count,taken);
    }
}
/* Head of a character in a 16x16 block at canvas (ORIGIN_X-8, ORIGIN_Y):
 * Honey/Kotetsu's front walking frame, or the top 16 opaque lines of the
 * Super Bomberman 4 front standing frame. */
static bool compose_head(unsigned key) {
    memset(canvas,0,sizeof(canvas));
    unsigned skin=key/8;
    if(skin<2) { compose(anim_frame(kAnimBase[skin]+1,0),ORIGIN_X,ORIGIN_Y+22);return true; }
    uint8_t px[32][32];int cx,b;
    if(!Sb4Frame(skin-2,6,px) || !sb4_box(px,&cx,&b)) return false;
    int top=0;
    for(bool found=false;top<32 && !found;) { for(int x=0;x<32;++x) found|=px[top][x]!=0; if(!found) ++top; }
    for(int y=0;y<16 && top+y<32;++y) for(int x=0;x<32;++x) if(px[top+y][x]) {
        int X=ORIGIN_X-cx+x,Y=ORIGIN_Y+y;
        if(X>=ORIGIN_X-8 && X<ORIGIN_X+8) canvas[Y][X]=px[top+y][x];
    }
    return true;
}
/* HUD faces: one 16x16 sprite 17 lines above the face object ($7E35C5+i);
 * an extra character plays as Bomberman, so its face animation is
 * Bomberman's ($C2:1470[0], defeated $C5:12FE[0]). */
static void draw_faces(Ppu *ppu) {
    unsigned face=rb(0xc21470),face_alt=rb(0xc512fe);
    int blocks[SKINS*8];memset(blocks,0xff,sizeof(blocks));
    for(unsigned i=0;i<5;++i) {
        int skin=skin_of(g_ram[SEAT_CHOICE+i]);
        if(skin<0 || g_ram[0x16c2+i]!=0) continue;
        unsigned key=(unsigned)skin*8+(skin>=2?g_ram[SEAT_COLOR+i]%SB4_COLORS:0);
        unsigned object=g_ram[0x35c5+i];
        if(object>=0x60 || g_ram[0x4ff+object]!=6) continue;
        unsigned anim=g_ram[0x79f+object];
        if(anim!=face && anim!=face_alt) continue;
        unsigned x=g_ram[0x55f+object],y=(g_ram[0x5bf+object]-17)&255;
        for(unsigned s=0;s<128;++s) {
            if(!visible(ppu,s) || !big(ppu,s) || sprite_x(ppu,s)!=x || (ppu->oam[s*2]>>8)!=y) continue;
            int palette=choose_palette(ppu,key,(int)sprite_palette(ppu,s));
            if(palette<0) return;
            if(blocks[key]<0) {
                if((blocks[key]=borrow_block(ppu))<0 || !compose_head(key)) return;
                paint_block(ppu,(unsigned)blocks[key],ORIGIN_X-8,ORIGIN_Y);
            }
            put(&ppu->oam[s*2+1],(uint16_t)((ppu->oam[s*2+1]&~0xfffu)|(unsigned)palette<<9|(unsigned)blocks[key]));
        }
    }
}

/* ---- Picker: grid figures (cells 10-17) and seat portraits. ---- */
static uint32_t picker_pc(void) {
    return g_ram[0xc90]|((uint32_t)g_ram[0xcb8]<<8)|((uint32_t)g_ram[0xce0]<<16);
}
static bool picker(void) {
    uint32_t pc=picker_pc();
    return g_ram[0x1031]==4 && pc>=0xc68b48 && pc<0xc68ee4;
}
/* Palette loads applied on a later line: grid rows have their own palettes,
 * loaded where the row starts, so the six figures of a row (and the rows
 * above) each keep their own colors. */
typedef struct { unsigned line,palette,key; } LineLoad;
static LineLoad line_loads[24];
static unsigned line_load_count;
static int sprite_bottom(const Ppu *ppu,unsigned s) {
    int y=ppu->oam[s*2]>>8;if(y>=240) y-=256;
    return y+(big(ppu,s)?16:8);
}
/* True when no native sprite in `palette` reaches line `from` or below. */
static bool palette_free_from(const Ppu *ppu,unsigned palette,int from,const bool *taken) {
    for(unsigned s=0;s<128;++s)
        if(!taken[s] && visible(ppu,s) && sprite_palette(ppu,s)==palette && sprite_bottom(ppu,s)>from) return false;
    return true;
}
/* First and last canvas lines with a drawn pixel. */
static int canvas_top(void) {
    for(int y=0;y<CANVAS_H;++y) for(int x=0;x<CANVAS_W;++x) if(canvas[y][x]) return y;
    return CANVAS_H;
}
static int canvas_bottom(void) {
    for(int y=CANVAS_H-1;y>=0;--y) for(int x=0;x<CANVAS_W;++x) if(canvas[y][x]) return y;
    return -1;
}
static bool compose_grid_figure(unsigned cell) {
    memset(canvas,0,sizeof(canvas));
    int skin=skin_of(cell+1);
    if(skin<0) return false;
    if(skin<2) { compose(anim_frame(kAnimBase[skin]+1,0),ORIGIN_X,ORIGIN_Y);return true; }
    /* The native figure's bottom line is canvas ORIGIN_Y+7: Super
     * Bomberman 4 figures keep the front standing frame's lowest line there. */
    return compose_sb4((unsigned)skin-2,6,ORIGIN_X,ORIGIN_Y+7,false);
}
static void draw_grid(Ppu *ppu,bool *taken) {
    unsigned scroll_x=ppu->hScroll[0]&0x3ff,scroll_y=ppu->vScroll[0]&0x3ff;
    int busy_until[8];for(unsigned p=0;p<8;++p) busy_until[p]=-1;
    for(unsigned r=0;r<rows;++r) {
        /* Same anchor as the native 16x24 figures: the cell's tile column,
         * the row's tile row, feet on the figure's bottom line. */
        unsigned row_tile=kRowTiles[rows-2][r];
        int py=(int)((((row_tile+1)*8+24)-scroll_y)&0xff);
        if(py>216) continue;
        unsigned keys[6],cols[6],count=0;int top=1000,bottom=-1000;
        for(unsigned c=0;c<6;++c) {
            unsigned choice=kGrid[r][c];
            if(choice<=STOCK_CELLS || choice>cells || !compose_grid_figure(choice-1)) continue;
            int skin=skin_of(choice);
            keys[count]=(unsigned)skin*8+(skin>=2?g_ram[CELL_COLORS+choice-1]%SB4_COLORS:0);cols[count++]=c;
            int t=py-8-ORIGIN_Y+canvas_top(),b=py-8-ORIGIN_Y+canvas_bottom();
            if(t<top) top=t;
            if(b>bottom) bottom=b;
        }
        if(!count) continue;
        if(top<1) top=1;
        int palettes[6];
        for(unsigned k=0;k<count;++k) {
            palettes[k]=-1;
            for(unsigned j=0;j<k && palettes[k]<0;++j) if(keys[j]==keys[k]) palettes[k]=palettes[j];
            for(int p=7;p>=0 && palettes[k]<0;--p) {
                bool mine=false;
                for(unsigned j=0;j<k;++j) mine|=palettes[j]==p;
                if(!mine && busy_until[p]<top && palette_free_from(ppu,(unsigned)p,top,taken)) palettes[k]=p;
            }
            if(palettes[k]<0) continue;
            busy_until[palettes[k]]=bottom;
            if(line_load_count<sizeof(line_loads)/sizeof(*line_loads))
                line_loads[line_load_count++]=(LineLoad){(unsigned)top,(unsigned)palettes[k],keys[k]};
        }
        for(unsigned k=0;k<count;++k) {
            if(palettes[k]<0) continue;
            unsigned c=cols[k];
            int px=(int)((kGridX[c]*8-scroll_x)&0x1ff);
            if(px>240) continue;
            compose_grid_figure(kGrid[r][c]-1);
            emit(ppu,px-ORIGIN_X+8,py-8-ORIGIN_Y,(unsigned)palettes[k],3,NULL,0,taken);
        }
    }
}
static void apply_line_loads(Ppu *ppu,unsigned line) {
    for(unsigned k=0;k<line_load_count;++k) if(line_loads[k].line==line || (line==1 && line_loads[k].line<1))
        load_palette(ppu,line_loads[k].palette,line_loads[k].key);
}
static void draw_portraits(Ppu *ppu) {
    int blocks[SKINS*8][4];memset(blocks,0xff,sizeof(blocks));
    for(unsigned i=0;i<5;++i) {
        int skin=skin_of(g_ram[0x625f+i]);
        if(skin<0) continue;
        unsigned key=(unsigned)skin*8+(skin>=2?g_ram[SEAT_COLOR+i]%SB4_COLORS:0);
        unsigned object=g_ram[0x62bf+i];
        if(object>=0x60 || g_ram[0x6df+object]>=8) continue;
        unsigned slot=g_ram[0x6df+object];
        unsigned left=(g_ram[0x55f+object]-8)&0x1ff,top=(g_ram[0x5bf+object]-25)&255;
        int quarter[4]={-1,-1,-1,-1};bool mine[128]={false};
        for(unsigned s=0;s<128;++s) {
            if(!visible(ppu,s) || !big(ppu,s) || sprite_palette(ppu,s)!=slot || ((ppu->oam[s*2+1]>>8)&0xc0)) continue;
            unsigned dx=(sprite_x(ppu,s)-left)&0x1ff,dy=((ppu->oam[s*2]>>8)-top)&255;
            if((dx==0 || dx==16) && (dy==0 || dy==16)) { quarter[(dy/16)*2+dx/16]=(int)s;mine[s]=true; }
        }
        if(quarter[0]<0 || quarter[1]<0 || quarter[2]<0 || quarter[3]<0) continue;
        bool usable=true;
        for(unsigned s=0;s<128;++s) if(visible(ppu,s) && sprite_palette(ppu,s)==slot && !mine[s]) usable=false;
        if(!usable) continue;
        if(blocks[key][0]<0) {
            /* Honey/Kotetsu: head and shoulders of the front standing frame,
             * its top 24 lines centred in the 32x32 portrait (native faces sit
             * at 4..27). Super Bomberman 4: the character's own Battle picker
             * portrait, with its palette for the player color. */
            for(unsigned q=0;q<4;++q) if((blocks[key][q]=borrow_block(ppu))<0) return;
            memset(canvas,0,sizeof(canvas));
            if(skin<2) {
                compose(anim_frame(kAnimBase[skin]+1,0),16,4+22);
                for(unsigned y=28;y<CANVAS_H;++y) memset(canvas[y],0,CANVAS_W);
            } else {
                uint8_t px[32][32];
                if(!Sb4Portrait((unsigned)skin-2,px)) return;
                for(unsigned y=0;y<32;++y) memcpy(canvas[y],px[y],32);
            }
            for(unsigned q=0;q<4;++q) paint_block(ppu,(unsigned)blocks[key][q],(q&1)*16,(q>>1)*16);
        }
        if(skin<2) load_palette(ppu,slot,key);
        else for(unsigned c=1;c<16;++c) put(&ppu->cgram[0x80+slot*16+c],Sb4PortraitColor((unsigned)skin-2,key%8,c));
        for(unsigned q=0;q<4;++q)
            put(&ppu->oam[quarter[q]*2+1],(uint16_t)((ppu->oam[quarter[q]*2+1]&~0x1ffu)|(unsigned)blocks[key][q]));
    }
}

/* Draws `w` x `h` color indices (0 = transparent, at most 48 x 64) at
 * screen (x, y) with its own 15 colors, in a palette no visible sprite
 * uses. For ability effects drawn during the stage overlay. */
static Ppu *effect_ppu;static bool *effect_taken;
bool ExtraEmitPixels(int x,int y,const uint8_t *pixels,unsigned w,unsigned h,const uint16_t colors[16],unsigned priority) {
    Ppu *ppu=effect_ppu;
    if(!ppu || w>CANVAS_W || h>CANVAS_H) return false;
    int palette=-1;
    for(int p=7;p>=0;--p) {
        if(!palette_given[p] && palette_unused(ppu,(unsigned)p)) { palette=p;break; }
        if(palette_given[p]) {
            bool same=true;
            for(unsigned c=1;c<16 && same;++c) same=ppu->cgram[0x80+p*16+c]==colors[c];
            if(same) { palette=p;break; }
        }
    }
    if(palette<0) return false;
    if(!palette_given[palette]) {
        for(unsigned c=1;c<16;++c) put(&ppu->cgram[0x80+palette*16+c],colors[c]);
        palette_given[palette]=true;
    }
    memset(canvas,0,sizeof(canvas));
    for(unsigned yy=0;yy<h;++yy) for(unsigned xx=0;xx<w;++xx) canvas[yy][xx]=pixels[yy*w+xx];
    return emit(ppu,x,y,(unsigned)palette,priority,NULL,0,effect_taken);
}

void HoneyKotetsuObserve(const Ppu *view,unsigned line,void *context) {
    (void)context;Ppu *ppu=(Ppu *)view;
    if(line==224) {
        line_load_count=0;
        if(!drawn) return;
        while(saved_count) { Saved w=saved[--saved_count];*w.word=w.value; }
        memcpy(ppu->highOam,saved_high,sizeof(saved_high));
        ++ppu->vramWriteCount;++ppu->oamWriteCount;drawn=false;return;
    }
    if(line!=1) {
        if(drawn && line_load_count) apply_line_loads(ppu,line);
        return;
    }
    if(!enabled || !patched || !art_ready || PPU_objSize(ppu)!=0) return;
    saved_count=0;borrowed_count=0;line_load_count=0;memcpy(saved_high,ppu->highOam,sizeof(saved_high));
    bool taken[128]={false};
    for(unsigned k=0;k<SKINS*8;++k) key_palette[k]=-1;
    memset(palette_given,0,sizeof(palette_given));
    if(picker()) { draw_portraits(ppu);draw_grid(ppu,taken);apply_line_loads(ppu,1); }
    else if(stage()) {
        draw_players(ppu,taken);draw_faces(ppu);
        effect_ppu=ppu;effect_taken=taken;SpecialAbilityDraw();effect_ppu=NULL;effect_taken=NULL;
    }
    if(saved_count || line_load_count) { drawn=true;++ppu->vramWriteCount;++ppu->oamWriteCount; }
}
