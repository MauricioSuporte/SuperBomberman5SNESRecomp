/* The player's own Super Bomberman 4 cartridge image, read at runtime.
 *
 * Nothing from Super Bomberman 4 is stored in this repository: the Bomber
 * Kings mod draws its characters from this image. The launcher's Mods page
 * asks for it (the package's [[external_rom]], verified by the engine) and
 * the plugin passes that path to Sb4RomLoad(). Without a selection it is
 * searched for, in order, at $SB5_SB4_ROM, then among the .sfc/.smc files of the working
 * directory, its roms/ folder and the folder of the Super Bomberman 5 ROM
 * named in rom.cfg. Only an image whose SHA-256 matches the verified
 * Japanese release is accepted (a 512-byte copier header is skipped). */
#include "sb4_rom.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

enum { SB4_SIZE = 0x400000 };
static const uint8_t kSb4Sha256[32]={
    0x68,0xdf,0x56,0x1d,0xba,0x2c,0x47,0xa2,0xb7,0x43,0xeb,0x5f,0x96,0x05,0x30,0xc7,
    0xa5,0x87,0x64,0x6d,0x3e,0x44,0x8f,0x68,0xf9,0x93,0x73,0x05,0x81,0x5c,0x9f,0x44,
};
static uint8_t *image;
static bool searched,decoded;
static char found_path[1024];

bool Sb4RomAccept(const uint8_t *data,size_t size) {
    if(size==SB4_SIZE+512) data+=512,size-=512;
    if(size!=SB4_SIZE) return false;
    uint8_t digest[32];sha256_compute(data,size,digest);
    return !memcmp(digest,kSb4Sha256,32);
}
static bool try_file(const char *path) {
    FILE *f=fopen(path,"rb");
    if(!f) return false;
    if(fseek(f,0,SEEK_END)) { fclose(f);return false; }
    long size=ftell(f);
    if(size!=SB4_SIZE && size!=SB4_SIZE+512) { fclose(f);return false; }
    uint8_t *data=malloc((size_t)size);
    bool ok=data && !fseek(f,0,SEEK_SET) && fread(data,1,(size_t)size,f)==(size_t)size && Sb4RomAccept(data,(size_t)size);
    fclose(f);
    if(!ok) { free(data);return false; }
    if(size==SB4_SIZE+512) memmove(data,data+512,SB4_SIZE);
    image=data;
    snprintf(found_path,sizeof(found_path),"%s",path);
    return true;
}
static bool rom_name(const char *name) {
    size_t n=strlen(name);
    if(n<4) return false;
    const char *e=name+n-4;
    return !strcmp(e,".sfc") || !strcmp(e,".smc") || !strcmp(e,".SFC") || !strcmp(e,".SMC");
}
static bool scan_dir(const char *dir) {
    char path[1024];
#ifdef _WIN32
    char pattern[1024];snprintf(pattern,sizeof(pattern),"%s\\*",dir[0]?dir:".");
    WIN32_FIND_DATAA fd;HANDLE h=FindFirstFileA(pattern,&fd);
    if(h==INVALID_HANDLE_VALUE) return false;
    bool ok=false;
    do {
        if(!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && rom_name(fd.cFileName)) {
            snprintf(path,sizeof(path),"%s\\%s",dir[0]?dir:".",fd.cFileName);
            ok=try_file(path);
        }
    } while(!ok && FindNextFileA(h,&fd));
    FindClose(h);
    return ok;
#else
    DIR *d=opendir(dir[0]?dir:".");
    if(!d) return false;
    bool ok=false;struct dirent *e;
    while(!ok && (e=readdir(d))) if(rom_name(e->d_name)) {
        snprintf(path,sizeof(path),"%s/%s",dir[0]?dir:".",e->d_name);
        ok=try_file(path);
    }
    closedir(d);
    return ok;
#endif
}
static void search(void) {
    searched=true;
    const char *env=getenv("SB5_SB4_ROM");
    if(env && env[0] && try_file(env)) return;
    if(scan_dir("") || scan_dir("roms")) return;
    FILE *f=fopen("rom.cfg","rb");
    if(f) {
        char line[1024]={0};
        size_t n=fread(line,1,sizeof(line)-1,f);fclose(f);line[n]=0;
        char *end=line+strlen(line);
        while(end>line && (end[-1]=='\n' || end[-1]=='\r')) *--end=0;
        while(end>line && end[-1]!='/' && end[-1]!='\\') --end;
        if(end>line) { end[-1]=0;scan_dir(line); }
    }
}
bool Sb4RomLoad(const char *path) {
    if(image) return true;
    return path && path[0] && try_file(path);
}
const uint8_t *Sb4Rom(void) {
    if(!searched) search();
    return image;
}
const char *Sb4RomPath(void) { return Sb4Rom()?found_path:NULL; }
void Sb4RomForTests(const uint8_t *data) { image=(uint8_t *)data;searched=true;decoded=false; }

/* HiROM: file offset of CPU address bank:addr (banks $C0-$FF). */
static unsigned file(uint32_t pc) { return ((pc>>16)&0x3f)<<16|(pc&0xffff); }
/* Battle sheets: 16 x 16 tiles (128 x 128 pixels), 64 frames of 32 x 32
 * pixels in rows of four; frame f is at base + (f/4)*$800 + (f%4)*$80, each
 * tile row $200 bytes further. Palettes: five per character (player colors
 * 1-5), $20 bytes apart. Verified by DMA logs of the cartridge's own battle
 * uploads (VRAM $6000-$6300) and CGRAM dumps with five players. */
static const uint32_t kSheet[SB4_CHARACTERS]={0xd20000,0xcc8000,0xcc0000,0xcb8000,0xcb0000,0xcd1800};
static const uint32_t kPalette[SB4_CHARACTERS]={0x00e000,0x00e2a0,0x00e1e0,0x1602ba,0x1601fa,0x00e360};
static void decode_tile(const uint8_t *t,uint8_t out[8][8]) {
    for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) {
        unsigned v=0;
        for(unsigned p=0;p<4;++p) v|=((t[(p/2)*16+y*2+(p%2)]>>(7-x))&1u)<<p;
        out[y][x]=(uint8_t)v;
    }
}
uint32_t Sb4FramePc(unsigned character,unsigned frame) {
    if(character>=SB4_CHARACTERS || frame>=64) return 0;
    return kSheet[character]+(frame/4)*0x800+(frame%4)*0x80;
}
bool Sb4FrameAt(uint32_t pc,uint8_t out[32][32]) {
    const uint8_t *rom=Sb4Rom();
    if(!rom || (pc>>16)<0xc0 || file(pc)+3*0x200+0x80>SB4_SIZE) return false;
    unsigned base=file(pc);
    for(unsigned ty=0;ty<4;++ty) for(unsigned tx=0;tx<4;++tx) {
        uint8_t px[8][8];decode_tile(rom+base+ty*0x200+tx*32,px);
        for(unsigned y=0;y<8;++y) memcpy(&out[ty*8+y][tx*8],px[y],8);
    }
    return true;
}
bool Sb4Frame(unsigned character,unsigned frame,uint8_t out[32][32]) {
    uint32_t pc=Sb4FramePc(character,frame);
    return pc && Sb4FrameAt(pc,out);
}
uint16_t Sb4Color(unsigned character,unsigned palette,unsigned index) {
    const uint8_t *rom=Sb4Rom();
    if(!rom || character>=SB4_CHARACTERS || palette>=SB4_COLORS || index>=16) return 0;
    unsigned at=kPalette[character]+palette*0x20+index*2;
    return (uint16_t)((rom[at]|rom[at+1]<<8)&0x7fff);
}

/* Tile streams of the cartridge's own loaders: a header byte pair (zero
 * marker, end marker), then per 32-byte tile either the zero marker (an
 * empty tile), the end marker (stop) or the 32 bytes themselves. The fire
 * sheet (explosion BG tiles $00-$FF, loader $C4:09A5) is two streams with
 * headers at $D1:D019 and $D1:E01C; the battle effect OBJ tiles (from OBJ
 * tile $80; loader $C1:874D) have theirs at $C8:FD36. Verified against the
 * VRAM of the cartridge's battles in a headless emulator. */
static uint8_t fire_tiles[256*32],effect_tiles[64*32];
static unsigned unpack(uint32_t header,uint8_t *out,unsigned max_tiles) {
    const uint8_t *rom=image;
    unsigned p=file(header),zero=rom[p],end=rom[p+1],n=0;
    p+=2;
    while(n<max_tiles && p+32<=SB4_SIZE) {
        unsigned c=rom[p];
        if(c==end) break;
        if(c==zero) { memset(out+n*32,0,32);++p; }
        else { memcpy(out+n*32,rom+p,32);p+=32; }
        ++n;
    }
    return n;
}
static bool decode_effects(void) {
    if(!Sb4Rom()) return false;
    if(!decoded) {
        unsigned n=unpack(0xd1d019,fire_tiles,128);
        unpack(0xd1e01c,fire_tiles+n*32,256-n);
        unpack(0xc8fd36,effect_tiles,64);
        decoded=true;
    }
    return true;
}
/* 16x16 block of a 16-tile-wide sheet: tiles t, t+1, t+16, t+17. */
static void block(const uint8_t *tiles,unsigned count,unsigned t,uint8_t out[16][16]) {
    for(unsigned q=0;q<4;++q) {
        unsigned n=t+(q&1)+(q>>1)*16;
        uint8_t px[8][8];
        if(n<count) decode_tile(tiles+n*32,px);
        else memset(px,0,sizeof(px));
        for(unsigned y=0;y<8;++y) memcpy(&out[(q>>1)*8+y][(q&1)*8],px[y],8);
    }
}
bool Sb4FireBlock(unsigned code,uint8_t out[16][16]) {
    if(code>0xee || !decode_effects()) return false;
    block(fire_tiles,256,code,out);
    return true;
}
uint16_t Sb4FireColor(unsigned index) {
    const uint8_t *rom=Sb4Rom();
    if(!rom || index>=16) return 0;
    return (uint16_t)((rom[0x17e11c+index*2]|rom[0x17e11d+index*2]<<8)&0x7fff);
}
bool Sb4EffectBlock(unsigned tile,uint8_t out[16][16]) {
    if(tile<0x80 || tile>0xae || !decode_effects()) return false;
    block(effect_tiles,64,tile-0x80,out);
    return true;
}

/* Battle picker portraits (32x32, VRAM tiles $200-$24B): two raw tile
 * blocks, $CD:E587 (16 tiles wide, rows $200 apart: Bomberman, Bazooka,
 * Lady) and $CD:ED87 (12 tiles wide, rows $184 apart: Great, Jet, Hammer);
 * found by matching the cartridge's picker VRAM. Five palettes per
 * character $20 apart. */
static const uint32_t kPortrait[SB4_CHARACTERS]={0xde587,0xdee87,0xdee07,0xde687,0xde607,0xded87};
static const uint16_t kPortraitStride[SB4_CHARACTERS]={0x200,0x184,0x184,0x200,0x200,0x184};
static const uint32_t kPortraitPalette[SB4_CHARACTERS]={0x16783c,0x167b7c,0x167adc,0x167a3c,0x16799c,0x167c1c};
bool Sb4Portrait(unsigned character,uint8_t out[32][32]) {
    const uint8_t *rom=Sb4Rom();
    if(!rom || character>=SB4_CHARACTERS) return false;
    for(unsigned ty=0;ty<4;++ty) for(unsigned tx=0;tx<4;++tx) {
        uint8_t px[8][8];decode_tile(rom+kPortrait[character]+ty*kPortraitStride[character]+tx*0x20,px);
        for(unsigned y=0;y<8;++y) memcpy(&out[ty*8+y][tx*8],px[y],8);
    }
    return true;
}
uint16_t Sb4PortraitColor(unsigned character,unsigned palette,unsigned index) {
    const uint8_t *rom=Sb4Rom();
    if(!rom || character>=SB4_CHARACTERS || palette>=SB4_COLORS || index>=16) return 0;
    unsigned at=kPortraitPalette[character]+palette*0x20+index*2;
    return (uint16_t)((rom[at]|rom[at+1]<<8)&0x7fff);
}

/* Bad Bomber frames: each character's extra sheet (same 32x32 layout),
 * found from the cartridge's own uploads with Bad Bomber on. Facing right
 * (left edge) +$3000/+$3080 throwing, down (top edge) +$3100/+$3180, left
 * (right edge) +$3800/+$3880, up (bottom edge) +$2900/+$2980. */
static const uint32_t kExtraSheet[SB4_CHARACTERS]={0xc55aed,0xc68000,0xc64000,0xc60000,0xc59aed,0xc6c000};
uint32_t Sb4BadBomberPc(unsigned character,unsigned direction,bool throwing) {
    static const uint16_t kOffset[4]={0x3000,0x3100,0x3800,0x2900};
    if(character>=SB4_CHARACTERS || direction>=4) return 0;
    return kExtraSheet[character]+kOffset[direction]+(throwing?0x80:0);
}
