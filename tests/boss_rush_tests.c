/* Boss Rush: title patches and restore, menu remap, the injected 3879
 * handler from the player-count menu, picker hosting/cancel, timer rules
 * and format, pause quit, save-state extension. */
#include "mod_runtime.h"
#include "snes/ppu.h"
#include "snes/saveload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static SNESModActivationCallback activation, reset_callback;
static const char *plugin;
int snes_mod_register_reset_callback(SNESModActivationCallback fn) { reset_callback = fn; return 1; }
int snes_mod_register_activation_plugin(const char *id, SNESModActivationCallback fn) { plugin = id; activation = fn; return 1; }
#include "snes/interp_bridge.h"
static InterpPreOpcodeHook hook;
static uint32_t hook_pc;
bool interp_bridge_add_pre_opcode_hook(uint32_t pc, InterpPreOpcodeHook fn) { hook_pc = pc; hook = fn; return true; }
static uint32_t redirect_pc;
void interp_bridge_pre_opcode_redirect(uint32_t pc) { redirect_pc = pc; }
static uint8 *image;
uint8_t *SnesRomPtr(uint32 v) { return image + (((v >> 16) & 0x3f) << 16 | (v & 0xffff)); }
static bool characters, snapshot_ok = true, open_ok = true;
static int snapshots, opens;
bool NormalCharacterEnabled(void) { return characters; }
bool NormalCharacterSnapshotBossRush(void) { ++snapshots; return snapshot_ok; }
bool NormalCharacterOpenBossRush(void) { ++opens; return open_ok; }
#include "../src/boss_rush_mod.c"
uint8 g_ram[0x20000];
const uint8 *g_rom;
Ppu *g_ppu;
typedef struct { SaveLoadInfo base; uint8 bytes[64]; size_t at, end; bool reading; } Stream;
static bool failed;
static void transfer(SaveLoadInfo *sli, void *data, size_t n) {
    Stream *s = (Stream *)sli;
    if (s->at + n > sizeof(s->bytes) || (s->reading && s->at + n > s->end)) { failed = true; return; }
    if (s->reading) memcpy(data, s->bytes + s->at, n);
    else { memcpy(s->bytes + s->at, data, n); s->end = s->at + n; }
    s->at += n;
}
size_t RtlStateBytesRemaining(SaveLoadInfo *sli) { Stream *s = (Stream *)sli; return s->end - s->at; }
void RtlStateStreamFail(SaveLoadInfo *sli) { (void)sli; failed = true; }
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

static uint8_t *at(uint32_t pc) { return image + (((pc >> 16) & 0x3f) << 16 | (pc & 0xffff)); }
static CpuState cpu;
static void op_at(uint32_t pc, unsigned task) {
    g_ram[0x0a] = (uint8_t)pc; g_ram[0x0b] = (uint8_t)(pc >> 8); g_ram[0x0c] = (uint8_t)(pc >> 16);
    cpu.X = (uint16_t)task; redirect_pc = 0;
    hook(&cpu, VM_DISPATCH);
}
static uint32_t spc(void) { return g_ram[0x0a] | g_ram[0x0b] << 8 | (uint32_t)g_ram[0x0c] << 16; }
static void set_task(unsigned task, uint32_t pc) {
    g_ram[0x0c87 + task] = (uint8_t)pc; g_ram[0x0caf + task] = (uint8_t)(pc >> 8); g_ram[0x0cd7 + task] = (uint8_t)(pc >> 16);
}
/* Each injected routine returns with RTL to the dispatch JSL. */
static int returns(uint32_t routine) {
    if (redirect_pc != routine) return 0;
    uint16_t s = cpu.S;
    if (g_ram[s + 1] != (uint8_t)(VM_DISPATCH - 1) || g_ram[s + 2] != (uint8_t)((VM_DISPATCH - 1) >> 8) ||
        g_ram[s + 3] != (uint8_t)(VM_DISPATCH >> 16))
        return 0;
    cpu.S = (uint16_t)(s + 3); /* RTL */
    cpu.A = 0x1234; cpu.X = 0x55; cpu.P = 0x30;
    redirect_pc = 0;
    hook(&cpu, VM_DISPATCH);
    return 1;
}
static void original_bytes(void) {
    static const uint8_t limit[12] = {0x5d, 0x36, 0x10, 0x86, 0x00, 0x49, 0x05, 0x5b, 0x02, 0x5f, 0x37, 0x10};
    static const uint8_t confirm[7] = {0x5d, 0x36, 0x10, 0x86, 0x00, 0x49, 0x26};
    static const uint8_t file[4] = {0x5d, 0x1a, 0x63, 0x86};
    memcpy(at(TITLE_LIMIT_OP), limit, 12); memcpy(at(TITLE_CONFIRM), confirm, 7); memcpy(at(FILE_MENU), file, 4);
    at(START_1P)[0] = 0x0f; at(START_1P)[1] = 0x03; at(START_2P)[0] = 0x0f; at(START_2P)[1] = 0x03;
    memset(at(PARK), 0xff, 32);
}
static void title_menu(uint8_t cursor) { g_ram[0x1031] = 2; g_ram[0x1036] = 0; g_ram[0x1032] = cursor; }
static int start_through(uint32_t op) {
    /* The handler runs as three RTL calls, then the scene changes to $67. */
    cpu.S = 0x1f0; cpu.A = 0x0a00; cpu.D = 0; cpu.DB = 0; cpu.P = 0x31;
    op_at(op, 11);
    if (!returns(CLEAR_FLAGS)) return 0;
    if (cpu.A != 0x1234 && (cpu.A & 0xff) != 4) return 0;
    if (!returns(SET_FLAG)) return 0;
    if (!(g_ram[0x6274] & 0x80) || g_ram[0x6248] != 0x67 || g_ram[0x6254]) return 0;
    if (!returns(RESET_PLAYERS)) return 0;
    return cpu.S == 0x1f0 && cpu.A == 0x0a00 && (cpu.X & 0xff) == 11 && cpu.P == 0x31 && redirect_pc == VM_NEXT &&
           spc() == op + 2 && g_ram[0x1031] == 0x67 && g_ram[0x1d1e] == 4;
}

int main(void) {
    image = calloc(0x400000, 1); g_rom = image;
    CHECK(image && !strcmp(plugin, "superbomberman5.boss.rush"));
    original_bytes();
    /* Disabled: nothing installed. */
    BossRushBeforeFrame();
    CHECK(!hook && at(TITLE_LIMIT_OP + 8)[0] == 2);
    /* Unexpected bytes: refused. */
    activation();
    at(START_2P)[1] = 0x04; BossRushBeforeFrame();
    CHECK(!installed && at(TITLE_LIMIT_OP + 8)[0] == 2 && at(PARK)[0] == 0xff);
    at(START_2P)[1] = 0x03; BossRushBeforeFrame();
    CHECK(installed && hook && hook_pc == VM_DISPATCH && at(TITLE_LIMIT_OP + 8)[0] == 3);
    CHECK(at(PARK_1P)[0] == 0xfd && at(PARK_1P)[5] == 0x44);
    /* The park jumps land on their ops. */
    for (unsigned k = 0; k < 3; ++k) {
        uint32_t self = PARK + 8 * k, target = k == 0 ? FILE_MENU : k == 1 ? START_1P : START_2P;
        int16_t rel = (int16_t)(at(self)[6] | at(self)[7] << 8);
        CHECK(((self + 8 + rel) & 0xffff) == (target & 0xffff));
    }
    CHECK(BossRushTitleMenu() == false);
    title_menu(0); CHECK(BossRushTitleMenu());

    /* Menu remap: Boss Rush -> Normal branch (pending), Options -> 2. */
    title_menu(3); op_at(TITLE_CONFIRM, 11); CHECK(g_ram[0x1032] == 2 && !pending);
    title_menu(1); op_at(TITLE_CONFIRM, 11); CHECK(g_ram[0x1032] == 1 && !pending);
    title_menu(2); op_at(TITLE_CONFIRM, 9); CHECK(g_ram[0x1032] == 2 && !pending); /* other task */
    title_menu(2); op_at(TITLE_CONFIRM, 11); CHECK(g_ram[0x1032] == 0 && pending == 1);
    /* Still mode 2 menu 0 for a few frames: kept; the player menu, then B. */
    BossRushAfterFrame(); CHECK(pending == 1);
    g_ram[0x1036] = 1; BossRushAfterFrame(); CHECK(pending == 2);
    g_ram[0x1036] = 0; BossRushAfterFrame(); CHECK(!pending);

    /* Without the picker: the 1P start op runs the handler. */
    title_menu(2); op_at(TITLE_CONFIRM, 11); g_ram[0x1036] = 1; BossRushAfterFrame();
    g_ram[0x1d25] = 1; g_ram[0x1d24] = 1; g_ram[0x6274] = 0x10;
    CHECK(start_through(START_1P));
    CHECK(BossRushActive() && !pending && g_ram[0x1d25] == 1 && g_ram[0x6274] == 0x90);
    /* Timer: counts on screen, not paused, not black. */
    g_ram[0x0300] = 0x0f; g_ram[0x1d1a] = 0; g_ram[0x1d1d] = 0;
    BossRushAfterFrame(); BossRushAfterFrame(); CHECK(frames == 2);
    g_ram[0x1d1d] = 1; BossRushAfterFrame(); CHECK(frames == 2);
    g_ram[0x1d1d] = 0; g_ram[0x0300] = 0x80; BossRushAfterFrame(); CHECK(frames == 2);
    g_ram[0x0300] = 0x0f; g_ram[0x1031] = 0x74; g_ram[0x1d1a] = 1; BossRushAfterFrame(); CHECK(frames == 2);
    g_ram[0x1031] = 0x68; g_ram[0x1d1a] = 0; BossRushAfterFrame(); CHECK(frames == 3);
    char text[16];
    BossRushFormat(0, text, sizeof(text)); CHECK(!strcmp(text, "00:00.00"));
    BossRushFormat(60 * 75 + 30, text, sizeof(text)); CHECK(!strcmp(text, "01:15.50"));
    /* Save/load round trip and corrupt rejection. */
    Stream st = {{transfer}, {0}, 0, 0, false};
    BossRushSave(&st.base);
    CHECK(!failed && st.end == 16);
    frames = 99; active = 0;
    st.at = 0; st.reading = true; BossRushLoad(&st.base, 1);
    CHECK(!failed && frames == 3 && active == 1 && saved_bit == 0);
    st.bytes[5] = 9; st.at = 0; BossRushLoad(&st.base, 1); CHECK(failed);
    failed = false; st.bytes[5] = 1;
    Stream empty = {{transfer}, {0}, 0, 0, true};
    BossRushLoad(&empty.base, 1); CHECK(!failed && !active && !frames);
    st.at = 0; BossRushLoad(&st.base, 1); CHECK(!failed && active);
    /* Completion: leaving ZONE5 - J ($6F) for the ending ($AA) freezes the
     * time; the ending's cutscenes reuse boss stage numbers. Reaching $AA
     * from another stage is not a completion. */
    unsigned before = frames;
    g_ram[0x1031] = 0x6e; BossRushAfterFrame(); g_ram[0x1031] = 0xaa; BossRushAfterFrame();
    CHECK(!BossRushFinished());
    g_ram[0x1031] = 0x6f; BossRushAfterFrame(); CHECK(frames == before + 2 && !BossRushFinished());
    g_ram[0x1031] = 0xaa; BossRushAfterFrame(); CHECK(BossRushFinished());
    g_ram[0x1031] = 0x67; BossRushAfterFrame(); BossRushAfterFrame(); CHECK(frames == before + 2 && BossRushFinished());
    /* ...and survives a save state. */
    Stream fin = {{transfer}, {0}, 0, 0, false};
    BossRushSave(&fin.base);
    finished = 0; fin.at = 0; fin.reading = true; BossRushLoad(&fin.base, 1);
    CHECK(!failed && BossRushFinished());
    fin.bytes[4 + 4] = 2; fin.at = 0; BossRushLoad(&fin.base, 1); CHECK(failed);
    failed = false; fin.bytes[4 + 4] = 1; fin.at = 0; BossRushLoad(&fin.base, 1); CHECK(!failed);
    /* Pause: back to the title, never awarding the stage. */
    g_ram[0x1031] = 0x67; g_ram[0x1d1a] = 0x40; g_ram[0x1d1d] = 1; g_ram[0x1d1e] = 0;
    BossRushQuitToTitle();
    CHECK(g_ram[0x1031] == 2 && g_ram[0x1d1e] == 4 && !g_ram[0x1d1d] && !(g_ram[0x1d1a] & 0x40));
    g_ram[0x6274] = 0x90; BossRushAfterFrame();
    CHECK(!BossRushActive() && g_ram[0x6274] == 0x10);

    /* With the picker: park, snapshot, host the picker, come back, start. */
    characters = true; g_ram[0x1d1e] = 0;
    title_menu(2); op_at(TITLE_CONFIRM, 11); g_ram[0x1036] = 1; BossRushAfterFrame();
    g_ram[0x1d25] = 3; g_ram[0x1d24] = 3;
    op_at(START_2P, 11); CHECK(picker == PICK_SNAPSHOT && spc() == PARK_2P && !redirect_pc);
    set_task(11, PARK_2P + 5); BossRushAfterFrame(); CHECK(snapshots == 1 && picker == PICK_HOSTING);
    op_at(PARK_2P, 11); CHECK(spc() == START_2P); /* the native "0F 03" runs */
    g_ram[0x1031] = 3; op_at(FILE_MENU, 9); CHECK(spc() == PARK_FILE);
    set_task(9, PARK_FILE); BossRushAfterFrame(); CHECK(opens == 1);
    /* The Normal character mod restores the title and reports. */
    g_ram[0x1031] = 2; g_ram[0x1036] = 1;
    BossRushPickerDone(false); CHECK(picker == PICK_DONE && BossRushKeepChoices() && g_ram[0x0300] == 0x80);
    CHECK(start_through(START_2P));
    CHECK(BossRushActive() && g_ram[0x1d25] == 3 && g_ram[0x1d24] == 3 && !BossRushKeepChoices());
    g_ram[0x1031] = 2; BossRushAfterFrame(); CHECK(!BossRushActive());
    /* Cancel: back to the title scene. */
    g_ram[0x1d1e] = 0; title_menu(2); op_at(TITLE_CONFIRM, 11); g_ram[0x1036] = 1; BossRushAfterFrame();
    op_at(START_1P, 11); set_task(11, PARK_1P); BossRushAfterFrame();
    g_ram[0x1031] = 2; BossRushPickerDone(true);
    op_at(PARK_1P, 11);
    CHECK(!pending && picker == PICK_NONE && g_ram[0x1031] == 2 && g_ram[0x1d1e] == 4 && spc() == START_1P + 2 &&
          redirect_pc == VM_NEXT && !BossRushActive());
    /* Snapshot refused: start without the picker. */
    g_ram[0x1d1e] = 0; snapshot_ok = false;
    title_menu(2); op_at(TITLE_CONFIRM, 11); g_ram[0x1036] = 1; BossRushAfterFrame();
    op_at(START_1P, 11); set_task(11, PARK_1P); BossRushAfterFrame(); CHECK(picker == PICK_DONE);
    CHECK(start_through(START_1P));
    g_ram[0x1031] = 2; BossRushAfterFrame();
    /* Picker lost (state loaded while hosting): the Normal file menu goes on. */
    snapshot_ok = true; open_ok = false; g_ram[0x1d1e] = 0;
    title_menu(2); op_at(TITLE_CONFIRM, 11); g_ram[0x1036] = 1; BossRushAfterFrame();
    op_at(START_1P, 11); set_task(11, PARK_1P); BossRushAfterFrame();
    g_ram[0x1031] = 3; set_task(9, PARK_FILE); BossRushAfterFrame(); CHECK(!pending && picker == PICK_NONE);

    /* Reset restores every patched byte. */
    reset_callback();
    CHECK(at(TITLE_LIMIT_OP + 8)[0] == 2 && at(PARK)[0] == 0xff && at(PARK + 23)[0] == 0xff && !installed && !enabled);
    puts("boss rush tests passed");
    return 0;
}
