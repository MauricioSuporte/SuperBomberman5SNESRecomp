/* Special abilities (B + direction) of the extra characters, after Super
 * Bomberman 4's Bomber Kings.
 *
 * Input (maintainer's rule): hold B and press a direction, or press B while
 * a direction is held. B alone keeps its native meaning (remote-control
 * detonation): for a seat whose ability is ready, a fresh B is held back for
 * a few frames waiting for a direction and released unchanged when none
 * comes. A rider (Louie, $1782 bit 7) never triggers an ability; Louie's own
 * skill stays on Y. While the ability recovers (debuff/cooldown) B is native.
 *
 * Abilities (Super Bomberman 5 versions of the Super Bomberman 4 moves):
 *   Honey shot / Lady disc  - projectile in a straight line, stops at blocks;
 *   Kotetsu slash / Hammer  - strike on the tile in front;
 *   Bazooka                 - flame line to the next block, for 24 frames;
 *   Jet                     - jet dash (forced walk, triple speed, invulnerable);
 *   Great                   - stands invulnerable for three seconds.
 * Damage uses the cartridge's own rules (point damage $C0:373F): a player
 * actor gets $15A2 bit 6 (hit) unless invulnerable ($15A2 bit 2), an enemy
 * loses one point of $1662 with 180 frames of invulnerability ($085F).
 * Normal Game hits enemies and bosses only, never a player (no friendly
 * fire). Battle hits every other player, or only the other team in Team
 * Battle ($7E625D bit 0; teams in $15A2 bits 3-4, from $6269 bit 7).
 *
 * Debuff (Super Bomberman 4, measured: Hammer, Jet, Great; also Honey and
 * Kotetsu): 640 frames (about 10.7 s) at half walking speed with the ability
 * blocked. Speed: the step count returned by $C4:3231 is halved at its store
 * ($C4:3180, X = seat).
 *
 * All state lives in free WRAM ($7F:F830-$F86B, zero in every research
 * capture), so save states and rewind carry it without an extension. */
#include "special_abilities.h"
#include "honey_kotetsu_mod.h"
#include "common_rtl.h"
#include "snes/interp_bridge.h"
#include <string.h>

enum {
    STATE = 0x1f830, SEAT_BYTES = 10, PENDING = 0x1f862, SPEED_ACC = 0x1f867, STATE_END = 0x1f86c,
    PAD_HOOK = 0xc00af9, SPEED_HOOK = 0xc43180,
    DEBUFF_FRAMES = 640, B_WINDOW = 8,
};
enum { AB_NONE, AB_SHOT, AB_SLASH, AB_HAMMER, AB_DASH, AB_DISC, AB_FIRE, AB_GUARD };
typedef struct { uint8_t kind; uint16_t debuff; uint16_t cooldown; uint8_t duration; uint8_t pose; } Ability;
/* Index: choice - 11 (Honey, Kotetsu, Bomberman, Hammer, Jet, Lady, Bazooka, Great). */
static const Ability kAbility[8]={
    {AB_SHOT,  DEBUFF_FRAMES,  30, 40, 12},
    {AB_SLASH, DEBUFF_FRAMES,  30, 12, 12},
    {AB_NONE,  0,               0,  0,  0},
    {AB_HAMMER,DEBUFF_FRAMES,  30, 12, 12},
    {AB_DASH,  DEBUFF_FRAMES,  30, 40, 40},
    {AB_DISC,  0,             120, 40, 12},
    {AB_FIRE,  0,             300, 24, 24},
    {AB_GUARD, DEBUFF_FRAMES,  30,180, 20},
};
/* Per seat: +0/1 debuff, +2/3 cooldown, +4 kind, +5 timer, +6 direction,
 * +7 travelled (pixels/4), +8 hit flag. */
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
/* Damages hostile actors within `reach` pixels of world (x, y). */
static bool strike(unsigned seat,int x,int y,int reach) {
    bool hit=false;
    for(unsigned a=0;a<24;++a) {
        if(!alive(a) || !hostile(seat,a)) continue;
        int dx=world_x(a)-x,dy=world_y(a)-y;
        if(dx<0) dx=-dx;
        if(dy<0) dy=-dy;
        if(dx<reach && dy<reach) { damage(a);hit=true; }
    }
    return hit;
}
/* Damages hostile actors standing in map cell (cx, cy). Actor cells use
 * the same rounding as world positions of tile-aligned actors (cell * 16). */
static int cell_of(int world) { return (world+8)>>4; }
static bool strike_cell(unsigned seat,int cx,int cy) {
    bool hit=false;
    for(unsigned a=0;a<24;++a) {
        if(!alive(a) || !hostile(seat,a)) continue;
        if(cell_of(world_x(a))==cx && cell_of(world_y(a))==cy) { damage(a);hit=true; }
    }
    return hit;
}
static const int kDx[4]={1,0,-1,0},kDy[4]={0,1,0,-1};
/* Stage map: $1052, 16 columns; $80 hard block, $30 soft block. */
static bool blocked_cell(int cx,int cy) {
    if(cx<0 || cy<0 || cx>=16 || cy>=13) return true;
    return (g_ram[0x1052+cy*16+cx]&0xb0)!=0;
}
static bool blocked(int x,int y) { return blocked_cell(cell_of(x),cell_of(y)); }
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
    return !get16(s) && !get16(s+2) && !s[4];
}
static void start(unsigned seat,int direction) {
    const Ability *a=ability_of(seat);
    uint8_t *s=seat_state(seat);
    s[4]=a->kind;s[5]=0;s[6]=(uint8_t)direction;s[7]=0;s[8]=0;
    set16(s+2,a->cooldown);
    if(a->kind==AB_GUARD || a->kind==AB_DASH) {
        /* Native post-hit invulnerability: $15A2 bit 2 with timer $170A. */
        g_ram[0x15a2+seat]|=0x04;
        if(g_ram[0x170a+seat]<a->duration) g_ram[0x170a+seat]=a->duration;
    }
}
static void finish(unsigned seat) {
    const Ability *a=ability_of(seat);
    uint8_t *s=seat_state(seat);
    s[4]=0;
    if(a && a->debuff) set16(s,a->debuff);
}
static void tick(unsigned seat) {
    uint8_t *s=seat_state(seat);
    if(get16(s)) set16(s,get16(s)-1);
    if(get16(s+2)) set16(s+2,get16(s+2)-1);
    if(!s[4]) return;
    const Ability *a=ability_of(seat);
    if(!a || !alive(seat)) { s[4]=0;return; }
    unsigned t=s[5]++,d=s[6]&3;
    int x=world_x(seat),y=world_y(seat);
    switch(s[4]) {
    case AB_SHOT: case AB_DISC: {
        unsigned speed=s[4]==AB_SHOT?4:6;
        unsigned travelled=(unsigned)s[7]*4+speed;
        int px=x+kDx[d]*(int)travelled,py=y+kDy[d]*(int)travelled;
        unsigned range=s[4]==AB_SHOT?7*16:12*16;
        if(s[8] || travelled>range || blocked(px,py) || strike(seat,px,py,12)) { s[8]=1;finish(seat);return; }
        s[7]=(uint8_t)(travelled/4);
        break;
    }
    case AB_SLASH: case AB_HAMMER:
        if(t>=2 && t<6) strike_cell(seat,cell_of(x)+kDx[d],cell_of(y)+kDy[d]);
        break;
    case AB_FIRE:
        s[7]=0;
        for(unsigned k=1;k<14;++k) {
            int cx=cell_of(x)+kDx[d]*(int)k,cy=cell_of(y)+kDy[d]*(int)k;
            if(blocked_cell(cx,cy)) break;
            strike_cell(seat,cx,cy);
            s[7]=(uint8_t)k;
        }
        break;
    case AB_DASH: {
        /* Until a wall stops it (two frames without moving) or 40 frames. */
        strike(seat,x+kDx[d]*8,y+kDy[d]*8,14);
        unsigned here=(unsigned)(x+y)&0xff;
        if(t>=2 && s[7]==here) { finish(seat);return; }
        s[7]=(uint8_t)here;
        break;
    }
    default: break;
    }
    if(s[5]>=a->duration) finish(seat);
}

/* $C0:0AF9: the pad reader has filled $2C/$31/$36/$3B+seat and the frame's
 * game logic has not consumed them yet. */
static void pad_hook(CpuState *cpu,uint32_t pc) {
    (void)cpu;(void)pc;
    if(!HoneyKotetsuInstalled() || !in_stage()) {
        /* Outside a stage every seat starts over (no debuff carried over). */
        for(unsigned b=STATE;b<STATE_END;++b) if(g_ram[b]) { memset(g_ram+STATE,0,STATE_END-STATE);break; }
        return;
    }
    if(!playing()) return;
    for(unsigned i=0;i<5;++i) {
        const Ability *a=ability_of(i);
        uint8_t *s=seat_state(i);
        if(!a) { g_ram[PENDING+i]=0;continue; }
        tick(i);
        /* Jet dash: keep walking that way. */
        if(s[4]==AB_DASH) { g_ram[0x31+i]=kDirBits[s[6]&3];g_ram[0x3b+i]=0;g_ram[0x36+i]&=(uint8_t)~0x08; }
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
    }
}
/* $C4:3180 STA $1CC2: A = pixels the seat (X) walks this frame; $C4:3284
 * walks that many single-pixel steps. */
static void speed_hook(CpuState *cpu,uint32_t pc) {
    (void)pc;
    unsigned seat=cpu->X&0xff;
    if(seat>=5 || !HoneyKotetsuInstalled() || !ability_of(seat)) return;
    uint8_t *s=seat_state(seat);
    unsigned steps=cpu->A&0xff;
    if(s[4]==AB_DASH) steps=steps?steps*3:3;
    else if(s[4]==AB_GUARD) steps=0;
    else if(get16(s)) {
        unsigned total=g_ram[SPEED_ACC+seat]+steps;
        steps=total/2;g_ram[SPEED_ACC+seat]=(uint8_t)(total&1);
    } else return;
    if(steps>12) steps=12;
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
int SpecialAbilityPose(unsigned seat) {
    if(seat>=5) return -1;
    const Ability *a=ability_of(seat);
    uint8_t *s=seat_state(seat);
    if(!a || !s[4] || s[5]>=a->pose) return -1;
    return s[6]&3;
}
bool SpecialAbilityDebuffed(unsigned seat) { return seat<5 && get16(seat_state(seat))!=0; }

/* ---- Effects: authored shapes, drawn by the extra-character overlay. ---- */
static uint16_t rgb(unsigned r,unsigned g,unsigned b) { return (uint16_t)(r|g<<5|b<<10); }
static void disc(uint8_t *px,unsigned w,unsigned h,float cx,float cy,float r,uint8_t c) {
    for(unsigned y=0;y<h;++y) for(unsigned x=0;x<w;++x) {
        float dx=(float)x+0.5f-cx,dy=(float)y+0.5f-cy;
        if(dx*dx+dy*dy<=r*r) px[y*w+x]=c;
    }
}
static int screen_x(int wx) { return wx-(int16_t)(g_ram[0x1d3b]|g_ram[0x1d3c]<<8); }
static int screen_y(int wy) { return wy-(int16_t)(g_ram[0x1d3d]|g_ram[0x1d3e]<<8); }
static void draw_at(int wx,int wy,const uint8_t *px,unsigned w,unsigned h,const uint16_t *colors) {
    ExtraEmitPixels(screen_x(wx)-(int)w/2,screen_y(wy)-(int)h/2,px,w,h,colors,3);
}
void SpecialAbilityDraw(void) {
    if(!HoneyKotetsuInstalled() || !in_stage()) return;
    uint16_t colors[16]={0,rgb(31,31,31),rgb(31,28,8),rgb(31,18,2),rgb(26,6,2),rgb(10,20,31),rgb(4,8,24),rgb(20,20,22),rgb(8,8,10)};
    for(unsigned i=0;i<5;++i) {
        uint8_t *s=seat_state(i);
        if(!s[4] || !alive(i)) continue;
        unsigned d=s[6]&3,t=s[5];
        int x=world_x(i),y=world_y(i);
        uint8_t px[32*32];memset(px,0,sizeof(px));
        switch(s[4]) {
        case AB_SHOT: {
            int travelled=s[7]*4;
            disc(px,8,8,4,4,3.5f,3);disc(px,8,8,4,4,2.2f,2);disc(px,8,8,4,4,1.0f,1);
            draw_at(x+kDx[d]*travelled,y+kDy[d]*travelled,px,8,8,colors);
            break;
        }
        case AB_DISC: {
            int travelled=s[7]*4;
            disc(px,12,12,6,6,6,5);disc(px,12,12,6,6,4,1);disc(px,12,12,6,6,2.5f,0);
            draw_at(x+kDx[d]*travelled,y+kDy[d]*travelled,px,12,12,colors);
            break;
        }
        case AB_SLASH: case AB_HAMMER: {
            /* An arc (slash) or a burst (hammer) on the tile in front. */
            if(t>=8) break;
            if(s[4]==AB_SLASH) {
                disc(px,16,16,8,8,8,1);disc(px,16,16,8,8,5.5f,5);disc(px,16,16,8+kDx[d]*-4.0f,8+kDy[d]*-4.0f,6.5f,0);
            } else {
                for(unsigned k=0;k<8;++k) {
                    static const int sx[8]={8,14,8,2,13,13,3,3},sy[8]={2,8,14,8,3,13,13,3};
                    disc(px,16,16,(float)sx[k],(float)sy[k],1.6f,2);
                }
                disc(px,16,16,8,8,4.5f,2);disc(px,16,16,8,8,2.5f,1);
            }
            draw_at((cell_of(x)+kDx[d])*16,(cell_of(y)+kDy[d])*16,px,16,16,colors);
            break;
        }
        case AB_FIRE: {
            unsigned flicker=(t/2)&1;
            for(unsigned k=1;k<=s[7] && k<14;++k) {
                memset(px,0,16*16);
                disc(px,16,16,8,8,7.5f-(float)flicker,4);disc(px,16,16,8,8,5.5f,3);disc(px,16,16,8,8,3,2);
                draw_at((cell_of(x)+kDx[d]*(int)k)*16,(cell_of(y)+kDy[d]*(int)k)*16,px,16,16,colors);
            }
            break;
        }
        case AB_DASH: {
            unsigned flicker=(t/2)&1;
            disc(px,12,12,6,6,5.5f-(float)flicker,3);disc(px,12,12,6,6,3.5f,2);disc(px,12,12,6,6,1.5f,1);
            draw_at(x-kDx[d]*14,y-kDy[d]*14,px,12,12,colors);
            break;
        }
        case AB_GUARD: {
            if((t/4)&1) break;
            disc(px,24,24,12,12,12,2);disc(px,24,24,12,12,10.5f,0);
            draw_at(x,y-6,px,24,24,colors);
            break;
        }
        default: break;
        }
    }
}
