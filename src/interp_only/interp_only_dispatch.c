/* Dispatch tables for an INTERPRETER-ONLY build (SB5_INTERPRETER_ONLY).
 *
 * Android cannot run the launcher's Generate & rebuild wizard (no compiler or
 * Rust analyzer on the device), and an APK must never carry generated C: that
 * code is derived from the ROM. This build therefore links no recompiled code
 * at all. Every guest PC misses this table, so the framework's 65816
 * interpreter runs the whole game, exactly as it already runs reset/NMI and
 * almost all of Super Bomberman 5's logic in the desktop build (only ~9
 * routine variants are AOT there; see AGENTS.md section 1).
 *
 * This is not a stub: no routine is replaced by an invented result. A miss is
 * the framework's documented fallback to authoritative LLE execution of the
 * player's own cartridge bytes. The layout mirrors what program_emit.py emits
 * for a game with no functions and no ram_routine entries. */

#include "cpu_state.h"

#if !defined(SB5_INTERPRETER_ONLY)
#error "interp_only_dispatch.c is only compiled into SB5_INTERPRETER_ONLY builds"
#endif

const DispatchEntry g_dispatch_table[] = {
    { 0xFFFFFFFFu, { NULL, NULL, NULL, NULL }, 0 },
};
const unsigned g_dispatch_table_count = 0u;

const RamRoutineGuard g_ram_routine_guards[] = {
    { 0xFFFFFFFFu, 0u, 0u },
};
const unsigned g_ram_routine_guard_count =
    (unsigned)(sizeof(g_ram_routine_guards) / sizeof(g_ram_routine_guards[0]));
