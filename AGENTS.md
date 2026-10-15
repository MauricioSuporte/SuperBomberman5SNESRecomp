# AGENTS.md — Super Bomberman 5 Recompiled

Guide for AI agents and future prompts working in this repository. Read it
before changing code, docs or releases. Humans: see README.md and CONTRIBUTING.md.

> Resumo (PT-BR): port não oficial do Super Bomberman 5 (SNES, JPN HiROM) feito
> com SNESRecomp. Código do jogo é gerado localmente a partir da ROM do usuário
> e nunca é versionado. O trabalho real do projeto está em `src/` (glue e mods
> nativos), `recomp/` (análise) e na documentação bilíngue. Siga as regras
> abaixo; para mods use a skill `super-bomberman5-mod-development`, para
> Git/versões/releases use `super-bomberman5-repository`.

## 1. What this project is

- Unofficial, noncommercial PC port of **Super Bomberman 5** built on
  [SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp) (submodule
  `snesrecomp/`) and the ImGui launcher `recomp-ui/` (submodule). Architecture
  follows Mega Man X Recompiled: one game repo, framework pinned as submodules.
- **Hybrid port, partial recompilation.** Only ~9 routine variants are AOT;
  reset/NMI and almost all game logic run on the framework's 65816
  interpreter. PPU, APU/SPC700, DMA are emulated. Never describe this as
  "fully recompiled" or claim full-game, multiplayer, netplay or broad Linux/macOS support.
- Reference ROM: file `Super Bomberman 5 (USA).sfc` but the header says
  **JPN** (`Hu SUPER BOMBERMAN 5`), Fast HiROM, 4 MiB, 8 KiB SRAM.
  `rom_identity.txt` is the single source of truth (hashes, `game_id =
  superbomberman5-jpn`). Hashes decide compatibility, never filenames.
- Owner/public remote: `MauricioSuporte/SuperBomberman5SNESRecomp`. Windows x64
  is the qualified platform; releases since v0.4.4 also offer an experimental Linux x86_64
  setup AppImage (Ubuntu CI and limited WSL evidence in docs/STATUS.md). Current version lives in `VERSION`.
- The maintainer communicates in Brazilian Portuguese. Reply in PT-BR unless
  asked otherwise; code, commits and code comments are in English.

## 2. Hard rules (never break)

1. **No game data in Git or releases.** Never commit or package the ROM,
   `src/gen/`, `recomp/funcs.h`, `bank*_v2.c`, executables built with generated
   code (`build-*/`), extracted graphics/audio/text, saves (`saves/`, `*.srm`),
   save states, memory dumps, screenshots/captures, `rom.cfg`,
   `last_run_report.json` or crash reports. `.gitignore` plus
   `tools/audit-publication.py` enforce this; never bypass with `git add -f`
   or by widening the allowlist without a documented provenance review.
   The single reviewed exception is the maintainer-requested README image
   `docs/images/five-player-showcase.png` (see docs/images/README.md); it is
   stripped from release packages.
2. **Never hand-edit generated output.** Change `recomp/symbols.toml` (or
   hand-written parts of `recomp/bank*.cfg`) and regenerate. The block between
   `>>> BEGIN symbols.toml` and `<<< END symbols.toml` is rewritten on regen.
3. **No stubs with invented values** in `src/gen_stubs.c`: an unresolved
   routine stays on the interpreter instead (framework principle "No stubs").
4. **Mods use only cartridge data at runtime or authored data.** Fonts/glyphs
   in `ptbr_mod.c` and cursor hints in `selection_color_mod.c` are drawn from
   authored bitmaps or derived from VRAM/ROM at runtime — never embed ripped
   pixels, palettes or text dumps in source.
5. **ROM patches are in-memory only** (see `normal_coop_mod.c`): reserve and
   save original bytes, install only while the feature is enabled, restore on
   reset. The ROM file on disk and the SRAM layout are never changed.
6. **Keep LLE.** `SNESRECOMP_FRAME_IMPL=LLE` is the validated compositor;
   HLE is unqualified. Keep reset/NMI `emit = false` until AOT continuation is
   fixed and validated (an earlier promotion caused `interp_cap` at
   `$C02346/$C00607`, dispatch abandonment at frame 56 and BRKs).
7. **Honest claims.** Building ≠ working. Separate "builds/tests pass" from
   "validated at runtime" and say exactly which route/frames were exercised.
   Record evidence and limits in `docs/STATUS.md`.
8. **Respect the user's working tree.** It often has uncommitted work. Stage
   explicit paths, never `git add -A` blindly, never reset/checkout files you
   did not change, never force-push, move a published tag or overwrite a
   release asset without explicit authorization for that exact action.
9. **Bilingual docs stay in sync.** When player-facing behavior changes,
   update both the English and Português sections of README.md (and the
   per-feature doc in `docs/`).

## 3. Repository map

| Path | What it is |
|---|---|
| `src/main.c` | Desktop host shim: title identity, `num_players = 5`, SRAM path. Host itself is the framework's. |
| `src/game_rtl.c` | **The port.** Frame loop (beam-aligned driver), multitap init, ordered mod hooks, reset and save-state extension wiring. |
| `src/ptbr_mod.c` | PT-BR menu overlay (presentation plugin). Labels come from `translations/ptbr/labels.json` → generated header `ptbr_labels.h` via `tools/generate-ptbr.py`. Also owns the final PPU draw and an observer chain. |
| `src/normal_character_mod.c` | Opens Battle's character picker after Normal file confirmation using an in-memory snapshot of the suspended campaign. |
| `src/selection_color_mod.c` | L/R palette cycling in the picker + authored "L+R" cursor hint drawn into borrowed OBJ tiles, restored at line 224. |
| `src/subordinate_mod.c` | Subordinate Bomber (index 9) in the picker's X cell: in-memory patches + render-only grid figure/HUD alive face from its ROM sprite tiles, authored per-color seat portraits and a palette-indexed defeated HUD face, all in borrowed OBJ tiles. Overlays chain after the color hint (`ObserveOverlays` in `game_rtl.c`). |
| `src/normal_coop_mod.c` | 1–5 player Normal Game: 65816 trampolines in unused ROM space, interpreter PC hook, safe-spawn search, shared lives, Start rejoin. |
| `src/host_contract.c`, `src/gen_stubs.c`, `src/variables.h` | Framework contract placeholders (trace hooks, stubs, WRAM names). |
| `recomp/` | Analysis inputs: `symbols.toml` (source of truth), `bank00.cfg`. `funcs.h` is generated. |
| `mods/preloaded/packages/<id>/<ver>/manifest.toml` | Reviewed, default-disabled mod packages staged by `snesrecomp_target_mod_catalog`. Data only. |
| `translations/ptbr/labels.json` | Authored translation metadata (positions, guards, styles). |
| `tests/*.c` | Native unit tests (include the mod `.c` directly with stubbed framework). |
| `tests/test_*.py` | Publication guard, package integrity, UTF-8 release notes (`python -m unittest discover -s tests`). |
| `tests/normal_coop_runtime.py` | Scripted runtime scenarios against a locally built exe + user ROM. |
| `tests/coop_input_fixture.c` | Test-only WRAM mailbox for pads 3–5 (`-DSB5_COOP_INPUT_FIXTURE=ON`). Must stay OFF in normal/release builds. |
| `tools/` | `setup-windows.ps1`, `build-windows.ps1`, `regen.sh`, `audit-publication.py`, `finalize-release.py`, `release-notes.py`, `package-*.py`, `generate-ptbr.py`. |
| `validation/first-stage.input` | Deterministic route title → Zone 1-1 (3,628 frames). |
| `docs/` | `STATUS.md` (evidence log), `BUILD_WINDOWS.md`, `PUBLISHING.md`, one doc per mod. |
| `.agents/skills/` | Agent skills (see §9). |
| `.bootstrap/` | **Ignored** local lab: research scripts (`dis-hi.py` HiROM disassembler, route generators), captures, package tests. Never publish; fine to use and add scratch here. |
| `.tools/` | Ignored portable toolchain (CMake, Ninja, Clang 22, SDL3, Rust 1.97.1). |
| `snesrecomp/`, `recomp-ui/` | Pinned submodules. Do not edit for game-specific needs; fixes go upstream. Pins also in `framework_pins.txt`. Since v0.5.0 both point to maintainer forks (branch `superbomberman5`) carrying generic multitap seats 3–8 (launcher SNES ceiling, keybinds `[player3]`.., host `SourceP3`/`EnableGamepad3`/`ControlsP3`), pending upstream; see `docs/PUBLISHING.md`. |

## 4. Build, run and test (Windows, PowerShell, repo root)

```powershell
git submodule update --init --recursive
./tools/setup-windows.ps1                      # once; local tools in .tools/
./tools/build-windows.ps1 -Regenerate          # after changing recomp/ or first time
./tools/build-windows.ps1                      # incremental (src/ changes)
./tools/build-windows.ps1 -Run                 # build + run with the default ROM
./play.cmd                                     # launcher with local ROM
```

- Output: `build-lle/SuperBomberman5SNESRecomp.exe` (contains ROM-derived code — local only).
- Native tests (built with `BUILD_TESTING`, default ON):
  `./build-lle/normal_coop_tests.exe`, `normal_character_tests.exe`,
  `selection_color_tests.exe`, `ptbr_render_tests.exe`, `infinite_time_tests.exe`, `subordinate_tests.exe`
  (run each one; the portable toolchain has no ctest.exe).
- Before any commit:
  ```powershell
  python tools/audit-publication.py --history
  python -m unittest discover -s tests -v
  git diff --check
  ```
- From the user's machine via a Linux shell bridge you usually cannot run the
  Windows toolchain; say so and give the commands rather than claiming a build.

### Headless scripted runs (runtime evidence)

```powershell
$env:SDL_VIDEODRIVER='dummy'; $env:SDL_AUDIODRIVER='dummy'
$env:SNESRECOMP_DUMP_DIR='.bootstrap/my-check'
./build-lle/SuperBomberman5SNESRecomp.exe --no-launcher --config headless.ini --script route.input 'Super Bomberman 5 (USA).sfc'
```

Script grammar (framework `host_main.c`): `wait N`, `press a+b [N]`
(`p2:` prefix for P2), `turbo on|off`, `poke <addr> <hex>`, `pokefor`,
`forcepoke`, `until/until16 <addr> ==|!= <hex> [timeout]` (exit 3 on timeout),
`dump <tag>` (scene dump into `SNESRECOMP_DUMP_DIR`), `loadstate N`, `reset`,
`quit`. Run in an isolated copy (fresh SRAM; enable mods via
`mods/preloaded/state.toml` in that copy) — see `tests/normal_coop_runtime.py`
for the canonical harness. Check logs for `interp_cap`, `unresolved-abandon`,
`BRK`, `error`, `fatal`. Keep all captures under `.bootstrap/`.

Start route by hand: Enter at title → Normal Game → 1 Player → New Game →
File 1 (A = keyboard X) → Start on Zone 1 map.

## 5. Runtime model you must understand

- `g_ram[]` is the 128 KiB WRAM (`$7E0000-$7FFFFF` → index `0x00000-0x1FFFF`);
  addresses below are WRAM offsets.
- `g_rom[]` is the **file image**. HiROM: file offset = `((bank & 0x3F) << 16) | addr`
  (so `g_rom[0xbb4e]` is `$C0:BB4E`, `g_rom[0x2130b]` is `$C2:130B`). For a
  CPU address use `SnesRomPtr(pc24)` (writable in-memory cartridge).
- Frame order in `GameRunOneFrame` matters:
  `NormalCharacterBeforeFrame → SelectionColorBeforeFrame → NormalCoopBeforeFrame →
  snes_beam_frame_driver_run_frame → NormalCoopAfterFrame → SelectionColorAfterFrame →
  NormalCharacterFrame` (`SubordinateBeforeFrame` installs its patches first). Draw: `GameDrawPpuFrame → PtbrDrawPpuFrameObserved(ObserveOverlays)`, which runs the color hint then the Subordinate overlays on line 1 and restores them in reverse order on line 224.
- Pads: `RtlGetPadState(slot)` uses the runtime bit layout used across the mods:
  B `0x0001`, Start `0x0008`, Left `0x0040`, Right `0x0080`, A `0x0100`,
  L `0x0400`, R `0x0800`. Five seats: P1 port 1, P2–P5 multitap port 2.
- Interpreter PC hook: `g_interp_bridge_pc_hook(pc24, m, x)` fires per
  interpreted instruction. Always chain the previous hook and restore it on
  reset. **It does not fire for AOT code** — promoting a routine in
  `symbols.toml` can silently break a mod that hooks inside it.
- Save states/rewind: each mod appends an extension in `GameSaveExtra`/
  `GameLoadExtra` (order: Character, SelectionColor, Coop — append new mods at
  the end, in the same order in both). Use a magic, validate every field,
  call `RtlStateStreamFail` on bad data, keep the size constant (rewind ring
  sizes slots from the first capture), and accept older states via
  `RtlStateBytesRemaining(sli)==0`.

## 6. Known WRAM/ROM facts (verified on the JPN image)

| Address | Meaning |
|---|---|
| `$1031` | Top-level mode: `2` title/player-count menu, `3` Normal file menu, `4` Battle character picker, `8` and `$0D–$73` campaign (also check `$1D1A` bit0 clear) |
| `$1032` | Sub-mode (`5` during picker exit range) |
| `$0C90/$0CB8/$0CE0` | Script task 9 PC: three parallel byte arrays (lo/hi/bank), not CPU registers |
| `$2000-$2008` | Script return addresses (menu/file confirm detection) |
| `$1038` | **Count** of confirmed picker seats (not a bit mask); reused during picker init |
| `$103A` | Number of picker seats |
| `$625F+i` / `$6264+i` | Chosen character 1–9 (10 = Subordinate Bomber with its mod) / palette 0–5 per seat |
| `$6269+i` | Seat kind for picker (1 human, 2 CPU/off as used by mods) |
| `$62AB+pad` | Controller → current seat (CPU hand-off after confirm) |
| `$62B5+i` | Seat state: `0` pending CPU, `1` active, `FF` confirmed |
| `$62BF+i`, `$62C9/$62D3+cell` | Portrait object ids; grid cell character / color |
| `$16C2+i` / `$16DA+i` | Stage character index / palette id = `ROM[$C0:BB4E+char] + color` |
| `$1D25` / `$1D24` / `$1D23` | Participation mask / 1-or-3 player flag / shared lives |
| `$1D1A` | Mode flags (bit0 set ⇒ not a campaign stage; bits `$0C` ⇒ boss spawn layout) |
| `$1052` / `$1152` | Static / dynamic collision maps, 16 columns, playfield x 1–13, y 1–11 |
| `$17FA+i`, `$15A2+i`, `$16AA+i` | Player object / status (`$80` dead) / collision cell |
| `$4E8D+i` / `$4EA5+i` | Spawn tile x / y |
| `$2910` | CGRAM shadow; set `$0351=1` after changing it |
| `$0349`, `$06DF`, `$2B10/$2B18` | Palette slot refcounts, object→slot, slot palette id / owner |
| `$C6:8E68` | Picker ready (accepting A) · `$C6:89CF` picker init · `$C7:1257` Normal file-confirm fade · `$C7:1014` file menu init |
| `$C4:2A69` | Player actor spawn allocator · free in-memory patch areas used by coop: `$C2:4300`, `$C2:4340`, `$C4:5B00`, `$C5:7000`, `$C5:7100` |
| `$C0:BB30/BB3A/BB44/BB4E` | Per-character tables, 10 entries (index 9 = Subordinate Bomber: sprite set `$19`, palettes `$48-$4D` colors, `$4E` effect, `$4F` rival). Palette block per character: base+0..5 colors, +6 effect (`$C4:46AD`, 90 frames), +7 rival (`$C4:3CCF`); Bomberman has no +7. `$C4:3CB5` creates rivals; Zone 3-D (`$6A`) uses types 4 and 9. Verified by Zone 3-D dumps and stage runs as index 9 |
| `$C6:8AF4-$8B2E` / `$C2:122A` | Picker script fills grid cell 9: X (char 0) when the secret slot is locked (`$C6:8B17`), char 1 color 5 when unlocked. Grid drawer: BG arrangements (set 6) only for chars 1-9, X = `$90`; figure at BG1 tile (`$C2:12D2[cell]+$C2:12E6[c]`, `$C2:12DC[cell]+$C2:12EF[c]`) |
| `$C2:1470` / `$C5:12FE` | HUD face animation ids (set 6) normal / defeated, 9 entries each (index 9 reads code). Face object per seat at `$7E35C5+i`, 16x16 sprite 17 lines above it, player's palette. Battle Time Over pose of a bomber: its own set, anim `$1F` |
| `$C2:4207-$7FFF` | `$FF` padding in bank C2; Subordinate mod uses `$C2:6000-$603F`, coop `$C2:4300-$434B` |
| `$0348` | OBJ palette-slot request mask for the next allocation (`$C0:028F`); players use `2<<player` (set at `$C4:2AD8/$C4:2B8D`), enemies `$38` |
| `$1D00-$1D0F`, `$1D10` | Sprite page owners (object id, `$FF` free, `$7F` pending free, `$7E` static) for 1 KiB VRAM pages at `$6000+$200*i`; `$1D10` = pool size (5 in campaign stages; `$C5:6A00` sets 8). Allocator `$C5:6AF5`, upload `$C5:6B77`, request queue `$1CCC-$1CF4` |
| `$0E1C/$0E1D/$0E1E` | Timer frames/seconds/minutes; seconds >=60 skip countdown at `$C5:046B`. Native 99/9 selects dash HUD at `$C5:0C00`. Verified by JPN disassembly and Windows/LLE Zone 1-1 forced-deadline runs. |
| `$C5:066C` | Per-frame HUD selector: picks the two/P1/P2 HUD archive (`$E6/$E4/$E5`) from P1/P2 state and writes `$1D25/$1D24` |
| `$C2:0C9F`, `$C5:6195/$C5:61C6`, `$C1:6BA1`, `$C0:373F` | P1/P2-only `CPX #$02` loops: script op `$12` (`$C2:0C67`, players on tile → exit warps/switches), floor gimmicks, cell hold, point damage (`$C0:374D` = first enemy actor). Coop raises them to the stage roster. Verified by warp runs with P3–P5 |
| `$1031` / `$1046` | Campaign stage id (`$0D` = 1-01 … `$48`, Zone 5 `$49-$70`, bosses `$67-$6E`: `$67` = ZONE1 - A) / Continue stage saved to SRAM record byte 17 |
| `$C2:16B8/$C2:16EE`, `$C4:3CCF` | Script actor creation at slot operand−1; bosses create rival bombers in slots 2 (and 3 in `$6A`). Bomber tables `$0FDE/$0F9B/$0FEA/$0FEF/$0FF4/$0FF9/$101F` hold five slots; slot 5 jumps into data. Boss choreography `$C5:2600-$C5:39FF` uses literal slots 2/3 |
| `$0300` / `$67` | INIDISP shadow / NMI busy latch (`$C0:F3D4`) |
| `$1FF4-$1FFF` | Enemy→player distance arrays (6 × 2 entries: dx, dy, dx hi, dy hi, dir codes) from `$C3:46CC`; coop relocates to 6 × 5 at `$1FC0-$1FDD` (+`$C3:FF70` seat loop). Verified by contact runs on P1/P3 |
| `$C5:3D3F` | Copies stage collision map to `$1052` after player spawns; coop settles P3-P5 at its RTL `$C5:3D5D` |
| SRAM `$30-$6A` | Checksum (16-bit sum of `$32-$6A`) and three 19-byte Normal records at `$32/$45/$58`: 16 flag bytes (stage N → byte N>>3 bit N&7), cleared count, Continue stage, `$6258` |

When you identify a new address, document it here (and optionally name it in
`src/variables.h`), with how it was verified.

Timer research: `$0E1C/$0E1D/$0E1E` hold frames/seconds/minutes. Seconds >=60 skip countdown at `$C5:046B`; native 99/9 gives the dash HUD at `$C5:0C00`. Verified by JPN disassembly and Zone 1-1 forced expiration tests.

## 7. Workflow for a change

1. Read `docs/STATUS.md`, the relevant `docs/*.md`, and `git status`.
2. Research in `.bootstrap/` (disassemble with
   `python .bootstrap/dis-hi.py C68E68 80` from repo root, scripted dumps).
   Confirm behavior on the real ROM; don't guess addresses.
3. Implement in `src/` following §5 and the mod skill. Add/extend a native test
   in `tests/` and register it in `CMakeLists.txt`.
4. Build, run native tests, run a scripted route that exercises the feature
   (with PT-BR on as well when menus/rendering are touched).
5. Update docs: feature doc, `docs/STATUS.md` (evidence + limitations, dated
   section at top), README (EN + PT), `mods/preloaded/...` manifest and
   `tools/package-*.py` when adding a mod, `tools/release-notes.py` when
   release scope changes.
6. Run the pre-commit checks (§4). Commit with an English message describing
   resulting behavior, plus the attribution lines your environment requires.
7. Publishing/version bumps/releases: follow
   `.agents/skills/super-bomberman5-repository/SKILL.md` and `docs/PUBLISHING.md`.

## 8. Pitfalls already paid for

- Windows console pipelines corrupt UTF-8 release notes (U+FFFD). Use
  `tools/release-notes.py --output <file>` and send UTF-8 bytes.
- PT-BR title text was clipped by the game's HDMA window hiding "Start";
  respect the original top margin. Screens reuse tile IDs with different
  content — guard labels by context before drawing anything.
- Normal's character picker: `$1038` is reused during init; only observe
  confirmations after `$C6:8E68` is reached.
- Picker previews of the same character share a palette slot; isolate the
  five portrait slots without stealing other owners' refcounts.
- Objects on the same animation frame show the same VRAM tiles (two seats on
  one character; index 9 even reuses Bomberman's portrait/HUD frames). Never
  repaint native tiles for one seat: draw into borrowed free OBJ tiles and
  repoint only your own sprites (`subordinate_mod.c`).
- `0x...e+N` in C parses as a float exponent: write `g_rom[0xbb4e + i]`.
- Final picker cleanup restores stock CPU colors; keep choices pending until
  the stage initializer consumes them (`$16C2/$16DA` match).
- P1/P2 mount backup actor slots overlapped P3/P4; coop moves mounts to 5–9
  and NPCs to 10+ (only 14 NPC slots remain).
- The five-player menu forces the accurate compositor for that frame because
  `NewRenderer=1` showed only two rows.
- Title script return slots can retain attract-mode `$C7:1B3B`. Detect the
  Normal player menu from mode 2 and both native BG3 tile pairs at VRAM word
  `$424B/$424C` (302/261) and `$428B/$428C` (278/261), before overlays. Verified
  against translation metadata, an interactive launch and scripted title runs.
- The development exe in `build-lle/` must never be shipped; releases are a
  clean-checkout **setup host** (`SNESRECOMP_SETUP_HOST=ON`, no `src/gen/*.c`).
- Mod package version (0.1.0) ≠ host version; a `.snesmod` cannot add native
  code to an older executable.
- The cartridge is written for two seats. Exit warps (`$C2:0C9F`), damage,
  floor gimmicks, enemy contact (`$1FF4` two-entry arrays) and boss
  choreography (`LDX #2`, `LDA $17FC`) all needed explicit five-seat handling;
  each fix is listed in §6 and docs/NORMAL_COOP.md. Check new features for the
  same assumption with P3–P5, not just P1.
- Bomber tables hold five slots. Boss rivals use them, so five players cannot
  all fight rival-bomber bosses; slot 5 jumps into data and freezes the game.
- Player spawns run before the stage collision map exists; anything reading
  `$1052` at spawn time sees zeros (P3/P4 appeared outside smaller arenas).
- Patch immediate operands at opcode+1. A one-byte offset error at
  `$C3:4766` corrupted the following branch and crashed every enemy update.
- Back (B) from the Normal character picker leaves a black screen in headless
  LLE runs (script stops at `$C7:108D`, NMI latch `$67` set); it predates
  v0.5.0 and is unresolved.
- Framework submodules point to maintainer forks for the five-seat launcher;
  push fork commits before updating gitlinks (see docs/PUBLISHING.md).
- The developer plays `build-lle/SuperBomberman5SNESRecomp.exe` through a
  desktop shortcut; source changes appear only after `./tools/build-windows.ps1`.

## 9. Skills in this repo

- `.agents/skills/super-bomberman5-mod-development/` — creating, debugging and
  validating native mods/features (RAM research, plugin pattern, overlays,
  in-memory ROM patches, save-state extensions, tests, docs).
- `.agents/skills/super-bomberman5-repository/` — Git hygiene, versions,
  bilingual docs, publication audit, setup-pack packaging, GitHub releases.

Framework references: `snesrecomp/docs/MOD_PACKAGES.md`, `FRAME_MODEL_HOSTS.md`,
`MULTITAP.md`, `LLE_FIRST_ANALYSIS.md`, `MULTI_TIER.md`, `GAME_PROJECT_SETUP.md`.
