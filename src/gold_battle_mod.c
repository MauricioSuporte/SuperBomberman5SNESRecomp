/* Gold battle features, using only the user's running cartridge. */
#include "gold_battle_mod.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "mod_runtime.h"
#include "snes/interp_bridge.h"
#include "snes/saveload.h"
#include <string.h>

static bool enabled,installed;
static uint8_t drop_suppressed;
static void (*previous_hook)(uint32_t,int,int);
static uint8_t original_limit,original_count,original_left[6],original_right[6],original_space[256],original_drop[3];
static uint8_t victims[5],equipment[5][5];
static const unsigned fields[]={0x4ebd,0x17ca,0x15ea,0x15d2,0x4ed5};
static uint8_t *rom(uint32_t pc) { return SnesRomPtr(pc); }
bool GoldBattleHyper(void) { return enabled && installed && (g_ram[0x625d]&0x40); }
unsigned GoldBattleArena(void) {
    uint32_t task=g_ram[0xc90]|g_ram[0xcb8]<<8|g_ram[0xce0]<<16;
    return enabled && installed && g_ram[0x1031]==4 && task>=0xc6a1ac && task<0xc6a33d?g_ram[0x625a]:0;
}
static bool battle(void) { return (g_ram[0x1d1a]&1)!=0; }
static void instruction(uint32_t pc,int m,int x) {
    if(previous_hook) previous_hook(pc,m,x);
    if(!enabled || !installed) return;
    /* Always restore the per-call drop decision, including non-Hyper deaths. */
    if(pc==0xc436eb) { memcpy(rom(0xc436e8),original_drop,3);drop_suppressed=0; }
    if(!GoldBattleHyper() || !battle()) return;
    unsigned seat=g_ram[0x8d];
    if(seat>=5) return;
    if(pc==0xc42a3c) {
        victims[seat]=0;memset(equipment[seat],0,5);
    } else if(pc==0xc4365a) {
        for(unsigned f=0;f<5;++f) equipment[seat][f]=g_ram[fields[f]+seat];
    } else if(pc==0xc43791 && g_ram[0xa1]<5 && g_ram[0xa1]!=seat) {
        /* Native contact code has just marked the Revenge bomber to rejoin. */
        victims[seat]=(uint8_t)(g_ram[0xa1]+1);
    } else if(pc==0xc436e6) {
        if(victims[seat]) { memset(rom(0xc436e8),0xea,3);drop_suppressed=1; }
    } else if(pc==0xc42b37) {
        for(unsigned v=0;v<5;++v) if(victims[v]==seat+1) {
            for(unsigned f=0;f<5;++f) g_ram[fields[f]+seat]=equipment[v][f];
            victims[v]=0;break;
        }
    }
}
/* Authored VM programs: compare the current rule, select its neighbor, clear
 * only Revenge bits, draw the native value, and rejoin the native input loop.
 * Hyper also keeps Super's bit so the native survivor/reentry code is reused. */
static unsigned emit_rule(uint8_t *out,bool right) {
    static const uint8_t modes[]={0,8,16,80},layouts[]={0x81,0x80,0x83,0x83};
    unsigned n=0,branches[3];
    for(unsigned i=0;i<3;++i) {
        const uint8_t test[]={0x5d,0x5d,0x62,0x6d,0x58,0x86,modes[i],0x49,0};
        memcpy(out+n,test,sizeof(test));n+=sizeof(test);branches[i]=n-1;
        out[n++]=0x45;out[n++]=0;
    }
    unsigned jumps[4];
    /* The fourth state is the fall-through. Each dispatch jumps to its case. */
    jumps[3]=n;out[n++]=0x45;out[n++]=0;
    for(unsigned i=0;i<3;++i) { out[branches[i]]=2;jumps[i]=branches[i]+1; }
    unsigned cases[4]={0};
    for(unsigned i=0;i<4;++i) {
        if((right && i==3) || (!right && i==1)) {
            unsigned at=cases[i-1];out[jumps[i]+1]=(uint8_t)(at-(jumps[i]+2));continue;
        }
        unsigned target=right?(i<3?i+1:3):(i?i-1:0);
        unsigned at=n;
        cases[i]=at;out[jumps[i]+1]=(uint8_t)(at-(jumps[i]+2));
        const uint8_t clear[]={0xff,0x60,0x58,0};memcpy(out+n,clear,sizeof(clear));n+=sizeof(clear);
        if(modes[target]) { out[n++]=0xff;out[n++]=0x60;out[n++]=modes[target];out[n++]=1; }
        const uint8_t draw[]={0xff,0x8e,layouts[target],6,1,22,18,0x42,1,2,0};
        memcpy(out+n,draw,sizeof(draw));n+=sizeof(draw);
    }
    return n;
}
void GoldBattleReset(void) {
    if(installed) {
        *rom(0xc048c4)=original_count;
        if(g_ram[0x6245]==13) g_ram[0x6245]=original_limit;
        memcpy(rom(0xc69175),original_left,6);memcpy(rom(0xc692d8),original_right,6);
        memcpy(rom(0xc73000),original_space,256);memcpy(rom(0xc436e8),original_drop,3);
        if(g_interp_bridge_pc_hook==instruction) g_interp_bridge_pc_hook=previous_hook;
    }
    installed=false;previous_hook=NULL;drop_suppressed=0;
    memset(victims,0,sizeof(victims));memset(equipment,0,sizeof(equipment));
}
static void reset_plugin(void) { GoldBattleReset();enabled=false; }
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_gold_battle) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.gold.battle",activate);
}
static void install(void) {
    const uint8_t left[]={0x5d,0x5d,0x62,0x6d,0x10,0x8a},right[]={0x5d,0x5d,0x62,0x6d,0x18,0x86},drop[]={0x20,0xcd,0x37};
    if(*rom(0xc048c4)!=10 || memcmp(rom(0xc69175),left,6) ||
       memcmp(rom(0xc692d8),right,6) || memcmp(rom(0xc436e8),drop,3)) return;
    for(unsigned i=0;i<256;++i) if(rom(0xc73000)[i]!=255) return;
    uint8_t code[256];memset(code,255,sizeof(code));
    if(emit_rule(code,true)>128 || emit_rule(code+128,false)>128) return;
    original_limit=g_ram[0x6245]==13?13:10;
    original_count=*rom(0xc048c4);memcpy(original_left,rom(0xc69175),6);
    memcpy(original_right,rom(0xc692d8),6);memcpy(original_space,rom(0xc73000),256);
    memcpy(original_drop,rom(0xc436e8),3);
    *rom(0xc048c4)=13;
    const uint8_t l[]={0x24,0x80,0x30,0xc7,0x45,0x35},r[]={0x24,0,0x30,0xc7,0x45,0x60};
    memcpy(rom(0xc69175),l,6);memcpy(rom(0xc692d8),r,6);memcpy(rom(0xc73000),code,256);
    previous_hook=g_interp_bridge_pc_hook;g_interp_bridge_pc_hook=instruction;installed=true;
}
void GoldBattleBeforeFrame(void) {
    if(!enabled) return;
    if(!installed) install();
    if(!installed) return;
    /* The native boot reader and selector both consume this volatile count.
     * Never call the cartridge's persistent SRAM unlock routine. */
    g_ram[0x6245]=13;
    if(!battle() || g_ram[0x1031]==4) {
        memset(victims,0,sizeof(victims));memset(equipment,0,sizeof(equipment));
    }
}
void GoldBattleSave(struct SaveLoadInfo *sli) {
    uint32_t magic=0x31444c47;sli->func(sli,&magic,4);
    sli->func(sli,victims,sizeof(victims));sli->func(sli,equipment,sizeof(equipment));sli->func(sli,&drop_suppressed,1);
}
void GoldBattleLoad(struct SaveLoadInfo *sli,unsigned version) {
    (void)version;memset(victims,0,sizeof(victims));memset(equipment,0,sizeof(equipment));
    drop_suppressed=0;
    if(installed) memcpy(rom(0xc436e8),original_drop,3);
    if(!RtlStateBytesRemaining(sli)) return;
    uint32_t magic=0;sli->func(sli,&magic,4);
    sli->func(sli,victims,sizeof(victims));sli->func(sli,equipment,sizeof(equipment));sli->func(sli,&drop_suppressed,1);
    if(magic!=0x31444c47 || drop_suppressed>1) { RtlStateStreamFail(sli);return; }
    for(unsigned i=0;i<5;++i) if(victims[i]>5 || victims[i]==i+1) { RtlStateStreamFail(sli);return; }
    if(installed && drop_suppressed) memset(rom(0xc436e8),0xea,3);
}
