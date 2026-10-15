/* Read-only Normal exit hints, using the authored PT-BR title-screen font. */
#include "portal_labels_mod.h"
#include "common_rtl.h"
#include "mod_runtime.h"
#include "menu_font.h"
#include "snes/ppu.h"
#include <stdio.h>
#include <string.h>

static bool enabled;
typedef struct { unsigned x, y, destination, variant; } Portal;
typedef struct { uint16_t address, value; } Saved;
static Saved saved[720];
static unsigned saved_count;
static bool drawn;
static uint8_t screen, window;

static void activate(void) { enabled = true; }
static void reset_plugin(void) { enabled = false; }
SNES_MOD_CONSTRUCTOR(register_portal_labels) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_presentation_plugin("superbomberman5.normal.portal-labels", activate);
}
bool PortalLabelsEnabled(void) { return enabled; }

/* Only C6 script pointers are accepted, and every read stays inside that bank. */
static const uint8_t *script(unsigned pc, unsigned size) {
    if (!g_rom || pc < 0xc60000 || pc > 0xc70000 - size) return NULL;
    return g_rom + (pc & 0x3fffff);
}
static unsigned pointer(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16; }
static unsigned destination(const uint8_t *root, unsigned *variant) {
    const uint8_t *p = script(pointer(root + 5), 16);
    if (!p) return 0;
    /* Some boss gates set/clear route flags before the native stage exit.
     * These commands affect boss variants, not the selected destination. */
    unsigned i = 0;
    *variant=0;
    while (i < 12 && (p[i] == 5 || p[i] == 6)) {
        if(p[i]==5 && p[i+1]>=7 && p[i+1]<=15) {
            unsigned flag=p[i+1];
            *variant=flag<=9?flag-6:flag<=11?flag-9:flag<=13?flag-11:flag-13;
        }
        i += 2;
    }
    return p[i] == 15 && p[i + 1] >= 13 && p[i + 1] < 116 ? p[i + 1] : 0;
}
static unsigned portals(Portal out[5]) {
    unsigned stage = g_ram[0x1031], count = 0;
    if (!enabled || !g_rom || stage < 13 || stage >= 116 ||
        stage != g_ram[0x102f] || (g_ram[0x1d1a] & 9) ||
        (g_ram[0x1d1e] & 6) || !(g_ram[0xaee]&0x7f)) return 0;
    const uint8_t *table = script(pointer(g_rom + 0x60000 + (stage - 1) * 3), 120);
    if (!table) return 0;
    for (unsigned i = 0; i < 40 && count < 5; ++i) {
        unsigned pc = pointer(table + i * 3);
        if (!pc) break;
        const uint8_t *root = script(pc, 13);
        if (!root || root[0] != 13 || root[4] != 11 || root[8] != 12 || root[12]) continue;
        const uint8_t *create = script(pointer(root + 1), 5);
        const uint8_t *touch = script(pointer(root + 9), 4);
        unsigned variant=0,dest = destination(root,&variant);
        if (!create || !touch || create[0] != 17 || create[4] || touch[0] != 18 || touch[3] ||
            create[2] != touch[1] || create[3] != touch[2] || !dest ||
            create[2] < 1 || create[2] > 13 || create[3] < 1 || create[3] > 11) continue;
        out[count++] = (Portal){ create[2], create[3], dest, variant };
    }
    return count;
}
static bool label(unsigned stage, unsigned variant, char text[8]) {
    unsigned zone, number;
    if (stage >= 13 && stage <= 102) {
        zone = stage < 73 ? (stage - 13) / 15 + 1 : 5;
        number = stage - (zone < 5 ? 13 + (zone - 1) * 15 : 73) + 1;
    } else if (stage >= 103 && stage <= 112) {
        static const uint8_t zones[10] = {1,2,2,3,3,4,4,4,5,5};
        zone = zones[stage - 103];
        if(variant) snprintf(text,8,"Z%u-%c%u",zone,'A'+stage-103,variant);
        else snprintf(text,8,"Z%u-%c",zone,'A'+stage-103);
        return true;
    } else if (stage >= 113 && stage <= 115) {
        snprintf(text,8,"Z%u-S",stage-112);return true;
    }
    else return false;
    snprintf(text, 8, "Z%u-%u", zone, number);
    return true;
}
static void write_word(Ppu *p, unsigned at, uint16_t value) {
    saved[saved_count++] = (Saved){(uint16_t)at, p->vram[at]}; p->vram[at] = value;
}
static unsigned glyph_width(char c) { return c=='-'?3:c>='A'&&c<='Z'?6:5; }
static unsigned glyph(char c, bool bits[24][40], unsigned left, unsigned top, unsigned height) {
    const uint8_t *source=Sb5MenuGlyph((unsigned char)c);
    unsigned width=glyph_width(c);
    if(!source)return width;
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<5;++x) {
        if(!(source[y*7/height]&(16u>>x)))continue;
        if(c=='-') { if(x>=1&&x<=3)bits[top+y][left+x-1]=true; }
        else {
            bits[top+y][left+x]=true;
            if(width==6)bits[top+y][left+x+1]=true;
        }
    }
    return width;
}
static bool gate_visible(const Ppu *p, unsigned object, int x, int y) {
    if(!g_ram[0x49f+object])return false;
    unsigned palette=g_ram[0x6df+object];
    if(palette>7)return false;
    for(unsigned s=0;s<128;++s) {
        unsigned pos=p->oam[s*2],attr=p->oam[s*2+1];
        unsigned sx=(pos&255)|(((p->highOam[s/4]>>((s%4)*2))&1u)<<8);
        int sy=(int)(pos>>8);
        if((sx==(unsigned)x||sx==(unsigned)x+8) && sy>=y-48 && sy<y &&
           ((attr>>9)&7)==palette) return true;
    }
    return false;
}
void PortalLabelsObserve(const struct Ppu *view, unsigned line, void *context) {
    (void)context; Ppu *p=(Ppu *)view;
    if (line==224 && drawn) {
        while(saved_count) { Saved w=saved[--saved_count];p->vram[w.address]=w.value; }
        p->screenEnabled[0]=screen;p->screenWindowed[0]=window;
        ++p->vramWriteCount;drawn=false;return;
    }
    if (line!=1 || drawn || !enabled || g_ram[0x1d1d] || PPU_mode(p)!=1 ||
        PPU_bgTileAdr(p,2)!=0x5000 || PPU_objTileAdr1(p)!=0x6000 ||
        PPU_bgTilemapWider(p,2)) return;
    Portal exits[5];unsigned count=portals(exits);
    if (!count) return;
    unsigned map=PPU_bgTilemapAdr(p,2);
    unsigned map_words=1024u << PPU_bgTilemapHigher(p,2);
    if(map+map_words>0x5000) return;
    bool used[512]={0};
    for(unsigned i=0;i<map_words;++i) { unsigned t=p->vram[map+i]&1023;if(t<512)used[t]=true; }
    unsigned first=0;
    for(unsigned t=256;t+75<=512;++t) {
        bool free=true;for(unsigned j=0;j<75;++j) if(used[t+j])free=false;
        if(free){first=t;break;}
    }
    if(!first)return;
    screen=p->screenEnabled[0];window=p->screenWindowed[0];saved_count=0;
    for(unsigned i=0;i<count;++i) {
        /* Native gate sprite set 6, animation archive $26. Require its live
         * object at this exit's location, so hints never reveal unopened exits. */
        int camera_x=(int16_t)(g_ram[0x1d3b]|g_ram[0x1d3c]<<8);
        int camera_y=(int16_t)(g_ram[0x1d3d]|g_ram[0x1d3e]<<8);
        int x=(int)exits[i].x*16-camera_x-8, y=(int)exits[i].y*16-camera_y+8;
        if(x<0||x>247||y<24||y>215)continue;
        bool visible=false;
        for(unsigned o=0;o<0x60;o+=2)
            if(g_ram[0x3d7+o]==0x38 && g_ram[0x4a0+o]==0x26 && g_ram[0x4ff+o]==6 &&
               g_ram[0x55f+o]==x && !g_ram[0x560+o] &&
               g_ram[0x5bf+o]==y && !g_ram[0x5c0+o] && gate_visible(p,o,x,y)) visible=true;
        if(!visible)continue;
        char text[8];if(!label(exits[i].destination,exits[i].variant,text))continue;
        unsigned width=0;for(const char *c=text;*c;++c)width+=glyph_width(*c)+1;
        --width;
        unsigned height=y+18<224?14:10;
        int left=x+8-(int)width/2,top=y+3;
        if(top+(int)height+1>224)top=223-(int)height;
        if(left<1 || left+(int)width+1>256 || top<24)continue;
        /* BG3 line zero is displayed at y=-1 by the native PPU. Include a
         * one-pixel outline and use up to three tile rows without clipping. */
        unsigned col=(unsigned)(left-1)/8,row=(unsigned)top/8;
        unsigned offset=(unsigned)(left-1)%8+1,offset_y=(unsigned)top%8+1;
        bool bits[24][40]={0};
        for(const char *c=text;*c;++c)offset+=glyph(*c,bits,offset,offset_y,height)+1;
        unsigned tiles=(offset+7)/8,rows=(offset_y+height+1+7)/8;
        for(unsigned ty=0;ty<rows;++ty)for(unsigned t=0;t<tiles;++t) {
            for(unsigned py=0;py<8;++py) {
                unsigned ink=0,outline=0;
                for(unsigned px=0;px<8;++px) {
                    unsigned bx=t*8+px,by=ty*8+py;
                    if(bits[by][bx])ink|=0x80>>px;
                    for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx) {
                        int nx=(int)bx+dx,ny=(int)by+dy;
                        if(nx>=0&&nx<40&&ny>=0&&ny<24&&bits[ny][nx])outline|=0x80>>px;
                    }
                }
                write_word(p,0x5000+(first+i*15+ty*5+t)*8+py,(uint16_t)(outline|(ink<<8)));
            }
            write_word(p,map+(row+ty)*32+col+t,(uint16_t)(0x2000+first+i*15+ty*5+t));
        }
    }
    if(saved_count) {
        drawn=true;p->screenEnabled[0]|=4;p->screenWindowed[0]&=(uint8_t)~4u;
        ++p->vramWriteCount;
    }
}
