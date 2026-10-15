#include "mod_runtime.h"
#include <stdio.h>
#include <string.h>
static SNESModActivationCallback activation, reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback f) { reset_callback=f;return 1; }
int snes_mod_register_presentation_plugin(const char *id,SNESModActivationCallback f) { (void)id;activation=f;return 1; }
#include "../src/portal_labels_mod.c"
uint8 g_ram[0x20000];
static uint8 cartridge[0x400000];
const uint8 *g_rom=cartridge;
static Ppu p;
static uint16_t before[0x8000];
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static void putptr(unsigned at,unsigned value) { for(unsigned i=0;i<3;++i)cartridge[at+i]=(uint8)(value>>(i*8)); }
int main(int argc,char **argv) {
    char text[8];CHECK(label(14,0,text)&&!strcmp(text,"Z1-2"));
    CHECK(label(17,0,text)&&!strcmp(text,"Z1-5"));CHECK(label(19,0,text)&&!strcmp(text,"Z1-7"));
    CHECK(label(73,0,text)&&!strcmp(text,"Z5-1"));CHECK(label(102,0,text)&&!strcmp(text,"Z5-30"));
    CHECK(label(103,3,text)&&!strcmp(text,"Z1-A3"));CHECK(label(113,0,text)&&!strcmp(text,"Z1-S"));
    CHECK(label(104,2,text)&&!strcmp(text,"Z2-B2"));CHECK(label(112,1,text)&&!strcmp(text,"Z5-J1"));
    CHECK(!label(12,0,text)&&!label(116,0,text));
    putptr(0x60000+12*3,0xc61000);putptr(0x61000,0xc61100);
    uint8_t root[]={13,0,0x12,0xc6,11,0x10,0x12,0xc6,12,0x20,0x12,0xc6,0};
    memcpy(cartridge+0x61100,root,sizeof(root));
    const uint8_t create[]={17,17,5,7,0},exit[]={15,14,0},touch[]={18,5,7,0};
    memcpy(cartridge+0x61200,create,5);memcpy(cartridge+0x61210,exit,3);memcpy(cartridge+0x61220,touch,4);
    g_ram[0x1031]=g_ram[0x102f]=13;g_ram[0xaee]=1;
    g_ram[0x1d3b]=0xf0;g_ram[0x1d3c]=0xff;g_ram[0x1d3d]=0xe4;g_ram[0x1d3e]=0xff;
    Portal found[5];CHECK(!portals(found));activation();CHECK(portals(found)==1&&found[0].destination==14);
    g_ram[0x1d1a]=1;CHECK(!portals(found));g_ram[0x1d1a]=0;
    cartridge[0x61221]=9;CHECK(!portals(found));cartridge[0x61221]=5;
    p.bgmode=9;p.bgTileAdr=0x500;p.bgXsc[2]=0x42;p.obsel=3;
    for(unsigned t=0x80;t<0xc0;++t)for(unsigned y=0;y<8;++y)p.vram[0x5000+t*8+y]=0x7e7e;
    memcpy(before,p.vram,sizeof(before));
    PortalLabelsObserve(&p,1,NULL);CHECK(!drawn); /* No native gate, no hint. */
    g_ram[0x3d7]=0x38;g_ram[0x4a0]=0x26;g_ram[0x4ff]=6;g_ram[0x55f]=88;g_ram[0x5bf]=148;
    PortalLabelsObserve(&p,1,NULL);CHECK(!drawn); /* Allocated but unrevealed gate. */
    g_ram[0x49f]=20;g_ram[0x6df]=4;
    PortalLabelsObserve(&p,1,NULL);CHECK(!drawn); /* No matching native OAM. */
    p.oam[0]=(120<<8)|88;p.oam[1]=4<<9;
    PortalLabelsObserve(&p,1,NULL);CHECK(drawn&&memcmp(before,p.vram,sizeof(before)));
    PortalLabelsObserve(&p,224,NULL);CHECK(!drawn&&!memcmp(before,p.vram,sizeof(before))&&!p.screenEnabled[0]);
    g_ram[0x49f]=0;PortalLabelsObserve(&p,1,NULL);CHECK(!drawn);g_ram[0x49f]=20;
    /* Native object ids are even byte offsets through $5E, not 0..47. */
    const unsigned fields[]={0x3d7,0x4a0,0x4ff,0x55f,0x5bf,0x49f,0x6df};
    for(unsigned f=0;f<sizeof(fields)/sizeof(fields[0]);++f) {
        g_ram[fields[f]+80]=g_ram[fields[f]];g_ram[fields[f]]=0;
    }
    PortalLabelsObserve(&p,1,NULL);CHECK(drawn);PortalLabelsObserve(&p,224,NULL);
    g_ram[0x1d1d]=1;PortalLabelsObserve(&p,1,NULL);CHECK(!drawn);g_ram[0x1d1d]=0;
    /* Bottom-row hints retain their title glyphs and remain inside the frame. */
    cartridge[0x61203]=cartridge[0x61222]=11;g_ram[0x5bf+80]=212;p.oam[0]=(180<<8)|88;
    PortalLabelsObserve(&p,1,NULL);CHECK(drawn);
    for(unsigned i=0;i<saved_count;++i)CHECK(saved[i].address<0x6000);
    PortalLabelsObserve(&p,224,NULL);CHECK(!memcmp(before,p.vram,sizeof(before)));
    cartridge[0x61203]=cartridge[0x61222]=7;g_ram[0x5bf+80]=148;p.oam[0]=(120<<8)|88;
    /* Tile exhaustion and malformed pointers must leave the frame untouched. */
    for(unsigned i=0;i<512;++i)p.vram[0x4000+i]=(uint16_t)i;
    PortalLabelsObserve(&p,1,NULL);CHECK(!drawn);
    putptr(0x61000,0xc6fff9);CHECK(!portals(found));
    /* Route flags disambiguate the three Zone 1 boss entrances. */
    const uint8_t boss[]={6,7,6,8,5,9,15,103,0};memcpy(cartridge+0x61210,boss,sizeof(boss));
    unsigned variant=0;CHECK(destination(root,&variant)==103&&variant==3);
    reset_callback();CHECK(!PortalLabelsEnabled());
    if(argc>1) {
        FILE *file=fopen(argv[1],"rb");CHECK(file);
        CHECK(fread(cartridge,1,sizeof(cartridge),file)==sizeof(cartridge));fclose(file);
        activation();g_ram[0x1d1a]=g_ram[0x1d1e]=0;g_ram[0xaee]=1;
        unsigned total=0;
        for(unsigned stage=13;stage<116;++stage) {
            g_ram[0x1031]=g_ram[0x102f]=(uint8)stage;
            unsigned n=portals(found);CHECK(stage>=103&&stage<=112?n==0:n>=1);
            total+=n;
            for(unsigned i=0;i<n;++i)CHECK(label(found[i].destination,found[i].variant,text)&&text[0]=='Z');
        }
        const struct { unsigned stage,x,y,dest,variant; } routes[]={
            {13,5,7,14,0},{13,7,5,17,0},{13,9,7,19,0},
            {28,5,10,29,0},{28,9,10,30,0},{43,13,2,48,0},
            {58,7,9,65,0},{73,9,4,74,0},{88,11,9,89,0},
            {21,7,6,103,1},{22,8,5,103,2},{25,9,8,103,3},{23,11,9,113,0}};
        for(unsigned i=0;i<sizeof(routes)/sizeof(routes[0]);++i) {
            g_ram[0x1031]=g_ram[0x102f]=(uint8)routes[i].stage;unsigned n=portals(found);bool match=false;
            for(unsigned j=0;j<n;++j)if(found[j].x==routes[i].x&&found[j].y==routes[i].y&&
                found[j].destination==routes[i].dest&&found[j].variant==routes[i].variant)match=true;
            CHECK(match);
        }
        printf("Real-ROM script audit: %u gates across 90 numbered and three special stages passed.\n",total);
    }
    puts("portal label tests passed");return 0;
}
