/* The player's own Super Bomberman 4 cartridge image, read at runtime.
 *
 * Nothing from Super Bomberman 4 is stored in this repository: the Bomber
 * Kings mod draws its characters from this image. The file is searched for,
 * in order, at $SB5_SB4_ROM, then among the .sfc/.smc files of the working
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
static bool searched;
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
const uint8_t *Sb4Rom(void) {
    if(!searched) search();
    return image;
}
const char *Sb4RomPath(void) { return Sb4Rom()?found_path:NULL; }
void Sb4RomForTests(const uint8_t *data) { image=(uint8_t *)data;searched=true; }

/* HiROM: file offset of CPU address bank:addr (banks $C0-$FF). */
static unsigned file(uint32_t pc) { return ((pc>>16)&0x3f)<<16|(pc&0xffff); }
/* Battle sheets: 16 x 16 tiles (128 x 128 pixels), 64 frames of 32 x 32
 * pixels in rows of four; frame f is at base + (f/4)*$800 + (f%4)*$80, each
 * tile row $200 bytes further. Palettes: five per character (player colors
 * 1-5), $20 bytes apart. Verified by DMA logs of the cartridge's own battle
 * uploads (VRAM $6000-$6300) and CGRAM dumps with five players. */
static const uint32_t kSheet[SB4_CHARACTERS]={0xd20000,0xcb0000,0xcc0000,0xcb8000,0xcc8000,0xcd1800};
static const uint32_t kPalette[SB4_CHARACTERS]={0x00e000,0x1601fa,0x00e1e0,0x1602ba,0x00e2a0,0x00e360};
bool Sb4Frame(unsigned character,unsigned frame,uint8_t out[32][32]) {
    const uint8_t *rom=Sb4Rom();
    if(!rom || character>=SB4_CHARACTERS || frame>=64) return false;
    unsigned base=file(kSheet[character])+(frame/4)*0x800+(frame%4)*0x80;
    for(unsigned ty=0;ty<4;++ty) for(unsigned tx=0;tx<4;++tx) {
        const uint8_t *t=rom+base+ty*0x200+tx*32;
        for(unsigned y=0;y<8;++y) for(unsigned x=0;x<8;++x) {
            unsigned v=0;
            for(unsigned p=0;p<4;++p) v|=((t[(p/2)*16+y*2+(p%2)]>>(7-x))&1u)<<p;
            out[ty*8+y][tx*8+x]=(uint8_t)v;
        }
    }
    return true;
}
uint16_t Sb4Color(unsigned character,unsigned palette,unsigned index) {
    const uint8_t *rom=Sb4Rom();
    if(!rom || character>=SB4_CHARACTERS || palette>=SB4_COLORS || index>=16) return 0;
    unsigned at=kPalette[character]+palette*0x20+index*2;
    return (uint16_t)((rom[at]|rom[at+1]<<8)&0x7fff);
}
