/* Boss Rush from the title screen.
 *
 * The cartridge already has a boss rush: password 3879 (handler $C5:695E)
 * clears the campaign script flags 16-115 ($C0:499C), sets flag 4 (boss gates
 * open the next boss, $C2:033A), sets $6274 bit 7 (no saving, $C0:48D6),
 * $6248 = $67 (ZONE1 - A), $1D25/$1D24 = 1 (one player), $6254 = 0, resets the
 * players ($C4:286F) and the password script changes scene with op 0F
 * ($1031 = $67, $1D1E = 4; $C2:0C20).
 *
 * This plugin adds "BOSS RUSH" to the title menu, between BATTLE GAME and
 * OPTIONS, and starts that same rush after the player-count menu:
 *
 *  - Title menu (task 11, script $C7:1831-): the cursor limit for menu 0
 *    ($1037, "5b 02" at $C7:19DA) becomes 3. When the confirmation branch
 *    ($C7:18A8) reads the cursor $1032, entry 2 (Boss Rush) is handed to the
 *    Normal branch (cursor 0: player-count menu, $1036 = 1) with the rush
 *    marked pending, and entry 3 to the Options branch (cursor 2).
 *  - The player-count menu stores $1D25/$1D24 (1P, 2P, or the co-op roster)
 *    and changes scene with "0F 03" ($C7:1918 one player, $C7:193A two). For
 *    the rush that op becomes the password's start: the handler's three
 *    routines run as injected JSL calls from the script interpreter's
 *    dispatch ($C2:0230, a JSL whose return frame the calls reuse), with the
 *    chosen player mask instead of the handler's one-player mask, then
 *    "0F $67". The title script then ends as it does for Options, and the
 *    scene manager ($C5:3AA0: it waits for the scene's script runner, then
 *    for $1D1E bit 2) fades out and starts ZONE1 - A.
 *  - With the Normal character mod enabled, the title script is parked on
 *    that op (one-frame wait + jump back, in $FF padding at $C7:3100) and a
 *    snapshot of the title is taken. The op then runs natively ("0F 03"), so
 *    the picker opens in the file-menu scene exactly as for a Normal file
 *    (file script parked at its first op $C7:0B82). When the picker closes,
 *    the Normal character mod loads the title snapshot back: the rush starts
 *    there with the choices applied, or B returns to the title ("0F 02").
 *    (Starting from the file-menu scene does not work: its runner moves on to
 *    the scene's next scripts, so the manager never sees the request.)
 *
 * The title layout (render only) lives in ptbr_mod.c; the run timer is drawn
 * over the finished frame here; the pause menu offers "return to title". */
#include "boss_rush_mod.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "mod_runtime.h"
#include "menu_font.h"
#include "normal_character_mod.h"
#include "snes/interp_bridge.h"
#include "snes/ppu.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum {
    VM_DISPATCH = 0xc20230,           /* JSL $C2038F: run the op at [$0A] (X = task) */
    VM_NEXT = 0xc20234,               /* after the dispatch: back to the op fetch */
    TITLE_CONFIRM = 0xc718a8,         /* menu confirmation: "5d 36 10 86 00 49 26" */
    TITLE_LIMIT_OP = 0xc719d3,        /* "5d 36 10 86 00 49 05 5b 02 5f 37 10" */
    START_1P = 0xc71918, START_2P = 0xc7193a, /* "0f 03" of the player-count menu */
    FILE_MENU = 0xc70b82,             /* first op of the file-menu script (task 9) */
    PARK = 0xc73100,                  /* $FF padding (Gold Battle uses $C7:3000-$30FF) */
    /* Each: fd 0e 01 00 00 (wait one frame, the menus' own idle wait), 44 rel16 (jump). */
    PARK_FILE = PARK, PARK_1P = PARK + 8, PARK_2P = PARK + 16, PARK_SIZE = 24,
    CLEAR_FLAGS = 0xc0499c, SET_FLAG = 0xc2033a, RESET_PLAYERS = 0xc4286f,
    MODE = 0x1031, SCENE = 0x1d1e, MENU = 0x1036, CURSOR = 0x1032, PAUSED = 0x1d1d,
    BITS = 0x6274, RUSH_BIT = 0x80, FIRST_BOSS = 0x67, FINAL_BOSS = 0x6f, ENDING = 0xaa,
};
enum { PICK_NONE, PICK_SNAPSHOT, PICK_DONE, PICK_CANCELLED, PICK_HOSTING };

static bool enabled, installed, hook_installed;
/* Simulation state (save-state extension). */
static uint8_t pending;      /* Boss Rush chosen on the title (1), its player-count menu seen (2) */
static uint8_t picker;       /* PICK_* */
static uint8_t active;       /* rush running */
static uint8_t saved_bit;    /* $6274 bit 7 before the rush */
static uint32_t frames;
static uint8_t finished;     /* Terrorin beaten: the time is final */
static uint8_t last_mode;    /* $1031 at the end of the previous frame */
/* Only inside one interpreted op (never across a frame boundary). */
static bool injecting;
static unsigned step;
static CpuState saved_cpu;
static uint32_t start_op;
static uint8_t mask, flag;

bool BossRushEnabled(void) { return enabled; }
bool BossRushActive(void) { return enabled && active; }
bool BossRushKeepChoices(void) { return enabled && pending && picker == PICK_DONE; }
uint32_t BossRushFrames(void) { return frames; }
bool BossRushFinished(void) { return enabled && active && finished; }
uint8_t BossRushRoster(void) { return mask; }
/* The Boss Rush player-count menu on the title: the category (1-5) under
 * the cursor, 0 elsewhere. */
unsigned BossRushPlayerMenuCategory(void) {
    if (!enabled || !installed || !pending || g_ram[MODE] != 2 || g_ram[MENU] != 1) return 0;
    unsigned c = g_ram[CURSOR] + 1u;
    return c >= 1 && c <= 5 ? c : 0;
}

static uint32_t task_pc(unsigned task) {
    return g_ram[0x0c87 + task] | g_ram[0x0caf + task] << 8 | (uint32_t)g_ram[0x0cd7 + task] << 16;
}
static uint32_t script_pc(void) { return g_ram[0x0a] | g_ram[0x0b] << 8 | (uint32_t)g_ram[0x0c] << 16; }
static void set_script_pc(uint32_t pc) {
    g_ram[0x0a] = (uint8_t)pc; g_ram[0x0b] = (uint8_t)(pc >> 8); g_ram[0x0c] = (uint8_t)(pc >> 16);
}
static bool parked(uint32_t pc, uint32_t at) { return pc >= at && pc < at + 8; }
/* Script op 0F n ($C2:0C20): request scene n unless one is already pending. */
static void change_scene(uint8_t mode) {
    if (g_ram[SCENE] & 4) return;
    g_ram[MODE] = mode; g_ram[SCENE] = 4;
}

/* ---- In-memory patches ------------------------------------------------- */

static const uint8_t kLimitSig[12] = {0x5d, 0x36, 0x10, 0x86, 0x00, 0x49, 0x05, 0x5b, 0x02, 0x5f, 0x37, 0x10};
static const uint8_t kConfirmSig[7] = {0x5d, 0x36, 0x10, 0x86, 0x00, 0x49, 0x26};
static const uint8_t kFileSig[4] = {0x5d, 0x1a, 0x63, 0x86};
static const uint8_t kStartSig[2] = {0x0f, 0x03};
static uint8_t park_original[PARK_SIZE], limit_original;

static void park_program(uint8_t *at, uint32_t self, uint32_t target) {
    int16_t rel = (int16_t)((target & 0xffff) - ((self + 8) & 0xffff));
    const uint8_t code[8] = {0xfd, 0x0e, 0x01, 0x00, 0x00, 0x44, (uint8_t)rel, (uint8_t)((uint16_t)rel >> 8)};
    memcpy(at, code, sizeof(code));
}
static void install(void) {
    if (installed || !g_rom) return;
    uint8_t *limit = SnesRomPtr(TITLE_LIMIT_OP), *confirm = SnesRomPtr(TITLE_CONFIRM),
            *file = SnesRomPtr(FILE_MENU), *park = SnesRomPtr(PARK), *one = SnesRomPtr(START_1P),
            *two = SnesRomPtr(START_2P);
    if (!limit || !confirm || !file || !park || !one || !two) return;
    if (memcmp(limit, kLimitSig, sizeof(kLimitSig)) || memcmp(confirm, kConfirmSig, sizeof(kConfirmSig)) ||
        memcmp(file, kFileSig, sizeof(kFileSig)) || memcmp(one, kStartSig, 2) || memcmp(two, kStartSig, 2))
        return;
    for (unsigned i = 0; i < sizeof(park_original); ++i) if (park[i] != 0xff) return;
    memcpy(park_original, park, sizeof(park_original));
    park_program(park, PARK_FILE, FILE_MENU);
    park_program(park + 8, PARK_1P, START_1P);
    park_program(park + 16, PARK_2P, START_2P);
    limit_original = limit[8];
    limit[8] = 3;
    installed = true;
}
static void uninstall(void) {
    if (!installed) return;
    memcpy(SnesRomPtr(PARK), park_original, sizeof(park_original));
    SnesRomPtr(TITLE_LIMIT_OP)[8] = limit_original;
    installed = false;
}

/* ---- Starting the rush (inside the script interpreter) ---------------- */

static void push(CpuState *cpu, uint8_t v) { g_ram[cpu->S] = v; cpu->S = (uint16_t)(cpu->S - 1); }
/* Every routine ends in RTL: one return frame for the dispatch JSL. */
static void call(CpuState *cpu, uint32_t routine, uint8_t a) {
    push(cpu, (uint8_t)(VM_DISPATCH >> 16));
    push(cpu, (uint8_t)((VM_DISPATCH - 1) >> 8));
    push(cpu, (uint8_t)(VM_DISPATCH - 1));
    cpu->A = (uint16_t)((cpu->A & 0xff00) | a);
    interp_bridge_pre_opcode_redirect(routine);
}
static void restore_cpu(CpuState *cpu) {
    cpu->A = saved_cpu.A; cpu->X = saved_cpu.X; cpu->Y = saved_cpu.Y;
    cpu->D = saved_cpu.D; cpu->DB = saved_cpu.DB; cpu->S = saved_cpu.S;
    cpu->P = saved_cpu.P; cpu_p_to_mirrors(cpu);
}
/* Skip the title's "0F 03" after doing its work: the script continues with
 * its own ending path (as for Options). */
static void skip_start_op(void) {
    set_script_pc(start_op + 2);
    interp_bridge_pre_opcode_redirect(VM_NEXT);
}
/* The handler of password 3879, step by step, with the chosen roster. */
static void run_step(CpuState *cpu) {
    switch (step++) {
    case 0: call(cpu, CLEAR_FLAGS, 0); return;
    case 1: call(cpu, SET_FLAG, 4); return;
    case 2:
        saved_bit = g_ram[BITS] & RUSH_BIT;
        g_ram[BITS] |= RUSH_BIT;
        g_ram[0x6248] = FIRST_BOSS;
        g_ram[0x1d25] = mask; g_ram[0x1d24] = flag;
        g_ram[0x6254] = 0;
        call(cpu, RESET_PLAYERS, 0);
        return;
    default:
        injecting = false;
        restore_cpu(cpu);
        /* $C4:286F keeps $1D25/$1D24 unless script flags 5/6 say otherwise. */
        g_ram[0x1d25] = mask; g_ram[0x1d24] = flag;
        change_scene(FIRST_BOSS);
        skip_start_op();
        pending = 0; picker = PICK_NONE; active = 1; frames = 0; finished = 0;
        return;
    }
}

static void vm_hook(CpuState *cpu, uint32_t pc) {
    (void)pc;
    if (injecting) { run_step(cpu); return; }
    if (!enabled || !installed) return;
    uint32_t spc = script_pc();
    unsigned task = cpu->X & 0xff;
    if (spc == TITLE_CONFIRM && task == 11 && g_ram[MODE] == 2 && g_ram[MENU] == 0) {
        /* Entries: 0 Normal, 1 Battle, 2 Boss Rush, 3 Options. */
        if (g_ram[CURSOR] == 2) { g_ram[CURSOR] = 0; pending = 1; picker = PICK_NONE; }
        else if (g_ram[CURSOR] == 3) { g_ram[CURSOR] = 2; pending = 0; }
        else pending = 0;
        return;
    }
    if (!pending) return;
    if (task == 9 && g_ram[MODE] == 3 && picker == PICK_HOSTING && (spc == FILE_MENU || parked(spc, PARK_FILE))) {
        /* The picker opens here between frames (BossRushAfterFrame). */
        set_script_pc(PARK_FILE);
        return;
    }
    if (task != 11 || g_ram[MODE] != 2) return;
    uint32_t op = spc == START_1P || parked(spc, PARK_1P) ? START_1P : spc == START_2P || parked(spc, PARK_2P) ? START_2P : 0;
    if (!op) return;
    start_op = op;
    uint32_t park = op == START_1P ? PARK_1P : PARK_2P;
    switch (picker) {
    case PICK_SNAPSHOT: set_script_pc(park); return;          /* until the snapshot is taken */
    case PICK_HOSTING: set_script_pc(op); return;             /* "0F 03": the file-menu scene hosts the picker */
    case PICK_CANCELLED:
        picker = PICK_NONE; pending = 0;
        change_scene(2);
        skip_start_op();
        return;
    case PICK_NONE:
        if (NormalCharacterEnabled()) { picker = PICK_SNAPSHOT; set_script_pc(park); return; }
        break;
    default: break;
    }
    /* Start (picker done or not used). */
    mask = g_ram[0x1d25]; flag = g_ram[0x1d24];
    if (!mask) { mask = 1; flag = 1; }
    set_script_pc(op);
    saved_cpu = *cpu;
    injecting = true; step = 0;
    run_step(cpu);
}

void BossRushPickerDone(bool cancelled) {
    picker = cancelled ? PICK_CANCELLED : PICK_DONE;
    /* The title snapshot is back on screen for a frame or two before the
     * scene changes: keep it dark (the fade-out then ends at once). */
    g_ram[0x0300] = 0x80;
}

void BossRushBeforeFrame(void) {
    if (!enabled) return;
    install();
    if (installed && !hook_installed) hook_installed = interp_bridge_add_pre_opcode_hook(VM_DISPATCH, vm_hook);
}

void BossRushAfterFrame(void) {
    if (!enabled) return;
    unsigned mode = g_ram[MODE];
    /* Back on the title's main menu from the player-count menu (B): Boss
     * Rush is chosen again from there. The confirmation itself waits a few
     * frames before it opens the player-count menu. */
    if (pending == 1 && mode == 2 && g_ram[MENU] == 1) pending = 2;
    if (pending == 2 && mode == 2 && g_ram[MENU] == 0) { pending = 0; picker = PICK_NONE; }
    if (pending && picker == PICK_SNAPSHOT && mode == 2 &&
        (parked(task_pc(11), PARK_1P) || parked(task_pc(11), PARK_2P))) {
        /* The title, waiting on its start op: the picker's return point. */
        picker = PICK_HOSTING;
        if (!NormalCharacterSnapshotBossRush()) picker = PICK_DONE; /* no picker: start directly */
    }
    if (pending && picker == PICK_HOSTING && mode == 3 && parked(task_pc(9), PARK_FILE)) {
        /* Without the snapshot (a state loaded in between) the Normal file
         * menu simply goes on. */
        if (!NormalCharacterOpenBossRush()) { pending = 0; picker = PICK_NONE; }
    }
    uint8_t previous = last_mode;
    last_mode = (uint8_t)mode;
    if (!active) return;
    /* The rush ends where the cartridge's ends: beating ZONE5 - J
     * (Terrorin, $6F) runs its "0F AA" (the ending). No other way out of
     * $6F leads there (Game Over and quitting go elsewhere). The ending's
     * cutscenes reuse the boss stage numbers, so the time stops for good. */
    if (!finished && previous == FINAL_BOSS && mode == ENDING) finished = 1;
    if (mode <= 2) {
        /* The title is back: the rush is over. Saving works again. */
        g_ram[BITS] = (uint8_t)((g_ram[BITS] & ~RUSH_BIT) | saved_bit);
        active = 0;
        return;
    }
    /* Counted while a stage is on screen and not paused: the black loading
     * between bosses (INIDISP shadow $0300 forced blank or brightness 0) is
     * not part of the time. */
    bool stage = mode >= 0x0d && mode < 0x74 && !(g_ram[0x1d1a] & 1);
    bool shown = !(g_ram[0x0300] & 0x80) && (g_ram[0x0300] & 0x0f);
    if (stage && shown && !g_ram[PAUSED] && !finished) ++frames;
}

void BossRushQuitToTitle(void) {
    if (!active) return;
    /* Leaving voluntarily never records a cleared stage ($C5:3D19). */
    g_ram[0x1d1a] &= (uint8_t)~0x40u;
    g_ram[PAUSED] = 0;
    g_ram[MODE] = 2;
    g_ram[SCENE] |= 4;
}

bool BossRushTitleMenu(void) {
    return enabled && installed && g_ram[MODE] == 2 && g_ram[MENU] == 0;
}

/* ---- Timer (presentation only) --------------------------------------- */

static void put(uint32_t *px, unsigned pitch, int x, int y, uint32_t c) {
    if (x >= 0 && x < 256 && y >= 0 && y < 224) px[(unsigned)y * pitch + (unsigned)x] = c;
}
static const uint8_t kColon[7] = {0, 4, 4, 0, 4, 4, 0}, kDot[7] = {0, 0, 0, 0, 0, 12, 12},
    kQuestion[7] = {14, 17, 1, 2, 4, 0, 4}, kHash[7] = {10, 31, 10, 10, 31, 10, 0}, kPlus[7] = {0, 4, 4, 31, 4, 4, 0},
    kSlash[7] = {1, 1, 2, 4, 8, 16, 16}, kBang[7] = {4, 4, 4, 4, 4, 0, 4}, kLess[7] = {2, 4, 8, 16, 8, 4, 2},
    kGreater[7] = {8, 4, 2, 1, 2, 4, 8}, kUnder[7] = {0, 0, 0, 0, 0, 0, 31};
static const uint8_t *glyph(char c) {
    switch (c) {
    case ':': return kColon;
    case '.': return kDot;
    case '?': return kQuestion;
    case '#': return kHash;
    case '+': return kPlus;
    case '/': return kSlash;
    case '!': return kBang;
    case '<': return kLess;
    case '>': return kGreater;
    case '_': return kUnder;
    default: break;
    }
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    if ((unsigned char)c >= 0x80) return kQuestion;
    return Sb5MenuGlyph((unsigned char)c);
}
static void draw_text_rgb(uint32_t *px, unsigned pitch, int x, int y, const char *s, uint32_t ink) {
    /* One-pixel black outline, white ink. */
    int len = (int)strlen(s);
    for (int pass = 0; pass < 2; ++pass) {
        int cx = x;
        for (int i = 0; i < len; ++i) {
            const uint8_t *g = glyph(s[i]);
            int w = (s[i] == ':' || s[i] == '.') ? 4 : 6;
            if (g)
                for (int r = 0; r < 7; ++r)
                    for (int b = 0; b < 5; ++b) {
                        if (!(g[r] >> (4 - b) & 1)) continue;
                        if (pass == 0) {
                            for (int dy = -1; dy <= 1; ++dy)
                                for (int dx = -1; dx <= 1; ++dx) put(px, pitch, cx + b + dx, y + r + dy, 0xff000000u);
                        } else {
                            put(px, pitch, cx + b, y + r, ink);
                        }
                    }
            cx += w;
        }
    }
}
static void draw_text(uint32_t *px, unsigned pitch, int x, int y, const char *s, unsigned brightness) {
    /* Follows the screen's fades (INIDISP brightness). */
    uint32_t v = 0xf8u * brightness / 15u;
    draw_text_rgb(px, pitch, x, y, s, 0xff000000u | v << 16 | v << 8 | v);
}
static int text_width(const char *s) {
    int w = 0;
    for (; *s; ++s) w += (*s == ':' || *s == '.') ? 4 : 6;
    return w - 1;
}
/* Shared with the ranking screens (boss_rush_results.c). */
static bool frame_buffer(uint32_t **px, unsigned *pitch) {
    if (!g_ppu || !g_ppu->renderBuffer) return false;
    *px = (uint32_t *)g_ppu->renderBuffer + g_ppu->extraLeftRight;
    *pitch = g_ppu->renderPitch / 4u;
    return true;
}
int BossRushTextWidth(const char *s) { return text_width(s); }
void BossRushText(int x, int y, const char *s, uint32_t rgb) {
    uint32_t *px; unsigned pitch;
    if (frame_buffer(&px, &pitch)) draw_text_rgb(px, pitch, x, y, s, 0xff000000u | rgb);
}
void BossRushShade(int x0, int y0, int x1, int y1) {
    uint32_t *px; unsigned pitch;
    if (!frame_buffer(&px, &pitch)) return;
    for (int y = y0 < 0 ? 0 : y0; y < y1 && y < 224; ++y)
        for (int x = x0 < 0 ? 0 : x0; x < x1 && x < 256; ++x) {
            uint32_t c = px[(unsigned)y * pitch + (unsigned)x];
            px[(unsigned)y * pitch + (unsigned)x] = 0xff000000u | ((c >> 2) & 0x3f3f3f);
        }
}
void BossRushFormat(uint32_t f, char *out, size_t n) {
    uint32_t cs = f * 100u / 60u;
    unsigned c = cs % 100, s = (cs / 100) % 60, m = cs / 6000;
    snprintf(out, n, "%02u:%02u.%02u", m > 99 ? 99 : m, s, c);
}
void BossRushDraw(void) {
    if (!BossRushActive() || !g_ppu || !g_ppu->renderBuffer) return;
    unsigned mode = g_ram[MODE];
    if (mode < 0x0d || mode >= 0x74 || (g_ram[0x1d1a] & 1) || (g_ppu->inidisp & 0x80) || !(g_ppu->inidisp & 0x0f)) return;
    uint32_t *px = (uint32_t *)g_ppu->renderBuffer + g_ppu->extraLeftRight;
    unsigned pitch = g_ppu->renderPitch / 4u;
    char text[16];
    BossRushFormat(frames, text, sizeof(text));
    draw_text(px, pitch, 252 - text_width(text), 21, text, g_ppu->inidisp & 0x0f);
}

/* ---- Lifecycle, save states ------------------------------------------- */

void BossRushReset(void) {
    injecting = false; step = 0; start_op = 0;
    pending = picker = active = finished = 0; frames = 0; saved_bit = 0; last_mode = 0;
}
#define BR_MAGIC 0x48535242u /* "BRSH" */
void BossRushSave(struct SaveLoadInfo *sli) {
    uint32_t magic = BR_MAGIC;
    uint8_t state[8] = {pending, picker, active, saved_bit, finished, mask, flag, last_mode};
    sli->func(sli, &magic, 4); sli->func(sli, state, sizeof(state)); sli->func(sli, &frames, 4);
}
void BossRushLoad(struct SaveLoadInfo *sli, unsigned version) {
    (void)version;
    BossRushReset();
    if (!RtlStateBytesRemaining(sli)) return;
    uint32_t magic = 0, f = 0;
    uint8_t state[8] = {0};
    sli->func(sli, &magic, 4); sli->func(sli, state, sizeof(state)); sli->func(sli, &f, 4);
    if (magic != BR_MAGIC || state[0] > 2 || state[1] > PICK_HOSTING || state[2] > 1 || (state[3] & ~RUSH_BIT) || state[4] > 1 ||
        state[5] > 0x1f || state[6] > 3) {
        RtlStateStreamFail(sli);
        return;
    }
    pending = state[0]; picker = state[1]; active = state[2]; saved_bit = state[3];
    mask = state[5]; flag = state[6]; frames = f; finished = state[4]; last_mode = state[7];
}

static void reset_plugin(void) { BossRushReset(); uninstall(); enabled = false; }
static void activate(void) { enabled = true; }
SNES_MOD_CONSTRUCTOR(register_boss_rush) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.boss.rush", activate);
}
