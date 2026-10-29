/* Battle stage variants: a picker between the arena carousel and the match.
 *
 * The cartridge has four layouts per Battle arena (mode $74 + 4*(arena-1) +
 * variant) and an "all items" rule, normally reached only through passwords
 * 4622 / 0413 / 0926 / 0110. The match-start script ($C6:A4A9) picks the
 * variant from the session password bits $7E6274 ($01 -> 1, $02 -> 2,
 * $04 -> 3), else (maybe at random) from the saved bits $7E6273, else 0; the
 * item rule keeps script flag 3 ($7E2900 bit 3, set by password 0110 through
 * $C2:033A) only while $6274 bit 5 is set.
 *
 * With this plugin, an A pressed on the interactive arena selector (task 9 in
 * $C6:A1AC-$A33D) opens a list of five choices drawn over the frame: original,
 * the three password layouts and the item rule, each with a minimap of the
 * arena read from the cartridge (map stream $D0:A000 + (map-1)*3 through the
 * $C0:1CB0 decompressor, solid metatiles from attribute table
 * $D0:8000 + (tileset-1)*2, spawns from the stage's actor script "22 P x y").
 * A or Start (the carousel starts the match with either; Start = $01 in
 * the action arrays) opens it. B closes it (the carousel stays), A or Start
 * writes the bits for the choice, masks
 * the saved bits until the stage has started, and hands the A back to the
 * cartridge on the next pad read, so the native transition runs unchanged.
 *
 * Input goes through the pad reader's last instruction ($C0:0AF9, arrays
 * D+$2C/$31/$36/$3B per seat), so every peer of a netplay room sees the same
 * choice; the menu state is a save-state extension. Drawing is presentation
 * only: it writes the finished frame (renderBuffer) after the PPU composed
 * it, never VRAM, OAM, CGRAM or WRAM. */
#include "battle_variants_mod.h"
#include "common_rtl.h"
#include "mod_runtime.h"
#include "menu_font.h"
#include "snes/interp_bridge.h"
#include "common_cpu_infra.h"
#include "snes/ppu.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum {
    PAD_HOOK = 0xc00af9,
    SELECTOR_FIRST = 0xc6a1ac, SELECTOR_END = 0xc6a33d,
    MODE = 0x1031, ARENA = 0x625a, BITS = 0x6274, SAVED_BITS = 0x6273, FLAGS = 0x2900,
    VARIANT_MASK = 0x27, ITEM_BIT = 0x20, ITEM_FLAG = 0x08,
    PAD_A = 0x80, PAD_B = 0x08, PAD_START = 0x01, PAD_CONFIRM = PAD_A | PAD_START, DIR_UP = 0x08, DIR_DOWN = 0x04,
    CHOICES = 5, ARENAS = 13,
};

static bool enabled, hook_installed;
/* Simulation state (saved with the machine). */
static uint8_t open_menu, cursor, inject_seat /* 0 = none, else seat+1 */, restore_pending;
static uint8_t saved_bits, saved_saved_bits;

bool BattleVariantsEnabled(void) { return enabled; }
bool BattleVariantsMenuOpen(void) { return open_menu != 0; }

static uint8_t rb(uint32_t pc) { return g_rom[((pc >> 16) & 0x3f) << 16 | (pc & 0xffff)]; }
static uint32_t rw(uint32_t pc) { return rb(pc) | (uint32_t)rb(pc + 1) << 8; }
static uint32_t rl(uint32_t pc) { return rw(pc) | (uint32_t)rb(pc + 2) << 16; }
static uint32_t task_pc(void) { return g_ram[0x0c90] | g_ram[0x0cb8] << 8 | (uint32_t)g_ram[0x0ce0] << 16; }
static bool in_selector(void) {
    uint32_t pc = task_pc();
    return g_ram[MODE] == 4 && pc >= SELECTOR_FIRST && pc < SELECTOR_END;
}

/* ---- Arena data from the cartridge ------------------------------------ */

typedef struct {
    bool ok;
    uint8_t solid[256];       /* 1 = hard block / outside, per 16-column cell */
    uint8_t spawn_x[5], spawn_y[5], spawns;
} Layout;

static Layout layouts[ARENAS][4];
static bool layout_done[ARENAS][4];

/* $C0:1CB0/$C0:1CCC: control byte = mode (bits 5-7) + count-1 (bits 0-4);
 * literal, previous+1, previous, then copies 2/4/16/32/64 back. */
static void decompress(uint32_t src, uint8_t *out, unsigned size) {
    static const uint8_t back[8] = {0, 0, 0, 2, 4, 16, 32, 64};
    uint8_t ring[256] = {0};
    unsigned index = 0, control = rb(src++), mode = control >> 5, count = (control & 0x1f) + 1;
    for (unsigned i = 0; i < size; ++i) {
        uint8_t v;
        if (mode == 0) v = rb(src++);
        else if (mode == 1) v = (uint8_t)(ring[(index - 1) & 255] + 1);
        else if (mode == 2) v = ring[(index - 1) & 255];
        else v = ring[(index - back[mode]) & 255];
        ring[index] = v; index = (index + 1) & 255; out[i] = v;
        if (!--count && i + 1 < size) { control = rb(src++); mode = control >> 5; count = (control & 0x1f) + 1; }
    }
}

/* Stage script handlers: $C6:0000 + (mode-1)*3 -> header whose first long
 * points at (id, long pointer) pairs ending in id 0. */
static uint32_t handler(unsigned mode, unsigned id) {
    uint32_t list = rl(rl(0xc60000 + (mode - 1) * 3));
    for (unsigned k = 0; k < 16 && rb(list); ++k, list += 4)
        if (rb(list) == id) return rl(list + 1);
    return 0;
}

/* Collision layer: "FF.79 n" in the setup script (handler 1 or 3) stores
 * $036E; without it the game keeps layer 2. */
static unsigned collision_layer(unsigned mode) {
    static const uint8_t setup[2] = {1, 3};
    for (unsigned k = 0; k < 2; ++k) {
        uint32_t s = handler(mode, setup[k]);
        for (uint32_t i = s; s && i < s + 40; ++i)
            if (rb(i) == 0xff && rb(i + 1) == 0x79) return rb(i + 2);
    }
    return 2;
}

/* The map loads after "27 nn": "0a map tileset" (layer 2, $C2:0BED) with an
 * optional "0c map" (layer 1, $C2:0C0C), or "0b tileset 0c map". The
 * collision map ($7E2400 <- $7E2500) is the one on the collision layer. */
static bool map_op(uint32_t script, unsigned layer, unsigned *map, unsigned *tileset) {
    if (!script) return false;
    for (uint32_t i = script; i < script + 64; ++i) {
        if (rb(i) != 0x27) continue;
        uint32_t j = i + 2;
        if (rb(j) == 0x0a) {
            bool second = rb(j + 3) == 0x0c;
            *tileset = rb(j + 2);
            *map = second && layer == 1 ? rb(j + 4) : rb(j + 1);
            return true;
        }
        if (rb(j) == 0x0b && rb(j + 2) == 0x0c) { *tileset = rb(j + 1); *map = rb(j + 3); return true; }
    }
    return false;
}

static void build_layout(unsigned arena, unsigned variant, Layout *l) {
    unsigned mode = 0x74 + 4 * (arena - 1) + variant, map = 0, tileset = 0;
    memset(l, 0, sizeof(*l));
    if (!g_rom) return;
    static const uint8_t map_handlers[3] = {2, 6, 7};
    bool found = false;
    unsigned layer = collision_layer(mode);
    for (unsigned k = 0; k < 3 && !found; ++k) found = map_op(handler(mode, map_handlers[k]), layer, &map, &tileset);
    if (!found || !map || !tileset) return;
    uint8_t cells[256];
    decompress(rl(rl(0xd0a000 + (map - 1) * 3)), cells, 256);
    uint32_t attr = 0xd00000 | rw(0xd08000 + (tileset - 1) * 2);
    for (unsigned c = 0; c < 256; ++c) l->solid[c] = !cells[c] || (rb(attr + cells[c]) & 0x80);
    uint32_t actors = handler(mode, 4);
    for (uint32_t i = actors; actors && i < actors + 64;) {
        if (rb(i) == 0x22 && rb(i + 1) >= 1 && rb(i + 1) <= 5) {
            unsigned p = rb(i + 1) - 1;
            l->spawn_x[p] = rb(i + 2); l->spawn_y[p] = rb(i + 3);
            l->spawns |= (uint8_t)(1u << p); i += 4; continue;
        }
        if (l->spawns) break;
        ++i;
    }
    l->ok = true;
}

static const Layout *layout(unsigned arena, unsigned variant) {
    if (arena < 1 || arena > ARENAS || variant > 3) return NULL;
    if (!layout_done[arena - 1][variant]) {
        build_layout(arena, variant, &layouts[arena - 1][variant]);
        layout_done[arena - 1][variant] = true;
    }
    return layouts[arena - 1][variant].ok ? &layouts[arena - 1][variant] : NULL;
}
/* ---- Stage picture ------------------------------------------------------
 * The arena as the cartridge draws it, so rails, conveyor belts, ice and the
 * rest show where each variant puts them. Handler 2 of the stage script is
 * replayed into a scratch VRAM/CGRAM: "26 n" graphics/palette set ($0AE7,
 * 4 for Battle), "27 n" archive n ($C0:180F -> $C0:1B04: table word
 * $C0:1C96 + 2*set in bank $E3, 8-byte entries VRAM word, size, source,
 * flags; bit 7 = raw, else the $C0:1CB0 stream), "29 n vram" the same at
 * another VRAM address ($C0:17F3), "2a n" palette ($C0:01AC: table word
 * $C0:0379 + 2*set in bank $81, entry = first color, count, colors), "0a map
 * ts" layer 2, "0b ts", "0c map" layer 1. Metatiles: $D0:0000 + (ts-1)*2 ->
 * 8 bytes = four BG tilemap words (verified against the running game's VRAM
 * tilemaps). Layers composed as Mode 1 (BG1 high, BG2 high, BG1 low, BG2
 * low), then area-averaged from 16 to 7 pixels per cell. */
enum { PIC_CELLS_X = 15, PIC_CELLS_Y = 13, PIC_W = PIC_CELLS_X * 16, PIC_H = PIC_CELLS_Y * 16 };
static uint8_t scratch_vram[0x10000];
static uint16_t scratch_cgram[256];
static uint32_t scratch_pic[PIC_W * PIC_H];
static uint8_t scratch_prio[PIC_W * PIC_H];
static void load_archive(unsigned set, unsigned id, int vram_override) {
    if (!id) return;
    uint32_t entry = 0xe30000 | (rw(0xc01c96 + 2 * set) + (id - 1) * 8u);
    unsigned vram = vram_override >= 0 ? (unsigned)vram_override : rw(entry), size = rw(entry + 2);
    uint32_t src = rl(entry + 4);
    if (!size || 2u * (vram & 0x7fff) + size > sizeof(scratch_vram)) return;
    uint8_t *dst = scratch_vram + 2u * (vram & 0x7fff);
    if (rb(entry + 7) & 0x80) for (unsigned i = 0; i < size; ++i) dst[i] = rb(src + i);
    else decompress(src, dst, size);
}
static void load_palette(unsigned set, unsigned id) {
    if (!id) return;
    uint32_t entry = 0x810000 | rw(0x810000 | (rw(0xc00379 + 2 * set) + 2u * (id - 1)));
    unsigned first = rw(entry), count = rw(entry + 2);
    for (unsigned k = 0; k < count && k < 256; ++k) scratch_cgram[(first + k) & 255] = (uint16_t)rw(entry + 4 + 2 * k);
}
static uint32_t rgb(uint16_t c) {
    uint32_t r = c & 31, g = c >> 5 & 31, b = c >> 10 & 31;
    return 0xff000000u | (r << 19 | (r >> 2) << 16) | (g << 11 | (g >> 2) << 8) | (b << 3 | b >> 2);
}
static bool render_stage(unsigned mode, uint32_t *out /* MAP_W x MAP_H */, unsigned out_w, unsigned out_h) {
    uint32_t script = handler(mode, 2);
    if (!script) return false;
    memset(scratch_vram, 0, sizeof(scratch_vram)); memset(scratch_cgram, 0, sizeof(scratch_cgram));
    unsigned set = 4, tileset = 0, maps[3] = {0, 0, 0};
    for (uint32_t i = script, end = script + 64; i < end && rb(i);) {
        unsigned op = rb(i);
        if (op == 0x26) { set = rb(i + 1); i += 2; }
        else if (op == 0x27) { load_archive(set, rb(i + 1), -1); i += 2; }
        else if (op == 0x29) { load_archive(set, rb(i + 1), (int)rw(i + 2)); i += 4; }
        else if (op == 0x2a) { load_palette(set, rb(i + 1)); i += 2; }
        else if (op == 0x0a) { maps[2] = rb(i + 1); tileset = rb(i + 2); i += 3; }
        else if (op == 0x0b) { tileset = rb(i + 1); i += 2; }
        else if (op == 0x0c) { maps[1] = rb(i + 1); i += 2; }
        else break;
    }
    if (!tileset || (!maps[1] && !maps[2])) return false;
    uint32_t metatiles = 0xd00000 | rw(0xd00000 + (tileset - 1) * 2);
    const uint32_t backdrop = rgb(scratch_cgram[0]);
    for (unsigned i = 0; i < PIC_W * PIC_H; ++i) { scratch_pic[i] = backdrop; scratch_prio[i] = 0; }
    /* Ranks: BG2 low 1, BG1 low 2, BG2 high 3, BG1 high 4. */
    for (unsigned layer = 2; layer >= 1; --layer) {
        if (!maps[layer]) continue;
        uint8_t cells[256];
        decompress(rl(rl(0xd0a000 + (maps[layer] - 1) * 3)), cells, 256);
        for (unsigned cy = 0; cy < PIC_CELLS_Y; ++cy)
            for (unsigned cx = 0; cx < PIC_CELLS_X; ++cx)
                for (unsigned q = 0; q < 4; ++q) {
                    unsigned word = rw(metatiles + 8u * cells[cy * 16 + cx] + 2 * q);
                    unsigned tile = word & 0x3ff, pal = word >> 10 & 7;
                    uint8_t rank = (uint8_t)((word & 0x2000 ? 3 : 1) + (layer == 1));
                    const uint8_t *t = scratch_vram + (tile * 32u & 0xffff);
                    for (unsigned y = 0; y < 8; ++y)
                        for (unsigned x = 0; x < 8; ++x) {
                            unsigned sx = word & 0x4000 ? 7 - x : x, sy = word & 0x8000 ? 7 - y : y, bit = 7 - sx;
                            unsigned p = (t[2 * sy] >> bit & 1) | (t[2 * sy + 1] >> bit & 1) << 1 |
                                         (t[16 + 2 * sy] >> bit & 1) << 2 | (t[16 + 2 * sy + 1] >> bit & 1) << 3;
                            if (!p) continue;
                            unsigned px = cx * 16 + (q & 1) * 8 + x, py = cy * 16 + (q >> 1) * 8 + y;
                            unsigned at = py * PIC_W + px;
                            if (rank < scratch_prio[at]) continue;
                            scratch_prio[at] = rank; scratch_pic[at] = rgb(scratch_cgram[pal * 16 + p]);
                        }
                }
    }
    /* Area average: output pixel (ox, oy) covers source [ox*W/w, (ox+1)*W/w). */
    for (unsigned oy = 0; oy < out_h; ++oy)
        for (unsigned ox = 0; ox < out_w; ++ox) {
            unsigned x0 = ox * PIC_W / out_w, x1 = (ox + 1) * PIC_W / out_w;
            unsigned y0 = oy * PIC_H / out_h, y1 = (oy + 1) * PIC_H / out_h;
            unsigned r = 0, g = 0, b = 0, n = 0;
            for (unsigned y = y0; y < y1; ++y)
                for (unsigned x = x0; x < x1; ++x) {
                    uint32_t c = scratch_pic[y * PIC_W + x];
                    r += c >> 16 & 255; g += c >> 8 & 255; b += c & 255; ++n;
                }
            out[oy * out_w + ox] = n ? 0xff000000u | (r / n) << 16 | (g / n) << 8 | (b / n) : backdrop;
        }
    return true;
}

static unsigned choice_variant(unsigned choice) { return choice < 4 ? choice : 0; }

/* Only stages whose setup script tests script flag 3 ("08 03 49 ..": no soft
 * blocks, every cell an item) honour the 0110 rule; in this cartridge that is
 * arena 1. Elsewhere the list offers the four layouts only. */
static bool items_supported(unsigned arena) {
    if (!g_rom || arena < 1 || arena > ARENAS) return false;
    uint32_t setup = handler(0x74 + 4 * (arena - 1), 1);
    for (uint32_t i = setup; setup && i < setup + 24; ++i)
        if (rb(i) == 0x08 && rb(i + 1) == 0x03 && rb(i + 2) == 0x49) return true;
    return false;
}
static bool items_ok[ARENAS], prepared;
static unsigned choice_count(void) {
    unsigned arena = g_ram[ARENA];
    if (arena < 1 || arena > ARENAS) return CHOICES - 1;
    return (prepared ? items_ok[arena - 1] : items_supported(arena)) ? CHOICES : CHOICES - 1;
}

/* ---- Input (inside the simulation) ------------------------------------ */

static void apply_choice(unsigned choice) {
    saved_bits = g_ram[BITS]; saved_saved_bits = g_ram[SAVED_BITS];
    uint8_t bits = (uint8_t)(saved_bits & ~(VARIANT_MASK));
    static const uint8_t variant_bit[4] = {0, 0x01, 0x02, 0x04};
    bits |= variant_bit[choice_variant(choice)];
    if (choice == 4) { bits |= ITEM_BIT; g_ram[FLAGS] |= ITEM_FLAG; }
    g_ram[BITS] = bits;
    /* Without these, the script may pick a saved password layout or rule. */
    g_ram[SAVED_BITS] = (uint8_t)(saved_saved_bits & ~(VARIANT_MASK));
    restore_pending = 1;
}

/* The pad reader computes "new" presses from the previous frame's held
 * arrays ($C0:0A5A copies D+$2C/$31 to $039F/$03A4 before reading). Clearing
 * the held arrays while the list is up therefore makes a key that is still
 * held look new again on the cartridge's next frames, and the list confirmed
 * itself a few frames after opening. The mod keeps its own previous held
 * state for the list, and after the list closes it keeps the selector's input
 * empty until every seat has released its keys. */
static uint8_t prev_act[5], prev_dir[5], suppress;

/* Menu sounds: the native group-2 menu commands (move 1, back 3, accept 4,
 * as the Battle setup screens use them) played through $C0:0CED from a small
 * trampoline in $FF padding at $C5:7500, entered from the pad reader's tail
 * like Louie's dash sound: it saves registers and flags, calls the driver
 * with 8-bit registers, then runs the original PLX/PLY/RTL. The id byte is
 * written right before the redirect, so the sound is part of the emulated
 * frame (save states and netplay replay it). */
enum { SFX_CODE = 0xc57500, SFX_ID = SFX_CODE + 7, SFX_MOVE = 1, SFX_BACK = 3, SFX_ACCEPT = 4 };
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
static void pad_hook(CpuState *cpu, uint32_t pc) {
    (void)pc;
    if (!enabled) return;
    const unsigned held = cpu->D + 0x2c, dirs = cpu->D + 0x31, fresh = cpu->D + 0x36, fresh_dirs = cpu->D + 0x3b;
    uint8_t new_act[5], new_dir[5];
    bool any_held = false;
    for (unsigned seat = 0; seat < 5; ++seat) {
        uint8_t act = g_ram[(held + seat) & 0xffff], dir = g_ram[(dirs + seat) & 0xffff];
        new_act[seat] = act & (uint8_t)~prev_act[seat];
        new_dir[seat] = dir & (uint8_t)~prev_dir[seat];
        prev_act[seat] = act; prev_dir[seat] = dir;
        if (act | dir) any_held = true;
    }
    const bool selector = in_selector();
    uint8_t sound = 0;
    if (inject_seat) {
        if (selector) {
            unsigned seat = inject_seat - 1u;
            for (unsigned s = 0; s < 5; ++s) {
                g_ram[(held + s) & 0xffff] = 0; g_ram[(dirs + s) & 0xffff] = 0;
                g_ram[(fresh + s) & 0xffff] = 0; g_ram[(fresh_dirs + s) & 0xffff] = 0;
            }
            g_ram[(held + seat) & 0xffff] = PAD_A; g_ram[(fresh + seat) & 0xffff] = PAD_A;
            suppress = 1;
        }
        inject_seat = 0;
        return;
    }
    if (suppress && !open_menu) {
        if (!selector || !any_held) {
            suppress = 0; /* released: this read (nothing held) goes through */
            return;
        }
    } else if (!open_menu) {
        if (!selector) return;
        for (unsigned seat = 0; seat < 5; ++seat)
            if (new_act[seat] & PAD_CONFIRM) { open_menu = 1; cursor = 0; sound = SFX_ACCEPT; break; }
        if (!open_menu) return;
    } else if (!selector) {
        open_menu = 0; /* the screen went away underneath (reset, netplay resync) */
        return;
    } else {
        for (unsigned seat = 0; seat < 5; ++seat) {
            uint8_t act = new_act[seat], dir = new_dir[seat];
            if (act & PAD_B) { open_menu = 0; suppress = 1; sound = SFX_BACK; break; }
            if (act & PAD_CONFIRM) {
                apply_choice(cursor < choice_count() ? cursor : 0);
                open_menu = 0; inject_seat = (uint8_t)(seat + 1); break;
            }
            unsigned count = choice_count();
            if (dir & DIR_UP) { cursor = (uint8_t)((cursor + count - 1) % count); sound = SFX_MOVE; break; }
            if (dir & DIR_DOWN) { cursor = (uint8_t)((cursor + 1) % count); sound = SFX_MOVE; break; }
        }
    }
    /* The carousel sees no input while the list is (or was just) up. */
    for (unsigned seat = 0; seat < 5; ++seat) {
        g_ram[(held + seat) & 0xffff] = 0; g_ram[(dirs + seat) & 0xffff] = 0;
        g_ram[(fresh + seat) & 0xffff] = 0; g_ram[(fresh_dirs + seat) & 0xffff] = 0;
    }
    play_sfx(sound); /* last: a redirect ends this PC's hooks for the frame */
}

void BattleVariantsAfterFrame(void) {
    if (!restore_pending) return;
    unsigned mode = g_ram[MODE];
    /* The stage started (its script has read the bits), or Battle was left. */
    if (mode >= 0x74 || mode < 4) {
        g_ram[BITS] = saved_bits;
        g_ram[SAVED_BITS] = saved_saved_bits;
        restore_pending = 0;
    }
}

static void prepare_all(void);
void BattleVariantsBeforeFrame(void) {
    if (!enabled) return;
    if (!hook_installed) hook_installed = interp_bridge_add_pre_opcode_hook(PAD_HOOK, pad_hook);
    install_sfx();
    prepare_all();
}

/* ---- Drawing (presentation only) -------------------------------------- */

typedef struct { uint32_t *px; unsigned pitch; int w, h; } Canvas;

static void put(Canvas *c, int x, int y, uint32_t col) {
    if (x >= 0 && x < c->w && y >= 0 && y < c->h) c->px[(unsigned)y * c->pitch + (unsigned)x] = col;
}
static void fill(Canvas *c, int x, int y, int w, int h, uint32_t col) {
    for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) put(c, x + i, y + j, col);
}
static void frame_rect(Canvas *c, int x, int y, int w, int h, uint32_t col) {
    fill(c, x, y, w, 1, col); fill(c, x, y + h - 1, w, 1, col);
    fill(c, x, y, 1, h, col); fill(c, x + w - 1, y, 1, h, col);
}

/* Text: the authored 5x7 menu font plus Ç and Ã (codes 1 and 2), with a
 * one-pixel drop shadow. */
static const uint8_t kArrow[7] = {16, 24, 28, 30, 28, 24, 16};
static const uint8_t kUp[7] = {0, 4, 14, 31, 0, 0, 0}, kDown[7] = {0, 0, 0, 31, 14, 4, 0};
static const uint8_t *glyph(unsigned ch) {
    if (ch == 1) return Sb5MenuGlyph('C');
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
    if (ch == 1) { fill(c, x + 2 * s, y + 7 * s, s, s, col); fill(c, x + 1 * s, y + 8 * s, s, s, col); }
    if (ch == 2) {
        static const uint8_t tilde[2] = {13, 22}; /* .##.# / #.##. */
        for (int r = 0; r < 2; ++r) for (int b = 0; b < 5; ++b)
            if (tilde[r] >> (4 - b) & 1) fill(c, x + b * s, y + (r - 3) * s, s, s, col);
    }
}
static int text(Canvas *c, int x, int y, const char *s, uint32_t col, int scale) {
    for (; *s; ++s, x += 6 * scale) {
        if (*s == ' ') continue;
        glyph_at(c, x + scale, y + scale, (uint8_t)*s, 0xff000000u, scale);
        glyph_at(c, x, y, (uint8_t)*s, col, scale);
    }
    return x;
}
static int text_width(const char *s, int scale) { return (int)strlen(s) * 6 * scale - scale; }

static bool ptbr(void);
/* Named after the passwords: what each layout changes depends on the arena. */
static const char *const kNamesPt[CHOICES] = {"ORIGINAL", "VARIANTE 1", "VARIANTE 2", "VARIANTE 3", "VARIANTE 4"};
static const char *const kNamesEn[CHOICES] = {"ORIGINAL", "VARIANT 1", "VARIANT 2", "VARIANT 3", "VARIANT 4"};

static void draw_minimap(Canvas *c, int x0, int y0, const Layout *l, bool items, bool picture) {
    /* Drawn once per arena and choice (prepare_all): over the stage picture when
     * it could be rebuilt (badges and item marks only), else flat cells. */
    static const uint32_t seat[5] = {0xfff4f4f4u, 0xff383838u, 0xffe04848u, 0xff4870e8u, 0xff48c048u};
    static const uint32_t ink[5] = {0xff202020u, 0xfff4f4f4u, 0xfff4f4f4u, 0xfff4f4f4u, 0xff202020u};
    const int cell = 7;
    for (int y = 0; y < 13; ++y)
        for (int x = 0; x < 15; ++x) {
            int px = x0 + x * cell, py = y0 + y * cell;
            if (picture) {
                if (items && !l->solid[y * 16 + x]) {
                    fill(c, px + 2, py + 2, 3, 3, 0xff000000u);
                    put(c, px + 3, py + 3, 0xfff0c040u);
                }
            } else if (l->solid[y * 16 + x]) {
                fill(c, px, py, cell, cell, 0xff9da0c0u);
                fill(c, px, py, cell, 1, 0xffd8daf0u); fill(c, px, py, 1, cell, 0xffd8daf0u);
                fill(c, px, py + cell - 1, cell, 1, 0xff5a5c7au); fill(c, px + cell - 1, py, 1, cell, 0xff5a5c7au);
            } else {
                fill(c, px, py, cell, cell, (x + y) & 1 ? 0xff3a2246u : 0xff432a52u);
                if (items) fill(c, px + 3, py + 3, 2, 2, 0xfff0c040u);
            }
        }
    /* "P1".."P5" badges centred on the spawn cell, kept inside the map. */
    for (unsigned p = 0; p < 5; ++p) {
        if (!(l->spawns >> p & 1)) continue;
        int bx = x0 + l->spawn_x[p] * cell + cell / 2 - 7, by = y0 + l->spawn_y[p] * cell + cell / 2 - 5;
        if (bx < x0) bx = x0;
        if (bx > x0 + 15 * cell - 15) bx = x0 + 15 * cell - 15;
        if (by < y0) by = y0;
        if (by > y0 + 13 * cell - 11) by = y0 + 13 * cell - 11;
        fill(c, bx, by, 15, 11, seat[p]);
        frame_rect(c, bx, by, 15, 11, 0xff000000u);
        char label[3] = {'P', (char)('1' + p), 0};
        const uint8_t *g0 = Sb5MenuGlyph('P'), *g1 = Sb5MenuGlyph((unsigned char)label[1]);
        for (int row = 0; row < 7; ++row)
            for (int b = 0; b < 5; ++b) {
                if (g0[row] >> (4 - b) & 1) put(c, bx + 2 + b, by + 2 + row, ink[p]);
                if (g1[row] >> (4 - b) & 1) put(c, bx + 8 + b, by + 2 + row, ink[p]);
            }
    }
}

/* Every minimap is built once, from the player's own cartridge, when the
 * feature is switched on: 13 arenas x (4 layouts + the item rule). Nothing
 * derived from the ROM is stored in the repository or the package. */
enum { MAP_W = 105, MAP_H = 91 }; /* 15 x 13 cells of 7 pixels */
static uint32_t images[ARENAS][CHOICES][MAP_W * MAP_H];
static bool images_ready[ARENAS][CHOICES];
static void prepare_all(void) {
    if (prepared || !g_rom) return;
    for (unsigned a = 1; a <= ARENAS; ++a) {
        items_ok[a - 1] = items_supported(a);
        for (unsigned choice = 0; choice < CHOICES; ++choice) {
            const Layout *l = layout(a, choice_variant(choice));
            Canvas img = {images[a - 1][choice], MAP_W, MAP_W, MAP_H};
            images_ready[a - 1][choice] = l != NULL;
            if (!l) continue;
            bool picture = render_stage(0x74 + 4 * (a - 1) + choice_variant(choice), images[a - 1][choice], MAP_W, MAP_H);
            draw_minimap(&img, 0, 0, l, choice == 4, picture);
        }
    }
    prepared = true;
}

void BattleVariantsDraw(void) {
    if (!enabled || !open_menu || !g_ppu || !g_ppu->renderBuffer) return;
    prepare_all();
    Canvas canvas = {(uint32_t *)g_ppu->renderBuffer, g_ppu->renderPitch / 4u, 256, 224}, *c = &canvas;
    canvas.px += g_ppu->extraLeftRight;
    bool pt = ptbr();
    /* Dim the carousel behind the panel. */
    for (int y = 0; y < 224; ++y)
        for (int x = 0; x < 256; ++x) {
            uint32_t *p = &canvas.px[(unsigned)y * canvas.pitch + (unsigned)x];
            *p = 0xff000000u | ((*p >> 2) & 0x003f3f3fu);
        }
    fill(c, 4, 4, 248, 216, 0xff24132eu);
    frame_rect(c, 4, 4, 248, 216, 0xffe8b830u);
    frame_rect(c, 5, 5, 246, 214, 0xff8a5a10u);
    const char *title = pt ? "ESCOLHA A VARIA\001\002O" : "CHOOSE A VARIANT";
    text(c, (256 - text_width(title, 2)) / 2, 14, title, 0xfff8f8f8u, 2);

    unsigned arena = g_ram[ARENA];
    char stage[16];
    const char *word = pt ? "FASE " : "STAGE ";
    size_t n = strlen(word);
    memcpy(stage, word, n);
    if (arena >= 10) stage[n++] = (char)('0' + arena / 10);
    stage[n++] = (char)('0' + arena % 10); stage[n] = 0;
    text(c, 12, 38, stage, 0xfff0c040u, 1);
    unsigned shown = cursor < choice_count() ? cursor : 0; /* read only */
    if (arena >= 1 && arena <= ARENAS && images_ready[arena - 1][shown]) {
        const uint32_t *img = images[arena - 1][shown];
        for (int y = 0; y < MAP_H; ++y)
            memcpy(&canvas.px[(unsigned)(50 + y) * canvas.pitch + 12], img + y * MAP_W, MAP_W * 4u);
    } else {
        fill(c, 12, 50, MAP_W, MAP_H, 0xff000000u);
    }
    {
        char info[40];
        static const char *const pt_info[CHOICES] = {"LAYOUT ORIGINAL DA ARENA", "SENHA 4622", "SENHA 0413", "SENHA 0926",
                                                     "SENHA 0110 - BLOCOS VIRAM ITENS"};
        static const char *const en_info[CHOICES] = {"ORIGINAL ARENA LAYOUT", "PASSWORD 4622", "PASSWORD 0413", "PASSWORD 0926",
                                                     "PASSWORD 0110 - BLOCKS ARE ITEMS"};
        snprintf(info, sizeof(info), "%s", (pt ? pt_info : en_info)[shown]);
        text(c, 12, 176, info, 0xffb8b0c8u, 1);
    }

    const char *const *names = pt ? kNamesPt : kNamesEn;
    unsigned count = choice_count();
    for (unsigned i = 0; i < count; ++i) {
        int y = 54 + (int)i * 22;
        if (i == shown) {
            fill(c, 119, y - 4, 132, 22, 0xff5a3a80u);
            frame_rect(c, 119, y - 4, 132, 22, 0xffe8b830u);
            text(c, 121, y + 3, ">", 0xfff0c040u, 1);
        }
        text(c, 129, y, names[i], i == shown ? 0xfff8f8f8u : 0xffb8b0c8u, 2);
    }
    /* Hints: up/down, A, B. */
    int x = 14;
    const int y = 198;
    x = text(c, x, y, "^v", 0xfff8f8f8u, 1);
    x = text(c, x + 4, y, pt ? "ESCOLHER" : "CHOOSE", 0xffb8b0c8u, 1);
    fill(c, x + 10, y - 2, 11, 11, 0xff3cb43cu); text(c, x + 13, y, "A", 0xfff8f8f8u, 1);
    x = text(c, x + 25, y, pt ? "CONFIRMAR" : "CONFIRM", 0xffb8b0c8u, 1);
    fill(c, x + 10, y - 2, 11, 11, 0xffd83c3cu); text(c, x + 13, y, "B", 0xfff8f8f8u, 1);
    text(c, x + 25, y, pt ? "VOLTAR" : "BACK", 0xffb8b0c8u, 1);
}

/* ---- Lifecycle, save states ------------------------------------------- */

#ifndef BATTLE_VARIANTS_TEST
#include "ptbr_mod.h"
static bool ptbr(void) { return PtbrEnabled(); }
#endif

void BattleVariantsReset(void) {
    remove_sfx();
    open_menu = cursor = inject_seat = suppress = 0;
    memset(prev_act, 0, sizeof(prev_act)); memset(prev_dir, 0, sizeof(prev_dir));
    if (restore_pending) { g_ram[BITS] = saved_bits; g_ram[SAVED_BITS] = saved_saved_bits; }
    restore_pending = 0;
}
#define BV_MAGIC 0x52415642u /* "BVAR" */
void BattleVariantsSave(struct SaveLoadInfo *sli) {
    uint32_t magic = BV_MAGIC;
    uint8_t state[6] = {open_menu, cursor, inject_seat, restore_pending, saved_bits, saved_saved_bits};
    sli->func(sli, &magic, 4); sli->func(sli, state, sizeof(state));
    /* v2.4.2: input tracking (states from v2.4.0/2.4.1 end after state[]). */
    uint8_t input[11];
    memcpy(input, prev_act, 5); memcpy(input + 5, prev_dir, 5); input[10] = suppress;
    sli->func(sli, input, sizeof(input));
}
void BattleVariantsLoad(struct SaveLoadInfo *sli, unsigned version) {
    (void)version;
    open_menu = cursor = inject_seat = restore_pending = suppress = 0;
    memset(prev_act, 0, sizeof(prev_act)); memset(prev_dir, 0, sizeof(prev_dir));
    if (!RtlStateBytesRemaining(sli)) return;
    uint32_t magic = 0;
    uint8_t state[6] = {0};
    sli->func(sli, &magic, 4); sli->func(sli, state, sizeof(state));
    if (magic != BV_MAGIC || state[0] > 1 || state[1] >= CHOICES || state[2] > 5 || state[3] > 1) {
        RtlStateStreamFail(sli);
        return;
    }
    open_menu = state[0]; cursor = state[1]; inject_seat = state[2]; restore_pending = state[3];
    saved_bits = state[4]; saved_saved_bits = state[5];
    if (RtlStateBytesRemaining(sli) < 11) return;
    uint8_t input[11];
    sli->func(sli, input, sizeof(input));
    if (input[10] > 1) { RtlStateStreamFail(sli); return; }
    memcpy(prev_act, input, 5); memcpy(prev_dir, input + 5, 5); suppress = input[10];
}

static void reset_plugin(void) { BattleVariantsReset(); enabled = false; }
static void activate(void) { enabled = true; }
SNES_MOD_CONSTRUCTOR(register_battle_variants) {
    snes_mod_register_reset_callback(reset_plugin);
    snes_mod_register_activation_plugin("superbomberman5.battle.variants", activate);
}
