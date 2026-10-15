#include "mod_runtime.h"
#include "snes/ppu.h"
#include <stdio.h>
static SNESModActivationCallback activation, reset;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset=fn; return 1; }
int snes_mod_register_presentation_plugin(const char *id, SNESModActivationCallback fn) { (void)id; activation=fn; return 1; }
#include "../src/ptbr_mod.c"
static bool coop_fixture;
bool NormalCoopPlayerMenu(void) { return coop_fixture; }
unsigned NormalCoopPlayerCount(void) { return 5; }

static Ppu fixture;
static uint16_t original[0x8000];
static unsigned changed, draws;
static uint16_t normal_tile;
static uint16_t normal_pixels[16];
static int inspect_title;
static int completion_tests(void) {
    for(unsigned rank=0;rank<33;++rank) for(unsigned page=0;page<2;++page) {
        memset(&fixture,0,sizeof(fixture));fixture.bgmode=9;fixture.bgXsc[0]=0x31;
        unsigned row=page?8:19;
        if(page) {fixture.vram[0x30ae]=0x21e8;fixture.vram[0x32c6]=0x21e9;}
        else {fixture.vram[0x3144]=0x2107;fixture.vram[0x3145]=0x2108;}
        for(unsigned x=0;x<24;++x) fixture.vram[0x3000+row*32+4+x]=0x2000|kEndings[rank].guard[x];
        unsigned digits=page?0x32d3:0x31a4;
        for(unsigned x=0;x<7;++x) fixture.vram[digits+x]=(uint16_t)(0x20b0+x);
        memcpy(original,fixture.vram,sizeof(original));
        apply_endings(&fixture);
        if(!s_saved_count || memcmp(original+digits,fixture.vram+digits,14)) return 30;
        if(!page && (fixture.vram[0x3309]!=0x22fe || fixture.vram[0x2fe0]!=0x00ff)) return 37;
        restore_labels(&fixture);
        if(memcmp(original,fixture.vram,sizeof(original))) return 31;
        fixture.vram[0x3000+row*32+4]^=1;
        apply_endings(&fixture);
        if(s_saved_count) return 32;
    }
    memset(&fixture,0,sizeof(fixture));fixture.bgmode=15;fixture.bgXsc[0]=0x31;
    for(unsigned row=0;row<2;++row) for(unsigned col=0;col<4;++col) {
        unsigned slot=row*4+col;
        fixture.oam[slot*2]=(uint16_t)((23+row*16)*256+62+col*16);
        fixture.oam[slot*2+1]=(uint16_t)(0x2400+row*32+col*2);
        fixture.highOam[slot/4]|=(uint8_t)(2u<<((slot%4)*2));
    }
    uint16_t saved_oam[256];memcpy(saved_oam,fixture.oam,sizeof(saved_oam));
    memcpy(original,fixture.vram,sizeof(original));apply_defeat(&fixture);
    if(!s_saved_count || memcmp(saved_oam,fixture.oam,sizeof(saved_oam))) return 33;
    restore_labels(&fixture);if(memcmp(original,fixture.vram,sizeof(original))) return 34;
    /* A shared visible tile or one misplaced piece rejects the entire strip. */
    fixture.oam[16]=0x1000;fixture.oam[17]=0x2400;
    apply_defeat(&fixture);if(s_saved_count) return 35;
    fixture.oam[17]=0;fixture.oam[0]++;
    apply_defeat(&fixture);if(s_saved_count) return 36;
    /* HUD variants must translate independently on either side, retaining
     * blink-off frames and restoring the shared BG3 character memory. */
    for(unsigned n=0;n<sizeof(kLabels)/sizeof(kLabels[0]);++n) {
        const PtbrLabel *label=&kLabels[n];
        if(label->layer!=2 || label->row!=0) continue;
        memset(&fixture,0,sizeof(fixture));fixture.bgmode=9;fixture.bgXsc[2]=0x42;fixture.bgTileAdr=0x500;
        unsigned address=0x4000+label->column;
        fixture.vram[address]=0x2000|label->guard0;fixture.vram[address+1]=0x2000|label->guard1;
        fixture.vram[0x4020+label->column]=0x2000|label->context_tile;
        memcpy(original,fixture.vram,sizeof(original));apply_labels(&fixture);
        if(!s_saved_count) return 38;
        restore_labels(&fixture);if(memcmp(original,fixture.vram,sizeof(original))) return 39;
        fixture.vram[address]=0;apply_labels(&fixture);if(s_saved_count) return 40;
    }
    return 0;
}
void snes_beam_frame_driver_draw_ppu_frame(void) { ++draws; }
void snes_beam_frame_driver_draw_ppu_frame_observed(SnesBeamRasterObserver *fn, void *ctx) {
    ++draws;
    fn(&fixture,1,ctx);
    changed=memcmp(fixture.vram,original,sizeof(original)) != 0;
    if(inspect_title) {
        normal_tile=fixture.vram[0x424B]&0x3FF;
        memcpy(normal_pixels,fixture.vram+0x5000+normal_tile*8,16);
        memcpy(normal_pixels+8,fixture.vram+0x5000+(normal_tile+12)*8,16);
    }
    fn(&fixture,224,ctx);
}
int main(void) {
    fixture.bgmode=9; fixture.bgXsc[0]=0x31;
    fixture.vram[0x314C]=0x313A; fixture.vram[0x314D]=0x313B;
    memcpy(original,fixture.vram,sizeof(original));
    activation(); PtbrDrawPpuFrame();
    if(!changed || memcmp(original,fixture.vram,sizeof(original))) {
        fprintf(stderr,"Translation must draw different graphics and restore every guest VRAM word.\n"); return 1;
    }
    fixture.vram[0x314C]=0; memcpy(original,fixture.vram,sizeof(original));
    PtbrDrawPpuFrame();
    if(changed || memcmp(original,fixture.vram,sizeof(original))) return 2;
    /* The title's first three rows must stay empty: its HDMA blink window
     * masks them together with Start. Test the independent white text and
     * temporary tile slots whether Start's surface is present or absent. */
    memset(&fixture,0,sizeof(fixture));
    fixture.bgmode=9; fixture.bgXsc[2]=0x41; fixture.bgTileAdr=0x500;
    fixture.vram[0x4208]=0x2102; fixture.vram[0x4209]=0x2103;
    fixture.vram[0x424B]=0x2115; fixture.vram[0x424C]=0x2116;
    inspect_title=1;
    memcpy(original,fixture.vram,sizeof(original)); PtbrDrawPpuFrame();
    uint16_t first_tile=normal_tile, first_pixels[16];
    memcpy(first_pixels,normal_pixels,sizeof(first_pixels));
    if(normal_pixels[0] || normal_pixels[1] || normal_pixels[2] || !normal_pixels[4]) return 4;
    fixture.vram[0x4208]=0; fixture.vram[0x4209]=0;
    memcpy(original,fixture.vram,sizeof(original)); PtbrDrawPpuFrame();
    if(first_tile!=normal_tile || memcmp(first_pixels,normal_pixels,sizeof(first_pixels)) ||
       memcmp(original,fixture.vram,sizeof(original))) return 5;
    inspect_title=0;
    /* A longer replacement must keep its source palette even when it extends
     * into blank tiles with a different palette. */
    memset(&fixture,0,sizeof(fixture)); fixture.bgmode=9; fixture.bgXsc[0]=0x31;
    fixture.vram[0x314C]=0x313A; fixture.vram[0x314D]=0x313B;
    apply_labels(&fixture);
    for(unsigned x=12;x<22;++x) if((fixture.vram[0x3140+x]&0x3C00)!=0x3000) return 6;
    restore_labels(&fixture);
    /* Screen context is checked before replacements: translating Player 1
     * must not make Player 2 or the Human value fail their common guard. */
    memset(&fixture,0,sizeof(fixture)); fixture.bgmode=9; fixture.bgXsc[0]=0x31;
    fixture.vram[0x3046]=0x2904; fixture.vram[0x3047]=0x2905;
    fixture.vram[0x30CA]=0x3134; fixture.vram[0x30CB]=0x3135;
    fixture.vram[0x30D4]=0x3165; fixture.vram[0x30D5]=0x3166;
    fixture.vram[0x314A]=0x3144; fixture.vram[0x314B]=0x3145;
    memcpy(original,fixture.vram,sizeof(original)); apply_labels(&fixture);
    if(fixture.vram[0x3046]==original[0x3046] || fixture.vram[0x30D4]==original[0x30D4] ||
       fixture.vram[0x314A]==original[0x314A]) return 7;
    restore_labels(&fixture);
    if(memcmp(original,fixture.vram,sizeof(original))) return 8;
    /* A reused banner tile pair on an unrelated screen must stay original. */
    fixture.vram[0x30CA]=0; fixture.vram[0x30CB]=0;
    memcpy(original,fixture.vram,sizeof(original)); apply_labels(&fixture);
    if(fixture.vram[0x3046]!=original[0x3046]) return 9;
    restore_labels(&fixture);
    /* The sprite heading changes only its unique graphics; OAM is untouched,
     * and all VRAM words return to the exact guest values afterwards. */
    memset(&fixture,0,sizeof(fixture)); fixture.bgmode=9; fixture.bgXsc[0]=0x31; fixture.obsel=3;
    fixture.vram[0x30C9]=0x3092;
    for(unsigned i=0;i<9;++i) {
        fixture.oam[i*2]=(19u<<8)|(56+i*16);
        fixture.oam[i*2+1]=0x2080+i*2+(i/8)*16;
        fixture.highOam[i/4]|=(uint8_t)(2u<<((i%4)*2));
    }
    uint16_t oam_before[256]; memcpy(oam_before,fixture.oam,sizeof(oam_before));
    memcpy(original,fixture.vram,sizeof(original)); apply_labels(&fixture);
    if(!memcmp(original,fixture.vram,sizeof(original)) || memcmp(oam_before,fixture.oam,sizeof(oam_before))) return 10;
    restore_labels(&fixture);
    if(memcmp(original,fixture.vram,sizeof(original))) return 11;
    memcpy(original,fixture.vram,sizeof(original));
    reset(); PtbrDrawPpuFrame();
    if(draws != 5 || memcmp(original,fixture.vram,sizeof(original))) return 3;
    /* Five player rows fit between Start and the unchanged copyright. The
     * title's blink window and cursor edits are render-only, in either language. */
    memset(&fixture,0,sizeof(fixture));fixture.bgmode=9;fixture.bgXsc[2]=0x41;fixture.bgTileAdr=0x500;
    fixture.screenWindowed[0]=23;fixture.oam[0]=0x8f40;fixture.oam[1]=0x2be8;
    fixture.vram[0x4348]=0x2132;fixture.vram[0x5000+0x132*8]=0x7e5a;fixture.vram[0x6100]=0xabcd;
    memcpy(original,fixture.vram,sizeof(original));
    coop_fixture=true;
    for(unsigned language=0;language<2;++language) {
        if(language) activation();
        fixture.renderFlags=kPpuRenderFlags_NewRenderer;
        observe_line(&fixture,1,NULL);
        if(fixture.renderFlags&kPpuRenderFlags_NewRenderer) return 15;
        if(fixture.screenWindowed[0]&4 || fixture.oam[0]!=(175u<<8|48u) ||
           fixture.vram[0x4348]!=0x2132 || !s_saved_count ||
           fixture.vram[0x5000+0x132*8]!=0x7e5a ||
           memcmp(fixture.vram+0x6000,original+0x6000,0x2000*sizeof(uint16_t))) return 12;
        for(unsigned seat=1;seat<=5;++seat) {
            unsigned at=0x4000+(12+seat*2)*32+6;
            if((fixture.vram[at]&0x3ff)!=seat*40) return 13;
        }
        observe_line(&fixture,224,NULL);
        if(fixture.renderFlags!=kPpuRenderFlags_NewRenderer) return 16;
        if(memcmp(original,fixture.vram,sizeof(original)) || fixture.screenWindowed[0]!=23 || fixture.oam[0]!=0x8f40) return 14;
    }
    /* The confirmation bounce (tiles $1EA-$1EE, y $8A-$90) keeps its offset
     * on the selected row instead of jumping back to the stock 2P row. */
    fixture.oam[0]=0x8b40;fixture.oam[1]=0x2bee;observe_line(&fixture,1,NULL);
    if(fixture.oam[0]!=(171u<<8|48u)) return 17;
    observe_line(&fixture,224,NULL);if(fixture.oam[0]!=0x8b40) return 18;
    fixture.oam[0]=0x7a40;observe_line(&fixture,1,NULL); /* 1P row bounce */
    if(fixture.oam[0]!=(170u<<8|48u)) return 19;
    observe_line(&fixture,224,NULL);
    /* An idle cursor stays on the selected row whatever stock row Up/Down left. */
    fixture.oam[0]=0x7f40;fixture.oam[1]=0x2be8;observe_line(&fixture,1,NULL);
    if(fixture.oam[0]!=(175u<<8|48u)) return 20;
    observe_line(&fixture,224,NULL);
    coop_fixture=false;
    int completion=completion_tests();if(completion) return completion;
    puts("PT-BR overlay: guards, restoration, disable, blink alignment, stable tiles, extended palettes, screen contexts and sprite heading passed.");
    return 0;
}
