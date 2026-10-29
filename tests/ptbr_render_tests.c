#include "mod_runtime.h"
#include "snes/ppu.h"
#include <stdio.h>
static SNESModActivationCallback activation, reset;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset=fn; return 1; }
int snes_mod_register_presentation_plugin(const char *id, SNESModActivationCallback fn) { (void)id; activation=fn; return 1; }
static bool gold_fixture;
bool GoldBattleHyper(void) { return gold_fixture; }
const uint8_t *g_rom;
uint8 g_ram[0x20000];
#include "../src/ptbr_mod.c"
static bool coop_fixture;
bool NormalCoopPlayerMenu(void) { return coop_fixture; }
static bool rush_title_fixture;
bool BossRushTitleMenu(void) { return rush_title_fixture; }
static unsigned coop_count_fixture=5;
unsigned NormalCoopPlayerCount(void) { return coop_count_fixture; }

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
static uint8_t font_cartridge[0x400000];
static void font_pixel(uint32_t tile,unsigned x,unsigned y,unsigned color) {
    for(unsigned plane=0;plane<4;++plane) {
        unsigned at=tile+(y&7)*2+(plane/2)*16+plane%2;
        font_cartridge[at]=(uint8_t)((font_cartridge[at]&~(1u<<(7-x)))|((color>>plane&1u)<<(7-x)));
    }
}
static int native_font_tests(void) {
    /* Synthetic cartridge pixels deliberately differ from the authored face.
     * Verify both source halves and proportional case without shipping art. */
    g_rom=font_cartridge;
    const PtbrNativeSmall *a=&kNativeSmall[0];
    for(unsigned y=0;y<16;++y) font_pixel(a->tiles[y/8],y%7,y,4);
    uint8_t glyph[16][24];
    if(native_small('A',glyph)!=7) return 60;
    for(unsigned y=0;y<16;++y) if(glyph[y][y%7]!=4) return 61;
    if(native_small('i',glyph)!=4 || native_small('m',glyph)!=8) return 62;
    /* Player counts use the title face even with a cartridge loaded: full
     * ten-row ink, bold stems, complete outline and empty top margin. */
    for(unsigned n=0;n<sizeof(kNativeWhite)/sizeof(kNativeWhite[0]);++n)
        if(kNativeWhite[n].character=='1' || kNativeWhite[n].character=='2') {
            const PtbrNativeWhite *d=&kNativeWhite[n];
            for(unsigned y=4;y<14;++y) {
                font_cartridge[d->tiles[y/8]+y%8*2]=0x10;
                font_cartridge[d->tiles[y/8]+y%8*2+1]=0x60;
            }
        }
    for(unsigned c='1';c<='5';++c) {
        if(native_white(c,glyph)!=8) return 80;
        if(c<='2' && (glyph[4][1]!=2 || glyph[13][2]!=2 || glyph[4][3]!=1)) return 84;
        unsigned rows=0,shadow=0;
        for(unsigned y=0;y<16;++y) {
            unsigned ink=0;
            for(unsigned x=0;x<24;++x) {
                ink+=glyph[y][x]==2;shadow+=glyph[y][x]==1;
                if(y<3 && glyph[y][x]) return 81;
            }
            if(ink) {if(ink<2 || y<4 || y>13) return 82;++rows;}
        }
        if(rows!=10 || !shadow) return 83;
    }

    char row_text[32];s_enabled=false;
    coop_menu_text(1,row_text,sizeof(row_text));if(strcmp(row_text,"  1 PLAYER")) return 87;
    coop_menu_text(5,row_text,sizeof(row_text));if(strcmp(row_text,"  5 PLAYERS")) return 88;
    const PtbrNativeWhite *title_y=NULL,*title_t=NULL;
    for(unsigned n=0;n<sizeof(kNativeWhite)/sizeof(kNativeWhite[0]);++n) {
        if(kNativeWhite[n].character=='Y') title_y=&kNativeWhite[n];
        if(kNativeWhite[n].character=='T') title_t=&kNativeWhite[n];
    }
    if(!title_y || !title_t) return 89;
    for(unsigned y=0;y<16;++y) {
        font_cartridge[title_y->tiles[y/8]+y%8*2+1]=0x60;
        font_cartridge[title_t->tiles[y/8]+y%8*2]=0x01;
        font_cartridge[title_t->tiles[y/8]+y%8*2+1]=0x30;
    }
    native_white('Y',glyph);
    for(unsigned y=0;y<16;++y) if(glyph[y][1]!=2 || glyph[y][2]!=2) return 90;
    native_white('T',glyph);
    for(unsigned y=6;y<15;++y) if(glyph[y][7] || glyph[y][2]!=2) return 91;
    if(glyph[4][7]!=1) return 92; /* Crossbar outline remains native. */
    const PtbrNativeBlue *b=NULL;
    for(unsigned n=0;n<sizeof(kNativeBlue)/sizeof(kNativeBlue[0]);++n)
        if(kNativeBlue[n].character=='A') b=&kNativeBlue[n];
    if(!b || b->width>20) return 63;
    /* A white bevel enclosed by native outline survives; the banner's white
     * stripe connected to gray background is transparent in the glyph. */
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<b->width+4;++x) {
        unsigned sx=x+b->offset,color=y<2?10:11;
        if(x==1 || x==5 || y==3 || y==8) color=6;
        if(x>=2&&x<=4&&y>=4&&y<=7) color=11;
        if(x==3&&y==9) color=4;
        font_pixel(b->tiles[(y/8)*4+sx/8],sx%8,y,color);
    }
    if(native_blue('A',glyph)!=b->width+3 || glyph[6][2]!=11 || glyph[6][b->width+2]!=0) return 64;
    uint8_t canvas[32][224];PtbrLabel label={.width=4,.height=16,.style=4,.text="A"};
    render_text(canvas,&label);bool white=false;
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<32;++x) if(canvas[y][x]==7) white=true;
    if(!white) return 65;
    for(unsigned n=0;n<sizeof(kNativeSmall)/sizeof(kNativeSmall[0]);++n)
        for(unsigned y=0;y<2;++y) if(kNativeSmall[n].tiles[y]>sizeof(font_cartridge)-32) return 66;
    for(unsigned n=0;n<sizeof(kNativeBlue)/sizeof(kNativeBlue[0]);++n)
        for(unsigned y=0;y<8;++y) if(kNativeBlue[n].tiles[y]>sizeof(font_cartridge)-32) return 67;
    /* Regression: a bright H crossbar touches both sides of the ink crop.
     * The surrounding native outline must prevent background flood leakage. */
    const PtbrNativeBlue *h=NULL;
    for(unsigned n=0;n<sizeof(kNativeBlue)/sizeof(kNativeBlue[0]);++n)
        if(kNativeBlue[n].character=='H') h=&kNativeBlue[n];
    if(!h) return 71;
    memset(font_cartridge,0,sizeof(font_cartridge));
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<h->width+4;++x) {
        unsigned color=(y==6||y==7)?11:10,sx=x+h->offset;
        if((x==1||x==h->width+2)&&y>=5&&y<=8) color=6;
        if((y==5||y==8)&&x>=1&&x<=h->width+2) color=6;
        if(y==0&&x>=2&&x<h->width+2) color=3;
        font_pixel(h->tiles[(y/8)*4+sx/8],sx%8,y,color);
    }
    native_blue('H',glyph);
    for(unsigned x=1;x<=h->width;++x) if(glyph[7][x]!=11 || glyph[8][x]!=11) return 72;
    if(glyph[0][h->width/2]!=6 || glyph[1][h->width/2]!=3) return 73;
    const PtbrNativeWhite *white_a=&kNativeWhite[0];
    for(unsigned y=4;y<14;++y) font_cartridge[white_a->tiles[y/8]+y%8*2+1]=0x40;
    native_white('A',glyph);
    for(unsigned y=0;y<3;++y) for(unsigned x=0;x<8;++x) if(glyph[y][x]) return 68;
    if(glyph[4][1]!=2 || glyph[13][1]!=2) return 69;
    label=(PtbrLabel){.width=1,.height=16,.style=1,.ink=2,.text="A"};
    render_text(canvas,&label);if(canvas[4][1]!=2 || canvas[13][1]!=2) return 70;
    g_rom=NULL;return 0;
}
/* Optional local cartridge check. Pixels are read only from the supplied ROM;
 * no game data is embedded in tests or published artifacts. */
static int cartridge_font_tests(const char *path) {
    FILE *file=fopen(path,"rb");if(!file) return 74;
    size_t size=fread(font_cartridge,1,sizeof(font_cartridge),file);
    int trailing=fgetc(file);fclose(file);
    if(size!=sizeof(font_cartridge) || trailing!=EOF) return 75;
    g_rom=font_cartridge;native_font_rom=NULL;
    for(const char *c="EFH";*c;++c) {
        uint8_t glyph[16][24];unsigned width=native_blue(*c,glyph);
        for(unsigned y=7;y<=8;++y) {
            unsigned bevel=0;for(unsigned x=0;x<width;++x) bevel+=glyph[y][x]==11;
            if(bevel<7) return 76;
        }
        if(glyph[0][1]!=6 || glyph[1][1]!=3) return 77;
    }
    uint8_t canvas[32][224];
    PtbrLabel line={.width=6,.height=16,.style=4,.text="EFH"};render_text(canvas,&line);
    unsigned top=0,middle=0;
    for(unsigned x=0;x<48;++x) {
        top+=canvas[0][x]==5;
        middle+=canvas[7][x]==7;middle+=canvas[8][x]==7;
    }
    if(top<25 || middle<45) return 78;
    g_rom=NULL;
    puts("Cartridge E/F/H crossbars and upper outlines passed.");return 0;
}
int main(int argc,char **argv) {
    int fonts=native_font_tests();if(fonts) return fonts;
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
    /* A stage arena holding a menu guard pair (ZONE2's boss arena showed
     * "Continuar") keeps its tiles while gameplay runs. */
    memset(g_ram+0x17fa,0xff,5);g_ram[0x1031]=0x68;g_ram[0x0aee]=1;
    memcpy(original,fixture.vram,sizeof(original));apply_labels(&fixture);
    if(s_saved_count || memcmp(original,fixture.vram,sizeof(original))) return 12;
    g_ram[0x0aee]=0;g_ram[0x17fa]=0;apply_labels(&fixture);if(s_saved_count) return 13; /* intro, P1 on the field */
    g_ram[0x17fa]=0xff;apply_labels(&fixture);if(!s_saved_count) return 14;              /* players gone */
    restore_labels(&fixture);g_ram[0x1031]=3;g_ram[0x17fa]=0;
    apply_labels(&fixture);if(!s_saved_count) return 15;                                 /* file menu */
    restore_labels(&fixture);g_ram[0x1031]=0;
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
        if(fixture.screenWindowed[0]&4 || fixture.oam[0]!=(175u<<8|(language?68u:76u)) ||
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
    if(fixture.oam[0]!=(171u<<8|68u)) return 17;
    observe_line(&fixture,224,NULL);if(fixture.oam[0]!=0x8b40) return 18;
    fixture.oam[0]=0x7a40;observe_line(&fixture,1,NULL); /* 1P row bounce */
    if(fixture.oam[0]!=(170u<<8|68u)) return 19;
    observe_line(&fixture,224,NULL);
    /* An idle cursor stays on the selected row whatever stock row Up/Down left. */
    fixture.oam[0]=0x7f40;fixture.oam[1]=0x2be8;observe_line(&fixture,1,NULL);
    if(fixture.oam[0]!=(175u<<8|68u)) return 20;
    observe_line(&fixture,224,NULL);
    /* Every selected translated row keeps an eight-pixel cursor/text gap;
     * the singular row is shorter. English uses PLAYER/PLAYERS with the same gap. */
    for(unsigned language=0;language<2;++language) for(unsigned seat=1;seat<=5;++seat) {
        s_enabled=language!=0;coop_count_fixture=seat;
        fixture.oam[0]=0x7f40;fixture.oam[1]=0x2be8;
        observe_line(&fixture,1,NULL);
        unsigned expected_x=language?(seat==1?76:68):(seat==1?80:76);
        if(fixture.oam[0]!=((12+seat*2)*8-1)*256+expected_x) return 85;
        observe_line(&fixture,224,NULL);
        if(fixture.oam[0]!=0x7f40 || memcmp(original,fixture.vram,sizeof(original))) return 86;
    }
    coop_count_fixture=5;s_enabled=true;
    coop_fixture=false;
    /* Boss Rush title: four entries on rows 16-22 under the heading on row 14,
     * the copyright on rows 25-26 untouched; English reuses the screen's own
     * tile words, Portuguese renders the rows; render only, exact restore. */
    {
        static const uint16_t rows[11][32]={
            [0]={[8]=0x2102,0x2103,0x2104,0x2103,0x2105,0x2104,0x2106,0x2107,0x2102,0x2108,0x2105,0x2102,0x2103,0x2108,0x2108,0x2109,0x210a},
            [1]={[8]=0x210b,0x210c,0x210d,0x210e,0x2105,0x210d,0x210f,0x2110,0x2111,0x2112,0x2105,0x2113,0x210c,0x2112,0x2112,0x210c,0x2114},
            [2]={[11]=0x2115,0x2116,0x2117,0x2118,0x2119,0x211a,0x2105,0x211b,0x2119,0x2118,0x211c,0x2105},
            [3]={[11]=0x211d,0x211e,0x211f,0x2120,0x2121,0x2122,0x2105,0x2123,0x2121,0x2120,0x2124,0x2105},
            [4]={[11]=0x2117,0x2119,0x2125,0x2125,0x211a,0x211c,0x2105,0x211b,0x2119,0x2118,0x211c,0x2105},
            [5]={[11]=0x2126,0x2121,0x2127,0x2127,0x2122,0x2124,0x2105,0x2123,0x2121,0x2120,0x2124,0x2105},
            [6]={[11]=0x2116,0x2117,0x2125,0x2128,0x2116,0x2115,0x2129,0x2105,0x2105,0x2105,0x2105,0x2105},
            [7]={[11]=0x211e,0x212a,0x2127,0x2127,0x211e,0x211d,0x212b,0x2105,0x2105,0x2105,0x2105,0x2105},
            [9]={[8]=0x212c,0x212d,0x212e,0x2116,0x2116,0x212f,0x2105,0x2130,0x2130,0x2117,0x2129,0x2116,0x2115},
            [10]={[8]=0x2132,0x2133,0x2127,0x2134,0x2134,0x2135,0x2105,0x2136,0x211e,0x2137,0x212b,0x211e,0x211d},
        };
        memset(&fixture,0,sizeof(fixture));fixture.bgmode=9;fixture.bgXsc[2]=0x41;fixture.bgTileAdr=0x500;
        fixture.screenWindowed[0]=23;fixture.oam[0]=(uint16_t)(175u<<8|64u);fixture.oam[1]=0x2be8;
        for(unsigned r=0;r<11;++r) for(unsigned c=0;c<32;++c) fixture.vram[0x4000+(16+r)*32+c]=rows[r][c]?rows[r][c]:0x2000;
        memcpy(original,fixture.vram,sizeof(original));
        rush_title_fixture=true;
        static const uint16_t boss[2][9]={{0x2117,0x2116,0x2129,0x2129,0x2105,0x2117,0x2130,0x2129,0x2130},
                                          {0x2126,0x211e,0x212b,0x212b,0x2105,0x211f,0x211e,0x212b,0x2136}};
        for(unsigned language=0;language<2;++language) for(unsigned hidden=0;hidden<2;++hidden) {
            s_enabled=language!=0;fixture.windowsel=hidden?0x800:0;
            fixture.renderFlags=kPpuRenderFlags_NewRenderer;
            observe_line(&fixture,1,NULL);
            if(fixture.renderFlags&kPpuRenderFlags_NewRenderer) return 90;
            if(fixture.screenWindowed[0]&4 || fixture.oam[0]!=(uint16_t)(159u<<8|64u)) return 91;
            const uint16_t *v=fixture.vram+0x4000;
            if(!language) {
                if((v[14*32+8]!=(hidden?0x2000:0x2102)) || v[16*32+11]!=0x2115 || v[18*32+11]!=0x2117 ||
                   v[22*32+11]!=0x2116 || v[17*32+11]!=0x211d) return 92;
                for(unsigned h=0;h<2;++h) for(unsigned i=0;i<9;++i) if(v[(20+h)*32+11+i]!=boss[h][i]) return 93;
            } else {
                if((v[16*32+11]&0x3ff)!=1+2*18 || (v[22*32+11]&0x3ff)!=1+8*18 || (v[14*32+8]&0x3ff)!=(hidden?0:1)) return 94;
            }
            for(unsigned r=24;r<27;++r) if(memcmp(v+r*32,original+0x4000+r*32,64)) return 95;
            observe_line(&fixture,224,NULL);
            if(fixture.renderFlags!=kPpuRenderFlags_NewRenderer || memcmp(original,fixture.vram,sizeof(original)) ||
               fixture.screenWindowed[0]!=23 || fixture.oam[0]!=(uint16_t)(175u<<8|64u)) return 96;
        }
        /* Another screen (guards differ): untouched. */
        s_enabled=false;fixture.vram[0x4000+18*32+11]=0x2000;memcpy(original,fixture.vram,sizeof(original));
        observe_line(&fixture,1,NULL);
        if(memcmp(original,fixture.vram,sizeof(original)) || fixture.oam[0]!=(uint16_t)(175u<<8|64u)) return 97;
        observe_line(&fixture,224,NULL);
        rush_title_fixture=false;s_enabled=true;
    }
    int completion=completion_tests();if(completion) return completion;
    gold_fixture=true;
    for(unsigned language=0;language<2;++language) {
        s_enabled=language!=0;
        memset(&fixture,0,sizeof(fixture));fixture.bgmode=9;fixture.bgXsc[0]=0x31;
        fixture.vram[0x3000+6*32+8]=308;
        fixture.vram[0x3000+18*32+22]=342;fixture.vram[0x3000+18*32+23]=468;
        memcpy(original,fixture.vram,sizeof(original));apply_labels(&fixture);
        if(!s_saved_count) return 40;
        restore_labels(&fixture);if(memcmp(original,fixture.vram,sizeof(original))) return 41;
        if(strcmp(PtbrPauseLabel(1),language?"NÃO":"NO")) return 42;
    }
    gold_fixture=false;s_enabled=true;
    for(unsigned confirmation=0;confirmation<3;++confirmation) for(unsigned arena=1;arena<=13;++arena) {
        memset(&fixture,0,sizeof(fixture));fixture.bgmode=9;fixture.bgXsc[0]=0x31;
        fixture.obsel=3;fixture.screenWindowed[0]=23;
        for(unsigned i=0;i<128;++i) fixture.oam[i*2]=224u<<8;
        for(unsigned i=0;i<(confirmation==2?0:9);++i) {
            fixture.oam[i*2]=19u*256+56+i*16;
            fixture.oam[i*2+1]=(uint16_t)(0x2000|(confirmation<<9)|object_label_tile(&kObjectLabels[confirmation],i));
            fixture.highOam[i/4]|=2u<<((i%4)*2);
        }
        unsigned stage_slot=confirmation==2?0:9+confirmation;
        for(unsigned i=0;i<3;++i) {
            unsigned slot=stage_slot+i;
            fixture.oam[slot*2]=171u*256+90+i*16;fixture.oam[slot*2+1]=0x24c4+i*2;
            fixture.highOam[slot/4]|=2u<<((slot%4)*2);
        }
        unsigned n=10+2*(arena-1),digit=0xc0+(n/16)*32+n%16;
        unsigned digit_slot=stage_slot+3,name_slot=digit_slot+1;
        fixture.oam[digit_slot*2]=171u*256+138;fixture.oam[digit_slot*2+1]=0x2000|digit;
        fixture.highOam[digit_slot/4]|=2u<<((digit_slot%4)*2);
        fixture.oam[name_slot*2]=195u*256+92;fixture.oam[name_slot*2+1]=0x222e;
        /* Synthetic native font; no cartridge glyphs in the test. */
        for(unsigned tile=2;tile<128;++tile) for(unsigned y=0;y<8;++y)
            fixture.vram[0x6000+tile*16+y+8]=0x00ff;
        uint16_t before_oam[256];uint8_t before_high[32];
        memcpy(before_oam,fixture.oam,sizeof(before_oam));memcpy(before_high,fixture.highOam,sizeof(before_high));
        memcpy(original,fixture.vram,sizeof(original));apply_labels(&fixture);
        if(!arena_drawn || arena_last!=arena || fixture.screenWindowed[0]!=23 || fixture.oam[digit_slot*2+1]!=before_oam[digit_slot*2+1]) return 43;
        for(unsigned slot=13;slot<128;++slot) if((fixture.oam[slot*2]>>8)<224 && fixture.oam[slot*2+1]==0x222e) return 45;
        if(!memcmp(original,fixture.vram,sizeof(original))) return 46;
        restore_labels(&fixture);
        if(memcmp(original,fixture.vram,sizeof(original)) || memcmp(before_oam,fixture.oam,sizeof(before_oam)) || memcmp(before_high,fixture.highOam,sizeof(before_high))) return 44;
        /* A transient hidden digit must retain Portuguese, with no live RAM
         * or task-PC dependency and no native glyph escaping the overlay. */
        fixture.oam[digit_slot*2]=224u<<8;apply_labels(&fixture);
        if(!arena_drawn || arena_last!=arena) return 47;
        restore_labels(&fixture);
        s_enabled=false;apply_labels(&fixture);if(arena_drawn || s_saved_count) return 48;s_enabled=true;
        fixture.oam[1]^=0x200;apply_labels(&fixture);
        if(arena_drawn || s_saved_count) return 79; /* wrong heading palette */
    }
    if(argc==2) {int cartridge=cartridge_font_tests(argv[1]);if(cartridge) return cartridge;}
    puts("PT-BR overlay: guards, restoration, disable, blink alignment, stable tiles, extended palettes, screen contexts and sprite heading passed.");
    return 0;
}
