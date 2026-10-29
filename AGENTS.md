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
  "fully recompiled" or claim full-game or broad Linux/macOS support. Netplay
  exists since v2.0.0 (delay-sync, docs/NETPLAY.md); the maintainer tested a
  real two-player internet match with the Discord integration successfully
  (2026-10-23). Three or more peers remain unqualified; never claim more.
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
   save states, memory dumps, raw diagnostic captures, `rom.cfg`,
   `last_run_report.json` or crash reports. `.gitignore` plus
   `tools/audit-publication.py` enforce this; never bypass with `git add -f`
   or by widening the allowlist without a documented provenance review.
   Exception authorized 2026-10-15: gameplay screenshots may be published in
   `docs/images/` for README/docs/releases, including `five-player-showcase.png`.
   Extracted sprites/tiles/palettes/audio remain prohibited.
2. **Never hand-edit generated output.** Change `recomp/symbols.toml` (or
   hand-written parts of `recomp/bank*.cfg`) and regenerate. The block between
   `>>> BEGIN symbols.toml` and `<<< END symbols.toml` is rewritten on regen.
3. **No stubs with invented values** in `src/gen_stubs.c`: an unresolved
   routine stays on the interpreter instead (framework principle "No stubs").
4. **Mods use only cartridge data at runtime or authored data.** Fonts/glyphs
   in `ptbr_mod.c` and cursor hints in `selection_color_mod.c` are drawn from
   authored bitmaps or derived from VRAM/ROM at runtime — never embed ripped
   pixels, palettes or text dumps in source. Data from another game (the
   Bomber Kings mod's Super Bomberman 4 frames) follows the same rule: read at
   runtime from the player's own ROM, verified by hash, never stored.
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
| `src/portal_labels_mod.c` | Read-only Normal portal labels, from native stage scripts and live gate/OAM visibility; authored PT-BR title font shared via `menu_font.h`, no persistent campaign state. |
| `src/game_rtl.c` | **The port.** Frame loop (beam-aligned driver), multitap init, ordered mod hooks, reset and save-state extension wiring. |
| `src/ptbr_mod.c` | PT-BR menu overlay (presentation plugin). Labels come from `translations/ptbr/labels.json` → generated header `ptbr_labels.h` via `tools/generate-ptbr.py`. Also owns the final PPU draw and an observer chain. |
| `src/normal_character_mod.c` | Opens Battle's character picker after Normal file confirmation using an in-memory snapshot of the suspended campaign. |
| `src/selection_color_mod.c` | L/R palette cycling in the picker + authored "L+R" cursor hint drawn into borrowed OBJ tiles, restored at line 224. |
| `src/subordinate_mod.c` | Subordinate Bomber (index 9) in the picker's X cell: in-memory patches + render-only grid figure/HUD alive face from its ROM sprite tiles, authored per-color seat portraits and a palette-indexed defeated HUD face, all in borrowed OBJ tiles. Overlays chain after the color hint (`ObserveOverlays` in `game_rtl.c`). |
| `src/extra_characters_mod.c` | Extra-character core, plugin of the **Super Bomberman 5 Extra Characters** mod (also turns on `subordinate_mod.c`): Honey/Kotetsu (secret-stage bosses, choices 11/12) and Emperor Terrorin (ZONE5 - J on foot, choice 19) in a 7x2 picker (7x3 with Bomber Kings, choices 13-18; 6x3 with Bomber Kings alone; empty cells skipped): relocated cell arrays (`$7F:F800/$F814`), lookup-table cursor movement, Bomberman gameplay, render-only skins (object set `$0D` from `$CC:289D`, set `$0E` from `$CC:657D`) replacing the bomber's sprites, HUD face and seat portrait; tinted boxes through the color window (`ExtraTintBox`). Exposes `PickerCellCharacters()`/`PickerCellColors()` for the other picker mods. |
| `src/bomber_kings_mod.c`, `src/sb4_rom.c` | Bomber Kings: switches the extra-character core to its three-row picker; Super Bomberman 4 frames/palettes read at runtime from the player's own SB4 ROM, selected on the launcher Mods page (manifest `[[external_rom]]` `sb4-rom`, `normalized_sha256`, `required`; path via `snes_mod_runtime_resource_path_c`). Never embed or package SB4 data. |
| `src/sb5_sound.c` | Super Bomberman 5 stage sound effects missing from the loaded sound set (Terrorin's barrier `$0288` in Battle): the cartridge's own driver and sets uploaded into a private APU like `$C0:0E31`/`$0E9C` (tables `$C5:F69B`, `$DA:C2E0`), command from `$C0:86D0` sent like `$C0:0E7F` (bit 7 toggle, rewritten until the echo `v^$80`: the driver clears its input ports); state of the boss's phase 3 = boot (`$58`,`$56`, set 0, cmd `$01`) + stream `$01` + `$0D/$26/$3F` + set `$18`, rendered to PCM for the mod mixer. Sending a command the set lacks hangs the game's driver. |
| `src/sb4_sound.c` | Super Bomberman 4 ability sound effects: SB4's sound driver uploaded from the player's SB4 ROM into a private APU (runner SPC700/DSP), effects rendered to PCM clips for the host mod mixer. Nothing from the game's own APU is touched. |
| `src/special_abilities.c` | B + direction abilities and SB4 debuff for Honey/Kotetsu/Terrorin/Bomber Kings: pre-opcode hooks at `$C0:0AF9` (pads) and `$C4:3180` (walking steps), native damage rules, SB4 timelines and graphics (fire, mace, laser, ability poses via `sb4_rom.c`), stun/item loss via `$15BA` bit 5 and `$C4:37CD`, Terrorin's barrier (`$C4:2CFE` fire-cell hook) and Clock (`$C4:398C` lost-item flight with `$8D` = seat and `$1CC7` = 0, `$C5:42D3` pickup hook, `$C4:4503` paralysis), state in WRAM `$7F:F840-$F8F0`. |
| `src/normal_coop_mod.c` | 1–5 player Normal Game: 65816 trampolines in unused ROM space, interpreter PC hook, safe-spawn search, shared lives, Start rejoin. Regular-stage spawns: P3/P5 beside P1, P4 beside P2. |
| `src/coop_corner_spawns_mod.c` | Plugin of the **5 Players → corner spawns** mod (`superbomberman5.coop-corner-spawns`): switches coop regular-stage spawns to P3 lower left, P4 lower right, P5 centre. Flag only; logic stays in `spawn_tile()`. |
| `src/infinite_lives_mod.c` | **Cheats → infinite lives**: captures `$1D23` before the retry/respawn charges (`$C5:3B1C`, `$C5:0659`, wrapping co-op's charge) and restores it before the Game Over test (`$C5:3B2D`/`$C5:065C`). Must be installed after co-op. |
| `src/com_level4_mod.c` | **Cheats → Battle COM level 4**: keeps the Battle COM level limit `$6247` at 4 (as password 1103) while enabled; WRAM only. |
| `src/battle_variants_mod.c` | **Gameplay → Battle stage variants**: A on the arena selector opens a drawn list (Original, Variants 1–4 = passwords 4622/0413/0926 and the 0110 item rule where the stage script has it); writes `$6274` bits / flag 3, masks `$6273` until the stage starts, re-injects A through `$C0:0AF9`. Minimaps decoded once from the ROM; drawn over the finished frame. docs/BATTLE_VARIANTS.md |
| `src/sb5_launcher.c` | Launcher links (Discord `https://discord.gg/kH54NCDQB4`, releases), the **Update game** action and the netplay policy (forced delay-sync, Bomber Kings needs each player's SB4 ROM). |
| `src/updater/sb5_updater.c` | ROM-free updater shipped in `updater/`: GitHub `releases/latest`, `SHA256SUMS` check, in-place install that keeps player files; defers to Git checkouts and Quiver installs. docs/UPDATER.md |
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
./SuperBomberman5Recomp.exe                    # launcher with local ROM (copied by the build)
```

- Output: `build-lle/SuperBomberman5SNESRecomp.exe` (contains ROM-derived code — local only).
- Native tests (built with `BUILD_TESTING`, default ON):
  `./build-lle/normal_coop_tests.exe`, `normal_character_tests.exe`,
  `selection_color_tests.exe`, `ptbr_render_tests.exe`, `infinite_time_tests.exe`, `subordinate_tests.exe`,
  `pause_menu_tests.exe`, `portal_labels_tests.exe`, `extra_characters_tests.exe`
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
`BRK`, `error`, `fatal`. Keep raw captures under `.bootstrap/`; selected gameplay screenshots for
README/docs/releases may be copied into `docs/images/`.

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
| `$C6:8E68` | Picker ready (accepting A) · `$C6:89CF` picker init · `$C7:1257` Normal file-confirm fade · `$C7:100E` full New Game file menu entry |
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
| `$71-$73` | Secret stages `ZONE1/2/3 - S`: Honey and Kotetsu boss actors (`$16C7/$16C8` = `$3C/$3E`), object set `$0D` (anims `$1E-$22` Honey, `$2B-$2F` Kotetsu: R/D/L/U/spin). Reached from Zone 1-11, 2-11, 3-7 exits |
| `$C0:171D` / `$C0:16CC` | Per object set (`$04FF`): 24-bit animation table (16-bit pointers to `frame, duration` pairs; `$07FF` = offset of the current pair, `$079F` = anim id, `$049F` = frame) / frame table (count + 7-byte pieces `dx16 dy16 tile16 attr`, attr bit0 = 16x16). Bomber set `$10`: anims 1-4 walk R/D/L/U, `$46-$49` stand |
| `$C0:1B04` / `$C0:1CB0`-`$1CCC` | Graphics archive loader (set `$0AE7`, entries `[vram, size, src24, flags]`, flag bit7 = raw) / decompressor: control = mode bits 5-7 (literal, prev+1, prev, copy -2/-4/-16/-32/-64 in a 256-byte ring at `$0200`) + count-1 bits 0-4. Honey/Kotetsu archive `$CC:289D` (`$1E00` bytes, ends `$CC:40E4`), palettes `$C1:8FC6/$C1:8FA6` |
| `$C6:8C02/$8C78/$8CEE/$8D62` | Picker cursor right/left/up/down blocks (`$8CEE` moves up, `$8D62` down; verified by three-row routes) (`$1034` = cell+1 = choice). Exit script `$C6:A575` copies the cell character into `$625F` and keeps the cell in `$6291`. Cursor positions `$C0:C927` with `$C0:C976/$C980` (10 entries) |
| `$C4:2A3C` / `$C4:3BDE` | Battle stage init `$16C2 = $625F - 1` / score board (script `$C6:7736`, FF.59) stores `$625F - 1` again. Honey/Kotetsu core patches both and uses `$C2:6088-$60B0` plus tables `$C2:60C0-$612F` |
| `$C5:4195` / `$7E4ED5+i` | Item effect jump table (item id at `$145A+cell`; revealed items have `$1052=$10`, `$1152=$80`) / ability bits: 1 remote (`$04`), 2 (`$0B`, with `$167A` bit 6), 4 power glove (`$0C`), 8 boxing glove (`$11`, Y), `$10/$20/$40` bomb types. Items `$12/$13` set `$176A` (not eggs; the egg is `$15`). Verified by placing items with pokes in Battle |
| Bomber anims (set `$10`) | 1-4 walk, `$15` cheer, `$16` death, `$17-$1A` airborne, `$1F` Time Up, `$20` victory, `$21` stun, `$27-$2A` punch, `$2B-$2E` lift, `$2F-$32` carry walk, `$33-$36` throw, `$37-$3A` hold, `$42-$45` Bad Bomber cart, `$46-$49` stand (R/D/L/U) |
| `$7E2EEA/$7E2F4A` + object, `$1782+i` | Object's current frame pointer (16-bit + bank; OAM shown can lag it by 1-2 steps) / bomber mount state: bit 7 riding, bit 1 jumping onto the mount. Mount is its own object (set `$01`) in the rider's palette slot. Egg item `$15`. Verified by Battle egg routes and OAM/frame matching |
| `$D6:D871`, set 6 anims `$50`/`$4E` | Battle champion screen archive (`$0D40` bytes, ends `$D6:E0BB`, 16x16 block layout at OBJ tile `$C0`) / Honey (frames `$A2/$A3`) and Kotetsu (`$9E/$9F`) cheering poses; Honey's palette there `$C1:A8AE`. Verified by champion-screen dumps |
| `$7F:F800-$F8F0` | Extra-character core: 20-entry picker arrays `$F800/$F814`, per-seat choice `$F828`, seat color `$F82D`; abilities `$F840-$F8F0` (dazed `$F8EA`, Clock event `$F8EF`). Zero in all research captures without the mods (4873 earlier, 453 in the Terrorin research); no ROM operand points there |
| Stage `$6F`, set `$0E`, `$CC:657D`, `$C1:906E` | ZONE5 - J (Terrorin): phase 3 is actor 5 on foot. Object set `$0E` (walk `$39-$3C`, lift `$3D-$40`, carry walk `$41-$44`, throw `$45-$48`, stand `$4A-$4D` (crouch; the mod stands on the walk's middle step, frames 68/74/80/86), cheer `$4E`, defeat `$50` (frames 152-169, head is piece 0 of 153-165), hurt `$51`, guarded spin `$52`), archive `$CC:657D` (`$1E80` bytes, stream end `$CC:8001`), palette `$C1:906E` (id `$37`). Barrier `$C3:3F2D-$4090`: spin `$40` frames, OR `$0C` into `$1052` at the 21 offsets of `$C3:4091`, color window ±`$28` (`$C1:313C`, fixed-color blue `$0317` 0-16), Clock script `$C2:2629` (A = `$72+$1632`), cells cleared at `$C3:4084`. `$0C` cells kill players (`$1CCA` bit 2 → `$C4:2CF0`). Item 8 (Clock) pickup `$C5:42D3` freezes actors 5-23 and sets `$0E20` (`$F0` in `$6F`, else `$3C0`; stops the Battle timer); Terrorin taking it calls `$C4:3A94` → `$C4:4503` (player paralysis, P1/P2 only). Verified by forced-stage captures, a WRAM write watch and Zone 1-1 pokes |
| `$C0:0AF9`, `$2C/$31/$36/$3B+seat` | End of the pad reader `$C0:0A56` (before `RTL`): held actions (A `$80`, X `$40`, L `$20`, R `$10`, B `$08`, Y `$04`, Select `$02`, Start `$01`; the Battle arena carousel confirms with A or Start), held directions (right 1, left 2, down 4, up 8), new actions, new directions. Remote detonation `$C4:2FF7` tests `$36,X & 8` → `$C5:1F84`. Verified by watch logs and ability routes |
| `$C4:3231/$C4:3180` | Walking pixels this frame (`$1602` += `$15D2`, + `$15EA`; 4 when riding) stored at `$C4:3180` (X = seat) and tested by the next `BEQ`; a pre-opcode hook changing A must set `_flag_Z` too. Verified by debuff speed routes |
| `$7E625D` bit 0, `$6269+i` bit 7, `$15A2+i` bits 3-4 | Team Battle / lower team on the Team Members screen / team at stage start (`$C4:2A1C`: `$18` Free-for-All, `$10`/`$08` teams). `$15A2` bit 2 + `$170A` = native player invulnerability. Verified by Team Battle routes |
| `$182A/$1842`, `$1872/$188A` + actor | Actor world x/y (lo/hi), cell = (pos + 8) / 16; actors 0-23, `$17FA` bit 7 = none; enemy point damage `$C0:373F` (`$1662` HP, `$085F` invulnerability). Verified by ability damage routes |
| `$101F+i`, `$C4:0164`, `$C4:3DA6` | Rival-bomber flag of boss stages `$67`-`$6E` (slot 2, `$6A` also 3): their update clears `$15A2` bit 6 every frame, only a flame on their cell (`$C4:2CFE`, `$1CCA` bit 2) hurts them (Louie, then HP `$1662`). Death: `$C4:3DA6` decrements `$1D50`; with `$102F` = `$6A` the last death starts the gates (`$7E4DA3` queue, `$C5:3B60` → `$C2:265C` retried each frame). Verified by watch/stack logs on forced `$6A` |
| `$C0:18FC`, `$7E30AA/$30EA/$314A` | Static OBJ archive allocation for created objects (VRAM block, archive id, refcount; `$C0:1A0A` fails when no block fits). A Normal HUD face (archive `$64`) blocked ZONE3 - D's gate archive. Verified by stack logs |
| `$0300` / `$67` | INIDISP shadow / NMI busy latch (`$C0:F3D4`) |
| `$1FF4-$1FFF` | Enemy→player distance arrays (6 × 2 entries: dx, dy, dx hi, dy hi, dir codes) from `$C3:46CC`; coop relocates to 6 × 5 at `$0100-$011D` (+`$C3:FF70` seat loop, run natively by a pre-opcode hook). `$1FC0-$1FDD` is NOT free: `$1FAB,Y` is the flame-thrower's segment-object table (ZONE5 - 27), `$1FBB,X` an actor array. Stack never below `$01BB` in 1263 dumps. Verified by contact runs on P1/P3 and a native/interpreted shadow check |
| `$C1:0015` / `$C1:0035` | OBJ/BG palette save (`$2910`→`$2CEA`) / restore (blink, flash), `MVN` with A=`$20` (operands `$C1:002A/$004A`) = 33 bytes: palette 15 overruns into `$2B10` (slot 0 palette id) and `$2EEA` (object 0 frame pointer). Co-op patches both to `$1F` (32 bytes) so seats can use OBJ slot 7. Verified by WRAM watch on forced `$6F` |
| `$1CBF` | Bomber-slot count AND mount offset (stock 2/2, Battle 5/5 via `$C4:2A60`). Count readers: `$C5:031C/$033D` survivors, `$C4:44DE/$4C94/$5585` player-on-cell, `$C5:1A14` bomb owner, `$C4:2815/$42D4`, `$C5:2652/$265C`, `$C5:60A4`. Offset readers (ADC/SBC/loose-mount bounds/boss backups, 28 operands): co-op points them to `$011E` = 19 and keeps `$1CBF` = 5. Verified by disassembly of every `$1CBF` operand and five-seat egg/platform routes |
| `$C0:01C8/$01DE`, `$C0:0379` | Palette load: table word `$C0:0379 + 2*$0AE7` (bank `$81`), entry word `[table + 2*(id-1)]`, colors at entry+4, `$AB` colors to `$2910 + 2*$AE` (`$AE` = `$80 + slot*16` for OBJ), `$0351` = 1. Players: `$0AE7` = 0, 9 colors (`$C4:37B0`) |
| `$085F+object` | Blink counter: the OAM pass `$C0:146F` decrements it and skips odd values. Co-op sets 2 to hide an object for the one frame its own page gets different tiles (`$C5:7140` deferral); requests for the chunk already in the page (tracked at `$C5:6B77`, Y = object) skip the upload via `$C5:7157` |
| `$C1:6B32`, `$C4:3C32/$3C4F` | Moving-platform carry (ZONE5 - 25 shuttles, `$1052` = `$08`): `CPX #$02` seat loop, player-on-cell test, object x move. Co-op raises it to the roster. Verified by teleporting P3 onto the platform |
| `$C0:18FC`, `$C0:193A/$193C`, `$C0:1342`, `$C0:196D` | Static OBJ archive allocator (`$0ADF` = archive) / its failure exits (no free entry / no VRAM block from `$C0:1A0A`) / native object delete (A = object; releases `$06E0` archive entry and `$06DF` palette) / archive entry release (refcount `$7E314A`). Gate archive `$63` in ZONE3 - 1; HUD-faces mod hooks the failure exits and frees its faces through `$C0:1342` from FF padding `$C5:7300-$735F`; a failure with a live face redirects to `$C5:7360` (drop faces, `JML $C0:18FC` retry) because ZONE5 - 3's gate archive `$3A` is loaded once at stage start. Both call the drop at `$C5:73C0-$743A`. `$C0:1342` also releases `$06DF` through `$C0:0345` (negative slot = none); a face shares its bomber's palette slot with no reference of its own, so the drop sets `$06DF=$FF` first, or P1/P2's slots read as free (`$2B10` id/`$0349` count 0) while still in use. Verified by ZONE3 - 1/ZONE1 - 1 forced routes, vest routes and disassembly |
| `$C0:0295`, `$0346/$0347/$0348`, `$C0:0303`, `$2B10/$2B18/$2B20`, `$0349` | OBJ palette allocator: palette id / slot count / slot mask; reuse a masked slot with the same id and set, else `$0347` consecutive free slots (`$2B10` = 0), else fail at `$C0:0303` (`LDA #$FF`). Slots: 0 stage (`$58`), 1/2 P1/P2, 3-5 enemies (mask `$38`), 6-7 large stage objects (mask `$C0`: ZONE5 - 1 tower `$38`, two slots). Final boss `$6F` palette `$36` takes 3-6. Co-op hooks `$C0:0303` (seat 3-5 slots shared into P1's) and `$C4:37B0` (per-object palette writer). Verified by allocator logs on forced `$49`/`$6F` |
| `$C4:53EB`/`$C4:53F0` | Mount palette: slot = `$16DA` of the mount (rider seat) + 1, then the mount colors at +8 of that slot. Seats 3-5 landed on enemy slots 3-5; co-op redirects to the rider object's slot (`$C4:5B40`). Verified by egg routes with five seats |
| `$C3:0458`, `$C4:2256/$2260` | First enemy actor (`LDA #$05`) / actor allocator from `$A5` up to `CPX #$18`. Mounts are allocated with `$A5` = rider + `$1CBF`. Co-op keeps enemies at 5 and caps the search at 19 (mounts 19-23). Stage scripts reference enemies by absolute actor: ZONE5 - 1's tower core script (`$C6:CE94`-`$CEEE`, script tasks 9/`$0B`) attaches to actor 9. Verified by forced `$49` routes |
| `$C5:6AE3`, `$C5:6B77`, `$1CCC/$1CEA/$1CD6` | Page request service / upload (fixed `$0400` bytes from sheet entry `$85:E22D + (archive-1)*8` bytes 4-6, + `$1CEA` * 512) / queue archive, half, object. With five seats all five pages are owned by players; co-op records refused requests (`$C5:714E`) so the overlay draws those objects from the sheet. No 1 KiB of OBJ VRAM is free in campaign stages (audited on `$0D`/`$49`) |
| `$C5:3D3F` | Copies stage collision map to `$1052` after player spawns; coop settles P3-P5 at its RTL `$C5:3D5D` |
| `$1D1D`, `$0AEE`, `$C5:00B7/$00C8/$009E` | Native in-stage pause latch / gameplay-ready byte / living-seat Start test and paused poll. Verified by Windows/LLE Normal and Battle pause/resume dumps and disassembly. Pause-menu trampoline uses free `$C5:7180-$719A`; resume sound call `$C5:0096` is suppressed while enabled. |
| `$C2:1AEC`, `$C7:0B8B/$0B97`, `$1036=4` | Native Normal menu entry store / New Game and Continue initialization / direct map state. VM `$0A` already points after the store (`$C7:0B8E/$0B9A`). Verified by Normal map return and reentry with 1/2/5 seats; skips the file menu. |
| `$35B2=1`, `$C0:3D8E`, `$C6:A28D` | Native end-of-match arena return flag (with mode 4 and `$1D1E` bit 2) / native setter / interactive Battle arena selector. Verified by disassembly and Windows/LLE Battle exit/reentry. |
| `$1D1A` bit `$40`, `$C5:3D19` | Completion-record permission: native success exit records `$102F+3` only with this bit. Pause-menu Normal exit clears it, keeping WRAM/SRAM cleared-stage flags unchanged. Verified by disassembly and isolated before/after saves. |
| `$C4:28CE`, `$15A2+i`, `$1BA3+i` bit 1, `$C5:0307` | End of campaign player reset / actor death / Revenge survivor flag / Battle survivor count. Pause-menu restores saved campaign fields at reset return; Force Draw removes all survivors. Verified by native tests and Normal/Battle runtime transitions. |
| `$C0:0CED`, `ROM[$8086D0+id]` | Native SFX command group 2: menu move/accept/back ids 1/4/3 (commands `$21/$24/$23`). Verified against Battle setup scripts and Windows/LLE CPU-to-APU traces. |
| `$C5:6875`, `$7E6241-$6244`, `$7E6274/$6273` | Password table (10 × [4 digits, handler], `0A`×4 end; checked by `$C5:6836`) / entered digits / password flag bits (session / saved to SRAM `$3061E8` by `$C0:4858`; boot reloads only bit `$10`). Handler `$C5:6909` ORs a bit. Verified by JPN disassembly; see docs/COM_LEVEL_4.md |
| `$7E6247` / `$7E625B` | Battle COM level limit (boot 3, 4 with password 1103 / the COM level mod) / chosen COM level, limited by script `$C6:91F1`; `$C4:0067` stores `$625B - 1` in `$1D26`. Verified by JPN disassembly; runtime not yet checked |
| `$C6:A4A9`, `$6246`, `$7E2900` bit 3 | Match-start script: variant `$6246` from `$6274` bits `$04/$02/$01` (3/2/1), else saved `$6273`, else 0; flag 3 (item rule, password 0110) cleared unless `$6274` has `$20`. Mode = `$74 + 4*(arena-1) + $6246`. Verified by disassembly and 52 forced-bit routes (Linux LLE) |
| Battle stage data | Stage script `$C6:0000+(mode-1)*3` → header → (handler, long ptr) list: 1 setup (`10 n` soft blocks, `08 03 49` item rule only in arena 1, `FF.79 n` collision layer, default 2), 2/6/7 map loads after `27 nn` (`0a map ts` layer 2 + optional `0c map` layer 1, or `0b ts 0c map`; `$C2:0BED/0C06/0C0C`), 4 actors `22 P x y` (spawns). Map `$D0:A000+(map-1)*3` → long → `$C0:1CB0` stream of 256 metatiles into `$7E2500`/`$2600` (`$7E2400` collision copy), solid = attr `& $80` at `$D0:8000+(ts-1)*2`. Script VM op table `$C2:04F8` (16-bit), FF ops `$C2:064D` (24-bit). Verified against `$1052`/`$16AA` for all 13 arenas × 4 variants |
| SRAM `$30-$6A` | Checksum (16-bit sum of `$32-$6A`) and three 19-byte Normal records at `$32/$45/$58`: 16 flag bytes (stage N → byte N>>3 bit N&7), cleared count, Continue stage, `$6258` |

Netplay name tags (2026-10-22, `src/name_tags.c`): a seat shows its room name when `$0AEE` = 1 (stage live; 0 in
title, menus, picker and during the stage restart after a lost life), `$1D1D` = 0 (not paused), mode `$1031` is a
campaign stage (`$0D-$73`, `$1D1A` bit 0 clear) or Battle (`$74+`, bit 0 set), `$17FA+i` != `$FF`, `$15A2+i` bit 7
clear (set from the explosion frame), and a person drives it: Battle `$6269+i & $7F` = 1 (2 = COM), campaign `$1D25`
bit i and `$101F+i` = 0. Feet on screen = world − signed scroll; head top ≈ feet − 17, Louie rider 8 higher.
Verified by Battle (2 humans + 3 COM) and Zone 1-1 dumps, an own-bomb death/respawn route (Linux LLE) and loopback
netplay logs.

When you identify a new address, document it here (and optionally name it in
`src/variables.h`), with how it was verified.

Portal labels research (2026-10-15): `$C6:0000 + (stage-1)*3` points to the
native per-stage script list (`$C2:0008`); gate entries use handlers 13/11/12,
with `$11` creation coordinates (`$C2:0C38`), `$0F` destination (`$C2:0C20`)
and `$12` contact (`$C2:0C67`). Boss route flags 7–9 select A1/A2/A3,
10–11 B1/B2, 12–13 E1/E2, 14–15 J1/J2. Verified by disassembly and a local
ROM audit of all 192 gate entries. `$049F+object` is the native animation
frame (`$C0:11F8`); gate object signature is `$03D7=38/$04A0=26/$04FF=06`.
Palette slots remain at `$06DF+object`. Gate positions follow `$C0:1F78`:
x = tile*16 − signed16(`$1D3B`) − 8; y = tile*16 − signed16(`$1D3D`) + 8.
Verified against hidden/revealed Zone 1-1 gate objects and native OAM dumps;
zero animation frames are deliberately invisible, even with an allocated object.
Boss stages (`$67-$70`) keep their lists in bank `$C7`; gate create/contact
scripts there start with VM conditions: `$08 f` = route flag f (`$C2:0BDA` →
`$C2:0372`, condition word `$0373`), `$83` negate (`$C2:1CBA`), `$5B lo hi`
set (`$C2:1AC2`), `$49 n`/`$47 n` skip n bytes after the operand when
false/true (`$C2:19B3/$19AA`). Flag 4 opens the next boss gate instead.
Verified by disassembly, a 30-exit ROM audit and a Zone 1-A route-flag-7 run.


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

- Pause panel transparency must dim the field through the color window, with
  BG3 excluded from color math. Half-adding BG3 also blends PAUSE/ZONE, menu
  glyphs and the glove. Restore window/fixed-color registers after drawing.

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
- P1/P2 mount backup actor slots overlapped P3/P4; coop moves the mounts to
  rider + 19 (actors 19-23) and keeps NPCs at 5-18 (14 slots, the
  cartridge's numbering; an earlier 5-9/10+ layout broke stage scripts).
- The five-player menu forces the accurate compositor for that frame because
  `NewRenderer=1` showed only two rows.
- Title script return slots can retain attract-mode `$C7:1B3B`. Detect the
  Normal player menu from mode 2 and both native BG3 tile pairs at VRAM word
  `$424B/$424C` (302/261) and `$428B/$428C` (278/261), before overlays. Verified
  against translation metadata, an interactive launch and scripted title runs.
- Picker cells are counted by the script in several places, but the choice is
  the *cursor value* (`$1034` → `$625F`) until the exit script rewrites
  `$625F` with the cell's character (`$C6:A575`). A new cell must hold its own
  choice as character, and every ten-entry table indexed by `$625F - 1`
  (FF.66/FF.67 portraits, `$C4:2A3C`, score board `$C4:3BDE`) needs a clamp.
- Script data operands are inline: relocating a WRAM array means patching each
  `51 lo hi 7E` / `36`/`37` operand plus native long reads (and any mod
  trampoline that reads it — use registers instead, as Subordinate now does).
- Hide replaced sprites at y=224, not 240: `y>=240` wraps a 16x16 sprite into
  the top lines and the overlays treat it as visible.
- The development exe in `build-lle/` must never be shipped; releases are a
  clean-checkout **setup host** (`SNESRECOMP_SETUP_HOST=ON`, no `src/gen/*.c`).
- Mod package version (0.1.0) ≠ host version; a `.snesmod` cannot add native
  code to an older executable.
- The cartridge is written for two seats. Exit warps (`$C2:0C9F`), damage,
  floor gimmicks, enemy contact (`$1FF4` two-entry arrays) and boss
  choreography (`LDX #2`, `LDA $17FC`) all needed explicit five-seat handling;
  each fix is listed in §6 and docs/NORMAL_COOP.md. Check new features for the
  same assumption with P3–P5, not just P1.
- Never move enemy actors. Stage scripts name enemies and gimmicks by
  absolute actor number (ZONE5 - 1's tower core attaches to actor 9); with
  enemies shifted to 10+ the tower lost its core and beams and froze when
  destroyed. Co-op keeps the cartridge numbering and moves the extra mounts
  to the end of the table instead (`$1CBF` = 19).
- Five seats exhaust the 8 OBJ palette slots and the 5 animation pages. Give
  stage objects priority (palette arbitration at `$C0:0303`) and draw
  page-less objects from the cartridge; check palette/page failures with a
  lab log when a stage looks wrong only with 3+ players. Seats use OBJ slots
  7/6/5 only because co-op shortens the 33-byte blink copies
  (`$C1:0015/$0035`); a borrowing seat reclaims its slot when it frees up.
- `$1CBF` doubles as bomber count and mount offset. Never set it past 5: the
  survivor and player-on-cell loops (`LDX $1CBF; DEX`) then treat enemies as
  players. Co-op moves only the offset readers to `$011E`.
- Scan every `CPX/CPY #$02` seat loop, including bodies that only call other
  routines (`$C1:6B32` platform carry calls `$C4:3C32`); simple pattern scans
  missed it and the tower shock loops in banks C0/C1.
- "Zero in every recorded dump" is not proof that WRAM is free: `$1FC0` was
  the flame-thrower's table in one stage. Prefer ranges no code can reach
  (the never-used bottom of the stack page) and audit ROM operands nearby.
- Bomber tables hold five slots. Boss rivals use them, so five players cannot
  all fight rival-bomber bosses; slot 5 jumps into data and freezes the game.
- Player spawns run before the stage collision map exists; anything reading
  `$1052` at spawn time sees zeros (P3/P4 appeared outside smaller arenas).
- Patch immediate operands at opcode+1. A one-byte offset error at
  `$C3:4766` corrupted the following branch and crashed every enemy update.
- Picker cancel must restart the complete native file-list entry (`$C7:100E`
  New Game, `$C7:0D59` Continue). `$C7:1014` is inside FF.8D operands; entering
  there corrupted VM execution and could stick at `$C7:108D` with NMI busy.
  Unwind the suspended fade call (task-9 depth `$0D08`, VM byte-stack pointer
  `$10/$11`, three-byte return address), and preserve `from_continue` across
  snapshot loading. The scheduler id `$73` need not be 9 at the checkpoint.
- Framework submodules point to maintainer forks for the five-seat launcher;
  push fork commits before updating gitlinks (see docs/PUBLISHING.md).
- A five-seat picker with every picker mod on leaves few idle OAM entries
  (cursor boxes are 8x8 sprites). The third-row Super Bomberman 4 figures
  silently disappeared with Subordinate Bomber enabled; they now reuse
  entries of native sprites that end above the row, written by the raster
  observer on that line (`late_places`). Test overlays with all mods on.
- The developer plays `build-lle/SuperBomberman5SNESRecomp.exe` through a
  desktop shortcut; source changes appear only after `./tools/build-windows.ps1`.
- Never store absolute paths in files the player keeps (`rom.cfg`, mod `state.toml`): game folders are copied
  between PCs. Write through `snesrecomp_portable_path` (relative to the game root, the folder holding
  `rom_identity.txt`) and read through `snesrecomp_resolve_stored_path`; ROMs are also found by SHA-256 in the
  game folder and `roms/` (v2.2.6, `portable_paths_tests`).

- Netplay rollback with the LLE beam driver needs host state in the snapshot:
  the driver's resume PC/WAI latch and the PPU raster journal now ride in
  `RtlGameInfo.exec_state_*` (game_rtl.c). Even so the rollback probe
  (`SNESRECOMP_RB_PROBE`) shows VRAM divergence, so the game forces
  delay-sync (`Sb5ForceDelaySync`). Check netplay changes with
  `tests/netplay_pair_runtime.py` (two loopback peers, `SB5_STATE_TRACE` WRAM
  hashes, `SNESRECOMP_NET_TEST_INPUT(_SCRIPT)` pads) and
  `tests/netplay_determinism_runtime.py`. Any new mod state that lives
  outside WRAM must also be in a save-state extension, or peers desync.
- Netplay refuses `--script`; headless peers get input only from the test-input knobs.
- Objects can share a palette slot without holding a reference (HUD faces use
  their bomber's slot). Deleting such an object with `$C0:1342` releases the
  slot anyway; set its `$06DF` to `$FF` first. A freed slot still in use shows up
  later as another object in a bomber's colors (ZONE3 - 1 bridge, no gate).
  Gate animation 2 alternates a visible frame with frame 0 every tick: sample two
  consecutive frames before calling a gate hidden.
## 9. Skills in this repo

- `.agents/skills/super-bomberman5-mod-development/` — creating, debugging and
  validating native mods/features (RAM research, plugin pattern, overlays,
  in-memory ROM patches, save-state extensions, tests, docs).
- `.agents/skills/super-bomberman5-repository/` — Git hygiene, versions,
  bilingual docs, publication audit, setup-pack packaging, GitHub releases.

Framework references: `snesrecomp/docs/MOD_PACKAGES.md`, `FRAME_MODEL_HOSTS.md`,
`MULTITAP.md`, `LLE_FIRST_ANALYSIS.md`, `MULTI_TIER.md`, `GAME_PROJECT_SETUP.md`.

## Gameplay screenshot policy — 2026-10-15

Gameplay screenshots are authorized for README/docs/releases under `docs/images/`.
Gameplay screenshots are optional; no per-mod image is required. Extracted assets, ROM,
generated code, saves and memory/audio dumps remain prohibited.

Picker grid palettes (2026-10-15): `$C2:1301` gives destinations 38/48/58/68/78
(top), 30/40/50/60/70 (bottom). Preserve the upper strip first entry: shared
with the lower figure at BG index 8. White records can contain a nonzero first
color. Verified from `$C2:12A1` and Normal/Battle CGRAM six-color column routes.
Portrait OBJ uploads still need nine colors.


PT-BR completion research (2026-10-15): native `$C6:E120-$E449` selects
33 rank/tip variants (0–32), using percentage, boss/ending flags and secret
stages. Layout group 4 `$66-$86` contains their 24-tile title rows and
`$89-$A9` their tips. Certificates identify BG1 rank row 19, column 4;
tips use row 8, column 4. The signature is baked into BG2, covered only
during drawing with native parchment color index 1. Game Over is Mode 7
OBJ: title x=62/134, y=23/39, tiles `$00-$3F`; choices x=98, y=159/183,
tiles `$40-$4B`, palette 2. HUD routine `$C5:073A` selects group-4 `$DD/$DE`
(Push Start/Game Over) at BG3 row 0, columns 3/21, and blinks every 32 frames.
Verified by JPN disassembly/layout audit, isolated Normal end/reward routes
and P1/P2 missing-actor fixtures. Script flag storage `$2900`, helper
`$C2:0372`: native completion count (`$C2:0E06`) counts flags 16–115.

Gold Battle research (2026-10-15): `$6245` is the volatile arena count (10/13),
loaded by `$C0:48B5`; arena id `$625A` selects native mode `$74+4*(arena-1)`
for the default variant. Arena 11/12/13 scripts `$C6:0EE4/$0F9A/$10A8` match
both supplied ROMs byte-for-byte. Verified by local comparison and isolated
Windows/LLE selection + gameplay in all three. `$625D` mask `$18` selects
Off/On/Super; Gold Battle adds `$40` and retains `$10` for Hyper. Authored VM
programs reserve FF padding `$C7:3000-$30FF` (VM `$C2:0254` rejects banks below
C6). Hooks: `$C4:365A` saves equipment after bomb cleanup; `$C4:3791` has victim
in `$8D` and Revenge owner in `$A1`; `$C4:36E8` drops items; `$C4:2B37` ends
reentry spawn. Gear is `$4EBD/$17CA/$15EA/$15D2/$4ED5 + seat`. Verified against
Gold disassembly (`$C4:37F9/$3822`) and P1/P2 native contact/death/cart/reentry
fixtures; all 20 ordered pairs covered by native tests. Gold's state extension
is appended after Pause, with matching save/load order. Pause audio correction:
`$C5:0096` sends the native entry command; `$C5:0037` sends its resume partner.
Keep both; menu accept/back must not add another SFX when resuming.

Battle arena selection text (2026-10-15): native Stage word uses OBJ slots
9–11; number slot 12 (16x16) stays native. For arena a, n=10+2*(a-1), its tile
is $C0+(n/16)*32+n%16. This displayed number can lag WRAM $625A by one frame.
Native names use palette 1, top/bottom rows y=195/203; the 8x16 font has
uppercase A at tile $02 and lowercase a at $36, increment 2 per letter.
Glyph halves are consecutive tiles. Pink ink is CGRAM 148; indices 1/3/4
are outline/shadow/ink. PT-BR derives letters from live VRAM and adds authored
accents, borrows free OBJ tiles/slots, and restores OAM/VRAM after drawing.
The nine-piece header is invariant; BG1 $30C9 is the changing preview and
must not guard it. Verified with all 13 numbers and 109 transition/stable
Windows/LLE frames, plus 82 frames without Gold Battle.

Subordinate unlocked-cell correction (2026-10-15): `$C6:8B02/$8B10` are the
unlocked cell-9 character/color immediates (stock 1/5), alongside the locked
branch `$C6:8B20/$8B2E` (stock 0/0). Subordinate now patches both to 10/6,
independent of the secret unlock; the operand addresses do not overlap
Honey/Kotetsu's relocated array references at `$C6:8AFE/$8B0C`. The native
FF.45 getter `$C0:4893` reads secret unlock SRAM `$1E9`; it is never changed
by this mod. Verified by JPN script bytes/disassembly.

Native PT-BR typography (2026-10-15): the arena alphabet's raw 4bpp halves
are present in ROM (A top $C3:7920, bottom $C3:7900; subsequent references
are deduplicated and not necessarily sequential). White title lettering is
raw 2bpp at $E3:D9D0 onward, with shared top/bottom halves. Blue BG headings
also have raw tiles; native colors 3/4/7/11 are bevel fill, 6 outline and
1 shadow. Background colors 8/9/10 plus connected 11 must be removed before
using letters on transparent OBJ. Stage-header palette maps BG 6/7/11 to
OBJ 5/6/7. `src/ptbr_font_locations.h` stores addresses/crop metadata only;
read pixels from the cartridge at runtime. Verify using native VRAM/ROM
byte comparisons, multiple original headings and paired Windows/LLE routes.
Do not replace these references with extracted bitmaps. Blue glyphs are
cached by character and ROM pointer to avoid a per-frame background flood fill.

Native heading crop correction (2026-10-15): palette index 11 is both the
bright middle bevel and banner background. Never flood-fill a crop bounded
by ink alone: E/F/H crossbars reach those edges. Preserve two native outline
columns on either side, then remove background and extract the ink. Metadata
uses four tiles per source row (eight references per glyph). Reserve the
first output row for the upper outline, shifting ink one row down; otherwise
transparent OBJ headings lose their top border. Verified with synthetic H,
optional reference-ROM E/F/H/OBJ checks and paired Windows/LLE menu routes.

Battle confirmation lettering (2026-10-15): BATTLE START is nine 16x16 OBJ
at (56+16*i,19), palette 1, tiles $A2/$A4/.../$AE/$C0/$C2, in the same
primary OBJ bank as STAGE SELECT. Tile wrapping must include the starting
column, not assume a row-aligned first tile. Native blink removes the whole
heading from OAM: Stage words shift to slots 0-2 and the numeric sprite to 3;
with the heading visible the number is 12/13 depending on cursor presence.
Native number position remains (138,171). Match the complete heading or
blink-off Stage strip and retain translated names through confirmation.
Verified from consecutive Windows/LLE OAM/VRAM/CGRAM captures; no ROM patch.


Louie improvements research (2026-10-15): egg selection at `$C4:514F/$5157`
passes inclusive maximum 5 to `$C0:1DEB`; selection is consumed at `$C4:515D`
and attached at `$C4:519C`. Mount indices: Red 0, Yellow 3, Black 6. Native
mount actor = rider + `$1CBF`, type at `$16C2`, tables `$C0:C3ED/$C3F7/$C401/$C40B`.
Verified by JPN disassembly and Windows/LLE deterministic egg pickups of 0/3/6.
`$C4:5560` passes the Red punch's facing cell to the player search `$5582`.
`$C4:5797` tests Yellow's facing cell; moving-block contact is `$C4:5973`.
Native ground bomb kick `$C5:2293` uses direction in A, origin cell `$8A`;
bomb arrays (20 slots): `$0E40` fuse, `$0E7C` cell, `$0E90` movement,
`$0F38` progress, `$0F4C` speed. Verified by disassembly, Yellow 0x33→0x36
rolling-bomb route and native tests. Do not replace this with Blue's airborne
kick `$C5:2500`. Clock helper `$C3:4C13` freezes one actor (X, object at `$52`)
using global timer `$0E20/$0E21`; animation speed `$0800+object`. The mod borrows
and restores the global timer within the helper's call, returning at `$C3:4C69`.
Verified by disassembly and Red enemy held/resumed captures. Native actor
dispatch `$C4:224F` has an existing long-call return frame; allocation `$C4:2266`
clears inherited mod effects. Native player screen position + signed scroll
`$1D3B/$1D3D` gives world position (`$C4:31BD`); Zone 1-1 scroll is -16/-28.
Keep local test captures/audio under ignored directories. New authored native
trampolines reserve `$C4:5B80-$5C1F`; fixture-only egg mailbox `$7F:FFF0` is not
present in normal builds. See docs/LOUIE_IMPROVEMENTS.md.


Louie audio cues (2026-10-15): Red's `$C4:5563` tests carry from the native
player search. With only an enemy hit, redirect to `$C4:5576` to reuse the
exact player-hit SFX (A=`$0238`, `$C0:0CED`); do not duplicate it when the native
player path already succeeds. Black acquisition retains `$C4:5168` (group 2 / `$2E`). Dash calls group 2 /
`$35` via `$C4:5C00`, setting M/X to 8-bit for `$C0:0CED`, restoring registers
and flags, then executing the original input-poll PLX/PLY/RTL tail. Verified
by disassembly and PCM comparison with the stage sound bank loaded.
Fixture-only FFF4/FFF5 count acquisition/dash native commands.

## Louie audio policy correction - 2026-10-16

The maintainer clarified that the two bundled Black Louie WAVs are extracted
game audio and requested removal. Current source and new packages must contain
neither WAV; use cartridge SFX at runtime. Exact historical hashes are retained
only to audit already published Git history, not to allow new distribution.
Published tags, history and release assets are not rewritten by this correction.

Title count font (2026-10-15): runtime 2bpp glyph halves 1 = ROM file
`$23DC90/$23DC20`, 2 = `$23DB10/$23DD50`. Verified by exact tile-byte
matching against Windows/LLE title BG3 VRAM `$12E/$127` and `$116/$13A`;
the copyright uses the same font page. No extracted pixels stored in source.

Normal picker cancellation (2026-10-16): `$C7:100E/$C7:0D59` call screen
cleanup `$C7:118B` before FF.8D loads the New/Continue file archive. `$C7:1014`
is an operand byte, not an entry. VM call `$C2:1737` pushes a 24-bit return
through `$C2:02EE` and increments `$0CFF+task` (task 9 = `$0D08`); the byte
stack pointer is `$10/$11`. Verified by disassembly and paired interactive
file/picker/cancel dumps (depth 1/2/1, pointer 6/9/6), including reentry.
White title Y halves: ROM file `$23DCB0/$23DD40`, matched to native title
VRAM tiles `$130/$139`. T's source cell includes a neighboring U outline
at column 7 below row 5; crop only that strip when isolating the glyph.

Normal HUD faces research (2026-10-16): `$C5:1264` allocates only a face
object (archive `$64`, set 6, `$04A0=F0`), guarded by Battle bit 0 at
`$C5:126B`. Normal's lives refresh calls `$C5:0C86` at `$C5:05F1`.
The optional HUD mod wraps it through verified FF padding `$C5:7200-$72FF`,
creates at most two faces at x=24/160, object y=20 (drawn y=3), and clears
BG2 HUD map `$3C00` columns 3–4/20–21 across rows 0–2 only while rendering.
The fixed heads use tiles `$EE-$F3` / `$F4-$F9`; preserve scanlines 0–3 and
21–23, fill 4–20 with native blue index 4 in borrowed 4bpp tiles. BG1/BG2
share tile base 0; scan both maps before borrowing `$280-$2FF`. Verified by
Windows/LLE VRAM, raster scroll writes (BG2 y=256 on HUD) and pixel captures. `$C4:36FF`
precedes Battle's death notification; a width-preserving wrapper calls
`$C5:12DD` for Normal P1/P2 without running Battle equipment/palette logic.
Verified by JPN disassembly and local Windows/LLE 1P/2P stage captures.

Normal Game Over Continue (2026-10-16): `$625F/$6264` and Honey/Kotetsu
`$7F:F818` markers survive native Game Over, but `$C4:289D-$28B1` resets
`$16C2/$16DA`. Continue can also leave `$1D25` with only the last dead seat.
Normal-character hook restores choices after reset (`$C4:28B4`) and before
actor creation (`$C2:1697`), with a one-shot one/two-seat roster reset.
Install this hook last, reset it first. Existing extension state 6 stores
selected+pending-restart without changing slot size; 0–3 remain readable.
Verified by isolated Windows/LLE before/after WRAM, own-bomb Game Over and
Continue routes, including Honey/Subordinate with co-op disabled.

Normal co-op HUD isolation (2026-10-16): Native reentry calls `$C5:1307`
from `$C4:2B33`, indexing the Battle face array even for co-op extra seats.
Normal HUD mod wraps that call at `$C5:72A0` to permit only seats 0/1, and
wraps `$C5:0608`'s score update through `$C5:72B0` to maintain just P1/P2
faces every HUD task. Living animations are checked against the native
lookup operand at `$C5:1319` (including relocated Subordinate table).
Honey/Subordinate HUD overlay loops are also capped at two in Normal,
retaining five in Battle. Verified by JPN disassembly, native context tests
and 1/3/4/5-seat Windows/LLE/NewRenderer=1 fixtures, including stale-pointer/
wrong-animation injection and extra-seat reentry. No new SRAM/state layout.

Release 0.13.1 Windows entry: src/windows_entry.c is a ROM-independent WIN32 target named SuperBomberman5Recomp. Keep the main SuperBomberman5SNESRecomp target/name unchanged for framework local rebuild compatibility. tools/build-windows.ps1 copies the entry to the source root; Since v1.0.1 tools/finalize-release.py instead ships the setup host itself as the ZIP's only root executable, SuperBomberman5Recomp.exe; SNESRECOMP_GAME_TARGET keeps rebuild/relaunch on SuperBomberman5SNESRecomp. The rebuild helper .cmd writes paths in the OEM code page (8.3 fallback) (accented folders such as "Área de Trabalho"). Its child lookup/Unicode argument/working-directory/exit-code contract is tested by tests/windows_entry_runtime.py. Extra Characters choice/color arrays are $7F:F828/F82D; Normal picker snapshot restoration preserves both via ExtraSetSeatChoice/ExtraSetSeatColor.
