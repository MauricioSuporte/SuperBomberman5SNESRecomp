/* Native Battle character picker, temporarily entered from Normal's file
 * confirmation. All graphics come from the player's verified cartridge. */
#include "normal_character_mod.h"
#include "honey_kotetsu_mod.h"
#include "normal_coop_mod.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "mod_runtime.h"
#include "snes/snes.h"
#include "snes/saveload.h"
#include <stdlib.h>
#include <string.h>

enum { SNAP_CAP = 1024*1024, RESUME_CAP = 384*1024 };
static bool enabled, picking, picker_ready, chosen, loaded;
static bool internal_snapshot;
static bool from_continue;   /* picker opened from Continue -> File */
static unsigned players;
static uint8_t *resume;
static uint32_t resume_size;
static uint16_t previous_pad;

static uint32_t picker_pc(void) {
    /* Script task 9. These are three parallel arrays, not CPU registers. */
    return g_ram[0xc90]|((uint32_t)g_ram[0xcb8]<<8)|((uint32_t)g_ram[0xce0]<<16);
}
static void set_picker_pc(uint32_t pc) {
    g_ram[0xc90]=(uint8_t)pc;g_ram[0xcb8]=(uint8_t)(pc>>8);g_ram[0xce0]=(uint8_t)(pc>>16);
}
void NormalCharacterReset(void) {
    free(resume);resume=NULL;resume_size=0;
    picking=picker_ready=chosen=loaded=from_continue=false;players=1;previous_pad=0;
}
static void reset_plugin(void) { NormalCharacterReset();enabled=false; }
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_normal_characters) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.normal.characters",activate);
}

static void finish_picker(bool cancel) {
    uint8_t choices[5],palettes[5];
    memcpy(choices,g_ram+0x625f,sizeof(choices));
    memcpy(palettes,g_ram+0x6264,sizeof(palettes));
    unsigned count=players;
    /* Detach before loading: the save extension replaces its own buffer. */
    uint8_t *snapshot=resume;uint32_t size=resume_size;
    resume=NULL;resume_size=0;
    bool ok=RtlLoadSnapshotFromMemory(snapshot,size);
    free(snapshot);
    picking=false;
    if (!ok) return;
    players=count;
    if (cancel) {
        chosen=false;
        if(from_continue) {
            /* Resume the Continue script right after its menu fade-out, where
             * it builds and fades in the file list ($C7:0D39, called from
             * $C7:0BE4 with inner return $C7:0D37). */
            set_picker_pc(0xc70d39);
            g_ram[0x2003]=0xe4;g_ram[0x2004]=0x0b;g_ram[0x2005]=0xc7;
            g_ram[0x2006]=0x37;g_ram[0x2007]=0x0d;g_ram[0x2008]=0xc7;
        }
        else {
            set_picker_pc(0xc71014); /* Initialize and rebuild the Normal file menu. */
            g_ram[0x2006]=0xc7;g_ram[0x2007]=0x10;g_ram[0x2008]=0xc7;
        }
    } else {
        chosen=true;
        for(unsigned i=0;i<count;++i) {
            /* 10: Subordinate mod; 11/12: Honey/Kotetsu; 13-18: Bomber Kings
             * (all extra characters play as 1). */
            unsigned choice=choices[i]>=1 && choices[i]<=EXTRA_CHOICE_LAST?choices[i]:1;
            HoneyKotetsuSetSeatChoice(i,choice);
            g_ram[0x625f+i]=(uint8_t)HoneyKotetsuBaseChoice(choice);
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
    /* Consume Back before the native picker changes its script task. */
    uint16_t pad=RtlGetPadState(0);
    if(enabled && picking && picker_pc()==0xc68e68 &&
       ((pad & ~previous_pad)&0x0001)) finish_picker(true); /* runtime B bit */
}

void NormalCharacterFrame(void) {
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
    if (g_ram[0x1031]!=3) { chosen=false;return; }
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
        players=NormalCoopPlayerCount();
        picking=true;
        set_picker_pc(0xc689cf);
        g_ram[0x1031]=4;
        for(unsigned i=0;i<5;++i) g_ram[0x6269+i]=i<players?1:2;
        g_ram[0x1038]=0;
    }
}

/* Rewind and save states include the suspended Normal machine, so loading
 * inside the picker is as safe as loading during a stage. No SRAM changes. */
void NormalCharacterSave(SaveLoadInfo *sli) {
    RtlSaveExecutionState(sli);
    uint32_t capacity=internal_snapshot?0:RESUME_CAP;
    /* State 3: picking, opened from Continue (cancel rebuilds that list). */
    uint32_t header[5]={0x32434e42, picking?(from_continue?3u:1u):chosen?2u:0u,players,resume_size,capacity};
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
    if(header[0]!=0x32434e42 || header[1]>3 || header[2]<1 || header[2]>5 ||
       (header[4]!=0 && header[4]!=RESUME_CAP) || header[3]>header[4] ||
       (header_picking!=(header[3]>0))) {
        RtlStateStreamFail(sli);return;
    }
    picking=header_picking;from_continue=header[1]==3;chosen=header[1]==2;players=header[2];
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
