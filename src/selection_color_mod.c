/* Reuse cartridge palettes and cursor glyphs at runtime. No extracted assets. */
#include "selection_color_mod.h"
#include "common_rtl.h"
#include "mod_runtime.h"
#include "ptbr_mod.h"
#include "subordinate_mod.h"
#include "honey_kotetsu_mod.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
#include <stdbool.h>
#include <string.h>

static bool enabled, live, pending;
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
void SelectionColorReset(void) {
    live=pending=false;memset(colors,0,sizeof(colors));memset(previous,0,sizeof(previous));
}
static void reset_plugin(void) { SelectionColorReset();enabled=false; }
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_selection_colors) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.selection.colors",activate);
}
static unsigned word(unsigned offset) { return g_rom[offset]|g_rom[offset+1]<<8; }
/* Palettes $48-$4F belong to Subordinate Bomber (character 10). */
static unsigned palette_source(unsigned id) {
    if(!id || id>0x4f) return 0;
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
static void isolate_portrait_palettes(void) {
    /* Native allocation shares slots when two previews name the same palette.
     * Detach the five portrait references before assigning private slots. */
    uint8_t refs[8];memcpy(refs,g_ram+0x349,sizeof(refs));
    for(unsigned i=0;i<5;++i) {
        unsigned object=g_ram[0x62bf+i];
        if(object>=0x60) return;
        unsigned slot=g_ram[0x6df+object];
        if(slot>=8 || g_ram[0x2b18+slot]!=0 || !refs[slot]) return;
        --refs[slot];
    }
    for(unsigned slot=0;slot<5;++slot) if(refs[slot]) return; /* Another native owner. */
    for(unsigned i=0;i<5;++i) {
        g_ram[0x6df+g_ram[0x62bf+i]]=(uint8_t)i;
        refs[i]=1;g_ram[0x2b18+i]=0;
    }
    memcpy(g_ram+0x349,refs,sizeof(refs));
}
static unsigned last_character(void) { return SubordinateEnabled()?10:9; }
static void apply_color(unsigned player) {
    unsigned character=g_ram[0x625f+player];
    /* Honey/Kotetsu have one cartridge palette: no L/R variants. The Super
     * Bomberman 4 characters have five; the core draws them, this keeps the
     * seat's choice and the cell's color. */
    unsigned extra=ExtraCharacterColorCount(character);
    if(extra==1 || (extra==0 && HoneyKotetsuBaseChoice(character)!=character)) { colors[player]=0;g_ram[0x6264+player]=0;return; }
    if(extra>1) {
        colors[player]=(uint8_t)(colors[player]%extra);g_ram[0x6264+player]=colors[player];
        uint8_t *cells=PickerCellCharacters(),*cell_colors=PickerCellColors();
        for(unsigned cell=10;g_ram[0x62b5+player]==1 && cell<PickerCellCount();++cell)
            if(cells[cell]==character) cell_colors[cell]=colors[player];
        return;
    }
    if(character<1 || character>last_character()) return; /* The native random/exit cell. */
    unsigned color=colors[player];
    g_ram[0x6264+player]=(uint8_t)color;
    unsigned source=palette_source(g_rom[0xbb4e + character-1]+color);
    /* Each preview has its own slot even when the native allocator shared it. */
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
    uint8_t *cell_characters=PickerCellCharacters(),*cell_colors=PickerCellColors();
    for(unsigned cell=0;g_ram[0x62b5+player]==1 && cell<10;++cell) if(cell_characters[cell]==character) {
        cell_colors[cell]=(uint8_t)color;
        /* Subordinate Bomber has no grid strip; its overlay reads $62D3. */
        if(character==10) break;
        unsigned id=g_rom[0x2130b+(character-1)*7+color];
        unsigned destination=g_rom[0x21301+cell];
        /* Upper-row strips share a 16-color BG palette with the lower row.
         * Their entry zero is the lower figure's common color at index 8;
         * white variants may put an opaque color there. Preserve that entry. */
        unsigned skip=(destination&8)?1:0;
        unsigned grid_source=palette_source(id);
        if(grid_source) upload_palette(grid_source+skip*2,destination+skip,8-skip);
        break;
    }
}
void SelectionColorBeforeFrame(void) {
    if(!enabled) return;
    /* Final picker cleanup can restore stock CPU colors after the last A.
     * Keep the choices until the native stage initializer consumes them. */
    if(pending) for(unsigned i=0;i<5;++i) g_ram[0x6264+i]=colors[i];
    uint16_t pressed[5];
    for(unsigned slot=0;slot<5;++slot) {
        uint16_t pad=RtlGetPadState(slot);
        pressed[slot]=pad & ~previous[slot];previous[slot]=pad;
    }
    /* Native seat state: 0 = pending CPU, 1 = active, FF = confirmed.
     * $62AB maps each controller to its current seat, including CPU handoff.
     * $1038 only counts confirmations; it is not a per-seat bit mask. */
    if(live && picker()) for(unsigned slot=0;slot<5;++slot) {
        unsigned i=g_ram[0x62ab+slot];
        if(i>=5 || g_ram[0x62b5+i]!=1) continue;
        unsigned shoulders=pressed[slot] & 0xc00;
        unsigned count=ExtraCharacterColorCount(g_ram[0x625f+i]);
        if(!count) count=6;
        if(shoulders==0x800) colors[i]=(uint8_t)((colors[i]+1)%count); /* R */
        else if(shoulders==0x400) colors[i]=(uint8_t)((colors[i]+count-1)%count); /* L */
        g_ram[0x6264+i]=colors[i]; /* Includes simultaneous color + A. */
    }
}
void SelectionColorAfterFrame(void) {
    if(!enabled) return;
    if(!picker()) {
        if(live && g_ram[0x103a]>0 && g_ram[0x103a]<=5 && g_ram[0x1038]>=g_ram[0x103a]) pending=true;
        live=false;
        if(pending) {
            bool consumed=true;
            for(unsigned i=0;i<5;++i) {
                g_ram[0x6264+i]=colors[i];
                unsigned character=HoneyKotetsuBaseChoice(g_ram[0x625f+i]);
                if(character<1 || character>last_character() || g_ram[0x16c2+i]!=character-1 ||
                   g_ram[0x16da+i]!=g_rom[0xbb4e + character-1]+colors[i]) consumed=false;
            }
            if(consumed) pending=false;
        }
        return;
    }
    if(!live && picker_pc()==0xc68e68) {
        live=true;pending=false;
        for(unsigned i=0;i<5;++i) colors[i]=(uint8_t)i;
    }
    if(live) {
        isolate_portrait_palettes();
        for(unsigned i=0;i<5;++i) apply_color(i);
    }
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
void SelectionColorObserve(const Ppu *view,unsigned line,void *unused) {
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
bool SelectionColorEnabled(void) { return enabled; }
int SelectionColorSeat(unsigned seat) { return enabled && (live || pending) && seat<5 ? colors[seat] : -1; }
void SelectionColorDrawPpuFrame(void) {
    PtbrDrawPpuFrameObserved(enabled?SelectionColorObserve:NULL,NULL);
}
void SelectionColorSave(SaveLoadInfo *sli) {
    uint32_t magic=0x314c4353;uint8_t flag=(live?1:0)|(pending?2:0);
    sli->func(sli,&magic,sizeof(magic));sli->func(sli,&flag,1);
    sli->func(sli,colors,sizeof(colors));sli->func(sli,previous,sizeof(previous));
}
void SelectionColorLoad(SaveLoadInfo *sli,uint32_t version) {
    (void)version;SelectionColorReset();
    if(RtlStateBytesRemaining(sli)==0) return; /* v0.3.0 snapshots. */
    uint32_t magic=0;uint8_t flag=0;
    sli->func(sli,&magic,sizeof(magic));sli->func(sli,&flag,1);
    sli->func(sli,colors,sizeof(colors));sli->func(sli,previous,sizeof(previous));
    if(magic!=0x314c4353 || flag>3) { RtlStateStreamFail(sli);return; }
    for(unsigned i=0;i<5;++i) if(colors[i]>=6) { RtlStateStreamFail(sli);return; }
    live=(flag&1)!=0;pending=(flag&2)!=0;
}
