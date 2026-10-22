/* Five-seat campaign adapter for the verified JPN HiROM revision.
 * Guest routines still allocate actors, load graphics and run player input.
 * Trampolines below are authored instructions, installed only in the in-memory
 * cartridge. The cartridge file and the original SRAM layout are preserved. */
#include "normal_coop_mod.h"
#include "coop_corner_spawns_mod.h"
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
static Patch patches[96];
static unsigned patch_count;

static uint8_t *rom(uint32_t pc) { return SnesRomPtr(pc); }
static void reserve(uint32_t pc,unsigned size) {
    if(patch_count>=sizeof(patches)/sizeof(*patches) || size>sizeof(patches[0].original)) return;
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
/* One step from an edge spawn toward the middle of the playfield. */
static int inward(int value,int middle) { return value<middle?1:-1; }
static int spawn_tile(unsigned player,bool retry) {
    int x,y;
    if(retry) { x=g_ram[0x4e8d+player];y=g_ram[0x4ea5+player]; }
    else if(g_ram[0x1d1a]&0x0c) {
        int p1x=g_ram[0x4e8d],p2x=g_ram[0x4e8e];
        x=player==2?p1x-2:player==3?p2x+2:(p1x+p2x)/2;
        y=g_ram[0x4ea5];
    } else if(CoopCornerSpawnsEnabled()) {
        /* Corner layout mod: P3 lower left, P4 lower right, P5 centre. */
        x=player==2?1:player==3?13:7;y=player==4?6:11;
    } else {
        /* Default: P3 beside P1, P5 below/above P1, P4 beside P2, each one
         * tile from the cartridge spawn toward the middle of the field. */
        unsigned anchor=player==3?1:0;
        x=g_ram[0x4e8d+anchor];y=g_ram[0x4ea5+anchor];
        if(player==4) y+=inward(y,6); else x+=inward(x,7);
    }
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
/* Boss start $C4:3C60 (Y = boss index, tables $C0:B9A5/$B9AE/$B9B7) sets
 * $1CBF from $C0:B9AE (STA at $C4:3C78), gives the rivals equipment through
 * $C4:2906 with literal slots 2/3 (operands $C4:3C83/$3C8C) and seats the
 * mounted rival on its Louie ($C0:B9B7 type, ZONE3 - D's Black Louie) with
 * literal slot 2 (operands $C4:3C98/$3C9F). Those literals follow the rival
 * move, and $C4:3C75 loads the five-seat $1CBF instead of the table's. */
static const uint32_t boss_rider_sites[]={0xc43c83,0xc43c8c,0xc43c98,0xc43c9f};
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
 * copies at $0100-$011D, the bottom of the stack page: the stack never went
 * below $01BB in any of 1263 recorded dumps (boot, menus, every stage).
 * $1FC0-$1FDD, used before, is the flame-thrower's segment-object table
 * ($1FAB,Y, ZONE5 - 27): the distances overwrote its object ids and the
 * flame deleted P4's object, corrupting the object list. The loops cover the
 * roster, and $C3:FF70 fills the extra seats. The nearest-player chooser at
 * $C3:4641 keeps its two-entry direct-page buffers. */
enum { DIST_BASE=0x0100 };
static uint32_t enemy_sites[64];
static uint8_t enemy_original[64];
static unsigned enemy_site_count;
/* Seat loops of enemy code (`CPY #$02` operands): distance-based contact
 * and area hits above, then cell tests on $16AA,Y: $C3:15BF/$C3:3CA6 grab
 * ($15A2 bit 6), $C3:1937/$C3:296B/$C3:2EE8 cell hits (ZONE5 flame-thrower
 * columns), $C3:27AE $15BA bit 7, $C3:48AD player blocking a move; area
 * hits around a point that set $15A2 bit 6: $C0:2DB0 (radius from
 * $1FB1/$1FB3, ZONE5 - 1's tower shock), $C1:19AA; $C1:5D73 $15BA bit 7;
 * $C1:6B32 players carried by a moving platform ($C4:3C32 test, $C4:3C4F
 * move; ZONE5 - 25's shuttles). */
static const uint32_t enemy_loops[]={0xc3119c,0xc313a9,0xc32cb3,0xc347fb,0xc34817,
    0xc315c0,0xc31938,0xc327af,0xc3296c,0xc32ee9,0xc33ca7,0xc348ae,
    0xc02db1,0xc119ab,0xc15d74,0xc16b33};
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
/* Actor layout with three or more seats. The cartridge keeps P1/P2 in actors
 * 0/1, their mounts at rider + $1CBF (2 → actors 2/3) and enemies from actor
 * 5 ($C3:0458) up to 23 ($C4:2260). Seats 3-5 need actors 2-4, so the mounts
 * move to the end of the table (rider + 19 → actors 19-23) and enemies keep
 * starting at 5, capped below the mounts. Stage scripts name enemies and
 * gimmicks by absolute actor (ZONE5 - 1's tower attaches its core to actor
 * 9), so enemies must keep the cartridge's numbering; an earlier layout with
 * mounts at 5-9 and enemies from 10 left those scripts pointing at empty
 * actors (missing tower core and beams, gimmicks acting on the wrong actor).
 * Boss rivals move to actor 4 (3/4 in ZONE3 - D), mounts 23 (22/23). */
/* The palette save/restore copies ($C1:0015 -> $2CEA, $C1:0035 -> $2910,
 * blink/flash) move $21 bytes (LDA #$20 / MVN): one byte past the 32-byte
 * palette. For OBJ palette 15 that byte is $2B10 (slot 0's palette id) and
 * $2EEA (object 0's frame pointer): with P3 in slot 7, ZONE5 - J's first
 * form drew garbage. In co-op both copies move exactly 32 bytes (the extra
 * byte only ever reached the next palette's transparent color 0). */
static const uint32_t palette_copy_lengths[]={0xc1002a,0xc1004a};
enum { MOUNT_BASE=19, BOMBER_SLOTS=5, MOUNT_OFFSET_RAM=DIST_BASE+30 };
/* $1CBF is both the number of bomber slots (actors below it are players or
 * boss rivals: survivor counts $C5:031C, player-on-cell searches $C4:44DE,
 * $C4:4C94, $C4:5585, bomb owners $C5:1A14 ...) and the mount offset (mount
 * = rider + $1CBF: stock 2/2, Battle 5/5). Co-op needs 5 bomber slots but
 * mounts at rider + 19, so $1CBF stays 5 and the offset uses lives in
 * MOUNT_OFFSET_RAM ($011E, after the distance arrays): every ADC/SBC that
 * converts rider <-> mount, the loose-mount routines' range bounds and the
 * boss mount backups read it instead. With $1CBF = 19 every loop above
 * counted enemies 5-18 as players. */
static const uint32_t offset_sites[]={
    0xc40275,0xc40817,0xc40b43,0xc41e00,0xc42bfd,0xc43588,0xc43ca2,0xc43f97,
    0xc4434a,0xc4435e,0xc4443e,0xc44f2f,0xc451bf,0xc454e4,0xc45509, /* ADC  */
    0xc4501a,0xc45028,0xc4504a,  /* loose mount: LDA / CPX / SBC  */
    0xc450ab,0xc450b3,           /* LDA / CPX                     */
    0xc450e0,0xc450ee,0xc450fa,  /* LDA / CPX / SBC               */
    0xc5265a,                    /* actor on cell: SBC (CMP stays) */
    0xc43d39,0xc43d51,0xc43d6b,0xc43d82, /* boss mount backups: LDA */
};
static const uint8_t point_damage[]={
    0xe0,0x05,        /* 4350 CPX #$05    players and boss rivals    */
    0x90,0x08,        /* 4352 BCC skip                               */
    0xe0,MOUNT_BASE,  /* 4354 CPX #19     mounts are not enemies     */
    0xb0,0x04,        /* 4356 BCS skip                               */
    0x5c,0x51,0x37,0xc0, /* 4358 JML $C0:3751 enemy point damage     */
    0x5c,0x78,0x37,0xc0, /* 435C skip: JML $C0:3778                  */
};
/* A mount's palette slot ($C4:53EB) is its rider's seat + 1 ($16DA of the
 * mount holds the rider): 1/2 for P1/P2, but 3-5 (the enemies' slots) for
 * seats 3-5, whose mounts then wrote their colors into an enemy palette and
 * were drawn with it (the egg/Louie looked garbled). Use the rider object's
 * own slot instead ($C4:53F0 LDA $16DA,X / INC A -> JSR $5B40 / NOP). */
static const uint8_t mount_slot[]={
    0xda,             /* 5B40 PHX                                   */
    0xbd,0xda,0x16,   /* 5B41 LDA $16DA,X  rider seat               */
    0xaa,             /* 5B44 TAX                                   */
    0xbd,0xfa,0x17,   /* 5B45 LDA $17FA,X  rider object             */
    0x30,0x0b,        /* 5B48 BMI $5B55    no rider: seat + 1       */
    0xaa,             /* 5B4A TAX                                   */
    0xbd,0xdf,0x06,   /* 5B4B LDA $06DF,X  rider's palette slot     */
    0xc9,0x08,        /* 5B4E CMP #$08                              */
    0xb0,0x03,        /* 5B50 BCS $5B55                             */
    0xfa,             /* 5B52 PLX                                   */
    0x60,             /* 5B53 RTS                                   */
    0xea,             /* 5B54 NOP                                   */
    0xfa,             /* 5B55 PLX                                   */
    0xbd,0xda,0x16,   /* 5B56 LDA $16DA,X  stock: seat + 1          */
    0x1a,             /* 5B59 INC A                                 */
    0x60,             /* 5B5A RTS                                   */
};
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
/* Page request service $C5:6AE3: JSR $6AF5 (allocate); BCS out; JSR $6B36
 * (tile offset, start animation, dequeue); JSR $6B77 (upload); DEX; STX
 * $1CFE. A request with no page left (a Louie when five seats own all five
 * pages) stays first in the queue and is retried every frame, so every later
 * request - the riders' animations - waits forever. With five seats the
 * service jumps here: a failed request starts its animation without tiles and
 * the queue moves on; the extra-character overlay draws that mount. */
static const uint8_t page_service[]={
    0x20,0xf5,0x6a,   /* 7140 JSR $6AF5   allocate                  */
    0xb0,0x09,        /* 7143 BCS $714E                             */
    0x20,0x36,0x6b,   /* 7145 JSR $6B36                             */
    0x20,0x77,0x6b,   /* 7148 JSR $6B77                             */
    0x4c,0xee,0x6a,   /* 714B JMP $6AEE   DEX; STX $1CFE; out       */
    0xbc,0xd6,0x1c,   /* 714E LDY $1CD6,X requesting object         */
    0x20,0x56,0x6b,   /* 7151 JSR $6B56   start animation, dequeue  */
    0x4c,0xee,0x6a,   /* 7154 JMP $6AEE                             */
    0x20,0x36,0x6b,   /* 7157 JSR $6B36   same tiles already loaded */
    0x4c,0xee,0x6a,   /* 715A JMP $6AEE                             */
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
    restore(0xc4286f);restore(0xc502ff);restore(0xc5060e);restore(0xc56af5);restore(0xc56ae3);
    restore(0xc21697);restore(0xc43794);restore(0xc21f2b);
    for(unsigned i=0;i<sizeof(seat_loops)/sizeof(*seat_loops);++i) restore(seat_loops[i]);
    restore(0xc0374d);restore(0xc24340);restore(0xc24350);restore(0xc42260);restore(0xc453f0);restore(0xc45b40);
    for(unsigned i=0;i<sizeof(bomber_ops)/sizeof(*bomber_ops);++i) restore(bomber_ops[i]);
    for(unsigned i=0;i<sizeof(boss_rider_sites)/sizeof(*boss_rider_sites);++i) restore(boss_rider_sites[i]);
    restore(0xc43c75);
    for(unsigned i=0;i<sizeof(offset_sites)/sizeof(*offset_sites);++i) restore(offset_sites[i]);
    for(unsigned i=0;i<2;++i) restore(palette_copy_lengths[i]);
    for(unsigned i=0;i<boss_site_count;++i) rom(boss_sites[i])[0]=boss_original[i];
    for(unsigned i=0;i<enemy_site_count;++i) { rom(enemy_sites[i])[0]=enemy_original[i];rom(enemy_sites[i])[1]=0x1f; }
    for(unsigned i=0;i<sizeof(enemy_loops)/sizeof(*enemy_loops);++i) restore(enemy_loops[i]);
    restore(0xc34766);restore(0xc34b8f);restore(0xc3ff70);
    for(unsigned i=0;i<sizeof(mount_backups)/sizeof(*mount_backups);++i) restore(mount_backups[i]);
    /* $1CBF (mount actor = rider + $1CBF) is 2 in stock stages, but boss
     * stages set 3 or 4 from $C0:B9AE at $C4:3C78: their rival bombers own
     * slot 2 (and 3). Only undo the five-player value set below; forcing 2
     * every frame put a carried mount on the boss's slot (ZONE4 - G: boss
     * never appeared, rider stuck). */
    if(!coop) {
        if(g_ram[MOUNT_OFFSET_RAM]==MOUNT_BASE) {
            g_ram[MOUNT_OFFSET_RAM]=0;
            if(campaign() && g_ram[0x1cbf]==BOMBER_SLOTS) g_ram[0x1cbf]=2;
        }
        return;
    }
    set_masks();g_ram[0x1cbf]=BOMBER_SLOTS;g_ram[MOUNT_OFFSET_RAM]=MOUNT_BASE;
    for(unsigned i=0;i<sizeof(offset_sites)/sizeof(*offset_sites);++i) {
        uint8_t *op=rom(offset_sites[i]);op[0]=(uint8_t)MOUNT_OFFSET_RAM;op[1]=(uint8_t)(MOUNT_OFFSET_RAM>>8);
    }
    for(unsigned i=0;i<2;++i) if(rom(palette_copy_lengths[i])[0]==0x20) rom(palette_copy_lengths[i])[0]=0x1f;
    { static const uint8_t keep[]={0xa9,BOMBER_SLOTS,0xea,0x8d,0xbf,0x1c}; /* LDA #5; NOP; STA $1CBF */
      memcpy(rom(0xc43c75),keep,sizeof(keep)); }
    /* 0–4 players, 5–18 enemies (cartridge numbering), 19–23 mounts. */
    rom(0xc42261)[0]=MOUNT_BASE;
    memcpy(rom(0xc45b40),mount_slot,sizeof(mount_slot));
    { static const uint8_t to[]={0x20,0x40,0x5b,0xea}; memcpy(rom(0xc453f0),to,sizeof(to)); }
    for(unsigned i=0;i<sizeof(mount_backups)/sizeof(*mount_backups);++i) rom(mount_backups[i])[1]+=MOUNT_BASE-2;
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
    jump(0xc56ae3,0xc57140); /* Requests without a page no longer block the queue. */
    jump(0xc5060e,0xc57000);
    /* Script op $12 (exit warps, switches) tests only P1/P2 against the tile,
     * so a warp entered by P3-P5 never finished the stage. */
    for(unsigned i=0;i<sizeof(seat_loops)/sizeof(*seat_loops);++i) rom(seat_loops[i])[1]=(uint8_t)seats();
    for(unsigned i=0;i<enemy_site_count;++i) {
        unsigned slot=(unsigned)(enemy_original[i]-0xf4);
        unsigned address=DIST_BASE+5*(slot/2)+slot%2;
        rom(enemy_sites[i])[0]=(uint8_t)address;rom(enemy_sites[i])[1]=(uint8_t)(address>>8);
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
        for(unsigned i=0;i<sizeof(boss_rider_sites)/sizeof(*boss_rider_sites);++i) rom(boss_rider_sites[i])[0]+=(uint8_t)shift;
    }
    /* Point damage: enemies are actors 5-18; mounts 19-23 are not. */
    memcpy(rom(0xc24350),point_damage,sizeof(point_damage));
    { static const uint8_t to[]={0x5c,0x50,0x43,0xc2}; memcpy(rom(0xc0374d),to,sizeof(to)); }
    /* Story scripts distinguish one/two-player choreography, not hardware
     * seats. Expose their legacy branch while the adapter owns the roster. */
    rom(0xc21f2b)[1]=0x24;
    jump(0xc21697,0xc24300);
}
/* OBJ palette slots ($C0:0295: reuse a slot of the same palette in the
 * request mask $0348, else $0347 consecutive free slots, else fail at
 * $C0:0303). The cartridge uses slot 0 for the stage, 1/2 for P1/P2, 3-5 for
 * enemies (mask $38) and 6-7 for large stage objects (mask $C0: ZONE5 - 1's
 * tower and its beams need both). Seats 3-5 take 7, 6 and 5 when free
 * (see palette_copy_lengths for slot 7). There are not enough slots for
 * every seat and every stage object, so the stage
 * wins. When a non-player request fails and slots held by seats 3-5 inside
 * its mask would make it fit, those seats' objects (bomber, mount, egg)
 * share P1's slot instead and the search runs again; a seat that finds no
 * slot shares P1's slot too. A seat sharing a slot does not write its own
 * colors into it ($C4:37B0). */
static int pending_seat=-1;
static const uint8_t extra_slots[]={0x80,0x40,0x20}; /* seats 3-5: slots 7, 6, 5 */
static unsigned fallback_slot(void) {
    for(unsigned s=1;s<=2;++s) if(g_ram[0x2b10+s] && g_ram[0x349+s]) return s;
    return 0;
}
static int seat_owning(unsigned slot) {
    if(slot<3 || slot>7 || !g_ram[0x349+slot]) return -1;
    for(unsigned p=2;p<seats();++p)
        if(g_ram[0x17fa+p]<0x60 && g_ram[0x6df+g_ram[0x17fa+p]]==slot) return (int)p;
    return -1;
}
static void share_slot(unsigned slot,unsigned target) {
    for(unsigned o=0;o<0x60;o+=2) if(g_ram[0x6df+o]==slot) g_ram[0x6df+o]=(uint8_t)target;
    g_ram[0x349+target]=(uint8_t)(g_ram[0x349+target]+g_ram[0x349+slot]);
    g_ram[0x349+slot]=0;g_ram[0x2b10+slot]=0;g_ram[0x2b18+slot]=0;g_ram[0x2b20+slot]=0;
}
static void palette_full(CpuState *cpu,uint32_t pc) {
    (void)pc;
    if(!enabled || !installed || !campaign() || count<=2) return;
    unsigned mask=g_ram[0x348],need=g_ram[0x347],target=fallback_slot();
    if(!target) return;
    if(pending_seat>=2 && mask==extra_slots[pending_seat-2]) {
        /* A seat with no slot of its own shares P1's (C0:033D counts it). */
        cpu->X=(uint16_t)target;interp_bridge_pre_opcode_redirect(0xc0033d);return;
    }
    bool usable[8];int run=0,first=-1;
    for(unsigned s=0;s<8;++s) {
        usable[s]=(mask>>s&1) && (!g_ram[0x2b10+s] || seat_owning(s)>=0);
        run=usable[s]?run+1:0;
        if(need && run>=(int)need) { first=(int)s-(int)need+1;break; }
    }
    if(first<0) return;
    bool moved=false;
    for(unsigned s=(unsigned)first;s<(unsigned)first+need;++s)
        if(g_ram[0x2b10+s] && seat_owning(s)>=0) { share_slot(s,target);moved=true; }
    if(moved) interp_bridge_pre_opcode_redirect(0xc002e2);
}
static void seat_colors(CpuState *cpu,uint32_t pc) {
    (void)pc;
    if(!enabled || !installed || !campaign() || count<=2) return;
    unsigned object=cpu->Y&0xff;
    if(object>=0x60 || g_ram[0x6df+object]>2) return;
    for(unsigned p=2;p<seats();++p)
        if(g_ram[0x17fa+p]==object) { interp_bridge_pre_opcode_redirect(0xc437cc);return; }
}
/* $C3:FF70 runs for every enemy each frame: it clears the six five-entry
 * distance arrays ($C3:475C, 30 bytes) and runs $C3:46CC for seats 3-5.
 * With many enemies (ZONE5 snake chains) that interpreted work pushed frames
 * past vblank. The same stores happen natively here, in the same order,
 * including $C3:46CC's scratch ($9F-$A6, $1D28-$1D2D), A and the flags
 * at the trampoline's RTS, then execution continues at that RTS. */
static uint8_t *dp_byte(const CpuState *cpu,unsigned offset) { return &g_ram[(cpu->D+offset)&0x1fff]; }
static void enemy_distances_native(CpuState *cpu,uint32_t pc) {
    (void)pc;
    if(!enabled || !installed || !campaign() || count<=2) return;
    if(memcmp(rom(0xc3ff70),enemy_distances,7) || !cpu->m_flag || !cpu->x_flag || cpu->emulation) return;
    unsigned db=cpu->DB;
    if(!(db<0x40 || (db>=0x80 && db<0xc0) || db==0x7e) || cpu->D>=0x1f00) return;
    unsigned e=cpu->X&0xff,limit=rom(0xc3ff77)[0];
    uint16_t a=(uint16_t)((cpu->A&0xff00)|0xff);
    bool v=cpu->_flag_V;
    memset(g_ram+DIST_BASE,0xff,30);
    for(unsigned seat=2;seat<limit;++seat) {
        a=(uint16_t)((a&0xff00)|(g_ram[0x15a2+seat]&0x80));
        if(a&0x80) continue;
        *dp_byte(cpu,0x9f)=0;
        g_ram[0x1d29]=g_ram[0x1842+e];g_ram[0x1d28]=g_ram[0x182a+e];
        g_ram[0x1d2b]=g_ram[0x188a+e];g_ram[0x1d2a]=g_ram[0x1872+e];
        *dp_byte(cpu,0xa0)=0;
        if(g_ram[0x17fa+seat]&0x80) {
            *dp_byte(cpu,0xa5)=*dp_byte(cpu,0xa6)=0xff;a=(uint16_t)((a&0xff00)|0xff);continue;
        }
        uint8_t xdir=1,ydir=2;
        unsigned px=g_ram[0x182a+seat]|g_ram[0x1842+seat]<<8,ex=g_ram[0x1d28]|g_ram[0x1d29]<<8;
        unsigned dx=(px-ex)&0xffff;v=((px^ex)&(px^dx)&0x8000)!=0;
        g_ram[0x1d28]=(uint8_t)dx;g_ram[0x1d29]=(uint8_t)(dx>>8);
        if(dx&0x8000) { xdir=3;*dp_byte(cpu,0xa2)=0;dx=(0x10000-dx)&0xffff; } /* 16-bit STA $A1 */
        g_ram[0x1d2c]=(uint8_t)dx;g_ram[0x1d2d]=(uint8_t)(dx>>8);
        g_ram[DIST_BASE+seat]=(uint8_t)dx;g_ram[DIST_BASE+10+seat]=(uint8_t)(dx>>8);
        unsigned py=g_ram[0x1872+seat]|g_ram[0x188a+seat]<<8,ey=g_ram[0x1d2a]|g_ram[0x1d2b]<<8;
        unsigned dy=(py-ey)&0xffff;v=((py^ey)&(py^dy)&0x8000)!=0;
        g_ram[0x1d2a]=(uint8_t)dy;g_ram[0x1d2b]=(uint8_t)(dy>>8);
        if(dy&0x8000) { ydir=4;*dp_byte(cpu,0xa4)=0;dy=(0x10000-dy)&0xffff; } /* 16-bit STA $A3 */
        g_ram[DIST_BASE+5+seat]=(uint8_t)dy;g_ram[DIST_BASE+15+seat]=(uint8_t)(dy>>8);
        g_ram[DIST_BASE+25+seat]=ydir;g_ram[DIST_BASE+20+seat]=xdir;
        *dp_byte(cpu,0xa1)=xdir;*dp_byte(cpu,0xa3)=ydir;
        a=(uint16_t)((dy&0xff)<<8|xdir);
    }
    /* State at $FF88 (RTS) after PLY: Y and X unchanged. */
    cpu->A=a;cpu->_flag_V=v;cpu->_flag_C=1;
    cpu->_flag_N=(cpu->Y&0x80)!=0;cpu->_flag_Z=(cpu->Y&0xff)==0;
    interp_bridge_pre_opcode_redirect(0xc3ff88);
}
static bool palette_hooks;
/* Objects whose page request failed ($C5:714E, page_service): the archive
 * they asked for ($1CCC,X) and their object set, so the overlay can draw
 * them from the cartridge (a piece's tile t is sheet tile t) instead of the
 * tiles of whatever page their stale offset points at. */
static uint8_t pageless_archive[0x30],pageless_set[0x30];
/* Refilling the page an object already shows ($C5:7120) happens while the
 * frame on screen still uses that page's old tiles: the old animation frame
 * was drawn with the new tiles for one frame (a garbled bomber whenever a
 * player started or stopped walking with five seats). The first time such a
 * request reaches the service, it is left queued for one frame and the object
 * is hidden for that frame through the native blink counter ($085F: the OAM
 * pass $C0:146F decrements it and skips odd values); the next frame the page
 * is refilled while the object is not on screen. */
static uint8_t deferred_refill[0x30];
/* What each page holds: owner object, archive and half of its last upload
 * ($C5:6B77). A walking bomber asks again for the chunk it already shows on
 * every step; that refill rewrote identical tiles, but with the hiding below
 * the bomber blinked on every step. Such requests now only restart the
 * animation ($C5:7157: JSR $6B36 without the upload). */
static uint8_t page_owner[16],page_archive[16],page_half[16];
static void page_upload(CpuState *cpu,uint32_t pc) {
    (void)pc;
    if(!enabled || !installed) return;
    unsigned x=cpu->X&0xff,page=(g_ram[0x1d11]&0x1f)>>1;
    if(x>=10 || page>=16) return;
    /* $C5:6B36 already dequeued the object ($1CD6,X = $FF); Y still holds it. */
    page_owner[page]=(uint8_t)cpu->Y;page_archive[page]=g_ram[0x1ccc+x];page_half[page]=g_ram[0x1cea+x];
}
static void page_service_entry(CpuState *cpu,uint32_t pc) {
    (void)pc;
    if(!enabled || !installed || !campaign() || count<=2) return;
    unsigned y=cpu->Y&0xff,x=cpu->X&0xff;
    if(y>=0x60 || (y&1) || x>=10) return;
    unsigned pool=g_ram[0x1d10]<16?g_ram[0x1d10]:16;
    int owned=-1;
    for(unsigned i=0;i<16;++i) {
        if(g_ram[0x1d00+i]&0x80) return;  /* a free page: stock path */
        if(i<pool && g_ram[0x1d00+i]==y) owned=(int)i;
    }
    if(owned<0) return;
    if(page_owner[owned]==y && page_archive[owned]==g_ram[0x1ccc+x] && page_half[owned]==g_ram[0x1cea+x]) {
            g_ram[0x1d11]=(uint8_t)(0x60|owned<<1);deferred_refill[y/2]=0;
        interp_bridge_pre_opcode_redirect(0xc57157);return;
    }
    /* New tiles for a page on screen (start or stop walking, a turn): hide
     * the object for the one frame that would show the old frame with them. */
    if(deferred_refill[y/2]) { deferred_refill[y/2]=0;return; }
    deferred_refill[y/2]=1;
    if(!g_ram[0x85f+y]) g_ram[0x85f+y]=2;
    interp_bridge_pre_opcode_redirect(0xc56af2); /* PLX / PLY / RTL: request stays queued */
}
static void page_refused(CpuState *cpu,uint32_t pc) {
    (void)pc;
    if(!enabled || !installed || !campaign() || count<=2) return;
    unsigned x=cpu->X&0xff,object=g_ram[0x1cd6+x],archive=g_ram[0x1ccc+x];
    if(x>=10 || object>=0x60 || (object&1)) return;
    pageless_archive[object/2]=(uint8_t)archive;pageless_set[object/2]=g_ram[0x4ff+object];
}
unsigned NormalCoopPagelessArchive(unsigned object) {
    if(!enabled || !installed || !campaign() || count<=2 || object>=0x60 || (object&1)) return 0;
    if(pageless_set[object/2]!=g_ram[0x4ff+object]) return 0;
    for(unsigned p=0;p<16;++p) if(g_ram[0x1d00+p]==object) return 0;
    return pageless_archive[object/2];
}
static void instruction(uint32_t pc,int m,int x) {
    if(previous_hook) previous_hook(pc,m,x);
    if(!campaign() || count<=2) return;
    if(pc==0xc4286f) {
        memset(pageless_archive,0,sizeof(pageless_archive));memset(deferred_refill,0,sizeof(deferred_refill));memset(page_owner,0xff,sizeof(page_owner));
        if(g_ram[0x1031]==8) {
            preserve_characters=true;
            memcpy(campaign_characters,g_ram+0x16c2,5);memcpy(campaign_palettes,g_ram+0x16da,5);
        }
        configure();
    }
    else if(pc==0xc4289d) g_ram[0x1d23]=(uint8_t)(count+2);
    else if(pc==0xc42ae3 || pc==0xc42ba5) pending_seat=-1; /* seat palette request returned */
    else if(pc==0xc42adc || pc==0xc42b90) {
        /* The actor allocators request OBJ palette slot 1<<(player+1). Slot 3
         * belongs to campaign enemies, so P3 failed and drew with an unrelated
         * palette. Seats 3-5 use extra_slots instead. */
        unsigned player=g_ram[0x8d];
        if(player>=2 && player<seats()) { g_ram[0x348]=extra_slots[player-2];pending_seat=(int)player; }
    }
    else if(pc==0xc428b4) {
        for(unsigned i=2;i<count;++i) { g_ram[0x16c2+i]=0;g_ram[0x16da+i]=(uint8_t)(g_rom[0xbb4e]+i); }
        if(preserve_characters) {
            memcpy(g_ram+0x16c2,campaign_characters,count);memcpy(g_ram+0x16da,campaign_palettes,count);
        }
        active_mask=(uint8_t)((1u<<count)-1);set_masks();g_ram[0x1cbf]=BOMBER_SLOTS;g_ram[MOUNT_OFFSET_RAM]=MOUNT_BASE;benched_mask=0;
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
        set_masks();g_ram[0x1cbf]=BOMBER_SLOTS;g_ram[MOUNT_OFFSET_RAM]=MOUNT_BASE;build_spawn();spawn_check=true;
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
    memset(pageless_archive,0,sizeof(pageless_archive));memset(deferred_refill,0,sizeof(deferred_refill));memset(page_owner,0xff,sizeof(page_owner));memset(pageless_set,0,sizeof(pageless_set));
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
        for(unsigned i=0;i<enemy_site_count;++i) { rom(enemy_sites[i])[0]=enemy_original[i];rom(enemy_sites[i])[1]=0x1f; }
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
        reserve(0xc21f2b,3);reserve(0xc0374d,4);reserve(0xc24350,sizeof(point_damage));reserve(0xc42260,2);reserve(0xc453f0,4);reserve(0xc45b40,sizeof(mount_slot));reserve(0xc24340,sizeof(bomber_slot));
        for(unsigned i=0;i<sizeof(bomber_ops)/sizeof(*bomber_ops);++i) reserve(bomber_ops[i],4);
        for(unsigned i=0;i<sizeof(boss_rider_sites)/sizeof(*boss_rider_sites);++i) reserve(boss_rider_sites[i],1);
        reserve(0xc43c75,6);
        for(unsigned i=0;i<sizeof(offset_sites)/sizeof(*offset_sites);++i) reserve(offset_sites[i],2);
        for(unsigned i=0;i<2;++i) reserve(palette_copy_lengths[i],1);
        for(unsigned i=0;i<sizeof(seat_loops)/sizeof(*seat_loops);++i) reserve(seat_loops[i],2);
        for(unsigned i=0;i<sizeof(mount_backups)/sizeof(*mount_backups);++i) reserve(mount_backups[i],3);
        reserve(0xc45b00,32);reserve(0xc24300,64);reserve(0xc57000,96);
        reserve(0xc57100,sizeof(page_allocator));memcpy(rom(0xc57100),page_allocator,sizeof(page_allocator));
        reserve(0xc56ae3,3);
        reserve(0xc57140,sizeof(page_service));memcpy(rom(0xc57140),page_service,sizeof(page_service));
        previous_hook=g_interp_bridge_pc_hook;g_interp_bridge_pc_hook=instruction;installed=true;
        /* Palette arbitration (see palette_full): the allocator's failure
         * exit and the per-object palette writer. Added once; inert when off. */
        if(!palette_hooks && rom(0xc00303)[0]==0xa9 && rom(0xc00304)[0]==0xff && rom(0xc437b0)[0]==0x48 && rom(0xc437cc)[0]==0x60)
            palette_hooks=interp_bridge_add_pre_opcode_hook(0xc00303,palette_full) &&
                          interp_bridge_add_pre_opcode_hook(0xc437b0,seat_colors) &&
                          interp_bridge_add_pre_opcode_hook(0xc5714e,page_refused) &&
                          interp_bridge_add_pre_opcode_hook(0xc57140,page_service_entry) &&
                          interp_bridge_add_pre_opcode_hook(0xc56b77,page_upload) &&
                          interp_bridge_add_pre_opcode_hook(0xc3ff70,enemy_distances_native);
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
/* A seat 3-5 that shares P1's (or P2's) slot gets its own slot back once
 * the stage released it: its bomber and mount objects move there and its
 * nine colors are copied from its palette entry exactly as $C0:01C8 does
 * for a player ($0AE7 = 0: table word $C0:0379, bank $81, entry + 4), into
 * the CGRAM shadow and the blink backup ($2910/$2CEA, $0351 = 1). */
static void load_seat_colors(unsigned id,unsigned slot) {
    if(!id) id=1;
    const uint8_t *t=rom(0xc00379);
    const uint8_t *e=rom(0x810000u|(unsigned)(t[0]|t[1]<<8))+2*(id-1);
    const uint8_t *c=rom(0x810000u|(unsigned)(e[0]|e[1]<<8))+4;
    unsigned dst=(0x80+slot*16)*2;
    for(unsigned i=0;i<18;++i) g_ram[0x2910+dst+i]=g_ram[0x2cea+dst+i]=c[i];
    g_ram[0x351]=1;
}
static void reclaim_palettes(void) {
    if(!enabled || !installed || !campaign() || count<=2 || g_ram[0x1031]<0x0d || (g_ram[0x69]&15)) return;
    for(unsigned p=2;p<seats();++p) {
        unsigned o=g_ram[0x17fa+p];
        if(o>=0x60 || (g_ram[0x15a2+p]&0x80)) continue;
        unsigned from=g_ram[0x6df+o],to=0;
        if(from<1 || from>2) continue;
        for(unsigned slot=3;slot<8;++slot) if(extra_slots[p-2]>>slot&1) to=slot;
        if(!to || g_ram[0x2b10+to] || g_ram[0x349+to]) continue;
        unsigned objects[2]={o,0xff},moved=0;
        if(p+MOUNT_BASE<24) objects[1]=g_ram[0x17fa+p+MOUNT_BASE];
        for(unsigned i=0;i<2;++i) {
            unsigned x=objects[i];
            if(x<0x60 && g_ram[0x6df+x]==from) { g_ram[0x6df+x]=(uint8_t)to;++moved; }
        }
        g_ram[0x349+from]=(uint8_t)(g_ram[0x349+from]>moved?g_ram[0x349+from]-moved:0);
        g_ram[0x349+to]=(uint8_t)moved;g_ram[0x2b10+to]=g_ram[0x16da+p];g_ram[0x2b18+to]=0;g_ram[0x2b20+to]=1;
        load_seat_colors(g_ram[0x16da+p],to);
    }
}
void NormalCoopAfterFrame(void) {
    reclaim_palettes();
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
