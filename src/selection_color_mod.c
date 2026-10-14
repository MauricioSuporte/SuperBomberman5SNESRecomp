/* Reuse cartridge palettes and cursor glyphs at runtime. No extracted assets. */
#include "selection_color_mod.h"
#include "common_rtl.h"
#include "mod_runtime.h"
#include "ptbr_mod.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
#include <stdbool.h>
#include <string.h>

static bool enabled, live;
static uint8_t colors[5];
static uint16_t previous[5];
static uint32_t picker_pc(void) {
    return g_ram[0xc90]|((uint32_t)g_ram[0xcb8]<<8)|((uint32_t)g_ram[0xce0]<<16);
}
static bool picker(void) {
    uint32_t pc=picker_pc();
    return g_ram[0x1031]==4 &&
        ((pc>=0xc68b48 && pc<0xc68ee4) ||
         (live && pc>=0xc6a300 && pc<0xc6a800 && g_ram[0x1032]==5));
}
static bool human(unsigned player) { return (g_ram[0x6269+player]&3)==1; }
void SelectionColorReset(void) {
    live=false;memset(colors,0,sizeof(colors));memset(previous,0,sizeof(previous));
}
static void reset_plugin(void) { SelectionColorReset();enabled=false; }
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_selection_colors) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.selection.colors",activate);
}
static unsigned word(unsigned offset) { return g_rom[offset]|g_rom[offset+1]<<8; }
static unsigned palette_source(unsigned id) {
    if(!id || id>0x47) return 0;
    unsigned table=word(0x379);
    unsigned entry=0x10000+table+(id-1)*2;
    return 0x10000+word(entry)+4;
}
static void upload_palette(unsigned source,unsigned destination,unsigned count) {
    if(!source || destination+count>256) return;
    for(unsigned i=0;i<count*2;++i) {
        uint8_t value=g_rom[source+i];
        if(g_ram[0x2910+destination*2+i]!=value) {
            g_ram[0x2910+destination*2+i]=value;g_ram[0x351]=1;
        }
    }
}
static void apply_color(unsigned player) {
    unsigned character=g_ram[0x625f+player];
    if(character<1 || character>9) return; /* The native random/exit cell. */
    unsigned color=colors[player];
    g_ram[0x6264+player]=(uint8_t)color;
    unsigned source=palette_source(g_rom[0xbb4e + character-1]+color);
    /* Portraits already have a dedicated native palette slot per seat. Keep
     * its allocation/refcount and replace the cartridge palette it names. */
    unsigned object=g_ram[0x62bf+player];
    if(object<0x60) {
        unsigned slot=g_ram[0x6df+object];
        if(slot==player && g_ram[0x2b18+slot]==0 && g_ram[0x349+slot]==1) {
            g_ram[0x2b10+slot]=g_rom[0xbb4e + character-1]+color;
            upload_palette(source,0x80+slot*16,9);
        }
    }
    /* Native grid entries use eight-color strips in the BG palette. The
     * cartridge lookup also handles each villain's original palette layout. */
    for(unsigned cell=0;cell<9;++cell) if(g_ram[0x62c9+cell]==character) {
        g_ram[0x62d3+cell]=(uint8_t)color;
        unsigned id=g_rom[0x2130b+(character-1)*7+color];
        upload_palette(palette_source(id),g_rom[0x21301+cell],8);
        break;
    }
}
void SelectionColorBeforeFrame(void) {
    if(!enabled) return;
    for(unsigned i=0;i<5;++i) {
        uint16_t pad=RtlGetPadState(i),pressed=pad & ~previous[i];
        previous[i]=pad;
        if(live && picker() && human(i) && !(g_ram[0x1038]&(1u<<i))) {
            unsigned shoulders=pressed & 0xc00;
            if(shoulders==0x800) colors[i]=(colors[i]+1)%6; /* R */
            else if(shoulders==0x400) colors[i]=(colors[i]+5)%6; /* L */
            g_ram[0x6264+i]=colors[i]; /* Includes simultaneous color + A. */
        }
    }
}
void SelectionColorAfterFrame(void) {
    if(!enabled) return;
    if(!picker()) { live=false;return; }
    if(!live && picker_pc()==0xc68e68) {
        live=true;memset(colors,0,sizeof(colors));
    }
    if(live) for(unsigned i=0;i<5;++i) if(human(i)) apply_color(i);
}

/* Render-only cursor changes; every guest OAM/VRAM word is restored. */
typedef struct { uint16_t address,value; } SavedWord;
static SavedWord saved_vram[5*3*16];
static unsigned saved_vram_count;
static uint16_t saved_oam[256];
static uint8_t saved_high[32];
static bool drawn;
static unsigned pixel(const Ppu *ppu,unsigned address,unsigned x,unsigned y) {
    unsigned color=0;
    for(unsigned p=0;p<4;++p)
        color|=((ppu->vram[(address+y+(p/2)*8)&0x7fff]>>((7-x)+(p%2)*8))&1)<<p;
    return color;
}
static unsigned tile_address(const Ppu *ppu,unsigned tile) {
    return ((tile&0x100)?PPU_objTileAdr2(ppu):PPU_objTileAdr1(ppu))+(tile&255)*16;
}
static void hint_tile(Ppu *ppu,unsigned tile,unsigned glyph,unsigned native_p) {
    uint8_t raster[8][8]={{0}},ink=0;
    for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) {
        unsigned c=pixel(ppu,native_p,x,y);
        if(c>1) { ink=(uint8_t)c;raster[y][x]=1; }
    }
    if(!ink) return;
    /* P's exact bowl/stem becomes R; L and + use that same stem, baseline,
     * ink and one-pixel outline rather than embedding a ripped font. */
    if(glyph==2) { raster[5][3]=1;raster[6][4]=1; }
    else {
        memset(raster,0,sizeof(raster));
        if(glyph==0) {
            for(unsigned y=1;y<7;++y) raster[y][2]=1;
            raster[6][3]=raster[6][4]=1;
        } else {
            raster[3][2]=raster[3][3]=raster[3][4]=1;
            raster[2][3]=raster[4][3]=1;
        }
    }
    uint8_t pixels[8][8]={{0}};
    for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) if(raster[y][x]) {
        for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
            int px=(int)x+dx,py=(int)y+dy;
            if(px>=0 && px<8 && py>=0 && py<8) pixels[py][px]=1;
        }
    }
    for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) if(raster[y][x]) pixels[y][x]=ink;
    unsigned base=tile_address(ppu,tile);
    for(unsigned y=0;y<8;++y) for(unsigned pair=0;pair<2;++pair) {
        uint16_t value=0;
        for(unsigned x=0;x<8;++x) for(unsigned plane=0;plane<2;++plane)
            if(pixels[y][x]&(1u<<(pair*2+plane))) value|=1u<<((7-x)+plane*8);
        unsigned a=(base+y+pair*8)&0x7fff;
        saved_vram[saved_vram_count++]=(SavedWord){(uint16_t)a,ppu->vram[a]};
        ppu->vram[a]=value;
    }
}
static void cursor_overlay(const Ppu *view,unsigned line,void *unused) {
    (void)unused;Ppu *ppu=(Ppu *)view;
    if(line==224 && drawn) {
        memcpy(ppu->oam,saved_oam,sizeof(saved_oam));memcpy(ppu->highOam,saved_high,sizeof(saved_high));
        while(saved_vram_count) { SavedWord w=saved_vram[--saved_vram_count];ppu->vram[w.address]=w.value; }
        ++ppu->vramWriteCount;++ppu->oamWriteCount;drawn=false;return;
    }
    if(line!=1 || !enabled || !live || !picker() || PPU_objSize(ppu)!=0) return;
    /* These fifteen tiles are borrowed only if no visible native OBJ uses
     * them. The cartridge owns them again before pending writes replay. */
    for(unsigned s=0;s<128;++s) if((ppu->oam[s*2]>>8)<224) {
        unsigned tile=ppu->oam[s*2+1]&0x1ff;
        if(tile>=0x1f0 && tile<=0x1fe) return;
    }
    memcpy(saved_oam,ppu->oam,sizeof(saved_oam));memcpy(saved_high,ppu->highOam,sizeof(saved_high));
    for(unsigned player=0;player<5;++player) {
        unsigned number=128,letter=128;
        for(unsigned s=0;s<128;++s) {
            unsigned pos=ppu->oam[s*2],attr=ppu->oam[s*2+1];
            if((pos>>8)<224 && (attr&0xfe00)==0x3c00) {
                if((attr&0x1ff)==6+player*2) number=s;
                if((attr&0x1ff)==7+player*2) letter=s;
            }
        }
        if(number==128 || letter==128) continue;
        unsigned npos=ppu->oam[number*2],ppos=ppu->oam[letter*2];
        unsigned y=npos>>8,x=npos&255;
        if((ppos>>8)!=y || (ppos&255)!=x+8 || y+48>224) continue;
        unsigned idle[3],found=0;
        for(unsigned s=0;s<128 && found<3;++s) if((ppu->oam[s*2]>>8)>=224) idle[found++]=s;
        if(found!=3) continue;
        unsigned native_p=tile_address(ppu,7+player*2);
        for(unsigned glyph=0;glyph<3;++glyph) {
            unsigned tile=0x1f0+player*3+glyph,s=idle[glyph];
            hint_tile(ppu,tile,glyph,native_p);
            ppu->oam[s*2]=(uint16_t)((y<<8)|((x-4+glyph*8)&255));
            ppu->oam[s*2+1]=(ppu->oam[letter*2+1]&0xfe00)|tile;
            ppu->highOam[s/4]&=(uint8_t)~(3u<<((s%4)*2));
        }
        ppu->oam[number*2]=(uint16_t)(((y+40)<<8)|x);
        ppu->oam[letter*2]=(uint16_t)(((y+40)<<8)|(ppos&255));
        drawn=true;
    }
    if(drawn) { ++ppu->vramWriteCount;++ppu->oamWriteCount; }
}
void SelectionColorDrawPpuFrame(void) {
    PtbrDrawPpuFrameObserved(enabled?cursor_overlay:NULL,NULL);
}
void SelectionColorSave(SaveLoadInfo *sli) {
    uint32_t magic=0x314c4353;uint8_t flag=live?1:0;
    sli->func(sli,&magic,sizeof(magic));sli->func(sli,&flag,1);
    sli->func(sli,colors,sizeof(colors));sli->func(sli,previous,sizeof(previous));
}
void SelectionColorLoad(SaveLoadInfo *sli,uint32_t version) {
    (void)version;SelectionColorReset();
    if(RtlStateBytesRemaining(sli)==0) return; /* v0.3.0 snapshots. */
    uint32_t magic=0;uint8_t flag=0;
    sli->func(sli,&magic,sizeof(magic));sli->func(sli,&flag,1);
    sli->func(sli,colors,sizeof(colors));sli->func(sli,previous,sizeof(previous));
    if(magic!=0x314c4353 || flag>1) { RtlStateStreamFail(sli);return; }
    for(unsigned i=0;i<5;++i) if(colors[i]>=6) { RtlStateStreamFail(sli);return; }
    live=flag!=0;
}
