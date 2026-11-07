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
static InterpPreOpcodeHook hooks[16];
static uint32_t hook_pcs[16];
static unsigned hook_count;
bool interp_bridge_add_pre_opcode_hook(uint32_t pc, InterpPreOpcodeHook fn) { hook_pcs[hook_count] = pc; hooks[hook_count++] = fn; return true; }
static uint32_t redirect_pc;
void interp_bridge_pre_opcode_redirect(uint32_t pc) { redirect_pc = pc; }
static void fire(uint32_t pc, CpuState *cpu) { for (unsigned i = 0; i < hook_count; ++i) if (hook_pcs[i] == pc) hooks[i](cpu, pc); }
void PtbrDrawBodyText(struct Ppu *p, unsigned a, unsigned b, unsigned c, unsigned d, unsigned e, const char *t) { (void)p; (void)a; (void)b; (void)c; (void)d; (void)e; (void)t; }
void PtbrBlankMap(struct Ppu *p, unsigned a, unsigned b, unsigned c, unsigned d, unsigned e) { (void)p; (void)a; (void)b; (void)c; (void)d; (void)e; }
static uint8 *image;
uint8_t *SnesRomPtr(uint32 v) { return image + (((v >> 16) & 0x3f) << 16 | (v & 0xffff)); }
static bool louie_owns;
bool LouieOwnsBomb(unsigned b) { (void)b; return louie_owns; }
#define BATTLE_RULES_TEST 1
#include "../src/battle_rules_mod.c"
static bool ptbr(void) { return false; }
uint8 g_ram[0x20000];
const uint8 *g_rom;
Ppu *g_ppu;
typedef struct { SaveLoadInfo base; uint8 bytes[128]; size_t at, end; bool reading; } Stream;
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
    memset(at(0xc57520), 0xff, 0x40); /* sound + domino trampolines */ at(0xc00af9)[0] = 0xfa; at(0xc00af9)[1] = 0x7a; at(0xc00af9)[2] = 0x6b;
    CHECK(image && !strcmp(plugin, "superbomberman5.battle.rules"));
    /* Disabled: nothing happens. */
    rules_screen(5); pad(0, 0, DIR_DOWN); CHECK(!BattleRulesPageOpen() && g_ram[REP] == DIR_DOWN);
    activation(); rules_screen(5); BattleRulesBeforeFrame();
    CHECK(hook_count == 10 && at(KICK_CODE)[0] == 0xda && at(KICK_CODE)[22] == 0x5c);
    /* Down on the last native row opens page 2 and the cartridge sees no key. */
    redirect_pc = 0; pad(0, 0, DIR_DOWN);
    CHECK(redirect_pc == SFX_CODE && at(SFX_ID)[0] == SFX_MOVE);
    CHECK(page == 1 && cur == 0 && g_ram[REP] == 0 && g_ram[FRESH_DIRS] == 0);
    release();
    CHECK(opt[0] == 1 && opt[1] == 0 && opt[2] == 0 && !BattleRulesAbilitiesOff()); /* defaults: start immediately, no shuffle, abilities on */
    redirect_pc = 0; pad(0, 0, DIR_RIGHT); CHECK(opt[0] == 0 && opt[1] == 0 && redirect_pc == SFX_CODE); release();
    pad(0, 0, DIR_DOWN); CHECK(cur == 1); release();
    pad(0, PAD_A, 0); CHECK(opt[1] == 1 && !(g_ram[HELD] & PAD_A)); release();
    pad(0, 0, DIR_DOWN); CHECK(cur == 2); release();
    pad(0, 0, DIR_LEFT); CHECK(opt[2] == 1 && BattleRulesAbilitiesOff()); release();
    pad(0, PAD_A, 0); CHECK(opt[2] == 0 && !BattleRulesAbilitiesOff()); release();
    pad(0, 0, DIR_DOWN); CHECK(cur == 3 && !BattleRulesDomino()); release(); /* domino defaults off */
    pad(0, 0, DIR_RIGHT); CHECK(opt[3] == 1 && BattleRulesDomino()); release();
    pad(0, PAD_A, 0); CHECK(opt[3] == 0); release();
    pad(0, 0, DIR_DOWN); CHECK(cur == 4 && opt[4] == 3 && BattleRulesThrowRange() == 3); release(); /* throw range defaults to 3 */
    pad(0, 0, DIR_RIGHT); CHECK(opt[4] == 4); release();
    pad(0, PAD_A, 0); CHECK(opt[4] == 5); release();
    pad(0, 0, DIR_RIGHT); CHECK(opt[4] == 1); release(); /* wraps */
    pad(0, 0, DIR_LEFT); CHECK(opt[4] == 5); release();
    pad(0, 0, DIR_LEFT); pad(0, 0, 0); pad(0, 0, DIR_LEFT); CHECK(opt[4] == 3); release();
    /* Down on the last rule leaves; entered from row 5 the cartridge gets Down to wrap to row 0. */
    pad(0, 0, DIR_DOWN); CHECK(page == 0 && g_ram[REP] == DIR_DOWN && g_ram[FRESH_DIRS] == DIR_DOWN);
    release();
    /* Up on row 0 enters at the last rule; up from the first rule gives the cartridge Up. */
    rules_screen(0); pad(0, 0, DIR_UP); CHECK(page == 1 && cur == 4); release();
    pad(0, 0, DIR_UP); CHECK(cur == 3); release();
    pad(0, 0, DIR_UP); CHECK(cur == 2); release();
    pad(0, 0, DIR_UP); CHECK(cur == 1); release();
    pad(0, 0, DIR_UP); CHECK(cur == 0); release();
    pad(0, 0, DIR_UP); CHECK(page == 0 && g_ram[REP] == DIR_UP && g_ram[FRESH_DIRS] == DIR_UP); /* row 0 + Up wraps natively to Bowling */
    release();
    /* Leaving the screen forgets the page. */
    rules_screen(5); pad(0, 0, DIR_DOWN); release(); g_ram[0x1031] = 5; BattleRulesBeforeFrame(); CHECK(page == 0);
    /* Countdown: only with the rule on, in Battle. */
    g_ram[0x1031] = 0x74; g_ram[0x1d1a] = 1; g_ram[0x0aee] = 0; g_ram[0x0e1c] = 7; g_ram[0x0e1d] = 5; g_ram[0x0e1e] = 2;
    opt[0] = 1; fire(ROUND_HOOK, &cpu); CHECK(countdown == 0);
    opt[0] = 0; fire(ROUND_HOOK, &cpu); CHECK(countdown == 0); /* no stage init: Hurry Up re-raises READY, no countdown */
    fire(INIT_HOOK, &cpu); fire(ROUND_HOOK, &cpu); CHECK(countdown == COUNT_FRAMES);
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
    /* Domino: bomb 0 rolls right (dir 1) into cell $25, where bomb 1 rests. */
    g_ram[0x1031] = 0x74; g_ram[0x1d1a] = 1; g_ram[0x1d1d] = 0;
    g_ram[0x0e40] = 50; g_ram[0x0e7c] = 0x24; g_ram[0x0e90] = 1;
    g_ram[0x0e41] = 50; g_ram[0x0e7d] = 0x25; g_ram[0x0e91] = 0;
    cpu.X = 0; cpu.A = 0x25;
    opt[3] = 0; redirect_pc = 0; fire(ROLL_HOOK, &cpu); CHECK(redirect_pc == 0);
    opt[3] = 1; fire(ROLL_HOOK, &cpu);
    CHECK(redirect_pc == KICK_CODE && at(KICK_CELL)[0] == 0x24 && at(KICK_DIR)[0] == 1);
    louie_owns = true; redirect_pc = 0; kick_resuming = false; fire(ROLL_HOOK, &cpu); CHECK(redirect_pc == 0); louie_owns = false; /* Louie's chain */
    kick_resuming = true; redirect_pc = 0; fire(ROLL_HOOK, &cpu); CHECK(redirect_pc == 0); /* resumed from the trampoline: native stop */
    g_ram[0x0e90] = 4; g_ram[0x0e7d] = 0x34; cpu.A = 0x34; fire(ROLL_HOOK, &cpu); /* rolling up into $34 */
    CHECK(redirect_pc == KICK_CODE && at(KICK_CELL)[0] == 0x44 && at(KICK_DIR)[0] == 4);
    redirect_pc = 0; fire(ROLL_HOOK, &cpu);
    g_ram[0x0e91] = 0x10; fire(ROLL_HOOK, &cpu); CHECK(redirect_pc == 0); /* the other bomb is airborne/moving */
    g_ram[0x0e91] = 0; cpu.A = 0x35; fire(ROLL_HOOK, &cpu); CHECK(redirect_pc == 0); /* no bomb in the blocked cell */
    g_ram[0x1031] = 0x20; g_ram[0x1d1a] = 0; cpu.A = 0x34; fire(ROLL_HOOK, &cpu); CHECK(redirect_pc == 0); /* Battle only */
    g_ram[0x1031] = 0x74; g_ram[0x1d1a] = 1;
    /* Throw range: the arc $2C-$37 walked in 4 * range frames, 4 px each. */
    for (unsigned range = 1; range <= 5; ++range) {
        opt[4] = (uint8_t)range; cpu.X = 7; fire(THROW_HOOK, &cpu);
        CHECK((throw_step[7] != 0) == (range != 3));
        g_ram[0x0f10 + 7] = 0x2c;
        unsigned frames = 0; uint8_t last = 0;
        while (frames < 40) {
            fire(ARC_HOOK, &cpu);
            uint8_t idx = g_ram[0x0f10 + 7]; ++frames;
            CHECK(idx >= last && idx >= 0x2c && idx <= 0x37); last = idx;
            g_ram[0x0f10 + 7] = (uint8_t)(idx + 1); /* the native INC */
            if (idx == 0x37) break; /* height 0: lands */
        }
        CHECK(frames == 4 * range && throw_step[7] == 0);
    }
    opt[4] = 5; cpu.X = 7; fire(THROW_HOOK, &cpu); g_ram[0x0f10 + 7] = 0x38; fire(ARC_HOOK, &cpu); /* bounce arc: native */
    CHECK(g_ram[0x0f10 + 7] == 0x38 && throw_step[7] == 0);
    g_ram[0x1031] = 0x20; g_ram[0x1d1a] = 0; fire(THROW_HOOK, &cpu); CHECK(throw_step[7] == 0); /* Battle only */
    g_ram[0x1031] = 0x74; g_ram[0x1d1a] = 1; opt[4] = 3;
    /* Save state. */
    opt[0] = 0; opt[1] = 1; opt[2] = 1; opt[3] = 1; opt[4] = 2; throw_step[3] = 5; page = 1; cur = 2; countdown = 77; rng = 0x12345; frames = 9;
    Stream out = {{transfer}, {0}, 0, 0, false};
    failed = false; BattleRulesSave(&out.base); CHECK(!failed);
    Stream in = out; in.at = 0; in.reading = true;
    opt[0] = 1; opt[1] = 0; opt[2] = 0; page = cur = 0; countdown = 0; rng = 1; frames = 0;
    BattleRulesLoad(&in.base, 1); CHECK(!failed && !opt[0] && opt[1] && opt[2] && opt[3] && opt[4] == 2 && throw_step[3] == 5 && page == 1 && cur == 2 && countdown == 77 && rng == 0x12345 && frames == 9);
    Stream bad = out; bad.at = 0; bad.reading = true; bad.bytes[4] = 5; /* invalid option */
    BattleRulesLoad(&bad.base, 1); CHECK(failed);
    Stream old = out; old.at = 0; old.reading = true; old.end -= 22; failed = false; /* state saved before the domino byte */
    BattleRulesLoad(&old.base, 1); CHECK(!failed && opt[2] && !opt[3] && opt[4] == 3);
    Stream bad2 = out; bad2.at = 0; bad2.reading = true; bad2.bytes[out.end - 21] = 6; /* range 6 */ failed = false;
    BattleRulesLoad(&bad2.base, 1); CHECK(failed);
    reset_callback(); CHECK(opt[3] == 0 && opt[4] == 3 && at(KICK_CODE)[0] == 0xff && opt[0] == 1 && opt[1] == 0 && opt[2] == 0 && countdown == 0);
    puts("battle_rules_tests ok");
    return 0;
}
