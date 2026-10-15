/* Five-seat campaign adapter for the verified JPN HiROM revision.
 * Guest routines still allocate actors, load graphics and run player input.
 * Trampolines below are authored instructions, installed only in the in-memory
 * cartridge. The cartridge file and the original SRAM layout are preserved. */
#include "normal_coop_mod.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "mod_runtime.h"
#include "snes/interp_bridge.h"
#include "snes/snes.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
#include <string.h>
#include <stdlib.h>

static bool enabled, installed;
static bool player_menu;
static uint8_t suppressed_mask,block_start_mask;
static uint16_t raw_seats[5];
static uint16_t raw_pad, previous_pad;
static uint16_t previous_seats[5];
static uint8_t rejoin_pressed;
static unsigned count=1;
static uint8_t active_mask=1;
static uint8_t revive_mask;
static bool preserve_characters;
static uint8_t campaign_characters[5],campaign_palettes[5];
/* The boss creator at $C4:3CCF overwrites the bomber fields of the slot it
 * takes. Seats sitting out a boss stage keep their character and items here
 * until the next regular stage spawns them again. */
static const uint16_t seat_fields[]={0x16c2,0x16da,0x1662,0x167a,0x1782,0x15d2,0x15ea,0x17ca,0x4ebd,0x4ed5};
#define SEAT_FIELDS (sizeof(seat_fields)/sizeof(*seat_fields))
static uint8_t benched_mask,benched[5][SEAT_FIELDS];
static bool campaign(void);
static void (*previous_hook)(uint32_t,int,int);
typedef struct { uint32_t pc; unsigned size; uint8_t original[128]; } Patch;
static Patch patches[64];
static unsigned patch_count;

static uint8_t *rom(uint32_t pc) { return SnesRomPtr(pc); }
static void reserve(uint32_t pc,unsigned size) {
    Patch *p=&patches[patch_count++];p->pc=pc;p->size=size;
    memcpy(p->original,rom(pc),size);
}
static void restore(uint32_t pc) {
    for(unsigned i=0;i<patch_count;++i) if(patches[i].pc==pc)
        memcpy(rom(pc),patches[i].original,patches[i].size);
}
static void jump(uint32_t pc,uint32_t target) {
    uint8_t *p=rom(pc);p[0]=0x4c;p[1]=(uint8_t)target;p[2]=(uint8_t)(target>>8);
}
unsigned NormalCoopPlayerCount(void) {
    if(enabled && (campaign() || NormalCoopPlayerMenu())) return count;
    return (g_ram[0x1d25]&3)==3?2:1;
}
/* Mode 3 is the confirmation fade-out: the title layout stays on screen. */
bool NormalCoopPlayerMenu(void) {
    return enabled && player_menu && (g_ram[0x1031]==2 || g_ram[0x1031]==3);
}
/* Script return slots retain attract-mode history. Identify the native
 * player menu before the render-only overlays replace these tile words. */
static bool native_player_menu(void) {
    const Ppu *ppu=g_snes ? g_snes->ppu : NULL;
    return ppu && (g_ram[0x1031]==2 || g_ram[0x1031]==3) && (ppu->bgmode&7)==1 &&
        PPU_bgTilemapAdr(ppu,2)==0x4000 &&
        (ppu->vram[0x424b]&0x3ff)==302 &&
        (ppu->vram[0x424c]&0x3ff)==261 &&
        (ppu->vram[0x428b]&0x3ff)==278 &&
        (ppu->vram[0x428c]&0x3ff)==261;
}
/* Collision maps are sixteen columns wide. $1052 holds static collision;
 * $1152 holds dynamic collision flags. Require an exit as well as an empty tile. */
static bool floor_at(int x,int y) {
    if(x<1 || x>13 || y<1 || y>11) return false;
    unsigned tile=(unsigned)(y*16+x);
    return !(g_ram[0x1052+tile]&0xe0) && !(g_ram[0x1152+tile]&0xc0);
}
static bool occupied(unsigned tile,unsigned player) {
    for(unsigned i=0;i<count;++i) if(i!=player &&
        !(g_ram[0x17fa+i]&0x80) && !(g_ram[0x15a2+i]&0x80)) {
        unsigned cell=g_ram[0x16aa+i];
        /* Equipment reset clears the collision cache during rejoin. The
         * actor's position still identifies its occupied tile. */
        if(!cell) {
            unsigned object=g_ram[0x17fa+i];
            unsigned x=(uint8_t)(g_ram[0x055f+object]+g_ram[0x1d3b]);
            unsigned y=(uint8_t)(g_ram[0x05bf+object]+g_ram[0x1d3d]);
            cell=((y+8)>>4)*16+((x+8)>>4);
        }
        if(cell==tile) return true;
    }
    return false;
}
static bool safe_at(int x,int y,unsigned player) {
    return floor_at(x,y) && !occupied((unsigned)(y*16+x),player) &&
        (floor_at(x-1,y)||floor_at(x+1,y)||floor_at(x,y-1)||floor_at(x,y+1));
}
static int nearest(int x,int y,unsigned player) {
    int best=-1,distance=1000;
    for(int row=1;row<=11;++row) for(int col=1;col<=13;++col) {
        int d=abs(x-col)+abs(y-row);
        if(d<distance && safe_at(col,row,player)) { best=row*16+col;distance=d; }
    }
    return best;
}
static int spawn_tile(unsigned player,bool retry) {
    int x,y;
    if(retry) { x=g_ram[0x4e8d+player];y=g_ram[0x4ea5+player]; }
    else if(g_ram[0x1d1a]&0x0c) {
        int p1x=g_ram[0x4e8d],p2x=g_ram[0x4e8e];
        x=player==2?p1x-2:player==3?p2x+2:(p1x+p2x)/2;
        y=g_ram[0x4ea5];
    } else { x=player==2?1:player==3?13:7;y=player==4?6:11; }
    return nearest(x,y,player);
}
static void position(unsigned player,bool retry) {
    int tile=spawn_tile(player,retry);
    if(tile<0) { g_ram[0x15a2+player]=0x80;return; }
    g_ram[0x1d28]=(uint8_t)(tile&15);g_ram[0x1d2a]=(uint8_t)(tile>>4);
    g_ram[0x4e8d+player]=g_ram[0x1d28];g_ram[0x4ea5+player]=g_ram[0x1d2a];
    g_ram[0x15a2+player]=1;
}
/* Campaign spawns run before $C5:3D3F copies the stage's collision map, so the
 * search above sees an empty map and P3-P5 could start inside walls or outside
 * smaller arenas (Zone 2-4, Zone 3-D/E). Once the map exists, move a seat
 * still standing on an unsafe tile to the nearest safe floor. */
static bool spawn_check;
static void add16(uint16_t address,int delta) {
    unsigned value=(unsigned)(g_ram[address]|g_ram[address+1]<<8)+(unsigned)delta;
    g_ram[address]=(uint8_t)value;g_ram[address+1]=(uint8_t)(value>>8);
}
static void settle(unsigned player) {
    unsigned object=g_ram[0x17fa+player],cell=g_ram[0x16aa+player];
    if((object&0x80) || (g_ram[0x15a2+player]&0x80) || !cell) return;
    int x=(int)(cell&15),y=(int)(cell>>4);
    if(safe_at(x,y,player)) return;
    int tile=spawn_tile(player,false);
    if(tile<0) return;
    int dx=16*((tile&15)-x),dy=16*((tile>>4)-y);
    g_ram[0x16aa+player]=(uint8_t)tile;
    g_ram[0x4e8d+player]=(uint8_t)(tile&15);g_ram[0x4ea5+player]=(uint8_t)(tile>>4);
    g_ram[0x182a+player]=(uint8_t)(g_ram[0x182a+player]+dx);
    g_ram[0x1872+player]=(uint8_t)(g_ram[0x1872+player]+dy);
    add16((uint16_t)(0x055f+object),dx);add16((uint16_t)(0x05bf+object),dy);
}
static void emit(uint8_t **p,uint8_t value) { *(*p)++=value; }
static void call(uint8_t **p,uint32_t pc) {
    emit(p,0x22);emit(p,(uint8_t)pc);emit(p,(uint8_t)(pc>>8));emit(p,(uint8_t)(pc>>16));
}
static bool alive(unsigned i) {
    return !(g_ram[0x17fa+i]&0x80) && !(g_ram[0x15a2+i]&0x80);
}
static bool campaign(void) {
    unsigned mode=g_ram[0x1031];
    return (mode==3 || mode==8 || (mode>=0x0d && mode<0x74)) && !(g_ram[0x1d1a]&1);
}
/* Boss stages $67-$6E script their rival bomber into actor slot 2 ($6A adds
 * slot 3) through the actor-creation ops at $C2:16B8/$C2:16EE. Bomber tables
 * such as $0FEA and $16C2 hold five slots, so the rival moves past the
 * players and the stage roster is trimmed only when five cannot hold both. */
static unsigned stage_bosses(void) {
    unsigned stage=g_ram[0x1031];
    return stage<0x67 || stage>0x6e ? 0 : stage==0x6a ? 2 : 1;
}
static unsigned seats(void) {
    unsigned limit=5-stage_bosses();
    return count<limit?count:limit;
}
static uint8_t stage_mask(void) {
    uint8_t mask=(uint8_t)(active_mask&((1u<<seats())-1));
    return mask?mask:1;
}
static const uint8_t bomber_slot[]={
    0xa7,0x0d,        /* 4340 LDA [$0D]   1-based actor slot operand */
    0x3a,             /* 4342 DEC A                                 */
    0xc9,0x02,        /* 4343 CMP #$02    P1/P2 keep their slots     */
    0x90,0x03,        /* 4345 BCC $434A                             */
    0x18,             /* 4347 CLC                                   */
    0x69,0x00,        /* 4348 ADC #seats-2                          */
    0xaa,             /* 434A TAX                                   */
    0x60,             /* 434B RTS                                   */
};
static const uint32_t bomber_ops[]={0xc216b8,0xc216ee};
/* Boss choreography at $C5:2600-$C5:39FF names the rival by literal slot:
 * LDX #2/#3 before the actor helpers $C4:3C24/$C4:3BDE, LDA #2/#3 before
 * $C4:2B3A/$C4:3BA3, and LDA $17FC/$17FD. Their operands follow the move. */
static uint32_t boss_sites[128];
static uint8_t boss_original[128];
static unsigned boss_site_count;
static void find_boss_sites(void) {
    boss_site_count=0;
    for(uint32_t pc=0xc52600;pc<0xc53a00 && boss_site_count<128;++pc) {
        const uint8_t *b=rom(pc);
        bool hit=false;
        if(b[0]==0xa2 && (b[1]==2 || b[1]==3) && b[2]==0xa9) {
            const uint8_t *call=b[4]==0xa0?b+6:b+4;
            hit=call[0]==0x22 && call[3]==0xc4 &&
                ((call[1]==0x24 && call[2]==0x3c) || (call[1]==0xde && call[2]==0x3b));
        } else if(b[0]==0xa9 && (b[1]==2 || b[1]==3) && b[2]==0x22 && b[5]==0xc4)
            hit=(b[3]==0x3a && b[4]==0x2b) || (b[3]==0xa3 && b[4]==0x3b);
        else if(b[0]==0xad && (b[1]==0xfc || b[1]==0xfd) && b[2]==0x17) hit=true;
        if(hit) { boss_sites[boss_site_count]=pc+1;boss_original[boss_site_count++]=b[1]; }
    }
}
/* Enemy code keeps per-player distances in six interleaved two-entry arrays
 * at $1FF4-$1FFF (dx, dy, dx high, dy high, two direction codes), filled by
 * $C3:46CC for P1/P2 only. Contact damage ($C3:47C9, $C3:4800) and the large
 * enemies' area hits ($C3:1120, $C3:1330, $C3:2C20) loop over those two
 * entries, so P3-P5 never took contact damage. The arrays move to five-entry
 * copies at $1FC0-$1FDD (unused WRAM in every recorded campaign dump), the
 * loops cover the roster, and $C3:FF70 fills the extra seats. The nearest-
 * player chooser at $C3:4641 keeps its two-entry direct-page buffers. */
static uint32_t enemy_sites[64];
static uint8_t enemy_original[64];
static unsigned enemy_site_count;
static const uint32_t enemy_loops[]={0xc3119c,0xc313a9,0xc32cb3,0xc347fb,0xc34817};
static const uint8_t enemy_distances[]={
    0x20,0x5c,0x47,   /* FF70 JSR $475C   clear all distances       */
    0x5a,             /* FF73 PHY                                   */
    0xa0,0x02,        /* FF74 LDY #$02                              */
    0xc0,0x05,        /* FF76 CPY #seats                            */
    0xb0,0x0d,        /* FF78 BCS $FF87                             */
    0xb9,0xa2,0x15,   /* FF7A LDA $15A2,Y                           */
    0x29,0x80,        /* FF7D AND #$80    dead seats stay far away   */
    0xd0,0x03,        /* FF7F BNE $FF84                             */
    0x20,0xcc,0x46,   /* FF81 JSR $46CC   stock per-seat distance    */
    0xc8,             /* FF84 INY                                   */
    0x80,0xef,        /* FF85 BRA $FF76                             */
    0x7a,             /* FF87 PLY                                   */
    0x60,             /* FF88 RTS                                   */
};
static void find_enemy_sites(void) {
    enemy_site_count=0;
    for(uint32_t pc=0xc31100;pc<0xc34900 && enemy_site_count<64;++pc) {
        const uint8_t *b=rom(pc);
        if((b[0]==0xb9 || b[0]==0x99 || b[0]==0x9d) && b[1]>=0xf4 && b[2]==0x1f) {
            enemy_sites[enemy_site_count]=pc+1;enemy_original[enemy_site_count++]=b[1];
        }
    }
}
static const uint32_t mount_backups[]={
    0xc43d3d,0xc43d43,0xc43d54,0xc43d5a,
    0xc43d71,0xc43d77,0xc43d89,0xc43d8f
};
/* Five resident players fill the campaign's five 1 KiB OBJ pages ($1D00,
 * VRAM $6000-$6BFF). The stock allocator needs a spare page before releasing
 * the requester's current page, so every animation change failed and the
 * sprites froze. Authored replacement for $C5:6AF5: use a free page when one
 * exists, otherwise refill the page the requesting object already owns. */
static const uint8_t page_allocator[]={
    0xda,             /* 7100 PHX                                   */
    0xa2,0x00,        /* 7101 LDX #$00                              */
    0xbd,0x00,0x1d,   /* 7103 LDA $1D00,X                           */
    0x30,0x15,        /* 7106 BMI $711D   free page                 */
    0xe8,             /* 7108 INX                                   */
    0xe0,0x10,        /* 7109 CPX #$10                              */
    0x90,0xf6,        /* 710B BCC $7103                             */
    0xa2,0x0f,        /* 710D LDX #$0F                              */
    0x98,             /* 710F TYA         requesting object         */
    0xdd,0x00,0x1d,   /* 7110 CMP $1D00,X                           */
    0xf0,0x0b,        /* 7113 BEQ $7120   page owned by requester   */
    0xca,             /* 7115 DEX                                   */
    0x10,0xf8,        /* 7116 BPL $7110                             */
    0xa0,0x7f,        /* 7118 LDY #$7F    stock failure path        */
    0x4c,0x04,0x6b,   /* 711A JMP $6B04                             */
    0x4c,0x04,0x6b,   /* 711D JMP $6B04   stock success path        */
    0x8a,             /* 7120 TXA                                   */
    0x0a,             /* 7121 ASL A                                 */
    0x09,0x60,        /* 7122 ORA #$60    VRAM page high byte       */
    0x8d,0x11,0x1d,   /* 7124 STA $1D11                             */
    0x18,             /* 7127 CLC                                   */
    0xfa,             /* 7128 PLX                                   */
    0x60,             /* 7129 RTS                                   */
};
/* `CPX #$02` bounds of cartridge loops that visit only P1/P2:
 * $C2:0C9F script op $12 (exit warps and floor switches), $C5:6195/$C5:61C6
 * stage floor gimmicks, $C1:6BA1 input hold on a cell, $C0:373F point damage
 * (players take a direct hit, NPCs lose HP). Each is raised to the roster. */
static const uint32_t seat_loops[]={0xc20c9f,0xc56195,0xc561c6,0xc16ba1,0xc0373f};
static void set_masks(void) {
    uint8_t mask=stage_mask();
    unsigned active=0;for(unsigned i=0;i<seats();++i) active+=(mask>>i)&1;
    g_ram[0x1d25]=mask;g_ram[0x1d24]=active>1?3:1;
}
static void build_spawn(void) {
    uint8_t *p=rom(0xc24300);emit(&p,0xda);emit(&p,0x5a);
    for(unsigned i=0;i<seats();++i) if(stage_mask()&(1u<<i)) {
        emit(&p,0xa2);emit(&p,(uint8_t)i);emit(&p,0xa9);emit(&p,(uint8_t)i);call(&p,0xc42a69);
    }
    emit(&p,0x7a);emit(&p,0xfa);emit(&p,0x60);
}
static void build_rejoin(void) {
    uint8_t *p=rom(0xc57000);
    for(unsigned i=0;i<count;++i) if(revive_mask&(1u<<i)) {
        emit(&p,0xa2);emit(&p,(uint8_t)i);emit(&p,0x8a);call(&p,0xc42a69);
        emit(&p,0x8a);call(&p,0xc428cf);
        emit(&p,0xce);emit(&p,0x23);emit(&p,0x1d);
    }
    emit(&p,0x4c);emit(&p,0x5c);emit(&p,0x06);
}
static void configure(void) {
    bool coop=campaign() && count>2;
    restore(0xc4286f);restore(0xc502ff);restore(0xc5060e);restore(0xc56af5);
    restore(0xc21697);restore(0xc43794);restore(0xc21f2b);restore(0xc30458);
    for(unsigned i=0;i<sizeof(seat_loops)/sizeof(*seat_loops);++i) restore(seat_loops[i]);
    restore(0xc0374d);restore(0xc24340);
    for(unsigned i=0;i<sizeof(bomber_ops)/sizeof(*bomber_ops);++i) restore(bomber_ops[i]);
    for(unsigned i=0;i<boss_site_count;++i) rom(boss_sites[i])[0]=boss_original[i];
    for(unsigned i=0;i<enemy_site_count;++i) rom(enemy_sites[i])[0]=enemy_original[i];
    for(unsigned i=0;i<sizeof(enemy_loops)/sizeof(*enemy_loops);++i) restore(enemy_loops[i]);
    restore(0xc34766);restore(0xc34b8f);restore(0xc3ff70);
    for(unsigned i=0;i<sizeof(mount_backups)/sizeof(*mount_backups);++i) restore(mount_backups[i]);
    if(!coop) { if(campaign()) g_ram[0x1cbf]=2;return; }
    set_masks();g_ram[0x1cbf]=5;
    rom(0xc30458)[1]=10; /* 0–4 players, 5–9 mount companions, 10–23 enemies. */
    for(unsigned i=0;i<sizeof(mount_backups)/sizeof(*mount_backups);++i) rom(mount_backups[i])[1]+=3;
    /* Initialize all selected seats with the cartridge's equipment reset. */
    uint8_t *p=rom(0xc45b00);
    for(unsigned i=0;i<count;++i) {
        emit(&p,0xa9);emit(&p,(uint8_t)i);emit(&p,0x20);emit(&p,0x06);emit(&p,0x29);
    }
    emit(&p,0x4c);emit(&p,0x79);emit(&p,0x28);
    jump(0xc4286f,0xc45b00);
    rom(0xc502ff)[1]=(uint8_t)seats(); /* Normal's living-player loop. */
    /* $C5:066C still selects and loads the one/two-player HUD from the P1/P2
     * state. Its two-seat $1D25/$1D24 values are replaced by set_masks() at
     * the $C5:060E hook before anything else reads them. */
    jump(0xc56af5,0xc57100); /* Sprite page allocator with own-page reuse. */
    jump(0xc5060e,0xc57000);
    /* Script op $12 (exit warps, switches) tests only P1/P2 against the tile,
     * so a warp entered by P3-P5 never finished the stage. */
    for(unsigned i=0;i<sizeof(seat_loops)/sizeof(*seat_loops);++i) rom(seat_loops[i])[1]=(uint8_t)seats();
    for(unsigned i=0;i<enemy_site_count;++i) {
        unsigned slot=(unsigned)(enemy_original[i]-0xf4);
        rom(enemy_sites[i])[0]=(uint8_t)(0xc0+5*(slot/2)+slot%2);
    }
    rom(0xc34766)[0]=30; /* CPX #$0C: clear the six five-entry arrays. */
    for(unsigned i=0;i<sizeof(enemy_loops)/sizeof(*enemy_loops);++i) rom(enemy_loops[i])[0]=(uint8_t)seats();
    memcpy(rom(0xc3ff70),enemy_distances,sizeof(enemy_distances));rom(0xc3ff77)[0]=(uint8_t)seats();
    rom(0xc34b8f)[0]=0x20;rom(0xc34b8f)[1]=0x70;rom(0xc34b8f)[2]=0xff;
    if(stage_bosses()) {
        unsigned shift=seats()-2;
        memcpy(rom(0xc24340),bomber_slot,sizeof(bomber_slot));rom(0xc24349)[0]=(uint8_t)shift;
        for(unsigned i=0;i<sizeof(bomber_ops)/sizeof(*bomber_ops);++i) {
            uint8_t *op=rom(bomber_ops[i]);op[0]=0x20;op[1]=0x40;op[2]=0x43;op[3]=0xea;
        }
        for(unsigned i=0;i<boss_site_count;++i) rom(boss_sites[i])[0]=(uint8_t)(boss_original[i]+shift);
    }
    rom(0xc0374d)[1]=10; /* Point damage: actors 5-9 are mount companions, not enemies. */
    /* Story scripts distinguish one/two-player choreography, not hardware
     * seats. Expose their legacy branch while the adapter owns the roster. */
    rom(0xc21f2b)[1]=0x24;
    jump(0xc21697,0xc24300);
}
static void instruction(uint32_t pc,int m,int x) {
    if(previous_hook) previous_hook(pc,m,x);
    if(!campaign() || count<=2) return;
    if(pc==0xc4286f) {
        if(g_ram[0x1031]==8) {
            preserve_characters=true;
            memcpy(campaign_characters,g_ram+0x16c2,5);memcpy(campaign_palettes,g_ram+0x16da,5);
        }
        configure();
    }
    else if(pc==0xc4289d) g_ram[0x1d23]=(uint8_t)(count+2);
    else if(pc==0xc42adc || pc==0xc42b90) {
        /* The actor allocators request OBJ palette slot 1<<(player+1). Slot 3
         * belongs to campaign enemies, so P3 failed and drew with an unrelated
         * palette. Seats 3-5 use the otherwise idle high slots instead. */
        static const uint8_t extra_slots[]={0x80,0x40,0x20};
        unsigned player=g_ram[0x8d];
        if(player>=2 && player<seats()) g_ram[0x348]=extra_slots[player-2];
    }
    else if(pc==0xc428b4) {
        for(unsigned i=2;i<count;++i) { g_ram[0x16c2+i]=0;g_ram[0x16da+i]=(uint8_t)(g_rom[0xbb4e]+i); }
        if(preserve_characters) {
            memcpy(g_ram+0x16c2,campaign_characters,count);memcpy(g_ram+0x16da,campaign_palettes,count);
        }
        active_mask=(uint8_t)((1u<<count)-1);set_masks();g_ram[0x1cbf]=5;benched_mask=0;
    } else if(pc==0xc21697) {
        /* Continue resets equipment again and may load stock campaign fields.
         * Restore the choices at the final boundary before graphics creation. */
        if(preserve_characters) {
            memcpy(g_ram+0x16c2,campaign_characters,count);memcpy(g_ram+0x16da,campaign_palettes,count);
            preserve_characters=false;
        }
        for(unsigned i=0;i<count;++i) {
            uint8_t bit=(uint8_t)(1u<<i);
            if(i>=seats() && !(benched_mask&bit)) {
                for(unsigned f=0;f<SEAT_FIELDS;++f) benched[i][f]=g_ram[seat_fields[f]+i];
                benched_mask|=bit;
            } else if(i<seats() && (benched_mask&bit)) {
                for(unsigned f=0;f<SEAT_FIELDS;++f) g_ram[seat_fields[f]+i]=benched[i][f];
                benched_mask&=(uint8_t)~bit;
            }
        }
        set_masks();g_ram[0x1cbf]=5;build_spawn();spawn_check=true;
    } else if(pc>=0xc24302 && pc<0xc2432a && (pc-0xc24302)%8==0 && rom(pc)[0]==0xa2) {
        unsigned player=rom(pc)[1];
        if(player>=2) position(player,false);
        else {
            g_ram[0x1d28]=g_ram[0x4e8d+player];g_ram[0x1d2a]=g_ram[0x4ea5+player];
        }
    } else if(pc==0xc53d5d && spawn_check) {
        spawn_check=false;
        for(unsigned i=2;i<seats();++i) settle(i);
    } else if(pc==0xc43794) {
        /* Retain the last participant for the stock stage-retry flow; other
         * dead seats remain absent until Start spends a shared life. */
        unsigned player=g_ram[0x8d];
        if(player<seats() && (active_mask & (active_mask-1))) active_mask&=(uint8_t)~(1u<<player);
        set_masks();
        rom(pc)[0]=0x60;
    } else if(pc==0xc53b1c) {
        /* Stock restart charges P1/P2 below. Include the remaining seats. */
        for(unsigned i=2;i<seats();++i) if(g_ram[0x1d25]&(1u<<i)) --g_ram[0x1d23];
    } else if(pc==0xc3439a) {
        /* Keep the stock two-seat mask in the cartridge's campaign record.
         * Extra seats are reinstated by the stage adapter on Continue. */
        g_ram[0x4da2]=(g_ram[0x4da2]&3)?(g_ram[0x4da2]&3):1;
    } else if(pc==0xc5060e) {
        unsigned live=0;revive_mask=0;
        for(unsigned i=0;i<seats();++i) live+=alive(i);
        unsigned lives=g_ram[0x1d23];
        if(live && lives>0 && lives<128 && !(g_ram[0x1d1e]&0xa7))
            for(unsigned i=0;i<seats() && lives;++i) if((g_ram[0x17fa+i]&0x80) && (rejoin_pressed&(1u<<i))) {
                if(spawn_tile(i,true)<0) continue;
                revive_mask|=(uint8_t)(1u<<i);active_mask|=(uint8_t)(1u<<i);--lives;
            }
        rejoin_pressed=0;
        set_masks();
        build_rejoin();
    } else if(pc>=0xc57002 && pc<0xc57060 && (pc-0xc57002)%15==0) {
        unsigned player=rom(pc)[-1];position(player,true);
    }
}
void NormalCoopReset(void) {
    player_menu=false;
    count=1;previous_pad=0;suppressed_mask=block_start_mask=0;rejoin_pressed=0;
    revive_mask=0;
    active_mask=1;
    preserve_characters=false;memset(campaign_characters,0,5);memset(campaign_palettes,0,5);
    benched_mask=0;memset(benched,0,sizeof(benched));spawn_check=false;
    memset(previous_seats,0,sizeof(previous_seats));
}
static void reset_plugin(void) {
    if(installed) {
        for(unsigned i=0;i<patch_count;++i) restore(patches[i].pc);
        for(unsigned i=0;i<boss_site_count;++i) rom(boss_sites[i])[0]=boss_original[i];
        for(unsigned i=0;i<enemy_site_count;++i) rom(enemy_sites[i])[0]=enemy_original[i];
        if(g_interp_bridge_pc_hook==instruction) g_interp_bridge_pc_hook=previous_hook;
    }
    installed=enabled=false;patch_count=0;boss_site_count=0;enemy_site_count=0;NormalCoopReset();
}
static void activate(void) { enabled=true; }
SNES_MOD_CONSTRUCTOR(register_normal_coop) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.normal.coop",activate);
}
void NormalCoopBeforeFrame(void) {
    if(!enabled) return;
    player_menu=native_player_menu();
    if(!installed && g_rom && g_snes && g_snes->cart) {
        reserve(0xc4286f,3);reserve(0xc502ff,2);reserve(0xc5060e,3);
        reserve(0xc56af5,3);reserve(0xc21697,3);reserve(0xc43794,1);
        find_boss_sites();find_enemy_sites();
        for(unsigned i=0;i<sizeof(enemy_loops)/sizeof(*enemy_loops);++i) reserve(enemy_loops[i],1);
        reserve(0xc34766,1);reserve(0xc34b8f,3);reserve(0xc3ff70,sizeof(enemy_distances));
        reserve(0xc21f2b,3);reserve(0xc0374d,2);reserve(0xc24340,sizeof(bomber_slot));
        for(unsigned i=0;i<sizeof(bomber_ops)/sizeof(*bomber_ops);++i) reserve(bomber_ops[i],4);
        for(unsigned i=0;i<sizeof(seat_loops)/sizeof(*seat_loops);++i) reserve(seat_loops[i],2);
        reserve(0xc30458,2);
        for(unsigned i=0;i<sizeof(mount_backups)/sizeof(*mount_backups);++i) reserve(mount_backups[i],3);
        reserve(0xc45b00,32);reserve(0xc24300,64);reserve(0xc57000,96);
        reserve(0xc57100,sizeof(page_allocator));memcpy(rom(0xc57100),page_allocator,sizeof(page_allocator));
        previous_hook=g_interp_bridge_pc_hook;g_interp_bridge_pc_hook=instruction;installed=true;
        build_spawn();build_rejoin();
    }
    raw_pad=RtlGetPadState(0);
    if(g_ram[0x1031]<=2) preserve_characters=false;
    if(!campaign()) rejoin_pressed=0;
    for(unsigned i=0;i<5;++i) {
        uint16_t buttons=RtlGetPadState((int)i);
        raw_seats[i]=buttons;
        if((buttons & ~previous_seats[i])&8) rejoin_pressed|=(uint8_t)(1u<<i);
        previous_seats[i]=buttons;
        if(!(buttons&8)) block_start_mask&=(uint8_t)~(1u<<i);
        unsigned mode=g_ram[0x1031];
        if(campaign() && count>2 && mode>=0x0d && mode<0x74) {
            /* Seats left out of a boss stage cannot rejoin or pause it. */
            if((buttons&8) && (i>=seats() || !alive(i))) block_start_mask|=(uint8_t)(1u<<i);
            /* A rejoined player is already alive when the stock pause task
             * runs. Hide this Start from the guest until physical release. */
            if(block_start_mask&(1u<<i)) {
                RtlSetPadState((int)i,buttons & ~8u);suppressed_mask|=(uint8_t)(1u<<i);
            }
        }
    }
    if(NormalCoopPlayerMenu() && g_ram[0x1031]==2) {
        uint16_t pressed=raw_pad & ~previous_pad;
        if(pressed&0x20) count=count%5+1;
        if(pressed&0x10) count=count==1?5:count-1;
        active_mask=(uint8_t)((1u<<count)-1);
        g_ram[0x1032]=(uint8_t)(count>1);
        /* Up/Down still reach the guest: the native two-row menu plays its
         * cursor sound on every press. Its row is replaced above each frame. */
    }
    previous_pad=raw_pad;
    if(installed) configure();
}
void NormalCoopAfterFrame(void) {
    for(unsigned i=0;i<5;++i) if(suppressed_mask&(1u<<i)) RtlSetPadState((int)i,raw_seats[i]);
    suppressed_mask=0;
}
void NormalCoopSave(SaveLoadInfo *sli) {
    uint32_t data[4]={0x3150434e,count,active_mask,previous_pad};sli->func(sli,data,sizeof(data));
    sli->func(sli,previous_seats,sizeof(previous_seats));sli->func(sli,&rejoin_pressed,1);
    sli->func(sli,&revive_mask,1);
    uint8_t preserve=preserve_characters?1:0;sli->func(sli,&preserve,1);
    sli->func(sli,campaign_characters,5);sli->func(sli,campaign_palettes,5);
    sli->func(sli,&block_start_mask,1);
    sli->func(sli,&benched_mask,1);sli->func(sli,benched,sizeof(benched));
}
void NormalCoopLoad(SaveLoadInfo *sli,uint32_t version) {
    (void)version;NormalCoopReset();
    if(!RtlStateBytesRemaining(sli)) { count=(g_ram[0x1d25]&3)==3?2:1;return; }
    uint32_t data[4]={0};sli->func(sli,data,sizeof(data));
    if(data[0]!=0x3150434e || data[1]<1 || data[1]>5 || !data[2] || data[2]>=(1u<<data[1]) || data[3]>65535) {
        RtlStateStreamFail(sli);return;
    }
    count=data[1];active_mask=(uint8_t)data[2];previous_pad=(uint16_t)data[3];
    sli->func(sli,previous_seats,sizeof(previous_seats));sli->func(sli,&rejoin_pressed,1);
    if(rejoin_pressed>31) { RtlStateStreamFail(sli);return; }
    sli->func(sli,&revive_mask,1);
    if(revive_mask>31) { RtlStateStreamFail(sli);return; }
    uint8_t preserve=0;sli->func(sli,&preserve,1);
    sli->func(sli,campaign_characters,5);sli->func(sli,campaign_palettes,5);
    if(preserve>1) { RtlStateStreamFail(sli);return; }
    preserve_characters=preserve!=0;
    sli->func(sli,&block_start_mask,1);
    if(block_start_mask>31) { RtlStateStreamFail(sli);return; }
    /* Snapshots older than boss-stage benching end here. */
    if(RtlStateBytesRemaining(sli)) {
        sli->func(sli,&benched_mask,1);sli->func(sli,benched,sizeof(benched));
        if(benched_mask>31) { RtlStateStreamFail(sli);return; }
    }
    if(installed) { configure();build_spawn();build_rejoin(); }
}

void NormalCoopPrepareWorldMapReturn(void) {
    if(!enabled || !campaign()) return;
    for(unsigned i=0;i<5;++i) if(benched_mask&(1u<<i))
        for(unsigned f=0;f<SEAT_FIELDS;++f) g_ram[seat_fields[f]+i]=benched[i][f];
    benched_mask=0;
}
