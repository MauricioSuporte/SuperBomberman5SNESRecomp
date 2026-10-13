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
void snes_beam_frame_driver_draw_ppu_frame(void) { ++draws; }
void snes_beam_frame_driver_draw_ppu_frame_observed(SnesBeamRasterObserver *fn, void *ctx) {
    ++draws;
    fn(&fixture,1,ctx);
    changed=memcmp(fixture.vram,original,sizeof(original)) != 0;
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
    reset(); PtbrDrawPpuFrame();
    if(draws != 3 || memcmp(original,fixture.vram,sizeof(original))) return 3;
    puts("PT-BR overlay: translated, guarded, restored and disabled paths passed.");
    return 0;
}
