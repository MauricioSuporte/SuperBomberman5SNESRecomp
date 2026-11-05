/* Battle rules, page 2: "round countdown" and "shuffle spawns".
 *
 * Rules screen (mode $1031 = 4, task PC $C6:9345, row $1032 = 0-5, wrapping):
 * with the plugin on, Down on the last row (Bowling Bomber) or Up on the first
 * (CPU Level) opens a second page on the same background that lists only the
 * two new rules. The pad reader's tail ($C0:0AF9, arrays D+$2C/$31/$36/$3B per
 * seat) is where the page is driven, so every peer of a netplay room sees the
 * same choices; moving off the page hands one native Down/Up back to the
 * cartridge so its own cursor wraps to the right row. Both rules default off.
 * The page itself is presentation: the BG1 map cells of the six native rows are
 * blanked while the frame is composed (and put back right after) and the new
 * rows are drawn over the finished frame with the authored menu font.
 *
 * Round countdown: the cartridge marks the round live by storing 1 in $0AEE.
 * Pre-opcode hooks on the stores that make it 1 ($C5:0007, $C2:0BC1, $C5:0912; the first also runs every live frame, hence the test of the old value) start a 3 second countdown during which the actor
 * update loop ($C4:221D, redirected to its RTL at $C4:224E) does not run, so
 * nobody (COM included) moves, the pad arrays are cleared, and the Battle
 * timer bytes $0E1C-$0E1E are put back every frame. The frame is dimmed, the
 * digit is drawn and each human's player number blinks over its spawn point.
 *
 * Shuffle spawns: the actor script op that creates a bomber ($C2:16B8, all
 * paths meet at $C2:16C7) has put the script's tile in $1D28/$1D2A and the seat
 * in X. The hook replaces that tile by the one of another seat of the same
 * stage script (spawns "22 P x y" of handler 4), through a permutation drawn
 * from a deterministic generator that is part of the save state. */
#include "battle_rules_mod.h"
#include "common_rtl.h"
#include "mod_runtime.h"
#include "menu_font.h"
#include "name_tags.h"
#include "ptbr_mod.h"
#include "snes/interp_bridge.h"
#include "common_cpu_infra.h"
#include "snes/ppu.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
    PAD_HOOK = 0xc00af9, ROUND_HOOK = 0xc50007, ROUND_HOOK2 = 0xc20bc1, ROUND_HOOK3 = 0xc50912, ACTOR_HOOK = 0xc4221d, ACTOR_RET = 0xc4224e, SPAWN_HOOK = 0xc216c7,
    RULES_PC = 0xc69345,
    MODE = 0x1031, ROW = 0x1032, FLAGS = 0x1d1a, PAUSE = 0x1d1d, READY = 0x0aee, TIMER = 0x0e1c,
    PAD_A = 0x80, DIR_RIGHT = 0x01, DIR_LEFT = 0x02, DIR_DOWN = 0x04, DIR_UP = 0x08,
    COUNT_FRAMES = 180, SEATS = 5, LAST_ROW = 5,
    MAP = 0x3000, BLANK = 0x100, BLANK_ROW0 = 5, BLANK_ROWS = 18, BLANK_COL0 = 6, BLANK_COLS = 24,
};

static bool enabled, hooks_installed;
/* Simulation state (saved with the machine). */
static uint8_t opt[2], page, cur, suppress, perm[SEATS], perm_valid;
static uint16_t countdown;
static uint8_t timer_bytes[3];
static uint32_t rng = 0x9e3779b9u, frames;
static uint8_t prev_act[SEATS], prev_dir[SEATS];

bool BattleRulesEnabled(void) { return enabled; }
bool BattleRulesCountdownActive(void) { return enabled && countdown != 0; }

static uint8_t rb(uint32_t pc) { return g_rom[((pc >> 16) & 0x3f) << 16 | (pc & 0xffff)]; }
static uint32_t rw(uint32_t pc) { return rb(pc) | (uint32_t)rb(pc + 1) << 8; }
static uint32_t rl(uint32_t pc) { return rw(pc) | (uint32_t)rb(pc + 2) << 16; }
static uint32_t task_pc(void) { return g_ram[0x0c90] | g_ram[0x0cb8] << 8 | (uint32_t)g_ram[0x0ce0] << 16; }
/* The task PC arrays change while the frame runs (the pad reader sees script
 * data there), so the screen is recognised between frames and latched. */
static bool rules_latch;
static bool on_rules(void) { return rules_latch; }
/* The rules screen is a script: its PC wanders through $C6:8340-$93FF while it
 * handles a key and rests at RULES_PC. Enter on the rest PC, stay until the
 * mode changes or the picker/arena selector scripts run. */
static void latch_rules(void) {
    uint32_t pc = task_pc();
    if (g_ram[MODE] != 4) { rules_latch = false; return; }
    if (pc == RULES_PC) rules_latch = true;
    else if (pc == 0xc689cf || pc == 0xc68e68 || (pc >= 0xc6a1ac && pc < 0xc6a33d) || pc < 0xc68000 || pc >= 0xc69400) rules_latch = false;
}
static bool in_battle(void) { return g_ram[MODE] >= 0x74 && (g_ram[FLAGS] & 1); }

static uint32_t next_random(void) {
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}

/* ---- Pad reader: the rules page and the input block ------------------- */

static void clear_pads(unsigned held, unsigned dirs, unsigned fresh, unsigned fresh_dirs) {
    const unsigned rep = fresh_dirs + 5; /* D+$40: first press and auto-repeat, what the menus read */
    for (unsigned s = 0; s < SEATS; ++s) {
        g_ram[(rep + s) & 0xffff] = 0;
        g_ram[(held + s) & 0xffff] = 0; g_ram[(dirs + s) & 0xffff] = 0;
        g_ram[(fresh + s) & 0xffff] = 0; g_ram[(fresh_dirs + s) & 0xffff] = 0;
    }
}

static void pad_hook(CpuState *cpu, uint32_t pc) {
    (void)pc;
    if (!enabled) return;
    const unsigned held = cpu->D + 0x2c, dirs = cpu->D + 0x31, fresh = cpu->D + 0x36, fresh_dirs = cpu->D + 0x3b;
    uint8_t new_act[SEATS], new_dir[SEATS];
    bool any_held = false;
    for (unsigned s = 0; s < SEATS; ++s) {
        uint8_t act = g_ram[(held + s) & 0xffff], dir = g_ram[(dirs + s) & 0xffff];
        new_act[s] = act & (uint8_t)~prev_act[s];
        new_dir[s] = dir & (uint8_t)~prev_dir[s];
        prev_act[s] = act; prev_dir[s] = dir;
        if (act | dir) any_held = true;
    }
    if (countdown) { clear_pads(held, dirs, fresh, fresh_dirs); return; }
    if (!on_rules()) { page = 0; suppress = 0; return; }
    if (suppress) {
        if (any_held) { clear_pads(held, dirs, fresh, fresh_dirs); return; }
        suppress = 0;
        return;
    }
    const unsigned row = g_ram[ROW];
    if (!page) {
        for (unsigned s = 0; s < SEATS; ++s) {
            bool down = (new_dir[s] & DIR_DOWN) && row == LAST_ROW, up = (new_dir[s] & DIR_UP) && row == 0;
            if (!down && !up) continue;
            page = 1; cur = down ? 0 : 1; suppress = 1;
            clear_pads(held, dirs, fresh, fresh_dirs);
            return;
        }
        return;
    }
    /* Page 2: directions and A belong to the page, B and Start still reach the cartridge. */
    bool leave = false;
    uint8_t inject = 0;
    unsigned who = 0;
    for (unsigned s = 0; s < SEATS && !leave; ++s) {
        uint8_t d = new_dir[s];
        if (d & DIR_DOWN) {
            if (cur == 0) cur = 1;
            else { leave = true; who = s; if (row == LAST_ROW) inject = DIR_DOWN; }
            break;
        }
        if (d & DIR_UP) {
            if (cur == 1) cur = 0;
            else { leave = true; who = s; if (row == 0) inject = DIR_UP; }
            break;
        }
        if ((d & (DIR_LEFT | DIR_RIGHT)) || (new_act[s] & PAD_A)) { opt[cur] ^= 1; break; }
    }
    for (unsigned s = 0; s < SEATS; ++s) {
        g_ram[(dirs + s) & 0xffff] = 0; g_ram[(fresh_dirs + s) & 0xffff] = 0; g_ram[(fresh_dirs + 5 + s) & 0xffff] = 0;
        g_ram[(held + s) & 0xffff] &= (uint8_t)~PAD_A; g_ram[(fresh + s) & 0xffff] &= (uint8_t)~PAD_A;
    }
    if (leave) {
        page = 0; suppress = 1;
        if (inject) { g_ram[(dirs + who) & 0xffff] = inject; g_ram[(fresh_dirs + who) & 0xffff] = inject; g_ram[(fresh_dirs + 5 + who) & 0xffff] = inject; }
    }
}

/* ---- Round start, freeze, spawn shuffle ------------------------------- */

static void round_hook(CpuState *cpu, uint32_t pc) {
    (void)cpu; (void)pc;
    if (g_ram[READY] == 1) return; /* these stores also run every frame the round is live */
    perm_valid = 0;
    if (!enabled || !opt[0] || !in_battle()) return;
    countdown = COUNT_FRAMES;
    memcpy(timer_bytes, &g_ram[TIMER], sizeof(timer_bytes));
}

static void actor_hook(CpuState *cpu, uint32_t pc) {
    (void)cpu; (void)pc;
    if (enabled && countdown && !g_ram[PAUSE]) interp_bridge_pre_opcode_redirect(ACTOR_RET);
}

static uint32_t stage_handler(unsigned mode, unsigned id) {
    uint32_t list = rl(rl(0xc60000 + (mode - 1) * 3));
    for (unsigned k = 0; k < 16 && rb(list); ++k, list += 4)
        if (rb(list) == id) return rl(list + 1);
    return 0;
}

static uint8_t spawn_x[SEATS], spawn_y[SEATS], spawn_mask;
static void read_spawns(unsigned mode) {
    spawn_mask = 0;
    uint32_t actors = stage_handler(mode, 4);
    for (uint32_t i = actors; actors && i < actors + 64;) {
        if (rb(i) == 0x22 && rb(i + 1) >= 1 && rb(i + 1) <= SEATS) {
            unsigned p = rb(i + 1) - 1u;
            spawn_x[p] = rb(i + 2); spawn_y[p] = rb(i + 3);
            spawn_mask |= (uint8_t)(1u << p); i += 4; continue;
        }
        if (spawn_mask) break;
        ++i;
    }
}

static void make_permutation(void) {
    uint8_t seats[SEATS];
    unsigned n = 0;
    rng ^= frames * 2654435761u; if (!rng) rng = 0x9e3779b9u;
    for (unsigned s = 0; s < SEATS; ++s) if (spawn_mask >> s & 1) seats[n++] = (uint8_t)s;
    for (unsigned s = 0; s < SEATS; ++s) perm[s] = (uint8_t)s;
    uint8_t shuffled[SEATS];
    memcpy(shuffled, seats, sizeof(shuffled));
    for (unsigned i = n; i > 1; --i) {
        unsigned j = next_random() % i;
        uint8_t t = shuffled[i - 1]; shuffled[i - 1] = shuffled[j]; shuffled[j] = t;
    }
    for (unsigned i = 0; i < n; ++i) perm[seats[i]] = shuffled[i];
    perm_valid = 1;
}

static void spawn_hook(CpuState *cpu, uint32_t pc) {
    (void)pc;
    if (!enabled || !opt[1] || !in_battle()) return;
    unsigned seat = cpu->X & 0xff;
    if (seat >= SEATS) return;
    if (!perm_valid || seat == 0) {
        read_spawns(g_ram[MODE]);
        if (!spawn_mask) return;
        make_permutation();
    }
    if (!(spawn_mask >> seat & 1)) return;
    unsigned to = perm[seat];
    g_ram[0x1d28] = spawn_x[to]; g_ram[0x1d2a] = spawn_y[to];
}

void BattleRulesBeforeFrame(void) {
    if (!enabled) return;
    if (!hooks_installed) {
        hooks_installed = interp_bridge_add_pre_opcode_hook(PAD_HOOK, pad_hook);
        interp_bridge_add_pre_opcode_hook(ROUND_HOOK, round_hook);
        interp_bridge_add_pre_opcode_hook(ROUND_HOOK2, round_hook);
        interp_bridge_add_pre_opcode_hook(ROUND_HOOK3, round_hook);
        interp_bridge_add_pre_opcode_hook(ACTOR_HOOK, actor_hook);
        interp_bridge_add_pre_opcode_hook(SPAWN_HOOK, spawn_hook);
    }
    ++frames;
    latch_rules();
    if (g_ram[MODE] < 0x74) { countdown = 0; if (g_ram[MODE] != 4) perm_valid = 0; }
    if (countdown && !g_ram[PAUSE]) --countdown;
    if (!on_rules()) page = 0;
}

void BattleRulesAfterFrame(void) {
    latch_rules();
    if (enabled && countdown) memcpy(&g_ram[TIMER], timer_bytes, sizeof(timer_bytes));
}

/* ---- Drawing ----------------------------------------------------------- */

static bool ptbr(void);
static uint16_t saved_oam0, attr_label, attr_blue, attr_red;
static bool blanked;

void BattleRulesPrepareDraw(void) {
    blanked = false;
    if (!enabled || page != 1 || !g_ppu || !on_rules()) return;
    attr_label = g_ppu->vram[(MAP + 6 * 32 + 8) & 0x7fff] & 0x3c00;
    attr_blue = g_ppu->vram[(MAP + 6 * 32 + 22) & 0x7fff] & 0x3c00;
    attr_red = g_ppu->vram[(MAP + 15 * 32 + 22) & 0x7fff] & 0x3c00;
    saved_oam0 = g_ppu->oam[0];
    /* The native hand, moved to the row of the page's cursor (rows 12 and 15). */
    g_ppu->oam[0] = (uint16_t)((saved_oam0 & 0xff) | (unsigned)(95 + 24 * cur) << 8);
    ++g_ppu->vramWriteCount; ++g_ppu->oamWriteCount;
    blanked = true;
}

bool BattleRulesPageOpen(void) { return enabled && page == 1 && on_rules(); }

void BattleRulesApplyText(struct Ppu *ppu) {
    if (!blanked) return;
    const bool pt = ptbr();
    /* After the translation pass has read its guard tiles (the banner needs the native row 6). */
    PtbrBlankMap(ppu, BLANK_ROW0, BLANK_COL0, BLANK_ROWS, BLANK_COLS, BLANK);
    static const char *const names_en[2] = {"Round Timer", "Shuffle Spawn"}, *const names_pt[2] = {"Cron\303\264metro", "Embaralhar"};
    for (unsigned i = 0; i < 2; ++i) {
        const unsigned row = 12 + 3 * i, tile = 584 + i * 34;
        PtbrDrawBodyText(ppu, row, 8, 11, tile, attr_label, (pt ? names_pt : names_en)[i]);
        PtbrDrawBodyText(ppu, row, 22, 6, tile + 22, opt[i] ? attr_red : attr_blue,
                         opt[i] ? (pt ? "Sim" : "Yes") : (pt ? "N\303\243o" : "No"));
    }
}

void BattleRulesFinishDraw(void) {
    if (!blanked || !g_ppu) return;
    g_ppu->oam[0] = saved_oam0;
    ++g_ppu->vramWriteCount; ++g_ppu->oamWriteCount;
    blanked = false;
}

typedef struct { uint32_t *px; unsigned pitch; int w, h; } Canvas;
static void put(Canvas *c, int x, int y, uint32_t col) {
    if (x >= 0 && x < c->w && y >= 0 && y < c->h) c->px[(unsigned)y * c->pitch + (unsigned)x] = col;
}
static void fill(Canvas *c, int x, int y, int w, int h, uint32_t col) {
    for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) put(c, x + i, y + j, col);
}
static const uint8_t kArrow[7] = {16, 24, 28, 30, 28, 24, 16};
static const uint8_t kUp[7] = {0, 4, 14, 31, 0, 0, 0}, kDown[7] = {0, 0, 0, 31, 14, 4, 0};
static const uint8_t *glyph(unsigned ch) {
    if (ch == 2) return Sb5MenuGlyph('A');
    if (ch == '>') return kArrow;
    if (ch == '^') return kUp;
    if (ch == 'v') return kDown;
    return Sb5MenuGlyph(ch);
}
static void glyph_at(Canvas *c, int x, int y, unsigned ch, uint32_t col, int s) {
    const uint8_t *g = glyph(ch);
    if (g)
        for (int r = 0; r < 7; ++r) for (int b = 0; b < 5; ++b)
            if (g[r] >> (4 - b) & 1) fill(c, x + b * s, y + r * s, s, s, col);
    if (ch == 2) {
        static const uint8_t tilde[2] = {13, 22};
        for (int r = 0; r < 2; ++r) for (int b = 0; b < 5; ++b)
            if (tilde[r] >> (4 - b) & 1) fill(c, x + b * s, y + (r - 3) * s, s, s, col);
    }
}
static int text(Canvas *c, int x, int y, const char *s, uint32_t col, int scale) {
    for (; *s; ++s, x += 6 * scale) {
        if (*s == ' ') continue;
        int shadow = scale > 3 ? scale / 4 : scale;
        glyph_at(c, x + shadow, y + shadow, (uint8_t)*s, 0xff000000u, scale);
        glyph_at(c, x, y, (uint8_t)*s, col, scale);
    }
    return x;
}
static int text_width(const char *s, int scale) { return (int)strlen(s) * 6 * scale - scale; }

static void draw_rules_page(Canvas *c) {
    const bool blink = (frames >> 4 & 1) == 0;
    if (page == 1) {
        if (blink) {
            text(c, 122, 40, "^", 0xfff8f8f8u, 2);
            text(c, 122, 184, "v", 0xfff8f8f8u, 2);
        }
    } else if (blink) {
        text(c, 122, 184, "v", 0xfff8f8f8u, 2); /* more rules below / above */
    }
}

static void draw_countdown(Canvas *c) {
    for (int y = 0; y < c->h; ++y)
        for (int x = 0; x < c->w; ++x) {
            uint32_t *p = &c->px[(unsigned)y * c->pitch + (unsigned)x];
            *p = 0xff000000u | ((*p - ((*p >> 2) & 0x003f3f3fu)) & 0x00ffffffu);
        }
    static const uint32_t box[SEATS] = {0xffe8e8e8u, 0xff404040u, 0xffd83c3cu, 0xff3c64d8u, 0xff3cb43cu};
    if ((countdown >> 3 & 1) == 0) {
        for (int seat = 0; seat < SEATS; ++seat) {
            int x, y;
            if (!NameTagPlace(g_ram, seat, &x, &y)) continue;
            /* Above the head; under the feet when the head is next to the HUD. */
            const bool below = y < 40;
            const int top = below ? y + 22 : y - 15;
            fill(c, x - 7, top, 15, 13, 0xff000000u);
            fill(c, x - 6, top + 1, 13, 11, box[seat]);
            glyph_at(c, x - 2, top + 3, (unsigned)('1' + seat), seat == 1 ? 0xfff8f8f8u : 0xff000000u, 1);
            if (below) { fill(c, x - 2, top - 1, 5, 1, 0xff000000u); fill(c, x - 1, top - 2, 3, 1, 0xff000000u); fill(c, x, top - 3, 1, 1, 0xff000000u); }
            else { fill(c, x - 2, top + 13, 5, 1, 0xff000000u); fill(c, x - 1, top + 14, 3, 1, 0xff000000u); fill(c, x, top + 15, 1, 1, 0xff000000u); }
        }
    }
    char d[2] = {(char)('0' + (countdown + 59) / 60), 0};
    int scale = 8, x = (256 - text_width(d, scale)) / 2;
    text(c, x, 76, d, 0xfff8f8f8u, scale);
}

void BattleRulesDraw(void) {
    if (!enabled || !g_ppu || !g_ppu->renderBuffer) return;
    const bool rules = on_rules(), cd = countdown && in_battle();
    if (!rules && !cd) return;
    Canvas canvas = {(uint32_t *)g_ppu->renderBuffer, g_ppu->renderPitch / 4u, 256, 224};
    canvas.px += g_ppu->extraLeftRight;
    if (rules) draw_rules_page(&canvas);
    if (cd) draw_countdown(&canvas);
}

/* ---- Lifecycle, save states ------------------------------------------- */

#ifndef BATTLE_RULES_TEST
static bool ptbr(void) { return PtbrEnabled(); }
#endif

void BattleRulesReset(void) {
    page = cur = suppress = perm_valid = 0; countdown = 0; rules_latch = false;
    memset(prev_act, 0, sizeof(prev_act)); memset(prev_dir, 0, sizeof(prev_dir));
    /* The two rules are a session choice: a reset puts them back to off. */
    opt[0] = opt[1] = 0;
}
#define BR_MAGIC 0x53524252u /* "RBRS" */
void BattleRulesSave(struct SaveLoadInfo *sli) {
    uint32_t magic = BR_MAGIC;
    uint8_t state[9 + SEATS * 2 + 3] = {opt[0], opt[1], page, cur, suppress, perm_valid, (uint8_t)countdown, (uint8_t)(countdown >> 8), 0};
    memcpy(state + 9, prev_act, SEATS); memcpy(state + 9 + SEATS, prev_dir, SEATS);
    memcpy(state + 9 + SEATS * 2, timer_bytes, 3);
    uint8_t order[SEATS]; memcpy(order, perm, SEATS);
    sli->func(sli, &magic, 4); sli->func(sli, state, sizeof(state));
    sli->func(sli, order, sizeof(order)); sli->func(sli, &rng, 4); sli->func(sli, &frames, 4);
}
void BattleRulesLoad(struct SaveLoadInfo *sli, unsigned version) {
    (void)version;
    BattleRulesReset();
    if (!RtlStateBytesRemaining(sli)) return;
    uint32_t magic = 0, r = 0, f = 0;
    uint8_t state[9 + SEATS * 2 + 3] = {0}, order[SEATS] = {0};
    sli->func(sli, &magic, 4); sli->func(sli, state, sizeof(state));
    sli->func(sli, order, sizeof(order)); sli->func(sli, &r, 4); sli->func(sli, &f, 4);
    uint16_t cd = (uint16_t)(state[6] | state[7] << 8);
    bool bad = magic != BR_MAGIC || state[0] > 1 || state[1] > 1 || state[2] > 1 || state[3] > 1 || state[4] > 1 || state[5] > 1 || cd > COUNT_FRAMES || !r;
    for (unsigned i = 0; i < SEATS; ++i) bad = bad || order[i] >= SEATS;
    if (bad) { RtlStateStreamFail(sli); return; }
    opt[0] = state[0]; opt[1] = state[1]; page = state[2]; cur = state[3]; suppress = state[4]; perm_valid = state[5]; countdown = cd;
    memcpy(prev_act, state + 9, SEATS); memcpy(prev_dir, state + 9 + SEATS, SEATS);
    memcpy(timer_bytes, state + 9 + SEATS * 2, 3);
    memcpy(perm, order, SEATS); rng = r; frames = f;
}

static void reset_plugin(void) { BattleRulesReset(); enabled = false; }
static void activate(void) { enabled = true; }
SNES_MOD_CONSTRUCTOR(register_battle_rules) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.battle.rules", activate);
}
