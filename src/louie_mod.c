/* Louie improvements use native actors, bomb motion and the Clock renderer. */
#include "louie_mod.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "mod_runtime.h"
#include "snes/interp_bridge.h"
#include "snes/saveload.h"
#include <string.h>

enum { RED=0, YELLOW=3, BLACK=6, ACTORS=24, TASKS=40 };
static bool enabled, installed, clock_borrowed, collision_resuming;
static uint16_t clock_original;
static bool red_hit;
static uint8_t original_space[160], original_limits[2];
static const uint32_t limits[]={0xc45150,0xc45158};
static const uint8_t pad_direction[]={0,1,4,2,8};
static const int dx[]={0,1,0,-1,0},dy[]={0,0,1,0,-1};
static struct {
    uint16_t freeze[ACTORS];
    uint8_t object[ACTORS], animation_speed[ACTORS];
    uint8_t dash[5], cooldown[5], direction[5], impact[5];
    uint8_t push[ACTORS], push_direction[ACTORS];
    uint8_t bomb_owner[20], block_owner[TASKS];
} state;
static uint8_t chain_direction;
static uint8_t *rom(uint32_t pc) { return SnesRomPtr(pc); }
static bool stage(void) {
    return (g_ram[0xaee]&0x7f) && !(g_ram[0x1d1e]&7) &&
        ((g_ram[0x1d1a]&1) || (g_ram[0x1031]>=13 && g_ram[0x1031]<116));
}
static bool alive(unsigned a) {
    return a<ACTORS && g_ram[0x17fa+a]<0x60 && !(g_ram[0x15a2+a]&0xc0);
}
static bool seat(unsigned a) {
    return a<5 && ((g_ram[0x1d1a]&1) || (g_ram[0x1d25]&(1u<<a)));
}
static unsigned mount(unsigned a) { return a+g_ram[0x1cbf]; }
/* Enemy actors. The cartridge puts mounts (and boss rivals) below 2 * $1CBF
 * and enemies above; five-seat co-op keeps enemies at 5-18 and moves the
 * mounts to rider + 19 (actors 19-23). */
static bool enemy(unsigned a) {
    unsigned base=g_ram[0x1cbf];
    return base>=5 ? a>=5 && a<base : a>=2*base;
}
static bool riding(unsigned a,unsigned type) {
    unsigned b=mount(a);
    return seat(a) && alive(a) && b<ACTORS && alive(b) &&
        (g_ram[0x1782+a]&0x80) && g_ram[0x16c2+b]==type;
}
static int position(unsigned a,bool y) {
    return (int16_t)(g_ram[(y?0x1872:0x182a)+a] |
                    g_ram[(y?0x188a:0x1842)+a]<<8);
}
static void set_position(unsigned a,int x,int y) {
    unsigned o=g_ram[0x17fa+a];
    int ox=position(a,false),oy=position(a,true);
    g_ram[0x182a+a]=(uint8_t)x;g_ram[0x1842+a]=(uint8_t)(x>>8);
    g_ram[0x1872+a]=(uint8_t)y;g_ram[0x188a+a]=(uint8_t)(y>>8);
    g_ram[0x16aa+a]=(uint8_t)(((y+8)/16)*16+(x+8)/16);
    if(o<0x60) {
        unsigned sx=g_ram[0x55f+o]|g_ram[0x560+o]<<8;
        unsigned sy=g_ram[0x5bf+o]|g_ram[0x5c0+o]<<8;
        sx+=x-ox;sy+=y-oy;
        g_ram[0x55f+o]=(uint8_t)sx;g_ram[0x560+o]=(uint8_t)(sx>>8);
        g_ram[0x5bf+o]=(uint8_t)sy;g_ram[0x5c0+o]=(uint8_t)(sy>>8);
    }
}
static bool move(unsigned a,unsigned dir,int pixels) {
    if(dir<1 || dir>4) return false;
    int x=position(a,false)+dx[dir]*pixels,y=position(a,true)+dy[dir]*pixels;
    /* Check the leading edge as well as the centre; never enter a wall/bomb. */
    int tx=(x+8+dx[dir]*6)/16,ty=(y+8+dy[dir]*6)/16;
    if(tx<1 || tx>13 || ty<1 || ty>11) return false;
    unsigned cell=(unsigned)(ty*16+tx);
    if((g_ram[0x1052+cell]&0xe8) || (g_ram[0x1152+cell]&0xc0) ||
       (cell!=g_ram[0x16aa+a] && (g_ram[0x1152+cell]&0x18))) return false;
    unsigned old=g_ram[0x16aa+a],next=(unsigned)(((y+8)/16)*16+(x+8)/16);
    if(old!=next) {
        uint8_t mask=seat(a)?8:16;bool shared=false;
        for(unsigned b=0;b<ACTORS;++b) if(b!=a && alive(b) && seat(b)==seat(a) && g_ram[0x16aa+b]==old) shared=true;
        if(!shared) g_ram[0x1152+old]&=(uint8_t)~mask;
    }
    set_position(a,x,y);return true;
}
static void freeze(unsigned a,unsigned duration) {
    if(!alive(a)) return;
    if(!state.freeze[a]) {
        state.object[a]=g_ram[0x17fa+a];
        state.animation_speed[a]=g_ram[0x800+state.object[a]];
    }
    state.freeze[a]=(uint16_t)duration;
}
static int bomb_at(unsigned cell) {
    for(unsigned i=0;i<20;++i)
        if(g_ram[0xe40+i] && g_ram[0xe7c+i]==cell &&
           !(g_ram[0xe90+i]&0xf8) && !(g_ram[0xecc+i]&0x20)) return (int)i;
    return -1;
}
static void a8(CpuState *cpu,unsigned value) {
    cpu->A=(uint16_t)((cpu->A&0xff00)|(value&255));
    cpu->_flag_Z=(value&255)==0;cpu->_flag_N=(value>>7)&1;
}
static void redirect(uint32_t pc) { interp_bridge_pre_opcode_redirect(pc); }

static void hook(CpuState *cpu,uint32_t pc) {
    if(!enabled || !installed) return;
    /* These two hooks bracket the native Clock helper, within one guest call. */
    if(pc==0xc34c69 && clock_borrowed) {
        g_ram[0xe20]=(uint8_t)clock_original;g_ram[0xe21]=(uint8_t)(clock_original>>8);
        clock_borrowed=false;return;
    }
    if(!stage() || g_ram[0x1d1d]) return;
    if(pc==0xc52182 && collision_resuming) { collision_resuming=false;return; }
    unsigned rider=g_ram[0x8d];
    if(pc==0xc42266 && cpu->X<ACTORS) {
        state.freeze[cpu->X]=0;state.push[cpu->X]=0;
    } else if(pc==0xc45560) {
        red_hit=false;
        if(riding(rider,RED) && !(g_ram[0x1d1a]&1)) {
            unsigned cell=cpu->A&255;
            for(unsigned a=5;a<ACTORS;++a)
                if(enemy(a) && alive(a) && g_ram[0x16aa+a]==cell) { freeze(a,180);red_hit=true; }
        }
    } else if(pc==0xc45563) {
        /* The native player-hit path already plays group 2 / $38 once. */
        if(red_hit && cpu->_flag_C) redirect(0xc45576);
        red_hit=false;
    } else if(pc==0xc4224f) {
        unsigned a=g_ram[0x8d];
        if(a<ACTORS && state.freeze[a] && alive(a) && state.object[a]==g_ram[0x17fa+a]) {
            /* Clock still checks native damage/death, but does not advance AI. */
            clock_original=g_ram[0xe20]|g_ram[0xe21]<<8;
            g_ram[0xe20]=0;g_ram[0xe21]=1;clock_borrowed=true;
            redirect(0xc34c13);
        }
    } else if(pc==0xc45797 && riding(rider,YELLOW)) {
        int b=bomb_at(cpu->X&255);
        if(b>=0) {
            unsigned dir=g_ram[0x1632+rider];
            if(dir<1 || dir>4) return;
            state.bomb_owner[b]=(uint8_t)(rider+1);
            rom(0xc45b8b)[0]=(uint8_t)dir;
            rom(0xc45b87)[0]=(uint8_t)(cpu->X-dx[dir]-16*dy[dir]);
            redirect(0xc45b80);
        }
    } else if(pc==0xc457b4) {
        if(chain_direction) { a8(cpu,chain_direction*2);chain_direction=0; }
        if(cpu->X<TASKS) state.block_owner[cpu->X]=(uint8_t)(rider+1);
    } else if(pc==0xc45973) {
        unsigned task=g_ram[0x73];int b=bomb_at(cpu->X&255);
        if(task<TASKS && state.block_owner[task] && b>=0) {
            state.bomb_owner[b]=state.block_owner[task];
            unsigned dir=g_ram[0xc87+task]/2;
            if(dir<1 || dir>4) return;
            rom(0xc45bab)[0]=(uint8_t)dir;
            rom(0xc45ba7)[0]=(uint8_t)(cpu->X-dx[dir]-16*dy[dir]);
            redirect(0xc45ba0);
        }
    } else if(pc==0xc52182 && cpu->X<20 && state.bomb_owner[cpu->X]) {
        unsigned cell=cpu->A&255,dir=g_ram[0xe90+cpu->X]&7;
        int b=bomb_at(cell);
        if(dir<1 || dir>4) return;
        if(b>=0) {
            state.bomb_owner[b]=state.bomb_owner[cpu->X];
            rom(0xc45bcb)[0]=(uint8_t)dir;rom(0xc45bc7)[0]=(uint8_t)(cell-dx[dir]-16*dy[dir]);collision_resuming=true;redirect(0xc45bc0);
        } else if(g_ram[0x1052+cell]==0x30 && g_ram[0xdef]<TASKS) {
            rom(0xc45be7)[0]=state.bomb_owner[cpu->X]-1;
            rom(0xc45beb)[0]=(uint8_t)cell;chain_direction=(uint8_t)dir;
            collision_resuming=true;redirect(0xc45be0);
        }
    } else if(pc==0xc00af9) {
        bool started=false;
        for(unsigned a=0;a<5;++a) {
            unsigned dir=g_ram[0x1632+a];
            if(!state.dash[a] && !state.push[a] && !state.cooldown[a] && g_ram[0x1782+a]==0x80 && riding(a,BLACK) &&
               (g_ram[0x36+a]&4) && dir>=1 && dir<=4) {
                state.dash[a]=30;state.cooldown[a]=36;state.direction[a]=(uint8_t)dir;
                started=true;
                g_ram[0x15a2+a]|=4;
                if(g_ram[0x170a+a]<2 && !g_ram[0x1722+a]) g_ram[0x170a+a]=2;
            }
            if(state.dash[a]) {
                g_ram[0x31+a]=state.impact[a]?0:pad_direction[state.direction[a]];g_ram[0x3b+a]=0;
                g_ram[0x36+a]&=(uint8_t)~4;
            } else if(state.push[a]) { g_ram[0x31+a]=0;g_ram[0x3b+a]=0; }
        }
        if(started) redirect(0xc45c00);
    } else if(pc==0xc43180 && cpu->X<5 && state.dash[cpu->X]) {
        a8(cpu,state.dash[cpu->X]%5<2?1:2);
    }
}

static const uint32_t hook_pcs[]={0xc45560,0xc45563,0xc4224f,0xc34c69,
    0xc42266,0xc45797,0xc457b4,0xc45973,0xc52182,0xc00af9,0xc43180};
static void install(void) {
    if(*rom(limits[0])!=5 || *rom(limits[1])!=5) return;
    for(unsigned i=0;i<sizeof(original_space);++i) if(rom(0xc45b80)[i]!=255) return;
    /* Native kick, three calling contexts: RTS; task cleanup; bomb update. */
    const uint8_t kick[]={0xda,0x5a,0x48,0xa5,0x8a,0x48,0xa9,0,0x85,0x8a,
        0xa9,0,0x22,0x93,0x22,0xc5,0x68,0x85,0x8a,0x68,0x7a,0xfa,0x60};
    uint8_t code[160];memset(code,255,sizeof(code));
    memcpy(code,kick,sizeof(kick));
    memcpy(code+32,kick,sizeof(kick));code[54]=0x5c;code[55]=0x8b;code[56]=0x59;code[57]=0xc4;
    memcpy(code+64,kick,sizeof(kick));code[86]=0x5c;code[87]=0x82;code[88]=0x21;code[89]=0xc5;
    /* Save caller A/X/Y and actor identity around the native block-task spawn. */
    const uint8_t block[]={0xda,0x5a,0x48,0xa5,0x8d,0x48,0xa9,0,0x85,0x8d,
        0xa9,0,0x85,0xa1,0xaa,0x20,0x9e,0x57,0x68,0x85,0x8d,
        0x68,0x7a,0xfa,0x5c,0x82,0x21,0xc5};
    memcpy(code+96,block,sizeof(block));
    /* Native group 2 / $35, then the original input-poll PLX/PLY/RTL tail.
       Preserve full registers and flags; the sound driver requires 8-bit X. */
    const uint8_t dash_sound[]={0x08,0xc2,0x30,0x48,0xda,0x5a,
        0xa9,0x35,0x02,0xe2,0x30,0x22,0xed,0x0c,0xc0,
        0xc2,0x30,0x7a,0xfa,0x68,0x28,0xfa,0x7a,0x6b};
    memcpy(code+128,dash_sound,sizeof(dash_sound));
    memcpy(original_space,rom(0xc45b80),sizeof(original_space));
    for(unsigned i=0;i<2;++i) { original_limits[i]=*rom(limits[i]);*rom(limits[i])=BLACK; }
    memcpy(rom(0xc45b80),code,sizeof(code));
    for(unsigned i=0;i<sizeof(hook_pcs)/sizeof(*hook_pcs);++i)
        if(!interp_bridge_add_pre_opcode_hook(hook_pcs[i],hook)) {
            for(unsigned j=0;j<2;++j) *rom(limits[j])=original_limits[j];
            memcpy(rom(0xc45b80),original_space,sizeof(original_space));return;
        }
    installed=true;
}
void LouieBeforeFrame(void) {
    if(!enabled) return;
    if(!installed) install();
    if(!installed) return;
    if(!stage()) { memset(&state,0,sizeof(state));return; }
    if(g_ram[0x1d1d]) return;
    for(unsigned a=0;a<ACTORS;++a) {
        if(!alive(a) || (state.freeze[a] && state.object[a]!=g_ram[0x17fa+a])) {
            if(state.freeze[a] && state.object[a]==g_ram[0x17fa+a] && state.object[a]<0x60)
                g_ram[0x800+state.object[a]]=state.animation_speed[a];
            state.freeze[a]=0;state.push[a]=0;
        }
        if(state.push[a]) {
            if(!move(a,state.push_direction[a],2)) state.push[a]=0;
        }
    }
    for(unsigned a=0;a<5;++a) if(state.dash[a]) {
        if(!seat(a) || !alive(a) || !(g_ram[0x1782+a]&0x80) ||
           mount(a)>=ACTORS || g_ram[0x16c2+mount(a)]!=BLACK) { state.dash[a]=0;continue; }
        g_ram[0x15a2+a]|=4;
        if(g_ram[0x170a+a]<2 && !g_ram[0x1722+a]) g_ram[0x170a+a]=2;
        unsigned dir=state.direction[a];
        for(unsigned b=0;b<ACTORS && !state.impact[a];++b) {
            if(b==a || !alive(b) || (!seat(b) && (!enemy(b) || (g_ram[0x1d1a]&1)))) continue;
            int x=position(b,false)-position(a,false),y=position(b,true)-position(a,true);
            int forward=x*dx[dir]+y*dy[dir],side=x*dy[dir]-y*dx[dir];
            if(forward>0 && forward<=20 && side>-10 && side<10 && !state.push[b]) {
                state.push[b]=16;state.push_direction[b]=(uint8_t)dir;state.impact[a]=state.dash[a]=16;
                if(!seat(b)) freeze(b,30);
            }
        }
    }
}
void LouieAfterFrame(void) {
    if(!enabled || !installed || !stage() || g_ram[0x1d1d]) return;
    for(unsigned a=0;a<ACTORS;++a) {
        if(state.freeze[a] && !--state.freeze[a] && state.object[a]<0x60 && state.object[a]==g_ram[0x17fa+a])
            g_ram[0x800+state.object[a]]=state.animation_speed[a];
        if(state.push[a]) --state.push[a];
    }
    for(unsigned a=0;a<5;++a) {
        if(state.dash[a]) --state.dash[a];
        if(state.impact[a] && !--state.impact[a]) state.dash[a]=0;
        if(state.cooldown[a]) --state.cooldown[a];
    }
    for(unsigned b=0;b<20;++b) if(!g_ram[0xe40+b] || !(g_ram[0xe90+b]&0xb0)) state.bomb_owner[b]=0;
}
void LouieReset(void) {
    for(unsigned a=0;a<ACTORS;++a)
        if(state.freeze[a] && state.object[a]<0x60 && state.object[a]==g_ram[0x17fa+a])
            g_ram[0x800+state.object[a]]=state.animation_speed[a];
    if(clock_borrowed) {
        g_ram[0xe20]=(uint8_t)clock_original;g_ram[0xe21]=(uint8_t)(clock_original>>8);
    }
    if(installed) {
        for(unsigned i=0;i<2;++i) *rom(limits[i])=original_limits[i];
        memcpy(rom(0xc45b80),original_space,sizeof(original_space));
    }
    installed=false;clock_borrowed=false;collision_resuming=false;chain_direction=0;memset(&state,0,sizeof(state));
    red_hit=false;
}
static void reset_plugin(void) {
    LouieReset();enabled=false;
}
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_louies) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.louie.improvements",activate);
}
void LouieSave(struct SaveLoadInfo *sli) {
    uint32_t magic=0x3149554c;sli->func(sli,&magic,4);sli->func(sli,&state,sizeof(state));
}
void LouieLoad(struct SaveLoadInfo *sli,unsigned version) {
    (void)version;memset(&state,0,sizeof(state));chain_direction=0;clock_borrowed=false;collision_resuming=false;
    red_hit=false;
    if(!RtlStateBytesRemaining(sli)) return;
    uint32_t magic=0;sli->func(sli,&magic,4);sli->func(sli,&state,sizeof(state));
    bool valid=magic==0x3149554c;
    for(unsigned a=0;a<ACTORS;++a)
        valid=valid && state.freeze[a]<=180 && state.push[a]<=16 && state.push_direction[a]<=4 &&
            (!state.freeze[a] || (state.object[a]<0x60 && !(state.object[a]&1))) &&
            (!state.push[a] || state.push_direction[a]);
    for(unsigned a=0;a<5;++a) valid=valid && state.dash[a]<=30 && state.cooldown[a]<=36 &&
        state.direction[a]<=4 && state.impact[a]<=16 && (!state.dash[a] || state.direction[a]);
    for(unsigned a=0;a<20;++a) valid=valid && state.bomb_owner[a]<=5;
    for(unsigned a=0;a<TASKS;++a) valid=valid && state.block_owner[a]<=5;
    if(!valid) { memset(&state,0,sizeof(state));RtlStateStreamFail(sli); }
    else if(!enabled) {
        for(unsigned a=0;a<ACTORS;++a)
            if(state.freeze[a] && state.object[a]==g_ram[0x17fa+a])
                g_ram[0x800+state.object[a]]=state.animation_speed[a];
        memset(&state,0,sizeof(state));
    }
}
