/* Authored menu glyphs and translation metadata. No ROM assets are embedded. */
#include "ptbr_mod.h"
#include "beam_frame_driver.h"
#include "mod_runtime.h"
#include "snes/ppu.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    unsigned layer, row, column, width, height, guard0, guard1, ink;
    const char *text;
} PtbrLabel;
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
static SavedWord s_saved[4096];
static unsigned s_saved_count;

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

static void render_text(uint8_t pixels[16][136], const PtbrLabel *label) {
    memset(pixels, 0, 16*136);
    const unsigned char *at = (const unsigned char *)label->text;
    unsigned x = 0, scale = label->height == 16 ? 2 : 1;
    while (*at) {
        unsigned c = next_character(&at), accent = 0;
        if (c == 0xC7) { c = 'C'; accent = 2; }
        else if (c == 0xD5) { c = 'O'; accent = 1; }
        else if (c == 0xC3) { c = 'A'; accent = 1; }
        else if (c == 0xC9) { c = 'E'; accent = 3; }
        unsigned top = accent == 1 || accent == 3 ? 1 : 0;
        if (c >= 'A' && c <= 'Z') {
            for (unsigned y = 0; y < 7; ++y)
                for (unsigned b = 0; b < 5; ++b)
                    if (kFont[c-'A'][y] & (1u << (4-b)))
                        for (unsigned s = 0; s < scale; ++s)
                            pixels[(y+top)*scale+s][x+b] = 1;
            if (accent == 1) { pixels[0][x+1]=1; pixels[0][x+3]=1; pixels[1 % scale][x+2]=1; }
            if (accent == 2) { pixels[label->height-1][x+2]=1; }
            if (accent == 3) { pixels[0][x+3]=1; }
        }
        x += 6;
    }
}

static void save_write(Ppu *ppu, unsigned address, uint16_t value) {
    address &= 0x7FFF;
    s_saved[s_saved_count++] = (SavedWord){(uint16_t)address, ppu->vram[address]};
    ppu->vram[address] = value;
}

static void apply_labels(Ppu *ppu) {
    unsigned next_tile[3] = {0x180,0,0x140};
    if ((ppu->bgmode & 7) != 1) return;
    for (size_t n = 0; n < sizeof(kLabels)/sizeof(kLabels[0]); ++n) {
        const PtbrLabel *label = &kLabels[n];
        unsigned map = PPU_bgTilemapAdr(ppu,label->layer), base = PPU_bgTileAdr(ppu,label->layer);
        if (map != (label->layer == 0 ? 0x3000u : 0x4000u)) continue;
        unsigned start = map + label->row*32 + label->column;
        if ((ppu->vram[start] & 0x3FF) != label->guard0 ||
            (ppu->vram[start+1] & 0x3FF) != label->guard1) continue;
        uint8_t pixels[16][136];
        render_text(pixels,label);
        unsigned rows=label->height/8, bpp=label->layer==0?4:2;
        for (unsigned ty=0; ty<rows; ++ty) for (unsigned tx=0; tx<label->width; ++tx) {
            unsigned tile=next_tile[label->layer]++;
            unsigned map_addr=start+ty*32+tx;
            uint16_t attributes=ppu->vram[map_addr] & 0xFC00;
            save_write(ppu,map_addr,(uint16_t)(attributes|tile));
            for (unsigned y=0; y<8; ++y) {
                uint8_t bits=0;
                for (unsigned x=0;x<8;++x) if(pixels[ty*8+y][tx*8+x]) bits|=(uint8_t)(1u<<(7-x));
                /* Preserve the title's white/yellow roles and normal-menu green. */
                save_write(ppu,base+tile*(bpp*4)+y,bpp==2?(uint16_t)(((label->ink&1)?bits:0)|((label->ink&2)?bits<<8:0)):0);
                if(bpp==4) save_write(ppu,base+tile*16+y+8,bits);
            }
        }
    }
    if(s_saved_count) ++ppu->vramWriteCount;
}

static void restore_labels(Ppu *ppu) {
    if(!s_saved_count) return;
    while(s_saved_count) { SavedWord w=s_saved[--s_saved_count]; ppu->vram[w.address]=w.original; }
    ++ppu->vramWriteCount;
}

static void observe_line(const Ppu *view, unsigned line, void *unused) {
    (void)unused;
    Ppu *ppu=(Ppu *)view;
    if(line==1) apply_labels(ppu);
    /* Restore before the driver replays pending guest writes at RenderEnd. */
    if(line==224) restore_labels(ppu);
}

void PtbrDrawPpuFrame(void) {
    if(s_enabled) snes_beam_frame_driver_draw_ppu_frame_observed(observe_line,NULL);
    else snes_beam_frame_driver_draw_ppu_frame();
}
