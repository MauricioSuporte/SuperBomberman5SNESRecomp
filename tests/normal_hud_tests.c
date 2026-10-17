#include "mod_runtime.h"
#include <stdio.h>
#include <stdlib.h>
static SNESModActivationCallback activation, cleanup;
int snes_mod_register_reset_callback(SNESModActivationCallback f){cleanup=f;return 1;}
int snes_mod_register_activation_plugin(const char *id,SNESModActivationCallback f){(void)id;activation=f;return 1;}
#include "../src/normal_hud_mod.c"
static InterpPreOpcodeHook hooks[2];static uint32_t hook_pcs[2];static unsigned hook_count;
bool interp_bridge_add_pre_opcode_hook(uint32_t pc,InterpPreOpcodeHook h){
    if(hook_count>=2)return false;hook_pcs[hook_count]=pc;hooks[hook_count++]=h;return true;}
uint8 g_ram[0x20000];
const uint8 *g_rom;
uint8 *SnesRomPtr(uint32 pc){return (uint8 *)g_rom+(pc&0x3fffff);}
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d\n",__LINE__);return 1;}}while(0)
int main(void){
    uint8 *r=calloc(1,0x400000);g_rom=r;
    memcpy(r+0x505f1,"\x22\x86\x0c\xc5",4);
    memcpy(r+0x5126b,"\xf0\x52",2);
    memcpy(r+0x436ff,"\xad\x1a\x1d\x89\x01",5);
    memset(r+0x57200,255,256);
    memset(r+0x57300,255,96);
    memcpy(r+0x193a,"\x80\x02\x80\x00",4);
    memcpy(r+0x50608,"\x20\xcd\x06",3);
    memcpy(r+0x42b33,"\x22\x07\x13\xc5",4);
    g_ram[0x1031]=13;
    NormalHudBeforeFrame();CHECK(!installed);
    activation();g_ram[0x1d1a]=1;NormalHudBeforeFrame();CHECK(!installed);
    g_ram[0x1d1a]=0;NormalHudBeforeFrame();CHECK(installed);
    CHECK(r[0x505f2]==0 && r[0x5126b]==0xea && r[0x436ff]==0x22);
    /* Branches must land on the next seat or the allocator, never operands. */
    for(unsigned seat=0;seat<2;++seat) {
        unsigned at=0x57206+seat*39;
        CHECK(r[at+5]==0x30 && at+7+r[at+6]==at+39);
        CHECK(r[at+24]==0xd0 && at+26+r[at+25]==at+33);
        CHECK(r[at+31]==0xf0 && at+33+r[at+32]==at+39);
    }
    /* The reentry guard calls the native face updater for P1/P2 only. */
    for(unsigned seat=0;seat<5;++seat) {
        unsigned pc=0x572a0;
        CHECK(r[pc]==0xc9 && r[pc+1]==2 && r[pc+2]==0xb0);
        pc=seat>=r[pc+1]?pc+4+r[pc+3]:pc+4;
        CHECK(r[pc]==(seat<2?0x22:0x6b));
    }
    CHECK(r[0x42b34]==0xa0 && r[0x50609]==0xb0);
    /* The per-tick update runs the full entry (with the lives refresh) one
     * frame in eight and the face checks only ($C5:7204) otherwise. */
    {
        const uint8_t *u=r+0x572b0;
        CHECK(u[0]==0x20 && u[7]==0xaf && u[8]==0x69 && u[13]==0xf0);
        unsigned fast=15,full=15+u[14];
        CHECK(u[fast+2]==0x22 && u[fast+3]==0x04 && u[fast+6]==0x60);
        CHECK(u[full]==0x68 && u[full+2]==0x22 && u[full+3]==0x00 && u[full+6]==0x60);
        CHECK(0x572b0+full+7<=0x57300);
    }
    CHECK(r[0x57200]==0x22 && r[0x57201]==0x86 && r[0x57204]==0xda);
    memcpy(r+0x51319,"\xbf\x70\x14\xc2",4);r[0x21470]=20;
    g_ram[0x35c5]=10;g_ram[0x4ff+10]=6;g_ram[0x4a0+10]=0xf0;
    g_ram[0x79f+10]=29;NormalHudBeforeFrame();CHECK(g_ram[0x35ca]==1);
    g_ram[0x35ca]=0;g_ram[0x15a2]=128;NormalHudBeforeFrame();CHECK(!g_ram[0x35ca]);
    g_ram[0x15a2]=0;
    CHECK(r[0x5727e]==0x6b && r[0x5727f]==255);
    uint8 copy[256];memcpy(copy,r+0x57200,256);
    NormalHudBeforeFrame();CHECK(!memcmp(copy,r+0x57200,256));
    /* Boss stages keep the stock heads: the face archive left no OBJ VRAM
     * for ZONE3 - D's exit gates. The patches come back on the next stage. */
    g_ram[0x1031]=0x6a;NormalHudBeforeFrame();
    CHECK(!installed && r[0x505f1]==0x22 && r[0x505f2]==0x86 && r[0x436ff]==0xad && r[0x57200]==0xff);
    g_ram[0x1031]=0x73;NormalHudBeforeFrame();CHECK(!installed);
    g_ram[0x1031]=13;NormalHudBeforeFrame();CHECK(installed && !memcmp(copy,r+0x57200,256));
    /* Allocator failure exits are watched; hooks are added once. */
    CHECK(hook_count==2 && hook_pcs[0]==0xc0193a && hook_pcs[1]==0xc0193c && hooks[0]==vram_full);
    /* Gate room routine: branches land on the next seat or the epilogue. */
    {
        const uint8_t *q=r+0x57300;
        CHECK(q[0]==0x20 && q[1]==0xcd && q[2]==0x06 && q[10]==0x30);
        for(unsigned seat=0;seat<2;++seat) {
            unsigned b=11+seat*34;
            CHECK(q[b]==0xaf && q[b+1]==0xc5+seat && q[b+6]==0xb0 && b+8+q[b+7]==b+34);
            CHECK(q[b+14]==0xd0 && b+16+q[b+15]==b+34 && q[b+21]==0xd0 && b+23+q[b+22]==b+34);
            CHECK(q[b+24]==0x22 && q[b+25]==0x42 && q[b+26]==0x13 && q[b+27]==0xc0);
            CHECK(q[b+30]==0x8f && q[b+31]==0xc5+seat);
        }
        CHECK(q[79]==0xc2 && q[85]==0x60 && q[86]==0xff);
    }
    /* A failed face allocation itself is ignored. */
    g_ram[0xadf]=0x64;hooks[1](NULL,0xc0193c);NormalHudBeforeFrame();
    CHECK(installed && !releasing && r[0x50609]==0xb0);
    /* Another archive failing (ZONE3 - 1's gate, $63) releases the faces:
     * one HUD tick through $C5:7300, then the stock HUD for this stage. */
    g_ram[0xadf]=0x63;hooks[0](NULL,0xc0193a);NormalHudBeforeFrame();
    CHECK(releasing && r[0x50608]==0x20 && r[0x50609]==0x00 && r[0x5060a]==0x73);
    NormalHudBeforeFrame();CHECK(releasing && installed); /* face still live */
    g_ram[0x35c5]=0xff;NormalHudBeforeFrame();
    CHECK(!installed && suspended_stage==13 && r[0x50609]==0xcd && r[0x57300]==0xff && r[0x57200]==0xff);
    NormalHudBeforeFrame();CHECK(!installed);
    g_ram[0x1031]=8;NormalHudBeforeFrame();CHECK(installed && !suspended_stage);
    g_ram[0x1031]=13;g_ram[0x35c5]=10;NormalHudBeforeFrame();CHECK(installed && !memcmp(copy,r+0x57200,256));
    Ppu *p=calloc(1,sizeof(Ppu));p->bgmode=1;p->bgTileAdr=0x500;p->obsel=3;
    for(unsigned i=0;i<0x8000;++i)p->vram[i]=(uint16_t)i;
    uint16_t before[0x8000];memcpy(before,p->vram,sizeof(before));
    g_ram[0x35c5]=10;g_ram[0x35c6]=12;g_ram[0x4ff+10]=g_ram[0x4ff+12]=6;g_ram[0x4a0+10]=g_ram[0x4a0+12]=0xf0;
    NormalHudObserve(p,1,NULL);CHECK(drawn);
    CHECK(p->vram[3]==0x2000 && p->vram[20]==0x2000);
    NormalHudObserve(p,224,NULL);CHECK(!memcmp(before,p->vram,sizeof(before)));
    /* Reproduce the real BG2 stock heads: all three rows, both seats.
     * Every interior pixel must be background; border scanlines stay native. */
    p->bgXsc[0]=0x32;p->bgXsc[1]=0x3a;
    for(unsigned i=0x3000;i<0x4000;++i)p->vram[i]=0x2000;
    for(unsigned seat=0;seat<2;++seat)for(unsigned y=0;y<3;++y)for(unsigned x=0;x<2;++x) {
        unsigned tile=(seat?0xf4:0xee)+y*2+x;
        p->vram[0x3c00+y*32+(seat?20:3)+x]=(uint16_t)(0x2000+tile);
        for(unsigned w=0;w<16;++w)p->vram[tile*16+w]=0xa55a;
    }
    memcpy(before,p->vram,sizeof(before));
    NormalHudObserve(p,1,NULL);CHECK(drawn);
    for(unsigned seat=0;seat<2;++seat)for(unsigned y=0;y<24;++y)for(unsigned x=0;x<2;++x) {
        unsigned tile=p->vram[0x3c00+(y/8)*32+(seat?20:3)+x]&1023;
        CHECK(tile>=0x280 && tile<0x300);
        CHECK(p->vram[tile*16+y%8]==(y>=4&&y<=20?0:0xa55a));
        CHECK(p->vram[tile*16+y%8+8]==(y>=4&&y<=20?255:0xa55a));
    }
    CHECK((p->vram[0x3c03]&1023)!=(p->vram[0x3c14]&1023));
    NormalHudObserve(p,224,NULL);CHECK(!memcmp(before,p->vram,sizeof(before)));
    g_ram[0x15a2]=g_ram[0x15a3]=128;
    for(unsigned i=0;i<1024;++i)p->vram[i]=0x2000;
    for(unsigned y=0;y<2;++y)for(unsigned x=0;x<10;++x)
        p->vram[y*32+3+x]=p->vram[y*32+21+x]=0x2001;
    for(unsigned y=0;y<8;++y)p->vram[0x5008+y]=0xffff;
    memcpy(before,p->vram,sizeof(before));
    NormalHudObserve(p,1,NULL);CHECK(drawn && saved_count<640);
    CHECK((p->vram[5]&1023)>=256 && (p->vram[22]&1023)>=272);
    CHECK(p->vram[0x5800]==0xffff); /* Both planes survive text resizing. */
    NormalHudObserve(p,224,NULL);CHECK(!memcmp(before,p->vram,sizeof(before)));
    g_ram[0x1031]=4;NormalHudBeforeFrame();CHECK(!installed);
    CHECK(r[0x42b34]==0x07 && r[0x50609]==0xcd);
    CHECK(r[0x505f2]==0x86 && r[0x5126b]==0xf0 && r[0x436ff]==0xad);
    for(unsigned i=0;i<256;++i)CHECK(r[0x57200+i]==255);
    for(unsigned i=0;i<96;++i)CHECK(r[0x57300+i]==255);
    g_ram[0x1031]=13;r[0x57280]=0;NormalHudBeforeFrame();CHECK(!installed);
    cleanup();CHECK(!NormalHudEnabled());free(r);free(p);
    puts("Normal HUD context, patches, refusal and exact render restoration passed.");return 0;
}
