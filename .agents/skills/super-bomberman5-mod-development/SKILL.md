---
name: super-bomberman5-mod-development
description: Create, extend, debug and validate native mods and gameplay/presentation features for the Super Bomberman 5 SNESRecomp port (src/*_mod.c, mod manifests, RAM research, overlays, in-memory ROM patches, save-state extensions, native tests). Use for any gameplay, menu, translation or rendering change; use super-bomberman5-repository for Git, versions and releases.
---

# Super Bomberman 5 — native mod development

Read `AGENTS.md` at the repository root first (hard rules, RAM map, build and
pitfalls). This skill is the step-by-step recipe for a feature. Every existing
mod (`ptbr`, `normal_character`, `selection_color`, `normal_coop`) is a worked
example; copy their structure instead of inventing a new one.

## 1. Scope the feature against evidence

- Read `docs/STATUS.md` and the existing feature doc in `docs/`. Decide whether
  the change is **presentation-only** (draws over the frame, never changes
  guest state → presentation plugin) or **gameplay** (writes WRAM/ROM →
  activation plugin, `presentation_only = false`).
- Find the guest state by research on the real JPN image, never by guessing:
  - Disassemble: `python .bootstrap/dis-hi.py <PC24 hex> [len hex]` from the
    repo root (HiROM file offset `(bank&0x3F)<<16|addr`; widths follow REP/SEP).
  - Observe: scripted headless runs with `dump <tag>`, `until <addr> == <v>`,
    `poke`, comparing WRAM across frames. Keep scripts and captures in
    `.bootstrap/<topic>/` (ignored).
  - Confirm each address under at least two situations (e.g. 1P and 2P, Normal
    and Battle) before relying on it. Add verified facts to AGENTS.md §6.

## 2. Implement the native plugin

Create `src/<name>_mod.c` + `src/<name>_mod.h`, then:

1. Register with the mod runtime (feature stays inert until the launcher
   enables it):
   ```c
   static bool enabled;
   void FooReset(void) { /* clear session state, keep `enabled` */ }
   static void reset_plugin(void) { FooReset(); enabled = false; /* restore ROM patches, hooks */ }
   static void activate(void) { enabled = true; }
   SNES_MOD_CONSTRUCTOR(register_foo) {
       snes_mod_register_reset_callback(reset_plugin);
       snes_mod_register_activation_plugin("superbomberman5.foo.feature", activate);
       /* presentation-only: snes_mod_register_presentation_plugin(...) */
   }
   ```
   The plugin id string must equal `[[plugin]] id` in the manifest.
2. Hook into `src/game_rtl.c`: before/after `snes_beam_frame_driver_run_frame`,
   in `GameSessionReset`, `GameHardwareReset`, `GameSaveExtra`/`GameLoadExtra`
   (append new extensions **last**, same order in both) and, if needed,
   `on_state_loaded`. Think about interaction order with the existing hooks
   (character picker snapshot/restore runs last in the frame).
3. Add the source to `add_executable` in `CMakeLists.txt`.
4. Guard every action with `enabled` **and** a precise context check (mode
   `$1031`, script PC from `$0C90/$0CB8/$0CE0`, return addresses at `$2000+`).
   Edge-detect input with a stored previous pad; never act on held buttons.

### Techniques and their rules

- **WRAM writes**: write only fields whose meaning is verified; respect native
  ownership (refcounts, allocators). If the game later overwrites your value,
  keep it "pending" until the native consumer has read it (see
  `selection_color_mod.c`).
- **Snapshots** (`RtlSaveSnapshotToMemory`/`RtlLoadSnapshotFromMemory`): used to
  suspend Normal and borrow Battle's picker. Detach buffers before loading, and
  bound sizes.
- **In-memory ROM patches** (`SnesRomPtr`): `reserve()` originals, patch only
  while enabled, restore all on reset; use verified-unused space for authored
  65816 trampolines; never touch the ROM file or SRAM layout.
- **Interpreter PC hook** (`g_interp_bridge_pc_hook`): chain the previous hook,
  uninstall cleanly, keep it cheap (runs per instruction). Only interpreted
  code triggers it — note in the doc that AOT promotion of that routine would
  break the mod.
- **Rendering overlays**: run inside the observer passed to
  `PtbrDrawPpuFrameObserved`; save every VRAM/OAM word you modify, restore at
  line 224, increment `vramWriteCount`/`oamWriteCount`, borrow tiles/OBJ slots
  only when no visible native object uses them. Glyphs are authored bitmaps or
  derived at runtime from cartridge VRAM — no extracted assets in source.
  Menu translation text goes in `translations/ptbr/labels.json` (with tile
  guards and screen context) rather than C.
- **Save-state extension**: magic + version-tolerant load
  (`RtlStateBytesRemaining(sli)==0` ⇒ older state), validate every field and
  call `RtlStateStreamFail` on bad input, keep a constant size per build.

## 3. Package the mod (data only)

- `mods/preloaded/packages/superbomberman5.<id>/0.1.0/manifest.toml` copying an
  existing manifest: `[[target]] game_id = "superbomberman5-jpn"` + ROM SHA-256
  from `rom_identity.txt`, `[[feature]]` with `default_enabled = false`, group
  `Gameplay` or `Translation`, `[[plugin]]` with the registered id.
- `tools/package-<id>.py` (copy `package-normal-coop.py`) producing
  `dist/superbomberman5-<id>-<ver>.snesmod` with manifest + LICENSE only.
- Mention the required host version: a package cannot add native code to an
  older executable.

## 4. Test

- Native unit test `tests/<name>_tests.c`: stub the framework functions it
  calls, `#include "../src/<name>_mod.c"`, drive `g_ram`/fake ROM through the
  scenarios (enable/disable, context guards, edge input, save/load round trip
  and corrupt-state rejection, reset restoring ROM/hook). Register in the
  `BUILD_TESTING` block of `CMakeLists.txt`.
- Build: `./tools/build-windows.ps1` (`-Regenerate` only if `recomp/` changed),
  run all four-plus test executables in `build-lle/`.
- Runtime: extend or mirror `tests/normal_coop_runtime.py` (isolated folder,
  fresh SRAM, `mods/preloaded/state.toml` enabling features, headless
  `--script`). Exercise the feature alone and combined with the other mods
  (PT-BR on/off, character + colors + coop). Inspect dumps; grep logs for
  `interp_cap`, `unresolved-abandon`, `BRK`, `fatal`.
- If you cannot run Windows builds from your environment, say so and list the
  exact commands for the user; do not claim validation.

## 5. Document

- New `docs/<FEATURE>.md` in Portuguese and English: how to enable, controls,
  implementation summary, what was validated (route, frames, seats), and
  what is **not** qualified.
- Dated section at the top of `docs/STATUS.md` with evidence and limitations.
- README.md: add the mod to the "Included mods"/source-build text in **both**
  English and Português sections; keep host/package version wording precise.
- New addresses → AGENTS.md §6; new pitfalls → AGENTS.md §8.
- Then run `python tools/audit-publication.py --history`,
  `python -m unittest discover -s tests -v`, `git diff --check`, and hand over
  to the `super-bomberman5-repository` skill for commit/version/release.
