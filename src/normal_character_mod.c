/* Native Battle character picker, temporarily entered from Normal's file
 * confirmation. All graphics come from the player's verified cartridge. */
#include "normal_character_mod.h"
#include "extra_characters_mod.h"
#include "normal_coop_mod.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "mod_runtime.h"
#include "snes/snes.h"
#include "snes/interp_bridge.h"
#include "snes/saveload.h"
#include <stdlib.h>
#include <string.h>

enum { SNAP_CAP = 1024*1024, RESUME_CAP = 384*1024 };
static bool enabled, picking, picker_ready, chosen, loaded;
static bool internal_snapshot;
static bool from_continue;   /* picker opened from Continue -> File */
static unsigned players;
static bool hook_installed, restarting;
static void (*previous_hook)(uint32_t,int,int);
static void campaign_hook(uint32_t pc,int m,int x) {
    if(previous_hook)previous_hook(pc,m,x);
    unsigned mode=g_ram[0x1031];
    if(!enabled || !chosen || picking || (g_ram[0x1d1a]&1) ||
       !(mode==8 || (mode>=13 && mode<116)) ||
       (pc!=0xc428b4 && pc!=0xc21697))return;
    /* Game Over/Continue resets the actor fields, but retains the picker
     * choices. Restore after reset and again before stage graphics load.
     * Honey/Kotetsu use base choice 1; their WRAM skin markers remain intact. */
    if(pc==0xc428b4)restarting=true;
    if(pc==0xc21697 && restarting) {
        /* Stock Continue keeps only the last survivor's participation bit.
         * Restart the selected one/two-seat roster, once, not on stage exits.
         * Larger rosters remain owned by the co-op boss/bench adapter. */
        if(players<=2)g_ram[0x1d24]=g_ram[0x1d25]=(uint8_t)((1u<<players)-1);
        restarting=false;
    }
    for(unsigned i=0;i<players;++i) {
        unsigned c=g_ram[0x625f+i],color=g_ram[0x6264+i];
        if(c<1 || c>10 || color>5)continue;
        g_ram[0x16c2+i]=(uint8_t)(c-1);
        g_ram[0x16da+i]=(uint8_t)(g_rom[0xbb4e + c-1]+color);
    }
}
void NormalCharacterInstallHook(void) {
    if(!enabled || hook_installed)return;
    previous_hook=g_interp_bridge_pc_hook;g_interp_bridge_pc_hook=campaign_hook;
    hook_installed=true;
}
static uint8_t *resume;
static uint32_t resume_size;
static uint16_t previous_pad;
static uint16_t previous_seat_pads[5],held_pads[5];
static uint8_t masked_b;
/* The file-confirmation snapshot of the last finished picker: B on the world
 * map, before any stage was played, reopens the picker from it. */
static uint8_t *repick;
static uint32_t repick_size;

static uint32_t picker_pc(void) {
    /* Script task 9. These are three parallel arrays, not CPU registers. */
    return g_ram[0xc90]|((uint32_t)g_ram[0xcb8]<<8)|((uint32_t)g_ram[0xce0]<<16);
}
static void set_picker_pc(uint32_t pc) {
    g_ram[0xc90]=(uint8_t)pc;g_ram[0xcb8]=(uint8_t)(pc>>8);g_ram[0xce0]=(uint8_t)(pc>>16);
}
void NormalCharacterReset(void) {
    if(hook_installed && g_interp_bridge_pc_hook==campaign_hook)
        g_interp_bridge_pc_hook=previous_hook;
    hook_installed=false;previous_hook=NULL;restarting=false;
    free(resume);resume=NULL;resume_size=0;
    free(repick);repick=NULL;repick_size=0;
    picking=picker_ready=chosen=loaded=from_continue=false;players=1;previous_pad=0;
    memset(previous_seat_pads,0,sizeof(previous_seat_pads));masked_b=0;
}
static void reset_plugin(void) { NormalCharacterReset();enabled=false; }
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_normal_characters) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.normal.characters",activate);
}

static void open_picker(void) {
    players=NormalCoopPlayerCount();
    picking=true;picker_ready=false;
    set_picker_pc(0xc689cf);
    g_ram[0x1031]=4;
    for(unsigned i=0;i<5;++i) g_ram[0x6269+i]=i<players?1:2;
    g_ram[0x1038]=0;
}
static void finish_picker(bool cancel) {
    uint8_t choices[5],palettes[5],extra_colors[5];
    for(unsigned i=0;i<5;++i)extra_colors[i]=(uint8_t)ExtraCharacterSeatColor(i);
    memcpy(choices,g_ram+0x625f,sizeof(choices));
    memcpy(palettes,g_ram+0x6264,sizeof(palettes));
    unsigned count=players;
    bool continued=from_continue;
    /* Detach before loading: the save extension replaces its own buffer. */
    uint8_t *snapshot=resume;uint32_t size=resume_size;
    resume=NULL;resume_size=0;
    bool ok=RtlLoadSnapshotFromMemory(snapshot,size);
    picking=false;
    free(repick);repick=NULL;repick_size=0;
    if(ok && !cancel) { repick=snapshot;repick_size=size; } else free(snapshot);
    if (!ok) return;
    players=count;from_continue=continued;
    if (cancel) {
        chosen=false;
        /* Restart the whole native file-list entry, including screen cleanup
         * and the FF.8D archive load. $C7:1014 is inside that load's operands.
         * Unwind the suspended file-confirm fade's VM call before restarting
         * its caller, so re-confirming the file has the original return stack. */
        set_picker_pc(from_continue?0xc70d59:0xc7100e);
        if(g_ram[0x0d08]) --g_ram[0x0d08]; /* task 9 VM call depth */
        unsigned stack=g_ram[0x10]|((unsigned)g_ram[0x11]<<8);
        if(stack>=3) {
            stack-=3;
            g_ram[0x10]=(uint8_t)stack;g_ram[0x11]=(uint8_t)(stack>>8);
        }
    } else {
        chosen=true;
        for(unsigned i=0;i<count;++i) {
            /* Extra characters retain their skin and independent picker palette. */
            unsigned choice=choices[i]>=1 && choices[i]<=EXTRA_CHOICE_LAST?choices[i]:1;
            ExtraSetSeatChoice(i,choice);
            ExtraSetSeatColor(i,extra_colors[i]);
            g_ram[0x625f+i]=(uint8_t)ExtraBaseChoice(choice);
            g_ram[0x6264+i]=palettes[i];
            /* Normal initializes these before showing the file menu, while
             * Battle derives them after the picker. Apply that same mapping
             * to the suspended campaign before it loads the stage graphics. */
            unsigned character=g_ram[0x625f+i]-1;
            g_ram[0x16c2+i]=(uint8_t)character;
            g_ram[0x16da+i]=g_rom[0xbb4e + character]+palettes[i];
        }
    }
}

/* Return stacks of the file-confirmation fade: New Game ($C7:0BFF/$C7:10E6)
 * and Continue ($C7:0BEA/$C7:0E2B). Other fades (menus, Back) differ. */
static bool return_stack(uint32_t outer,uint32_t inner) {
    return g_ram[0x2003]==(uint8_t)outer && g_ram[0x2004]==(uint8_t)(outer>>8) &&
        g_ram[0x2005]==(uint8_t)(outer>>16) && g_ram[0x2006]==(uint8_t)inner &&
        g_ram[0x2007]==(uint8_t)(inner>>8) && g_ram[0x2008]==(uint8_t)(inner>>16);
}
static bool confirmed_new_game(void) { return return_stack(0xc70bff,0xc710e6); }
static bool confirmed_continue(void) { return return_stack(0xc70bea,0xc70e2b); }

void NormalCharacterBeforeFrame(void) {
    /* Consume Back before the native picker changes its script task. Any
     * seat's B returns to the file menu; the native picker never sees it
     * (its own B left Battle's picker through Battle's exit script, which
     * with a confirmed seat carried the suspended Normal file confirmation
     * on to the world map). */
    masked_b=0;
    if(enabled && !picking && chosen && repick && g_ram[0x1031]==3 &&
       picker_pc()>=0xc71370 && picker_pc()<0xc713b0) {
        /* World map before any stage: B goes back to the character picker
         * (same seats) instead of the Normal menu. */
        bool back=false;
        for(unsigned i=0;i<5;++i) {
            uint16_t pad=RtlGetPadState((int)i);
            if(i<players && ((pad & ~previous_seat_pads[i])&0x0001)) back=true;
            previous_seat_pads[i]=pad;
        }
        if(back) {
            bool continued=from_continue;unsigned count=players;
            uint8_t *snapshot=repick;uint32_t size=repick_size;
            repick=NULL;repick_size=0;
            if(RtlLoadSnapshotFromMemory(snapshot,size)) {
                from_continue=continued;
                resume=snapshot;resume_size=size;
                open_picker();players=count;
                for(unsigned i=0;i<5;++i) {
                    uint16_t pad=RtlGetPadState((int)i);
                    if(pad&0x0001) { held_pads[i]=pad;RtlSetPadState((int)i,(uint16_t)(pad&~0x0001));masked_b|=(uint8_t)(1u<<i); }
                }
            } else free(snapshot);
        }
        return;
    }
    if(enabled && !picking) for(unsigned i=0;i<5;++i) previous_seat_pads[i]=RtlGetPadState((int)i);
    if(!enabled || !picking) return;
    bool back=false;
    for(unsigned i=0;i<5;++i) {
        uint16_t pad=RtlGetPadState((int)i);
        if(i<players && picker_pc()==0xc68e68 && ((pad & ~previous_seat_pads[i])&0x0001)) back=true;
        previous_seat_pads[i]=pad;
        if(pad&0x0001) { held_pads[i]=pad;RtlSetPadState((int)i,(uint16_t)(pad&~0x0001));masked_b|=(uint8_t)(1u<<i); }
    }
    if(back) finish_picker(true);
}

void NormalCharacterFrame(void) {
    for(unsigned i=0;i<5;++i) if(masked_b&(1u<<i)) RtlSetPadState((int)i,held_pads[i]);
    masked_b=0;
    if (!enabled) return;
    uint32_t pc=picker_pc();
    uint16_t pad=RtlGetPadState(0);
    previous_pad=pad;
    if (picking) {
        /* Initialization reuses $1038 before the picker starts accepting A. */
        if(pc==0xc68e68) picker_ready=true;
        /* The last human's confirmation can advance the native script in
         * this same field. $1038 counts confirmed seats in selection order. */
        if (picker_ready && g_ram[0x1038]>=players) finish_picker(false);
        return;
    }
    if(repick && g_ram[0x1031]!=3) { free(repick);repick=NULL;repick_size=0; } /* a stage started */
    if (g_ram[0x1031]!=3) {
        /* Game Over and its Continue map keep the current campaign choice. */
        if(g_ram[0x1031]==2){chosen=false;restarting=false;}
        return;
    }
    if(pc==0xc7100e || pc==0xc70d59){chosen=false;restarting=false;}
    /* This fade belongs to a confirmed file; Back/cancel uses another return
     * address. Intercept once, before the Zone map initializes gameplay. */
    if (!chosen && pc==0xc71257 && (confirmed_new_game() || confirmed_continue())) {
        from_continue=confirmed_continue();
        uint8_t *snapshot=malloc(SNAP_CAP);
        if (!snapshot) return;
        internal_snapshot=true;
        size_t size=RtlSaveSnapshotToMemory(snapshot,SNAP_CAP);
        internal_snapshot=false;
        if (!size || size>RESUME_CAP) { free(snapshot);return; }
        resume=snapshot;resume_size=(uint32_t)size;
        open_picker();
    }
}

/* Rewind and save states include the suspended Normal machine, so loading
 * inside the picker is as safe as loading during a stage. No SRAM changes. */
void NormalCharacterSave(SaveLoadInfo *sli) {
    RtlSaveExecutionState(sli);
    uint32_t capacity=internal_snapshot?0:RESUME_CAP;
    /* State 3: picker from Continue. State 6: selected campaign awaiting
     * its post-Continue spawn. The header and rewind slot sizes stay fixed. */
    uint32_t header[5]={0x32434e42, picking?(from_continue?3u:1u):chosen?(restarting?6u:2u):0u,players,resume_size,capacity};
    sli->func(sli,header,sizeof(header));
    if(resume_size) sli->func(sli,resume,resume_size);
    /* The rewind ring sizes its slots from the first capture. Keep public
     * snapshots a constant size even while the suspended machine is present.
     * The private checkpoint omits this reserve and never contains itself. */
    static const uint8_t zeroes[4096]={0};
    for(uint32_t at=resume_size;at<capacity;) {
        uint32_t n=capacity-at;if(n>sizeof(zeroes)) n=sizeof(zeroes);
        sli->func(sli,(void *)zeroes,n);at+=n;
    }
}
void NormalCharacterLoad(SaveLoadInfo *sli,uint32_t version) {
    (void)version;
    NormalCharacterReset();loaded=true;
    if(!RtlLoadExecutionState(sli)) { RtlStateStreamFail(sli);return; }
    uint32_t header[5]={0};
    sli->func(sli,header,sizeof(header));
    bool header_picking=header[1]==1 || header[1]==3;
    if(header[0]!=0x32434e42 || (header[1]>3 && header[1]!=6) || header[2]<1 || header[2]>5 ||
       (header[4]!=0 && header[4]!=RESUME_CAP) || header[3]>header[4] ||
       (header_picking!=(header[3]>0))) {
        RtlStateStreamFail(sli);return;
    }
    picking=header_picking;from_continue=header[1]==3;chosen=header[1]==2 || header[1]==6;restarting=header[1]==6;players=header[2];
    if(header[3]) {
        resume=malloc(header[3]);
        if(!resume) { RtlStateStreamFail(sli);return; }
        resume_size=header[3];sli->func(sli,resume,resume_size);
    }
    uint8_t padding[4096];
    for(uint32_t at=resume_size;at<header[4];) {
        uint32_t n=header[4]-at;if(n>sizeof(padding)) n=sizeof(padding);
        sli->func(sli,padding,n);at+=n;
    }
}
void NormalCharacterLoaded(uint32_t version) {
    (void)version;
    if(loaded) RtlApplyExecutionState();
    else NormalCharacterReset(); /* Historical states have no extension. */
    loaded=false;
    previous_pad=RtlGetPadState(0);
}
