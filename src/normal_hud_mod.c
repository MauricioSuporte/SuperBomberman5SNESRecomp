/* Native Battle face objects in the Normal HUD; cartridge art only. */
#include "normal_hud_mod.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "mod_runtime.h"
#include "snes/ppu.h"
#include "snes/interp_bridge.h"
#include <string.h>
static bool enabled, installed;
static uint8_t original[4], guard[2], death[5], respawn[4], tick[3], scratch[256], release[96];
/* Static OBJ allocation failures seen this frame: see vram_full(). */
static bool alloc_failed, hooks_added;
static bool releasing;
static uint8_t suspended_stage;
static uint8_t *rom(unsigned pc) { return SnesRomPtr(pc); }
static bool campaign(void) {
    unsigned m=g_ram[0x1031];
    return (m==8 || (m>=13 && m<116)) && !(g_ram[0x1d1a]&1);
}
/* Boss stages ($67-$70) and the secret boss stages ($71-$73) keep the stock
 * HUD heads. A face object loads its archive ($64) into static OBJ VRAM
 * ($7E30AA/$30EA/$314A); after ZONE3 - D's rivals died the exit gates' own
 * archive no longer fitted ($C0:18FC -> $C0:1A0A failed), $C5:3B60 retried
 * the gate objects every frame and the portals never appeared. */
static bool boss_stage(void) {
    unsigned m=g_ram[0x1031];
    return m>=0x67 && m<=0x73;
}
/* Regular stages hit the same limit: in ZONE3 - 1 (`$2B`) the enemies'
 * archive `$13` is released when the last enemy dies and the exit gate's
 * archive `$63` must take its VRAM, but with face archive `$64` loaded it
 * does not fit; the gate stays queued in `$7E4DA3` and never appears. The
 * static allocator `$C0:18FC` reaches `$C0:193C` when no VRAM block fits
 * (`$C0:1A0A` carry) and `$C0:193A` when all 32 entries are taken. On such
 * a failure for another archive while a face is live, the face objects are
 * deleted through the native `$C0:1342` (which also releases their archive
 * and palette) and the stock heads return until the next stage; the native
 * retry then succeeds. Stages whose gates fit keep their faces (the queue
 * alone is no signal: covered gates stay queued until revealed). */
static void vram_full(CpuState *cpu,uint32_t pc) {
    (void)cpu;(void)pc;
    if(enabled && installed && g_ram[0xadf]!=0x64) alloc_failed=true;
}
static bool suspended(void) {
    if(suspended_stage && g_ram[0x1031]!=suspended_stage) suspended_stage=0;
    return suspended_stage!=0;
}
static bool face_live(unsigned seat) {
    unsigned o=g_ram[0x35c5+seat];
    return o<0x60 && g_ram[0x4ff+o]==6 && g_ram[0x4a0+o]==0xf0;
}

bool NormalHudEnabled(void) { return enabled; }
void NormalHudReset(void) {
    if(installed) {
        memcpy(rom(0xc505f1),original,4);
        memcpy(rom(0xc5126b),guard,2);
        memcpy(rom(0xc436ff),death,5);
        memcpy(rom(0xc42b33),respawn,4);
        memcpy(rom(0xc50608),tick,3);
        memcpy(rom(0xc57200),scratch,sizeof(scratch));
        memcpy(rom(0xc57300),release,sizeof(release));
    }
    installed=false;alloc_failed=false;releasing=false;
}
void NormalHudBeforeFrame(void) {
    if(!enabled || !g_rom) return;
    if(!campaign() || boss_stage() || suspended()) { NormalHudReset();return; }
    /* Resuming an older scene or an extra-seat reentry can leave a face on
     * another character's animation. Let the native updater repair it. */
    if(installed) {
        if(releasing) {
            /* The HUD tick ran the release routine: back to stock heads. */
            if(!face_live(0) && !face_live(1)) {
                suspended_stage=(uint8_t)g_ram[0x1031];NormalHudReset();
            }
            return;
        }
        if(alloc_failed) {
            alloc_failed=false;
            if(!face_live(0) && !face_live(1)) { suspended_stage=(uint8_t)g_ram[0x1031];NormalHudReset();return; }
            static const uint8_t release_call[]={0x20,0x00,0x73};
            memcpy(rom(0xc50608),release_call,3);releasing=true;
            return;
        }
        uint8_t *lookup=rom(0xc51319);
        unsigned table=lookup[1]|lookup[2]<<8|lookup[3]<<16;
        if(lookup[0]==0xbf)for(unsigned i=0;i<2;++i) {
            unsigned o=g_ram[0x35c5+i],c=g_ram[0x16c2+i];
            if(c<10 && !(g_ram[0x17fa+i]&128) && !(g_ram[0x15a2+i]&128) &&
               o<0x60 && g_ram[0x4ff+o]==6 && g_ram[0x4a0+o]==0xf0 &&
               g_ram[0x79f+o]!=rom(table)[c])g_ram[0x35ca+i]=1;
        }
        return;
    }
    static const uint8_t call[]={0x22,0x86,0x0c,0xc5};
    static const uint8_t death_stock[]={0xad,0x1a,0x1d,0x89,0x01};
    if(memcmp(rom(0xc50608),"\x20\xcd\x06",3) || memcmp(rom(0xc42b33),"\x22\x07\x13\xc5",4) || memcmp(rom(0xc436ff),death_stock,5) || memcmp(rom(0xc505f1),call,4) || memcmp(rom(0xc5126b),"\xf0\x52",2)) return;
    for(unsigned i=0;i<sizeof(scratch);++i) if(rom(0xc57200)[i]!=0xff)return;
    for(unsigned i=0;i<sizeof(release);++i) if(rom(0xc57300)[i]!=0xff)return;
    /* Allocator failure exits (see vram_full); both are BRA $193E. */
    if(memcmp(rom(0xc0193a),"\x80\x02\x80\x00",4)) return;
    if(!hooks_added) {
        if(!interp_bridge_add_pre_opcode_hook(0xc0193a,vram_full) ||
           !interp_bridge_add_pre_opcode_hook(0xc0193c,vram_full)) return;
        hooks_added=true;
    }
    memcpy(release,rom(0xc57300),sizeof(release));
    memcpy(original,rom(0xc505f1),4);memcpy(guard,rom(0xc5126b),2);
    memcpy(scratch,rom(0xc57200),sizeof(scratch));
    memcpy(death,rom(0xc436ff),5);memcpy(respawn,rom(0xc42b33),4);memcpy(tick,rom(0xc50608),3);
    /* Lives refresh may execute repeatedly. Only allocate if the previous
     * face object is not live. The native scene clears its object pool. */
    uint8_t *p=rom(0xc57200);
    const uint8_t head[]={0x22,0x86,0x0c,0xc5,0xda,0x5a};
    memcpy(p,head,sizeof(head));p+=sizeof(head);
    for(unsigned seat=0;seat<2;++seat) {
        uint8_t code[]={
            0xa2,seat,0xbc,0xfa,0x17,0x30,0x20,
            0x86,0x8d,0xa9,seat?0xa0:0x18,0x8d,0x36,0x1d,
            0xbf,0xc5,0x35,0x7e,0xa8,0xb9,0xff,0x04,0xc9,0x06,0xd0,0x07,
            0xb9,0xa0,0x04,0xc9,0xf0,0xf0,0x06,
            0xbc,0xfa,0x17,0x20,0x64,0x12};
        memcpy(p,code,sizeof(code));p+=sizeof(code);
    }
    /* Normal rejoins through $C4:28CF, bypassing Battle's $C5:1307.
     * The HUD task restores a defeated or stale living face after reentry. */
    for(unsigned seat=0;seat<2;++seat) {
        const uint8_t revive[]={0xa2,seat,0xbd,0xa2,0x15,0x30,0x0d,
            0xbd,0xca,0x35,0xf0,0x08,0x8a,0x22,0x07,0x13,0xc5,0x9e,0xca,0x35};
        memcpy(p,revive,sizeof(revive));p+=sizeof(revive);
    }
    *p++=0x7a;*p++=0xfa;*p++=0x6b;
    const uint8_t redirect[]={0x22,0x00,0x72,0xc5};
    memcpy(rom(0xc505f1),redirect,4);memset(rom(0xc5126b),0xea,2);
    /* Normal death takes the branch around Battle's HUD notification.
     * Notify just P1/P2 without executing Battle's palette/equipment code. */
    const uint8_t notify[]={
        0x48,0xda,0x5a,0xad,0x1a,0x1d,0x89,0x01,0xd0,0x0b,
        0xa6,0x8d,0xe0,0x02,0xb0,0x05,0xa9,0x02,0x20,0xdd,0x12,
        0x7a,0xfa,0x68,0xad,0x1a,0x1d,0x89,0x01,0x6b};
    memcpy(rom(0xc57280),notify,sizeof(notify));
    const uint8_t death_call[]={0x22,0x80,0x72,0xc5,0xea};
    memcpy(rom(0xc436ff),death_call,5);
    /* Co-op actors can take Battle's reentry branch. Its face lookup has
     * five entries, but Normal owns only P1/P2 faces; stale P3-P5 pointers
     * must never select an unrelated object or another seat's portrait. */
    const uint8_t reentry[]={
        0xc9,0x02,0xb0,0x04, /* CMP #2 / BCS return */
        0x22,0x07,0x13,0xc5,0x6b};
    memcpy(rom(0xc572a0),reentry,sizeof(reentry));
    const uint8_t reentry_call[]={0x22,0xa0,0x72,0xc5};
    memcpy(rom(0xc42b33),reentry_call,4);
    /* Ensure P1/P2 after their actors exist, rather than only on life-count
     * changes. Late co-op spawns and restored scenes need the same update.
     * This runs every HUD tick. The full entry ($C5:7200) starts with the
     * lives refresh $C5:0C86, which redraws the score boxes; running it
     * every frame dropped frames (24% with five players in Zone 1-1).
     * Run it on one tick in eight (main-loop counter $69, which advances
     * once per tick; the NMI counter $68 can keep one parity at this call)
     * and only the face checks ($C5:7204) on the others. */
    const uint8_t update[]={
        0x20,0xcd,0x06,             /* JSR $06CD (native)      */
        0x08,0xe2,0x20,0x48,        /* PHP / SEP #$20 / PHA    */
        0xaf,0x69,0x00,0x7e,        /* LDA $7E0069             */
        0x29,0x07,0xf0,0x07,        /* AND #7 / BEQ full       */
        0x68,0x28,                  /* PLA / PLP               */
        0x22,0x04,0x72,0xc5,0x60,   /* JSL $C5:7204 / RTS      */
        0x68,0x28,                  /* full: PLA / PLP         */
        0x22,0x00,0x72,0xc5,0x60};  /* JSL $C5:7200 / RTS      */
    memcpy(rom(0xc572b0),update,sizeof(update));
    /* Gate room ($C5:7300, called instead of $C5:72B0 once): run the native
     * tick, then delete each live P1/P2 face object ($04FF = 6, $04A0 = $F0)
     * with $C0:1342 and forget it in $35C5. */
    uint8_t *q=rom(0xc57300);
    const uint8_t enter[]={0x20,0xcd,0x06,0x08,0xc2,0x30,0x48,0xda,0x5a,0xe2,0x30};
    memcpy(q,enter,sizeof(enter));q+=sizeof(enter);
    for(unsigned seat=0;seat<2;++seat) {
        const uint8_t drop[]={
            0xaf,0xc5+seat,0x35,0x7e,  /* LDA $7E35C5+seat */
            0xc9,0x60,0xb0,26,         /* CMP #$60 / BCS next */
            0xaa,                      /* TAX */
            0xbd,0xff,0x04,0xc9,0x06,0xd0,18, /* LDA $04FF,X / CMP #6 / BNE */
            0xbd,0xa0,0x04,0xc9,0xf0,0xd0,11, /* LDA $04A0,X / CMP #$F0 / BNE */
            0x8a,0x22,0x42,0x13,0xc0,  /* TXA / JSL $C0:1342 */
            0xa9,0xff,0x8f,0xc5+seat,0x35,0x7e}; /* LDA #$FF / STA $7E35C5+seat */
        memcpy(q,drop,sizeof(drop));q+=sizeof(drop);
    }
    const uint8_t leave[]={0xc2,0x30,0x7a,0xfa,0x68,0x28,0x60};
    memcpy(q,leave,sizeof(leave));
    const uint8_t update_call[]={0x20,0xb0,0x72};
    memcpy(rom(0xc50608),update_call,3);
    installed=true;
}
static void activate(void) { enabled=true; }
static void reset_plugin(void) { NormalHudReset();enabled=false;suspended_stage=0; }
SNES_MOD_CONSTRUCTOR(register_normal_hud) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.normal.hud-faces",activate);
}
/* Hide fixed heads and fit the existing blinking rejoin text beside a dead
 * face. Read the already translated BG3 pixels, so no text/art is embedded. */
typedef struct { uint16_t address, value; } SavedWord;
static SavedWord saved[640];
static unsigned saved_count;
static bool drawn;
static void write_word(Ppu *p,unsigned a,uint16_t value) {
    saved[saved_count++]=(SavedWord){(uint16_t)a,p->vram[a]};p->vram[a]=value;
}
/* The stock heads belong to BG2's three-row HUD strip, not BG3 text.
 * Keep its top/bottom frame rows and replace only y=4..20 with palette
 * index 4 (the native blue background). Borrow tiles instead of repainting
 * shared native graphics. BG1 and BG2 share the same 4bpp tile bank. */
static void hide_fixed_head(Ppu *p,unsigned seat) {
    if(PPU_bgTileAdr(p,0) || PPU_bgTileAdr(p,1) ||
       PPU_bgTilemapAdr(p,0)!=0x3000 || PPU_bgTilemapAdr(p,1)!=0x3800 ||
       !PPU_bgTilemapHigher(p,1) || PPU_bgTilemapWider(p,1))return;
    unsigned col=seat?20:3;
    const unsigned top[2]={0xee,0xf4};
    for(unsigned y=0;y<3;++y)for(unsigned x=0;x<2;++x)
        if(p->vram[0x3c00+y*32+col+x]!=(0x2000+top[seat]+y*2+x))return;
    bool used[768]={0};
    for(unsigned layer=0;layer<2;++layer) {
        unsigned map=PPU_bgTilemapAdr(p,layer);
        unsigned size=1024u<<(!!PPU_bgTilemapHigher(p,layer)+!!PPU_bgTilemapWider(p,layer));
        if(map+size>0x4000)return;
        for(unsigned i=0;i<size;++i) {
            unsigned tile=p->vram[map+i]&1023;if(tile<768)used[tile]=true;
        }
    }
    unsigned first=0;
    for(unsigned t=0x280;t+6<=0x300;++t) {
        bool free=true;for(unsigned j=0;j<6;++j)if(used[t+j])free=false;
        if(free){first=t;break;}
    }
    if(!first)return;
    for(unsigned y=0;y<3;++y)for(unsigned x=0;x<2;++x) {
        unsigned map=0x3c00+y*32+col+x,source=(p->vram[map]&1023)*16;
        unsigned tile=first+y*2+x;
        for(unsigned row=0;row<8;++row) {
            bool interior=y*8+row>=4 && y*8+row<=20;
            write_word(p,tile*16+row,interior?0:p->vram[source+row]);
            write_word(p,tile*16+row+8,interior?0x00ff:p->vram[source+row+8]);
        }
        write_word(p,map,(uint16_t)(0x2000+tile));
    }
}
static unsigned pixel(const Ppu *p,unsigned map,unsigned x,unsigned y) {
    unsigned w=p->vram[map+(y/8)*32+x/8],px=x%8,py=y%8;
    if(w&0x4000)px=7-px;
    if(w&0x8000)py=7-py;
    unsigned row=p->vram[0x5000+(w&1023)*8+py],bit=7-px;
    return ((row>>bit)&1)|(((row>>(bit+8))&1)<<1);
}
void NormalHudObserve(const struct Ppu *view,unsigned line,void *context) {
    (void)context;Ppu *p=(Ppu *)view;
    if(line==224) {
        if(drawn) {
            while(saved_count) { SavedWord w=saved[--saved_count];p->vram[w.address]=w.value; }
            ++p->vramWriteCount;drawn=false;
        }
        return;
    }
    if(line!=1 || drawn || !enabled || !campaign() || boss_stage() || PPU_mode(p)!=1 ||
        PPU_bgTileAdr(p,2)!=0x5000 || PPU_objTileAdr1(p)!=0x6000 ||
        PPU_bgTilemapWider(p,2)) return;
    unsigned base=PPU_bgTilemapAdr(p,2);
    if(base+1024>0x5000)return;
    bool used[512]={0};
    unsigned words=1024u<<PPU_bgTilemapHigher(p,2);
    if(base+words>0x5000)return;
    for(unsigned i=0;i<words;++i) { unsigned t=p->vram[base+i]&1023;if(t<512)used[t]=true; }
    saved_count=0;
    for(unsigned seat=0;seat<2;++seat) {
        unsigned o=g_ram[0x35c5+seat],col=3+seat*17;
        if(o>=0x60 || g_ram[0x4ff+o]!=6 || g_ram[0x4a0+o]!=0xf0)continue;
        hide_fixed_head(p,seat);
        if(g_ram[0x15a2+seat]&0x80) {
            unsigned first=0;
            for(unsigned t=256;t+16<=512;++t) {
                bool free=true;for(unsigned i=0;i<16;++i)if(used[t+i])free=false;
                if(free){first=t;break;}
            }
            if(!first)continue;
            unsigned source=seat?21:3,dest=seat?22:5;
            uint8_t pixels[16][64];
            for(unsigned y=0;y<16;++y)for(unsigned x=0;x<64;++x)
                pixels[y][x]=(uint8_t)pixel(p,base,source*8+x*80/64,y);
            for(unsigned y=0;y<2;++y)for(unsigned x=0;x<10;++x)
                write_word(p,base+y*32+source+x,0x2000);
            for(unsigned ty=0;ty<2;++ty)for(unsigned tx=0;tx<8;++tx) {
                unsigned tile=first+ty*8+tx;used[tile]=true;
                for(unsigned py=0;py<8;++py) {
                    unsigned row=0;
                    for(unsigned px=0;px<8;++px) {
                        unsigned v=pixels[ty*8+py][tx*8+px],bit=7-px;
                        row|=(v&1)<<bit;row|=((v>>1)&1)<<(bit+8);
                    }
                    write_word(p,0x5000+tile*8+py,(uint16_t)row);
                }
                write_word(p,base+ty*32+dest+tx,(uint16_t)(0x2000+tile));
            }
        }
        for(unsigned y=0;y<2;++y)for(unsigned x=0;x<2;++x)
            write_word(p,base+y*32+col+x,0x2000);
    }
    if(saved_count){++p->vramWriteCount;drawn=true;}
}

