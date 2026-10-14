/* Authored menu glyphs and translation metadata. No ROM assets are embedded. */
#include "ptbr_mod.h"
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
    unsigned slot, count, x, y, guard;
    const char *text;
} PtbrObjectLabel;
#include "ptbr_labels.h"

/* Five-column, seven-row uppercase alphabet authored for this overlay. */
static const uint8_t kFont[26][7] = {
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};
static bool s_enabled;
typedef struct { uint16_t address, original; } SavedWord;
static SavedWord s_saved[8192];
static unsigned s_saved_count;
static bool coop_menu_applied,coop_cursor_moved;
static uint8_t coop_windowed;
static uint16_t coop_cursor;

static void reset_ptbr(void) { s_enabled = false; s_saved_count = 0; }
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

static const uint8_t kDigits[10][7] = {
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14}
};

/* Own glyphs with the original menu's proportions, weight, shadow and palette.
 * Banners use a wider face and a blue bevel. No extracted pixels are stored. */
static void render_text(uint8_t pixels[32][224], const PtbrLabel *label) {
    unsigned width = label->width*8;
    memset(pixels, 0, 32*224);
    const unsigned char *at = (const unsigned char *)label->text;
    unsigned count=0;
    while(*at) { next_character(&at); ++count; }
    unsigned stride=label->style==1?8:7;
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
    unsigned glyph_width=label->style==0?5:stride-2;
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
        const uint8_t *glyph=c>='A'&&c<='Z'?kFont[c-'A']:c>='0'&&c<='9'?kDigits[c-'0']:c=='>'?arrow:NULL;
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

static void save_write(Ppu *ppu, unsigned address, uint16_t value) {
    address &= 0x7FFF;
    s_saved[s_saved_count++] = (SavedWord){(uint16_t)address, ppu->vram[address]};
    ppu->vram[address] = value;
}

/* Stage selection's blue heading is a strip of 16x16 OBJ tiles. Its
 * tiles are unique to this strip, so replace their pixels in place only
 * during rendering, preserving OAM, priority and the original palette. */
static void apply_object_labels(Ppu *ppu) {
    if(PPU_objSize(ppu)!=0 || PPU_bgTilemapAdr(ppu,0)!=0x3000u || ppu->vram[0x30C9]!=0x3092) return;
    for(size_t n=0;n<sizeof(kObjectLabels)/sizeof(kObjectLabels[0]);++n) {
        const PtbrObjectLabel *label=&kObjectLabels[n];
        bool matched=true;
        for(unsigned i=0;i<label->count;++i) {
            unsigned slot=label->slot+i, word=ppu->oam[slot*2+1];
            unsigned tile=((label->guard>>4)+(i/8)*2)*16+((label->guard+i*2)&15);
            if(ppu->oam[slot*2] != (label->y*256+label->x+i*16) ||
               word!=(0x2000u|tile) ||
               ((ppu->highOam[slot/4]>>((slot%4)*2))&3)!=2) { matched=false;break; }
        }
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

static void apply_labels(Ppu *ppu) {
    if ((ppu->bgmode & 7) != 1) return;
    enum { count=sizeof(kLabels)/sizeof(kLabels[0]) };
    bool matched[count];
    uint16_t palette[count];
    /* Decide from the guest surface before any replacement changes context
     * tiles. A banner and all of its rows may share the same screen guard. */
    for (size_t n=0;n<count;++n) {
        const PtbrLabel *label=&kLabels[n];
        unsigned map=PPU_bgTilemapAdr(ppu,label->layer);
        unsigned start=map+label->row*32+label->column;
        matched[n]=map==(0x3000u+label->layer*0x800u) &&
            (ppu->vram[start]&0x3FF)==label->guard0 &&
            (ppu->vram[start+1]&0x3FF)==label->guard1 &&
            (!label->context_tile || (ppu->vram[map+label->context_row*32+label->context_column]&0x3FF)==label->context_tile);
        palette[n]=matched[n]?(ppu->vram[start]&0x3C00):0;
    }
    for (size_t n=0;n<count;++n) {
        if(!matched[n]) continue;
        const PtbrLabel *label=&kLabels[n];
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
    apply_object_labels(ppu);
    if(s_saved_count) ++ppu->vramWriteCount;
}

static void restore_labels(Ppu *ppu) {
    if(!s_saved_count) return;
    while(s_saved_count) { SavedWord w=s_saved[--s_saved_count]; ppu->vram[w.address]=w.original; }
    if(coop_menu_applied) {
        ppu->screenWindowed[0]=coop_windowed;
        if(coop_cursor_moved) { ppu->oam[0]=coop_cursor;++ppu->oamWriteCount; }
        coop_menu_applied=coop_cursor_moved=false;
    }
    ++ppu->vramWriteCount;
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
        ppu->oam[0]=(uint16_t)(48+((unsigned)y&0xff)*256);
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
        if(!row) snprintf(text,sizeof(text),"%s",s_enabled?"APERTE START":"PUSH START BUTTON");
        else if(s_enabled) snprintf(text,sizeof(text),"  %u %s",row,row==1?"JOGADOR":"JOGADORES");
        else snprintf(text,sizeof(text),"  %u PLAYER GAME",row);
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
        if(s_enabled) apply_labels(ppu);apply_coop_menu(ppu);
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
    if(s_enabled || observer || NormalCoopPlayerMenu()) snes_beam_frame_driver_draw_ppu_frame_observed(observe_line,NULL);
    else snes_beam_frame_driver_draw_ppu_frame();
    extra_observer=NULL;extra_context=NULL;
}
