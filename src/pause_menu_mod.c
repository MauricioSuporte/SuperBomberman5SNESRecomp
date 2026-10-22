/* Optional pause actions. Cartridge routines still own freezing and scene teardown. */
#include "pause_menu_mod.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "mod_runtime.h"
#include "normal_coop_mod.h"
#include "ptbr_mod.h"
#include "snes/interp_bridge.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
#include <string.h>

static bool enabled, installed, open_menu, confirm_menu, resume_requested, action_requested;
static uint8_t selection, sound_pending, returning;
static uint16_t previous[5];
static const uint16_t campaign_fields[]={0x16c2,0x16da,0x1662,0x167a,0x1782,0x15d2,0x15ea,0x17ca,0x4ebd,0x4ed5};
static uint8_t campaign_values[10][5],campaign_lives;
static bool keep_campaign;
static void (*previous_hook)(uint32_t, int, int);
static uint8_t original_test[4], original_space[27];
static uint8_t *rom(uint32_t pc) { return SnesRomPtr(pc); }
static bool battle(void) { return (g_ram[0x1d1a]&1)!=0; }
static bool stage(void) {
    return (g_ram[0xaee]&0x7f) && !(g_ram[0x1d1a]&8) &&
        !(g_ram[0x1d1e]&7) && (battle() || (g_ram[0x1031]>=13 && g_ram[0x1031]<116));
}
static void action(void);
static void instruction(uint32_t pc,int m,int x) {
    if(previous_hook) previous_hook(pc,m,x);
    if(!enabled) return;
    if(pc==0xc500b7) {
        rom(0xc57185)[0]=sound_pending;
        sound_pending=0;
    }
    if(pc==0xc500c8 && open_menu) {
        if(action_requested) { action_requested=false;action(); }
        g_ram[0xb1]=resume_requested?1:0;
        resume_requested=false;
    }
    if(pc==0xc428ce && keep_campaign) {
        for(unsigned f=0;f<10;++f) memcpy(g_ram+campaign_fields[f],campaign_values[f],5);
        g_ram[0x1d23]=campaign_lives;
    }
    /* Normal's native menu actor chooses its entry state at $C7:0B8B
     * (or Continue at $0B97). State 4 runs the existing map builder and
     * selector, bypassing the file dialogs without awarding a stage clear. */
    if(pc==0xc21aec && g_ram[0xbd]==0x36 && g_ram[0xbe]==0x10 &&
       g_ram[0x0c]==0xc7 && g_ram[0x0b]==0x0b &&
       (g_ram[0x0a]==0x8e || g_ram[0x0a]==0x9a)) {
        /* A New Game or Continue chosen in the file menu (map -> file
         * select, or title) starts a fresh campaign: drop the lives and
         * fields kept for the world-map return. */
        if(returning) { g_ram[0x373]=4;returning=0; }
        else keep_campaign=false;
    }
}
void PauseMenuReset(void) {
    if(installed) {
        memcpy(rom(0xc500b7),original_test,4);
        memcpy(rom(0xc57180),original_space,sizeof(original_space));
        if(g_interp_bridge_pc_hook==instruction) g_interp_bridge_pc_hook=previous_hook;
    }
    installed=false;previous_hook=NULL;
    open_menu=confirm_menu=resume_requested=action_requested=false;
    selection=sound_pending=returning=0;memset(previous,0,sizeof(previous));
    keep_campaign=false;campaign_lives=0;memset(campaign_values,0,sizeof(campaign_values));
}
static void reset_plugin(void) { PauseMenuReset();enabled=false; }
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_pause_menu) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.pause.menu",activate);
}
static void install(void) {
    const uint8_t test_signature[]={0x64,0xb1,0xa0,0x04};
    const uint8_t resume_signature[]={0x22,0xed,0x0c,0xc0};
    if(memcmp(rom(0xc500b7),test_signature,4) || memcmp(rom(0xc50096),resume_signature,4)) return;
    for(unsigned i=0;i<sizeof(original_space);++i) if(rom(0xc57180)[i]!=0xff) return;
    /* Preserve A/X/Y/P around the native menu SFX command (group 2). */
    const uint8_t code[]={0x08,0x48,0xda,0x5a,0xa9,0x00,0xf0,0x08,
        0xeb,0xa9,0x02,0xeb,0x22,0xed,0x0c,0xc0,
        0x7a,0xfa,0x68,0x28,0x64,0xb1,0xa0,0x04,0x4c,0xbb,0x00};
    /* Keep both native pause/resume audio commands. The group-2 command
     * at $C5:0096 enters pause; $C5:0037 leaves it. */
    memcpy(original_test,rom(0xc500b7),4);
    memcpy(original_space,rom(0xc57180),sizeof(original_space));
    memcpy(rom(0xc57180),code,sizeof(code));
    rom(0xc500b7)[0]=0x5c;rom(0xc500b7)[1]=0x80;
    rom(0xc500b7)[2]=0x71;rom(0xc500b7)[3]=0xc5;
    previous_hook=g_interp_bridge_pc_hook;g_interp_bridge_pc_hook=instruction;installed=true;
}
static void action(void) {
    if(battle() && selection==1) {
        /* The round resolver counts these statuses. Zero survivors takes its
         * existing Draw branch, with no fabricated winner or victory count. */
        for(unsigned i=0;i<5;++i) {
            g_ram[0x15a2+i]|=0x80;
            g_ram[0x1ba3+i]&=(uint8_t)~2u; /* Revenge riders also count as survivors. */
        }
        resume_requested=true;
    } else {
        /* Native stage teardown observes bit 2, then rebuilds the requested
         * scene. Normal uses its existing map entry state. */
        if(!battle()) {
            NormalCoopPrepareWorldMapReturn();
            for(unsigned f=0;f<10;++f) memcpy(campaign_values[f],g_ram+campaign_fields[f],5);
            campaign_lives=g_ram[0x1d23];keep_campaign=true;
            /* The success path marks $102F+3 only when bit 6 is set.
             * Leaving voluntarily must never award that completion flag. */
            g_ram[0x1d1a]&=(uint8_t)~0x40u;
            g_ram[0x1046]=g_ram[0x1031];
            g_ram[0x1031]=3;returning=1;
        }
        else { g_ram[0x35b2]=1;g_ram[0x1031]=4; }
        g_ram[0x1d1d]=0;g_ram[0x1d1e]|=4;
        open_menu=false;
    }
}
void PauseMenuBeforeFrame(void) {
    if(!enabled) return;
    if(!installed) install();
    if(!installed) return;
    if(returning && g_ram[0x1031]!=3) returning=0;
    uint16_t pressed=0;
    for(unsigned i=0;i<5;++i) {
        uint16_t pad=RtlGetPadState((int)i);
        pressed|=pad & ~previous[i];previous[i]=pad;
    }
    if(!open_menu || !g_ram[0x1d1d]) return;
    unsigned count=confirm_menu?2:(battle()?3:2);
    if(pressed&1) {
        sound_pending=3;
        if(confirm_menu) { confirm_menu=false;selection=battle()?2:1; }
        else { resume_requested=true;sound_pending=0; }
    } else if(pressed&0x30) {
        selection=(uint8_t)((selection+((pressed&0x10)?count-1:1))%count);sound_pending=1;
    } else if(pressed&8) {
        sound_pending=4;
        if(confirm_menu) {
            if(selection==1) { selection=battle()?2:1;action_requested=true; }
            else { confirm_menu=false;selection=battle()?2:1; }
        } else if(selection==0) { resume_requested=true;sound_pending=0; }
        else if(battle() && selection==1) action_requested=true;
        else { confirm_menu=true;selection=0; }
    }
}
void PauseMenuAfterFrame(void) {
    if(!enabled || !installed) return;
    if(keep_campaign && stage()) keep_campaign=false;
    if(keep_campaign && g_ram[0x1031]<=2) keep_campaign=false; /* back at the title */
    if(!g_ram[0x1d1d]) { open_menu=false;confirm_menu=false;selection=0; }
    else if(!open_menu && stage()) { open_menu=true;confirm_menu=false;selection=0; }
}
bool PauseMenuEnabled(void) { return enabled; }

/* The same authored alphabet used by the translation overlay. */
static const uint8_t kFont[26][7] = {
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};

typedef struct { uint16_t at,value; } Saved;
static Saved saved[1600];static unsigned saved_count;
static uint8_t screens[2],windows[2],color_select,color_add;
static unsigned glyph_first,panel_top,panel_bottom;
static uint16_t fixed_color;
static uint32_t window_select;
static uint8_t window_left,window_right;
static bool blend_panel;
static bool drawn;
static void write_word(Ppu *p,unsigned at,uint16_t value) {
    at&=0x7fff;saved[saved_count++]=(Saved){(uint16_t)at,p->vram[at]};p->vram[at]=value;
}
static void text(Ppu *p,unsigned row,unsigned col,const char *s) {
    unsigned map=PPU_bgTilemapAdr(p,2);
    for(;*s;++s,++col) {
        unsigned glyph=*s>='A'&&*s<='Z'?*s-'A'+1:0;
        if((unsigned char)s[0]==0xc3 && (unsigned char)s[1]==0x83) { glyph=39;++s; }
        write_word(p,map+row*32+col,0x2000+glyph_first+glyph);
    }
}
static void restore_panel_math(Ppu *p) {
    p->cgwsel=color_select;p->cgadsub=color_add;p->fixedColor=fixed_color;
    p->windowsel=window_select;p->window1left=window_left;p->window1right=window_right;
}
void PauseMenuObserve(const struct Ppu *view,unsigned line,void *context) {
    (void)context;Ppu *p=(Ppu *)view;
    if(line==224 && drawn) {
        while(saved_count) { Saved w=saved[--saved_count];p->vram[w.at]=w.value; }
        memcpy(p->screenEnabled,screens,2);memcpy(p->screenWindowed,windows,2);
        restore_panel_math(p);
        ++p->vramWriteCount;drawn=false;return;
    }
    if(drawn && blend_panel) {
        if(line==panel_top*8) {
            /* Dim only the field inside the panel. BG3 text/art stays opaque. */
            p->windowsel=(window_select&0x000fffff)|0x00200000;
            p->window1left=(battle()?2:3)*8;p->window1right=(battle()?30:29)*8-1;
            p->fixedColor=0;p->cgwsel=0x10;p->cgadsub=0x7b;
        } else if(line==(panel_bottom+1)*8) {
            restore_panel_math(p);
        }
    }
    if(line!=1 || !open_menu || !enabled || PPU_mode(p)!=1) return;
    /* Stay inside the BG3 character region, before the live OBJ pages. */
    unsigned chars=PPU_bgTileAdr(p,2),map=PPU_bgTilemapAdr(p,2);
    if(chars!=0x5000 || PPU_objTileAdr1(p)!=0x6000) return;
    unsigned words=1024u << (PPU_bgTilemapWider(p,2)+PPU_bgTilemapHigher(p,2));
    bool used[512]={0};
    for(unsigned i=0;i<words;++i) {
        unsigned tile=p->vram[(map+i)&0x7fff]&1023;
        if(tile<512) used[tile]=true;
    }
    glyph_first=0;
    for(unsigned candidate=256;candidate+40<=512;++candidate) {
        bool free=true;
        for(unsigned i=0;i<40;++i) if(used[candidate+i]) { free=false;break; }
        if(free) { glyph_first=candidate;break; }
    }
    if(!glyph_first) return;
    drawn=true;saved_count=0;memcpy(screens,p->screenEnabled,2);memcpy(windows,p->screenWindowed,2);
    color_select=p->cgwsel;color_add=p->cgadsub;
    fixed_color=p->fixedColor;window_select=p->windowsel;
    window_left=p->window1left;window_right=p->window1right;
    blend_panel=color_select==0 && color_add==0 && !(window_select&0x000aaaaa);
    panel_top=battle()?13:10;panel_bottom=confirm_menu || battle()?24:22;
    p->screenEnabled[0]|=4;p->screenWindowed[0]&=(uint8_t)~4u;
    unsigned base=chars+glyph_first*8;
    for(unsigned c=0;c<40;++c) for(unsigned y=0;y<8;++y) {
        unsigned bits=c>=1&&c<=26&&y<7?kFont[c-1][y]<<2:0;
        if(c==39) { static const uint8_t a_tilde[8]={10,5,14,17,31,17,17,0};bits=a_tilde[y]<<2; }
        /* Opaque dark fill, white glyphs. */
        write_word(p,base+c*8+y,(uint16_t)((blend_panel?bits:0xff)|(bits<<8)));
    }
    /* Authored glove: cuff at the left and index finger pointing right. */
    const char *hand[16]={
        "     ###        ","    #WWW#       ","    #WWW########","    #WWWWWWWWWW#",
        "  ###WWW########"," #WWWWWWW#      "," #WWWWWWW###    ","##WWWWWWWWWW#   ",
        "#WWWWWWWWWWW#   ","###WWWWWWWWW#   ","#W#WWWWWWWWW#   ","#W#WWWWWWWW#    ",
        "#W##WWWWWW#     ","### ######      ","                ","                "};
    for(unsigned ty=0;ty<2;++ty) for(unsigned tx=0;tx<2;++tx) for(unsigned y=0;y<8;++y) {
        uint16_t word=0;
        for(unsigned x=0;x<8;++x) {
            char ch=hand[ty*8+y][tx*8+x];unsigned c=ch=='W'?3:ch=='#'?2:blend_panel?0:1;
            if(c&1) word|=1u<<(7-x);
            if(c&2) word|=1u<<(15-x);
        }
        write_word(p,base+(27+ty*2+tx)*8+y,word);
    }
    /* Thin authored frame, using the existing cartridge's gold palette. */
    for(unsigned edge=0;edge<8;++edge) for(unsigned y=0;y<8;++y) {
        uint16_t word=0;
        for(unsigned x=0;x<8;++x) {
            bool top=edge<=2,bottom=edge>=5,left=edge==0 || edge==3 || edge==5;
            bool right=edge==2 || edge==4 || edge==7;
            unsigned c=(top&&y==1)||(bottom&&y==6)||(left&&x==1)||(right&&x==6)?2:
                       (top&&y==2)||(bottom&&y==5)||(left&&x==2)||(right&&x==5)?3:0;
            if(c&1) word|=1u<<(7-x);
            if(c&2) word|=1u<<(15-x);
        }
        write_word(p,base+(31+edge)*8+y,word);
    }
    unsigned left=battle()?2:3,right=battle()?29:28;
    for(unsigned row=panel_top;row<=panel_bottom;++row) for(unsigned col=left;col<=right;++col) {
        unsigned at=map+row*32+col,tile=p->vram[at]&1023;
        if(row==panel_top || row==panel_bottom || col==left || col==right) {
            unsigned edge=row==panel_top?(col==left?0:col==right?2:1):
                          row==panel_bottom?(col==left?5:col==right?7:6):col==left?3:4;
            write_word(p,at,0x3800+glyph_first+31+edge);
        } else if(!((row==11 || row==12 || row==14 || row==15) && tile && tile!=0x80)) {
            write_word(p,at,0x2000+glyph_first);
        }
    }
    /* Preserve the cartridge's stage heading and PAUSE artwork above the rows. */
    unsigned label_col=battle()?7:8,cursor_col=label_col-3;
    if(confirm_menu) {
        text(p,17,10,PtbrPauseLabel(0));text(p,19,16,PtbrPauseLabel(1));text(p,21,16,PtbrPauseLabel(2));
        label_col=16;cursor_col=13;
    } else {
        text(p,17,label_col,PtbrPauseLabel(3));
        text(p,19,label_col,PtbrPauseLabel(battle()?4:5));
        if(battle()) text(p,21,label_col,PtbrPauseLabel(6));
    }
    unsigned cursor_row=(confirm_menu?19:17)+selection*2;
    for(unsigned y=0;y<2;++y) for(unsigned x=0;x<2;++x)
        write_word(p,map+(cursor_row+y)*32+cursor_col+x,0x2000+glyph_first+27+y*2+x);
    ++p->vramWriteCount;
}
void PauseMenuSave(struct SaveLoadInfo *sli) {
    uint32_t magic=0x31534d50;uint8_t state[8]={open_menu,confirm_menu,resume_requested,selection,sound_pending,returning,action_requested,keep_campaign};
    sli->func(sli,&magic,4);sli->func(sli,state,8);sli->func(sli,previous,sizeof(previous));
    sli->func(sli,campaign_values,sizeof(campaign_values));sli->func(sli,&campaign_lives,1);
}
void PauseMenuLoad(struct SaveLoadInfo *sli,unsigned version) {
    (void)version;open_menu=confirm_menu=resume_requested=action_requested=false;selection=sound_pending=returning=0;memset(previous,0,sizeof(previous));
    keep_campaign=false;campaign_lives=0;memset(campaign_values,0,sizeof(campaign_values));
    if(!RtlStateBytesRemaining(sli)) return;
    uint32_t magic=0;uint8_t state[8]={0};sli->func(sli,&magic,4);sli->func(sli,state,8);sli->func(sli,previous,sizeof(previous));
    sli->func(sli,campaign_values,sizeof(campaign_values));sli->func(sli,&campaign_lives,1);
    if(magic!=0x31534d50 || state[0]>1 || state[1]>state[0] || state[2]>1 || state[3]>2 ||
       (state[1]&&state[3]>1) || state[4]>4 || state[5]>1 || state[6]>1 || state[7]>1) { RtlStateStreamFail(sli);return; }
    for(unsigned i=0;i<5;++i) if(campaign_values[0][i]>9 || campaign_values[1][i]>0x7f) {
        RtlStateStreamFail(sli);return;
    }
    open_menu=state[0];confirm_menu=state[1];resume_requested=state[2];selection=state[3];sound_pending=state[4];returning=state[5];action_requested=state[6];keep_campaign=state[7];
}
