---
name: super-bomberman5-mod-development
description: Create, extend, debug and validate native mods and gameplay/presentation features for the Super Bomberman 5 SNESRecomp port (src/*_mod.c, mod manifests, RAM research, overlays, in-memory ROM patches, save-state extensions, native tests). Use for any gameplay, menu, translation or rendering change; use super-bomberman5-repository for Git, versions and releases.
---

# Super Bomberman 5 — native mod development

Read `AGENTS.md` at the repository root first (hard rules, RAM map, build and
pitfalls). This skill is the step-by-step recipe for a feature. Every existing
mod (`ptbr`, `normal_character`, `selection_color`, `infinite_time`,
`normal_coop`, `subordinate`, `honey_kotetsu` (extra-character core),
`bomber_kings` + `special_abilities` + `sb4_rom`/`sb4_sound`, `pause_menu`,
`portal_labels`, `gold_battle`, `louie`)
is a worked example; copy their structure instead of inventing a new one. For a
new playable character read both character sections below first.

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

### Adding a non-bomber playable character (Honey and Kotetsu, v0.9.0)

Honey and Kotetsu are boss actors of the secret stages `$71-$73`, not bombers:
there is no entry for them in any per-character table. `src/honey_kotetsu_mod.c`
makes them choices 11/12 that **play as Bomberman** and are **drawn** with
their own frames. Reuse this split for any future boss/NPC character.

1. **Identify the art, not the gameplay.** Force the character's stage
   (`poke 0x1031 <stage>` after `until 0x1031 == 0d`), list objects
   (`$04FF` set, `$079F` anim, `$07FF` step offset, `$055F/$05BF` x/y,
   `$06DF` palette slot) and read the set's tables: animations
   `$C0:171D+set*3` (16-bit pointers to `frame, duration` pairs ending with 0)
   and frames `$C0:16CC+set*3` (count + 7-byte pieces `dx16 dy16 tile16 attr`,
   attr bit 0 = 16x16, `$40/$80` flips). Map each animation to a direction
   (R/D/L/U/spin for Honey `$1E-$22`, Kotetsu `$2B-$2F`) by rendering it.
2. **Decode graphics at runtime.** Log `$C0:1B04` archive loads (set `$0AE7`,
   entries `[vram,size,src24,flags]`, bit 7 raw) to find the archive, port the
   `$C0:1CB0/$1CCC` decompressor (control: mode bits 5-7 literal/prev+1/prev/
   copy -2/-4/-16/-32/-64 in a 256-byte ring, count-1 bits 0-4) and check the
   **exact stream end** (`$CC:40E4`) at install; refuse the feature on
   mismatch. VRAM layout is 16x16 blocks (TL, BL, TR, BR): frame tile `t` is
   archive block `(t%16)/2 + 8*(t/32)`, sub-tile `(t%2)*2 + (t/16)%2`.
   Validate every frame you will draw (piece count, tile range); some pieces
   use tiles outside the archive (Kotetsu `$F0-$F7`) — skip those, don't fail.
   Palettes are read from the ROM (`$C1:8FC6` / `$C1:8FA6`); nothing ripped
   goes into source.
3. **Grow the picker without touching native tables.** The picker script
   counts ten cells everywhere. Relocate the cell arrays (`$62C9/$62D3`) to
   verified-free WRAM (`$7F:F800`, 12 entries) by patching every script
   operand (`51 lo hi 7E`, `36/37`) and native long read, replace the cursor
   arithmetic with lookup tables (`$C6:8C02/$8C78/$8CEE/$8D62`, op `$36`),
   extend cursor-position tables, and repack the grid (`$C2:12D2`). The exit
   script writes the **cell character** to `$625F` (`$C6:A575`): the new
   cells must hold their own choice numbers. Expose the arrays through
   `PickerCellCharacters()/PickerCellColors()` so the colors, Normal-picker
   and Subordinate mods keep working (their tests stub these helpers).
4. **Map the choice before the native consumers.** Every table indexed by
   `$625F-1` must be clamped: stage init `$C4:2A3C` (store the real choice in
   a per-seat marker, `$7F:F824` (was `$F818` before the three-row core), then play as 1), score board `$C4:3BDF`,
   portrait helpers FF.66/FF.67. Clear markers on the title (`$1031 == 2`);
   the Normal hand-off (`HoneyKotetsuSetSeatChoice`) records them too.
5. **Replace exactly the bomber's sprites.** Do not pick sprites by palette
   and distance: the Louie mount (its own object, set `$01`) and eggs share
   the rider's palette slot. Read the object's current frame pointer
   (`$7E2EEA/$7E2F4A + object`), find each piece in OAM at object + offset
   (search a constant screen offset; airborne frames are offset), and keep the
   last 2-3 distinct pointers because the OAM shown lags the pointer by one or
   two steps (without that, ~3/48 frames flashed the native bomber). Borrow
   all 16x16 blocks before hiding anything; reuse the hidden native OAM slots
   first to keep priority; hide at y=224.
6. **Pick a free OBJ palette.** Use a palette no visible sprite uses (7 first)
   and recolor the seat's slot only as a last resort; one palette per skin per
   frame, shared with the HUD face (set the face sprite's palette bits).
7. **Pose mapping from bomber animations** (set `$10`): walk 1-4 and carry
   walk `$2F-$32` (step from `$07FF`); actions `$27-$2A` punch, `$33-$36`
   throw, `$3B-$3E`; stand for the other four-way groups (riding `$05-$14`,
   lift, hold, airborne `$17-$1A`, cart); victory `$20`/cheer `$15` (also the
   DRAW GAME screen, mode `$0A`, and the champion screen, mode 5); spin for the
   rest (death `$16` — keep the native last step, the ashes —, stun `$21`,
   Time Up `$1F`). `$1782+seat` bit 7 riding (stand the figure where the
   native rider frame ends, no shadow), bit 1 jumping onto the mount (no
   shadow). Look for dedicated cartridge poses before inventing: the Battle
   champion screen (archive `$D6:D871`, set 6 anims `$50`/`$4E`) holds the
   cheering frames used for victory/draw, alternated with native durations.
8. **Canvas.** Compose frames into a palette-index canvas (48x64, origin
   16,42) and emit 16x16 blocks for non-empty areas (second column for wide
   weapon frames). Different archives use different anchors (champion frames
   are left-anchored, feet one line above the origin): align feet, not tops.
9. **Validate like a player.** Side-by-side scripted routes against the same
   route without the mod, dumped every frame for animated cases: walking,
   power/boxing glove (items placed with `poke 0x145a+cell`, revealed via
   `$1052=$10`, `$1152=$80`), own-bomb death, stun (`poke 0x15ba 40`), victory
   (other seats `poke 0x15a3.. 99`), draw (all seats), Time Up, egg (item
   `$15`, revealed from a destroyed soft block) → ride → dismount, score board
   and champion screen (three won rounds), picker portraits and HUD faces,
   plus picker/Battle with Subordinate and colors enabled.

### Characters from another game and special abilities (Bomber Kings)

The Bomber Kings mod adds Super Bomberman 4's Bomberman and five Bomber Kings
(choices 13-18) on a third picker row, and B + direction abilities with SB4's
debuff (also for Honey/Kotetsu). `src/honey_kotetsu_mod.c` is now the
**extra-character core**; add future characters there as new skins, not as a
second copy of the picker/skin code.

1. **Second ROM, requested by the launcher.** Declare it in the manifest as
   `[[external_rom]]` (`feature`, `id`, `label`, `format = "snes"`,
   `normalized_sha256` — header-stripped digest, so `.smc` copies pass —,
   `file_patterns = "*.sfc,*.smc,*"` — the trailing `*` lists every file, so a ROM with an unusual extension can still be picked; the hash decides —, `required = true`). The Mods page then
   shows a file picker under the feature, verifies the file ("ROM verified")
   and refuses Play until it is selected; the selection is saved in
   `state.toml` as `[[resource]]`. The plugin reads the path with
   `snes_mod_runtime_resource_path_c(package, feature, id, ...)` and
   `src/sb4_rom.c` re-checks SHA-256 before using it. Headless tests add the
   `[[resource]]` block to their `state.toml`. Never commit or package
   anything from that ROM.
2. **Research the other game in an emulator, not by guessing.** Build the
   libretro snes9x core locally with debug exports (RAM/VRAM/OAM/CGRAM, a DMA
   log, a write watch with PCs, PC breakpoints with registers) and drive it
   from Python with ctypes (save states for each scenario). SB4 facts found
   that way: battle sheets of raw 32x32 frames (`base + (f/4)*$800 +
   (f%4)*$80`, tile rows `$200` apart) found from the DMA source of the
   player's VRAM uploads; frame roles from the frames uploaded while walking
   each direction; five player palettes per character `$20` apart from
   five-player CGRAM dumps (multitap: `retro_set_controller_port_device(1,
   0x101)`); ability input/debuff from breakpoints and RAM diffs.
3. **Three-row picker.** Generate the movement tables from one grid array
   (`kGrid`), with separate up/down tables (`$C6:8CEE` moves **up**,
   `$C6:8D62` **down**). Move the native BG figures by patching the row table
   `$C2:12DC`; cursor Y = row*8+24. Patch records need room for 19-byte
   tables (the patch helper silently refused >16 bytes and the whole install
   was rolled back — keep the count check).
4. **Many figures, few OBJ palettes.** Rows have their own palettes: the
   overlay schedules CGRAM loads at the first opaque line of each row (the
   raster observer runs every line; `put()` saves originals so line 224
   restores everything). A palette is usable for a row when no native sprite
   in it reaches that line or below and the previous row's figures ended
   above it. Emit 16x16 block rows starting at the first drawn line (32-line
   figures then need two block rows, not three). Draw seat portraits before
   grid figures, or a portrait's own-palette check sees your figures.
5. **Anchor foreign frames to the native bomber.** Compute the native standing
   frame's centre/bottom from its pieces (set `$10`, anim `$47`) and place
   every foreign frame by a fixed reference (its own standing frame's opaque
   box), so walking frames don't jitter; riders use the current native
   frame's bottom and the foreign frame's own box.
6. **Abilities through pre-opcode hooks.** `interp_bridge_add_pre_opcode_hook`
   gives the `CpuState` at one PC: `$C0:0AF9` (pad reader done, arrays
   `$2C/$31/$36/$3B+seat` filled, nothing consumed yet) and `$C4:3180` (walk
   steps in A, X = seat). When you change A before a store, also set
   `_flag_Z`/`_flag_N`: the following `BEQ` reads the flags of the original
   computation (a 0 count without Z made the walker loop 256 pixels).
7. **Input rules that keep native B.** Ability = B newly pressed with a
   direction (held or new), or a new direction while B is held. A fresh B
   alone is held back for 8 frames waiting for a direction and then given
   back (remote detonation unchanged); riding (`$1782` bit 7), debuff or
   cooldown → native B immediately.
8. **Damage with the cartridge's own rules.** Actors 0-23 (`$17FA` bit 7 =
   none), world `$182A/$1842`, `$1872/$188A`, cell = (pos+8)/16. Player:
   `$15A2 |= $40` unless `$15A2` bit 2 (native invulnerability, timer
   `$170A`, also used for Jet/Great). Enemy: `$085F[object]` = 0 → `DEC
   $1662`; zero → `$15A2 |= $40`, else `$085F = $B4`. Normal: never a seat in
   `$1D25`. Battle: Team Battle `$625D` bit 0, teams `$15A2` bits 3-4.
9. **State in free WRAM** (`$7F:F830-$F8D9`) instead of C statics: save
   states and rewind keep it without a new extension. Clear it whenever the
   game is outside a stage.
10. **Port moves, don't reinvent them.** Capture the other game's ability
    frame by frame (pose addresses from the DMA log, OBJ effect tiles, BG
    fire tilemap phases, timings) and replay that timeline; draw its graphics
    (decoded from the ROM's own tile streams) through the core's
    `ExtraEmitPixels()`. Identify each character by its sheet in a capture
    of a known seat before mapping it (Hammer and Bazooka were once swapped).
11. **Native reactions over custom ones.** Stun + item loss = `$15BA |= $20`
    (the cartridge's `$C4:4618`/`$C4:4631`). A routine that ends in `RTS`
    can be called from the `$C0:0AF9` hook by pushing an `RTL` frame back to
    `$C0:0AF9`, then an `RTS` frame to any `RTL` in the routine's bank
    (`$C4:36D1`), and `interp_bridge_pre_opcode_redirect()`; the hook runs
    again on return — skip the frame logic and restore A/X/Y/D/DB there.
12. **Don't refresh the invulnerability timer every frame** if the native
    sprite must stay visible: a fixed `$170A` freezes the blink (hidden);
    let it run (extend it in steps of 4 so the blink phase is kept).
13. **Shared palette slots: recolor by lines.** A seat's mount, eggs and HUD
    face share its OBJ palette slot. Recolor the slot only over the figure's
    screen lines (line loads + restore on the next line); fall back to a free
    palette only when another sprite of the slot is on those lines.
14. **Sounds of another game.** Upload that game's sound driver from its ROM
    into a private `Apu` (`apu_init`, the cartridge's own upload handshake),
    send the effect number, render the DSP output to PCM and play it with
    `snes_mod_audio_*` (`src/sb4_sound.c`). Never touch the game's APU.
    Render each effect once (cache by id) and restart the same voice on a
    repeat: the original driver restarts an effect on its own channel, so
    stacking copies (Jet's trail every 8 frames) sounded wrong.
15. **Generalize injected guest calls.** Keep a small queue of calls
    (routine, A, X, ends in RTS?) and run them one per hook visit from
    `$C0:0AF9`: an `RTL` routine (`$C5:441D` burns a soft block, A = cell)
    needs only the return frame to `$C0:0AF9`; an `RTS` routine also needs
    the frame to an `RTL` of its bank. Restore A/X/Y/D/DB only after the last
    call returns.
16. **Use native objects instead of drawing look-alikes.** Bombs: list head
    `$0E2A`, next `$0EA4`, cell `$0E7C`, fuse `$0E40` (`$FF` = remote; set 1
    to make it explode next frame). `$1052`: `$80` hard, `$40` bomb, `$20`
    soft (`$30` with item), `$10` item. A beam that should "stop at" a soft
    block includes that cell and burns it with the cartridge's routine.
17. **Collision stops: watch the motion axis.** The walker slides around
    corners, so "position unchanged" never fires while it slides. A flight
    ends when the coordinate along its own direction stops changing; then
    play the original game's landing time (SB4: crash frame 96, walks at 120).
18. **Immunity needs the kill sites, not the invulnerability bit.** Enemy
    contact (and several blasts) ignore `$15A2` bit 2. Scan the image for the
    kill pattern (`LDA/ORA #$40` → `ORA $15A2,idx` → `STA $15A2,X/Y`; 22
    sites), hook each `STA` with a pre-opcode hook that clears bit 6 in A for
    the protected seat (index = X for `9D`, Y for `99`), and register a site
    only when the image holds the expected `99/9D A2 15` bytes.
19. **Rider and special-cart frames come from the other game too.** SB4
    riding = sheet frames 16-19, mounting jump 20-23; Bad Bomber cart/throw
    frames live in separate per-character sheets (see `docs/BOMBER_KINGS.md`).
    Check the rider state (`$1782` bit 7 riding, bit 1 jumping) and Bad
    Bomber (`$15A2` bit 7, anims `$42-$45`/`$3B-$3E`) before choosing a pose.
20. **Boss attacks as abilities.** Reuse the boss's own animations from its
    object set (Honey `$23-$26` attack, `$27-$2A` bullet; Kotetsu `$30-$33`)
    with the native durations, and spawn the projectile/hit on the frame the
    boss flashes (Honey: frame 32).
21. **OAM runs out before tiles do.** A full five-seat picker (cursor boxes
    are 8x8 sprites, "L+R" hints, Subordinate's figure) left too few idle
    entries for six 32-line figures. The scanline renderer evaluates OAM line
    by line, so a figure low on the screen may reuse the entry of a native
    sprite that ends a few lines above it: write that entry when the observer
    reaches the line (`late_places` in `honey_kotetsu_mod.c`), restore it on
    line 224 with the rest. Always test the picker with **all** picker mods
    enabled; a figure silently missing is the symptom.
22. **Test damage in Normal with enemies placed on purpose.** `forcepoke`
    enemy actors' `$182A/$1872` into each ability's path (Zone 1-1 actors
    10-13), trigger the abilities on the same frame and read `$15A2` (`$C0` =
    dead) for enemies and seats every few frames.

### Playable-character checklist (any new one)

- Native table or skin? If the cartridge has per-character table entries, use
  the Subordinate route; otherwise play as an existing character and skin it.
- Choice number and cell: next free choice (19+), its own picker cell in
  `kGrid` (tables are generated from it), cursor tables and grid rows; keep
  `PickerCellCount()` the single source of truth for other mods.
- Every `$625F-1`/`$16C2` consumer clamped; markers per seat; Normal hand-off.
- Art fully from ROM at runtime with an exact-end check; refuse on mismatch.
- Exact-piece replacement, OAM-lag tolerant, free palette, mount/egg intact.
- Poses for walk/stand/action/victory/draw/death/stun/Time Up/riding,
  picker figure, seat portrait, HUD face (alive and defeated), score board,
  champion screen.
- Colors: `ExtraCharacterColorCount()` (1 = no L/R; SB4 characters 5); the
  seat's color lives at `$7F:F829+seat`.
- Ability? Add it to `kAbility` (`special_abilities.c`) with debuff/cooldown
  and a pose; test input, damage rules (Normal/FFA/Team) and blocks natively.
- Tests: patch bytes and restore, movement tables inverse/reachable, refusal
  on unexpected bytes or corrupt stream, synthetic archive encoder with the
  exact end, pose mapping, piece matching (nearby same-slot sprite untouched),
  palette fallback, riding lift, alternating poses, exact restoration.

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
