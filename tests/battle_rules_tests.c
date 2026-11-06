/* Battle rules page 2: page navigation and toggles through the pad reader,
 * native Down/Up hand-back, round countdown (freeze, input block, timer),
 * spawn permutation on a synthetic stage script, save-state extension. */
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
static InterpPreOpcodeHook hooks[8];
static uint32_t hook_pcs[8];
static unsigned hook_count;
bool interp_bridge_add_pre_opcode_hook(uint32_t pc, InterpPreOpcodeHook fn) { hook_pcs[hook_count] = pc; hooks[hook_count++] = fn; return true; }
static uint32_t redirect_pc;
void interp_bridge_pre_opcode_redirect(uint32_t pc) { redirect_pc = pc; }
static void fire(uint32_t pc, CpuState *cpu) { for (unsigned i = 0; i < hook_count; ++i) if (hook_pcs[i] == pc) hooks[i](cpu, pc); }
void PtbrDrawBodyText(struct Ppu *p, unsigned a, unsigned b, unsigned c, unsigned d, unsigned e, const char *t) { (void)p; (void)a; (void)b; (void)c; (void)d; (void)e; (void)t; }
void PtbrBlankMap(struct Ppu *p, unsigned a, unsigned b, unsigned c, unsigned d, unsigned e) { (void)p; (void)a; (void)b; (void)c; (void)d; (void)e; }
static uint8 *image;
uint8_t *SnesRomPtr(uint32 v) { return image + (((v >> 16) & 0x3f) << 16 | (v & 0xffff)); }
#define BATTLE_RULES_TEST 1
#include "../src/battle_rules_mod.c"
static bool ptbr(void) { return false; }
uint8 g_ram[0x20000];
const uint8 *g_rom;
Ppu *g_ppu;
typedef struct { SaveLoadInfo base; uint8 bytes[96]; size_t at, end; bool reading; } Stream;
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
static void put24(uint32_t pc, uint32_t v) { at(pc)[0] = (uint8_t)v; at(pc)[1] = (uint8_t)(v >> 8); at(pc)[2] = (uint8_t)(v >> 16); }
static CpuState cpu;
enum { HELD = 0x2c, DIRS = 0x31, FRESH = 0x36, FRESH_DIRS = 0x3b, REP = 0x40 };
static void rules_screen(unsigned row) {
    g_ram[0x1031] = 4; g_ram[0x1032] = (uint8_t)row;
    g_ram[0x0c90] = 0x45; g_ram[0x0cb8] = 0x93; g_ram[0x0ce0] = 0xc6; /* $C6:9345 */
    BattleRulesBeforeFrame();
}
static void pad(unsigned seat, uint8_t act, uint8_t dir) {
    memset(g_ram + cpu.D + HELD, 0, 5); memset(g_ram + cpu.D + DIRS, 0, 5); memset(g_ram + cpu.D + FRESH, 0, 5);
    memset(g_ram + cpu.D + FRESH_DIRS, 0, 5); memset(g_ram + cpu.D + REP, 0, 5);
    g_ram[cpu.D + HELD + seat] = act; g_ram[cpu.D + DIRS + seat] = dir;
    g_ram[cpu.D + REP + seat] = dir; g_ram[cpu.D + FRESH_DIRS + seat] = dir;
    fire(PAD_HOOK, &cpu);
}
static void release(void) { pad(0, 0, 0); pad(0, 0, 0); }
int main(void) {
    image = calloc(0x400000, 1); g_rom = image; cpu.D = 0;
    memset(at(0xc57520), 0xff, 0x40); at(0xc00af9)[0] = 0xfa; at(0xc00af9)[1] = 0x7a; at(0xc00af9)[2] = 0x6b;
    CHECK(image && !strcmp(plugin, "superbomberman5.battle.rules"));
    /* Disabled: nothing happens. */
    rules_screen(5); pad(0, 0, DIR_DOWN); CHECK(!BattleRulesPageOpen() && g_ram[REP] == DIR_DOWN);
    activation(); rules_screen(5); BattleRulesBeforeFrame();
    CHECK(hook_count == 6);
    /* Down on the last native row opens page 2 and the cartridge sees no key. */
    redirect_pc = 0; pad(0, 0, DIR_DOWN);
    CHECK(redirect_pc == SFX_CODE && at(SFX_ID)[0] == SFX_MOVE);
    CHECK(page == 1 && cur == 0 && g_ram[REP] == 0 && g_ram[FRESH_DIRS] == 0);
    release();
    CHECK(opt[0] == 1 && opt[1] == 0); /* defaults: start immediately, no shuffle */
    redirect_pc = 0; pad(0, 0, DIR_RIGHT); CHECK(opt[0] == 0 && opt[1] == 0 && redirect_pc == SFX_CODE); release();
    pad(0, 0, DIR_DOWN); CHECK(cur == 1); release();
    pad(0, PAD_A, 0); CHECK(opt[1] == 1 && !(g_ram[HELD] & PAD_A)); release();
    /* Down on the second rule leaves; entered from row 5 the cartridge gets Down to wrap to row 0. */
    pad(0, 0, DIR_DOWN); CHECK(page == 0 && g_ram[REP] == DIR_DOWN && g_ram[FRESH_DIRS] == DIR_DOWN);
    release();
    /* Up on row 0 enters at the last rule; up from the first rule gives the cartridge Up. */
    rules_screen(0); pad(0, 0, DIR_UP); CHECK(page == 1 && cur == 1); release();
    pad(0, 0, DIR_UP); CHECK(cur == 0); release();
    pad(0, 0, DIR_UP); CHECK(page == 0 && g_ram[REP] == DIR_UP && g_ram[FRESH_DIRS] == DIR_UP); /* row 0 + Up wraps natively to Bowling */
    release();
    /* Leaving the screen forgets the page. */
    rules_screen(5); pad(0, 0, DIR_DOWN); release(); g_ram[0x1031] = 5; BattleRulesBeforeFrame(); CHECK(page == 0);
    /* Countdown: only with the rule on, in Battle. */
    g_ram[0x1031] = 0x74; g_ram[0x1d1a] = 1; g_ram[0x0aee] = 0; g_ram[0x0e1c] = 7; g_ram[0x0e1d] = 5; g_ram[0x0e1e] = 2;
    opt[0] = 1; fire(ROUND_HOOK, &cpu); CHECK(countdown == 0);
    opt[0] = 0; fire(ROUND_HOOK, &cpu); CHECK(countdown == COUNT_FRAMES);
    g_ram[0x0aee] = 1; fire(ROUND_HOOK, &cpu); CHECK(countdown == COUNT_FRAMES); /* live frames do not restart it */
    redirect_pc = 0; fire(ACTOR_HOOK, &cpu); CHECK(redirect_pc == ACTOR_RET);
    pad(0, PAD_A, DIR_UP); CHECK(g_ram[HELD] == 0 && g_ram[DIRS] == 0);
    g_ram[0x0e1c] = 1; BattleRulesAfterFrame(); CHECK(g_ram[0x0e1c] == 7 && g_ram[0x0e1d] == 5);
    { unsigned before = countdown; g_ram[0x1d1d] = 1; BattleRulesBeforeFrame(); CHECK(countdown == before); } /* paused: unchanged */
    g_ram[0x1d1d] = 0;
    for (unsigned i = 0; i < COUNT_FRAMES; ++i) BattleRulesBeforeFrame();
    CHECK(countdown == 0); redirect_pc = 0; fire(ACTOR_HOOK, &cpu); CHECK(redirect_pc == 0);
    /* Shuffle on a synthetic stage: P1 (1,1) P2 (13,11) P3 (13,1). */
    put24(0xc60000 + 0x73 * 3, 0xc61000); put24(0xc61000, 0xc61010);
    uint8_t list[] = {4, 0x40, 0x11, 0xc6, 0};
    memcpy(at(0xc61010), list, sizeof(list));
    uint8_t actors[] = {0x22, 1, 1, 1, 0x22, 2, 13, 11, 0x22, 3, 13, 1, 0};
    memcpy(at(0xc61140), actors, sizeof(actors));
    opt[1] = 0; g_ram[0x1d28] = 1; g_ram[0x1d2a] = 1; cpu.X = 0; fire(SPAWN_HOOK, &cpu); CHECK(g_ram[0x1d28] == 1);
    opt[1] = 1;
    for (unsigned round = 0; round < 40; ++round) {
        bool seen[3] = {0}; unsigned distinct = 0;
        for (unsigned seat = 0; seat < 5; ++seat) {
            g_ram[0x1d28] = 99; g_ram[0x1d2a] = 99; cpu.X = seat; fire(SPAWN_HOOK, &cpu);
            if (seat >= 3) { CHECK(g_ram[0x1d28] == 99); continue; } /* no spawn in the script: untouched */
            unsigned k = g_ram[0x1d28] == 1 ? 0 : g_ram[0x1d28] == 13 && g_ram[0x1d2a] == 11 ? 1 : 2;
            CHECK((g_ram[0x1d28] == 1 && g_ram[0x1d2a] == 1) || (g_ram[0x1d28] == 13 && (g_ram[0x1d2a] == 11 || g_ram[0x1d2a] == 1)));
            if (!seen[k]) { seen[k] = true; ++distinct; }
        }
        CHECK(distinct == 3); /* a permutation: every spawn used once */
        CHECK(perm[0] != 0 && perm[1] != 1 && perm[2] != 2); /* nobody keeps its own spawn */
    }
    /* Save state. */
    opt[0] = 0; opt[1] = 1; page = 1; cur = 1; countdown = 77; rng = 0x12345; frames = 9;
    Stream out = {{transfer}, {0}, 0, 0, false};
    failed = false; BattleRulesSave(&out.base); CHECK(!failed);
    Stream in = out; in.at = 0; in.reading = true;
    opt[0] = 1; opt[1] = 0; page = cur = 0; countdown = 0; rng = 1; frames = 0;
    BattleRulesLoad(&in.base, 1); CHECK(!failed && !opt[0] && opt[1] && page == 1 && cur == 1 && countdown == 77 && rng == 0x12345 && frames == 9);
    Stream bad = out; bad.at = 0; bad.reading = true; bad.bytes[4] = 5; /* invalid option */
    BattleRulesLoad(&bad.base, 1); CHECK(failed);
    reset_callback(); CHECK(opt[0] == 1 && opt[1] == 0 && countdown == 0);
    puts("battle_rules_tests ok");
    return 0;
}
