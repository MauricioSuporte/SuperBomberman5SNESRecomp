#include "mod_runtime.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static SNESModActivationCallback activation,reset_callback;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback=fn;return 1; }
int snes_mod_register_activation_plugin(const char *id,SNESModActivationCallback fn) { (void)id;activation=fn;return 1; }
void NormalCoopPrepareWorldMapReturn(void) {}
static bool portuguese;
const char *PtbrPauseLabel(unsigned i) {
    static const char *const en[]={"ARE YOU SURE","NO","YES","RESUME","FORCE DRAW","RETURN TO WORLD MAP","RETURN TO STAGE SELECT"};
    static const char *const pt[]={"TEM CERTEZA","NÃO","SIM","CONTINUAR","EMPATAR RODADA","VOLTAR AO MAPA","ESCOLHER ARENA"};
    return portuguese?pt[i]:en[i];
}
#include "../src/pause_menu_mod.c"
uint8 g_ram[0x20000];
static uint8 cartridge[0x400000];
uint8_t *SnesRomPtr(uint32_t pc) { return cartridge+(pc&0x3fffff); }
static uint16 pads[5];
uint16 RtlGetPadState(int slot) { return pads[slot]; }
void (*g_interp_bridge_pc_hook)(uint32_t,int,int);
typedef struct { SaveLoadInfo base;uint8 bytes[128];size_t at,end;bool reading; } Stream;
static bool failed;
static void transfer(SaveLoadInfo *sli,void *data,size_t n) {
    Stream *s=(Stream *)sli;
    if(s->at+n>sizeof(s->bytes) || (s->reading && s->at+n>s->end)) { failed=true;return; }
    if(s->reading) memcpy(data,s->bytes+s->at,n);
    else { memcpy(s->bytes+s->at,data,n);s->end=s->at+n; }
    s->at+=n;
}
size_t RtlStateBytesRemaining(SaveLoadInfo *sli) { Stream *s=(Stream *)sli;return s->end-s->at; }
void RtlStateStreamFail(SaveLoadInfo *sli) { (void)sli;failed=true; }
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1; } } while(0)
static void edge(uint16_t pad) { pads[0]=0;PauseMenuBeforeFrame();pads[0]=pad;PauseMenuBeforeFrame(); }
static unsigned chained;
static void other(uint32_t pc,int m,int x) { (void)pc;(void)m;(void)x;++chained; }
int main(void) {
    memset(cartridge,0xff,sizeof(cartridge));
    g_interp_bridge_pc_hook=other;PauseMenuBeforeFrame();CHECK(!installed);
    activation();PauseMenuBeforeFrame();CHECK(!installed); /* Refuse unknown code. */
    const uint8_t signature[]={0x64,0xb1,0xa0,0x04},call[]={0x22,0xed,0x0c,0xc0};
    memcpy(rom(0xc500b7),signature,4);memcpy(rom(0xc50096),call,4);
    activation();PauseMenuBeforeFrame();CHECK(installed && rom(0xc500b7)[0]==0x5c);
    CHECK(!memcmp(rom(0xc50096),call,4)); /* Native entry sound stays installed. */
    g_ram[0x1031]=13;g_ram[0xaee]=1;g_ram[0x1d1d]=1;g_ram[0x1d1a]=0x42;g_ram[0x1d23]=1;
    g_ram[0x16c2]=3;g_ram[0x16da]=25;g_ram[0x17ca]=7;
    PauseMenuAfterFrame();CHECK(open_menu && selection==0 && !confirm_menu);
    edge(0x20);CHECK(selection==1 && sound_pending==1);
    PauseMenuBeforeFrame();CHECK(selection==1); /* Held direction never repeats. */
    edge(0x100);CHECK(!confirm_menu && selection==1);
    edge(8);CHECK(confirm_menu && selection==0);
    PauseMenuBeforeFrame();CHECK(confirm_menu && selection==0); /* Held Start cannot cascade. */
    edge(0x100);CHECK(confirm_menu && selection==0); /* A cannot accept No. */
    edge(8);CHECK(!confirm_menu && selection==1 && g_ram[0x1031]==13);
    edge(8);CHECK(confirm_menu && selection==0);
    edge(1);CHECK(!confirm_menu && sound_pending==3);
    edge(8);edge(0x20);edge(8);instruction(0xc500c8,1,1);
    CHECK(g_ram[0x1031]==3 && (g_ram[0x1d1e]&4) && returning && !(g_ram[0x1d1a]&0x40));
    g_ram[0x16c2]=0;g_ram[0x16da]=1;g_ram[0x17ca]=2;g_ram[0x1d23]=3;
    instruction(0xc428ce,1,1);
    CHECK(g_ram[0x16c2]==3 && g_ram[0x16da]==25 && g_ram[0x17ca]==7 && g_ram[0x1d23]==1);
    g_ram[0xbd]=0x36;g_ram[0xbe]=0x10;g_ram[0x0c]=0xc7;g_ram[0x0b]=0x0b;g_ram[0x0a]=0x8e;
    instruction(0xc21aec,1,1);CHECK(g_ram[0x373]==4 && !returning && keep_campaign);
    /* From the map back to the file menu: New Game/Continue drops the kept
     * lives, so the next player reset keeps its own values. */
    g_ram[0x373]=0;instruction(0xc21aec,1,1);CHECK(g_ram[0x373]==0 && !keep_campaign);
    g_ram[0x1d23]=7;instruction(0xc428ce,1,1);CHECK(g_ram[0x1d23]==7);
    keep_campaign=true;g_ram[0x1031]=2;PauseMenuAfterFrame();CHECK(!keep_campaign);
    returning=0;g_ram[0x1031]=116;g_ram[0x1d1a]=1;g_ram[0x1d1e]=0;g_ram[0x1d1d]=1;
    PauseMenuAfterFrame();CHECK(open_menu && selection==0);
    for(unsigned i=0;i<5;++i) g_ram[0x1ba3+i]=2;
    edge(0x20);edge(8);CHECK(action_requested);instruction(0xc500c8,1,1);CHECK(g_ram[0xb1]==1);
    for(unsigned i=0;i<5;++i) CHECK((g_ram[0x15a2+i]&0x80) && !(g_ram[0x1ba3+i]&2));
    CHECK(!resume_requested);
    instruction(0xc500c8,1,1);CHECK(g_ram[0xb1]==0);
    edge(0x20);CHECK(selection==2);edge(8);CHECK(confirm_menu && selection==0);
    Stream stream={.base.func=transfer};PauseMenuSave(&stream.base);
    selection=2;stream.reading=true;stream.at=0;PauseMenuLoad(&stream.base,0);
    CHECK(!failed && confirm_menu && selection==0);
    stream.bytes[7]=255;stream.at=0;PauseMenuLoad(&stream.base,0);CHECK(failed);
    failed=false;stream.end=stream.at=0;PauseMenuLoad(&stream.base,0);CHECK(!failed && !open_menu);
    open_menu=true;confirm_menu=false;selection=2;g_ram[0x1d1d]=1;
    edge(8);edge(0x20);edge(8);instruction(0xc500c8,1,1);CHECK(g_ram[0x1031]==4 && g_ram[0x35b2]==1);
    /* Render-only writes restore every word and register, with no guest data mutation. */
    Ppu *p=calloc(1,sizeof(*p)),*copy=malloc(sizeof(*p));CHECK(p && copy);
    p->bgmode=9;p->bgTileAdr=0x500;p->bgXsc[2]=0x42;p->obsel=3;p->screenEnabled[0]=23;
    open_menu=true;confirm_menu=false;selection=0;*copy=*p;
    p->vram[0x4000+14*32+12]=0x288d;*copy=*p;
    PauseMenuObserve(p,1,NULL);CHECK(drawn && saved_count<1600);
    CHECK(p->screenEnabled[0]==23 && p->vram[0x4000+14*32+12]==0x288d);
    CHECK(p->vram[0x4000]==copy->vram[0x4000]);
    CHECK(!memcmp(p->vram+0x6000,copy->vram+0x6000,0x2000*sizeof(uint16_t)));
    CHECK(!memcmp(p->cgram,copy->cgram,sizeof(p->cgram)));
    PauseMenuObserve(p,panel_top*8,NULL);CHECK(p->cgadsub==0x7b && !(p->cgadsub&4));
    CHECK(p->fixedColor==0 && p->cgwsel==0x10 && p->window1left==16 && p->window1right==239);
    CHECK(p->vram[0x5000+glyph_first*8]==0); /* Transparent fill; only the field dims. */
    PauseMenuObserve(p,(panel_bottom+1)*8,NULL);CHECK(p->cgadsub==copy->cgadsub);
    PauseMenuObserve(p,224,NULL);CHECK(!drawn && !memcmp(p->vram,copy->vram,sizeof(p->vram)));
    CHECK(!memcmp(p->cgram,copy->cgram,sizeof(p->cgram)) && p->screenEnabled[0]==23);
    CHECK(p->windowsel==copy->windowsel && p->fixedColor==copy->fixedColor &&
          p->window1left==copy->window1left && p->window1right==copy->window1right);
    portuguese=true;confirm_menu=true;selection=0;*copy=*p;
    PauseMenuObserve(p,1,NULL);CHECK(drawn);
    CHECK((p->vram[0x4000+19*32+17]&1023)==glyph_first+39);
    PauseMenuObserve(p,224,NULL);CHECK(!memcmp(p->vram,copy->vram,sizeof(p->vram)));
    confirm_menu=false;selection=0;open_menu=true;g_ram[0x1d1d]=1;
    edge(8);CHECK(resume_requested && sound_pending==0);
    free(p);free(copy);
    reset_callback();CHECK(g_interp_bridge_pc_hook==other && !memcmp(rom(0xc500b7),signature,4) && !memcmp(rom(0xc50096),call,4) && rom(0xc57180)[0]==255);
    CHECK(chained>=3);puts("Pause menu defaults, confirmations, actions, states and render restoration passed.");return 0;
}
