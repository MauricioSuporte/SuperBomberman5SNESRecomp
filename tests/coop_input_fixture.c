/* Opt-in integration fixture, never present in default/release builds.
 * Input scripts can poke these mailboxes to drive the three extra devices
 * through the real multitap and NMI input path. This is not gameplay state. */
#include "common_rtl.h"
void CoopInputFixture(void) {
    for(unsigned seat=2;seat<5;++seat) {
        unsigned at=0x1ffe0+(seat-2)*2;
        RtlSetPadState((int)seat,(uint16)(g_ram[at]|g_ram[at+1]<<8));
    }
    /* Synthetic death scenarios enter the native final cleanup atomically.
     * Separate script pokes of its three PC bytes would execute a torn PC. */
    uint8_t deaths=g_ram[0x1ffe7];g_ram[0x1ffe7]=0;
    for(unsigned seat=0;seat<5;++seat) if((deaths&(1u<<seat)) && !(g_ram[0x17fa+seat]&0x80)) {
        g_ram[0x164a+seat]=1;g_ram[0x15a2+seat]|=0x80;
        g_ram[0x155a+seat]=0xd2;g_ram[0x1572+seat]=0x36;g_ram[0x158a+seat]=0xc4;
    }
}
