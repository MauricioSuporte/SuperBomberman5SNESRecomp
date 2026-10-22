/* Test-only egg choice: mailbox $7F:FFF0 = native mount index + 1.
 * $7F:FFF1 counts native $0238 SFX commands; FFF2/FFF3 record the
 * FFF4/FFF5 count native Black acquisition / dash commands.
 * FFF2/FFF3 hold the native frame counter at Black acquisition cue / attachment.
 * The native egg pickup still allocates, animates and attaches the mount. */
#include "../src/actor_layout.h"
#include "common_rtl.h"
#include "snes/interp_bridge.h"
static void egg_choice(CpuState *cpu,uint32_t pc) {
    if(pc==0xc00ced) {
        if(cpu->A==0x022e && g_ram[0x1fff4]<255) ++g_ram[0x1fff4];
        if(cpu->A==0x0235 && g_ram[0x1fff5]<255) ++g_ram[0x1fff5];
        if(cpu->A==0x0238 && g_ram[0x1fff1]<255) ++g_ram[0x1fff1];
        return;
    }
    if(pc==0xc45168) {
        if(g_ram[0x9f]==6) g_ram[0x1fff2]=g_ram[0x69];
        return;
    }
    if(pc==0xc4519c) {
        unsigned companion=cpu->X+Sb5MountOffset(g_ram);
        if(companion<24 && g_ram[0x16c2+companion]==6) g_ram[0x1fff3]=g_ram[0x69];
        return;
    }
    unsigned choice=g_ram[0x1fff0];
    if(choice && choice<=7) {
        cpu->A=(uint16_t)((cpu->A&0xff00)|(choice-1));
        cpu->_flag_Z=choice==1;cpu->_flag_N=0;
    }
}
void LouieFixture(void) {
    interp_bridge_add_pre_opcode_hook(0xc4515d,egg_choice);
    interp_bridge_add_pre_opcode_hook(0xc00ced,egg_choice);
    interp_bridge_add_pre_opcode_hook(0xc45168,egg_choice);
    interp_bridge_add_pre_opcode_hook(0xc4519c,egg_choice);
}
