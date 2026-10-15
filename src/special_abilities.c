/* Special abilities (B + direction) of the extra characters: the Super
 * Bomberman 4 Bomber Kings' moves, ported with the timings, poses and
 * graphics of Super Bomberman 4 (read from the player's own SB4 image), and
 * Honey's and Kotetsu's attacks from their Super Bomberman 5 boss fights.
 *
 * Input (maintainer's rule): hold B and press a direction, or press B while
 * a direction is held. B alone keeps its native meaning (remote-control
 * detonation): for a seat whose ability is ready, a fresh B is held back for
 * a few frames waiting for a direction and released unchanged when none
 * comes. A rider (Louie, $1782 bit 7) never triggers an ability; Louie's own
 * skill stays on Y. While the ability recovers (debuff) B is native.
 *
 * Timelines (frames after the trigger), measured in Super Bomberman 4 with a
 * headless emulator (frame-by-frame captures of the characters' battles):
 *   Bazooka: stands still for 98 frames; the aiming pose turns through the
 *     four directions every 8 frames (0-31) and holds the chosen one; frames
 *     68-82 a fire beam from the next tile up to the first block (explosion
 *     tiles, phases 2,2,4,4,6,6,8,8,8,8,6,6,4,4,2); lethal like an explosion.
 *   Jet: 48-frame wind-up (four poses), then flies 4 px per frame until a
 *     block stops it, invulnerable, leaving a fire trail: every 8 frames a
 *     two-tile segment (tail end + middle) that lives 16 frames (phases
 *     2,4,6,8,8,6,4,2), lethal.
 *   Hammer: stands still for 125 frames swinging the mace around himself
 *     (frames 29-106: angle -90 - 22.5 deg per frame, radius growing from 16
 *     to 48 px by frame 61, then shrinking back); a player it touches is
 *     stunned and loses an item. No debuff.
 *   Lady: six-pose throw (0-40), then from frame 48 a laser leaves 14 px in
 *     front of her at 2 px per frame until a block or a target; a player it
 *     hits is stunned and loses two items. No debuff.
 *   Great: stands invulnerable for 160 frames facing the camera.
 *   Honey (boss): shooting animation; the shot hits the first target in
 *     line before a block. Kotetsu (boss): sword cut on the tile in front.
 * Debuff (Super Bomberman 4: Bazooka, Jet, Great; also Honey and Kotetsu):
 * 640 frames (about 10.7 s) at half walking speed with the ability blocked.
 *
 * Damage uses the cartridge's own rules (point damage $C0:373F): a player
 * actor gets $15A2 bit 6 (hit) unless invulnerable ($15A2 bit 2), an enemy
 * loses one point of $1662 with 180 frames of invulnerability ($085F).
 * Normal Game hits enemies and bosses only, never a player (no friendly
 * fire). Battle hits every other player, or only the other team in Team
 * Battle ($7E625D bit 0; teams in $15A2 bits 3-4, from $6269 bit 7).
 * Stun and item loss are the cartridge's own: $15BA bit 5 makes the player
 * dazed for 50 frames and drop an item ($C4:4618/$C4:4631, which calls the
 * Battle routine $C4:37CD with A = 0: the first item flies off). Lady's
 * second item is taken with $C4:37CD itself, called from the pad hook
 * through pushed return frames.
 *
 * All state lives in free WRAM ($7F:F830-$F8D9, zero in every research
 * capture), so save states and rewind carry it without an extension. */
#include "special_abilities.h"
#include "honey_kotetsu_mod.h"
#include "sb4_rom.h"
#include "common_rtl.h"
#include "snes/interp_bridge.h"
#include <string.h>

enum {
    STATE = 0x1f830, SEAT_BYTES = 32, PENDING = 0x1f8d0, SPEED_ACC = 0x1f8d5, STATE_END = 0x1f8da,
    PAD_HOOK = 0xc00af9, SPEED_HOOK = 0xc43180,
    LOSE_ITEM = 0xc437cd, RTL_C4 = 0xc436d1,
    DEBUFF_FRAMES = 640, B_WINDOW = 8, TRAILS = 3,
};
/* Per seat: +0/1 debuff, +2/3 cooldown, +4 kind, +5/6 timer, +7 direction
 * (0 right, 1 down, 2 left, 3 up), +8/9 distance (pixels), +10..12 actors
 * already hit, +13 flags (bit 0 stopped, bit 1 projectile done), +14 frame
 * the flight stopped, +15/16 last position, +17 items this seat still has
 * to lose, +18..26 Jet trail (TRAILS x: cell, age, flags), +27 beam tiles. */
enum { S_DEBUFF = 0, S_COOLDOWN = 2, S_KIND = 4, S_TIMER = 5, S_DIR = 7, S_DIST = 8, S_HIT = 10,
       S_FLAGS = 13, S_STOP = 14, S_LAST = 15, S_DROPS = 17, S_TRAIL = 18, S_BEAM = 27 };
enum { AB_NONE, AB_SHOT, AB_SLASH, AB_HAMMER, AB_JET, AB_LASER, AB_BAZOOKA, AB_GREAT };
typedef struct { uint8_t kind; uint16_t debuff; uint16_t cooldown; } Ability;
/* Index: choice - 11 (Honey, Kotetsu, Bomberman, Hammer, Jet, Lady, Bazooka, Great). */
static const Ability kAbility[8]={
    {AB_SHOT,   DEBUFF_FRAMES, 0},
    {AB_SLASH,  DEBUFF_FRAMES, 0},
    {AB_NONE,   0,             0},
    {AB_HAMMER, 0,            30},
    {AB_JET,    DEBUFF_FRAMES, 0},
    {AB_LASER,  0,            30},
    {AB_BAZOOKA,DEBUFF_FRAMES, 0},
    {AB_GREAT,  DEBUFF_FRAMES, 0},
};
enum { SHOT_FRAMES = 24, SHOT_AT = 8, SLASH_FRAMES = 16, BAZOOKA_FRAMES = 98, BEAM_FROM = 68, BEAM_TO = 82,
       JET_WINDUP = 48, JET_SPEED = 4, TRAIL_LIFE = 16, HAMMER_FRAMES = 125, MACE_FROM = 29, MACE_TO = 106,
       LASER_FROM = 48, LASER_POSE = 41, GREAT_FRAMES = 160 };
static uint8_t *seat_state(unsigned i) { return g_ram+STATE+i*SEAT_BYTES; }
static unsigned get16(const uint8_t *p) { return p[0]|p[1]<<8; }
static void set16(uint8_t *p,unsigned v) { p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8); }

static bool hooks_installed;
static const Ability *ability_of(unsigned seat) {
    unsigned choice=ExtraCharacterSeatChoice(seat);
    if(choice<HK_CHOICE_HONEY || choice>EXTRA_CHOICE_LAST) return NULL;
    const Ability *a=&kAbility[choice-HK_CHOICE_HONEY];
    return a->kind?a:NULL;
}
static bool battle(void) { return (g_ram[0x1d1a]&1)!=0; }
static bool in_stage(void) {
    unsigned mode=g_ram[0x1031];
    if(battle()) return mode==0x74;
    return mode>=0x0d && mode<=0x73;
}
static bool playing(void) { return in_stage() && g_ram[0x0aee] && !g_ram[0x1d1d]; }
static bool alive(unsigned actor) {
    return g_ram[0x17fa+actor]<0x80 && !(g_ram[0x15a2+actor]&0xc0);
}
static int world_x(unsigned a) { return g_ram[0x182a+a]|g_ram[0x1842+a]<<8; }
static int world_y(unsigned a) { return g_ram[0x1872+a]|g_ram[0x188a+a]<<8; }
/* A player actor of a campaign seat ($1D25 participation) is an ally. */
static bool hostile(unsigned seat,unsigned actor) {
    if(actor==seat) return false;
    if(actor>=5) return true;
    if(!battle()) return !((g_ram[0x1d25]>>actor)&1);
    if(g_ram[0x625d]&1) return (g_ram[0x15a2+actor]&0x18)!=(g_ram[0x15a2+seat]&0x18);
    return true;
}
static void damage(unsigned actor) {
    if(actor<5) {
        if(!(g_ram[0x15a2+actor]&0x04)) g_ram[0x15a2+actor]|=0x40;
        return;
    }
    unsigned object=g_ram[0x17fa+actor];
    if(object>=0x60 || g_ram[0x85f+object]) return;
    if(!--g_ram[0x1662+actor]) g_ram[0x15a2+actor]|=0x40;
    else g_ram[0x85f+object]=0xb4;
}
/* Mace and laser: a player is stunned and loses `items` items (Battle; a
 * rival bomber in Normal Game is damaged instead), once per ability. */
static void knock(unsigned seat,unsigned actor,unsigned items) {
    uint8_t *s=seat_state(seat);
    if((s[S_HIT+actor/8]>>(actor%8))&1) return;
    s[S_HIT+actor/8]|=(uint8_t)(1u<<(actor%8));
    if(actor>=5 || !battle()) { damage(actor);return; }
    if(g_ram[0x15a2+actor]&0x04) return;
    /* $15BA bit 5: the cartridge's own "stunned, drops an item" reaction
     * ($C4:4618 -> $C4:4631: one item, 50 frames dazed); further items
     * are taken with the same routine from the pad hook. */
    g_ram[0x15ba+actor]|=0x20;
    uint8_t *v=seat_state(actor);
    v[S_DROPS]=(uint8_t)(v[S_DROPS]+items-1);
}
static int cell_of(int world) { return (world+8)>>4; }
/* Hostile actors within `reach` pixels of world (x, y). */
static unsigned near(unsigned seat,int x,int y,int reach,unsigned *found,unsigned max) {
    unsigned n=0;
    for(unsigned a=0;a<24 && n<max;++a) {
        if(!alive(a) || !hostile(seat,a)) continue;
        int dx=world_x(a)-x,dy=world_y(a)-y;
        if(dx<0) dx=-dx;
        if(dy<0) dy=-dy;
        if(dx<reach && dy<reach) found[n++]=a;
    }
    return n;
}
/* Damages hostile actors standing in map cell (cx, cy). */
static bool strike_cell(unsigned seat,int cx,int cy) {
    bool hit=false;
    for(unsigned a=0;a<24;++a) {
        if(!alive(a) || !hostile(seat,a)) continue;
        if(cell_of(world_x(a))==cx && cell_of(world_y(a))==cy) { damage(a);hit=true; }
    }
    return hit;
}
static const int kDx[4]={1,0,-1,0},kDy[4]={0,1,0,-1};
/* Stage map: $1052, 16 columns; $80 hard block, $30 soft block, $20 bomb. */
static bool blocked_cell(int cx,int cy) {
    if(cx<0 || cy<0 || cx>=16 || cy>=13) return true;
    return (g_ram[0x1052+cy*16+cx]&0xb0)!=0;
}
/* Pad direction bits ($31+seat): right 1, left 2, down 4, up 8 -> R/D/L/U. */
static int direction_of(unsigned bits) {
    if(bits&1) return 0;
    if(bits&4) return 1;
    if(bits&2) return 2;
    if(bits&8) return 3;
    return -1;
}
static const uint8_t kDirBits[4]={1,4,2,8};

static bool ready(unsigned seat) {
    uint8_t *s=seat_state(seat);
    return !get16(s+S_DEBUFF) && !get16(s+S_COOLDOWN) && !s[S_KIND];
}
static void invulnerable(unsigned seat,unsigned frames) {
    /* Native post-hit invulnerability: $15A2 bit 2 with timer $170A. */
    g_ram[0x15a2+seat]|=0x04;
    if(g_ram[0x170a+seat]<frames) g_ram[0x170a+seat]=(uint8_t)frames;
}
static void start(unsigned seat,int direction) {
    const Ability *a=ability_of(seat);
    uint8_t *s=seat_state(seat);
    uint8_t drops=s[S_DROPS];
    memset(s+S_KIND,0,SEAT_BYTES-S_KIND);
    s[S_DROPS]=drops;
    s[S_KIND]=a->kind;s[S_DIR]=(uint8_t)direction;
    if(a->kind==AB_GREAT) invulnerable(seat,GREAT_FRAMES);
    if(a->kind==AB_JET) invulnerable(seat,JET_WINDUP+8);
}
static void finish(unsigned seat) {
    const Ability *a=ability_of(seat);
    uint8_t *s=seat_state(seat);
    s[S_KIND]=0;
    if(a) { set16(s+S_DEBUFF,a->debuff);set16(s+S_COOLDOWN,a->cooldown); }
}
/* Fire beam of Bazooka: tiles from the next one up to the first block. */
static unsigned beam_length(unsigned seat) {
    unsigned d=seat_state(seat)[S_DIR]&3,k=0;
    int cx=cell_of(world_x(seat)),cy=cell_of(world_y(seat));
    while(k<15 && !blocked_cell(cx+kDx[d]*(int)(k+1),cy+kDy[d]*(int)(k+1))) ++k;
    return k;
}
static int mace_radius(unsigned t) {
    if(t<=61) return 16+(int)(t-MACE_FROM);
    int r=48-(int)((t-61)*105/100);
    return r>0?r:0;
}
/* Mace angle -90 - 22.5 deg per frame from frame 32: 16 directions (x1024). */
static const int16_t kMaceCos[16]={0,-392,-724,-946,-1024,-946,-724,-392,0,392,724,946,1024,946,724,392};
static const int16_t kMaceSin[16]={-1024,-946,-724,-392,0,392,724,946,1024,946,724,392,0,-392,-724,-946};
static int scale(int v,int r) { int p=v*r;return p>=0?(p+512)/1024:-((-p+512)/1024); }
static void mace_position(unsigned seat,unsigned t,int *x,int *y) {
    unsigned k=(t+16-32)&15;int r=mace_radius(t);
    *x=world_x(seat)+scale(kMaceCos[k],r);*y=world_y(seat)+scale(kMaceSin[k],r);
}
static void laser_position(unsigned seat,int *x,int *y) {
    uint8_t *s=seat_state(seat);unsigned d=s[S_DIR]&3;
    int reach=14+(int)get16(s+S_DIST);
    *x=world_x(seat)+kDx[d]*reach;*y=world_y(seat)+kDy[d]*reach;
}
/* True while the seat may not walk, turn or act (it stands for the move). */
static bool frozen(unsigned seat) {
    uint8_t *s=seat_state(seat);unsigned t=get16(s+S_TIMER);
    switch(s[S_KIND]) {
    case AB_SHOT: case AB_SLASH: case AB_HAMMER: case AB_BAZOOKA: case AB_GREAT: case AB_LASER: return true;
    case AB_JET: return !(s[S_FLAGS]&1) || t<(unsigned)s[S_STOP]+8u;
    default: return false;
    }
}
static void tick(unsigned seat) {
    uint8_t *s=seat_state(seat);
    if(get16(s+S_DEBUFF)) set16(s+S_DEBUFF,get16(s+S_DEBUFF)-1);
    if(get16(s+S_COOLDOWN)) set16(s+S_COOLDOWN,get16(s+S_COOLDOWN)-1);
    if(!s[S_KIND]) return;
    const Ability *a=ability_of(seat);
    if(!a || !alive(seat)) { s[S_KIND]=0;return; }
    unsigned t=get16(s+S_TIMER),d=s[S_DIR]&3;
    set16(s+S_TIMER,t+1);
    int x=world_x(seat),y=world_y(seat),cx=cell_of(x),cy=cell_of(y);
    switch(s[S_KIND]) {
    case AB_SHOT:
        if(t==SHOT_AT) {
            for(int k=1;k<16 && !blocked_cell(cx+kDx[d]*k,cy+kDy[d]*k);++k)
                if(strike_cell(seat,cx+kDx[d]*k,cy+kDy[d]*k)) break;
        }
        if(t+1>=SHOT_FRAMES) finish(seat);
        break;
    case AB_SLASH:
        if(t>=4 && t<8) strike_cell(seat,cx+kDx[d],cy+kDy[d]);
        if(t+1>=SLASH_FRAMES) finish(seat);
        break;
    case AB_BAZOOKA:
        if(t==BEAM_FROM) s[S_BEAM]=(uint8_t)beam_length(seat);
        if(t>=BEAM_FROM && t<=BEAM_TO)
            for(int k=1;k<=s[S_BEAM];++k) strike_cell(seat,cx+kDx[d]*k,cy+kDy[d]*k);
        if(t+1>=BAZOOKA_FRAMES) finish(seat);
        break;
    case AB_GREAT:
        if(t+1>=GREAT_FRAMES) finish(seat);
        break;
    case AB_HAMMER:
        if(t>=MACE_FROM && t<=MACE_TO) {
            int mx,my;mace_position(seat,t,&mx,&my);
            unsigned found[24],n=near(seat,mx,my,12,found,24);
            for(unsigned k=0;k<n;++k) knock(seat,found[k],1);
        }
        if(t+1>=HAMMER_FRAMES) finish(seat);
        break;
    case AB_LASER:
        if(t>=LASER_FROM && !(s[S_FLAGS]&2)) {
            if(t>LASER_FROM) set16(s+S_DIST,get16(s+S_DIST)+2);
            int lx,ly;laser_position(seat,&lx,&ly);
            unsigned found[1];
            if(blocked_cell(cell_of(lx+kDx[d]*4),cell_of(ly+kDy[d]*4))) s[S_FLAGS]|=2;
            else if(near(seat,lx,ly,10,found,1)) { knock(seat,found[0],2);s[S_FLAGS]|=2; }
        }
        if(s[S_FLAGS]&2) finish(seat);
        break;
    case AB_JET: {
        uint8_t *trail=s+S_TRAIL;
        bool live=false;
        for(unsigned k=0;k<TRAILS;++k) {
            uint8_t *seg=trail+k*3;
            if(!seg[1]) continue;
            if(++seg[1]>TRAIL_LIFE) { seg[1]=0;continue; }
            live=true;
            int sx=seg[0]%16,sy=seg[0]/16;
            strike_cell(seat,sx,sy);
            if(seg[2]&1) strike_cell(seat,sx-kDx[d],sy-kDy[d]);
        }
        if(t<JET_WINDUP) { invulnerable(seat,8);break; }
        if(!(s[S_FLAGS]&1)) {
            invulnerable(seat,8);
            unsigned f=t-JET_WINDUP,here=(unsigned)((x&0xff)<<8|(y&0xff));
            /* A block stops the flight: the position stays for two frames. */
            if(f>=2 && get16(s+S_LAST)==here) { s[S_FLAGS]|=1;s[S_STOP]=(uint8_t)(t>255?255:t); }
            set16(s+S_LAST,here);
            if(!(s[S_FLAGS]&1) && f%8==0) {
                uint8_t *seg=trail+((f/8)%TRAILS)*3;
                seg[0]=(uint8_t)(cy*16+cx);seg[1]=1;seg[2]=f?1:0;live=true;
                strike_cell(seat,cx,cy);
            }
        }
        if((s[S_FLAGS]&1) && t>=(unsigned)s[S_STOP]+8u && !live) finish(seat);
        if(t>600) finish(seat);
        break;
    }
    default: break;
    }
}

/* Items lost by stunned players: the cartridge's routine $C4:37CD (A = 0,
 * X = seat; it ends in RTS) runs from the pad hook. A return frame for
 * $C0:0AF9 (RTL) and one for an RTL in bank $C4 are pushed, so the routine
 * comes back here; the hook then starts the next one or restores the
 * registers. One item per seat per frame. */
static bool injecting;
static CpuState saved_cpu;
static uint8_t drop_seats[5],drop_count,drop_next;
static void push(CpuState *cpu,uint8_t v) { g_ram[cpu->S]=v;cpu->S=(uint16_t)(cpu->S-1); }
static void inject_drop(CpuState *cpu,unsigned seat) {
    push(cpu,(uint8_t)(PAD_HOOK>>16));push(cpu,(uint8_t)((PAD_HOOK-1)>>8));push(cpu,(uint8_t)(PAD_HOOK-1));
    push(cpu,(uint8_t)((RTL_C4-1)>>8));push(cpu,(uint8_t)(RTL_C4-1));
    cpu->A=(uint16_t)(cpu->A&0xff00);cpu->X=(uint16_t)seat;
    interp_bridge_pre_opcode_redirect(LOSE_ITEM);
}
static bool next_drop(CpuState *cpu) {
    if(drop_next>=drop_count) return false;
    inject_drop(cpu,drop_seats[drop_next++]);
    return true;
}

/* $C0:0AF9: the pad reader has filled $2C/$31/$36/$3B+seat and the frame's
 * game logic has not consumed them yet. */
static void pad_hook(CpuState *cpu,uint32_t pc) {
    (void)pc;
    if(injecting) {
        if(next_drop(cpu)) return;
        injecting=false;
        cpu->A=saved_cpu.A;cpu->X=saved_cpu.X;cpu->Y=saved_cpu.Y;cpu->D=saved_cpu.D;cpu->DB=saved_cpu.DB;
        return;
    }
    if(!HoneyKotetsuInstalled() || !in_stage()) {
        /* Outside a stage every seat starts over (no debuff carried over). */
        for(unsigned b=STATE;b<STATE_END;++b) if(g_ram[b]) { memset(g_ram+STATE,0,STATE_END-STATE);break; }
        return;
    }
    if(!playing()) return;
    drop_count=drop_next=0;
    for(unsigned i=0;i<5;++i) {
        uint8_t *s=seat_state(i);
        if(s[S_DROPS]) {
            --s[S_DROPS];
            if(alive(i) && battle()) drop_seats[drop_count++]=(uint8_t)i;
        }
        const Ability *a=ability_of(i);
        if(!a) { g_ram[PENDING+i]=0;continue; }
        tick(i);
        if(s[S_KIND] && frozen(i)) {
            /* Standing for the move: no walking, turning or new actions;
             * Jet's flight keeps walking his way. */
            bool flying=s[S_KIND]==AB_JET && get16(s+S_TIMER)>JET_WINDUP && !(s[S_FLAGS]&1);
            g_ram[0x31+i]=flying?kDirBits[s[S_DIR]&3]:0;g_ram[0x3b+i]=0;g_ram[0x36+i]=0;
            g_ram[PENDING+i]=0;
            continue;
        }
        bool rider=(g_ram[0x1782+i]&0x80)!=0;
        if(rider || !alive(i) || !ready(i)) {
            if(g_ram[PENDING+i]) { g_ram[PENDING+i]=0;if(g_ram[0x2c+i]&0x08) g_ram[0x36+i]|=0x08; }
            continue;
        }
        unsigned held_b=g_ram[0x2c+i]&0x08,new_b=g_ram[0x36+i]&0x08;
        int held=direction_of(g_ram[0x31+i]&0x0f),fresh=direction_of(g_ram[0x3b+i]&0x0f);
        if(new_b) {
            g_ram[0x36+i]&=(uint8_t)~0x08;
            if(fresh>=0 || held>=0) { start(i,fresh>=0?fresh:held);g_ram[PENDING+i]=0; }
            else g_ram[PENDING+i]=B_WINDOW;
        } else if(g_ram[PENDING+i]) {
            if(!held_b) { g_ram[PENDING+i]=0;g_ram[0x36+i]|=0x08; }      /* released: native B */
            else if(fresh>=0) { g_ram[PENDING+i]=0;start(i,fresh); }
            else if(!--g_ram[PENDING+i]) g_ram[0x36+i]|=0x08;            /* no direction came */
        } else if(held_b && fresh>=0) start(i,fresh);
        if(s[S_KIND]) { g_ram[0x31+i]=0;g_ram[0x3b+i]=0;g_ram[0x36+i]=0; }
    }
    if(drop_count) { saved_cpu=*cpu;injecting=true;next_drop(cpu); }
}
/* $C4:3180 STA $1CC2: A = pixels the seat (X) walks this frame; $C4:3284
 * walks that many single-pixel steps. */
static void speed_hook(CpuState *cpu,uint32_t pc) {
    (void)pc;
    unsigned seat=cpu->X&0xff;
    if(seat>=5 || !HoneyKotetsuInstalled() || !ability_of(seat)) return;
    uint8_t *s=seat_state(seat);
    unsigned steps=cpu->A&0xff;
    if(s[S_KIND]==AB_JET && get16(s+S_TIMER)>JET_WINDUP && !(s[S_FLAGS]&1)) steps=JET_SPEED;
    else if(s[S_KIND] && frozen(seat)) steps=0;
    else if(get16(s+S_DEBUFF)) {
        unsigned total=g_ram[SPEED_ACC+seat]+steps;
        steps=total/2;g_ram[SPEED_ACC+seat]=(uint8_t)(total&1);
    } else return;
    cpu->A=(uint16_t)((cpu->A&0xff00)|steps);
    /* STA leaves the flags of the computation: the following BEQ ($C4:3183)
     * must see the new count (zero = no movement this frame). */
    cpu->_flag_Z=steps==0;cpu->_flag_N=0;
}
void SpecialAbilitiesInstall(void) {
    if(hooks_installed) return;
    hooks_installed=interp_bridge_add_pre_opcode_hook(PAD_HOOK,pad_hook) &&
                    interp_bridge_add_pre_opcode_hook(SPEED_HOOK,speed_hook);
}
bool SpecialAbilityDebuffed(unsigned seat) { return seat<5 && get16(seat_state(seat)+S_DEBUFF)!=0; }
bool SpecialAbilityShown(unsigned seat) {
    if(seat>=5 || !ability_of(seat)) return false;
    uint8_t *s=seat_state(seat);
    return s[S_KIND]==AB_JET && alive(seat) && (!(s[S_FLAGS]&1) || get16(s+S_TIMER)<(unsigned)s[S_STOP]+8u);
}

/* Super Bomberman 4 directions: up, right, down, left. */
static unsigned sb4_dir(unsigned d) { return (d+1)&3; }
int SpecialAbilityPose(unsigned seat,unsigned *step,uint32_t *sb4_frame) {
    *step=0;*sb4_frame=0;
    if(seat>=5) return -1;
    const Ability *a=ability_of(seat);
    uint8_t *s=seat_state(seat);
    if(!a || !s[S_KIND]) return -1;
    unsigned t=get16(s+S_TIMER),d=s[S_DIR]&3,k;
    switch(s[S_KIND]) {
    case AB_SHOT: *step=t<6?0:t<12?1:2;return (int)d;
    case AB_SLASH: *step=t<4?0:t<8?1:2;return (int)d;
    case AB_BAZOOKA:
        k=t<32?(sb4_dir(d)+t/8)&3:sb4_dir(d);
        *sb4_frame=0xca98b0+k*0x80;return (int)d;
    case AB_JET:
        if((s[S_FLAGS]&1) && t>=(unsigned)s[S_STOP]+8u) return -1;
        k=t<9?0:t<19?1:t<25?2:3;
        *sb4_frame=0xcad0b0+sb4_dir(d)*0x800+k*0x80;return (int)d;
    case AB_HAMMER:
        *sb4_frame=t<10?0xcd0900:t<37?0xcd0980:((t-37)/8)&1?0xcd0980:0xcd1000;return (int)d;
    case AB_LASER:
        k=t<LASER_POSE?t/7:5;
        *sb4_frame=0xcaa0b0+k*0x800+sb4_dir(d)*0x80;return (int)d;
    case AB_GREAT:
        *sb4_frame=Sb4FramePc(5,6);return 1;
    default: return -1;
    }
}

/* ---- Effects: Super Bomberman 4 graphics, drawn by the overlay. ---- */
static int screen_x(int wx) { return wx-(int16_t)(g_ram[0x1d3b]|g_ram[0x1d3c]<<8); }
static int screen_y(int wy) { return wy-(int16_t)(g_ram[0x1d3d]|g_ram[0x1d3e]<<8); }
/* Fire block kinds (low nibble; the phase 2-A is the high nibble): centre
 * $C, horizontal middle $A, right end $6 (left end flipped), vertical middle
 * $8, top end $0 (bottom end flipped). */
static void flip(uint8_t px[16][16],bool h,bool v) {
    uint8_t out[16][16];
    for(unsigned y=0;y<16;++y) for(unsigned x=0;x<16;++x) out[y][x]=px[v?15-y:y][h?15-x:x];
    memcpy(px,out,sizeof(out));
}
static void draw_block(int wx,int wy,uint8_t px[16][16],const uint16_t *colors,unsigned priority) {
    ExtraEmitPixels(screen_x(wx)-8,screen_y(wy)-8,&px[0][0],16,16,colors,priority);
}
/* `end`: the tile is the tip of a line running in direction d (the tip
 * points that way). */
static void draw_fire(int cx,int cy,unsigned d,bool end,unsigned phase,const uint16_t *colors) {
    bool horizontal=!(d&1);
    unsigned kind=end?(horizontal?6:0):(horizontal?0xa:8);
    uint8_t px[16][16];
    if(!Sb4FireBlock(phase<<4|kind,px)) return;
    if(end) flip(px,d==2,d==1);
    draw_block(cx*16,cy*16,px,colors,2);
}
static const uint8_t kBeamPhase[15]={2,2,4,4,6,6,8,8,8,8,6,6,4,4,2};
static const uint8_t kTrailPhase[8]={2,4,6,8,8,6,4,2};
void SpecialAbilityDraw(void) {
    if(!HoneyKotetsuInstalled() || !in_stage()) return;
    uint16_t fire[16];
    for(unsigned c=0;c<16;++c) fire[c]=Sb4FireColor(c);
    for(unsigned i=0;i<5;++i) {
        uint8_t *s=seat_state(i);
        if(!s[S_KIND] || !alive(i)) continue;
        unsigned t=get16(s+S_TIMER),d=s[S_DIR]&3,choice=ExtraCharacterSeatChoice(i);
        int cx=cell_of(world_x(i)),cy=cell_of(world_y(i));
        uint16_t own[16];
        for(unsigned c=0;c<16;++c)
            own[c]=choice>=KINGS_CHOICE_FIRST?Sb4Color(choice-KINGS_CHOICE_FIRST,ExtraCharacterSeatColor(i)%SB4_COLORS,c):0;
        switch(s[S_KIND]) {
        case AB_BAZOOKA:
            if(t<BEAM_FROM || t>BEAM_TO) break;
            for(unsigned k=1;k<=s[S_BEAM];++k)
                draw_fire(cx+kDx[d]*(int)k,cy+kDy[d]*(int)k,d,k==s[S_BEAM],kBeamPhase[t-BEAM_FROM],fire);
            break;
        case AB_JET:
            for(unsigned k=0;k<TRAILS;++k) {
                uint8_t *seg=s+S_TRAIL+k*3;
                if(!seg[1] || seg[1]>TRAIL_LIFE) continue;
                unsigned phase=kTrailPhase[(seg[1]-1)/2];
                int sx=seg[0]%16,sy=seg[0]/16;
                draw_fire(sx,sy,d,false,phase,fire);
                if(seg[2]&1) draw_fire(sx-kDx[d],sy-kDy[d],(d+2)&3,true,phase,fire);
            }
            break;
        case AB_HAMMER: {
            if(t<MACE_FROM || t>MACE_TO) break;
            int mx,my;mace_position(i,t,&mx,&my);
            uint8_t px[16][16];
            if(Sb4EffectBlock(0x80,px)) draw_block(mx,my,px,own,3);
            break;
        }
        case AB_LASER: {
            if(t<LASER_FROM || (s[S_FLAGS]&2)) break;
            int lx,ly;laser_position(i,&lx,&ly);
            uint8_t px[16][16];
            if(Sb4EffectBlock(d&1?0x84:0x82,px)) { flip(px,d==2,d==3);draw_block(lx,ly,px,own,3); }
            break;
        }
        default: break;
        }
    }
}
