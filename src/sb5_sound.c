/* Stage sound effects of Super Bomberman 5 rendered from the player's own
 * cartridge, for effects whose samples the current stage has not loaded
 * (Terrorin's barrier, effect $0288 of ZONE5 - J, played in Battle).
 *
 * Sending a stage effect the loaded sound set lacks hangs the game's own
 * sound driver (Battle froze after $0288), so the effect is rendered in a
 * private APU instead (the runner's SPC700 + DSP, separate from the game's)
 * the way sb4_sound.c does for Super Bomberman 4: the sound sets are
 * uploaded like the cartridge's loader $C0:0E9C (set list at $DA:C2E0: a
 * block stream, then the sample destination and sample ids, each sample
 * from $DA:D042 + id*3 with its size at $DA:D1BF + id*2, uploaded with the
 * IPL-style handshake of $C0:0F13/$0F62/$0F35), then the effect command
 * from $C0:86D0 is sent like $C0:0E7F (port 0, bit 7 toggling). The DSP
 * output becomes a PCM clip for the host's mod mixer; no sound data is
 * stored and the game's APU is never touched. Barrier sound ($74): about
 * 2.6 s, rendered and checked in a headless run. */
#include "sb5_sound.h"
#include "common_rtl.h"
#include "snes/apu.h"
#include "mod_audio.h"
#include <stdlib.h>
#include <string.h>

enum { RATE = 32000, MAX_SECONDS = 3, CYCLES_PER_SAMPLE = 32 };
typedef struct { Apu *apu; uint32_t ptr; uint8_t counter, toggle; long budget; } Driver;

static unsigned file(uint32_t pc) { return ((pc>>16)&0x3f)<<16|(pc&0xffff); }
static uint8_t rb(uint32_t pc) { return g_rom[file(pc)]; }
static unsigned rw(uint32_t pc) { return rb(pc)|rb(pc+1)<<8; }
static bool step(Driver *d) {
    for(int i=0;i<64;++i) apu_cycle(d->apu);
    d->budget-=64;
    return d->budget>0;
}
static void put(Driver *d,int port,uint8_t v) { apu_writePortNow(d->apu,(uint8_t)port,v); }
static uint8_t get(Driver *d,int port) { return d->apu->outPorts[port]; }
static bool wait_for(Driver *d,int port,uint8_t v) {
    while(get(d,port)!=v) if(!step(d)) return false;
    return true;
}
static uint8_t next(Driver *d) { uint8_t v=rb(d->ptr);d->ptr=(d->ptr+1)&0xffffff;return v; }  /* $C0:0FDF carries into the bank */
/* $C0:0F13: ask for a transfer until the driver (or the IPL) answers AA/BB. */
static bool begin(Driver *d) {
    for(;;) {
        put(d,1,0x10);
        for(int k=0;k<4096;++k) {
            if(get(d,0)==0xaa && get(d,1)==0xbb) { d->counter=0xcc;return true; }
            if(!step(d)) return false;
        }
    }
}
static bool kick(Driver *d,uint8_t v) { put(d,0,v);return wait_for(d,0,v); }
/* $C0:0F62 body: one block of `size` bytes at `dest`. */
static bool block(Driver *d,unsigned size,unsigned dest) {
    put(d,1,0xff);put(d,2,(uint8_t)dest);put(d,3,(uint8_t)(dest>>8));
    if(!kick(d,d->counter)) return false;
    d->counter=0;
    for(unsigned k=0;k<size;++k) { put(d,1,next(d));if(!kick(d,d->counter)) return false;++d->counter; }
    if(!++d->counter) ++d->counter;
    return true;
}
/* $C0:0F62 with $03D1 = 0: blocks (size, destination, bytes) until size 0. */
static bool blocks(Driver *d) {
    for(unsigned guard=0;guard<64;++guard) {
        unsigned size=next(d);size|=next(d)<<8;
        if(!size) return true;
        unsigned dest=next(d);dest|=next(d)<<8;
        if(!block(d,size,dest)) return false;
    }
    return false;
}
/* $C0:0F35: start address (the next two stream bytes). */
static bool start(Driver *d) {
    put(d,1,0);put(d,2,next(d));put(d,3,next(d));
    if(d->counter==0xaa) ++d->counter;
    return kick(d,d->counter);
}
/* $C0:0E31 with A = `index`: a whole stream from the table at $C5:F69B
 * (the boot loads $58, the driver, then $56). */
static bool upload_stream(Driver *d,unsigned index) {
    uint32_t at=0xc5f69b+index*3;
    d->ptr=rw(at)|(uint32_t)rb(at+2)<<16;
    return begin(d) && blocks(d) && start(d);
}
/* $C0:0E9C with A = `set`. */
static bool upload(Driver *d,unsigned set) {
    uint32_t list=0xda0000|rw(0xdac2e0+set*2);
    d->ptr=0xda0000|rw(list);
    if(!begin(d) || !blocks(d)) return false;
    unsigned dest=rw(list+2);
    for(unsigned k=4;k<64;++k) {
        unsigned id=rb(list+k);
        if(id==0xff) break;
        unsigned size=rw(0xdad1bf+id*2);
        uint32_t at=0xdad042+id*3;
        d->ptr=rw(at)|(uint32_t)rb(at+2)<<16;
        if(!size || size>0x10000 || !block(d,size,dest)) return false;
        dest=(dest+size)&0xffff;
    }
    /* $C0:0F35 reads the start address after the last data read. */
    return start(d);
}
/* $C0:0E7F: command byte with bit 7 toggling ($03C0), echoed on port 0. */
static bool send(Driver *d,uint8_t command) {
    uint8_t v=(uint8_t)((command|d->toggle)^0x80);
    put(d,0,v);
    if(!wait_for(d,0,(uint8_t)(v^0x80))) return false;
    d->toggle^=0x80;
    return true;
}
static int16_t *render(unsigned set,uint8_t command,uint32_t *frames) {
    if(!g_rom) return NULL;
    Driver d={apu_init(),0,0,0,400000000};
    if(!d.apu) return NULL;
    apu_reset(d.apu);
    for(int i=0;i<20000;++i) apu_cycle(d.apu);
    int16_t *pcm=NULL;
    /* Boot: driver, common data, set 0 and command $01; the stage
     * (ZONE5 - J): streams $03/$20/$36, its set and command $02 (the
     * cartridge's own order, from a $C0:0E31/$0E9C/$0E7F trace). */
    bool ready=upload_stream(&d,0x58) && upload_stream(&d,0x56) && upload(&d,0) && send(&d,0x01) &&
               upload_stream(&d,0x03) && upload_stream(&d,0x20) && upload_stream(&d,0x36) && upload(&d,set) &&
               send(&d,0x02);
    if(ready) {
        for(int i=0;i<RATE*CYCLES_PER_SAMPLE/10;++i) apu_cycle(d.apu);   /* settle 0.1 s */
        int16_t scratch[2*512];
        while(dsp_available(d.apu->dsp)) dsp_getSamples(d.apu->dsp,scratch,512);
        if(send(&d,command)) {
            uint32_t cap=RATE*MAX_SECONDS,n=0,last=0;
            pcm=malloc(cap*2*sizeof(int16_t));
            while(pcm && n<cap) {
                for(int i=0;i<CYCLES_PER_SAMPLE*256;++i) apu_cycle(d.apu);
                uint32_t got=dsp_getSamples(d.apu->dsp,pcm+n*2,(int)(cap-n<256?cap-n:256));
                for(uint32_t k=0;k<got*2;++k) if(pcm[n*2+k]>16 || pcm[n*2+k]<-16) last=n+k/2+1;
                n+=got;
                if(!got || (last && n>last+RATE/5) || (!last && n>RATE/2)) break;
            }
            *frames=last;
            if(!last) { free(pcm);pcm=NULL; }
        }
    }
    apu_free(d.apu);
    return pcm;
}

enum { SLOTS = 4 };
static struct { unsigned set; uint16_t effect; bool tried; SNESModAudioClip clip; SNESModAudioVoice voice; } slots[SLOTS];
void Sb5StageSoundPlay(unsigned set,uint16_t effect) {
    unsigned k;
    for(k=0;k<SLOTS;++k) if(slots[k].tried && slots[k].set==set && slots[k].effect==effect) break;
    if(k==SLOTS) {
        for(k=0;k<SLOTS && slots[k].tried;++k) {}
        if(k==SLOTS) return;
        slots[k].tried=true;slots[k].set=set;slots[k].effect=effect;
        /* Group 2 effects: command byte from the table at $C0:86D0. */
        uint8_t command=(effect>>8)==2?rb(0xc086d0+(effect&0xff)):0;
        uint32_t frames=0;
        int16_t *pcm=command?render(set,command,&frames):NULL;
        if(pcm) {
            slots[k].clip=snes_mod_audio_register_pcm_s16(pcm,frames,RATE,SNES_MOD_AUDIO_CHANNELS_STEREO);
            free(pcm);
        }
    }
    if(!slots[k].clip) return;
    if(slots[k].voice) snes_mod_audio_stop_voice(slots[k].voice);
    slots[k].voice=snes_mod_audio_play_voice(slots[k].clip,100);
}
