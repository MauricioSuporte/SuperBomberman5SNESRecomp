---
name: super-bomberman5-mod-development
description: Create, extend, debug and validate native mods and gameplay/presentation features for the Super Bomberman 5 SNESRecomp port (src/*_mod.c, mod manifests, RAM research, overlays, in-memory ROM patches, save-state extensions, native tests). Use for any gameplay, menu, translation or rendering change; use super-bomberman5-repository for Git, versions and releases.
---

# Super Bomberman 5 — native mod development

Read `AGENTS.md` at the repository root first (hard rules, RAM map, build and
pitfalls). This skill is the step-by-step recipe for a feature. Every existing
mod (`ptbr`, `normal_character`, `selection_color`, `infinite_time`,
`normal_coop`) is a worked
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
- **Picker grid palettes**: preserve the upper BG strip first entry, shared
  with the lower figure at index 8; upload the other seven colors. Portrait
  OBJ palettes still need nine colors. See `docs/SELECTION_COLORS.md`.
- **Save-state extension**: magic + version-tolerant load
  (`RtlStateBytesRemaining(sli)==0` ⇒ older state), validate every field and
  call `RtlStateStreamFail` on bad input, keep a constant size per build.

### Lessons from the five-player co-op mod (v0.5.0)

- **Hunt every two-seat assumption.** The cartridge was written for P1/P2:
  `CPX/CPY #$02` loops (exit warps, damage, floor gimmicks), unrolled P1/P2
  code, two-entry interleaved arrays (`$1FF4-$1FFF` enemy distances) and
  hard-coded slot literals (`LDX #2`, `LDA $17FC` = actor slot 2). Search the
  ROM for the pattern family, not only the first hit, and list each site with
  its caller in AGENTS.md. A feature that works for P1 but not P3 almost
  always means one of these.
- **Know the fixed capacities.** Bomber tables (`$0FDE`, `$0FEA`, `$101F`, …)
  have five entries; boss rivals take bomber slots. Exceeding them jumps into
  data (BRK in the log, frozen/garbled frame). Prefer adapting the roster
  (bench a seat for one stage, restore its fields afterwards) over growing a
  native table you cannot fully enumerate.
- **Relocate small arrays instead of overlapping them.** When an array must
  grow, move every operand to verified-unused WRAM (scan many recorded dumps)
  and patch the clear loop, the fill routine and each consumer loop together.
- **Patch the operand byte, not the opcode.** `CPX #$0C` at `$C3:4765` has its
  operand at `$C3:4766`; writing `pc+1` of the wrong base corrupted the next
  instruction and crashed. Unit-test each patched byte against a fake ROM
  containing the original bytes.
- **Order matters.** Campaign spawns run before `$C5:3D3F` copies the stage
  collision map; checks that read the map must run at its return (`$C5:3D5D`).
  Hook the consumer boundary, not the first convenient PC.
- **Interpreted code only.** In-memory ROM patches and the PC hook affect only
  interpreted routines; confirm with a PC trace that the routine runs in the
  interpreter before relying on a patch.
- **Overlays follow native animation.** Sprites animate (tile and y bounce) and
  screens keep drawing through fades (mode `$1031` 2 → 3). Recognize the whole
  tile range, pin idle sprites to the overlay position, keep animation offsets
  only for the animated tiles, and keep the overlay until the native screen is
  gone while ignoring input outside the interactive mode.
- **Leave native feedback alone.** Suppressing guest input (Up/Down) to drive
  your own menu also removed the native cursor sound; let the guest see the
  input and overwrite its state each frame instead.
- **Snapshot extensions grow at the end.** Append new fields after the last
  existing one and read them only when `RtlStateBytesRemaining` says they are
  present, so older save states still load.

### Adding a playable character (Subordinate Bomber, v0.6.0)

The game already carries more bombers than the picker offers. Recipe used for
character 10 (`src/subordinate_mod.c`), reusable for the next ones:

1. **Find the character index.** Player chars are `$625F` 1-9 → `$16C2` 0-8.
   Rivals are created by `$C4:3CB5` with `$16C2 = type` and `$16DA =
   BB4E[type]+7`. Force a boss stage (Normal route, `until 0x1031 == 0d`, then
   `poke 0x1031 <stage>`; `$6A` = Zone 3-D) and read `$16C2/$16DA` of the rival
   slots. Subordinate Bomber is index 9.
2. **Audit every table read through `$16C2`.** Search the ROM for
   `LDA $16C2,X` (`BD C2 16`) and check each table's length: `$C0:BB30/BB3A/
   BB44/BB4E`, `B9C0/B9CA`, `BB66`, `$C4:551B`, `$C5:242C` have 10 entries
   (gameplay works as is); HUD faces `$C2:1470` / `$C5:12FE` have 9 (the 10th
   reads code). Tables indexed with `+$1CBF` (`BB58/BB5F`) are mounts, not
   players. Relocate short tables to free ROM (`$C2:4207-$7FFF` padding) and
   repoint the long operand.
3. **Palettes.** Each character has a block at `BB4E[c]`: +0..5 the six L/R
   colors, +6 an effect palette, +7 the rival palette (Bomberman has 7 slots,
   no rival). Palette records live through the table at `g_rom[0x379]`
   (bank C1), nine colors each. Raise any `id <= 0x47` guard.
4. **Put it in the picker.** The picker is a bytecode script (task 9,
   `$0C90/$0CB8/$0CE0`): `4F nn 59 FF 51 <addr> 7E 4F vv 4A` stores `vv` at
   `<addr>[nn]`. Cell 9 is filled at `$C6:8B17` (X) / `$C6:8AF9` (unlocked
   secret). Patch the stored value, not the script flow. The grid drawer
   `$C2:122A` only has BG arrangements for chars 1-9, so skip its X for the new
   char (trampoline) and draw the figure as an overlay.
5. **Missing art.** Nothing exists for grid figure, seat portrait (index 9
   shows Bomberman's frame) or HUD faces. Build what you can at render time
   from the character's own sprite tiles in ROM (find them by matching a stage
   OAM/VRAM dump against the ROM; the standing frame is three 8x8 rows,
   mirrored); maintainer-authored art goes in source as palette indices plus,
   when its colors differ from the cartridge, one authored color set per
   selectable color. Ask the maintainer for art provenance; never paste
   cartridge pixels (AGENTS.md rule 4).
6. **Overlay mechanics that bit us.** Seat portraits are four 16x16 OBJ at
   (objX-8, objY-25), face at (4..27, 4..27); the frame and per-player
   background are BG, keep them. HUD faces are one 16x16 OBJ 17 lines above
   the face object `$7E35C5+i` (anims: alive table, defeated table; Time Over
   is the bomber's own anim `$1F`). Objects on the same frame share VRAM
   tiles, including across characters, so draw into borrowed free OBJ tile
   blocks and repoint only your sprites; write palettes render-only into the
   seat's slot only when no other visible sprite uses it.
7. **Integrate.** Let `selection_color_mod.c` accept the new character
   (`last_character()`), let `normal_character_mod.c` accept the choice, chain
   the overlay in `ObserveOverlays` (draw after the color hint, restore
   before it), and test: patch bytes/restore, every pixel, shared tiles with
   Bomberman, two seats on the new char, palette conflicts, exact
   VRAM/OAM/CGRAM restoration. Runtime: picker cycling all colors (PT-BR on),
   Battle through the score board, a defeated HUD face (`poke 0x1ffe7 <mask>`
   with the fixture), Normal Zone 1-1 and the character's boss stage.

### Research toolbox that paid off

- Lab harness (kept outside the repo or under ignored `.bootstrap/`): build the
  host with an opt-in instrumentation file that, from environment variables,
  logs value changes of one WRAM address with a ring of the last 64 PCs and of
  recent jumps (`from>to`), records executed PCs per frame window (diff two
  runs to find the code that differs), and dumps the jump history when a given
  PC is reached (find what led to a crash). Never compile it into releases.
- Headless routes: `--script` with `press`, `poke`, `wait`, `dump`; the
  `SB5_COOP_INPUT_FIXTURE` mailbox drives pads 3–5. Compare every scenario with
  the same route on the unmodified game (mods off, 2P) as a control.
- A stuck run at low FPS with only IRQ/NMI PCs: sample the native stack with
  `gdb -p <pid> -batch -ex bt` to see whether it loops in an interrupt.
- Test saves: SRAM records at `$32/$45/$58` (19 bytes) with a 16-bit checksum
  at `$30`; byte 17 is the Continue stage id (`$0D` = 1-01, bosses `$67-$6E`).
  Generate one file per scenario to start directly at a stage or boss. Saves
  stay local; never commit them.
- Read a full screenshot grid (several frames) before trusting a single dump;
  sprites blink and labels cover the playfield during stage intros.

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
- The developer runs `build-lle/SuperBomberman5SNESRecomp.exe` (desktop
  shortcut). Changes are invisible until `./tools/build-windows.ps1` rebuilds
  it; say so when handing over.
- Preserve each file's line endings (several sources/docs are CRLF) and never
  overwrite the user's working tree blindly: compare checksums with the last
  delivered version first.

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
