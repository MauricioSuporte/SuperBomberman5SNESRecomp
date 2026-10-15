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
    /* Sheets by address: Hammer $CC:8000, Bazooka $CB:0000. */
    CHECK(Sb4FramePc(1,0)==0xcc8000 && Sb4FramePc(4,5)==0xcb0000+0x800+0x80 && !Sb4FramePc(6,0));
    CHECK(Sb4FrameAt(0xcc0000+0x800+0x80,px) && px[0][0]==1 && !Sb4FrameAt(0x400000,px));
    /* Fire stream at $D1:D019: markers $AA (empty tile) / $BB (end); an
     * empty tile, then one whose first byte lights pixel (0, 0) with color
     * 1; the second stream ($D1:E01C) ends at once. Fire block 0 = tiles 0,
     * 1, 16, 17: tile 1 is the block's top-right quarter. */
    uint8_t *fire=image_bytes+0x11d019;
    fire[0]=0xaa;fire[1]=0xbb;fire[2]=0xaa;fire[3]=0x80;fire[3+32]=0xbb;
    image_bytes[0x11e01c]=0xaa;image_bytes[0x11e01d]=0xbb;image_bytes[0x11e01e]=0xbb;
    image_bytes[0x17e11c+4]=0x21;image_bytes[0x17e11c+5]=0x43;
    uint8_t block[16][16];
    CHECK(Sb4FireBlock(0,block) && block[0][8]==1 && block[0][0]==0 && block[0][9]==0 && block[8][8]==0);
    CHECK(Sb4FireColor(2)==0x4321 && !Sb4FireBlock(0xef,block));
    /* Effect stream at $C8:FD36: OBJ tile $80 is its first tile. */
    uint8_t *effect=image_bytes+0x08fd36;
    effect[0]=0x11;effect[1]=0x22;effect[2]=0x40;effect[2+32]=0x22;
    Sb4RomForTests(image_bytes); /* decoded again */
    CHECK(Sb4EffectBlock(0x80,block) && block[0][1]==1 && !Sb4EffectBlock(0x7f,block));
    /* Portraits: Hammer's starts at $CD:EE87 with tile rows $184 apart;
     * palette 1 color 2 at $167B7C + $20 + 4. */
    image_bytes[0xdee87+0x184+0x20]=0x80;
    image_bytes[0x167b7c+0x20+4]=0x78;image_bytes[0x167b7c+0x20+5]=0x56;
    CHECK(Sb4Portrait(1,px) && px[8][8]==1 && px[8][9]==0 && px[0][0]==0 && Sb4PortraitColor(1,1,2)==0x5678);
    CHECK(!Sb4Portrait(6,px) && !Sb4PortraitColor(1,5,0));
    Sb4RomForTests(NULL);
    CHECK(!Sb4Frame(2,5,px) && !Sb4Color(3,2,3));
    puts("sb4_rom_tests: ok");
    return 0;
}
