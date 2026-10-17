/* Runtime cartridge fonts and authored translations. No ROM assets embedded. */
#include "ptbr_mod.h"
#include "common_rtl.h"
#include "gold_battle_mod.h"
#include "normal_coop_mod.h"
#include "beam_frame_driver.h"
#include "mod_runtime.h"
#include "snes/ppu.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    unsigned layer, row, column, width, height, guard0, guard1, ink, tile, style, context_row, context_column, context_tile;
    const char *text;
} PtbrLabel;
typedef struct {
    unsigned slot, count, x, y, guard, palette;
    const char *text;
} PtbrObjectLabel;
typedef struct {
    uint16_t guard[24];
    const char *title, *tips[3];
} PtbrEnding;
#include "ptbr_labels.h"

#include "menu_font.h"
static bool s_enabled;
static uint16_t arena_oam[256];
static uint8_t arena_high[32];
static bool arena_drawn;
static unsigned arena_last;

typedef struct { uint16_t address, original; } SavedWord;
static SavedWord s_saved[8192];
static unsigned s_saved_count;
static bool coop_menu_applied,coop_cursor_moved;
static uint8_t coop_windowed;
static uint16_t coop_cursor;

static void reset_ptbr(void) { s_enabled = false; s_saved_count = 0;arena_drawn=false;arena_last=0; }
static void enable_ptbr(void) { s_enabled = true; }
SNES_MOD_CONSTRUCTOR(register_ptbr) {
    snes_mod_register_reset_callback(reset_ptbr);
    snes_mod_register_presentation_plugin("superbomberman5.ptbr.menus", enable_ptbr);
}

static unsigned next_character(const unsigned char **at) {
    unsigned c = *(*at)++;
    if (c == 0xC3) { unsigned d = *(*at)++; return 0xC0 + (d & 63); }
    return c;
}



/* Authored fallback for tests without a cartridge and missing signs.
 * The runtime menu families are provided by ptbr_native_font.h below. */
static void render_authored(uint8_t pixels[32][224], const PtbrLabel *label) {
    unsigned width = label->width*8;
    memset(pixels, 0, 32*224);
    const unsigned char *at = (const unsigned char *)label->text;
    unsigned count=0;
    while(*at) { next_character(&at); ++count; }
    unsigned stride=label->style==1?8:label->style==5?6:7;
    if(label->style==2 || label->style==4) {
        stride=width/count;
        if(stride>11) stride=11;
        if(label->style==2)
            for(unsigned y=0;y<16;++y) for(unsigned x=0;x<width;++x) pixels[y][x]=y<8?11:10;
    }
    if(label->style==3) {
        stride=18;
        for(unsigned y=24;y<label->height;++y) for(unsigned b=0;b<width;++b) pixels[y][b]=12;
    }
    unsigned glyph_width=label->style==0 || label->style==5?5:stride-2;
    unsigned x=(label->style>=2 || (label->style==1 && label->ink==3))?(width-count*stride)/2:0;
    at=(const unsigned char *)label->text;
    while (*at) {
        unsigned c = next_character(&at), accent = 0;
        if (c == 0xC7) { c = 'C'; accent = 2; }
        else if (c == 0xD5) { c = 'O'; accent = 1; }
        else if (c == 0xC3) { c = 'A'; accent = 1; }
        else if (c == 0xC1) { c = 'A'; accent = 3; }
        else if (c == 0xC9) { c = 'E'; accent = 3; }
        else if (c == 0xCD) { c = 'I'; accent = 3; }
        else if (c == 0xD3) { c = 'O'; accent = 3; }
        else if (c == 0xDA) { c = 'U'; accent = 3; }
        else if (c == 0xC2) { c = 'A'; accent = 4; }
        else if (c == 0xCA) { c = 'E'; accent = 4; }
        else if (c == 0xD4) { c = 'O'; accent = 4; }
        static const uint8_t arrow[7]={8,4,2,1,2,4,8};
        static const uint8_t punctuation[3][7]={{14,17,1,2,4,0,4},{4,4,4,4,4,0,4},{25,25,2,4,8,19,19}};
        const uint8_t *glyph=c>='A'&&c<='Z'?kMenuLetters[c-'A']:c>='0'&&c<='9'?kMenuDigits[c-'0']:c=='>'?arrow:c=='?'?punctuation[0]:c=='!'?punctuation[1]:c=='%'?punctuation[2]:NULL;
        unsigned top=label->style==1?4:accent && accent!=2?2:0, height=(label->height==32?28:14)-top;
        uint8_t mask[32][20]={{0}};
        if(glyph) {
            bool bold=label->style==1 || label->style==2 || label->style==4;
            unsigned raster_width=glyph_width-(bold?1:0);
            for(unsigned y=0;y<height;++y) for(unsigned b=0;b<raster_width;++b)
                if(glyph[y*7/height] & (1u<<(4-b*5/raster_width))) {
                    mask[y+top][b]=1;
                    if(bold) mask[y+top][b+1]=1;
                }
            if(accent==1) { mask[0][1]=1;mask[0][3]=1;mask[1][2]=1; }
            if(accent==2) { mask[14][2]=1;mask[15][1]=1; }
            if(accent==3) { mask[0][3]=1;mask[1][2]=1; }
            if(accent==4) { mask[0][2]=1;mask[1][1]=1;mask[1][3]=1; }
            for(unsigned y=0;y<label->height-1;++y) for(unsigned b=0;b<glyph_width;++b)
                if(mask[y][b] && x+b+1<width) {
                    pixels[y+1][x+b+1]=1;
                    if(label->style==1 || label->style==2 || label->style==4) {
                        if(y) pixels[y-1][x+b]=1;
                        if(x+b) pixels[y][x+b-1]=1;
                        pixels[y][x+b+1]=1;
                    }
                }
            for(unsigned y=0;y<label->height;++y) for(unsigned b=0;b<glyph_width;++b)
                if(mask[y][b] && x+b<width) pixels[y][x+b]=label->style==2?(y<7?7:3):label->style==4?(y<7?6:3):label->ink;
        }
        x+=stride;
    }
}

#include "ptbr_native_font.h"

static void save_write(Ppu *ppu, unsigned address, uint16_t value) {
    address &= 0x7FFF;
    s_saved[s_saved_count++] = (SavedWord){(uint16_t)address, ppu->vram[address]};
    ppu->vram[address] = value;
}

/* Certificates share an authored character atlas instead of reserving a tile
 * for every letter position. This keeps long pages below the BG tile budget.
 * Recognition uses the complete native rank row, never the percentage alone:
 * the game also distinguishes boss routes, endings and secret-stage progress. */
typedef struct { uint16_t tiles[256]; unsigned next; bool opaque; } EndingFont;
static void ending_clear(Ppu *ppu, unsigned row, unsigned column, unsigned width, unsigned height) {
    for(unsigned y=0;y<height;++y) for(unsigned x=0;x<width;++x)
        save_write(ppu,0x3000+(row+y)*32+column+x,0x20cd);
}
static void ending_text(Ppu *ppu, EndingFont *font, unsigned row, unsigned column, unsigned width, const char *text) {
    const unsigned char *at=(const unsigned char *)text;
    unsigned count=0;
    while(*at) { next_character(&at);++count; }
    if(count>width) return;
    if(font->opaque) {
        for(unsigned y=0;y<2;++y) for(unsigned x=0;x<width;++x)
            save_write(ppu,0x3000+(row+y)*32+column+x,0x22fe);
    } else ending_clear(ppu,row,column,width,2);
    column+=(width-count)/2;
    at=(const unsigned char *)text;
    for(unsigned x=0;*at;++x) {
        const unsigned char *begin=at;
        unsigned c=next_character(&at);
        if(c==' ') continue;
        if(c>=256) return;
        if(!font->tiles[c]) {
            char letter[3]={0};memcpy(letter,begin,(size_t)(at-begin));
            PtbrLabel label={.width=1,.height=16,.ink=2,.text=letter};
            uint8_t pixels[32][224];render_text(pixels,&label);
            if(font->opaque) for(unsigned y=0;y<16;++y) for(unsigned b=0;b<8;++b)
                if(!pixels[y][b]) pixels[y][b]=1;
            unsigned tile=font->next;font->next+=2;font->tiles[c]=(uint16_t)tile;
            for(unsigned y=0;y<16;++y) {
                uint16_t planes[2]={0};
                for(unsigned b=0;b<8;++b) for(unsigned plane=0;plane<4;++plane)
                    if(pixels[y][b]&(1u<<plane)) planes[plane/2]|=(uint16_t)(1u<<(7-b+(plane%2)*8));
                unsigned address=(tile+y/8)*16+y%8;
                save_write(ppu,address,planes[0]);save_write(ppu,address+8,planes[1]);
            }
        }
        for(unsigned y=0;y<2;++y)
            save_write(ppu,0x3000+(row+y)*32+column+x,(uint16_t)(0x2000|font->tiles[c]+y));
    }
}
static void apply_endings(Ppu *ppu) {
    if((ppu->bgmode&7)!=1 || PPU_bgTilemapAdr(ppu,0)!=0x3000 || PPU_bgTileAdr(ppu,0)!=0) return;
    bool certificate=ppu->vram[0x3144]==0x2107 && ppu->vram[0x3145]==0x2108;
    bool tips=ppu->vram[0x30ae]==0x21e8 && ppu->vram[0x32c6]==0x21e9;
    if(!certificate && !tips) return;
    const PtbrEnding *ending=NULL;
    unsigned rank_row=certificate?19:8;
    for(unsigned n=0;n<sizeof(kEndings)/sizeof(kEndings[0]);++n) {
        bool matched=true;
        for(unsigned x=0;x<24;++x)
            if((ppu->vram[0x3000+rank_row*32+4+x]&1023)!=kEndings[n].guard[x]) {matched=false;break;}
        if(matched) {ending=&kEndings[n];break;}
    }
    if(!ending || s_saved_count || sizeof(s_saved)/sizeof(s_saved[0])<4096) return;
    EndingFont font={.next=0x220};
    if(certificate) {
        ending_clear(ppu,4,3,26,6);
        ending_text(ppu,&font,6,4,24,kCompletion_certificate);
        ending_text(ppu,&font,10,4,24,kCompletion_cleared);
        /* Preserve the three native digits and percent sign at columns 4-10. */
        ending_text(ppu,&font,13,11,17,kCompletion_rank_intro);
        ending_text(ppu,&font,16,4,24,kCompletion_rank_award);
        ending_text(ppu,&font,19,4,24,ending->title);
        /* The signature is baked into BG2's parchment. Cover only its paper
         * rectangle with the matching native palette entry, keeping the seal
         * and border intact. Other page text remains transparent. */
        memset(font.tiles,0,sizeof(font.tiles));font.next=0x2a0;font.opaque=true;
        for(unsigned y=0;y<8;++y) {save_write(ppu,0x2fe0+y,0x00ff);save_write(ppu,0x2fe8+y,0);}
        ending_text(ppu,&font,24,9,20,kCompletion_signature);
    } else {
        ending_text(ppu,&font,5,4,24,kCompletion_tips);
        ending_text(ppu,&font,8,4,24,ending->title);
        ending_clear(ppu,12,4,24,8);
        for(unsigned i=0;i<3;++i) ending_text(ppu,&font,12+i*3,4,24,ending->tips[i]);
        /* Native digits, percent sign and punctuation remain at 19-27. */
        ending_text(ppu,&font,22,6,13,kCompletion_progress);
    }
    ++ppu->vramWriteCount;
}

/* Defeat is Mode 7 with OBJ lettering. Match every piece of each native
 * strip, including palette, position and size, before replacing its unique
 * tiles. Both halves of the large title retain the native inter-word gap. */
static bool defeat_strip(Ppu *ppu, unsigned x, unsigned y, unsigned columns,
                         unsigned rows, unsigned first, const char *text) {
    unsigned slots[16],found=0;
    for(unsigned row=0;row<rows;++row) for(unsigned col=0;col<columns;++col) {
        unsigned tile=first+row*32+col*2,slot=128;
        for(unsigned i=0;i<128;++i) {
            if(ppu->oam[i*2+1]!=(0x2400|tile) || (ppu->oam[i*2]>>8)>=224) continue;
            if(slot!=128 || ppu->oam[i*2]!=((x+col*16)|((y+row*16)<<8)) ||
               ((ppu->highOam[i/4]>>(i%4*2))&3)!=2) return false;
            slot=i;
        }
        if(slot==128) return false;
        slots[found++]=slot;
    }
    if(s_saved_count+found*64>sizeof(s_saved)/sizeof(s_saved[0])) return false;
    PtbrLabel label={.width=columns*2,.height=rows*16,.style=rows==2?4:0,.ink=12,.text=text};
    uint8_t pixels[32][224];render_text(pixels,&label);
    for(unsigned i=0;i<found;++i) {
        unsigned tile=ppu->oam[slots[i]*2+1]&255;
        for(unsigned dy=0;dy<16;++dy) for(unsigned tx=0;tx<2;++tx) {
            uint16_t planes[2]={0};
            for(unsigned dx=0;dx<8;++dx) for(unsigned plane=0;plane<4;++plane)
                if(pixels[i/columns*16+dy][i%columns*16+tx*8+dx]&(1u<<plane))
                    planes[plane/2]|=(uint16_t)(1u<<(7-dx+plane%2*8));
            unsigned address=PPU_objTileAdr1(ppu)+(tile+dy/8*16+tx)*16+dy%8;
            save_write(ppu,address,planes[0]);save_write(ppu,address+8,planes[1]);
        }
    }
    return true;
}
static void apply_defeat(Ppu *ppu) {
    if((ppu->bgmode&7)!=7 || PPU_objSize(ppu)!=0 || PPU_bgTilemapAdr(ppu,0)!=0x3000) return;
    unsigned before=s_saved_count;
    defeat_strip(ppu,62,23,4,2,0x00,kCompletion_defeat_left);
    defeat_strip(ppu,134,23,4,2,0x08,kCompletion_defeat_right);
    defeat_strip(ppu,98,159,4,1,0x40,kCompletion_continue);
    defeat_strip(ppu,98,183,2,1,0x48,kCompletion_end);
    if(s_saved_count!=before) ++ppu->vramWriteCount;
}

/* Both arena headings use unique 16x16 OBJ strips. The confirmation strip
 * starts at A2 rather than a row boundary and uses the native pink palette. */
static unsigned object_label_tile(const PtbrObjectLabel *label,unsigned i) {
    unsigned n=(label->guard&15)/2+i;
    return (label->guard&0xf0)+(n/8)*32+(n%8)*2;
}
static bool object_label_matches(const Ppu *ppu,const PtbrObjectLabel *label) {
    for(unsigned i=0;i<label->count;++i) {
        unsigned slot=label->slot+i,tile=object_label_tile(label,i);
        if(ppu->oam[slot*2]!=(label->y*256+label->x+i*16) ||
           ppu->oam[slot*2+1]!=(0x2000u|(label->palette<<9)|tile) ||
           ((ppu->highOam[slot/4]>>((slot%4)*2))&3)!=2) return false;
    }
    return true;
}
static void apply_object_labels(Ppu *ppu) {
    if(PPU_objSize(ppu)!=0 || PPU_bgTilemapAdr(ppu,0)!=0x3000u) return;
    for(size_t n=0;n<sizeof(kObjectLabels)/sizeof(kObjectLabels[0]);++n) {
        const PtbrObjectLabel *label=&kObjectLabels[n];
        bool matched=object_label_matches(ppu,label);
        if(!matched || label->count*64 > sizeof(s_saved)/sizeof(s_saved[0])-s_saved_count) continue;
        PtbrLabel face={.width=label->count*2,.height=16,.style=4,.text=label->text};
        uint8_t pixels[32][224]; render_text(pixels,&face);
        for(unsigned i=0;i<label->count;++i) {
            unsigned tile=ppu->oam[(label->slot+i)*2+1]&255;
            for(unsigned ty=0;ty<2;++ty) for(unsigned tx=0;tx<2;++tx) for(unsigned y=0;y<8;++y) {
                uint16_t planes[2]={0,0};
                for(unsigned x=0;x<8;++x) {
                    unsigned color=pixels[ty*8+y][i*16+tx*8+x];
                    for(unsigned plane=0;plane<4;++plane)
                        if(color&(1u<<plane)) planes[plane/2]|=(uint16_t)(1u<<((7-x)+(plane%2)*8));
                }
                unsigned address=PPU_objTileAdr1(ppu)+(tile+ty*16+tx)*16+y;
                save_write(ppu,address,planes[0]);save_write(ppu,address+8,planes[1]);
            }
        }
    }
}

/* Battle names use the cartridge's own 8x16 proportional OBJ alphabet.
 * Decode it before borrowing tiles, so shared native letters never change.
 * Only accent marks and their outline are authored here. */
static unsigned arena_glyph(Ppu *p,unsigned c,uint8_t pixels[16][8]) {
    unsigned accent=0;
    switch(c) {
    case 0xe7:c='c';accent=1;break;
    case 0xc7:c='C';accent=1;break;
    case 0xe3:c='a';accent=2;break;
    case 0xf5:c='o';accent=2;break;
    case 0xe1:c='a';accent=3;break;
    case 0xe9:c='e';accent=3;break;
    case 0xed:c='i';accent=3;break;
    case 0xf3:c='o';accent=3;break;
    case 0xfa:c='u';accent=3;break;
    case 0xe2:c='a';accent=4;break;
    case 0xea:c='e';accent=4;break;
    case 0xf4:c='o';accent=4;break;
    }
    memset(pixels,0,16*8);
    if(c==' ') return 6;
    unsigned tile=c>='A'&&c<='Z'?2+2*(c-'A'):
                  c>='a'&&c<='z'?0x36+2*(c-'a'):
                  c=='!'?0x6e:c=='?'?0x70:c=='-'?0x7c:0;
    if(!tile) return 0;
    unsigned base=PPU_objTileAdr1(p);
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<8;++x)
        for(unsigned plane=0;plane<4;++plane)
            pixels[y][x]|=((p->vram[(base+(tile+y/8)*16+y%8+(plane/2)*8)&0x7fff]>>((7-x)+(plane%2)*8))&1u)<<plane;
    unsigned ax[4],ay[4],count=0;
    if(accent==1) { ax[0]=3;ay[0]=14;ax[1]=2;ay[1]=15;count=2; }
    if(accent==2) { ax[0]=2;ay[0]=1;ax[1]=3;ay[1]=0;ax[2]=4;ay[2]=1;ax[3]=5;ay[3]=0;count=4; }
    if(accent==3) { ax[0]=4;ay[0]=1;ax[1]=3;ay[1]=2;count=2; }
    if(accent==4) { ax[0]=2;ay[0]=2;ax[1]=3;ay[1]=1;ax[2]=4;ay[2]=2;count=3; }
    for(unsigned i=0;i<count;++i) for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
        int x=(int)ax[i]+dx,y=(int)ay[i]+dy;
        if(x>=0 && x<8 && y>=0 && y<16 && !pixels[y][x]) pixels[y][x]=1;
    }
    for(unsigned i=0;i<count;++i) pixels[ay[i]][ax[i]]=4;
    return c=='i'||c=='l'||c=='I'?4:c=='r'||c=='t'||c=='f'?6:c=='!'?3:c=='-'?6:7;
}
static unsigned arena_text(Ppu *p,const char *text,uint8_t canvas[16][208]) {
    memset(canvas,0,16*208);
    const unsigned char *at=(const unsigned char *)text;unsigned left=0;
    while(*at) {
        uint8_t glyph[16][8];unsigned width=arena_glyph(p,next_character(&at),glyph);
        if(!width || left+8>208) return 0;
        for(unsigned y=0;y<16;++y) for(unsigned x=0;x<8;++x)
            if(!canvas[y][left+x]) canvas[y][left+x]=glyph[y][x];
        left+=width;
    }
    return left;
}
static unsigned arena_number(const Ppu *p) {
    /* Native 16x16 number art: CA/CC/CE, E0..EE, 100/102. It is the
     * displayed arena, unlike live $625A which can lead OAM by a frame. */
    /* Confirmation inserts one OBJ before the arena label. Match the native
     * numeric sprite's position instead of assuming slot 12. */
    static const unsigned digit_slots[3]={3,12,13};
    for(unsigned index=0;index<3;++index) {
        unsigned slot=digit_slots[index];
        unsigned tile=p->oam[slot*2+1]&0x1ff;
        if(p->oam[slot*2]!=(171u*256+138) ||
           (p->oam[slot*2+1]&0xfe00)!=0x2000 ||
           ((p->highOam[slot/4]>>((slot%4)*2))&3)!=2) continue;
        for(unsigned arena=1;arena<=13;++arena) {
            unsigned n=10+2*(arena-1),expected=0xc0+(n/16)*32+n%16;
            if(tile==expected) return arena;
        }
    }
    return 0;
}
static void arena_write_line(Ppu *p,uint8_t canvas[16][208],unsigned width,unsigned x,unsigned y,
                             unsigned palette,unsigned *tile,unsigned slots[128],unsigned *slot_at) {
    unsigned base=PPU_objTileAdr2(p),columns=(width+7)/8;
    for(unsigned cy=0;cy<2;++cy) for(unsigned cx=0;cx<columns;++cx) {
        unsigned t=(*tile)++,slot=slots[(*slot_at)++];
        for(unsigned py=0;py<8;++py) {
            uint16_t planes[2]={0};
            for(unsigned px=0;px<8;++px) {
                unsigned c=canvas[cy*8+py][cx*8+px];
                for(unsigned plane=0;plane<4;++plane) if(c&(1u<<plane)) planes[plane/2]|=1u<<(7-px+(plane%2)*8);
            }
            save_write(p,(base+(t-256)*16+py)&0x7fff,planes[0]);
            save_write(p,(base+(t-256)*16+py+8)&0x7fff,planes[1]);
        }
        p->oam[slot*2]=(uint16_t)(((y+cy*8)<<8)|(x+cx*8));
        p->oam[slot*2+1]=(uint16_t)(0x2000|(palette<<9)|t);
        p->highOam[slot/4]&=(uint8_t)~(3u<<((slot%4)*2));
    }
}
static void apply_battle_arena(Ppu *p) {
    if(PPU_objSize(p)!=0 || PPU_objTileAdr1(p)!=0x6000) { arena_last=0;return; }
    bool matched=false;
    for(unsigned n=0;n<sizeof(kObjectLabels)/sizeof(kObjectLabels[0]);++n)
        if(object_label_matches(p,&kObjectLabels[n])) {matched=true;break;}
    if(!matched) {
        /* The confirmation heading blinks by removing its OBJ entirely.
         * The remaining native Stage strip moves to slots 0-2. */
        matched=true;
        for(unsigned i=0;i<3;++i)
            if(p->oam[i*2]!=(171u*256+90+i*16) || p->oam[i*2+1]!=(0x24c4+i*2) ||
               ((p->highOam[i/4]>>((i%4)*2))&3)!=2) {matched=false;break;}
    }
    if(!matched) {arena_last=0;return;}
    unsigned arena=arena_number(p);if(arena) arena_last=arena;else arena=arena_last;
    bool replace[128]={0},used[512]={0};unsigned slots[128],slot_count=0;
    for(unsigned slot=0;slot<128;++slot) {
        unsigned xy=p->oam[slot*2],word=p->oam[slot*2+1],y=xy>>8,tile=word&0x1ff;
        bool visible=y<224 || y>=240,big=(p->highOam[slot/4]>>((slot%4)*2+1))&1;
        /* Hide every native name glyph, including transitional positions.
         * Keep the native numeric sprite and the preview/cursor artwork. */
        replace[slot]=(y==171 && (word==0x24c4 || word==0x24c6 || word==0x24c8) &&
                       (xy&255)==90+((word-0x24c4)/2)*16) ||
                      ((word&0xe00)==0x200 && tile<0x80 && visible);
        if(replace[slot] || (slot>=9 && !visible)) slots[slot_count++]=slot;
        if(visible) {
            used[tile]=true;
            if(big) { used[(tile+1)&511]=true;used[(tile+16)&511]=true;used[(tile+17)&511]=true; }
        }
    }
    uint8_t name[16][208],stage[16][208];
    unsigned nw=arena?arena_text(p,kBattleArenas[arena-1],name):0;
    unsigned sw=arena_text(p,kBattleStage,stage),need=nw?2*((nw+7)/8+(sw+7)/8):0,first=0;
    if(need && need<=slot_count) for(unsigned t=256;t+need<=496;++t) {
        bool free=true;for(unsigned i=0;i<need;++i) if(used[t+i]) { free=false;break; }
        if(free) { first=t;break; }
    }
    if(need*16>sizeof(s_saved)/sizeof(s_saved[0])-s_saved_count) return;
    memcpy(arena_oam,p->oam,sizeof(arena_oam));memcpy(arena_high,p->highOam,sizeof(arena_high));arena_drawn=true;
    for(unsigned slot=0;slot<128;++slot) if(replace[slot]) p->oam[slot*2]=(224u<<8)|(p->oam[slot*2]&255);
    if(first) {
        unsigned tile=first,slot_at=0;
        arena_write_line(p,stage,sw,104,171,2,&tile,slots,&slot_at);
        arena_write_line(p,name,nw,(256-nw)/2,195,1,&tile,slots,&slot_at);
    }
    ++p->oamWriteCount;
}

/* While a stage is being played the arena's BG1 can hold a menu label's
 * guard tiles by chance: ZONE2's boss arena put 328/329 at the file menu's
 * "Continue" position and "Continuar" appeared over the blocks. In a stage
 * mode ($0D-$73 Normal, $74+ Battle) with the gameplay byte $0AEE set or a
 * bomber on the field, only the HUD labels (BG3 row 0) apply. Game Over and
 * its password reward run after the players are gone. */
static bool playing_stage(void) {
    unsigned mode=g_ram[0x1031];
    bool battle=(g_ram[0x1d1a]&1)!=0;
    if(battle?mode<0x74:(mode<0x0d || mode>0x73)) return false;
    if(g_ram[0x0aee]) return true;
    for(unsigned i=0;i<5;++i) if(g_ram[0x17fa+i]<0x80) return true;
    return false;
}
static void apply_labels(Ppu *ppu) {
    if(s_enabled) { apply_defeat(ppu);apply_endings(ppu); }
    if ((ppu->bgmode & 7) != 1) return;
    bool stage=playing_stage();
    enum { count=sizeof(kLabels)/sizeof(kLabels[0]) };
    bool matched[count];
    uint16_t palette[count];
    /* Decide from the guest surface before any replacement changes context
     * tiles. A banner and all of its rows may share the same screen guard. */
    for (size_t n=0;n<count;++n) {
        const PtbrLabel *label=&kLabels[n];
        unsigned map=PPU_bgTilemapAdr(ppu,label->layer);
        unsigned start=map+label->row*32+label->column;
        matched[n]=(!stage || (label->layer==2 && label->row==0)) &&
            map==(0x3000u+label->layer*0x800u) &&
            (ppu->vram[start]&0x3FF)==label->guard0 &&
            (ppu->vram[start+1]&0x3FF)==label->guard1 &&
            (!label->context_tile || (ppu->vram[map+label->context_row*32+label->context_column]&0x3FF)==label->context_tile);
        palette[n]=matched[n]?(ppu->vram[start]&0x3C00):0;
    }
    for (size_t n=0;n<count;++n) {
        if(!matched[n]) continue;
        PtbrLabel translated=kLabels[n];
        bool hyper=GoldBattleHyper() && translated.row==18 && translated.column==22;
        if(!s_enabled && !hyper) continue;
        if(hyper) translated.text=s_enabled?kGoldHyper:"HYPER";
        const PtbrLabel *label=&translated;
        unsigned map=PPU_bgTilemapAdr(ppu,label->layer), base=PPU_bgTileAdr(ppu,label->layer);
        unsigned start=map+label->row*32+label->column;
        uint8_t pixels[32][224];
        render_text(pixels,label);
        unsigned rows=label->height/8, bpp=label->layer!=2?4:2;
        if(rows*label->width*(1+bpp*4) > sizeof(s_saved)/sizeof(s_saved[0])-s_saved_count) continue;
        uint16_t attributes=palette[n];
        for (unsigned ty=0; ty<rows; ++ty) for (unsigned tx=0; tx<label->width; ++tx) {
            unsigned tile=label->tile+ty*label->width+tx;
            unsigned map_addr=start+ty*32+tx;
            save_write(ppu,map_addr,(uint16_t)(attributes|tile));
            for (unsigned y=0; y<8; ++y) {
                uint16_t planes[2]={0,0};
                for (unsigned x=0;x<8;++x) {
                    unsigned color=pixels[ty*8+y][tx*8+x], bit=7-x;
                    for(unsigned plane=0;plane<bpp;++plane)
                        if(color & (1u<<plane)) planes[plane/2]|=(uint16_t)(1u<<(bit+(plane%2)*8));
                }
                save_write(ppu,base+tile*(bpp*4)+y,planes[0]);
                if(bpp==4) save_write(ppu,base+tile*16+y+8,planes[1]);
            }
        }
    }
    if(s_enabled) { apply_object_labels(ppu);apply_battle_arena(ppu); }
    if(s_saved_count) ++ppu->vramWriteCount;
}

static void restore_labels(Ppu *ppu) {
    if(!s_saved_count && !arena_drawn) return;
    if(arena_drawn) {
        memcpy(ppu->oam,arena_oam,sizeof(arena_oam));memcpy(ppu->highOam,arena_high,sizeof(arena_high));
        ++ppu->oamWriteCount;arena_drawn=false;
    }
    while(s_saved_count) { SavedWord w=s_saved[--s_saved_count]; ppu->vram[w.address]=w.original; }
    if(coop_menu_applied) {
        ppu->screenWindowed[0]=coop_windowed;
        if(coop_cursor_moved) { ppu->oam[0]=coop_cursor;++ppu->oamWriteCount; }
        coop_menu_applied=coop_cursor_moved=false;
    }
    ++ppu->vramWriteCount;
}

static void coop_menu_text(unsigned row, char *text, size_t capacity) {
    if(!row) snprintf(text,capacity,"%s",s_enabled?"APERTE START":"PUSH START BUTTON");
    else if(s_enabled) snprintf(text,capacity,"  %u %s",row,row==1?"JOGADOR":"JOGADORES");
    else snprintf(text,capacity,"  %u %s",row,row==1?"PLAYER":"PLAYERS");
}

/* Compact five-row title layout. Copyright remains in its original row 26.
 * All writes are temporary, including when PT-BR is disabled. */
static void apply_coop_menu(Ppu *ppu) {
    if(!NormalCoopPlayerMenu() || (ppu->bgmode&7)!=1 || PPU_bgTilemapAdr(ppu,2)!=0x4000) return;
    unsigned map=PPU_bgTilemapAdr(ppu,2),base=PPU_bgTileAdr(ppu,2);
    coop_menu_applied=true;coop_windowed=ppu->screenWindowed[0];
    ppu->screenWindowed[0]&=(uint8_t)~4;
    /* Native cursor: tile $1E8 idle, $1EA-$1EE during the confirmation bounce
     * (y within -5..+1 of the 1P/2P rows at $7F/$8F). The idle cursor sits on
     * the selected row; Up/Down toggles the stock two-row y for a frame, so
     * only the bounce keeps its offset. */
    if((ppu->oam[1]&0xfff9)==0x2be8 && (ppu->oam[0]&0xff)==0x40) {
        int target=(12+(int)NormalCoopPlayerCount()*2)*8-1,y=target;
        if(ppu->oam[1]!=0x2be8) y+=(ppu->oam[0]>>8)-((ppu->oam[0]>>8)>=0x87?0x8f:0x7f);
        coop_cursor_moved=true;coop_cursor=ppu->oam[0];
        char text[32];coop_menu_text(NormalCoopPlayerCount(),text,sizeof(text));
        /* Both languages center each row. Follow its first digit while
         * preserving the native eight-pixel cursor/digit cell gap. */
        unsigned digit=48+(160-(unsigned)strlen(text)*8)/2+16;
        unsigned x=digit-24;
        ppu->oam[0]=(uint16_t)(x+((unsigned)y&0xff)*256);
        ++ppu->oamWriteCount;
    }
    for(unsigned row=10;row<24;++row) for(unsigned col=6;col<28;++col)
        save_write(ppu,map+row*32+col,0x2000);
    /* The native heading blinks through BG3 window 2 (W34SEL bit 11), whose
     * HDMA band covers its original rows only. Follow the same phase here. */
    const bool heading_hidden=(ppu->windowsel & 0x800)!=0;
    for(unsigned row=0;row<6;++row) {
        char text[32];
        if(!row && heading_hidden) continue;
        coop_menu_text(row,text,sizeof(text));
        PtbrLabel label={.width=20,.height=16,.style=1,.ink=3,.text=text};
        uint8_t pixels[32][224];render_text(pixels,&label);
        unsigned yrow=row?12+row*2:12;
        for(unsigned ty=0;ty<2;++ty) for(unsigned tx=0;tx<20;++tx) {
            /* The native text starts at tile $100; OBJ graphics start past
             * this font page. Use only its unused lower 240 tiles. Tile zero
             * remains blank because the heading has a centered left margin. */
            unsigned tile=row*40+ty*20+tx;
            save_write(ppu,map+(yrow+ty)*32+6+tx,(uint16_t)(0x2000|tile));
            for(unsigned y=0;y<8;++y) {
                uint16_t planes=0;
                for(unsigned x=0;x<8;++x) {
                    unsigned color=pixels[ty*8+y][tx*8+x];
                    /* Native colors: the heading fills with BG3 palette 0
                     * color 3 (yellow), player rows with color 2 (white). */
                    if(row && color==3) color=2;
                    if(color&1) planes|=1u<<(7-x);
                    if(color&2) planes|=1u<<(15-x);
                }
                save_write(ppu,base+tile*8+y,planes);
            }
        }
    }
    ++ppu->vramWriteCount;
}

static SnesBeamRasterObserver *extra_observer;
static void *extra_context;
static uint32_t coop_render_flags;
static bool coop_renderer_changed;
static void observe_line(const Ppu *view, unsigned line, void *unused) {
    (void)unused;
    Ppu *ppu=(Ppu *)view;
    if(line==1) {
        coop_renderer_changed=NormalCoopPlayerMenu();
        if(coop_renderer_changed) {
            coop_render_flags=ppu->renderFlags;
            ppu->renderFlags&=~kPpuRenderFlags_NewRenderer;
        }
        if(s_enabled || GoldBattleHyper()) apply_labels(ppu);apply_coop_menu(ppu);
    }
    if(extra_observer) extra_observer(view,line,extra_context);
    /* Restore before the driver replays pending guest writes at RenderEnd. */
    if(line==224) {
        restore_labels(ppu);
        if(coop_renderer_changed) ppu->renderFlags=coop_render_flags;
        coop_renderer_changed=false;
    }
}

void PtbrDrawPpuFrame(void) {
    PtbrDrawPpuFrameObserved(NULL,NULL);
}
void PtbrDrawPpuFrameObserved(SnesBeamRasterObserver *observer,void *context) {
    /* The optimized compositor does not reproduce the title's temporary
     * BG3 replacement correctly. Use the accurate compositor for this
     * menu only, keeping the user's renderer setting for every other screen. */
    extra_observer=observer;extra_context=context;
    if(s_enabled || GoldBattleHyper() || observer || NormalCoopPlayerMenu()) snes_beam_frame_driver_draw_ppu_frame_observed(observe_line,NULL);
    else snes_beam_frame_driver_draw_ppu_frame();
    extra_observer=NULL;extra_context=NULL;
}

const char *PtbrPauseLabel(unsigned index) {
    static const char *const english[]={"ARE YOU SURE","NO","YES","RESUME","FORCE DRAW","RETURN TO WORLD MAP","RETURN TO STAGE SELECT"};
    return index<7?(s_enabled?kPauseText[index]:english[index]):"";
}
