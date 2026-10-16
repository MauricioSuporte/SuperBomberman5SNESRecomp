/* Super Bomberman 4 sound effects, rendered from the player's own SB4 image.
 *
 * The cartridge's sound driver runs in a private APU (the runner's SPC700 +
 * DSP, separate from the game's): it is uploaded the way the cartridge's
 * own loader does at boot ($C0:0376 -> $C0:0416 with streams $31, $2E and
 * $2F: driver, common samples, battle effects; stream table at $C0:0190,
 * blocks of size/destination/bytes ended by the entry address, IPL-style
 * handshake), then the effect number is sent the way $C0:071D does (port 0,
 * bit 7 toggling). The DSP output is kept as a PCM clip and played through
 * the host's mod mixer, so nothing from the game's own APU is touched and no
 * sound data is stored. Verified against the cartridge's battles in a
 * headless emulator (effect numbers from its $2140 writes). */
#include "sb4_sound.h"
#include "sb4_rom.h"
#include "snes/apu.h"
#include "mod_audio.h"
#include <stdlib.h>
#include <string.h>

enum { SB4_SIZE = 0x400000, RATE = 32000, MAX_SECONDS = 3, CYCLES_PER_SAMPLE = 32 };
typedef struct { Apu *apu; const uint8_t *rom; uint32_t ptr; uint8_t d3, e0; long budget; } Driver;

static unsigned file(uint32_t pc) { return ((pc>>16)&0x3f)<<16|(pc&0xffff); }
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
static uint8_t next(Driver *d) { return d->rom[file(d->ptr++)]; }
/* $C0:029B: request a transfer until the driver (or the IPL) answers AA/BB. */
static bool begin(Driver *d) {
    for(;;) {
        put(d,1,0x10);
        for(int k=0;k<4096;++k) {
            if(get(d,0)==0xaa && get(d,1)==0xbb) { d->d3=0xcc;return true; }
            if(!step(d)) return false;
        }
    }
}
static bool kick(Driver *d,uint8_t v) { put(d,0,v);return wait_for(d,0,v); }
/* $C0:0416: stream `index` (24-bit pointers at $C0:0190), $02F0 and $02BD. */
static bool upload(Driver *d,unsigned index) {
    unsigned at=0x190+index*3;
    d->ptr=d->rom[at]|d->rom[at+1]<<8|(uint32_t)d->rom[at+2]<<16;
    if(!begin(d)) return false;
    for(;;) {
        unsigned size=next(d);size|=next(d)<<8;
        if(!size) break;
        unsigned dest=next(d);dest|=next(d)<<8;
        if(size>0x10000) return false;
        put(d,1,0xff);put(d,2,(uint8_t)dest);put(d,3,(uint8_t)(dest>>8));
        if(!kick(d,d->d3)) return false;
        d->d3=0;
        for(unsigned k=0;k<size;++k) { put(d,1,next(d));if(!kick(d,d->d3)) return false;++d->d3; }
        if(!++d->d3) ++d->d3;
    }
    put(d,1,0);put(d,2,next(d));put(d,3,next(d));
    if(d->d3==0xaa) ++d->d3;
    if(!kick(d,d->d3)) return false;
    d->e0=(uint8_t)((d->d3&0x80)^0x80);
    return true;
}
/* $C0:071D: effect number with the toggling bit 7, acknowledged on port 0. */
static bool send(Driver *d,uint8_t id) {
    uint8_t v=(uint8_t)((id|d->e0)^0x80);
    put(d,0,v);
    if(!wait_for(d,0,(uint8_t)(v^0x80))) return false;
    d->e0^=0x80;
    return true;
}

static SNESModAudioClip clips[128];
static SNESModAudioVoice voices[128];
static bool tried[128];
static int16_t *render(uint8_t id,uint32_t *frames) {
    const uint8_t *rom=Sb4Rom();
    if(!rom) return NULL;
    Driver d={apu_init(),rom,0,0,0,200000000};
    if(!d.apu) return NULL;
    apu_reset(d.apu);
    for(int i=0;i<20000;++i) apu_cycle(d.apu);
    int16_t *pcm=NULL;
    if(upload(&d,0x31) && upload(&d,0x2e) && upload(&d,0x2f)) {
        for(int i=0;i<RATE*CYCLES_PER_SAMPLE/10;++i) apu_cycle(d.apu);   /* settle 0.1 s */
        int16_t scratch[2*512];
        while(dsp_available(d.apu->dsp)) dsp_getSamples(d.apu->dsp,scratch,512);
        if(send(&d,id)) {
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
void Sb4SoundPlay(uint8_t id) {
    id&=0x7f;
    if(!tried[id]) {
        tried[id]=true;
        uint32_t frames=0;
        int16_t *pcm=render(id,&frames);
        if(pcm) {
            clips[id]=snes_mod_audio_register_pcm_s16(pcm,frames,RATE,SNES_MOD_AUDIO_CHANNELS_STEREO);
            free(pcm);
        }
    }
    /* The driver restarts an effect on its own channel: a repeat cuts the
     * previous one instead of stacking (Jet's trail plays $39 every 8 frames). */
    if(!clips[id]) return;
    if(voices[id]) snes_mod_audio_stop_voice(voices[id]);
    voices[id]=snes_mod_audio_play_voice(clips[id],100);
}
void Sb4SoundReset(void) {
    for(unsigned i=0;i<128;++i) {
        if(clips[i]) snes_mod_audio_unregister(clips[i]);
        clips[i]=SNES_MOD_AUDIO_CLIP_INVALID;voices[i]=SNES_MOD_AUDIO_VOICE_INVALID;tried[i]=false;
    }
}
