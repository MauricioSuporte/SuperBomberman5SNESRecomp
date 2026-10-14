/* Native Battle character picker, temporarily entered from Normal's file
 * confirmation. All graphics come from the player's verified cartridge. */
#include "normal_character_mod.h"
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
    picking=picker_ready=chosen=loaded=false;players=1;previous_pad=0;
}
static void reset_plugin(void) { NormalCharacterReset();enabled=false; }
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_normal_characters) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.normal.characters",activate);
}

static void finish_picker(bool cancel) {
    uint8_t choices[2]={g_ram[0x625f],g_ram[0x6260]};
    uint8_t palettes[2]={g_ram[0x6264],g_ram[0x6265]};
    unsigned count=players;
    /* Detach before loading: the save extension replaces its own buffer. */
    uint8_t *snapshot=resume;uint32_t size=resume_size;
    resume=NULL;resume_size=0;
    bool ok=RtlLoadSnapshotFromMemory(snapshot,size);
    free(snapshot);
    picking=false;
    if (!ok) return;
    if (cancel) {
        chosen=false;
        set_picker_pc(0xc71014); /* Initialize and rebuild the Normal file menu. */
        g_ram[0x2006]=0xc7;g_ram[0x2007]=0x10;g_ram[0x2008]=0xc7;
    } else {
        chosen=true;
        for(unsigned i=0;i<count;++i) {
            g_ram[0x625f+i]=choices[i]>=1 && choices[i]<=9?choices[i]:1;
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
    if (!chosen && pc==0xc71257 &&
        g_ram[0x2003]==0xff && g_ram[0x2004]==0x0b && g_ram[0x2005]==0xc7 &&
        g_ram[0x2006]==0xe6 && g_ram[0x2007]==0x10 && g_ram[0x2008]==0xc7) {
        uint8_t *snapshot=malloc(SNAP_CAP);
        if (!snapshot) return;
        internal_snapshot=true;
        size_t size=RtlSaveSnapshotToMemory(snapshot,SNAP_CAP);
        internal_snapshot=false;
        if (!size || size>RESUME_CAP) { free(snapshot);return; }
        resume=snapshot;resume_size=(uint32_t)size;
        players=(g_ram[0x1d25]&3)==3?2:1;
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
    uint32_t header[5]={0x32434e42, picking?1u:chosen?2u:0u,players,resume_size,capacity};
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
    if(header[0]!=0x32434e42 || header[1]>2 || header[2]<1 || header[2]>2 ||
       (header[4]!=0 && header[4]!=RESUME_CAP) || header[3]>header[4] ||
       ((header[1]==1)!=(header[3]>0))) {
        RtlStateStreamFail(sli);return;
    }
    picking=header[1]==1;chosen=header[1]==2;players=header[2];
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
