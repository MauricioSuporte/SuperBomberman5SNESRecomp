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
 *     block stops it, leaving a fire trail: every 8 frames a two-tile
 *     segment (tail end + middle) that lives 16 frames (phases 2,4,6,8,8,6,
 *     4,2), lethal. After the crash he keeps the flying pose for 20 frames.
 *     He blinks invulnerable from the trigger until about a second after the
 *     crash (the native countdown, never frozen).
 *   Hammer: stands still for 125 frames swinging the mace around himself
 *     (frames 29-106: angle -90 - 22.5 deg per frame, radius growing from 16
 *     to 48 px by frame 61, then shrinking back); a player it touches is
 *     stunned and loses an item. No debuff.
 *   Lady: stands 109 frames: a turn of the gun, the shot pose while the
 *     laser leaves 14 px in front of her at frame 48, another turn; the
 *     laser flies on its own at 2 px per frame until a block or a target; a
 *     player it hits is stunned and loses two items. No debuff.
 *   Great: stands invulnerable for 160 frames facing the camera.
 *   Honey (boss): shooting animation; the shot hits the first target in
 *     line before a block. Kotetsu (boss): sword cut on the tile in front.
 * Debuff (Super Bomberman 4: Bazooka, Jet, Great; also Honey and Kotetsu):
 * 640 frames (about 10.7 s) from the trigger at half walking speed with the
 * ability blocked. Sound effects are Super Bomberman 4's (sb4_sound.c).
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
#include "extra_characters_mod.h"
#include "sb4_rom.h"
#include "sb4_sound.h"
#include "sb5_sound.h"
#include "common_rtl.h"
#include "snes/interp_bridge.h"
#include <string.h>

enum {
    STATE = 0x1f840, SEAT_BYTES = 32, PENDING = 0x1f8e0, SPEED_ACC = 0x1f8e5, DAZED = 0x1f8ea,
    CLOCK_EVENT = 0x1f8ef, CLOCK_OWNER = 0x1f8f1, STATE_END = 0x1f8f2,
    PAD_HOOK = 0xc00af9, SPEED_HOOK = 0xc43180, DEATH_HOOK = 0xc42cfe, CLOCK_HOOK = 0xc542d3,
    LOSE_ITEM = 0xc437cd, RTL_C4 = 0xc436d1, BURN_BLOCK = 0xc5441d,
    BARRIER_SET = 0x18, BARRIER_SOUND = 0x0288, THROW_ITEM = 0xc4398c, FREEZE_PLAYER = 0xc44503, ITEM_CLOCK = 0x08,
    DEBUFF_FRAMES = 640, B_WINDOW = 8, TRAILS = 3,
};
/* Per seat: +0/1 debuff, +2/3 cooldown, +4 kind, +5/6 timer, +7 direction
 * (0 right, 1 down, 2 left, 3 up), +8/9 distance (pixels), +10..12 actors
 * already hit, +13 flags (bit 0 stopped, bit 1 projectile done), +14 frame
 * the flight stopped, +15/16 last position, +17 items this seat still has
 * to lose, +18..26 Jet trail (TRAILS x: cell, age, flags), +27 beam tiles. */
enum { S_DEBUFF = 0, S_COOLDOWN = 2, S_KIND = 4, S_TIMER = 5, S_DIR = 7, S_DIST = 8, S_HIT = 10,
       S_FLAGS = 13, S_STOP = 14, S_LAST = 15, S_DROPS = 17, S_TRAIL = 18, S_BEAM = 27, S_SHOT = 28,
       /* Terrorin (over the Jet trail bytes): barrier centre cell and the
        * 21 barrier cells this seat raised (bit mask). */
       S_WALL_CELL = 18, S_WALL_MASK = 19 };
enum { AB_NONE, AB_SHOT, AB_SLASH, AB_HAMMER, AB_JET, AB_LASER, AB_BAZOOKA, AB_GREAT, AB_BARRIER };
typedef struct { uint8_t kind; uint16_t debuff; uint16_t cooldown; } Ability;
/* Index: choice - 11 (Honey, Kotetsu, Bomberman, Hammer, Jet, Lady, Bazooka,
 * Great, Terrorin). */
static const Ability kAbility[9]={
    {AB_SHOT,   DEBUFF_FRAMES, 0},
    {AB_SLASH,  DEBUFF_FRAMES, 0},
    {AB_NONE,   0,             0},
    {AB_HAMMER, 0,            30},
    {AB_JET,    DEBUFF_FRAMES, 0},
    {AB_LASER,  0,            30},
    {AB_BAZOOKA,DEBUFF_FRAMES, 0},
    {AB_GREAT,  DEBUFF_FRAMES, 0},
    {AB_BARRIER,DEBUFF_FRAMES, 0},
};
enum { SHOT_FRAMES = 55, SHOT_AT = 32, SHOT_SPEED = 4, SLASH_FRAMES = 61, SLASH_FROM = 13, BAZOOKA_FRAMES = 98, BEAM_FROM = 68, BEAM_TO = 82,
       JET_WINDUP = 48, JET_SPEED = 4, TRAIL_LIFE = 16, HAMMER_FRAMES = 125, MACE_FROM = 29, MACE_TO = 106,
       LASER_FROM = 48, LADY_FRAMES = 109, GREAT_FRAMES = 160, JET_LANDING = 24, JET_GUARD_AFTER = 60, JET_TRAIL_STOP = 4,
       /* Terrorin's barrier, as in ZONE5 - J ($C3:3F2D-$C3:4090): spin
        * $40 frames, raise the barrier and fade the blue in over 16, hold
        * $40, the Clock flies off ($C3:4010), fade out over 16 from frame
        * 176 and drop it at 192. */
       WALL_UP = 64, WALL_FADE = 16, WALL_CLOCK = 144, WALL_DOWN = 176, WALL_FRAMES = 192, DAZED_FRAMES = 0x90 };
/* Super Bomberman 4 sound effects ($C0:071D numbers). */
enum { SND_EXPLOSION = 0x39, SND_MACE = 0x4e, SND_LASER = 0x5a, SND_GUARD = 0x5f };
static uint8_t *seat_state(unsigned i) { return g_ram+STATE+i*SEAT_BYTES; }
static unsigned get16(const uint8_t *p) { return p[0]|p[1]<<8; }
static void set16(uint8_t *p,unsigned v) { p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8); }

static bool hooks_installed;
static const Ability *ability_of(unsigned seat) {
    unsigned choice=ExtraCharacterSeatChoice(seat);
    if(choice<HK_CHOICE_HONEY || choice>EXTRA_CHOICE_LAST) return NULL;
    if(choice==TERRORIN_CHOICE && !Sb5ExtrasEnabled()) return NULL;
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
/* Rival bombers of the boss stages ($101F set: ZONE3 - D's Pirate Bomber
 * and his partner, the other rival bosses) ignore the death bit: their
 * update clears $15A2 bit 6 every frame ($C4:0164), and only a flame on
 * their cell hurts them ($C4:2CFE: $1CCA bit 2 -> mount lost, HP $1662,
 * 120 frames of invulnerability, death and the boss script's ending).
 * Their hits wait here until their own fire check on this frame. */
static uint8_t fire_hits;
static bool rival(unsigned actor) { return actor<5 && g_ram[0x101f+actor]; }
/* A Louie lives in its rider's mount slot (rider + $1CBF, the rider a
 * bomber 0-4) and is never a target of its own: the rider takes the hit and
 * the cartridge plays the Louie's loss ($C4:2D05 -> $C4:51AB, removed about
 * 47 frames later). Setting the mount's death bit, mounted or during that
 * loss, left ZONE3 - D without its exit gates. */
static bool mount(unsigned actor) {
    unsigned base=g_ram[0x1cbf];
    if(actor<base || actor-base>=5 || actor-base==actor) return false;
    return g_ram[0x17fa+actor-base]<0x80;
}
static void damage(unsigned actor) {
    if(actor<5) {
        if(g_ram[0x15a2+actor]&0x04) return;
        if(rival(actor)) fire_hits|=(uint8_t)(1u<<actor);
        else g_ram[0x15a2+actor]|=0x40;
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
        if(!alive(a) || !hostile(seat,a) || mount(a)) continue;
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
        if(!alive(a) || !hostile(seat,a) || mount(a)) continue;
        if(cell_of(world_x(a))==cx && cell_of(world_y(a))==cy) { damage(a);hit=true; }
    }
    return hit;
}
static const int kDx[4]={1,0,-1,0},kDy[4]={0,1,0,-1};
/* Stage map: $1052, 16 columns; $80 hard block, $40 bomb, $30 soft block,
 * $10 alone a revealed item (verified by bomb/item/block dumps). */
static unsigned map_cell(int cx,int cy) {
    if(cx<0 || cy<0 || cx>=16 || cy>=13) return 0x80;
    return g_ram[0x1052+cy*16+cx];
}
static bool blocked_cell(int cx,int cy) { return (map_cell(cx,cy)&0xe0)!=0; }
static bool soft_cell(int cx,int cy) { unsigned v=map_cell(cx,cy);return !(v&0xc0) && (v&0x20); }
/* A bomb in the bomb list (head $0E2A, next $0EA4, cell $0E7C, fuse $0E40;
 * $FF = remote, which $C5:1F84 detonates by setting 5) explodes at once. */
static void detonate(int cx,int cy) {
    unsigned cell=(unsigned)(cy*16+cx),x=g_ram[0xe2a];
    for(unsigned n=0;x<0x14 && n<0x14;++n,x=g_ram[0xea4+x]) {
        if(g_ram[0xe7c+x]!=cell || (g_ram[0xe90+x]&0xe8)) continue;
        if(g_ram[0xe40+x]==0xff) g_ram[0xe40+x]=5;
        else if(g_ram[0xe40+x]>1) g_ram[0xe40+x]=1;
    }
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
/* Keeps the native invulnerability running for at least `frames` more
 * frames without disturbing its blink: the cartridge blinks on bits 0-1 of
 * the countdown ($C4:2E42), so it only ever grows in steps of 4. */
static void extend_invulnerability(unsigned seat,unsigned frames) {
    g_ram[0x15a2+seat]|=0x04;
    unsigned v=g_ram[0x170a+seat];
    while(v<frames && v+4<=255) v+=4;
    g_ram[0x170a+seat]=(uint8_t)v;
}
static void start(unsigned seat,int direction) {
    const Ability *a=ability_of(seat);
    uint8_t *s=seat_state(seat);
    uint8_t drops=s[S_DROPS];
    memset(s+S_KIND,0,SEAT_BYTES-S_KIND);
    s[S_DROPS]=drops;
    s[S_KIND]=a->kind;s[S_DIR]=(uint8_t)direction;
    /* Super Bomberman 4 starts the debuff counter with the move ($E2,X =
     * 160 on the trigger frame), not when it ends. */
    set16(s+S_DEBUFF,a->debuff);
    if(a->kind==AB_GREAT) { invulnerable(seat,GREAT_FRAMES);Sb4SoundPlay(SND_GUARD); }
    if(a->kind==AB_JET) invulnerable(seat,JET_WINDUP+8);
}
static void finish(unsigned seat) {
    const Ability *a=ability_of(seat);
    uint8_t *s=seat_state(seat);
    s[S_KIND]=0;
    if(a) set16(s+S_COOLDOWN,a->cooldown);
}
/* Fire beam of Bazooka, like a bomb's flame to one side: from the next
 * tile up to a hard block; a soft block is the last tile (it burns, the
 * cartridge's own $C5:441D), a bomb stops it and explodes. */
static unsigned beam_length(unsigned seat,int *soft) {
    unsigned d=seat_state(seat)[S_DIR]&3,k=0;
    int cx=cell_of(world_x(seat)),cy=cell_of(world_y(seat));
    *soft=-1;
    while(k<15) {
        int nx=cx+kDx[d]*(int)(k+1),ny=cy+kDy[d]*(int)(k+1);
        unsigned v=map_cell(nx,ny);
        if(v&0x80) break;
        if(v&0x40) { detonate(nx,ny);break; }
        ++k;
        if(soft_cell(nx,ny)) { *soft=ny*16+nx;break; }
    }
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
    case AB_SLASH: case AB_HAMMER: case AB_BAZOOKA: case AB_GREAT: case AB_BARRIER: return true;
    case AB_SHOT: return t<SHOT_FRAMES;
    case AB_LASER: return t<LADY_FRAMES;
    case AB_JET: return !(s[S_FLAGS]&1) || t<(unsigned)s[S_STOP]+JET_LANDING;
    default: return false;
    }
}
/* Cartridge routines called from the pad hook: items lost by stunned
 * players ($C4:37CD, A = 0, X = seat; ends in RTS) and soft blocks burnt by
 * Bazooka's beam ($C5:441D, A = cell; RTL). Return frames for $C0:0AF9 (and
 * for an RTL in bank $C4) are pushed, so each routine comes back to the
 * hook, which starts the next one or restores the registers. */
static bool injecting,scratch_saved;
static uint8_t saved_8d,saved_1cc7;
static CpuState saved_cpu;
/* `ah` >= 0: the whole accumulator is (ah << 8) | a (a sound command);
 * `ae` >= 0: direct-page $AE is set first (the item placer's cell). */
typedef struct { uint32_t routine;uint8_t a,x;bool rts;int16_t ah,ae,actor; } Call;
static Call calls[24];
static uint8_t call_count,call_next;
static void queue_call(uint32_t routine,uint8_t a,uint8_t x,bool rts) {
    if(call_count<sizeof(calls)/sizeof(*calls)) calls[call_count++]=(Call){routine,a,x,rts,-1,-1,-1};
}
/* $C4:398C (RTS, bank C4) with A = item, $8D = seat, $1CC7 = 0: the item
 * flies off from the seat's cell. $8D and $1CC7 are restored afterwards. */
static void queue_throw(unsigned seat,uint8_t item) {
    if(call_count<sizeof(calls)/sizeof(*calls)) calls[call_count++]=(Call){THROW_ITEM,item,(uint8_t)seat,true,-1,-1,(int16_t)seat};
}
static void push(CpuState *cpu,uint8_t v) { g_ram[cpu->S]=v;cpu->S=(uint16_t)(cpu->S-1); }
/* A JSL-style routine gets a return frame for $C0:0AF9; an RTS routine in
 * bank $C4 also one for the RTL at $C4:36D1. */
static void inject(CpuState *cpu,const Call *c) {
    push(cpu,(uint8_t)(PAD_HOOK>>16));push(cpu,(uint8_t)((PAD_HOOK-1)>>8));push(cpu,(uint8_t)(PAD_HOOK-1));
    if(c->rts) { push(cpu,(uint8_t)((RTL_C4-1)>>8));push(cpu,(uint8_t)(RTL_C4-1)); }
    cpu->A=(uint16_t)(c->ah>=0?((unsigned)c->ah<<8)|c->a:(cpu->A&0xff00)|c->a);cpu->X=(uint16_t)c->x;
    if(c->ae>=0) g_ram[(cpu->D+0xae)&0xffff]=(uint8_t)c->ae;
    if(c->actor>=0) {
        if(!scratch_saved) { saved_8d=g_ram[(cpu->D+0x8d)&0xffff];saved_1cc7=g_ram[0x1cc7];scratch_saved=true; }
        g_ram[(cpu->D+0x8d)&0xffff]=(uint8_t)c->actor;g_ram[0x1cc7]=0;
    }
    interp_bridge_pre_opcode_redirect(c->routine);
}
static bool next_call(CpuState *cpu) {
    if(call_next>=call_count) return false;
    inject(cpu,&calls[call_next++]);
    return true;
}

/* ---- Terrorin's barrier. ---- */
/* Cells of the barrier around its centre (the table at $C3:4091): the
 * 5 x 5 square without the four pillar positions inside it, so it fences
 * the 3 x 3 square he stands in. */
static const int8_t kWallDx[21]={-2,-1,0,1,2, -2,0,2, -2,-1,0,1,2, -2,0,2, -2,-1,0,1,2};
static const int8_t kWallDy[21]={-2,-2,-2,-2,-2, -1,-1,-1, 0,0,0,0,0, 1,1,1, 2,2,2,2,2};
static uint32_t wall_mask(const uint8_t *s) { return s[S_WALL_MASK]|(uint32_t)s[S_WALL_MASK+1]<<8|(uint32_t)s[S_WALL_MASK+2]<<16; }
static int wall_cell(const uint8_t *s,unsigned k) {
    int cx=s[S_WALL_CELL]%16+kWallDx[k],cy=s[S_WALL_CELL]/16+kWallDy[k];
    return cx<0 || cx>15 || cy<0 || cy>12?-1:cy*16+cx;
}
/* Raises it: like the cartridge ($C3:3FAB: ORA #$0C) on every free cell
 * (bit 2: lethal like fire, $C4:2CF0 via $1CCA; bit 3: solid). Cells with
 * a block, bomb or item are left as they are. */
static void raise_wall(unsigned seat) {
    uint8_t *s=seat_state(seat);uint32_t mask=0;
    s[S_WALL_CELL]=g_ram[0x16aa+seat];
    for(unsigned k=0;k<21;++k) {
        int c=wall_cell(s,k);
        if(c<0) continue;
        uint8_t *v=&g_ram[0x1052+c];
        if(*v&0xfc) continue;
        *v|=0x0c;mask|=1u<<k;
    }
    s[S_WALL_MASK]=(uint8_t)mask;s[S_WALL_MASK+1]=(uint8_t)(mask>>8);s[S_WALL_MASK+2]=(uint8_t)(mask>>16);
}
/* Drops the cells this seat raised. Fire that crossed them leaves its own
 * low bits behind ($0F, then $08), so a cell still empty above them is
 * cleared whole, as the cartridge does ($C3:4084). */
static void drop_wall(unsigned seat) {
    uint8_t *s=seat_state(seat);uint32_t mask=wall_mask(s);
    for(unsigned k=0;k<21;++k) if(mask&(1u<<k)) {
        int c=wall_cell(s,k);
        if(c<0) continue;
        uint8_t *v=&g_ram[0x1052+c];
        *v=(uint8_t)((*v&0xf0)?*v&~0x0c:0);
    }
    s[S_WALL_MASK]=s[S_WALL_MASK+1]=s[S_WALL_MASK+2]=0;
}
/* True when `cell` is one of the barrier cells raised by a seat that does
 * not fight `actor` (itself, a Normal Game ally, a Team Battle teammate). */
static bool friendly_wall(unsigned actor,unsigned cell) {
    for(unsigned t=0;t<5;++t) {
        uint8_t *s=seat_state(t);uint32_t mask=wall_mask(s);
        if(s[S_KIND]!=AB_BARRIER || !mask || (t!=actor && hostile(t,actor))) continue;
        for(unsigned k=0;k<21;++k) if((mask&(1u<<k)) && wall_cell(s,k)==(int)cell) return true;
    }
    return false;
}
/* Blue of the barrier: the cartridge fades the fixed color's blue ($0317)
 * from 0 to 16 and back. */
static unsigned wall_blue(unsigned t) {
    if(t<WALL_UP) return 0;
    if(t<WALL_UP+WALL_FADE) return t-WALL_UP+1;
    if(t<WALL_DOWN) return 16;
    return t<WALL_FRAMES?WALL_FRAMES-t:0;
}
static void tick(unsigned seat) {
    uint8_t *s=seat_state(seat);
    if(get16(s+S_DEBUFF)) set16(s+S_DEBUFF,get16(s+S_DEBUFF)-1);
    if(get16(s+S_COOLDOWN)) set16(s+S_COOLDOWN,get16(s+S_COOLDOWN)-1);
    if(!s[S_KIND]) return;
    const Ability *a=ability_of(seat);
    if(!a || !alive(seat)) { if(s[S_KIND]==AB_BARRIER) drop_wall(seat);s[S_KIND]=0;return; }
    unsigned t=get16(s+S_TIMER),d=s[S_DIR]&3;
    set16(s+S_TIMER,t+1);
    int x=world_x(seat),y=world_y(seat),cx=cell_of(x),cy=cell_of(y);
    switch(s[S_KIND]) {
    case AB_SHOT:
        /* Honey's boss attack: the shooting animation ($23-$26, 55 frames,
         * muzzle flash at 32) and a bullet ($27-$2A) that flies on until a
         * block or a target. */
        if(t==SHOT_AT) { s[S_FLAGS]|=4;set16(s+S_DIST,8); }
        else if(s[S_FLAGS]&4) set16(s+S_DIST,get16(s+S_DIST)+SHOT_SPEED);
        if(s[S_FLAGS]&4) {
            int bx=x+kDx[d]*(int)get16(s+S_DIST),by=y+kDy[d]*(int)get16(s+S_DIST);
            unsigned found[1];
            if(blocked_cell(cell_of(bx+kDx[d]*4),cell_of(by+kDy[d]*4))) s[S_FLAGS]&=(uint8_t)~4;
            else if(near(seat,bx,by,10,found,1)) { damage(found[0]);s[S_FLAGS]&=(uint8_t)~4; }
        }
        if(t+1>=SHOT_FRAMES && !(s[S_FLAGS]&4)) finish(seat);
        if(t>600) finish(seat);
        break;
    case AB_SLASH:
        /* Kotetsu's thrust ($30-$33: 5, 8 and 48 frames): the blade is out
         * on the tile in front for the last 48. */
        if(t>=SLASH_FROM) strike_cell(seat,cx+kDx[d],cy+kDy[d]);
        if(t+1>=SLASH_FRAMES) finish(seat);
        break;
    case AB_BAZOOKA:
        if(t==BEAM_FROM-1) Sb4SoundPlay(SND_EXPLOSION);
        if(t==BEAM_FROM) {
            int soft;s[S_BEAM]=(uint8_t)beam_length(seat,&soft);
            if(soft>=0) queue_call(BURN_BLOCK,(uint8_t)soft,0,false);
        }
        if(t>=BEAM_FROM && t<=BEAM_TO)
            for(int k=1;k<=s[S_BEAM];++k) strike_cell(seat,cx+kDx[d]*k,cy+kDy[d]*k);
        if(t+1>=BAZOOKA_FRAMES) finish(seat);
        break;
    case AB_GREAT:
        if(t+1>=GREAT_FRAMES) finish(seat);
        break;
    case AB_BARRIER:
        /* Terrorin spins in place (untouchable), raises the barrier around
         * the 3 x 3 square he stands in, puts
         * a Clock on a free cell and drops the barrier. */
        if(t==WALL_UP) { raise_wall(seat);Sb5StageSoundPlay(BARRIER_SET,BARRIER_SOUND); }
        if(t==WALL_CLOCK) {
            /* The Clock flies off like an item a player loses ($C4:398C
             * with $1CC7 = 0: flight script $73-$76 or $77-$7A, a random
             * side, landing on a random free cell; the boss's own flight is
             * script $74 with the same item drop). */
            queue_throw(seat,ITEM_CLOCK);
            g_ram[CLOCK_OWNER]=(uint8_t)(seat+1);
        }
        if(t+1>=WALL_FRAMES) { drop_wall(seat);finish(seat); }
        break;
    case AB_HAMMER:
        if(t>=28 && t<=92 && (t-28)%16==0) Sb4SoundPlay(SND_MACE);
        if(t>=MACE_FROM && t<=MACE_TO) {
            int mx,my;mace_position(seat,t,&mx,&my);
            unsigned found[24],n=near(seat,mx,my,12,found,24);
            for(unsigned k=0;k<n;++k) knock(seat,found[k],1);
        }
        if(t+1>=HAMMER_FRAMES) finish(seat);
        break;
    case AB_LASER:
        /* Lady stands for 109 frames (two turns of the gun around the
         * shot); the laser flies on its own until a block or a target. */
        if(t==LASER_FROM-1) Sb4SoundPlay(SND_LASER);
        if(t>=LASER_FROM && !(s[S_FLAGS]&2)) {
            if(t>LASER_FROM) set16(s+S_DIST,get16(s+S_DIST)+2);
            int lx,ly;laser_position(seat,&lx,&ly);
            unsigned found[1];
            if(blocked_cell(cell_of(lx+kDx[d]*4),cell_of(ly+kDy[d]*4))) s[S_FLAGS]|=2;
            else if(near(seat,lx,ly,10,found,1)) { knock(seat,found[0],2);s[S_FLAGS]|=2; }
        }
        if((s[S_FLAGS]&2) && t+1>=LADY_FRAMES) finish(seat);
        if(t>600) finish(seat);
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
        /* Invulnerable (blinking) from the wind-up until about a second
         * after he lands, as in Super Bomberman 4. */
        if(t<JET_WINDUP) { extend_invulnerability(seat,8);break; }
        /* His body (flying, and against the obstacle) hits what it touches. */
        if(!(s[S_FLAGS]&1) || t<(unsigned)s[S_STOP]+JET_LANDING) {
            unsigned found[24],n=near(seat,x+kDx[d]*4,y+kDy[d]*4,14,found,24);
            for(unsigned k=0;k<n;++k) damage(found[k]);
        }
        if(!(s[S_FLAGS]&1)) {
            extend_invulnerability(seat,8);
            /* A block stops the flight: the position along the flight stays
             * for two frames (sliding round a corner does not count). */
            unsigned f=t-JET_WINDUP,here=(unsigned)((d&1)?y:x)&0xffff;
            if(f>=2 && get16(s+S_LAST)==here) {
                s[S_FLAGS]|=1;s[S_STOP]=(uint8_t)(t>255?255:t);
                extend_invulnerability(seat,JET_LANDING+JET_GUARD_AFTER);
            }
            set16(s+S_LAST,here);
        }
        /* A flame pair every 8 frames behind him, also while he is pressed
         * against the obstacle: Super Bomberman 4 keeps blowing fire there
         * until he lands (stop at frame 96, new pairs up to about 116,
         * the last one gone at 136; arena tilemap $7E2000). */
        if(t>=JET_WINDUP && (!(s[S_FLAGS]&1) || t+JET_TRAIL_STOP<(unsigned)s[S_STOP]+JET_LANDING)) {
            unsigned f=t-JET_WINDUP;
            if(f%8==0) {
                uint8_t *seg=trail+((f/8)%TRAILS)*3;
                seg[0]=(uint8_t)(cy*16+cx);seg[1]=1;seg[2]=f?1:0;live=true;
                strike_cell(seat,cx,cy);
                Sb4SoundPlay(SND_EXPLOSION);
            }
        }
        /* After the crash he stays against the block in the flying pose
         * for 24 frames (SB4: stops at frame 96, walks again at 120). */
        if((s[S_FLAGS]&1) && t+1>=(unsigned)s[S_STOP]+JET_LANDING && !live) finish(seat);
        if(t>600) finish(seat);
        break;
    }
    default: break;
    }
}

/* Terrorin's Clock in Battle (the cartridge's rule in ZONE5 - J: the
 * Clock he picks up paralyzes the players, $C3:3E06 -> $C4:3A94 ->
 * $C4:4503; the one a player picks up paralyzes him). The item's own
 * effect ($C5:42D3: freeze every enemy actor) has nothing to freeze in
 * Battle; the pickup hook below records who took whose Clock and this
 * applies the paralysis ($C4:4503, X = seat, 144 frames) on the next
 * frame. In Normal Game the Clock keeps its stock effect on the enemies. */
static void freeze_seat(unsigned seat) {
    if(!alive(seat) || (g_ram[0x1782+seat]&0x82)) return;
    queue_call(FREEZE_PLAYER,0,(uint8_t)seat,false);
    g_ram[DAZED+seat]=DAZED_FRAMES;
}
static void clock_rules(void) {
    for(unsigned i=0;i<5;++i) if(g_ram[DAZED+i]) --g_ram[DAZED+i];
    unsigned owner=g_ram[CLOCK_EVENT],picker=g_ram[CLOCK_EVENT+1];
    g_ram[CLOCK_EVENT]=g_ram[CLOCK_EVENT+1]=0;
    if(!owner || !picker || owner>5 || picker>5) return;
    --owner;--picker;
    /* The item's own effect also stopped the Battle clock ($0E20, 960
     * frames, for the enemies' freeze); Terrorin's Clock only paralyzes. */
    g_ram[0xe20]=g_ram[0xe21]=0;
    if(picker==owner) { for(unsigned a=0;a<5;++a) if(hostile(owner,a)) freeze_seat(a); }
    else if(hostile(owner,picker)) freeze_seat(owner);
}
static void clock_hook(CpuState *cpu,uint32_t pc) {
    (void)pc;
    if(!ExtraCharactersInstalled() || !battle()) return;
    unsigned picker=g_ram[(cpu->D+0x8d)&0xffff];
    if(picker>=5) return;
    /* Only Terrorin brings Clocks into Battle: the last one he threw is
     * the one taken (a burnt Clock is replaced by his next). */
    unsigned owner=g_ram[CLOCK_OWNER];
    if(!owner || owner>5) return;
    g_ram[CLOCK_OWNER]=0;g_ram[CLOCK_EVENT]=(uint8_t)owner;g_ram[CLOCK_EVENT+1]=(uint8_t)(picker+1);
}
/* $C4:2CFE AND #$04: A = $1CCA, the flags of the cell under player X (bit
 * 2: fire, lethal). Terrorin is untouchable while he uses his barrier, and
 * the barrier only hurts those who fight its owner. */
static void death_hook(CpuState *cpu,uint32_t pc) {
    (void)pc;
    unsigned actor=cpu->X&0xff;
    if(actor>=5 || !ExtraCharactersInstalled()) return;
    if((fire_hits>>actor)&1) { fire_hits&=(uint8_t)~(1u<<actor);if(rival(actor)) { cpu->A|=4;return; } }
    if(!(cpu->A&4)) return;
    unsigned cell=g_ram[0x16aa+actor],v=g_ram[0x1052+cell];
    if(seat_state(actor)[S_KIND]==AB_BARRIER || ((v&0x0f)==0x0c && friendly_wall(actor,cell)))
        cpu->A=(uint16_t)(cpu->A&~4u);
}
/* $C0:0AF9: the pad reader has filled $2C/$31/$36/$3B+seat and the frame's
 * game logic has not consumed them yet. */
static void pad_hook(CpuState *cpu,uint32_t pc) {
    (void)pc;
    if(injecting) {
        if(next_call(cpu)) return;
        injecting=false;
        cpu->A=saved_cpu.A;cpu->X=saved_cpu.X;cpu->Y=saved_cpu.Y;cpu->D=saved_cpu.D;cpu->DB=saved_cpu.DB;
        if(scratch_saved) { g_ram[(cpu->D+0x8d)&0xffff]=saved_8d;g_ram[0x1cc7]=saved_1cc7;scratch_saved=false; }
        return;
    }
    if(!ExtraCharactersInstalled() || !in_stage()) {
        /* Outside a stage every seat starts over (no debuff carried over). */
        for(unsigned b=STATE;b<STATE_END;++b) if(g_ram[b]) { memset(g_ram+STATE,0,STATE_END-STATE);break; }
        return;
    }
    fire_hits=0;
    if(!playing()) return;
    call_count=call_next=0;
    clock_rules();
    for(unsigned i=0;i<5;++i) {
        uint8_t *s=seat_state(i);
        if(s[S_DROPS]) {
            --s[S_DROPS];
            if(alive(i) && battle()) queue_call(LOSE_ITEM,0,(uint8_t)i,true);
        }
        const Ability *a=ability_of(i);
        if(!a) { g_ram[PENDING+i]=0;continue; }
        /* Rendered once, on the stage's first frame rather than mid-move. */
        if(a->kind==AB_BARRIER) Sb5StageSoundPrepare(BARRIER_SET,BARRIER_SOUND);
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
    if(call_count) { saved_cpu=*cpu;injecting=true;next_call(cpu); }
}
/* $C4:3180 STA $1CC2: A = pixels the seat (X) walks this frame; $C4:3284
 * walks that many single-pixel steps. */
static void speed_hook(CpuState *cpu,uint32_t pc) {
    (void)pc;
    unsigned seat=cpu->X&0xff;
    if(seat>=5 || !ExtraCharactersInstalled() || !ability_of(seat)) return;
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
/* Jet is untouchable while he uses his ability (SB4: invulnerable): the
 * cartridge's kill stores ($15A2 |= $40 for actor X or Y; enemy contact,
 * explosions, gimmicks) leave bit 6 out for his seat. Found by scanning the
 * JPN image for LDA #$40 / ORA $15A2,idx / STA $15A2,idx and ORA #$40 /
 * STA $15A2,X; verified in Zone 1-1 (an enemy in his path used to kill
 * him). */
static const uint32_t kKillStores[]={
    0xc119a6,0xc31197,0xc313a4,0xc315b6,0xc32cae,0xc33ca2,0xc34d0a,0xc5594a,0xc5595b,
    0xc02daa,0xc03748,0xc03775,0xc32422,0xc32699,0xc326b7,0xc3363e,0xc337d4,
    0xc316da,0xc317f2,0xc317fb,0xc32133,0xc442e5,
};
static bool shielded(unsigned seat) {
    if(seat>=5 || !ability_of(seat)) return false;
    uint8_t *s=seat_state(seat);
    return s[S_KIND]==AB_JET || s[S_KIND]==AB_BARRIER;
}
static void kill_hook(CpuState *cpu,uint32_t pc) {
    unsigned op=g_rom[((pc>>16)&0x3f)<<16|(pc&0xffff)];
    unsigned actor=(op==0x99?cpu->Y:cpu->X)&0xff;
    if(ExtraCharactersInstalled() && shielded(actor)) cpu->A=(uint16_t)(cpu->A&~0x40u);
}
void SpecialAbilitiesInstall(void) {
    if(hooks_installed) return;
    hooks_installed=interp_bridge_add_pre_opcode_hook(PAD_HOOK,pad_hook) &&
                    interp_bridge_add_pre_opcode_hook(SPEED_HOOK,speed_hook);
    if(!g_rom) return;
    /* Only on the expected instructions: AND #$04 and PHX / LDA $8D. */
    unsigned death=((DEATH_HOOK>>16)&0x3f)<<16|(DEATH_HOOK&0xffff),clock=((CLOCK_HOOK>>16)&0x3f)<<16|(CLOCK_HOOK&0xffff);
    if(g_rom[death]==0x29 && g_rom[death+1]==0x04) interp_bridge_add_pre_opcode_hook(DEATH_HOOK,death_hook);
    if(g_rom[clock]==0xda && g_rom[clock+1]==0xa5 && g_rom[clock+2]==0x8d) interp_bridge_add_pre_opcode_hook(CLOCK_HOOK,clock_hook);
    if(!g_rom) return;
    for(unsigned k=0;k<sizeof(kKillStores)/sizeof(*kKillStores);++k) {
        uint32_t pc=kKillStores[k];unsigned at=((pc>>16)&0x3f)<<16|(pc&0xffff);
        if((g_rom[at]==0x99 || g_rom[at]==0x9d) && g_rom[at+1]==0xa2 && g_rom[at+2]==0x15)
            interp_bridge_add_pre_opcode_hook(pc,kill_hook);
    }
}
bool SpecialAbilityDebuffed(unsigned seat) { return seat<5 && get16(seat_state(seat)+S_DEBUFF)!=0; }
bool SpecialAbilityDazed(unsigned seat) { return seat<5 && g_ram[DAZED+seat]!=0; }


/* Lady's gun turns (k of $CA:A0B0 + k*$800 + dir*$80), measured frame by
 * frame in Super Bomberman 4: a turn of the gun, the shot pose held while
 * the laser leaves (frame 48), another turn, then she walks again. */
static unsigned lady_pose(unsigned t) {
    static const uint8_t until[]={4,9,15,19,24,29,34,39,80,87,90,96,100,105,109};
    static const uint8_t pose[]={2,3,4,0,1,2,3,4,5,0,1,2,3,4,0};
    for(unsigned i=0;i<sizeof(until);++i) if(t<until[i]) return pose[i];
    return 0;
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
    case AB_SHOT:
        if(t>=SHOT_FRAMES) return -1;
        *step=t<5?0:t<16?1:t<32?2:t<35?3:t<39?4:5;return (int)d;
    case AB_SLASH: *step=t<5?0:t<SLASH_FROM?1:2;return (int)d;
    case AB_BAZOOKA:
        k=t<32?(sb4_dir(d)+t/8)&3:sb4_dir(d);
        *sb4_frame=0xca98b0+k*0x80;return (int)d;
    case AB_JET:
        if((s[S_FLAGS]&1) && t>=(unsigned)s[S_STOP]+JET_LANDING) return -1;
        k=t<9?0:t<19?1:t<25?2:3;
        *sb4_frame=0xcad0b0+sb4_dir(d)*0x800+k*0x80;return (int)d;
    case AB_HAMMER:
        *sb4_frame=t<10?0xcd0900:t<37?0xcd0980:((t-37)/8)&1?0xcd0980:0xcd1000;return (int)d;
    case AB_LASER:
        if(t>=LADY_FRAMES) return -1;
        k=lady_pose(t);
        *sb4_frame=0xcaa0b0+k*0x800+sb4_dir(d)*0x80;return (int)d;
    case AB_GREAT:
        *sb4_frame=Sb4FramePc(5,6);return 1;
    case AB_BARRIER: *step=t;return (int)d;
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
    if(!ExtraCharactersInstalled() || !in_stage()) return;
    uint16_t fire[16];
    for(unsigned c=0;c<16;++c) fire[c]=Sb4FireColor(c);
    for(unsigned i=0;i<5;++i) {
        uint8_t *s=seat_state(i);
        if(!s[S_KIND] || !alive(i)) continue;
        unsigned t=get16(s+S_TIMER),d=s[S_DIR]&3,choice=ExtraCharacterSeatChoice(i);
        int cx=cell_of(world_x(i)),cy=cell_of(world_y(i));
        uint16_t own[16];
        for(unsigned c=0;c<16;++c)
            own[c]=choice>=KINGS_CHOICE_FIRST && choice<=KINGS_CHOICE_LAST?Sb4Color(choice-KINGS_CHOICE_FIRST,ExtraCharacterSeatColor(i)%SB4_COLORS,c):0;
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
        case AB_BARRIER: {
            /* The cartridge's box: 40 pixels around him ($C3:3F4C). */
            unsigned blue=wall_blue(t);
            if(blue && wall_mask(s)) {
                /* Inside the arena (its border row at most): never the HUD. */
                int sx=screen_x((s[S_WALL_CELL]%16)*16),sy=screen_y((s[S_WALL_CELL]/16)*16),arena=screen_y(0)-8;
                ExtraTintBox(sx-40,sy-40<arena?arena:sy-40,sx+39,sy+39,(uint16_t)(blue<<10));
            }
            break;
        }
        case AB_SHOT:
            /* Honey's bullet: object set $0D anims $27-$2A (right, down,
             * left, up). */
            if(s[S_FLAGS]&4) {
                int bx=world_x(i)+kDx[d]*(int)get16(s+S_DIST),by=world_y(i)+kDy[d]*(int)get16(s+S_DIST);
                ExtraEmitSkinFrame(0,0x27+d,0,screen_x(bx),screen_y(by)-6);
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
