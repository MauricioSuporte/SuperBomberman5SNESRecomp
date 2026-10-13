#include "mod_runtime.h"
#include "snes/ppu.h"
#include <stdio.h>
static SNESModActivationCallback activation, reset;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset=fn; return 1; }
int snes_mod_register_presentation_plugin(const char *id, SNESModActivationCallback fn) { (void)id; activation=fn; return 1; }
#include "../src/ptbr_mod.c"

static Ppu fixture;
static uint16_t original[0x8000];
static unsigned changed, draws;
static uint16_t normal_tile;
static uint16_t normal_pixels[16];
static int inspect_title;
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
    puts("PT-BR overlay: guards, restoration, disable, blink alignment, stable tiles, extended palettes, screen contexts and sprite heading passed.");
    return 0;
}
