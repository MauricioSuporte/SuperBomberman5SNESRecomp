/* Native tests for the Super Bomberman 4 image reader: digest check, copier
 * header, frame decoding and palettes on a synthetic image (no game data). */
#include "../src/sb4_rom.c"
#include <stdio.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
/* The digest is checked by sha256_compute; tests replace it so a synthetic
 * image can be accepted. */
static int accept_all;
void sha256_compute(const uint8_t *data,size_t len,uint8_t out[32]) {
    (void)data;(void)len;
    if(accept_all) memcpy(out,kSb4Sha256,32);
    else memset(out,0,32);
}
static uint8_t image_bytes[SB4_SIZE];
int main(void) {
    static uint8_t with_header[SB4_SIZE+512];
    CHECK(!Sb4RomAccept(image_bytes,SB4_SIZE)); /* wrong digest */
    accept_all=1;
    CHECK(Sb4RomAccept(image_bytes,SB4_SIZE) && Sb4RomAccept(with_header,SB4_SIZE+512));
    CHECK(!Sb4RomAccept(image_bytes,SB4_SIZE-1));
    /* Frame 5 of Jet (character 2, sheet $CC:0000): tile row 0 of the frame
     * at $0C0000 + 1*$800 + 1*$80; first tile, row 0, plane 0 = $80 -> the
     * top-left pixel has color 1; tile row 1 ($200 further) plane 3 -> 8. */
    unsigned base=0x0c0000+0x800+0x80;
    image_bytes[base]=0x80;image_bytes[base+0x200+17]=0x80;
    /* Lady's palette 2 (file $1602BA + 2*$20), color 3. */
    image_bytes[0x1602ba+0x40+6]=0x34;image_bytes[0x1602ba+0x40+7]=0x92;
    Sb4RomForTests(image_bytes);
    uint8_t px[32][32];
    CHECK(Sb4Frame(2,5,px) && px[0][0]==1 && px[8][0]==8 && px[0][1]==0);
    CHECK(!Sb4Frame(6,0,px) && !Sb4Frame(0,64,px));
    CHECK(Sb4Color(3,2,3)==0x1234 && Sb4Color(3,5,0)==0 && Sb4Color(6,0,0)==0);
    Sb4RomForTests(NULL);
    CHECK(!Sb4Frame(2,5,px) && !Sb4Color(3,2,3));
    puts("sb4_rom_tests: ok");
    return 0;
}
