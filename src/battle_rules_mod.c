/* Battle rules, page 2: "round countdown", "shuffle spawns" and "disable skills".
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
 * from a deterministic generator that is part of the save state.
 *
 * Domino: a rolling bomb blocked by a resting bomb ($C5:2182 in the bomb
 * update, X = bomb, A = blocked cell, direction = $0E90 & 7) kicks that bomb
 * with the native ground kick ($C5:2293, A = direction, $8A = origin cell)
 * through a trampoline in $FF padding at $C5:7540, then resumes $C5:2182 so the
 * first bomb stops natively. Same path as the Yellow Louie's chain kick.
 *
 * Throw range: the Power Glove throw ($C5:2436, carried bomb $0E90 bit $40)
 * makes the bomb airborne (bit $20) with $0F10 = $2C, an index into the height
 * arc at $80:8CDA. Each airborne frame ($C5:1FED) moves it 4 px forward
 * ($80:8C9F) and reads the arc at $C5:20E7 (then INC $0F10); height 0 (index
 * $37) lands it ($C5:2243). Indices $2C-$37 are 12 frames = 48 px = 3 tiles.
 * For N tiles the hook at $C5:20E7 walks the same arc in 4N frames instead,
 * writing $0F10 before the native read; a bounce off a blocked cell ($C5:2160,
 * $0F10 = $38) is left native. */
#include "battle_rules_mod.h"
#include "common_rtl.h"
#include "mod_runtime.h"
#include "menu_font.h"
#include "ptbr_mod.h"
#include "louie_mod.h"
#include "snes/interp_bridge.h"
#include "common_cpu_infra.h"
#include "snes/ppu.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum {
    PAD_HOOK = 0xc00af9, ROUND_HOOK = 0xc50007, ROUND_HOOK2 = 0xc20bc1, ROUND_HOOK3 = 0xc50912, INIT_HOOK = 0xc42a3c, ACTOR_HOOK = 0xc4221d, ACTOR_RET = 0xc4224e, SPAWN_HOOK = 0xc216c7, ROLL_HOOK = 0xc52182, THROW_HOOK = 0xc5245b, ARC_HOOK = 0xc520e7,
    RULES_PC = 0xc69345,
    MODE = 0x1031, ROW = 0x1032, FLAGS = 0x1d1a, PAUSE = 0x1d1d, READY = 0x0aee, TIMER = 0x0e1c,
    PAD_A = 0x80, DIR_RIGHT = 0x01, DIR_LEFT = 0x02, DIR_DOWN = 0x04, DIR_UP = 0x08,
    COUNT_FRAMES = 180, SEATS = 5, LAST_ROW = 5, RULES = 5, RANGE = 4, RANGE_DEFAULT = 3, RANGE_MAX = 5, BOMBS = 20,
    ARC_FIRST = 0x2c, ARC_LAST = 0x37, FIRST_ROW = 9,
    MAP = 0x3000, BLANK = 0x100, BLANK_ROW0 = 5, BLANK_ROWS = 18, BLANK_COL0 = 6, BLANK_COLS = 24,
};

static bool enabled, hooks_installed;
/* Simulation state (saved with the machine). */
static uint8_t opt[RULES] = {1, 0, 0, 0, RANGE_DEFAULT}, throw_step[BOMBS], page, cur, suppress, perm[SEATS], perm_valid;
static uint16_t countdown;
static uint8_t timer_bytes[3];
static uint32_t rng = 0x9e3779b9u, frames;
static uint8_t prev_act[SEATS], prev_dir[SEATS];

bool BattleRulesEnabled(void) { return enabled; }
bool BattleRulesCountdownActive(void) { return enabled && countdown != 0; }
/* Third rule, "disable abilities": the B + direction moves of the extra characters are not used. */
bool BattleRulesAbilitiesOff(void) { return enabled && opt[2]; }
/* Fourth rule, "domino": a kicked bomb that hits a resting bomb stops and passes its motion on. */
bool BattleRulesDomino(void) { return enabled && opt[3]; }
/* Fifth rule, Power Glove throw range in tiles (1-5, native 3). */
unsigned BattleRulesThrowRange(void) { return enabled ? opt[RANGE] : RANGE_DEFAULT; }

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

/* Menu sounds like the cartridge's rules screen: native group-2 command (move = 1) through $C0:0CED from a
 * small trampoline in $FF padding at $C5:7520 (the variants mod keeps $C5:7500), entered from the pad
 * reader's tail; the id byte is written right before the redirect so the sound is part of the emulated
 * frame (save states and netplay replay it). */
enum { SFX_CODE = 0xc57520, SFX_ID = SFX_CODE + 7, SFX_MOVE = 1 };
static const uint8_t kSfxCode[24] = {0x08, 0xc2, 0x30, 0x48, 0xda, 0x5a, 0xa9, 0x00, 0x02, 0xe2, 0x30,
                                     0x22, 0xed, 0x0c, 0xc0, 0xc2, 0x30, 0x7a, 0xfa, 0x68, 0x28, 0xfa, 0x7a, 0x6b};
static uint8_t sfx_original[sizeof(kSfxCode)];
static bool sfx_installed;
static void install_sfx(void) {
    if (sfx_installed) return;
    uint8_t *code = SnesRomPtr(SFX_CODE);
    const uint8_t *tail = SnesRomPtr(PAD_HOOK);
    if (!code || !tail || tail[0] != 0xfa || tail[1] != 0x7a || tail[2] != 0x6b) return; /* PLX PLY RTL */
    for (unsigned i = 0; i < sizeof(kSfxCode); ++i) if (code[i] != 0xff) return;
    memcpy(sfx_original, code, sizeof(sfx_original));
    memcpy(code, kSfxCode, sizeof(kSfxCode));
    sfx_installed = true;
}
static void remove_sfx(void) {
    if (!sfx_installed) return;
    memcpy(SnesRomPtr(SFX_CODE), sfx_original, sizeof(sfx_original));
    sfx_installed = false;
}
static void play_sfx(uint8_t id) {
    if (!sfx_installed || !id) return;
    SnesRomPtr(SFX_ID)[0] = id;
    interp_bridge_pre_opcode_redirect(SFX_CODE);
}

static uint8_t pad_logic(CpuState *cpu) {
    if (!enabled) return 0;
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
    if (countdown) { clear_pads(held, dirs, fresh, fresh_dirs); return 0; }
    if (!on_rules()) { page = 0; suppress = 0; return 0; }
    if (suppress) {
        if (any_held) { clear_pads(held, dirs, fresh, fresh_dirs); return 0; }
        suppress = 0;
        return 0;
    }
    const unsigned row = g_ram[ROW];
    if (!page) {
        for (unsigned s = 0; s < SEATS; ++s) {
            bool down = (new_dir[s] & DIR_DOWN) && row == LAST_ROW, up = (new_dir[s] & DIR_UP) && row == 0;
            if (!down && !up) continue;
            page = 1; cur = down ? 0 : RULES - 1; suppress = 1;
            clear_pads(held, dirs, fresh, fresh_dirs);
            return SFX_MOVE;
        }
        return 0;
    }
    /* Page 2: directions and A belong to the page, B and Start still reach the cartridge. */
    bool leave = false;
    uint8_t inject = 0, sound = 0;
    unsigned who = 0;
    for (unsigned s = 0; s < SEATS && !leave; ++s) {
        uint8_t d = new_dir[s];
        if (d & DIR_DOWN) {
            if (cur < RULES - 1) { ++cur; sound = SFX_MOVE; }
            else { leave = true; who = s; if (row == LAST_ROW) inject = DIR_DOWN; }
            break;
        }
        if (d & DIR_UP) {
            if (cur > 0) { --cur; sound = SFX_MOVE; }
            else { leave = true; who = s; if (row == 0) inject = DIR_UP; }
            break;
        }
        if ((d & (DIR_LEFT | DIR_RIGHT)) || (new_act[s] & PAD_A)) {
            if (cur == RANGE) /* Right/A: +1, Left: -1, wrapping 1-5 */
                opt[RANGE] = (d & DIR_LEFT) ? (opt[RANGE] > 1 ? opt[RANGE] - 1 : RANGE_MAX) : (opt[RANGE] < RANGE_MAX ? opt[RANGE] + 1 : 1);
            else opt[cur] ^= 1;
            sound = SFX_MOVE; break;
        }
    }
    for (unsigned s = 0; s < SEATS; ++s) {
        g_ram[(dirs + s) & 0xffff] = 0; g_ram[(fresh_dirs + s) & 0xffff] = 0; g_ram[(fresh_dirs + 5 + s) & 0xffff] = 0;
        g_ram[(held + s) & 0xffff] &= (uint8_t)~PAD_A; g_ram[(fresh + s) & 0xffff] &= (uint8_t)~PAD_A;
    }
    if (leave) {
        page = 0; suppress = 1;
        sound = inject ? 0 : SFX_MOVE; /* with an injected key the cartridge plays its own */
        if (inject) { g_ram[(dirs + who) & 0xffff] = inject; g_ram[(fresh_dirs + who) & 0xffff] = inject; g_ram[(fresh_dirs + 5 + who) & 0xffff] = inject; }
    }
    return sound;
}

static void pad_hook(CpuState *cpu, uint32_t pc) {
    (void)pc;
    play_sfx(pad_logic(cpu)); /* last: a redirect ends this PC's hooks for the frame */
}

/* ---- Round start, freeze, spawn shuffle ------------------------------- */

/* Battle stage init ($C4:2A3C) runs once per round. Hurry Up also drops and
 * re-raises READY, so only a round that went through the init may count down. */
static bool round_armed;
static void init_hook(CpuState *cpu, uint32_t pc) { (void)cpu; (void)pc; round_armed = true; }

static void round_hook(CpuState *cpu, uint32_t pc) {
    (void)cpu; (void)pc;
    if (g_ram[READY] == 1) return; /* these stores also run every frame the round is live */
    perm_valid = 0;
    const bool armed = round_armed;
    round_armed = false;
    if (!armed || !enabled || opt[0] || !in_battle()) return; /* opt[0] = "start immediately" */
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
    /* Every seat (humans and COM) gets another seat's spawn: reshuffle until nobody keeps its own,
     * with a rotation as the bounded fallback. */
    for (unsigned attempt = 0;; ++attempt) {
        memcpy(shuffled, seats, sizeof(shuffled));
        if (attempt >= 32 || n < 2) {
            for (unsigned i = 0; i < n; ++i) shuffled[i] = seats[(i + 1) % n];
            break;
        }
        for (unsigned i = n; i > 1; --i) {
            unsigned j = next_random() % i;
            uint8_t t = shuffled[i - 1]; shuffled[i - 1] = shuffled[j]; shuffled[j] = t;
        }
        bool fixed = false;
        for (unsigned i = 0; i < n; ++i) fixed = fixed || shuffled[i] == seats[i];
        if (!fixed) break;
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

/* ---- Domino ------------------------------------------------------------ */

enum { KICK_CODE = 0xc57540, KICK_CELL = KICK_CODE + 7, KICK_DIR = KICK_CODE + 11 };
/* PHX PHY PHA; LDA $8A; PHA; LDA #cell; STA $8A; LDA #dir; JSL $C5:2293; PLA; STA $8A; PLA PLY PLX; JML $C5:2182 */
static const uint8_t kKickCode[26] = {0xda, 0x5a, 0x48, 0xa5, 0x8a, 0x48, 0xa9, 0x00, 0x85, 0x8a, 0xa9, 0x00, 0x22,
                                      0x93, 0x22, 0xc5, 0x68, 0x85, 0x8a, 0x68, 0x7a, 0xfa, 0x5c, 0x82, 0x21, 0xc5};
static uint8_t kick_original[sizeof(kKickCode)];
static bool kick_installed, kick_resuming;
static void install_kick(void) {
    if (kick_installed) return;
    uint8_t *code = SnesRomPtr(KICK_CODE);
    if (!code) return;
    for (unsigned i = 0; i < sizeof(kKickCode); ++i) if (code[i] != 0xff) return;
    memcpy(kick_original, code, sizeof(kick_original));
    memcpy(code, kKickCode, sizeof(kKickCode));
    kick_installed = true;
}
static void remove_kick(void) {
    if (!kick_installed) return;
    memcpy(SnesRomPtr(KICK_CODE), kick_original, sizeof(kick_original));
    kick_installed = false;
}
/* A resting bomb on the ground (the test of louie_mod.c's bomb_at). */
static int resting_bomb(unsigned cell) {
    for (unsigned i = 0; i < 20; ++i)
        if (g_ram[0x0e40 + i] && g_ram[0x0e7c + i] == cell && !(g_ram[0x0e90 + i] & 0xf8) && !(g_ram[0x0ecc + i] & 0x20)) return (int)i;
    return -1;
}
static void roll_hook(CpuState *cpu, uint32_t pc) {
    (void)pc;
    if (kick_resuming) { kick_resuming = false; return; } /* the trampoline's JML back here */
    if (!enabled || !opt[3] || !kick_installed || !in_battle() || g_ram[PAUSE]) return;
    unsigned bomb = cpu->X & 0xff, cell = cpu->A & 0xff, dir = g_ram[0x0e90 + bomb] & 7;
    static const int dx[5] = {0, 1, 0, -1, 0}, dy[5] = {0, 0, 1, 0, -1};
    if (bomb >= 20 || dir < 1 || dir > 4) return;
    if (LouieOwnsBomb(bomb)) return; /* the Yellow Louie's own chain kick handles it */
    int hit = resting_bomb(cell);
    if (hit < 0 || hit == (int)bomb || LouieOwnsBomb((unsigned)hit)) return;
    SnesRomPtr(KICK_CELL)[0] = (uint8_t)(cell - dx[dir] - 16 * dy[dir]);
    SnesRomPtr(KICK_DIR)[0] = (uint8_t)dir;
    kick_resuming = true;
    interp_bridge_pre_opcode_redirect(KICK_CODE);
}

/* ---- Throw range -------------------------------------------------------- */

static void throw_hook(CpuState *cpu, uint32_t pc) {
    (void)pc;
    unsigned bomb = cpu->X & 0xff;
    if (bomb >= BOMBS) return;
    throw_step[bomb] = 0;
    if (enabled && in_battle() && opt[RANGE] != RANGE_DEFAULT) throw_step[bomb] = 1; /* step + 1 */
}
/* Arc index for step k of a throw lasting 4 * range frames: first $2C, last (height 0) $37. */
static uint8_t arc_index(unsigned step, unsigned range) {
    unsigned frames = 4 * range;
    if (step >= frames - 1) return ARC_LAST;
    return (uint8_t)(ARC_FIRST + (step * (ARC_LAST - ARC_FIRST)) / (frames - 1));
}
static void arc_hook(CpuState *cpu, uint32_t pc) {
    (void)pc;
    unsigned bomb = cpu->X & 0xff;
    if (bomb >= BOMBS || !throw_step[bomb]) return;
    const uint8_t now = g_ram[0x0f10 + bomb];
    if (!enabled || !in_battle() || now < ARC_FIRST || now > ARC_LAST || opt[RANGE] == RANGE_DEFAULT) { throw_step[bomb] = 0; return; }
    const uint8_t idx = arc_index(throw_step[bomb] - 1u, opt[RANGE]);
    g_ram[0x0f10 + bomb] = idx;
    throw_step[bomb] = idx == ARC_LAST ? 0 : (uint8_t)(throw_step[bomb] + 1);
}

void BattleRulesBeforeFrame(void) {
    if (!enabled) return;
    if (!hooks_installed) {
        hooks_installed = interp_bridge_add_pre_opcode_hook(PAD_HOOK, pad_hook);
        interp_bridge_add_pre_opcode_hook(ROUND_HOOK, round_hook);
        interp_bridge_add_pre_opcode_hook(ROUND_HOOK2, round_hook);
        interp_bridge_add_pre_opcode_hook(ROUND_HOOK3, round_hook);
        interp_bridge_add_pre_opcode_hook(INIT_HOOK, init_hook);
        interp_bridge_add_pre_opcode_hook(ACTOR_HOOK, actor_hook);
        interp_bridge_add_pre_opcode_hook(SPAWN_HOOK, spawn_hook);
        interp_bridge_add_pre_opcode_hook(ROLL_HOOK, roll_hook);
        interp_bridge_add_pre_opcode_hook(THROW_HOOK, throw_hook);
        interp_bridge_add_pre_opcode_hook(ARC_HOOK, arc_hook);
    }
    install_sfx();
    install_kick();
    ++frames;
    latch_rules();
    if (g_ram[MODE] < 0x74) { countdown = 0; memset(throw_step, 0, sizeof(throw_step)); if (g_ram[MODE] != 4) perm_valid = 0; }
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
    /* The native hand, moved to the row of the page's cursor (rows 9, 12, ... 21). */
    g_ppu->oam[0] = (uint16_t)((saved_oam0 & 0xff) | (unsigned)(8 * FIRST_ROW - 1 + 24 * cur) << 8);
    ++g_ppu->vramWriteCount; ++g_ppu->oamWriteCount;
    blanked = true;
}

bool BattleRulesPageOpen(void) { return enabled && page == 1 && on_rules(); }

void BattleRulesApplyText(struct Ppu *ppu) {
    if (!blanked) return;
    const bool pt = ptbr();
    /* After the translation pass has read its guard tiles (the banner needs the native row 6). */
    PtbrBlankMap(ppu, BLANK_ROW0, BLANK_COL0, BLANK_ROWS, BLANK_COLS, BLANK);
    static const char *const names_en[RULES] = {"Instant Start", "Shuffle Spawn", "Disable Skills", "Domino", "Throw Range"},
                      *const names_pt[RULES] = {"In\303\255cio imediato", "Embaralhar", "Sem habilidades", "Domin\303\263", "Alcance"};
    for (unsigned i = 0; i < RULES; ++i) {
        /* BG1 tiles 584-763: 36 per row (label 14x2, value 4x2). Tile 768 is the BG1 map ($3000). */
        const unsigned row = FIRST_ROW + 3 * i, tile = 584 + i * 36;
        PtbrDrawBodyText(ppu, row, 8, 14, tile, attr_label, (pt ? names_pt : names_en)[i]);
        if (i == RANGE) {
            char value[2] = {(char)('0' + opt[RANGE]), 0};
            PtbrDrawBodyText(ppu, row, 22, 4, tile + 28, opt[RANGE] == RANGE_DEFAULT ? attr_blue : attr_red, value);
            continue;
        }
        PtbrDrawBodyText(ppu, row, 22, 4, tile + 28, opt[i] ? attr_red : attr_blue,
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

static int s16(int at) { return (int16_t)(uint16_t)(g_ram[at] | g_ram[at + 1] << 8); }
/* Head position of a living bomber, humans and COM alike (the layout of name_tags.c without its human test). */
static bool seat_place(unsigned seat, int *x, int *y) {
    if (g_ram[0x17fa + seat] & 0x80 || g_ram[0x15a2 + seat] & 0x80) return false;
    int sx = (g_ram[0x182a + seat] | g_ram[0x1842 + seat] << 8) - s16(0x1d3b);
    int sy = (g_ram[0x1872 + seat] | g_ram[0x188a + seat] << 8) - s16(0x1d3d) - 17;
    if (g_ram[0x1782 + seat] & 0x80) sy -= 8;
    if (sx < 0 || sx > 255 || sy < 8 || sy > 223) return false;
    *x = sx; *y = sy;
    return true;
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
            if (!seat_place((unsigned)seat, &x, &y)) continue;
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
    remove_sfx(); remove_kick(); kick_resuming = false;
    page = cur = suppress = perm_valid = 0; countdown = 0; round_armed = false; rules_latch = false;
    memset(prev_act, 0, sizeof(prev_act)); memset(prev_dir, 0, sizeof(prev_dir));
    /* Session choices: a reset puts them back to their defaults (start immediately yes, shuffle no). */
    opt[0] = 1; opt[1] = 0; opt[2] = 0; opt[3] = 0; opt[RANGE] = RANGE_DEFAULT;
    memset(throw_step, 0, sizeof(throw_step));
}
#define BR_MAGIC 0x53524252u /* "RBRS" */
void BattleRulesSave(struct SaveLoadInfo *sli) {
    uint32_t magic = BR_MAGIC;
    uint8_t state[9 + SEATS * 2 + 3] = {opt[0], opt[1], page, cur, suppress, perm_valid, (uint8_t)countdown, (uint8_t)(countdown >> 8), opt[2]};
    memcpy(state + 9, prev_act, SEATS); memcpy(state + 9 + SEATS, prev_dir, SEATS);
    memcpy(state + 9 + SEATS * 2, timer_bytes, 3);
    uint8_t order[SEATS]; memcpy(order, perm, SEATS);
    sli->func(sli, &magic, 4); sli->func(sli, state, sizeof(state));
    sli->func(sli, order, sizeof(order)); sli->func(sli, &rng, 4); sli->func(sli, &frames, 4);
    uint8_t extra[2 + BOMBS] = {opt[3], opt[RANGE]}; /* appended later (domino, throw range); older states end before it */
    memcpy(extra + 2, throw_step, BOMBS);
    sli->func(sli, extra, sizeof(extra));
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
    bool bad = magic != BR_MAGIC || state[0] > 1 || state[1] > 1 || state[2] > 1 || state[3] >= RULES || state[8] > 1 || state[4] > 1 || state[5] > 1 || cd > COUNT_FRAMES || !r;
    for (unsigned i = 0; i < SEATS; ++i) bad = bad || order[i] >= SEATS;
    if (bad) { RtlStateStreamFail(sli); return; }
    opt[0] = state[0]; opt[1] = state[1]; opt[2] = state[8]; page = state[2]; cur = state[3]; suppress = state[4]; perm_valid = state[5]; countdown = cd;
    memcpy(prev_act, state + 9, SEATS); memcpy(prev_dir, state + 9 + SEATS, SEATS);
    memcpy(timer_bytes, state + 9 + SEATS * 2, 3);
    memcpy(perm, order, SEATS); rng = r; frames = f;
    if (!RtlStateBytesRemaining(sli)) return;
    uint8_t extra[2 + BOMBS] = {0};
    sli->func(sli, extra, sizeof(extra));
    bool bad2 = extra[0] > 1 || extra[1] < 1 || extra[1] > RANGE_MAX;
    for (unsigned i = 0; i < BOMBS; ++i) bad2 = bad2 || extra[2 + i] > 4 * RANGE_MAX;
    if (bad2) { RtlStateStreamFail(sli); return; }
    opt[3] = extra[0]; opt[RANGE] = extra[1]; memcpy(throw_step, extra + 2, BOMBS);
}

static void reset_plugin(void) { BattleRulesReset(); enabled = false; }
static void activate(void) { enabled = true; }
SNES_MOD_CONSTRUCTOR(register_battle_rules) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.battle.rules", activate);
}
