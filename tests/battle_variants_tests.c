/* Battle stage variants: menu input, password bits, restore, item rule,
 * cartridge layout parsing on a synthetic image, save-state extension. */
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
#define BATTLE_VARIANTS_TEST 1
#include "../src/battle_variants_mod.c"
static bool ptbr(void) { return false; }
uint8 g_ram[0x20000];
const uint8 *g_rom;
static uint8 *image;
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
static void put24(uint32_t pc, uint32_t v) { at(pc)[0] = (uint8_t)v; at(pc)[1] = (uint8_t)(v >> 8); at(pc)[2] = (uint8_t)(v >> 16); }
static CpuState cpu;
enum { HELD = 0x2c, DIRS = 0x31, FRESH = 0x36, FRESH_DIRS = 0x3b };
static void frame(unsigned seat, uint8_t act, uint8_t dir) {
    memset(g_ram + cpu.D + HELD, 0, 20);
    g_ram[cpu.D + HELD + seat] = act; g_ram[cpu.D + FRESH + seat] = act;
    g_ram[cpu.D + DIRS + seat] = dir; g_ram[cpu.D + FRESH_DIRS + seat] = dir;
    hook(&cpu, PAD_HOOK);
}
static void selector(void) {
    g_ram[0x1031] = 4;
    g_ram[0x0c90] = 0x8d; g_ram[0x0cb8] = 0xa2; g_ram[0x0ce0] = 0xc6; /* $C6:A28D */
}

int main(void) {
    image = calloc(0x400000, 1); g_rom = image;
    CHECK(image && !strcmp(plugin, "superbomberman5.battle.variants"));
    /* Synthetic arena 1 / variant 0 (mode $74): header -> handler list with
     * setup (1) testing flag 3, map handler (2) "27 20 0a 01 01 00" and
     * actor script (4) with P1 (1,1) and P2 (13,11). */
    put24(0xc60000 + 0x73 * 3, 0xc61000);
    put24(0xc61000, 0xc61010);
    uint8_t list[] = {1, 0x00, 0x11, 0xc6, 2, 0x20, 0x11, 0xc6, 4, 0x40, 0x11, 0xc6, 0};
    memcpy(at(0xc61010), list, sizeof(list));
    uint8_t setup[] = {0x08, 0x03, 0x49, 0x04, 0x10, 0x00, 0x00};
    memcpy(at(0xc61100), setup, sizeof(setup));
    uint8_t maps[] = {0x2a, 0x1e, 0x27, 0x20, 0x0a, 0x01, 0x01, 0x00};
    memcpy(at(0xc61120), maps, sizeof(maps));
    uint8_t actors[] = {0x0e, 0x22, 0x01, 0x01, 0x01, 0x22, 0x02, 0x0d, 0x0b, 0x00};
    memcpy(at(0xc61140), actors, sizeof(actors));
    /* Map 1 -> stream: 256 cells; row 0 metatile 0 (outside), the rest 1
     * (floor) except cell $22 = 2 (hard). Tileset 1 -> attributes. */
    put24(0xd0a000, 0xd0b000); put24(0xd0b000, 0xd0c000);
    uint8_t stream[] = {0x1f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        0x01, 1, 2, 0x5f /* 0x40 prev 32 times */};
    memcpy(at(0xd0c000), stream, sizeof(stream));
    for (unsigned k = 0; k < 7; ++k) at(0xd0c000 + sizeof(stream))[k] = 0x5f; /* keep copying "previous" */
    at(0xd08000)[0] = 0x00; at(0xd08000)[1] = 0xe0; /* attributes at $D0:E000 */
    at(0xd0e001)[0] = 0x00; at(0xd0e002)[0] = 0x80;

    Layout l;
    build_layout(1, 0, &l);
    CHECK(l.ok && l.solid[0] && l.solid[31] && !l.solid[32] && l.solid[33]);
    CHECK(l.spawns == 3 && l.spawn_x[0] == 1 && l.spawn_y[0] == 1 && l.spawn_x[1] == 13 && l.spawn_y[1] == 11);
    CHECK(items_supported(1) && !items_supported(2));

    /* Disabled: no hook, nothing changes. */
    BattleVariantsBeforeFrame(); CHECK(!hook);
    activation(); BattleVariantsBeforeFrame(); CHECK(hook && hook_pc == 0xc00af9 && prepared);
    cpu.D = 0;
    g_ram[0x625a] = 1; g_ram[0x6274] = 0x41; g_ram[0x6273] = 0x27;

    /* Outside the selector the A goes through. */
    g_ram[0x1031] = 4; g_ram[0x0ce0] = 0xc7;
    frame(0, 0x80, 0); CHECK(!open_menu && g_ram[FRESH] == 0x80);
    /* A on the selector opens the list and is swallowed. */
    selector();
    frame(1, 0x80, 0); CHECK(open_menu && cursor == 0 && g_ram[FRESH + 1] == 0 && g_ram[HELD + 1] == 0);
    frame(2, 0, 0x04); CHECK(cursor == 1 && g_ram[FRESH_DIRS + 2] == 0);
    frame(0, 0, 0x08); frame(0, 0, 0x08); CHECK(cursor == 4); /* wraps to Items (arena 1) */
    /* B closes; the carousel does not see it. */
    frame(3, 0x08, 0); CHECK(!open_menu && g_ram[FRESH + 3] == 0);
    /* Reopen, choose 0413 (third entry), confirm with seat 2. */
    frame(0, 0x80, 0); frame(0, 0, 0x04); frame(0, 0, 0x04); CHECK(open_menu && cursor == 2);
    frame(2, 0x80, 0);
    CHECK(!open_menu && inject_seat == 3 && restore_pending);
    CHECK(g_ram[0x6274] == 0x42 && g_ram[0x6273] == 0x00 && g_ram[FRESH + 2] == 0);
    /* Next pad read hands the A back to the same seat. */
    frame(2, 0, 0); CHECK(g_ram[FRESH + 2] == 0x80 && g_ram[HELD + 2] == 0x80 && !inject_seat);
    /* Restored once the stage has started. */
    BattleVariantsAfterFrame(); CHECK(g_ram[0x6274] == 0x42);
    g_ram[0x1031] = 0x76; BattleVariantsAfterFrame();
    CHECK(!restore_pending && g_ram[0x6274] == 0x41 && g_ram[0x6273] == 0x27);

    /* Item rule: bit 5 plus script flag 3, variant bits cleared. */
    selector(); g_ram[0x2900] = 0;
    frame(0, 0x80, 0); frame(0, 0, 0x08); CHECK(cursor == 4);
    frame(0, 0x80, 0); CHECK(g_ram[0x6274] == 0x60 && (g_ram[0x2900] & 0x08));
    frame(0, 0, 0);
    /* Start opens the list and confirms too (the carousel accepts it). */
    g_ram[0x1031] = 0x74; BattleVariantsAfterFrame();
    selector(); g_ram[0x2900] = 0;
    frame(1, 0x01, 0); CHECK(open_menu && cursor == 0 && g_ram[FRESH + 1] == 0);
    frame(1, 0, 0x04); frame(1, 0x01, 0);
    CHECK(!open_menu && inject_seat == 2 && g_ram[0x6274] == 0x41 && g_ram[FRESH + 1] == 0);
    frame(1, 0, 0); CHECK(g_ram[FRESH + 1] == 0x80);
    /* Arena without the rule: four entries, the cursor wraps at 3. */
    g_ram[0x1031] = 0x74; BattleVariantsAfterFrame();
    selector(); g_ram[0x625a] = 2;
    frame(0, 0x80, 0); frame(0, 0, 0x08); CHECK(cursor == 3);

    /* Save state round trip, older states, corrupt data. */
    Stream w = {{transfer}, {0}, 0, 0, false};
    BattleVariantsSave(&w.base);
    open_menu = 0; cursor = 0;
    Stream r = w; r.at = 0; r.reading = true;
    BattleVariantsLoad(&r.base, 0); CHECK(!failed && open_menu && cursor == 3);
    Stream empty = {{transfer}, {0}, 0, 0, true};
    BattleVariantsLoad(&empty.base, 0); CHECK(!failed && !open_menu);
    r.at = 0; r.bytes[5] = 9; /* cursor out of range */
    BattleVariantsLoad(&r.base, 0); CHECK(failed);
    failed = false;

    /* Reset closes the list and gives the bits back. */
    open_menu = 1; restore_pending = 1; saved_bits = 0x11; saved_saved_bits = 0x22;
    reset_callback(); CHECK(!open_menu && !restore_pending && g_ram[0x6274] == 0x11 && g_ram[0x6273] == 0x22);
    selector(); frame(0, 0x80, 0); CHECK(!open_menu); /* disabled again */
    puts("Battle variants layout parsing, menu input, bits, item rule, restore and states passed.");
    return 0;
}
