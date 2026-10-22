# Super Bomberman 5: estado do protótipo

## v2.1.1 — 2026-10-22: netplay guest catch-up and diagnostics

- Report: online, host + one guest; the guest's screen froze/went black at some point, the host saw stutter.
- Cause (reproduced with recomp-net loopback sessions, no ROM needed): host-authority delay-sync never lets the
  host wait, but the desktop host's catch-up budget defaulted to 0, so every guest stall became permanent lag
  that only grew (the v2.1.0 name-tag harness already showed a guest 51 frames behind). recomp-net then resent at
  most 64 rows past a guest's ACK and a guest dropped host rows more than 64 ahead of its sim tick: once 64+ rows
  behind, it waited forever. With two seats (LAN/Direct IP) the guest acknowledged the highest row seen, so one lost
  host row older than the redundancy window was never resent. Loopback model, 16 ms per host tick: a 150-tick
  guest stall froze the guest even with catch-up; repeated 20-tick stalls without catch-up left 61 frames of
  permanent lag; 20% INPUT loss froze a two-seat guest at tick 0.
- Fix: recomp-net fork — host-authority guest window `[sim, sim+127]`, host resend up to 120 rows past the guest's
  contiguous ACK, contiguous ACK also for two seats, stats `auth_backlog/auth_resend_rows/auth_rows_dropped`,
  test knob `RNET_TEST_INPUT_LOSS_PCT`, new `session_host_authority_catchup_test` (2 and 5 seats, 160+40+40-tick
  guest stalls, 10% INPUT loss: guest back within 1 tick, identical rows; fails on the previous code).
  snesrecomp fork — catch-up cap 3 for host-authority guests (`SNES_NET_CATCHUP_CAP` still overrides; other modes
  unchanged), `host_authority backlog` log lines, a guest unrecoverable for 3 s leaves to the room
  (`host_authority_guest_lost`) instead of a frozen frame, per-match `net_diag_*` files with frame timing
  (fps, `frame_ms_max`, catch-up frames, backlog, resends) and stderr redirected to `saves/netplay/netplay_log.txt`
  when `SNES_NET_DIAG=1`. Game repo: `tools/netplay-diag.cmd`, `tools/netplay-diag-report.py`.
- Evidence and limits: recomp-net builds and its ctest suite passes except `open_url_test` and
  `session_state_test` (liveness case), which fail identically on the unmodified pin in this container.
  `snes_netplay.c`, `snes_host_app.c`, `host_main.c` pass `gcc -fsyntax-only` against SDL3 headers. **Not** built with
  the Windows toolchain, not run in the game (no ROM here), not tried in a real internet match. Catch-up frames
  run back to back (audio of those frames is produced faster than real time). The host-side stutter was not
  explained by this; the new per-frame timing in `net_diag` is meant to show it on the next match.

## v2.1.0 — 2026-10-22: in-game chat, name tags, spectators

- snesrecomp fork: new `runner/src/desktop/host_chat.c/.h` (SDL/GL-free state, layout, stb_truetype raster from
  recomp-ui's vendored `imstb_truetype.h`, Lato Bold from `assets/fonts`), compiled by `snesrecomp_enable_recomp_net`
  when recomp-ui is present (`SNES_HOST_HAS_INGAME_CHAT`). `host_main.c`: T opens the bar during a netplay match,
  text input via `SDL_StartTextInput`, Enter/Esc/Backspace consumed, keyboard pad neutral in `netplay_capture_pad`
  while typing and until keys are released, SDL renderer draws the overlay after the frame; `opengl.c`:
  `snesrecomp_opengl_set_window_overlay` draws it at drawable resolution after the GLSL shader and after the
  screenshot readback. Source: the launcher backend's `chat_send/chat_count/chat_get` (same ring as the lobby
  panel; `np_pump_session` pumps the lobby WebSocket every match frame), ignore list via `recomp_moderation`.
- Evidence: `ingame_chat_tests` (game repo) passes under gcc ASan/UBSan on Linux; `host_main.c` and `opengl.c`
  pass `-fsyntax-only -Wall` with and without the chat defines against the toolchain's SDL3 headers; a rendered
  1920×1080 preview matched the requested layout. Not built with the Windows Clang toolchain, not run in a
  real online match. Open: server-side relay of room chat during a match (client allows it: `in_lobby` survives
  `launch`); LAN/Direct IP has no in-match chat (waiting-room socket closes at launch); emoji draw as `?`.
- Linux build of the game (container, gcc, SDL3 static, LLE) compiled every changed file and passed all 19 ctest
  cases (18 after the name tags). Linking caught a missing `libm` for `ingame_chat_tests` on Linux (fixed in CMakeLists).
- Name tags (`src/name_tags.c`): guards and position verified on real dumps of Battle (2 humans + 3 COM: `$6269`
  1/2, `$15A2` `0x19`/`0x1a`, scroll -16/-28) and Zone 1-1 (`$1D25` = 1, `$17FA+i` = `$FF` for absent seats); menus,
  title and picker have `$0AEE` = 0; pause sets `$1D1D`. Screen = world (`$182A/$1842`, `$1872/$188A`) − signed
  scroll; head top ≈ feet − 17. A rendered 4x preview over the dumped frames put each tag above its bomber.
  `name_tags_tests` covers Battle/COM, death, pause, Louie, campaign mask, boss rivals and off-screen. Two real
  executables in LAN loopback (`SNESRECOMP_NET_TEST_NAMES`, Battle route, 7,600 frames): both peers logged the tags
  shown at Battle start and hidden after the seats died, with 251 identical WRAM samples. The harness's frame-count
  assertion failed on this 2-CPU container for the guest (51 frames behind the host-authoritative host); the
  unmodified harness without the feature failed the same way (5,620/7,600), so it is the machine, not a regression.
- Death/respawn (Linux LLE, Zone 1-1 route, P1 drops a bomb with `a` and stays on it, dumps every 15-20 frames):
  tag shown while alive (`$15A2` = `01`); hidden from the explosion frame (`$15A2` = `81`, object still `08`) through
  the object's removal (`$17FA` = `FF`), the life charge (`$1D23` 3 → 2) and the stage restart (`$0AEE` = 0, the
  new object `08` already exists with status `05`); shown again once `$0AEE` = 1 (status `05`, then `01`). A
  composed preview of four of those frames matches. Battle deaths hide it too (pair run above); Battle has no
  respawn. Tag bitmaps are 75% opaque (alpha 191, checked by `ingame_chat_tests`, which also checks the viewport
  mapping and that moving a tag reuses its bitmap). Both tests pass under gcc ASan/UBSan.
- Spectators: the lobby server (`RetroPortingToolKit/recomp-net-server`, `handle_join`/`seat_joiner_locked`) seats a
  joiner in a started room (free seat or gallery) and never sends the match state to clients, so a mid-match joiner
  stays in the room and chats. Client bug fixed in the recomp-net fork: any BYE set `peer_gone`, so a spectator
  quitting ended the match. New `session_state_test` case fails on the old code and passes on the new one; the
  pre-existing "4-seat liveness" timing case fails on this container with and without the change;
  `session_state_multipeer_test`, `session_host_authority_test`, `lobby_client_test` pass. Not run with a real
  relayed spectator.

## v2.0.8 — 2026-10-22

- Netplay mod agreement (recomp-ui fork `recomp_netplay_host.c`): guests adopt the host's effective mod set in the lobby
  and again right before launch (`cb_fill_launch` → `mod_set_sync_step`), and adoption switches off every feature the
  host does not run before enabling the host's lines; the game then commits that selection at startup
  (`host_main.c` → `snes_mod_runtime_commit_c`). Gap found: a host with no mods publishes an empty `mod_set` and an
  empty plan online, which `room_want_set` read as "nothing to compare", so a guest kept its own mods and the launch
  gate passed. The empty set now reads as `(none)` and is adopted. Adoption no longer caches successes, so a guest
  who re-enables a mod in the lobby is realigned instead of refused at Start; only failed sets are cached. A LAN
  `(plan-too-large)` room holds the launch instead of being adopted as an empty set. Presentation-only features stay
  per-player only when the host grants them (SB5 grants none). Evidence: `netplay_host_test` (Linux, recomp-net
  pinned) with three new fake-runtime cases plus all existing cases passing. Not yet verified in a real
  online/LAN match. Adoption is persisted as the guest's Mods selection (existing behaviour).

### Launcher on OpenGL 3.0-3.2 and without OpenGL

- Player crash report (v2.0.6 setup host, Windows 10.0.26200, i5-2400 / Sandy Bridge, no vendor OpenGL ICD
  loaded, `D3D10Warp.dll` present): access violation reading `0x2C` 720 ms after "launcher: opening GUI".
  Minidump code bytes at exe+`0x2765AF` decode to imgui `ImGui_ImplOpenGL3_DestroyFontsTexture`
  (`bd = io.BackendRendererUserData` = NULL, `cmp [bd+0x2C]` = `FontTexture`), called from the launcher's font
  rebuild. Cause: without `WGL_ARB_create_context` SDL returns a legacy GDI OpenGL 1.1 context for the 3.3 core
  request instead of failing; `ImGui_ImplOpenGL3_Init` then fails in its loader (return value ignored) and never
  installs its backend data.
- recomp-ui fork: the launcher asked for a 3.3 core context only, although imgui 1.91.9's OpenGL3 backend supports
  GL 3.0 with GLSL 130 and checks its 3.2/3.3 features at runtime. The SDL3/SDL2 platform layers now try 3.3 core,
  then any 3.0+ context (Intel Sandy Bridge drivers stop at 3.1), record the version and pick `#version 330` or
  `#version 130`; a context below 3.0 is refused (launcher UNAVAILABLE instead of the crash). The font rebuild
  skips an uninitialized GL backend. snesrecomp fork: a setup host whose launcher is UNAVAILABLE explains the
  OpenGL 3.0 requirement (driver / Mesa3D software `opengl32.dll`) instead of SnesInit's "start it with --launcher".
- Evidence: launcher sources syntax-checked against SDL 3.4.10 and SDL 2.30 headers (SDL2 with and without
  `LNG_GLES2`). Linux runtime harness with the real `launcher_platform_sdl2.c` + imgui OpenGL3 backend on Mesa
  llvmpipe/Xvfb: default driver -> GL 4.5, `#version 330`; `MESA_GL_VERSION_OVERRIDE=3.1` + GLSL 1.40 -> 3.3 core
  refused (GLXBadFBConfig), 3.1 context, `#version 130`, font rebuild + 3 frames rendered (text and button
  visible, `glGetError` 0); override 2.1 -> both requests refused, UNAVAILABLE. Not yet built on Windows nor run on
  the player's Intel driver; the full launcher (box art, emoji atlas) was not exercised on 3.1. The crash dump shows
  no Intel OpenGL ICD loaded, so that machine also needs Intel's driver installed for OpenGL 3.1.

## v2.0.7 — 2026-10-22

- 2.0.6 still ran a three-player internet match at 27.6 fps (778 frames / 28.2 s, 128 `wait_confirm` streaks at
  D=7): lockstep admits a tick only after every peer has every input AND every peer's confirmation, which
  travels guest -> host -> guest, and a guest slower than 60 fps sets everyone's pace.
- New host-authoritative delay-sync (recomp-net fork `RNetConfig.host_authority`, snesrecomp fork
  `SNES_NET_HOST_AUTHORITY`, set by SB5 on every peer): seat 0 never waits; each tick it uses its live pad and
  the newest input received from each guest (repeating the last), and sends the resolved row packed in its own
  input sample; guests simulate only those rows. No confirmation barrier, no neutral prefix.
- Evidence: recomp-net `session_host_authority_test` (host + fast + 1/3-speed guest over loopback: 300 host
  ticks, 0 stalls, identical rows on all peers, guest input present); Linux LLE pair runtime with two real
  executables, Battle with pseudo-random inputs, 2400 frames at ~59 fps, 79 identical WRAM samples without
  mods and 79 with every mod (`host_authority=1` in both logs). Not yet verified: 3-5 players over the
  internet. A guest that falls more than ~1 s behind the host cannot catch up (resend window).

## v2.0.6 — 2026-10-22

- 2.0.5 started a three-player internet match (host + guests at ~470 ms and ~60 ms RTT) but it ran at 28.6 fps:
  813 frames in 28.4 s, `delay_sync_starvation` every 4-8 frames, 237 `wait_confirm` stall streaks (max 192 ms)
  at D=7 (`SNES_NET_DIAG=1` net_diag.jsonl + host log). The host's auto D used the rollback tiers (UI toggle)
  although SB5 forces delay-sync, and only the worst single host RTT although guests exchange input through the
  host. recomp-ui fork now uses the delay-sync formula when `SNES_NET_MODE=delay` and the sum of the two worst
  RTTs with 2+ guests. Not yet re-measured in a real match.

## v2.0.5 — 2026-10-22

- With 2.0.4 a four-player room stayed open, but PLAY failed: host log "ice_hub: no connected agents to take
  (slot 0 of 2)" then "snes_netplay_start failed (-3)"; guests saw a black screen.
  `snes_host_app_apply_launch` never copied the launch's `player_count` into the netplay config, which kept its
  default of two seats. Now it does (snesrecomp fork, `snes_host_app.c`). Evidence: the maintainer's host log and
  a build. Not yet verified: a 3-5 player internet match (ICE hub with more than one guest).

## v2.0.4 — 2026-10-22

- Fix: online rooms closed when a third player joined (maintainer's 2.0.3 host log: ICE to the first guest,
  then a silent lobby reconnect). recomp-net read server frames into a 4 KiB buffer; a lobby_update repeats
  every member's mod offer (14 SB5 packages each), so the third member's update filled it and the pump
  disconnected. The buffer is 64 KiB in the recomp-net fork (`MauricioSuporte/recomp-net`, branch
  superbomberman5, `27ab002`), pinned by the snesrecomp fork (`9fb4eb5`). Evidence: new lobby_client_test case
  (5 members x 14 offered packages, ~8 KiB frame) fails on the old buffer and passes on the new; game builds.
  Not yet verified: a real room with 3-5 players.

## v2.0.3 — 2026-10-22

- Launcher header shows the installed version (`VERSION`, else the build's lobby version). On start a background
  thread runs `updater --check` (hidden, no install): with a newer release it says so above the Update button,
  shows a warning on the Netplay pages (rooms only match the same version) and disables Host Lobby (recomp-ui
  fork `829e48a`: `GameInfo.game_version_label` / `netplay_notice`). Evidence: Linux Xvfb launcher with VERSION
  2.0.1 against the published v2.0.2 showed "v2.0.1" in the header, the update note and the Netplay warning.
  Not exercised: the disabled Host Lobby button (needs a signed-in lobby page) and the Windows check process.
- Discord announcements carry only the new version's changes (`release-notes.py --whats-new`).

## v2.0.2 — 2026-10-22

- Online netplay rooms have five seats. The maintainer's v2.0.1 lobby showed "1 / 4 seated": the framework
  clamped SNES lobbies to a fixed four (`SNES_LOBBY_MAX_PLAYERS`) although recomp-net carries eight; the
  ceiling is now `RNET_LOBBY_MAX_PLAYERS` and the game's `num_players = 5` picks the size (fork snesrecomp
  `66e3f79`). Evidence: the setup host reports "5-player rooms online" on start. Not yet verified: that the
  shared lobby server accepts a five-seat room, and any match with three or more peers.
- Updater temp cleanup: each run deletes its `sb5-update-<pid>` folder (download + unpacked package) on exit,
  removes leftovers of earlier runs at start, and on Windows a detached `cmd` removes the folder of the
  running temporary copy after it exits. Verified on Linux by a real 1.1.0 → 2.0.1 AppImage update with a
  stale 5 MB folder present: the temp root was empty afterwards. Windows: cross-compiles without warnings;
  not yet run on a real install.
- Online rooms closed when the guest's mods differed from the host's. Cause: with about a dozen SB5 packages
  on, the host's exact mod set (~600-700 bytes) exceeds the 512-byte `mod_set` wire field, so recomp-ui
  published it empty and the guest never adopted the host's configuration. The guest now rebuilds the set
  from the plan (package rows with enabled features; SB5 mods have no options, and the plan follows the same
  selection/feature order as the runtime's effective set) for adoption and the launch gate (fork recomp-ui
  `2f07062`). Evidence: maintainer confirmed rooms stay open with identical mods; code builds on Linux and
  Windows. Not yet verified: a real join with different mods. LAN rooms keep the 512-byte limit
  (recomp-net `RNET_LAN_LOBBY_MOD_SET_MAX`, "(plan-too-large)").

## v2.0.1 — 2026-10-22

- Fix: Windows **Generate & rebuild** in a folder whose path has accents. The maintainer's v2.0.0 install in
  `OneDrive\Área de Trabalho` stopped at "Configure failed … does not appear to contain CMakeLists.txt": the
  framework's UTF-8 `activeCodePage` manifest makes `CP_OEMCP` 65001 inside the process, so the rebuild
  helper wrote the path in UTF-8 while `cmd.exe` read it as CP850, and the junction under
  `C:\Users\Public\retcomm-builds` pointed at "├ürea de Trabalho". The helper now converts paths to the
  system OEM code page (registry `Nls\CodePage\OEMCP`, locale fallback) and relinks a junction whose target
  has no `CMakeLists.txt` (fork snesrecomp `e24e6d6`).
- Evidence: the failing helper and dangling junction inspected on the maintainer's machine; the fixed source
  cross-compiles for Windows without warnings and the Release CI builds it. A Windows rebuild from an accented
  folder with v2.0.1 still needs confirmation on real hardware.

## v2.0.0 release / Release v2.0.0 - 2026-10-22

New in v2.0.0 (evidence and limits; details in each linked document):

- **Framework:** forks rebased onto the revisions Mega Man X Recompiled 2.3.1 pins (snesrecomp
  `7a67b85`, recomp-ui `7c635c6`), branch `superbomberman5-v1.2`; multitap seat routing is upstream now.
  Setup host built on Linux; all native tests pass (17 with `updater_tests` and `infinite_lives_tests`).
- **Cheats group:** infinite time moved from Gameplay to **Cheats**; new **infinite lives**
  ([INFINITE_LIVES.md](INFINITE_LIVES.md)). Linux LLE headless route: lives 0 + forced time-over in
  Zone 1-1 with 1, 2 and 5 players: Game Over without the mod (mode 8), stage restart with lives 0 with it.
- **Netplay** ([NETPLAY.md](NETPLAY.md)): recomp-net with ICE, forced delay-sync. Two real executables on
  loopback stayed byte-identical in WRAM for 7,600 frames (route to Battle, ~1,700 Battle frames,
  pseudo-random pads, every gameplay mod). Rollback forked (PPU/VRAM) under forced mispredictions; the
  beam driver's resume PC/WAI latch and the raster journal now ride in rollback snapshots (fork), but the
  in-process probe still reports VRAM divergence in 7 of 207 probes. Not done: real internet match,
  three or more peers, Windows netplay run.
- **Discord:** lobby Discord sign-in (framework) seen in the launcher under Xvfb; launcher links to
  <https://discord.gg/kH54NCDQB4> on the main page and the Netplay pages (recomp-ui fork:
  `GameInfo.game_links`/`game_action`).
- **Box art:** maintainer-supplied cover staged as `assets/img/boxart.tga` (provenance in
  `assets/README.md`); seen in the launcher.
- **Updater** ([UPDATER.md](UPDATER.md)): Linux AppImage update verified end to end against the published
  v1.1.2 (`SHA256SUMS` checked); Windows ZIP path cross-compiles (MinGW) and builds in CI, not yet run on
  a real Windows install.
- **Quiver Launcher** ([QUIVER.md](QUIVER.md)): one root executable (updater in `updater/`), catalog entry
  submitted for review on quiverlauncher.com.

Na v2.0.0: framework nos pins do MMX, grupo Cheats com vidas infinitas, netplay (delay-sync, validado com
dois peers no loopback), Discord no launcher, capa, atualizador (testado no Linux) e Quiver. Partida real
pela internet, 3+ peers e o atualizador no Windows ainda não foram testados.

## v1.1.2 release / Release v1.1.2 - 2026-10-22

v1.1.2 packages the five-player fixes logged below since v1.1.1 (ZONE5 - J palettes, Zone 5 slowdown, ZONE5 - 3 gate, seat colors, walking blink, tower shocks, platforms, lives reset, rejoin with a Louie, ZONE3 - D rival state) and picker Back / world-map repick. Evidence: Linux LLE scripted routes and native tests listed in each entry, plus the maintainer's confirmation on a Windows build; CI builds both platforms before publication. The upstream `continuous` AppImage type2 runtime was republished (old pin no longer downloadable), so the workflow pin moved to SHA-256 `617aa773…ed98a46` (same MIT-licensed project, same URL).

A v1.1.2 traz as correções de cinco jogadores registradas abaixo desde a v1.1.1 e o B no seletor/mapa. O README só muda os links de download.

## Five-player follow-ups: palettes, page refills, mounts, lives / Ajustes de cinco jogadores - 2026-10-22

Reported after the 2026-10-17 fixes: a one-frame deformed bomber when tapping
a direction (ZONE5 - 1, also elsewhere), ZONE5 - 1's tower shock paralysing
only P1/P2, P3-P5 in P1's white palette in many stages, shared lives kept
after leaving the world map for the title/file menu, and ZONE5 - 25's slow
platforms carrying only P1/P2.

Causes and changes (Linux LLE headless, forced stages, lab-only hooks):
- **Palettes.** Seats 3-5 had moved to slots 6/5/4, inside the enemies'
  range, so most stages evicted P4/P5 at once. The real hazard of slot 7 was
  the 33-byte blink copy at `$C1:0015/$0035`; co-op now makes it 32 bytes and
  seats use 7/6/5 again. A borrowing seat takes its slot back once the stage
  frees it (every 16 ticks; colors copied like `$C0:01C8`). 12-stage sample at
  frame 1500: all of P3-P5 keep their own slots in 8 stages; in stages `$1B`,
  `$2B`, `$49` and `$61` some borrow P1's, as the stage needs those slots. ZONE5 - J
  stays clean over ten 150-frame captures (P3 in slot 7).
- **Page refill.** With five seats a player's page is refilled while still on
  screen (`$C5:7120`), so the old frame showed the new tiles for one frame.
  A refill for different tiles now waits one frame with the object hidden
  via `$085F` (`$C5:7140` hook). A first version hid the bomber on every
  step (walking re-requests the chunk already loaded, every frame); requests
  for the chunk a page already holds now skip the upload. Frame-by-frame
  P3 walk in ZONE1 - 1: one hidden frame at start, stop and turn, none in
  30-40 walking frames, no garbled frame.
- **Start rejoin with a Louie on the field.** The rejoining seat found no
  free page (the left seat's page stayed `$7F`) and animated without tiles.
  Pending pages are now freed and the request retried next frame. Route
  (ZONE1 - 1, five seats): P3 hatches a Louie, P4 dies and rejoins: pages
  `08 0a 0c 7f 10` → `08 0a 0e 0c 10`, P4 drawn correctly.
- **ZONE3 - D → world map → next stage.** Benched P4/P5 fields were saved
  after the boss start had seated the rival (riding bit, Black Louie) in
  P4's seat; P4 kept them, and `$101F` still flagged P4/P5 as rivals. Saved
  at `$C4:3C60` now, flags cleared at `$C4:28B4`. Route: forced `$6A`, pause
  → world map → `$35`: P4 `$1782` = 0, no mount, `$101F` clear; ZONE3 - D
  still flags its rivals. P4 still borrows P1's palette in `$35` because all
  eight OBJ slots are in use there (stage id `$31` holds slot 6).
- **Normal picker Back.** Only P1's B was intercepted; P2-P5's B ran the
  native picker exit and the suspended file confirmation continued to the
  world map. Any seat's B now returns to the file menu (native picker never
  sees B), and B on the world map before any stage reopens the picker from
  the kept snapshot. Routes (2 seats): P2 confirm + B → file menu; confirm
  both → map → B → picker → re-pick → map → stage `$0D` with the new
  choices.
- **`$1CBF`.** The 2026-10-17 layout set `$1CBF` = 19, but it is also the
  bomber-slot count read by survivor counts, player-on-cell searches and bomb
  owners, which then included enemies 5-18. `$1CBF` is 5 again; the 28 offset
  readers use `$011E` = 19. Five-seat egg pickup: P3's Red Louie hatches in
  actor 21, slot 7, and is ridden.
- **Seat loops.** `$C0:2DB0`, `$C1:19AA`, `$C1:5D73` (tower shock: `$15BA`
  bit 7 on P1-P4 at frame ~1110 of a ZONE5 - 1 run) and `$C1:6B32` (platform
  carry) cover the roster. Teleporting P3 onto ZONE5 - 25's platform: P3 now
  moves with it (cell `$66`→`$68`), as P1 did before.
- **Lives.** The pause menu's world-map return kept lives/fields until the
  next player reset, even after the title or a New Game. They are dropped at
  the title and at a file-menu New Game/Continue (native tests).

Evidence: all 14 native test executables, `tests/normal_coop_runtime.py`
counts/five/lifecycle and the routes above pass; stage sweep below. Not
checked in a played Windows campaign.

Ajustes de cinco jogadores: P3-P5 voltam às paletas 7/6/5 (a cópia de piscar
agora tem o tamanho certo) e recuperam as cores quando a fase libera a paleta;
o quadro deformado ao andar virou um quadro invisível; `$1CBF` volta a 5 (a
montaria usa `$011E`); choque da torre e plataformas da ZONE5-25 valem para
P3-P5; vidas não voltam mais depois de sair do mapa para o título/arquivos.

## Five-player stage objects: actors, palettes and pages / Objetos de fase com cinco jogadores - 2026-10-17

Reported with the five-player mod: garbled egg/Louie sprite, ZONE5 - 1's
electric tower without its core and beams (and a freeze after destroying it),
slowdown in Zone 5, broken gimmicks (5-24 platforms, ZONE2 - 6's fire rail,
a red block at ZONE3 - 9's sand pits), no exit gate in 5-3 and a garbled,
slow first form of the final boss.

Causes found (Linux LLE headless, forced stages, lab-only allocator and
script hooks under ignored `.bootstrap/`):
- **Actors.** Co-op moved enemies to actor 10+ so P1/P2 mount backups could
  live at 5-9. Stage scripts name enemies by absolute actor: the tower core
  script (`$C6:CE94`-`$CEEE`) attaches to actor 9, which was empty. Enemies now
  keep the cartridge's numbering (5-18, allocator capped at 19) and the mounts
  move to rider + 19 (`$1CBF` = 19); point damage (`$C0:374D` → `$C2:4350`)
  skips the mounts. ZONE5 - 1 with five seats: tower actors 9/10 as in 2P.
- **Palettes.** Eight OBJ slots: 0 stage, 1/2 P1/P2, 3-5 enemies, 6-7 large
  stage objects. Seats 3-5 held 7/6/5, so the tower's two-slot palette `$38`
  (mask `$C0`) failed every frame (`$C0:0303`). Failed non-player requests now
  take slots held by seats 3-5 when that makes them fit (those seats share
  P1's slot and colors until the next stage); a seat without a slot shares
  P1's; a sharing seat does not write its colors into P1's slot (`$C4:37B0`).
  ZONE5 - 1: tower core and beams created (10 script objects, as in 2P), no
  palette failure; destroying the tower actors kept the main loop running.
  Seats 3-5 now use slots 6/5/4, never 7: the palette save/restore used when
  objects blink (`$C1:0015/$C1:0035`) moves 33 bytes, so for OBJ palette 15
  it overwrote `$2B10` (slot 0's palette id) and `$2EEA` (object 0's frame
  pointer). With P3 in slot 7, ZONE5 - J's first form drew garbage pieces,
  filled OAM (P2-P5 vanished) and ran at 0.49 game ticks per frame (2P:
  0.89); now 0.88 and clean through ten 150-frame captures. Found by
  disabling co-op patch groups one by one and a WRAM watch on `$2B10`.
- **Mounts.** `$C4:53F0` gave a mount slot = rider seat + 1, i.e. an enemy
  slot for seats 3-5. It now uses the rider object's slot (`$C4:5B40`).
- **Pages.** Five seats own the five animation pages; no 1 KiB of OBJ VRAM is
  free in campaign stages. Co-op records refused page requests (`$C5:714E`);
  the page-less overlay draws them from the cartridge sheet (hidden when too
  large). The mount overlay now matches the mount's own pieces, so the
  jump onto a Louie (rider 24 lines up) no longer leaves garbage.

- **Distance arrays.** The five-entry enemy→player distance copies sat at
  `$1FC0-$1FDD`, inside the flame-thrower's segment-object table (`$1FAB,Y`)
  and the actor array `$1FBB,X`. In ZONE5 - 27 the distances overwrote flame
  object ids, the flame deleted P4's object (`$C3:2F08` → `$C0:1342`) and the
  object list (`$043B`) became a cycle: the animation pass ran ~170 times per
  tick (0.43 ticks per frame) and the stage looked frozen. They now live at
  `$0100-$011D` (the stack never went below `$01BB` in 1263 lab dumps).
- **Enemy cost.** `$C3:FF70` (clear + `$C3:46CC` for seats 3-5, per enemy) is
  run natively by a pre-opcode hook that writes the same bytes, scratch,
  A and flags and resumes at its RTS. A lab shadow check (native result
  saved, interpreted code run, compared at the RTS) matched on ~24k calls
  in ZONE5 - 22/27 except bytes owned by interrupts (`$6D`, one NMI frame).
- **Cell attacks.** `CPY #$02` seat loops over `$16AA,Y` at `$C3:15BF`,
  `$1937`, `$27AE`, `$296B`, `$2EE8` (flame columns), `$3CA6`, `$48AD` now
  cover the roster, so those attacks reach P3-P5.
- **HUD faces, ZONE5 - 3.** The gate archive `$3A` failed to load at stage
  start with faces on (also with one player) and is never retried. The
  failure exit now deletes the faces and re-enters the allocator
  (`$C5:7360`); the static archive table then matches a run without mods.

Evidence (Linux LLE headless, forced stages, 1906 frames per run, ticks =
main-loop `$C0:0B3E` per NMI `$C0:F431`): all 47 stages `$49-$73` plus
`$21/$33/$35/$43`, 2P vs 5P co-op: no stage more than 0.02 below 2P (worst
`$6F/$70` 0.877/0.890 vs 0.894/0.908); before, `$58` 0.49, `$5A` 0.61,
`$63` 0.46, `$6F` 0.49, `$43` 0.73. No palette, page or static-allocation
failure and no BRK/interp_cap in those runs. Native tests (all 14) and
`tests/normal_coop_runtime.py` counts/five/lifecycle pass (the five case's
palette check now uses each seat's confirmed color, `$6264`; it failed the
same way on v1.1.1). Egg pickup with five seats (with and without Extra
Characters) shows a clean Louie through the jump; ZONE3 - 9's sand pit sink
shows no red block; ZONE2 - 6's fire rail and 5-24's platforms run like 2P
in idle captures, but a bomb on the rail was not scripted. Not checked in a
played Windows campaign or with five physical controllers.

Objetos de fase com cinco jogadores: inimigos voltam à numeração original
(scripts chamam atores pelo número), a fase tem prioridade nas paletas
(P3-P5 podem ficar com as cores do P1 enquanto um objeto grande precisa da
paleta deles; P3-P5 nunca usam a paleta 7, cuja cópia ao piscar estragava o
chefe final), o Louie usa a paleta do cavaleiro, objetos sem página de
animação são desenhados a partir da ROM, as distâncias inimigo→jogador saem
da tabela do lança-chamas (que congelava a 5-27) e são calculadas em código
nativo (Zona 5 sem lentidão extra), ataques por casa atingem P3-P5 e, com os
rostos na HUD, o portal da 5-3 volta a aparecer.

## v1.1.1 release / Release v1.1.1 - 2026-10-17

v1.1.1 packages the ZONE3 - 1 exit-gate fix for HUD faces (below) on top of v1.1.0. The README only changes its download links; the fix is in the release notes, docs/NORMAL_HUD_FACES.md and this log. Evidence is the Linux LLE forced-stage routes and 14 native tests listed below; CI builds both platforms before publication. Not checked in a played Windows campaign.

A v1.1.1 traz a correção do portal da ZONE3 - 1 com rostos na HUD sobre a v1.1.0. O README só muda os links de download.

## ZONE3 - 1 exit gate with HUD faces / Portal da ZONE3 - 1 com rostos na HUD - 2026-10-17

Reported after v1.1.0: no exit gate in ZONE3 - 1 (`$2B`) after all enemies were
defeated. Same cause as the ZONE3 - D fix, in a regular stage: when the last
enemy dies its archive `$13` is released and the gate's archive `$63` must take
its static OBJ VRAM, but with face archive `$64` resident `$C0:18FC` fails
(`$C0:1A0A` carry → `$C0:193C`), the gate stays queued in `$7E4DA3` (`$41`)
and its object never gets a frame. The queue alone is no signal (ZONE1 - 1
keeps its covered gates queued until revealed). Pre-opcode hooks on the
allocator's failure exits `$C0:193A/$193C` now flag a failure for any archive
other than `$64`; the HUD mod then runs one HUD tick through `$C5:7300` (free
FF padding, saved/restored) that deletes each live P1/P2 face object with the
native `$C0:1342` (object, archive and palette) and sets `$35C5+seat` = `$FF`,
restores its patches and keeps the stock heads until the stage id changes.

Evidence (Linux LLE headless, local only): forced `$2B` from the first-stage
route, native restart, P1 invulnerable, enemies 5-8 killed with `$15A2` bit 6.
No mods: gate object at screen (216,68) = cell (13,2), frame 22. HUD faces
before the fix: gate object at frame 0, queue stuck at `$41`. After the fix
(HUD faces alone and with co-op, PT-BR, portal labels, pause, Louie, infinite
time, Gold Battle, corner spawns): gate visible at (216,68), faces released.
ZONE1 - 1 with the same mods: three gates revealed (frame 17) and the P1 face
kept. All 14 native tests pass (new allocator-hook, release-routine and
suspension cases). Not verified: Windows build, a played route, other regular
stages (which ones hit the limit is unknown; the fix applies to any).

O portal da ZONE3 - 1 não aparecia com os rostos na HUD: o gráfico do rosto
ocupava a VRAM do portal. Agora, se o jogo não consegue carregar um gráfico por
falta de espaço, os rostos saem até a próxima fase e o portal aparece.
Validado no Linux com a fase forçada; falta conferir no Windows.

## v1.1.0 release / Release v1.1.0 - 2026-10-17

v1.1.0 packages the spawn change below on top of v1.0.3 and adds a thirteenth optional mod. README, PUBLISHING and the release notes list it. Native coop test built and run with gcc on Linux; the maintainer confirmed the new spawns in a Windows build of his working tree. CI builds both platforms before publication.

A v1.1.0 traz a mudança de spawn abaixo sobre a v1.0.3 e um 13º mod opcional. O mantenedor confirmou os spawns novos num build Windows.

## Five-player spawns beside P1/P2, corner layout as a separate mod / Spawns junto de P1/P2, layout dos cantos como mod separado - 2026-10-17

Requested by the maintainer. The five-player mod moved to its own **5 Players**
group on the Mods page. In regular campaign stages P3 and P5 now start next to
P1 and P4 next to P2 (P3/P4 one tile beside the cartridge spawn of P1/P2, P5
one tile above/below P1, each toward the middle of the field), still through
the nearest-safe-floor search and the post-map settle at `$C5:3D5D`. The old
layout (P3 lower left, P4 lower right, P5 centre) is the new optional package
`superbomberman5.coop-corner-spawns` (feature `corner_spawns`, same group),
which only works with the five-player mod. Boss layouts are unchanged in both
modes. `normal_coop_tests` covers both layouts, inward stepping from opposite
corners, a blocked neighbour and the unchanged boss pattern. The maintainer
checked the result in his Windows build; no scripted runtime route was added.

Pedido do mantenedor: P3 e P5 nascem junto do P1 e P4 junto do P2; o layout
antigo dos cantos virou um mod separado no grupo **5 Players**. Conferido pelo
mantenedor no Windows; teste nativo cobre os dois layouts.

## v1.0.3 release / Release v1.0.3 - 2026-10-17

v1.0.3 packages the two fixes below on top of v1.0.2: B + direction abilities in every Battle arena (they only worked in arena 1) and the five-player slowdown caused by Normal Game HUD faces. The README only changes its download links; the fixes are in the release notes and this log. Verified on Linux LLE headless with the release commit: 14 native tests, `tests/normal_hud_runtime.py`, `tests/normal_hud_coop_runtime.py --players 5`, a five-seat Zone 1-1 lag measurement (4.0% of 1,808 frames with all gameplay mods) and a five-seat Battle route in Gold arena 13 with abilities firing. Not checked in a played Windows campaign or with physical controllers.

A v1.0.3 traz as duas correções abaixo sobre a v1.0.2: habilidades de B + direção em todas as arenas do Battle (só funcionavam na arena 1) e a lentidão com cinco jogadores causada pelos rostos no HUD do Normal Game. O README só muda os links de download. Verificadas no Linux com o commit da release; ainda não numa campanha jogada no Windows.

## Battle abilities in every arena / Habilidades do Battle em todas as arenas - 2026-10-17

B + direction abilities (Honey, Kotetsu, Terrorin, Bomber Kings) only worked in Battle arena 1: `in_stage()` in `src/special_abilities.c` required `$1031 == $74`, but each arena runs as mode `$74+4*(arena-1)` (arena 5 = `$84`, Gold arena 13 = `$A4`). The Battle check now accepts `$74-$A7`; the picker (mode 4) still is not a stage. Evidence (Linux LLE, JPN ROM, local): native special-abilities test extended with modes `$84`/`$A4` (start) and `$A8` (no start); headless five-seat Battle routes in arena 13 (Bazooka, Jet, Hammer, Lady, Great) and arena 5 (Honey, Kotetsu, Terrorin, two Subordinates) recorded ability states `$7F:F844+32*seat` going non-zero, where before the fix they stayed 0 for 24 s. Windows build and physical play not checked.

As habilidades B + direção só funcionavam na arena 1 do Battle: cada arena roda num modo próprio (`$74+4*(arena-1)`) e o mod exigia `$74`. Agora valem em todas as arenas, inclusive as do Gold Cartridge. Verificado no Linux LLE; Windows não testado.

## v1.0.2 release / Release v1.0.2 - 2026-10-17

v1.0.2 packages the three fixes below on top of v1.0.1: ZONE3 - D exit gates with HUD faces enabled (stock HUD icons in boss stages), ability hits on rival bomber bosses through their fire test (Louie, then HP) and Jet's trail at the obstacle, and PT-BR menu labels kept off stage arenas. The README only changes its download links; the fixes are in the release notes and this log. The fixes were verified on Linux LLE headless with forced-stage routes and native tests, not in a played Windows campaign.

A v1.0.2 traz as três correções abaixo sobre a v1.0.1. O README só muda os links de download; as correções estão nas notas da release e neste registro. Verificadas no Linux com rotas de fase forçada e testes nativos, ainda não numa campanha jogada no Windows.

## PT-BR menu label over a boss arena / Rótulo de menu sobre a arena do chefe - 2026-10-17

Reported: "Continuar" appeared over the blocks of ZONE2's boss arena (`$68`).
Menu labels were matched only by two BG tile ids at a fixed map position;
that arena has tiles 328/329 at the file menu's "Continue" position (BG1 row
13, column 12). While a stage is played (mode `$0D`-`$73`, or Battle `$74`+,
with `$0AEE` set or a bomber on the field) only the HUD labels (BG3 row 0)
apply now. Linux LLE headless, all of the maintainer's mods plus PT-BR,
forced `$68`: the old code draws "Continuar" in the arena, the new one does
not; HUD "APERTE START" and the Normal file menu labels still appear. Native
render test covers stage, intro, players-gone and menu cases. Windows not
built here.

O "Continuar" no meio da arena do chefe era o rótulo do menu de arquivos
encaixando por acaso nos tiles da arena; durante a fase só os textos da HUD
são traduzidos.

## Boss gates with HUD faces, abilities against boss rivals / Portais dos chefes com rostos na HUD, habilidades contra chefes - 2026-10-17

Reported: after ZONE3 - D (Pirate Bomber and partner) was won as Jet, no exit
gates appeared; Jet's rocket did not hurt bosses and its trail looked shorter
than in Super Bomberman 4.

Gates: when the last rival dies in `$6A` (`$C4:3DA6`, `$102F` = `$6A` branch,
`$1D50` = 0) the gate objects queued in `$7E4DA3` are created by `$C5:3B60`
→ `$C2:265C`; with the HUD-faces mod the face archive `$64` held static OBJ
VRAM (`$7E30AA/$30EA/$314A`), the gates' archive did not fit (`$C0:18FC` →
`$C0:1A0A` fails), and the gate object was created and freed every frame.
HUD faces now stay off in boss stages `$67`-`$73` (stock heads there).

Bosses `$67`-`$6E` are rival bombers in slot 2 with `$101F` set; their update
clears `$15A2` bit 6 every frame (`$C4:0164`), so ability hits (which set that
bit) did nothing. Those hits now force the rival's own fire test (`$C4:2CFE`,
A |= 4 once): Louie lost first (`$C4:2D05`), then HP `$1662`, 120 frames of
invulnerability, death and the boss ending. Abilities also no longer hit a
Louie actor (rider + `$1CBF`), which had set its death bit during the loss.
Jet keeps spawning trail pairs for 20 of his 24 frames against the obstacle
(SB4 arena tilemap `$7E2000`: new pairs until ~20 frames after the stop);
flight speed, pair size (2 cells) and life (16 frames) already matched.

Evidence (Linux LLE headless, local research hooks not committed): forced
stage `$6A` from Zone 1-1 with `$102F` = `$6A`. Native kills (no mods)
show the three gates; with all of the maintainer's mods they never appeared
before the fix and appear after it (same route, leave-one-out isolated
hud-faces). Jet (choice 15) with all mods dismounts and kills the Pirate
Bomber and kills the partner through the native fire path; gates with labels
`Z4-2/3/4` appear. All 14 native tests pass (new rival/Louie and boss-HUD
cases). Not verified: Windows build, a played (not forced) route, other boss
stages at runtime, co-op boss fights with abilities.

Portais: com o mod de rostos na HUD, o gráfico do rosto ocupava a VRAM dos
portais e no ZONE3 - D eles não apareciam; nas fases de chefe a HUD fica com
os ícones originais. As habilidades agora machucam os chefes bombers (primeiro
o Louie, depois a vida, como o fogo de uma bomba). O Jet continua soltando
fogo para trás enquanto está parado no obstáculo, como no SB4. Validado no
Linux com a fase forçada; falta conferir no Windows.
## Five-player slowdown from HUD faces / Lentidão com cinco jogadores e rostos no HUD - 2026-10-17

With **Normal Game HUD faces** on, the per-tick update at `$C5:72B0` called the full `$C5:7200` entry every frame, and that entry starts with the lives refresh `$C5:0C86`. With five players in Zone 1-1 this made the main loop miss 24% of frames (HUD faces + co-op + picker; 9% with all gameplay mods), against 0.3% with one player and no mods. The tick now runs the full entry on one tick in eight (main-loop counter `$69`) and only the face checks (`$C5:7204`) on the others. Skipping the refresh entirely was tried and rejected: the P2 score box then lost its frame and digits. Evidence (Linux LLE, JPN ROM, local): lag measured as frames where `$69` did not advance, over 1,808 Zone 1-1 frames with five bot-driven seats: 24.4% → 4.5% (HUD + co-op + picker) and 9.0% → 4.0% (all gameplay mods). Native HUD test checks the new branch; `tests/normal_hud_runtime.py` and `tests/normal_hud_coop_runtime.py --players 5` pass. Windows not checked.

Com o mod de rostos no HUD, a atualização de cada quadro chamava a rotina de vidas `$C5:0C86` toda vez; com cinco jogadores na Zona 1-1, 24% dos quadros ficavam lentos. Agora a rotina completa roda a cada 8 quadros e nos outros só a verificação dos rostos: 4,0% com todos os mods. Verificado no Linux LLE; Windows não testado.

## v1.0.1 setup pack / Pacote de setup v1.0.1 - 2026-10-16

The v1.0.0 Windows ZIP had two root executables, and Generate & rebuild failed to configure when the folder path had accents ("Área de Trabalho" reached cmd.exe as "┴rea"). v1.0.1 ships only SuperBomberman5Recomp.exe (the setup host itself; rebuild/relaunch stay on build/SuperBomberman5SNESRecomp.exe through SNESRECOMP_GAME_TARGET) and the framework's rebuild helper writes paths in the console code page and builds non-ASCII folders through an ASCII junction under %PUBLIC%\retcomm-builds (CMake's own UTF-8 post-build scripts otherwise skipped the mods and launcher assets). Verified by the maintainer on Windows under OneDrive "Área de Trabalho": configure, build, 12 mods and assets staged, renamed setup host handing off to the rebuilt game. Published as the first full release: the maintainer has completed the campaign on Windows; mod combinations, physical five-player play and broad Linux remain unqualified.

O ZIP Windows v1.0.0 tinha dois executáveis na raiz e o Generate & rebuild falhava em pastas com acento. A v1.0.1 traz só o SuperBomberman5Recomp.exe; o rebuild compila pastas com acento por uma junção sem acento em %PUBLIC%\retcomm-builds. Verificado pelo mantenedor no Windows. Primeira release completa: o mantenedor zerou a campanha.

## v1.0.0 release / Release v1.0.0 - 2026-10-16

v1.0.0 packages the fixes below (Louies with five seats, identical controllers, ZONE3 - D co-op, mounted boss entry, portal labels after bosses, retired mod pruning, SuperBomberman5Recomp.exe only) and a shorter bilingual README. It stays a prerelease: the full campaign, all mod combinations, physical five-player play and broad Linux compatibility remain unqualified. The identical-controller seating was verified by code paths and build only, not with two physical DualSense pads.

A v1.0.0 reúne as correções abaixo e um README bilíngue mais curto. Continua pré-release: campanha completa, combinações de mods, cinco controles físicos e compatibilidade ampla no Linux não foram qualificados. O assento de controles iguais foi verificado apenas por código e build, sem dois DualSense físicos.

## Louies with five co-op seats and identical controllers / Louies com cinco jogadores e controles iguais - 2026-10-16

Five co-op seats own all five campaign OBJ pages (`$1D00-$1D04`, pool
`$1D10` = 5), so a Louie got none: the stock allocator left its request first
in the queue (`$C5:6AE3`, retried every frame), every later request - the
riders' animations - waited forever, and the Louie was invisible or drawn with
another page's tiles (the broken green Subordinate in the old README image).
Co-op now routes the page service through `$C5:7140`: a failed request starts
its animation without tiles and the queue moves on. The extra-character
overlay draws such a mount from the cartridge (sheet from `$C0:C415[type]`,
archive entry `$85:E22D`, raw 4bpp tile t at sheet offset t*32, verified
against an uploaded Black Louie page), hides the native leftovers by place and
keeps the native order (the object list draws a Louie facing down over its
rider). Linux LLE, five seats, Zone 1-1 egg pickups (Red, Yellow, Blue, Black
via the fixture build) match one-player captures in all directions; no
interpreter cap, BRK or fatal; Zone 3-D restart and ZONE4 - G mounted entry
re-run clean. The README image now shows green Subordinate on Black Louie with
Jet, Bazooka, Hammer and Terrorin using their abilities (fixture build, enemy
positions poked).

Controllers: the SNES launcher listed live pads once per SDL GUID and disabled
that entry for other players, so two identical controllers (two DualSense)
could not drive different players. recomp-ui `18cd477` lists and claims pads
per device and binds each GUID seat to one connected pad; snesrecomp `91052ad`
seats each pad on the device the launcher picked and no longer lets SDL's
preferred player index take an occupied seat. Built and unit-tested here; not
tested with physical controllers.

Com 5 jogadores os Louies aparecem e as animações não travam mais; dois
controles iguais (DualSense) podem ser escolhidos para jogadores diferentes.

## ZONE3 - D with co-op / ZONE3 - D no co-op - 2026-10-16

Reported with five players: P3 got a Red Louie in ZONE3 - D instead of the
Pirate Bomber riding the Black Louie; after everyone died P1 restarted on a
Red Louie with frozen animations, and the game froze when he died. The
maintainer's slot-3 state (frozen) shows the CPU looping in the object
allocator `$C0:12E3`: the priority list (`$049B` head, `$043B` next) had lost
its end marker `$62`. Cause: boss start `$C4:3C60` uses literal slots 2/3 for
rival equipment and the mounted rival and loads `$1CBF` = 4, while co-op moves
rivals to slots 3/4 and uses mount offset 5 (P3 marked riding, Black Louie type
in P2's mount slot). Co-op now shifts the four operands and loads 5 at
`$C4:3C75`. Linux LLE headless, five players, all gameplay defaults, forced
stage `$6A` from Zone 1-1: Pirate Bomber (type 4, slot 3) rides the Black
Louie (slot 8, type 6) as in the native one-player run; P3 is on foot; after
all players died the stage restarted, P1 rejoined on foot and died twice more
with the object list intact and no freeze, interpreter cap, BRK or fatal
(7,179 frames). The existing frozen state stays frozen (its list is already
broken). Native coop test covers the patched/restored operands.

## Retired mod packages in reused builds / Pacotes de mods antigos em builds reaproveitados - 2026-10-16

v0.13.0 merged `superbomberman5.honey-kotetsu` and
`superbomberman5.subordinate-bomber` into `superbomberman5.extra-characters`,
but the framework's catalog staging only copies packages: a `build-lle/` used
since earlier releases still listed both on the launcher's Gameplay page
(with a warning mark) and its `state.toml` kept them enabled. A post-build step
(`tools/prune-retired-mods.cmake`) now removes the two package folders and
their state entries; when either was enabled and the state has no Extra
Characters entry, it enables `extra_characters`. Checked on Linux with a copy
of the maintainer's `build-lle/mods/preloaded/state.toml` (29 → 25 tables,
other mods and the SB4 ROM path unchanged, idempotent) and through a
`cmake --build` of the game target. Fresh setup packs never contained them.

`play.cmd` was removed: `SuperBomberman5Recomp.exe`, which every Windows
build copies to the repository root since v0.13.1, opens the launcher.
O `play.cmd` foi removido; use o `SuperBomberman5Recomp.exe` da raiz.

O launcher não mostra mais Honey/Kotetsu e Subordinate Bomber separados num
build antigo; quem os tinha ligados passa a ter o Extra Characters ligado.

## Portal destinations after bosses / Destinos dos portais após chefes - 2026-10-16

The portal-labels plugin showed nothing on the gates a defeated boss opens:
boss stage lists are in bank `$C7` (the parser accepted only `$C6`) and their
create/contact scripts start with native route-flag conditions
(`$08/$83/$5B/$49/$47`, handlers `$C2:0BDA`, `$C2:1CBA`, `$C2:1AC2`,
`$C2:19B3/$19AA`). The parser now follows those conditions with the live
flags at `$7E2900`. Native test: synthetic `$C7` gate (flag on/off,
suppressing flag, contact mismatch, bank end) and the real-ROM audit (192
numbered/special exits unchanged, 30 boss exits across 20 route cases).
Linux LLE headless run: Zone 1-1 → forced stage `$67` with route flag 7,
enemy completion forced; both native gates appear with `Z2-1` and `Z2-5`,
no interpreter cap, abandonment, BRK or fatal. ZONE4-G gates were not opened
at runtime (multi-part boss); they are covered only by the script audit.
Windows was not built in this session.

Os rótulos agora aparecem nos portais que o chefe abre; validado no Linux
na ZONE1-A (rota A1). Falta conferir no Windows.
## v0.13.1 published and package checks / Publicação e pacotes — 2026-10-16

Published [v0.13.1](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.13.1)
as a prerelease, tag/packaged sources at 735429343a9a985b6b9113ca460f03c79fa9aca5.
[Release workflow 38971420388](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/actions/runs/38971420388)
passed Windows and Linux source/history/package audits, dependency pins, Python
discovery, all 14 native tests, setup-host builds, packaged-analyzer checks,
native Windows entry tests and first/repeat AppImage setup launches.

The exact downloaded CI Windows ZIP passed every FILE_SHA256SUMS entry,
ROM-independent package metadata, twelve installed manifests, native-entry
Unicode/space/precedence/exit-code tests and the bundled analyzer without Rust.
Local generation/build from the user ROM then completed the 3,628-frame
Normal Zone 1-1 route through SuperBomberman5Recomp.exe (exit 0, mode 13).
The exact AppImage passed first/repeat launches, local cartridge generation,
build and the same first-stage route on Ubuntu 24.04.4/WSL with portable
Linux tools 1.0.14. These package routes use default-disabled mods; the new
five-player HUD and Continue combinations have separate Windows runtime
evidence below. This does not qualify the full campaign, physical controllers,
all mod combinations or broad Linux distributions.

Public downloads were fetched again and their sizes/SHA-256 matched GitHub
asset digests and SHA256SUMS; release body matched the generated UTF-8 notes
with no replacement characters. Existing tags/assets were preserved.

| Public asset | Bytes | SHA-256 |
|---|---:|---|
| Windows x64 setup ZIP | 17177915 | b29168b7562dc019e817bf6c26d4efe05fc562d422a8a49f1d58c0e5b8d6515b |
| Linux x86_64 AppImage | 13629944 | 8b20817624b53945115a977a8aec2513584ec1a1a42b84fc5bd43dfcb50bd48d |

A v0.13.1 foi publicada como prévia com ZIP Windows, AppImage Linux e
SHA256SUMS. As duas plataformas passaram nos builds, auditorias e 14 testes
nativos. Os pacotes finais foram extraídos e geraram o jogo com a ROM local;
a entrada nativa Windows completou a rota de 3.628 frames e o AppImage
chegou à mesma primeira fase em Ubuntu 24.04.4/WSL. Esses testes de pacote
usam os mods desativados; a HUD de cinco jogadores e Continue têm validação
Windows específica abaixo. Downloads públicos, tamanhos, hashes, tag e notas
UTF-8 foram reconferidos. Não há qualificação de campanha completa, controles
físicos, todas as combinações ou distribuições Linux em geral.

## Release 0.13.1 integration / Integração da release — 2026-10-16

Integrated on main 94eba67 with its pinned framework/UI dependencies and unified Extra Characters implementation. Windows LLE runtime passed two-player Game Over → Continue retention and the five-player HUD stale-pointer, death/reentry regression. All 14 native setup-host test binaries pass; Python discovery passes (10 tests, 2 conditional skips). The additional SB4 picker-color regression covers choices 11–19 across snapshot restoration.

The ROM-independent Windows entry SuperBomberman5Recomp.exe uses the existing reviewed assets/app.ico, finds the sibling setup/game or build-lle executable, starts it without a console, forwards Unicode arguments and preserves its exit status. A synthetic child verified sibling/development lookup, spaces, accents, working directory, default --launcher and exit code 37. Source builds copy the entry to the repository root; the Windows finalizer includes it in the setup ZIP. Existing game target/binary name stays compatible.

Integração sobre a main 94eba67, mantendo os pins e o Extra Characters unificado. Rotas Windows LLE de Continue em dois jogadores e HUD em cinco passaram. Os 14 testes nativos passam; os testes Python também (10, dois condicionais ignorados). A entrada nativa com o ícone atual do “5” localiza o setup/jogo ou build-lle, não abre console e preserva argumentos Unicode e código de saída; testes com processo filho sintético passaram. Campanha completa, controles físicos, todas as combinações e distribuições Linux não são qualificados por esses testes.


The extracted Windows setup ZIP passed its bundled-analyzer synthetic test, local user-ROM generation/build with the portable toolchain, and the root native entry completed validation/first-stage.input (3,628 frames, no fatal report). The entry prefers a generated build-lle/build game over the sibling setup host. This precedence is covered with two distinct synthetic child exit codes.

O ZIP Windows extraído passou no teste sintético do analisador, geração/compilação local com a ROM do usuário e rota de 3.628 frames pela entrada nativa, sem fatal no relatório. A entrada prioriza o jogo gerado em build-lle/build diante do setup ao lado; teste com códigos de saída distintos cobre essa prioridade.

## Co-op HUD limited to P1/P2 / HUD cooperativa somente P1/P2 — 2026-10-16

Normal's HUD now isolates P1/P2 from extra seats in both native reentry
updates and Honey/Subordinate overlays. The native HUD task maintains the
two face objects after actor creation, without waiting for a life change,
and repairs an unexpected living animation via the native character table.
P3–P5 cannot repaint these portraits; a one-player game creates no P2 face.
Battle overlay loops still accept five seats. All memory patches restore
on reset; no SRAM, save-state size or cartridge-file changes.

Windows/LLE NewRenderer=1 fixtures passed 3/4/5-player Normal Zone 1-1
(4,643 / 4,955 / 5,297 simulated frames). They enable character/color/HUD,
co-op, Honey/Subordinate, Gold, pause, Louie and infinite-time mods; the
3-player route uses English, 4/5 use PT-BR. The regression deliberately
aliases P3's old Battle face pointer to P1 and injects an incorrect P1 face
animation, then checks repair, P3 death/reentry and P2 death/reentry. WRAM
and OAM checks confirm the correct two faces, and resumed captures were
inspected. A fixture-free 3,971-frame English one-player route checks the
absence of a P2 portrait. Raw captures remain under ignored
`.bootstrap/normal-hud-coop/`. `tests/normal_hud_coop_runtime.py` reproduces
the scenarios (extra controls require a fixture host). Fresh control routes
with five seats were also inspected; the injected stale-state regression
models the reported failure rather than claiming every natural trigger was
reproduced. Full campaign, every character/mount combination, physical
controllers and Linux were not separately qualified.

All native binaries and Python discovery passed (10 tests, 2 skipped).
Native tests cover extra-seat reentry guards, both overlay contexts,
living/dead animation repair and exact restoration. The developer executable
is rebuilt with `SB5_COOP_INPUT_FIXTURE=OFF`; test executables/captures stay
local. Existing releases and submodule work are unchanged.

A HUD do Normal mantém somente os rostos de P1/P2, mesmo com três a cinco
participantes. Jogadores extras não atualizam nem repintam esses retratos;
sem participante P2, não é criado seu rosto. Rotas locais conferem 1/3/4/5
jogadores, incluindo referência antiga de P3 sobre P1, reparo da animação,
morte e retorno de P3/P2. Testes nativos/Python passaram e o executável do
atalho foi recompilado com fixtures desativadas. Limites de cobertura acima.


## Normal Game Over → Continue choices / Personagens no Continue — 2026-10-16

The Normal character-selection mod now keeps its selected-campaign state
through Game Over and restores native actor character/palette fields before
the continued stage creates graphics. Choices `$625F/$6264` and Honey/Kotetsu
skin markers already survive the native path; `$16C2/$16DA` were reset to
stock white/black. A one-shot restart flag also restores the original one/
two-seat roster: stock Continue could otherwise retain only the last dead
seat. Larger rosters remain owned by co-op, including boss bench handling.
No new ROM patches or SRAM layout changes. Existing fixed-size state header
accepts state 6 for selected+pending restart; historical states 0–3 still load.

Windows/LLE isolated routes passed own-bomb Game Over → Continue → live
Zone 1-1: 6,919 frames for one player in English, 7,012 for two distinct
characters/colors with PT-BR/co-op, and 7,201 for Honey plus Subordinate with
co-op disabled. Compared characters, palette ids, picker choices, skin markers
and living actors before/after; inspected resumed HUD/gameplay captures.
All captures/saves remain ignored under `.bootstrap/normal-hud-runtime/`.
`tests/normal_character_continue_runtime.py` reproduces these scenarios
(`--single --english`, default, or `--skins`). One-seat New Game picker Back/
reentry/navigation regressions also passed with PT-BR on/off. Native tests
cover 1–5 choice mappings, hook chaining/reset and constant-size state/load
round trips. All native binaries and Python discovery passed (10 tests,
2 skipped). The developer executable is rebuilt with fixtures off. Full
campaign, physical controls and Linux were not separately qualified.

Após Game Over, Continue mantém os personagens e cores selecionados, inclusive
Honey/Subordinate, e retoma os participantes da partida de um/dois jogadores.
A correção foi conferida com e sem cooperativo, em inglês e PT-BR, incluindo
os atores vivos e a HUD após continuar. Testes de Back/reentrada no seletor
continuam passando. SRAM e seu formato permanecem originais; o executável do
atalho já está recompilado. Evidências e limites acima.


## Normal HUD fixed-head cleanup / Limpeza dos rostos fixos — 2026-10-16

The first HUD-face implementation incorrectly masked BG3 text while the
stock white/black heads remain in BG2's three-row strip. The corrected
observer recognizes both native six-tile head signatures in map `$3C00`,
borrows unused 4bpp tiles after scanning BG1/BG2, and fills only the head
interior with native blue index 4. Original top/bottom frame rows remain.
Every tile and map word is restored after drawing. Native regression tests
check both seats, all 24 rows, border preservation and exact restoration.
The developer executable is rebuilt with fixtures off. The 5,046-frame
Windows/LLE PT-BR route with character/color selection and co-op passed P1/P2
own-bomb deaths and P1 Start rejoin. Magnified before/after crops confirm
both stock heads are gone, with the selected faces and HUD frame retained.
Captures remain under ignored `.bootstrap/normal-hud-runtime/`
(`two-color-ptbr-j6nf6wk0`). All native binaries and Python tests passed;
the changed-source archive passed publication audit. Other stages/characters,
Linux and physical controls were not separately qualified for this correction.

A primeira implementação deixava os ícones branco/preto na camada BG2,
atrás do retrato. A correção remove toda a área do rosto fixo nas três linhas
de tiles, mantendo o azul e a moldura. Testes cobrem P1/P2 e restauração de
VRAM; o executável do atalho foi recompilado. Capturas ampliadas conferem
ambos os retratos sem o rosto antigo atrás; a rota com PT-BR, cores e
cooperativo também passou mortes de P1/P2 e retorno de P1 com Start.


## Normal HUD character faces / Rostos na HUD do Normal — 2026-10-16

Current source adds the optional, default-disabled `superbomberman5.hud-faces`
0.1.0 package. P1/P2 use native Battle face objects in the Normal HUD, sharing
the current character palette and native defeated animation. Normal heart,
lives, points and timer remain; no wins counters or extra P3–P5 HUD slots.
The existing blinking rejoin prompt is fitted beside a defeated portrait by
resampling its live BG3 pixels, including PT-BR, and restoring all VRAM after
drawing. No extracted graphics, persistent campaign fields or SRAM changes.

Windows/LLE isolated routes checked stock 1P/2P Zone 1-1 entry (3,817/3,880
frames) and two different characters/colors with PT-BR, character selection,
colors, co-op and the extra-character plugins enabled. A 5,046-frame route in both English and PT-BR
covers own-bomb death of P1, the alive/defeated native face transition, Start
rejoin restoring its living face, then own-bomb death of P2. Character ids
1/6 and palette ids 9/50 are preserved. Captures and saves remain ignored
under `.bootstrap/normal-hud/`; `tests/normal_hud_runtime.py` reproduces the
combined route with a local executable and the user's ROM.

Native tests cover patch guards/refusal, branch destinations, reset,
Normal/Battle contexts, both dead prompts and exact VRAM restoration.
All native test executables and Python discovery passed (10 tests, 2 skipped).
Package/source/history publication audit and whitespace checks passed.
The developer executable is rebuilt with fixtures off. Full campaign,
physical controls, every character/color combination, save-state/rewind and
Linux runtime were not separately qualified for this mod. Published releases
and existing submodule working trees are unchanged.

O novo mod opcional troca os ícones fixos de P1/P2 pelos rostos do Battle,
seguindo personagem, cor e derrota, sem placar de vitórias. Start restaura o
rosto vivo ao retornar; o aviso de retorno continua legível e piscando nos
dois idiomas. As rotas locais incluem Normal 1P/2P e seleção de personagens
e cores diferentes com PT-BR/cooperativo, morte por bomba de ambos e retorno
de P1. Testes e auditoria passaram; campanha completa, todos os personagens,
controles físicos, rewind/save states e Linux não foram qualificados nesta
rodada. Ative **Battle character faces in Normal HUD** em Mods → Gameplay.
[Instruções](NORMAL_HUD_FACES.md).


### Mounted entry into boss stages with 1-2 players / Montado em chefes com 1-2 jogadores

Reported: 1 player as Terrorin, entering ZONE4 - G on Blue Louie, P1 and the
boss broke. Reproduced on Linux LLE with all gameplay mods (co-op, characters,
Louie, colors, pause, infinite time, Gold Battle): picker route choosing
Terrorin, egg pickup in Zone 1-1, forced transition to stage `$6C`. The boss
never appeared, the rider stood still. Same with Bomberman; absent without the
co-op mod. Cause: the co-op adapter wrote `$1CBF = 2` every campaign frame
with one/two players, overriding the boss value 3 from `$C0:B9AE`, so the mount
used the boss's actor slot 2. Now only the adapter's own value 5 is undone.
After the fix the same routes (four egg timings, Black Louie from the native
lottery; Blue/other colors were not reached from the full picker route) show
the boss, its rival and minions, the rider and mount in slots 0/3, and normal
mount loss on a hit; no interpreter cap, abandonment, BRK or fatal. Native
coop test covers values 3/4 kept and 5 reset. Windows not built here.

Corrigido: com 1–2 jogadores e o co-op ligado, entrar montado num chefe
deixava o Louie no lugar do chefe. Validado no Linux; falta o Windows.

## v0.13.0 published / Publicada - 2026-10-16

[Prerelease v0.13.0](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.13.0)
at `4a69471` (tag created by the release workflow; tag pushes and the Git
tags API were refused by this session's proxy, so it is not annotated).
Release workflow test run 38942331672 and publishing run 38942922204 passed
the Windows and Ubuntu 22.04 builds, native tests, Python tests, source/
history and package audits. Assets match GitHub digests and SHA256SUMS:
Windows ZIP 16,975,157 bytes (`2045e62e...071458`), Linux AppImage
13,605,368 bytes (`1a5c28f1...2871f4`), SHA256SUMS 216 bytes. The ZIP holds
the eleven mod packages (Extra Characters; no Honey and Kotetsu or
Subordinate Bomber package) and no ROM, generated C, WAV or screenshot. Remote
notes equal the UTF-8 generator output with no U+FFFD.

Downloaded AppImage on the Linux cloud host: first launch installed the
writable 0.13.0 setup, `tools/regen.sh` generated from the real ROM and the
game rebuilt (system compilers, not the launcher's portable tools); a Battle
route with Bomber Kings + Extra Characters + colors showed Terrorin standing
and his barrier with dummy video/audio. The Windows ZIP was not run by me
(the workflow artifacts could not be downloaded here); the maintainer's
Windows check covered the source build before release.

Publicada com ZIP Windows, AppImage Linux experimental e SHA256SUMS. O
AppImage baixado gerou e recompilou o jogo e passou numa rota de Battle com o
Terrorin; o ZIP Windows não foi executado nesta sessão.

## v0.13.0 release candidate - 2026-10-16

Based on published v0.12.1 with pinned dependencies unchanged. Adds the
Super Bomberman 5 Extra Characters mod (below), which replaces the Honey and
Kotetsu and Subordinate Bomber packages: eleven optional mods, default
disabled. The maintainer confirmed the mod's latest build in Windows gameplay
before release. Rebased tree: all thirteen native tests, Python tests,
publication audit and `git diff --check` pass on Linux.

README montage: the top-left v0.12.1 frame is kept; the other three were
recaptured from the Linux LLE build of these sources (Normal picker with
Terrorin/Honey/Jet/Lady/Great, Zone 1-1 with Terrorin's barrier, Jet's flight,
Honey, Lady and Great; Battle stage 1 with Kotetsu, Bazooka's beam, Hammer,
Honey and Terrorin's barrier). Seats 3-5 were driven through a local,
uncommitted script extension for the multitap seats (not part of the release);
Zone 1-1 positions were poked. Package checks are recorded after the workflow.

## Super Bomberman 5 Extra Characters + Emperor Terrorin / personagens extras + imperador Terrorin — 2026-10-16

Developed on branch `extra-characters` (from v0.12.0, rebased on v0.12.1) and
released in v0.13.0. One mod, `superbomberman5.extra-characters` 0.1.0 (default
disabled), replaces `superbomberman5.honey-kotetsu` and
`superbomberman5.subordinate-bomber`: Honey, Kotetsu, Subordinate Bomber and
the new **Emperor Terrorin** (on-foot phase of ZONE5 - J, stage `$6F`), with
Honey/Kotetsu's B + direction abilities and Terrorin's barrier and Clock.
`honey_kotetsu_mod.c` became `extra_characters_mod.c`; the picker has seven
columns (Terrorin choice 19, top right; cursor skips empty cells), six with
Bomber Kings alone. WRAM moved: cells `$7F:F800/$F814`, seats
`$7F:F828/$F82D`, ability state `$7F:F840-$F8F0` (all zero in 453 research
captures). Details in docs/EXTRA_CHARACTERS.md.

Research (Linux LLE headless, isolated copies, local captures only): stage
`$6F` forced from Zone 1-1 (`until 0x1031 == 0d`, `poke 0x1031 6f`); phases 1-2
ended with `$1667=0`, `$15A7=$40`; phase 3 dumped every 8 frames (300 dumps).
Terrorin archive found by matching phase-3 OBJ VRAM against every set-8
archive entry (entry 56, 238 tiles); every frame of set `$0E` rendered to map
the animations. Barrier routine found with a lab-only WRAM write watch on
`$1052` (`$C3:3FB3`), Clock placement on `$145A` (`$C5:455D`). `$0C` cells
kill a player (Zone 1-1 poke under P1: death through `$1CCA`, `$C4:2D58`).

Evidence (same harness, SB4 ROM selected through `[[resource]]`):
- Native tests: all thirteen programs pass; `extra_characters_tests`
  (seven-column tables for extras only, Kings only, both; Terrorin archive,
  frames, palette and poses; restoration), `special_abilities_tests` (barrier
  timeline, 20 cells around a soft block and pillars, fire-cell hook for
  owner/opponent/teammate, barrier sound request, Clock flight through
  `$C4:398C` with `$8D`/`$1CC7` restored, pickup rules and paralysis through
  `$C4:4503`, Battle clock not stopped, Normal Game stock effect; Terrorin
  standing/paralysis frame and defeat steps), `subordinate_tests` (activation by the
  new plugin). Python tests and the publication audit pass.
- Battle picker (Bomber Kings + extras + colors, PT-BR too): seven columns,
  Terrorin figure and portrait, cursor 5→11→19→1 and down 1→6→13.
- Battle stage as Terrorin: walking, HUD face, death; B + down: spin, blue
  barrier box (HUD excluded), 21-cell barrier with Terrorin standing on it
  unharmed, Clock placed on a free cell, barrier dropped at frame 192.
  Clock pickup by Terrorin froze seats 2-5; by an opponent froze Terrorin
  (frozen standing); the Battle timer kept running.
- Follow-up (same day): mod renamed "Super Bomberman 5 Extra Characters";
  the barrier fades in over 16 frames like the boss and plays `$0288`,
  rendered in a private APU (`src/sb5_sound.c`; sending `$74` to the game's
  own driver in Battle froze it, since the Battle set lacks it); the Clock flies off him like a lost item (`$C4:398C`)
  and landed on a random free cell; defeat replays the boss's animation `$50`
  (head flying right, body falling), kept after the native sprite vanishes.
- Second follow-up (same day, Windows report): standing (and walking on a
  mount, and the Clock paralysis) uses the walk's middle step, legs straight
  (frames 68/74/80/86; the walk's first step had one leg out and `$4A-$4D`
  are a crouch). The barrier sound never played: the driver clears its input
  ports, so the command must be rewritten until echoed (as `$C0:0E82`
  does), and the right state is the third phase's: a stage-load trace
  (LAB_PCS on `$C0:0E31/$0E9C/$0E7F/$0CED`) showed stream `$01` before every
  stage's streams, phase 3 loading `$0D/$26/$3F` + set `$18`, and the boss
  sending `$0288` (command `$74`). A headless harness linking `sb5_sound.c`
  with the runner's APU renders 1.8 s with the command and silence without
  it; the clip is now rendered on the stage's first frame. The up-throw legs
  (tiles `$F6-$F9`, Terrorin's last archive blocks) were dropped by a tile
  bound that ignored the 16x16 block layout; fixed for every archive.
  Battle run: standing frame and barrier checked in dumps.
- Normal Game (normal character selection): Terrorin chosen, Zone 1-1 with
  his skin and HUD face, barrier and Clock.

Found and fixed: the stock Terrorin stream's last control byte is read at
`$CC:8000`, so the checked end is `$CC:8001`; queuing the barrier sound
(`$C0:0CED`, A = `$0288`) from the pad hook froze the game 37 frames later
(APU handshake), so the barrier is silent; the native Clock pickup also stops
the Battle timer (`$0E20` = 960), cleared for Terrorin's Clock.

Not validated: Windows build and play, physical controllers, an opponent
actually dying inside the barrier in a live match (the lethal cell and the
hook are covered separately), five seats on Terrorin, score board/champion
screen (the champion screen shows Bomberman, as for the Bomber Kings).

## v0.12.1 published / Publicada - 2026-10-16

[Prerelease v0.12.1](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.12.1),
annotated tag at `7e9e631`. Release workflow 38851739281 passed Windows and
Ubuntu 22.04 builds, thirteen native tests per platform, Python tests, source/
history and package audits. Publication followed local extracted-package checks:

- Windows: setup launcher first/repeat startup, bundled analyzer without Rust,
  real-ROM generation/rebuild, all thirteen native tests, 3,628-frame LLE
  first-stage route, and PT-BR/co-op title capture passed.
- Linux AppImage on Ubuntu/WSL: first/repeat writable installation, bundled
  analyzer, real-ROM generation/rebuild, thirteen native tests and the same
  3,628-frame LLE route passed, using dummy video/audio. This does not qualify
  physical Linux display/audio devices or other distributions.
- Release-built Windows New Game/Continue picker cancellation: 1 and 5 seats,
  PT-BR off/on (eight cases), two cancels/reentries, file navigation and Back
  passed. Earlier 1/2/5-seat evidence remains below; synthetic Continue files
  are fixtures rather than completed campaigns.
- A private fixture executable linked from extracted Windows release objects
  counted native Black Louie $022E acquisition and $0235 dash commands with
  real egg pickup/attachment and enemy push, for P1 and P5/co-op/PT-BR. One
  command per event, no dash repeat while active. Production package builds
  retain SB5_COOP_INPUT_FIXTURE=OFF; no fixture executable is distributed.

Assets verified against GitHub digests and SHA256SUMS: Windows ZIP 16,943,808
bytes (`e11e9438...372a2e`), Linux AppImage 13,576,696 bytes
(`19b726b4...db4fc8`), plus SHA256SUMS (218 bytes). Both setup payloads contain
twelve default-disabled mods, complete per-file checksums and no WAVs, ROMs,
generated game code or documentation screenshots. Remote notes exactly match
the UTF-8 generator output; remote README and montage match local bytes.
Raw evidence stays under ignored `.bootstrap/v0121-*`.

Publicada com ZIP Windows, AppImage Linux experimental e SHA256SUMS. Pacotes
extraídos passaram por geração/rebuild e entrada na primeira fase; retorno
com B foi verificado em New Game/Continue com 1/5 jogadores nos dois idiomas.
Montagem e arrancada do Black usam comandos nativos da ROM, sem WAVs. A nova
imagem do menu está no README. Campanha completa, todas as combinações de mods
e multiplayer físico continuam sem qualificação; releases anteriores intactas.


## v0.12.1 release candidate - 2026-10-16

Based on published v0.12.0, preserving all twelve mods and pinned dependencies.
Includes English/PT-BR player-menu typography/cursor corrections, New Game and
Continue picker cancellation, and cartridge Black Louie SFX without bundled
WAVs. The maintainer confirmed the audio fix in local gameplay. The top-left
README frame was recaptured from actual Windows/LLE output; other frames are
unchanged. Prior runtime evidence below predates integration with Bomber Kings;
release package generation/rebuild evidence is recorded after package checks.


## v0.12.0 published / Publicada — 2026-10-16

[Prerelease v0.12.0](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.12.0),
tag at `3392683` (main). Release workflow run 38846580292: Windows and Ubuntu
22.04 builds, thirteen native tests per platform, audits and publish passed.
Assets: `superbomberman5-0.12.0-windows-x64.zip` (17,109,642 bytes, SHA-256
`1fc5950e…44db`), `superbomberman5-0.12.0-linux-x86_64.AppImage` (13,744,632
bytes, `4fa03f10…dbcd`) and `SHA256SUMS`, matching GitHub's digests. Notes
equal `tools/release-notes.py` output (UTF-8, no U+FFFD). The Windows ZIP holds
the Bomber Kings manifest and no ROM or `docs/images`. Not re-run here: the
extracted packages' first launch and local Generate & rebuild.

## v0.12.0 — Bomber Kings release preparation / preparação — 2026-10-16

`bomber-kings` merged with `main` (v0.11.1: Louie improvements, PT-BR title
font); no conflicts beyond this log. After the merge (Linux LLE, fixture build):
thirteen native test programs pass; Normal Zone 1-1 ability route repeated
(four enemies killed, Jet alive). New finding while capturing README frames:
with Subordinate Bomber, colors and Bomber Kings all on, a five-seat picker
left too few idle OAM entries and the third row's Bazooka/Great figures
vanished (emit failed for lack of slots). Fixed by reusing entries of native
sprites that end above the row, written on the row's first line by the raster
observer and restored on line 224 (`late_places`; native test with only four
idle entries). Rechecked: all six figures with all picker mods on.
Showcase routes: title five-player menu; picker with Jet/Lady/Honey/Kotetsu/
Great in changed colors; Normal Zone 1-1 with those five using abilities;
Battle stage 1 with SB4 Bomberman, Hammer, Bazooka, Honey and Subordinate
Bomber in changed colors. Windows: maintainer playtest of the abilities on a
local build; release CI builds/tests both platforms.

## Bomber Kings and special abilities / Reis Bomber e habilidades — 2026-10-15

Scope: new optional mod **Bomber Kings — Super Bomberman 4 characters**
(`superbomberman5.bomber-kings` 0.1.0, host v0.11.0+, default disabled) and B +
direction special abilities for Honey, Kotetsu and the five Bomber Kings. The
Honey/Kotetsu module became the extra-character core (18-cell, three-row
picker when the Bomber Kings mod is on and a verified Super Bomberman 4 image
is found). Super Bomberman 4 graphics/palettes are read at runtime from the
player's own ROM (`src/sb4_rom.c`, SHA-256 checked); nothing from it is in the
repository. Research tools (snes9x libretro core with debug exports, Python
frontend, captures) stayed local.

The Super Bomberman 4 ROM is requested on the launcher's Mods page (manifest
`[[external_rom]]` `sb4-rom`, `normalized_sha256`, `required`). Headless check:
with the `[[resource]]` selection in `state.toml` the picker had 18 cells;
without it the plan was rejected ("Resolve the highlighted mod conflicts").

Evidence (Linux LLE headless, fixture build, SB4 ROM via `SB5_SB4_ROM`):
- Native tests: all eleven pass, including new `special_abilities_tests`
  (input rule, remote detonation, riding, debuff speed and Z flag, Normal /
  Free-for-All / Team damage, blocks) and `sb4_rom_tests`; `honey_kotetsu_tests`
  extended to the three-row layout, row palettes and Super Bomberman 4 skins.
- Picker, Battle, five seats: three rows, all six SB4 figures with their own
  palettes, SB4 portraits, cursor through the rows, L/R through five colors
  (choices 14/15/17/18/18 with colors 2/0/2/3/0 reached the stage with those
  palettes). Honey/Kotetsu-only picker unchanged (two rows).
- Battle stage with five kings: walking four directions, power-glove throw,
  Louie riding (egg placed by poke), victory, defeat and score board drawn with
  SB4 frames; HUD faces.
- Abilities: Honey shot, Bazooka flame column (killed Honey: `$15A2`=`$D9`), Jet
  dash (stopped by walls), Great shield (native invulnerability blink), debuff
  speed 10 px per 20 frames vs 20 and back after 640 frames. Remote control
  (`$4ED5`=1): B + direction used the ability and kept the bomb; B alone
  detonated after the wait; riding (`$1782` bit 7): no ability.
- Team Battle: Bazooka's flames killed an opponent and spared a teammate
  (`$6269` bit 7 / `$625D` bit 0).
- Normal Game (five-player coop, Zone 1-1): flames killed two enemies; no
  player was hit by an ability.
- Normal Game, Zone 1-1, enemies poked into the paths of Jet (P1, down),
  Bazooka (P2, left), Honey (P3, up) and Kotetsu (P4, up): all four enemies
  died (`$15A2` = `$C0`); Jet touched his enemy and survived (`$15A2` stayed
  `$05`; before the kill-store hooks he died on contact and never hit it).

SB4-faithful abilities (second pass, same day): the moves were ported from
Super Bomberman 4 frame captures instead of SB5 approximations. Hammer and
Bazooka sheets/palettes were swapped (fixed: Hammer `$CC:8000`/`$E2A0`,
Bazooka `$CB:0000`/`$1601FA`). Native tests: twelve pass (new timeline, Jet
flight, mace/laser stun and item-loss injection checks; SB4 fire/effect stream,
portrait and frame-by-address decoding). Battle routes (five kings):
- Bazooka: aiming pose turned through four directions, fire beam with SB4
  explosion tiles frames 68-82 along 12 tiles, killed the player at the end
  (`$15A2`=`$D9`), debuff after 98 frames.
- Jet: wind-up poses, flight 4 px/frame from x `$D0` to `$10` with a two-tile
  fire trail every 8 frames, killed the player in the row, drawn during the
  held invulnerability; stopped at the wall, debuff after the trail.
- Hammer: mace orbit (frames 29-106); the player it touched was dazed with the
  SB4 electrocuted frame (`$164A`=50) and lost a bomb (`$17CA` 5 → 4, item
  flew off). Lady: laser up the column, target dazed and lost two bombs
  (5 → 3). Great: front pose, blinking 160 frames. Honey: shooting animation,
  first player in the row killed.
- Picker: each seat shows the SB4 Battle portrait of its Bomber King in the
  chosen palette (Bazooka, Jet, Hammer, Lady with R, Great).

Third pass (same day, maintainer feedback): Jet keeps the flying pose 20
frames after the crash and blinks invulnerable (native `$170A`, never frozen)
until about a second after it; Lady stands 109 frames (two gun turns around
the shot) while the laser flies on its own; debuff counted from the trigger
(SB4 `$E2,X` = 160 on that frame); SB4 riding frames 20-23; SB4 Bad Bomber
board frames per arena side and throw; SB4 sound effects (`$39`, `$4E`,
`$5A`, `$5F`) from SB4's own driver in a private APU (dev harness rendered
1.77/0.22/0.86/1.71 s clips, peaks 12588/4703/6614/10896); seat palettes
recolored only on the figure's lines so Louie/eggs/HUD keep native colors.
Evidence (Linux headless, five kings): Jet flight blinking, crash pose and
kill; Lady laser hit at frame 118 after walking was allowed from 109 (two
bombs lost); egg pickup and Louie ride with consistent rider/Louie colors (the
rider used to flip palettes); Bad Bomber on left/top edges and throwing with
the SB4 board frames; native tests 12/12. Not heard: audio was disabled in the
headless runs, so the in-game playback of the clips is not verified by ear.

Fourth pass (same day): Honey fires her boss bullet (anims `$27-$2A`) after
her shooting animation (`$23-$26`, 55 frames) — headless: bullet flew the top
row and killed the player at x 208; Kotetsu uses his 61-frame thrust;
Bazooka's beam burns the first soft block (`$C5:441D` through a pushed RTL
frame; headless: row 1 cell 3 `$30` → burn animation → `$00`) and sets off
bombs (native test); Jet stops on the flight axis and stays 24 frames
(SB4: crash at 96, walks at 120; headless: crash 97, ability until 121);
riding frames 16-19, mounting 20-23 (SB4 uploads with Lady and Great).
Native tests 12/12.

Not verified: Windows build/run, physical controllers, every SB4 pose (stun,
Time Up and Bad Bomber cart were not captured), boss stages, CPU seats with
extra characters, save state/rewind during an ability (state lives in WRAM
`$7F:F830-$F8D9`, so it is included in snapshots).

Novo mod opcional dos Reis Bomber (lidos da ROM do Super Bomberman 4 do
jogador) com habilidades B + direção também para Honey e Kotetsu. Na segunda
rodada as habilidades foram portadas do Super Bomberman 4 (tempos, poses,
fogo, maça, laser e retratos lidos da ROM do SB4; atordoamento e perda de
itens com as rotinas do próprio SB5). Testes
nativos e rotas roteirizadas no Linux passaram (seleção, Battle Free-for-All e
Team Battle, Normal com cinco jogadores); Windows ainda não foi testado.

## Black Louie native cartridge audio / SFX nativos - 2026-10-16

Current source removes both Black Louie WAVs, their loader and the package
provenance exception. Acquisition now retains native A=$022E at $C4:5168;
dash calls A=$0235 via an authored $C4:5C00 trampoline once per input poll
with an accepted ability. M/X widths, registers and the native return tail
are preserved. No sound data is embedded in source or the new manifest-only
package. The ROM file and SRAM layout remain unchanged.

Local Windows/LLE research compared stage-bank PCM against both supplied
references: acquisition duration 1.256 s vs 1.261 s; dash 0.490 s vs 0.492 s.
Waveform correlation is affected by capture clocks/resampling; these are native
SFX matches, not claims of byte-identical WAV reproduction. All raw audio
and diagnostic captures remain ignored under .bootstrap/native-louie-audio/.

Windows/LLE fixtures passed for Normal P1 and five-player Normal P5 with
PT-BR: real egg acquisition, attachment, Y dash and enemy push, exactly one
native $022E acquisition command and one $0235 command per accepted dash,
with no repeated dash sound while active. All native test executables passed;
Python discovery passed 10 tests (2 optional checks skipped). The manifest-only
archive and tracked source plus Git history passed publication audit;
`git diff --check` passed. Battle sound-bank playback and subjective
speaker listening were not separately qualified in this run.

O código atual remove os dois WAVs e usa os comandos nativos da ROM para
montagem ($022E) e arrancada ($0235). O pacote novo tem apenas manifesto e
licença; o executável do atalho foi recompilado com fixtures desativadas.
Histórico Git e releases anteriores permanecem intactos; hashes antigos só
servem para auditoria histórica, sem permitir WAVs em novos pacotes.
Validados ovo, montagem e arrancada no Normal com P1 e no cooperativo com P5
mais PT-BR, sem repetição do som durante a habilidade. Battle e escuta por
alto-falantes não foram qualificados separadamente nesta rodada.


## English player menu and Normal Back / Menu inglês e retorno — 2026-10-16

Current source uses 1 PLAYER / 2–5 PLAYERS in the five-seat English menu,
with the original title Y and T's neighboring outline cropped below its
crossbar. The face follows the selected row in both languages. Native tests
cover the strings, source halves, T crop, cursor positions and restoration;
Windows/LLE captures checked all five rows and both heading blink phases.

Normal picker cancellation now enters the complete native file-list scripts
($C7:100E New Game / $C7:0D59 Continue), including cleanup and archive load.
The former $C7:1014 entry was inside FF.8D operands: co-op combinations could
stick at $C7:108D with NMI busy; Honey/Kotetsu-only also exposed WRAM damage.
Cancellation unwinds the suspended fade's VM call (depth and three-byte
return), preserves Continue origin across snapshot loading, and leaves the
snapshot layout unchanged. Unit tests now simulate the mod reset during
private checkpoint loading, rather than restoring WRAM alone.

Paired Windows/LLE routes exercise New Game/Continue, 1/2/5 seats, PT-BR
on/off, co-op/colors/Subordinate/Honey/Kotetsu, repeated Back/reentry, file
navigation and return to the outer menu. Both cancels preserve SRAM exactly.
Additional one-seat/PT-BR routes canceled, reopened, confirmed a character
and entered live Zone 1-1 in New Game (4,309 simulated frames) and Continue
(5,207 simulated frames). No interpreter errors in these routes.
`tests/normal_character_cancel_runtime.py` records the isolated fixtures and
checks menu responsiveness, VM call depth and NMI progress. Full campaign,
physical controls and Linux runtime coverage for this fix remain unqualified.
The developer executable is rebuilt; published v0.11.1 assets are unchanged.

Builds atuais: singular/plural no menu inglês, Y original, contorno do T
corrigido e cursor alinhado nos dois idiomas. B reconstrói a lista nativa de
arquivos, sem entrar no meio dos parâmetros nem deixar retornos extras na
pilha. A origem Continue é preservada ao carregar o snapshot. Rotas locais
conferem 1/2/5 jogadores, PT-BR ligado/desligado, cancelamentos repetidos,
reentrada, navegação e retorno ao menu anterior, sem alterar SRAM. Testes
nativos/Python e auditoria passaram. Sem mudança nas releases já publicadas;
campanha completa, controles físicos e Linux não qualificados nesta correção.

## v0.11.1 published / Publicada — 2026-10-15

[Prerelease v0.11.1](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.11.1)
targets `bdb50bedb28946551b7ecf1c5f150bcc4f8708c9`.
[Clean Release workflow 38823031455](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/actions/runs/38823031455)
passed Windows and Ubuntu 22.04 builds, native/Python tests, dependency-pin
checks, history/package audits and setup/analyzer smoke tests. The release
includes title digit and cursor spacing fixes. Framework pins and eleven
default-disabled mod packages are unchanged from v0.11.0. No SB4 development,
ROM, generated game code, saves or captures are included.

Downloaded artifacts matched GitHub SHA-256; payload checksums cover all
files and both approved Black Louie WAVs match their authorized hashes.
Extracted Windows setup and Linux AppImage (Ubuntu 24.04/WSL) each generated
and rebuilt using the local JPN ROM, passed all eleven native binaries and
the 3,628-frame LLE first-stage route. Windows launcher startup passed twice;
Linux first/repeat writable setup launches and generated-game launch passed.
Both bundled analyzers passed synthetic-ROM checks. Both rebuilt packages
also passed paired PT-BR off/on 1,766-frame routes through all five menu
rows, confirmation and file entry, with identical WRAM/VRAM/OAM/CGRAM/SRAM
in every paired dump. Menu captures were inspected on both platforms.
Physical controllers, audio latency, full campaign and all mod combinations
remain unqualified. Partial recompilation; most game logic is interpreted.

Public assets verified by size and SHA-256:

- `superbomberman5-0.11.1-windows-x64.zip`: 17,021,409 bytes, SHA-256
  `3b0863351f865de0e07c4ed7435545cf36c976ba83dc843247ef3bff2cf48b76`.
- `superbomberman5-0.11.1-linux-x86_64.AppImage`: 13,662,712 bytes, SHA-256
  `de6ed7d6c000412e0896b32410fe7401b3cc3dee8ddbba8b02425d3be445124c`.
- SHA256SUMS covers both public packages. Remote bilingual UTF-8 notes match
  the generated file exactly, with no replacement characters.

Pré-release v0.11.1 publicada com correções da fonte e da posição do cursor.
ZIP Windows, AppImage Linux e SHA256SUMS conferidos. CI limpa e onze testes
nativos passaram; os dois pacotes geraram/recompilaram e executaram a rota
LLE de 3.628 frames. Rotas pareadas de 1.766 frames verificaram as cinco
opções com PT-BR ligado/desligado e memória/saves idênticos. Pins e onze mods
mantidos. Linux experimental; campanha completa, controles físicos e todas
as combinações não qualificados. Alterações locais dos submódulos excluídas.

## PT-BR cursor spacing / Espaçamento do cursor — 2026-10-15

The translated five-player menu aligns the face to the selected row's text:
x=76 for 1 JOGADOR, x=68 for 2–5 JOGADORES, with an eight-pixel cell gap.
English retains x=48. The confirmation bounce remains vertical and OAM is
restored after drawing. Native tests cover all five selections/languages,
bounce and exact restoration. Paired Windows/LLE routes (1,766 frames each)
visited all five rows with PT-BR off/on, confirmation and file-menu entry;
captures were inspected, with identical WRAM/VRAM/OAM/CGRAM/SRAM in all
seven paired dumps. No interpreter errors; full campaign unqualified.
Menu PT-BR com cursor junto da opção atual, inclusive a linha singular.
Rotas pareadas de 1.766 frames conferiram as cinco opções, confirmação e
entrada nos arquivos, com tradução ligada/desligada e sem erros.

## PT-BR player-count font / Fonte da quantidade de jogadores — 2026-10-15

Player counts 1/2 now read the original title glyphs from the cartridge;
3–5 use authored extensions with matching six-column, ten-row ink and full
outline. This supersedes the earlier 5x7 approximation. Windows/LLE: 1,448
frames to the five-player menu with PT-BR/co-op, inspected beside the native
copyright digits. Native tests cover cartridge reads, digit weight/margins
and render restoration. Full campaign is outside this check.
Os algarismos 1/2 agora vêm da fonte original da ROM; 3–5 são extensões
autorais com o mesmo traço e contorno completo. Substitui a aproximação 5x7.
Menu conferido em Windows/LLE com PT-BR/co-op, junto dos números do copyright.

## v0.11.0 published / Publicada — 2026-10-15

[Prerelease v0.11.0](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.11.0)
targets `17e97f6c742630dbc4f45c9ef339fb868cb3c62d`.
[Clean Release workflow 38819354429](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/actions/runs/38819354429)
passed Windows and Ubuntu 22.04 builds, native/Python tests, dependency-pin
checks, publication audits and setup/analyzer smoke tests. Both downloaded
artifacts matched GitHub's SHA-256. Their payload checksums cover every file;
eleven manifests are default-disabled. Both approved Black Louie WAVs match
the documented hashes. No SB4 character implementation/package, generated
game code, ROM, local preferences or captures are included.

Extracted Windows setup and Linux AppImage (Ubuntu 24.04/WSL) each generated
and rebuilt using the local JPN ROM, passed all eleven native binaries and
the 3,628-frame LLE first-stage route with dummy video/audio. Windows setup
launcher startup passed twice; both analyzers passed the synthetic-ROM test.
Linux AppImage first/repeat writable setup launches and subsequent generated
game launch passed. A separate local fixture build from the Windows package
passed Red (3,813 frames, native hit SFX command once and enemy freeze/resume),
Black (3,642 frames, egg/protected push), Yellow chain (3,724 frames) and
P5/co-op/PT-BR (3,756 frames). The fixture is OFF in public setup builds;
fixture game builds and captures stay ignored. Physical audio latency,
controllers, full campaign and every mod combination remain unqualified.

Public assets verified by size and SHA-256:

- Windows ZIP: 17,015,993 bytes,
  `9c9f7565e61f3485f4ecde0ba2d5a515dc360eb41a8db2c7987baf7007349fcb`.
- Linux AppImage: 13,658,616 bytes,
  `b80138be830b1fde6fc097e0fb255751d682497d944135be0a7c5850e2f52c5a`.
- SHA256SUMS covers both. The published bilingual UTF-8 body matches the
  generated notes exactly, without replacement characters; prerelease=true.

Pré-release v0.11.0 publicada com ZIP Windows, AppImage Linux e checksums.
Builds CI limpos, pins, testes, auditorias, onze mods desligados por padrão e
hashes dos arquivos conferidos. Os dois WAVs adicionais autorizados do Black
estão incluídos; personagens do SB4 em desenvolvimento ficam de fora.
Os pacotes geraram/recompilaram o jogo com a ROM local, passaram nos onze
testes nativos e na rota de 3.628 frames em Windows e Ubuntu 24.04/WSL.
Fixtures separadas do pacote Windows confirmaram Red/SFX, Black, cadeia do
Yellow e P5 com co-op/PT-BR. Fixture desligada nos setups públicos. Áudio
físico, controles, campanha completa e todas as combinações de mods não foram
qualificados. Notas bilíngues e hashes remotos verificados.


## v0.11.0 release scope / Escopo da release — 2026-10-15

The Louie branch starts at published main `9ceb220` and adds only Louie
improvements plus release metadata. The in-development Super Bomberman 4
characters, external ROM loader, abilities and package are excluded. Existing
framework pins remain unchanged; local dirty submodules are not released.
The eleven activation packages are default-disabled. The two Black Louie WAVs are included as additional resources explicitly
authorized by the maintainer, with provenance and SHA-256 checks. This release decision supersedes the
ignored-local-audio arrangement described in the earlier development notes below.

All eleven native test binaries and the Python publication/encoding tests
passed locally (two Linux AppRun tests skipped on Windows). Windows/LLE
feature routes and sound evidence are recorded below; the maintainer
confirmed the final behavior. Release setup packages are built from clean CI
checkouts using the pinned dependencies. Package smoke tests and Linux
coverage do not establish full-campaign or all-mod-combination accuracy.

A branch dos Louies parte da main publicada `9ceb220` e acrescenta somente
as melhorias dos Louies e metadados da release. Personagens do Super Bomberman
4, carregador de ROM externa, habilidades e pacote ainda em desenvolvimento
ficam de fora. Pins dos frameworks preservados; alterações locais dos
submódulos não são publicadas. Onze pacotes desligados por padrão; os dois WAVs adicionais do Black
estão incluídos com autorização expressa, origem e hashes documentados. Os onze testes nativos e os testes Python
passaram (dois testes Linux pulados no Windows). Rotas e limites abaixo;
builds de setup usam checkouts CI limpos e dependências fixadas.


## Louie sound feedback / Sons dos Louies — 2026-10-15 (v0.11.0)

Red now reuses the exact native player-hit cue (group 2, command `$38`) when
its Normal punch hits an enemy. The return from the native player search at
`$C4:5563` redirects to the existing sound tail `$C4:5576` only if an enemy was
hit and the player search missed. A native player hit keeps its original path,
so mixed hits do not duplicate the sound; misses produce no hit cue.

Black Y starts SB6's `Assets/Sounds/BlackLouieDash.wav` once per accepted dash,
with separate host voices for P1–P5. Its acquisition PCM now starts at
`$C4:5168`, the same native cue as the other Louies, instead of the later
attachment handler `$C4:519C`. The local WAV loader skips initial digital zero
samples (37.56 ms mount / 49.50 ms dash in the supplied files); original files
are unchanged and remain ignored under `mods/local/` / `build-lle/mods/local/`.
Both clips are stopped on reset/load and unregistered on plugin deactivation;
no audio is embedded or packaged. Override dash path with
`SB5_BLACK_LOUIE_DASH_SFX`; acquisition keeps `SB5_BLACK_LOUIE_SFX`.

Evidence: all eleven Windows native test executables and strict warning checks
pass. Python checks pass (seven tests, two Linux-only checks skipped on Windows).
Louie coverage includes
one native Red cue, no duplicate cue for native player hits, silent misses,
P1–P5 dash one-shots/held Y, early acquisition dispatch, no replay on attachment,
silent-prefix trimming and existing gameplay/state tests. Windows/LLE isolated
Normal routes passed: Red 3,813 frames (fixture observed native `$0238` count
0→1 at the enemy hit, enemy held/resumed); Black 3,642 frames (egg pickup,
dash, protected enemy push/resume). Black's native cue/attachment frame counters
were 54/101: the previous dispatch was 47 simulation frames later in this route.
No interpreter caps, architectural BRKs, dispatch abandonment or fatal errors.
Raw captures stay in `.bootstrap/louie-audio-validation/`. Production build
returns to `SB5_COOP_INPUT_FIXTURE=OFF`. Dummy audio and command/timing tests
are evidence of dispatch/mixing; audible device latency is not measured.

Red passou a tocar uma vez o mesmo SFX nativo de acerto contra jogadores ao
congelar um inimigo do Normal, sem duplicação quando a busca nativa já acerta
um jogador. Black toca o som de arrancada do SB6 ao ativar Y; o som do ovo foi
antecipado para o ponto nativo usado pelos demais Louies. O silêncio digital
inicial dos WAVs é ignorado em memória, sem modificar os arquivos locais.
Testes nativos e rotas Windows/LLE de Red/Black passaram; nessa rota, o som do
ovo deixou de esperar os 47 frames até a montagem. Fixture desligada no build
normal. A latência audível do dispositivo não foi medida com áudio dummy.


## Louie improvements / Melhorias dos Louies — 2026-10-15 (v0.11.0)

Branch `codex/louie-improvements` starts at local `main` (`9ceb220`). Adds the
optional, default-disabled `superbomberman5.louie-improvements` 0.1.0 package:
Yellow rolls bombs and transfers motion across bombs/soft blocks; Red freezes
Normal enemies with the native Clock helper for 180 simulation frames; Black
(type 6) joins the egg lottery and gains a protected Y dash that pushes players
and Normal enemies. The maintainer selected the SB6 `MountBlackLouie.wav`;
it is loaded from an ignored local resource, never committed or packaged.
Native cartridge graphics, allocation and mount-loss/death sounds are retained.
These development notes preceded release preparation; v0.11.0 now includes
the plugin and approved additional WAVs. Earlier hosts lack this native plugin. See [LOUIE_IMPROVEMENTS.md](LOUIE_IMPROVEMENTS.md).

Evidence (Windows x64, LLE, local JPN image, SDL dummy video/audio):
- Build and eleven native test executables pass. New `louie_tests` covers
  five seats, Battle/Normal player push, Normal-only enemy freeze, pause,
  180-frame lifetime, dead-target cleanup, Clock timer restoration, own-cell
  occupancy, bomb ownership, collision callback resumption, scheduler capacity,
  WAV registration, old/disabled/corrupt state loading and ROM restoration.
- Normal Zone 1-1, Red, 3,813-frame route: enemy world position (60,40) and
  animation speed 0 stayed unchanged across the held capture; after the timer
  the enemy moved to (51,40) with animation speed 1. Rider kept its mount.
- Yellow, 3,636-frame route: native ground-motion flags `$11`; bomb moved from
  cell `$33` through `$34` to `$36` without the Blue Louie's airborne throw.
- Yellow mixed chain, 3,724 frames: three real native bombs plus a soft block
  placed as fixtures; impulse went bomb → bomb → soft block → third bomb.
  Assertions checked both transferred bombs moved and the original block cell
  no longer held the block. One scheduler task per contact; no repeated
  callback allocation. Raw captures stay local.
- Black, 3,642 frames: deterministic native egg pickup of type 6, cartridge
  graphics inspected; first hit held the dash and pushed an enemy from about
  x=68 to x=96. Rider invulnerability bit was set during impact and expired
  afterwards; enemy animation resumed. The 48-pixel maximum dash and obstacle
  clipping also have native unit coverage.
- Five-player Normal + PT-BR, 3,756 frames: P5 acquired type 6 in companion
  actor 9, used Y through the real multitap/fixture input path and pushed enemy
  actor 10. Final roster `$1D25=$1F`, mount boundary `$1CBF=5`; P5 stayed mounted.
- Final production build has `SB5_COOP_INPUT_FIXTURE=OFF`; a separate
  3,585-frame mod-disabled control reached Zone 1-1, collected a native egg
  and accepted Y without guest errors. Strict `-Wall -Wextra -Werror` syntax
  checks passed for the new module/tests. Publication history audit and
  `git diff --check` passed; Python checks: seven pass, two Linux AppRun checks
  skipped on Windows. The `.snesmod` contains only manifest + LICENSE.
- `tests/louie_runtime.py` assertions passed for Red, the full mixed chain and
  P5 + PT-BR. Isolated folders/fresh SRAM; opt-in `louie_fixture.c` chooses the
  egg but leaves allocation/pickup/attachment to native code. No guest errors,
  interpreter caps, architectural BRKs or dispatch abandonment in these logs.

Limits: no full campaign/boss qualification, physical controllers, live
save/rewind UI, Battle gameplay route, every stage gimmick or all other mod
combinations. Player push in Battle and P1–P5 uses unit coverage; P5 enemy push
has runtime evidence. Dummy audio verifies the code/mixer path, not subjective
listening quality. The normal executable is rebuilt with
`SB5_COOP_INPUT_FIXTURE=OFF`. The original local launcher configuration is
backed up in `.bootstrap/`; only its old Bomber Kings feature (absent from this
main-based branch) is disabled, and Louie improvements is enabled locally.
The existing submodule working changes are preserved.

Mod opcional dos Louies na branch `codex/louie-improvements`, baseada na
`main`, sem release nem alteração de versão. Yellow chuta bombas e transmite
movimento em cadeias mistas; Red congela inimigos do Normal por 180 frames;
Black sai dos ovos e usa Y para uma arrancada protegida com empurrão. O áudio
escolhido do SB6 fica em arquivo local ignorado. Build Windows e onze testes
nativos passaram; rotas LLE confirmaram Red, Yellow, cadeia completa e P5
montado no Black com co-op/PT-BR. Capturas brutas ficam em `.bootstrap/`.
Não há validação de campanha completa, chefes, controles físicos, interface de
save/rewind, gameplay Battle ou todas as combinações de mods. O executável
normal é recompilado sem a fixture de teste. Configuração local anterior
preservada em backup; Bomber Kings desativado só nessa configuração por não
existir no código da `main` usada como base.


## v0.10.0 package/publication verification — 2026-10-15

Published prerelease: [v0.10.0](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.10.0),
tag `a1f2eae6d4b24371147909ca0af396c5a107c82b`.
[Release run 38781163438](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/actions/runs/38781163438)
and source checks passed. Public Windows ZIP: 16,809,231 bytes; Linux AppImage:
13,466,104 bytes; SHA256SUMS: 218 bytes. Uploaded sizes/digests, annotated tag,
prerelease state, exact UTF-8 notes and public checksum download were verified.
Both payloads match the release commit/pins and contain ten default-disabled
mods. ZIP audits and every internal FILE_SHA256SUMS entry passed.

The extracted Windows ZIP generated/rebuilt using the user's ROM and bundled
analyzer; all ten native tests and optional cartridge-font checks passed.
The root setup handed off to the rebuilt launcher on first/repeat hidden
startup smoke tests. The AppImage passed first/repeat writable installation
and bundled synthetic-ROM analyzer checks under WSL Ubuntu. These Linux setup
checks do not qualify gameplay of the new Gold/PT-BR changes.

The executable rebuilt from the downloaded Windows ZIP passed arena 1/13
confirmation (74 translated samples plus original controls, native blink,
Portuguese names, exact guest-memory restoration and identical gameplay after
the overlay ended), controlled Hyper cart knockout/reentry with enabled and
disabled controls, equipment transfer and combined pause Draw/restart.
Pause routes passed for Normal 1/2/5 seats and Battle, including native SFX
commands, safe defaults, confirmations, exits and reentry. Subordinate routes
passed for locked/unlocked Battle, unlocked Normal with Honey/Kotetsu and
an unlocked disabled-mod control, including standard Bomberman yellow L/R.

Raw captures, generated code, saves and package tests remain in ignored
`.bootstrap/v0100-*`. No full-campaign, physical-multiplayer or HLE claim.

Publicada a pré-release v0.10.0 com ZIP Windows, AppImage Linux e SHA256SUMS.
Tag, notas UTF-8, arquivos e hashes foram confirmados no GitHub; auditorias e
checksums internos passaram. O ZIP extraído gerou/recompilou o jogo e passou
as rotas Windows de Gold/Hiper, confirmação PT-BR, pause/áudio e Subordinado
acima. O AppImage passou instalação/reabertura e analisador no WSL; gameplay
Linux novo continua sem qualificação específica. Dados locais ficam ignorados.


## v0.10.0 release source / Fontes da release v0.10.0 — 2026-10-15

Release scope: Gold Battle (arenas 11–13 / Hyper Revenge), native-style PT-BR
headings and arena names/confirmation, translated pause options and corrected
entry/resume sounds, and Subordinate in both locked/unlocked secret-cell paths.
The ten preloaded mod packages remain 0.1.0 and disabled by default. Versioned
source and Windows/Linux setup artifacts use host version 0.10.0.

A separate source checkout retained the published framework pins
(snesrecomp 2c25f35, recomp-ui 7aa25f4), with no generated game sources.
Windows x64 ROM-independent setup-host build, all ten native test executables,
Python checks (two Linux-only skips on Windows) and setup `--help` passed.
Feature runtime routes and their exact limits are recorded in the sections
below. Package/CI verification is recorded after publication. This does not
qualify full-game accuracy, physical multiplayer or broad Linux support.

A release reúne Gold Battle, fontes/menu/arenas PT-BR, pause/áudio e correção
da célula secreta do Subordinado. Checkout separado preservou os pins públicos,
sem código gerado. Build Windows do setup sem ROM, dez testes nativos, testes
Python e `--help` passaram. As evidências de gameplay e seus limites estão
abaixo; Linux continua experimental e o jogo continua parcialmente interpretado.


## Battle Start translation / Aviso de início da batalha — 2026-10-15

Current sources translate BATTLE START as INÍCIO DA BATALHA. The nine native
16×16 OBJ retain their pink palette, position and blink. Object metadata now
supports a non-row-aligned first tile ($A2) and palette selection. Arena names
and Fase also stay Portuguese through confirmation: native blink removes the
heading, moving the Stage strip and number to earlier OAM slots. Recognition
uses those complete native signatures rather than a fixed numeric slot.

Native tests passed for all 13 numbers in selection, visible confirmation and
blink-off confirmation, with exact restoration and wrong-palette rejection.
Windows/LLE paired routes passed for arena 1 without Gold and arena 13 with
Gold: 37 captured frames each (74 per language), including 32 consecutive
confirmation frames. Native pink heading was visible in 17 samples per arena;
blink-off samples remained blank. Arena names passed native-font pixel checks
through both phases. WRAM/SRAM/VRAM/OAM/CGRAM matched translation-off controls,
and the gameplay frame was identical after the overlay disappeared. Routes
simulated 6,213 / 6,489 frames and reached default native modes $74 / $A4.
Captures stay in `.bootstrap/battle-start-validation/`. Native/Python tests
passed (two Linux-only skips); desktop-shortcut executable rebuilt.
Evidence covers these routes and samples, not every Battle variant or HLE.

BATTLE START agora aparece como INÍCIO DA BATALHA, com paleta rosa e piscar
originais. Fase e nome da arena permanecem em PT-BR mesmo quando o aviso
apaga. Testes nativos cobrem as 13 arenas nas três fases de apresentação;
as rotas pareadas das arenas 1 e 13 passaram em 74 quadros por idioma, com
memória nativa idêntica e imagem da partida idêntica após o fim do aviso.
Executável do atalho recompilado. Cobertura limitada às rotas descritas.


## Heading stroke restoration / Traços dos cabeçalhos — 2026-10-15

Fixed blue E/F/H crossbars and large SENHA lettering: bright native fill
uses palette index 11, also used by the banner background. Cropping to ink
bounds exposed the crossbar to the background flood fill and removed it.
Font metadata now includes two outline columns on each side (four source
tile columns per row). Background removal runs on that padded crop before
extracting the letter. Ink moves down one row before outlining so the upper
edge fits the 16-pixel canvas, including the transparent arena-selection title.

Native tests cover a synthetic white H crossbar touching both ink edges and
its top outline. Optional `ptbr_render_tests.exe <reference-ROM>` also checks
E/F/H bevel bars directly from the user's cartridge and after OBJ palette
mapping. Both passed. Paired Windows/LLE routes passed on 22 captured frames,
50 translated label contexts and 12,951 simulated frames per run, with exact
WRAM/SRAM/VRAM/OAM/CGRAM parity. Visually inspected SENHA, TIPO DE BATALHA,
ESCOLHA O PERSONAGEM and ESCOLHA A FASE. Captures remain ignored under
`.bootstrap/heading-strokes-validation/`. All native and Python tests passed
(two Linux-only skips). The desktop-shortcut executable was rebuilt.
Evidence covers these routes and glyphs, not every font/input/renderer variant.

Restauradas as barras de E/F/H, inclusive no título SENHA, e o contorno
superior de ESCOLHA A FASE. A faixa clara dos glifos era confundida com o
fundo porque faltava margem no recorte; agora a separação preserva o contorno
nativo. Testes sintéticos e com a ROM local verificam esses traços. A rota
pareada passou em 22 quadros e 50 contextos, com memória nativa idêntica;
quatro cabeçalhos foram inspecionados visualmente. Executável recompilado.


## Native translated menu fonts / Fontes nativas na tradução — 2026-10-15

Current source builds use cartridge-derived beveled blue headings, proportional
8×16 menu letters and native white title letters throughout the shared PT-BR
renderer. Body labels follow native mixed case. Large password/Game Over
lettering scales in both axes; title margins and blink remain native.
Glyph locations/crop coordinates are metadata only; pixels come from the
user's ROM at runtime. Accents, missing blue J/Q/V and fallback signs are
authored; missing white letters can be composed from native pieces.

Windows/LLE paired tests (`tests/ptbr_typography_runtime.py`) passed on 22
captured frames and 50 recognized label contexts across title/player menus,
Normal/file menus, options, password, sound test, Battle setup, character
selection, rules and arena transitions. Translated/original runs had identical
WRAM, SRAM, VRAM, OAM and CGRAM at every capture. The final build also passed
109 arena-name pixel checks across arenas 1–13, including transition frames
(`.bootstrap/native-arena-regression/gold-2u968x4f`, 5,932 simulated frames).
Final ending/perfect fixtures
also passed Game Over, reward, certificate and tips comparisons. HUD blink
fixtures passed before the final change to large-letter scaling/cache; those
changes do not affect the HUD path. Native tests cover synthetic font data,
white-title margins, banner background removal and exact restoration; all
native executables and Python tests passed (two Linux-only skips). Publication
history audit passed. The desktop-shortcut executable was rebuilt.

As fontes atuais usam o relevo azul, letras proporcionais e fonte branca
nativos em todos os caminhos do renderer compartilhado da tradução, com
acentos e complementos autorais. Foram comparados 22 quadros e 50 contextos
com PT-BR ligado/desligado, mantendo toda a memória nativa idêntica. Senha,
Fim de Jogo, recompensa, certificados e dicas foram inspecionados. O executável
do atalho foi recompilado; testes e auditoria passaram. Capturas ficam em
`.bootstrap/ptbr-typography-final/` e `.bootstrap/native-completion-final/`.
A evidência cobre essas rotas Windows/LLE e os fixtures descritos; não comprova
toda sequência de entrada, toda variante de menu, HLE ou campanha completa.


## Battle arena translation / Tradução das arenas — 2026-10-15

Current sources translate Fase and all 13 arena names with the native 8×16
OBJ font read from live VRAM, adding authored accents. Original ten arenas
work without Gold Battle. Displayed number sprites select the translation,
avoiding WRAM/script timing flashes; all borrowed OAM/VRAM is restored.
Windows/LLE pixel checks passed on 109 frames across 13 arenas and 82 frames
across ten arenas without Gold, including eight consecutive frames per
forward transition. Captures remain in `.bootstrap/battle-translation-validation/`.
Native rendering tests cover all numbers, disabled behavior and restoration.
This qualifies those routes and frames, not every input sequence or HLE.

As fontes atuais traduzem Fase e os 13 nomes com a fonte OBJ nativa e acentos
autorais. As dez arenas originais dispensam Gold Battle. A tradução segue o
número exibido; os 109/82 quadros verificados, incluindo transições, não
mostraram texto original. OAM/VRAM são restauradas. Evidência limitada às rotas
Windows/LLE descritas, sem qualificação de HLE ou de toda sequência de entrada.



## Subordinate secret-cell correction / Correção da célula secreta — 2026-10-15

Current sources replace both native cell-9 branches with Subordinate Bomber
when its mod is enabled. Previously only the locked X branch was replaced;
a save with secret yellow Bomberman unlocked bypassed it. Both branches now
store character 10 / default preview color 6, without changing the unlock flag.
The standard Bomberman still cycles to yellow with L/R. With Subordinate off,
the original unlocked yellow cell remains unchanged.

Windows/LLE isolated runs passed: Battle locked (10-cell grid), Battle unlocked
with Honey/Kotetsu (12-cell grid), Normal unlocked with Honey/Kotetsu, and
unlocked Battle with Subordinate disabled as a control. Each enabled route
selected cell 9 and reached gameplay as character index 9; the disabled control
reached index 0. Each route also cycled the standard Bomberman to yellow first.
SRAM unlock byte `$1E9` was preserved. Battle routes: 6,393 frames; Normal:
4,847 frames. All ten native tests and Python tests passed (two Linux-only
skips); publication audit passed. The desktop-shortcut executable was rebuilt.
No full-campaign or physical-controller claim. Raw captures stay ignored.

Corrigidos os dois caminhos da célula: X e Bomberman amarelo secreto. Com o
mod ativo, o Subordinado continua disponível mesmo em saves desbloqueados,
sem alterar o desbloqueio. Battle/Normal, Honey/Kotetsu, cores e controle com o
mod desligado passaram nas rotas descritas acima.

## Gold Battle and translated pause / Gold Battle e pausa traduzida — 2026-10-15

Current-source feature, not part of the published v0.9.0 setup packs:
optional, default-disabled `superbomberman5.gold-battle` 0.1.0 enables Battle
arenas 11–13 and Hyper Revenge. Both user-supplied ROMs contain identical
scripts for these arenas. No Gold ROM dependency or extracted game data is
introduced. The regular cartridge supplies all arena resources. Authored
in-memory VM code at `$C7:3000-$30FF` adds the fourth Revenge value; native
Super handles reentry/survivors, with host-side capture/transfer of the defeated
opponent's equipment. All patches are reserved/verified/restored; SRAM layout
and ROM files stay unchanged. Fixed 35-byte validated state extension follows
Pause and includes a pending item-drop decision. Details: `docs/GOLD_BATTLE.md`.

Pause options and confirmation now follow PT-BR, including NÃO's authored
accent. Native pause entry (`$C5:0096`) and resume (`$C5:0037`) audio calls are
preserved; Resume/B do not queue an additional menu confirm/back sound. Native
menu move/accept/back sounds remain for the other actions. Labels live in the
authored translation JSON; Hyper/HIPER renders with and without PT-BR.

Windows x64 LLE evidence, local JPN ROM, 2026-10-15:
- Build and all ten native test executables passed; Gold tests cover all four
  rules in both directions, preservation of other flags, all twenty ordered
  seat pairs, equipment/state validation and patch/hook restoration. PT-BR
  and pause tests cover new labels and exact draw restoration. Extra arena
  names/numbers use authored BG3 labels, with native text OBJ temporarily
  hidden/restored. The stage-selection banner no longer depends on an arena-1
  preview tile signature.
- Three isolated PT-BR Battle runs reached arenas 11/12/13, native default
  modes `$9C/$A0/$A4`, after cycling every Revenge option. 7,016 / 7,039 /
  7,062 frames; each arena observed for 900 frames after confirmation.
  Another 6,786-frame run exercised arena 1 and HYPER with PT-BR disabled.
- Controlled native P1 death/cart → owned-flame contact with P2 → native
  reentry transferred bomb count 4, range 5, speed 3, glove bits `$0C` to P1.
  The matched stock-Super control restored defaults 1/2/1/0. Both runs also
  enable Pause and PT-BR; the native tests cover the other seat pairings.
- `tests/pause_menu_runtime.py --case all --new-renderer` passed Normal 1P,
  Normal 2P with other mods/PT-BR, Normal 5P and Battle: pause menu, safe
  confirmation defaults, native SFX commands, draw/map/arena exits and reentry. APU timestamps
  confirm the native pause command is sent inside the entry interval and again
  inside the resume interval, rather than deferred until resuming. Gold + PT-BR
  + Pause also passed Force Draw and native round restart after Hyper reentry.
- Python tests: nine discovered, seven passed, two Linux-only tests skipped.
  Publication source/history audit and `git diff --check` passed. The local
  `.snesmod` contains only manifest/license. No release/version bump or commit.

Limits: no claim of full arena-variant/team/Super Battle qualification, physical
five-player controls, long matches or rewind during equipment transfer.
Raw captures remain under ignored `.bootstrap/`. The existing dirty submodule
checkouts were preserved; no framework changes were made for this feature.

Nas fontes atuais, o novo mod opcional libera as arenas 11–13 e Hiper; não é
necessário carregar a ROM Gold. Pausa e confirmação acompanham o PT-BR e os
sons originais de pausar/retomar foram preservados. Build Windows, dez testes
nativos, rotas das três arenas, ciclo de regras, transferência controlada de
poderes e as quatro rotas de pausa passaram. Sem commit, release ou alteração
do cartucho/SRAM. Ver os limites acima e `docs/GOLD_BATTLE.md`.

## v0.9.0 published / Publicada — 2026-10-15

Published prerelease:
https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.9.0
Tag and both packages' SOURCE_REVISIONS target
`271fc1963fbc0931be7b0afe30c8e71e3e5d9385`. Release workflow `38734943052`
(and the preceding test run `38733401457`) passed Windows/Ubuntu builds, all
nine native tests, Python checks, source/package audits, analyzer smoke and
AppImage first/repeat startup. Framework pins are unchanged from v0.8.2.

Public assets were downloaded and match SHA256SUMS: Windows ZIP
`1b3fa772...f868` (16,743,922 bytes), AppImage `8203ec81...f748`
(13,412,856 bytes). The ZIP passes the package audit, contains the Honey and
Kotetsu source/manifest and no `docs/images`, generated code or ROM/save
files. The extracted AppImage payload verified every FILE_SHA256SUMS entry and
its bundled analyzer smoke; on Linux (cloud container, local SDL3) local ROM
generation and rebuild from that payload passed all nine native tests, the
3,628-frame first-stage route reached Zone 1-1, and a Battle route with PT-BR,
colors, Subordinate and Honey/Kotetsu reached stage 1 with Honey (P1) and
Kotetsu (seat 2) drawn. No interpreter cap, BRK or fatal. The release body
matches `tools/release-notes.py` byte-for-byte (UTF-8, no U+FFFD); its title
kept the workflow default because this session could not edit releases. The
extracted Windows package was not run on Windows from this session.

Pré-release v0.9.0 publicada com ZIP Windows, AppImage Linux e checksums.
Hashes, auditoria do ZIP e integridade do AppImage conferidos; o pacote Linux
gerou/recompilou o jogo com a ROM local, passou nos nove testes, na rota de
3.628 quadros e numa rota de Battle com Honey e Kotetsu. O teste do pacote no
Windows fica pendente.

## v0.9.0 preparation — Honey and Kotetsu / Preparação — 2026-10-15

Release scope: v0.8.2 plus the optional **Honey and Kotetsu — playable
characters** mod (`superbomberman5.honey-kotetsu` 0.1.0, host v0.9.0+), default
disabled, and the helpers the picker mods now share (`PickerCellCharacters()`,
`PickerCellColors()`, `HoneyKotetsuBaseChoice()`). Framework pins unchanged
from v0.8.2. The README showcase keeps the five-player title frame and replaces
the other three with v0.9.0 captures (six-column Battle picker with Honey,
Kotetsu, Subordinate Bomber and colors; Zone 1-1 and Battle stage 1 with five
different characters and palettes). The mod-development skill gained the
playable-character recipe and checklist learned from this mod.

Pre-publication evidence (Linux, this source tree with pinned submodules,
locally regenerated code): build, all nine native test executables, Python
tests, publication audit and `git diff --check` pass. Showcase captures used a
local opt-in `SB5_COOP_INPUT_FIXTURE=ON` host to drive pads 3-5 (never shipped):
five-seat Battle picker → stage 1 and five-player Normal Zone 1-1 with
Honey/Kotetsu/Subordinate Bomber and colors, no interpreter cap, BRK or fatal.
Windows evidence comes from the release workflow; Windows runtime of the new
mod, physical controllers and full campaign remain unqualified.

Preparada a v0.9.0 com o mod Honey e Kotetsu, imagem do README atualizada e
skill de mods ampliada. Build Linux, nove testes nativos, testes Python e
auditoria passaram; validação Windows vem do workflow de release.

## Honey and Kotetsu playable / Honey e Kotetsu jogáveis — 2026-10-15

Optional, default-disabled `superbomberman5.honey-kotetsu` 0.1.0 turns the
picker into a 6x2 grid: stock columns packed to tiles 5/9/13/17/21, Honey
(choice 11) and Kotetsu (choice 12), the secret-stage bosses of `$71-$73`, at
tile 25. In-memory patches relocate the cell arrays to `$7F:F800/$7F:F80C`,
replace cursor movement with lookup tables, extend cursor positions, clamp the
portrait helpers, and map choices 11/12 to Bomberman at Battle stage init
(`$C4:2A3C`) and the score board (`$C4:3BDF`), recording each seat's choice at
`$7F:F818`. Their sprites are render-only: object set `$0D` frames composed from
the `$CC:289D` archive (decompressed at runtime, `$C0:1CCC` format) with
palettes `$C1:8FC6/$C1:8FA6`, replacing the bomber's OBJ sprites; HUD face and
seat portrait use the head of the front frame. Subordinate Bomber's X-skip
trampoline now tests the character in A instead of reading `$62C9`, and the
color/Normal-character mods read the cells through `PickerCellCharacters()`.

Evidence (Linux LLE build of the same sources, JPN ROM): new
`honey_kotetsu_tests` and all other native test executables pass; Python tests
pass. Scripted: Battle picker (movement/wrap, portraits, PT-BR), five-seat
Battle with P1 Honey and seat 2 Kotetsu for ~9,600 frames through the score
board and next round, Normal Zone 1-1 as Honey, `ZONE1 - S` as Honey and
`ZONE3 - S` as Kotetsu, controls with the mod off. No BRK, `interp_cap`,
unresolved abandon or fatal. Not qualified: Windows build, physical controls,
full campaign, coop P3-P5 as them, Louie. Details in `docs/HONEY_KOTETSU.md`.

Update: poses now follow the bomber animation groups — glove punch/throw use
their attack frames (Honey's gun, Kotetsu's spear), victory/cheer a victory
frame, carrying walks, death/stun/Time Up spin and the last death frame stays
native (ashes). Wide frames get a second block column. Verified with
side-by-side Battle routes (power/boxing glove items placed by poke, own-bomb
death, stun poke, victory, Time Up); egg items gave no visible Louie even
without mods, so riding stays unverified.

Update 2: the replaced sprites are now exactly the pieces of the bomber's
current frame (`$7E2EEA/$7E2F4A`, with the last three distinct pointers for
OAM lag), drawn in a free OBJ palette, so the Louie mount and eggs sharing the
seat's slot keep their sprites and colors; riding (`$1782` bit 7) shows the
figure on the mount. Victory, cheer and the DRAW GAME screen alternate the two
champion-screen poses (set 6 anims `$50`/`$4E`, archive `$D6:D871` decoded at
runtime). Verified by Linux LLE routes: egg `$15` collected and ridden, DRAW
GAME, victory (60 frames), champion screen with each as winner (48 frames each
dumped every frame) and walking (60 frames) without native fallback frames;
glove/stun/death/picker regressions; all native tests pass.

Honey e Kotetsu podem ser escolhidos numa sexta coluna da seleção (Honey em
cima, Kotetsu embaixo, ao lado do Subordinate/X), no Battle e no Normal. Jogam
como Bomberman e aparecem com os próprios sprites, retrato e rosto no HUD,
lidos da ROM.

## v0.8.2 published / Publicada — 2026-10-15

Published prerelease:
https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.8.2
Tag targets `fc66241464a23ed4d8749da7a726df2ad6e34616`, matching both
packages' SOURCE_REVISIONS. Release workflow `38728341451` passed both
Windows and Ubuntu builds, native/Python checks, publication audits, analyzer
smokes and AppImage writable first/repeat-launch checks. Existing tags/assets
were preserved. The same eight mods remain preinstalled/default-disabled;
only PT-BR coverage, tests and release documentation changed from v0.8.1.

Downloaded CI archives matched GitHub artifact digests. Both inner setup ZIPs
passed publication audit, eight-mod/source-revision checks and payload
FILE_SHA256SUMS verification. Public Windows ZIP (16,685,351 bytes), Linux
AppImage (13,363,704 bytes) and SHA256SUMS (216 bytes) were verified against
each uploaded asset's size and SHA-256 digest. The published UTF-8 title/body
match their local source exactly without replacement characters.

Extracted Windows setup launcher startup passed. Both packages' bundled
analyzers ran, then generated/rebuilt the game locally using the user ROM;
all eight native tests passed on each OS. The rebuilt Linux AppImage also
completed the 3,628-frame first-stage route. On both OSes, packaged
`ptbr_completion_runtime.py` passed Continue/Erase headings, 0% Game Over →
certificate → tips (5,347 frames), 100% Game Over → password reward →
Perfect Bomber certificate → tips (5,952 frames), and both HUD messages on
both sides with blink-off frames (3,725 frames). Additional HUD runs passed
with NewRenderer=1 on each OS, including final APERTE START wording.
Translated/original WRAM, VRAM, OAM, CGRAM and SRAM match byte-for-byte;
reward changes stay within its three text lines, preserving the password.
No interpreter cap, unresolved abandonment, architectural BRK or fatal.
Raw local evidence: `.bootstrap/v082-{windows,linux}-ptbr[-fast]/` and
`.bootstrap/v082-*-{build,generate,runtime}.log`. These forced progress/death
fixtures do not qualify natural playthroughs for all ranks, full campaign,
physical multiplayer, other Linux distributions, macOS or HLE.

Pré-release v0.8.2 publicada com ZIP Windows, AppImage Linux experimental e
checksums. Título/notas bilíngues UTF-8, tag, workflow, tamanho e hash de cada
asset foram confirmados. Os dois pacotes extraídos geraram/recompilaram o
jogo; oito testes nativos passaram em cada sistema. Rotas PT-BR de arquivos,
0%/100%, senha e HUD passaram nos dois sistemas, com memória nativa intacta;
APERTE START também passou nos dois compositores LLE. Linux completou a
rota inicial de 3.628 quadros. Campanha completa e demais limites acima
permanecem. Importar só o .snesmod não atualiza o código: use Generate & rebuild.


## v0.8.2 PT-BR completion coverage / Telas de conclusão — 2026-10-15

Expanded the same default-disabled PT-BR plugin: Continue/Erase headings,
Normal Game Over title/choices, HUD APERTE START / FIM DE JOGO on either
side, the 100% password reward, and all 33 certificate ranks and tips.
Native rank selection and layout guards were audited from the local JPN ROM,
including secret stages. Text and glyphs are authored; native percentages,
passwords and blink timing are preserved. Package version remains 0.1.0;
Generate & rebuild with host v0.8.2 is required to install the native change.

The maintainer confirmed the translated screens and APERTE START wording.
Local Windows development checks passed all nine native tests (the release
retains its existing eight; unrelated Honey/Kotetsu source is excluded),
seven Python tests and publication audit. PT-BR native regression covers
all 33 ranks on both pages, negative guards, digit preservation, the opaque
parchment signature, shared-sprite rejection, both HUD positions and VRAM
restoration. Isolated LLE death/progress fixtures exercise 0% Game Over →
certificate → tips (5,347 frames), 100% Game Over → password reward →
Perfect Bomber certificate → tips (5,952 frames), Continue/Erase headings
and both HUD messages/blink-off frames (3,725 frames). The 100% and HUD
routes passed both compositors; all four 100% framebuffers match. Native
WRAM/VRAM/OAM/CGRAM/SRAM are identical between translated/original runs.
Reward differences stay in its three text lines; the password is unchanged.
The later APERTE START wording uses compact spacing and passed native tests
and maintainer runtime confirmation. Clean v0.8.2 setup-host Windows compilation/linking and all eight release
native tests also passed with the unchanged v0.8.1 dependency pins.
Release package checks follow below.

This release is based on v0.8.1, retaining its framework pins and eight mods.
No other local gameplay work is included. Full natural campaign routes for
every rank, physical multiplayer and HLE remain unqualified. Raw evidence is
local under `.bootstrap/ptbr-completion-validation/` and `ptbr-completion-fast/`.

PT-BR ampliado para arquivos, Fim de Jogo/Continuar/Sair, HUD dos dois lados
com APERTE START, recompensa com senha e todas as 33 classificações/dicas,
incluindo fases secretas. Confirmado pelo mantenedor. Porcentagem, senha,
piscar e memória nativa são preservados; rotas de 0%/100% e HUD passaram
nos dois compositores LLE. É preciso gerar/recompilar com o host v0.8.2.
Campanha completa e os demais limites acima continuam sem qualificação.

## v0.8.1 published / Publicada — 2026-10-15

Published prerelease:
https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.8.1
Tag and both packages' SOURCE_REVISIONS target
`01ea6aa51751ef63f743abdf2cf4739d26828e85`. Release workflow `38720100087`
passed Windows/Ubuntu builds, all eight native tests, Python checks,
source/package audits, analyzer smoke and AppImage first/repeat startup.
The eight existing mods remain default-disabled; unrelated Honey/Kotetsu
work was excluded. Framework pins are unchanged from v0.8.0.

Both downloaded workflow archives match their GitHub artifact SHA-256.
Extracted Windows/Linux setup packages passed bundled-analyzer smoke, local
ROM generation/rebuild and all eight native tests. Windows setup launcher
remained running normally during a startup smoke test. Linux rebuilt AppImage
completed the 3,628-frame first-stage route. On both OSes, the packaged
selection-color regression completed Normal (3,636 frames) and Battle
(4,750 frames) with PT-BR, covering six colors across five columns, initial
seat colors 00/01/02/03/04 and byte-identical lower palette strips. No
interpreter cap, unresolved abandonment, architectural BRK or fatal.
The maintainer also confirmed the original local Windows correction.

Public asset sizes/SHA-256 digests, three-asset prerelease state, tag target,
default branch and exact UTF-8 bilingual release body were verified through
the GitHub API. Evidence stays local under `.bootstrap/v081-*`.
No ROM or ROM-derived executable was published. Full campaign, physical
five-controller play and broader Linux distributions remain unqualified.

Pré-release v0.8.1 publicada com ZIP Windows, AppImage Linux e checksums.
Os pacotes extraídos geraram/recompilaram o jogo e passaram nos oito testes
nativos nos dois sistemas. A regressão Normal/Battle com PT-BR passou nos
dois pacotes; cores iniciais e paletas inferiores foram verificadas nas seis
cores das cinco colunas. Notas UTF-8, tag, assets e hashes conferidos no GitHub.
Nenhuma tag ou asset anterior foi substituído; o trabalho local foi preservado.


## v0.8.1 color correction / Correção de cores — 2026-10-15

P1 starts white, P2 black, P3 red, P4 blue and P5 green, including CPU seat
handoff. Grid palette changes preserve the upper strip's shared index 8,
preventing the figure below from becoming brighter. Native tests cover five
defaults and all six variants across five upper strips. Save-state size and
mod package version 0.1.0 remain unchanged; rebuilding host v0.8.1 is required.

Maintainer confirmed the fix in the local Windows game. Fresh-SRAM Windows/LLE
Normal (3,636 frames) and Battle (4,750 frames) routes with PT-BR capture all
six colors across five stock columns, verify seat colors 00/01/02/03/04 and
unchanged lower palette strips. No interpreter cap, unresolved abandonment,
architectural BRK or fatal. Local evidence: `.bootstrap/selection-color-fix/`.
The release branch excludes unrelated local Honey/Kotetsu development and
keeps the v0.8.0 framework pins. Release package checks are recorded in the publication section above. Full campaign and physical five-controller play remain unqualified.

P1–P5 começam em branco, preto, vermelho, azul e verde. Corrigido o brilho
indevido no personagem abaixo, confirmado pelo mantenedor e por rotas com
PT-BR no Normal/Battle. Exige gerar/recompilar o jogo com o novo host.


## v0.8.0 published / Publicada — 2026-10-15

Published prerelease: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.8.0
Tag targets `34613f7a2c1825a906a751e465a1aa604a559d6c`, matching both setup
packages' SOURCE_REVISIONS. Release workflow `38692068021` passed Windows
and Ubuntu builds, all eight native tests, Python checks, source/package
audits, native analyzer smoke and AppImage first/repeat launch checks.
The eight game mods are preinstalled/default-disabled. Supplied screenshot
`docs/images/portal-destinations.png` is byte-identical to the maintainer's
attachment and appears in the English and Portuguese README explanations.

Downloaded artifacts match GitHub Actions archive digests. Public Windows ZIP,
Linux AppImage and SHA256SUMS asset sizes/digests were verified after upload;
the bilingual UTF-8 release body matches its source without replacement chars.
Pristine Windows setup launcher opened and closed normally. Both extracted
packages passed bundled-analyzer smoke, local ROM generation/rebuild and
all eight native test executables. The packaged Windows game passed all six
portal fixtures, including independently revealed/hidden gates, both PPUs,
PT-BR/pause integration and exact native-state parity. The Linux rebuilt
AppImage completed the 3,628-frame first-stage route without interpreter cap,
unresolved abandonment, BRK or fatal. Its portable toolchain has no ctest;
all native test executables were run directly instead. No system packages
were installed. Linux portal runtime, full campaign, all boss paths, physical
controllers and rewind across transitions remain unqualified.

Pré-release v0.8.0 publicada com ZIP Windows, AppImage Linux e checksums.
O README bilíngue inclui exatamente a captura fornecida para o novo mod.
Pacotes extraídos geraram/recompilaram o jogo, oito testes nativos passaram
nos dois sistemas, seis rotas do mod passaram no Windows e a rota de 3.628
quadros passou no AppImage recompilado. Campanha completa e os demais limites
acima permanecem. Alterações locais do mod Honey/Kotetsu foram preservadas e
não fazem parte desta release. Nenhuma tag/asset anterior foi substituída.


## v0.8.0 portal destinations / Destinos dos portais — 2026-10-15

Added optional **Gameplay → Normal Game — portal destinations → Show portal
destinations**, default-disabled, in the v0.8.0 release candidate. Previous v0.7.0 packages lack this native
plugin. Release setup-package checks are being performed in a clean checkout.
The read-only presentation plugin resolves exits from cartridge scripts and
checks each live gate's native frame, palette and raster OAM before drawing.
Covered/queued gates and invisible animation frames do not reveal hints;
pause suppresses the labels. The labels share the authored PT-BR title font
via `menu_font.h`, with full-width strokes, 14px height and a dark outline
(10px on the bottom row to fit below the gate). All boss letters use the same
font, independently of whether translation is enabled. Labels
are centered below each gate: Z1-2/Z1-5/Z1-7, Z1-S and Z1-A1/A2/A3, with
map letters and native route flags for other bosses. See PORTAL_LABELS.md
for the shared title font and full naming scheme.

Windows incremental LLE build and eight native test executables pass. The
optional real-ROM native test audits all 192 exit scripts across 90 numbered
and three special stages, including coordinate/destination samples in all
five zones and A1/A2/A3 flags. This audits data, not combat or every route.
Fresh-SRAM runtime fixtures force enemy completion and clear covering blocks
in Zone 1-1; enabled/disabled runs have identical WRAM, restored VRAM, OAM,
CGRAM and SRAM. Hidden/queued gates have no label, the three revealed labels
match the supplied reference, hiding the left gate suppresses its label and
pause suppresses hints. Optimized/accurate PPU and combined PT-BR/pause yield
the same revealed frame. A separate run reveals only the left portal while
the other two remain covered; only Z1-2 appears. The final shared title font
passes all six runs. Bottom-row rendering/restoration is covered natively.
Local evidence: `.bootstrap/portal-labels-validation/` and
`.bootstrap/portal-runtime-title.log`. Seven Python tests, source/history
publication audit and the manifest+LICENSE-only package audit also pass.

Mod e documentação preparados para a v0.8.0, com a captura fornecida pelo mantenedor no README bilíngue. Os 192 portais
passaram na auditoria dos scripts da ROM local. A fixture Windows/LLE da
ZONE1-1 cobre revelação/ocultação e preservação do estado, com os dois PPUs e
PT-BR/pausa. A conclusão dos inimigos e remoção dos blocos são forçadas pelo
teste, nunca pelo mod. Campanha completa, combate natural até cada saída,
todos os chefes, cinco jogadores, controles físicos e Linux ainda não foram
qualificados para este overlay. Nenhum dado extraído ou executável foi adicionado.


## v0.7.0 published / Publicada — 2026-10-15

Published prerelease: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.7.0
Tag targets `0a3aa0a97908b4b8d8f12908fc8463ecf11706c3`, matching both packages'
SOURCE_REVISIONS. Release workflow run `38685519146` passed Windows and Linux
builds, native/Python tests, audits, analyzer/setup smoke and AppImage first/
repeat launch checks. Uploaded Windows ZIP, Linux AppImage and SHA256SUMS
were verified against local sizes/SHA-256; bilingual release body matches UTF-8.

Local package checks: pristine Windows setup launcher and rebuilt launcher
opened; bundled analyzers passed synthetic-ROM checks. Both extracted
packages generated code from the user's ROM and rebuilt locally. Seven native
tests passed in Windows and Ubuntu/WSL. The Windows rebuilt package passed
all four pause routes (Normal 1P/2P/PT-BR/5P, Battle 2 humans/3 CPUs), including
map/arena return and reentry, Draw/restart, Start/B, SFX and opaque text/glove
pixels. Linux AppRun launched the rebuilt game and completed the 3,628-frame
first-stage route with no interpreter cap, unresolved abandonment or fatal.
The local Linux build used the existing portable OpenGL sysroot; no system
packages were installed. Full campaign, all arenas, physical controllers and
rewind across transitions remain unqualified; Linux remains experimental.

README retains its pre-existing showcase and adds bilingual pause-mod details;
the per-mod gallery request was cancelled and no future screenshot requirement
was retained. Gameplay screenshots remain permitted, optional documentation.
No old tag/asset was replaced and no shared history was rewritten.

Pré-release v0.7.0 publicada com ZIP Windows, AppImage Linux e checksums.
Geração/compilação pelos dois pacotes, sete testes nativos, rotas do mod no
Windows e entrada na primeira fase no Linux passaram. O README mantém a
imagem anterior e acrescenta as informações do mod de pausa nos dois idiomas.
Capturas por mod não são obrigatórias. As limitações acima permanecem.


## v0.7.0 preparation / Preparação — 2026-10-15

Adds Pause — exit & draw and its bilingual README instructions. The requested
per-mod gallery was cancelled; the existing showcase is preserved and future
mods do not require a screenshot. Gameplay screenshots are authorized
under docs/images; no history rewriting was needed. The existing README showcase is kept; no new images are added. Source uses the published v0.6.0 dependency pins;
local uncommitted framework edits are not packaged. Publication pending CI.

## Opaque pause text / Textos opacos na pausa — 2026-10-15

The compact panel now dims the field through a bounded color-math window,
with BG3 excluded from color math. Native PAUSE/ZONE artwork, authored menu
options and glove retain their original opaque colors; only the background
is transparent. Scenes already using color math or layer windows retain the
solid-fill fallback. All borrowed window/fixed-color registers are restored
at the panel bottom and after rendering, alongside VRAM/screen state.

PAUSE, ZONE, opções e cursor agora são opacos; somente o fundo mantém a fase
visível e escurecida. Start/B e as seleções iniciais continuam iguais.

Validation: incremental Windows/LLE build and seven native test executables
passed; unit coverage checks BG3 exclusion, transparent fill and restoration
of window/fixed-color registers. Runtime framebuffer assertions check exact
white glyph/glove pixels in main and confirmation menus, preventing blended
text from passing unnoticed. Isolated Normal 1P, combined Normal 2P/PT-BR,
Normal 5P co-op and Battle 2 humans/3 CPUs passed with the optimized PPU;
Normal 1P also passed with the accurate PPU, including the opaque-pixel
assertions. Six Python tests and tracked-source audit passed. History audit
still reports the pre-existing `docs/images/five-player-showcase.png`
provenance issue documented below. Captures remain local in `.bootstrap/pause-menu-validation/`; no game assets are added.
Full campaign/all arenas, physical controls and rewind remain unqualified.

## Compact pause panel / Painel compacto de pausa — 2026-10-15

Pause menu controls now use Start to accept the selected action, including
No/Yes; A is ignored and B retains back/resume behavior. Held Start cannot
accept the next screen. Resume and No remain the initial selections.
A compact panel preserves the native PAUSE heading and paused field, with
options immediately below the heading. Authored BG3 glyphs use unused tiles
before live OBJ pages; CGRAM and sprite tiles are untouched. VRAM/screen
registers are restored after drawing. Background blending is enabled only
when the scene has no native color math; other scenes use a solid fill.
This supersedes the Start-ignored confirmation behavior recorded below.

Controles: Start confirma a opção selecionada, A é ignorado e B mantém
voltar/retomar. O painel compacto preserva PAUSE e a fase ao redor, com as
opções abaixo do título. Resume e No continuam sendo as seleções iniciais.

Validation: Windows x64/LLE/JPN, optimized PPU, the four isolated routes in
`tests/pause_menu_runtime.py --case all --new-renderer` exercise Normal 1P,
Normal 2P with PT-BR and other mods, Normal 5P co-op and Battle 2 humans/3 CPUs.
They check Start acceptance, ignored A on confirmation, cancel, map return,
Draw/restart, arena return/reentry and native sound commands. Native tests
also check held Start, preserved heading/outside-panel map, unchanged CGRAM
and OBJ tiles, and restoration of VRAM/screen registers. Captures reviewed
for the compact panel in Normal and Battle remain local under `.bootstrap/`.
All four routes passed, as did the incremental build, seven native test
executables, six Python tests, tracked-source/package publication audits and
`git diff --check`. Full campaign/all arenas, physical controls and rewind
transitions remain unqualified; the earlier limits below still apply.

## Pause actions / Ações no menu de pausa — 2026-10-15

Optional, default-disabled `superbomberman5.pause-menu` 0.1.0 adds Resume and
Return to world map in Normal, and Resume / Force Draw / Return to stage select
in Battle. Main menus start on Resume; exit confirmations start on No and
ignore Start. An authored right-pointing glove and alphabet draw render-only
on BG3. Cursor/accept/back use native Battle setup SFX ids 1/4/3, observed as
cartridge-derived CPU-to-APU commands. The stock unpause beep is suppressed.

Normal uses native teardown and the Normal actor's direct map entry state 4,
without setting cleared-stage flags; the success completion permission bit is
cleared before teardown. Campaign characters, palettes, equipment and shared
lives survive the native menu's player reset, with benched co-op seats restored
before capture. Native saving can normalize a new file's count/checksum;
SRAM layout stays unchanged. Battle arena return uses `$35B2=1`, mode 4, as
`$C0:3D8E`; zero survivor statuses (including Revenge flags) take the original
Draw result and round restart. ROM patches are reserved/signature-checked and
restored on reset/disable; a constant-size validated save extension follows
co-op. See `docs/PAUSE_MENU.md` for hooks, controls and reproducible commands.

Evidence (Windows x64, LLE, JPN ROM, optimized PPU `NewRenderer=1`):
`tests/pause_menu_runtime.py --case all --new-renderer` passed Normal 1P
through frame 5,303, Normal 2P with PT-BR/characters/colors/Subordinate/
infinite time/co-op through frame 6,717 (script timeline 5,631 after private
picker restore), Normal 5P co-op through frame 5,415, and Battle 2 humans +
3 CPUs with PT-BR/colors/Subordinate through frame 9,268. Each checks pause,
Resume, No/cancel, B/back, Start ignored on confirmation, exit, reentry and
native APU commands for all three sounds; Normal also checks unchanged
WRAM/SRAM cleared-stage masks and retained characters/palettes/lives. Battle
reaches Draw Game, restarts the round, returns to the native interactive arena
selector `$C6:A28D` and starts another round. No `interp_cap`, unresolved
abandon, architectural BRK or fatal in these runs. Earlier accurate-PPU
checks also exercised Normal map return and Battle Draw/arena return.

An additional run of the final `build-lle` executable through frame 5,303
forced shared lives to 1 and non-default equipment bytes `$17CA=5` /
`$4EBD=4` before pausing; all three survived map return and stage reentry.
Evidence remains in `.bootstrap/pause-menu-validation/preservation-hkwyz4pa`.

Incremental Windows build and all seven native test executables pass. Native
tests cover held input, safe cursor defaults, guards, pending actions, campaign
restoration, benched seats, save/load/corrupt-state rejection, older states,
ROM/hook reset and VRAM/CGRAM/screen-register restoration. Six Python tests and
`git diff --check` pass. Tracked-source and data-only `.snesmod` publication
audits pass. The separate `--history` audit reports the pre-existing
`docs/images/five-player-showcase.png` media asset without provenance review;
no allowlist change, commit, version bump or release was made here.

Not qualified: full campaign, every arena, real boss transitions, Super/Tag
Battle, physical controller input, save/load or rewind across a live native
transition. Save/load state is covered by native tests, not an interactive
rewind qualification. The package cannot add this plugin to an older host.

O mod opcional inclui as opções solicitadas nos dois modos, com a mão,
confirmações inicialmente em Não e sons nativos. Voltar ao mapa não libera
fases nem repõe vidas; mantém personagens, cores e equipamentos. O Battle
usa o Draw original e a volta nativa às arenas. Build, sete testes nativos,
seis testes Python e as rotas Windows passaram, incluindo um teste adicional
com uma vida e equipamentos diferentes dos padrões. A auditoria do histórico
ainda aponta uma imagem preexistente sem revisão de proveniência; nada foi
publicado. Chefes reais, campanha completa, Super/Tag Battle, controles físicos
e rewind durante a transição continuam sem qualificação.


## v0.6.0 Subordinate Bomber playable / Subordinate Bomber jogável — 2026-10-14

Release scope: the optional **Subordinate Bomber — playable character** mod
(`superbomberman5.subordinate-bomber` 0.1.0, host v0.6.0+), default-disabled.
The Zone 3-D rival is cartridge character index 9: every per-character table
read through `$16C2` has a tenth entry, its sprite set is `$19`, palettes
`$48-$4D` are its six colors, `$4E` the shared effect palette and `$4F` the
rival palette (`$C4:3CCF` uses base + 7). In-memory patches: `$C6:8B20/$8B2E`
(the picker script's locked X cell becomes character 10 in the rival color; the
unlocked secret-character branch is untouched), `$C2:1276` → `$C2:6000` (no X
arrangement for it) and ten-entry HUD face tables at `$C2:6020/$6030`.
Render-only overlays: grid figure from its standing-frame tiles, authored seat
portraits (one shape, nine colors per selectable color) and HUD faces (alive
from the standing frame, defeated from an authored palette-indexed sad face).
Native portrait/HUD frames for index 9 are Bomberman's, and the object system
shares VRAM tiles between objects on the same frame, so overlays draw into
borrowed free OBJ tile blocks and repoint only their own sprites. Selection
colors (palettes up to `$4F`) and the Normal picker hand-off accept character 10.

Evidence (Linux LLE build of these sources, JPN ROM): six native test
executables pass (new `subordinate_tests`: patch bytes and restore, refusal on
unexpected bytes, grid figure, every portrait pixel and color set, shared tiles
with Bomberman, palette conflicts, HUD alive/defeated faces, exact
VRAM/OAM/CGRAM restoration). Scripted routes: Battle picker cycling the six
portraits with PT-BR; P1 Bomberman with P2-P5 on Subordinate Bomber in four
colors (picker and Battle HUD); defeated HUD faces for P3-P5; five-seat Battle
through the score board; Battle without the colors mod; Normal Zone 1-1 with
five players (Subordinate red as P1); Normal Zone 3-D next to the rival
Subordinate Bomber. Logs had no BRK, `interp_cap`, unresolved abandon or fatal.
Not qualified: Windows runtime (CI builds/tests only), physical controllers,
full campaign, a save with the secret X-cell character unlocked, Continue path.
The README showcase was recaptured for v0.6.0 (see `docs/images/README.md`).

O Subordinate Bomber pode ser escolhido no lugar do X, no Battle e no Normal,
com as seis cores do cartucho via L/R. Figura, retratos e rostos do HUD (vivo e
derrotado) acompanham a cor; tudo é desfeito ao desativar o mod.

## v0.5.0 five-player Normal Game demo / Demo com cinco jogadores — 2026-10-14

Release scope: the optional **Normal Game — five players** mod (package 0.1.0,
host v0.5.0+) joins the four existing mods in the Windows ZIP and Linux
AppImage. The launcher's PLAYER 1–5 tabs and seat 3–5 input come from the
framework forks (`MauricioSuporte/snesrecomp`, `MauricioSuporte/recomp-ui`,
branch `superbomberman5`) based on upstream `e54427b`/`41c88ff`. The README
gains the maintainer-requested showcase (`docs/images/five-player-showcase.png`:
five-player title menu, Zone 1-1 and Zone 1-A with distinct characters/colors,
five-player Battle Mode), captured from this project's Linux LLE build.

Evidence before tagging (Linux LLE, JPN ROM, local): native tests for all five
mods, Python publication tests, fixture runtime suite with both renderers, the
co-op scenarios in the sections below and the showcase routes. The Windows
package/build evidence comes from the Release workflow; Windows playtesting of
the final changes was done by the maintainer. Known open issue: Back (B) in the
Normal character picker can leave a black screen (pre-existing).

Escopo: mod opcional de cinco jogadores no Normal Game incluído nos pacotes
Windows e Linux, abas PLAYER 1–5 no launcher (forks dos frameworks) e imagem de
apresentação no README. Problema conhecido: voltar com B na seleção de
personagens do Normal pode deixar a tela preta.

## Five-player menu confirmation / Confirmação no menu de cinco jogadores — 2026-10-14

The overlay moved the title cursor only for its idle tile, so the confirmation
bounce (tiles `$1EA-$1EE`) jumped to the stock 2P row, and the fade (mode 3)
showed the stock two rows. The bounce now keeps its offset on the chosen row
and the five rows stay through the fade; menu input remains mode 2 only.
Evidence (Linux LLE): frame captures of 1P/EN, 3P/PT-BR and 5P/PT-BR
confirmations; native/overlay tests; fixture suite with both renderers.

O cursor faz a animação de confirmação ao lado da opção escolhida e as cinco
opções ficam na tela até o fade-out.

## P3-P5 enemy contact and spawns in smaller arenas / Dano por inimigos e spawns em arenas menores — 2026-10-14

Enemy contact used per-player distance arrays with two entries (`$1FF4-$1FFF`,
loops `CPY #$02` at `$C3:47FB` and four other routines), so P3-P5 never took
contact damage. Co-op moves them to five-entry arrays at `$1FC0-$1FDD` and
fills seats 3-5 via `$C3:FF70`. Spawns ran before `$C5:3D3F` built the stage
map, so P3/P4 appeared outside smaller arenas; seats are re-checked at
`$C5:3D5D`. Evidence (Linux LLE): an enemy pinned on P3 sets status `$41` then
`$C1` like P1; P1 control unchanged; Zone 2-4 (`$1F`) now places P3/P4 at
(5,11)/(9,11), `$6A` P3 at (3,9), `$6B` P3/P4 at (2,9)/(12,9); unit tests,
fixture suite, P3-P5 exit warp and boss continues pass with no BRK. Enemies
still choose P1/P2 as chase targets. Windows build and physical play pending.

Inimigos agora causam dano a P3-P5 ao encostar, e P3-P5 não nascem mais fora
de arenas menores (ZONE2-4, ZONE3-D, ZONE3-E). Os inimigos continuam
escolhendo P1/P2 para perseguir, como no original.

## Five-player stage exits, boss stages, menu and Continue picker / Saídas, chefes, menu e seleção no Continue — 2026-10-14

Exit warps tested only P1/P2: script op `$12` (`$C2:0C67`) loops `CPX #$02`
at `$C2:0C9F`; the same bound sits at `$C5:6195/$C5:61C6` (floor gimmicks),
`$C1:6BA1` (cell hold) and `$C0:373F` (point damage). All follow the roster.
Boss stages `$67-$6E` create rival bombers in actor slots 2/3, which replaced
P3 and pushed P5 to slot 5; five-entry bomber tables (`$0FEA/$0FEF`) then sent
`$C4:06C7` into data and the game froze. Rivals now take the slots after the
players (trampoline `$C2:4340` for `$C2:16B8/$C2:16EE`, 110 literal operands in
`$C5:2600-$C5:39FF`), boss rosters are `min(players, 5 - rivals)` and benched
seats keep character/items. The player menu uses native colors (heading color
3, rows color 2), follows the W34SEL blink and passes Up/Down to the guest for
its cursor sound. Continue -> File (`$C7:0BEA/$C7:0E2B`) opens the picker.

Evidence (Linux LLE, JPN ROM): five native tests and the fixture runtime
suite passed; P3-P5 warps from Zone 1-1 to 1-5 with every seat's bombs/fire
kept; Continue picker applied five characters; Continue into `ZONE1 - A`
(5P: four fight, rival in slot 4) and `$6A` (three fight, two rivals) ran
without the freeze, 2P/3P stock comparisons unchanged. Not qualified: boss
defeat and choreography, full campaign, Windows build, physical controls.
Picker Back (B) leaves a black screen in headless runs for New Game and
Continue, also with the previous version; unresolved.

Portais agora funcionam para P3-P5 e os itens de todos seguem para a fase
seguinte. Nas fases de chefe Bomber o rival fica depois dos jogadores; com
cinco slots, lutam até quatro (três na fase com dois rivais) e os demais
voltam na fase seguinte com personagem e itens. Menu com opções brancas,
PUSH START piscando e som do cursor. Continue -> File abre a seleção de
personagens. Voltar com B na seleção ainda deixa a tela preta nos testes.

## v0.4.4 Linux AppImage + Windows ZIP — 2026-10-14

Experimental Linux x86_64 ROM-independent setup AppImage added alongside the Windows x64 ZIP, retaining v0.4.3's four optional mods. Clean published framework pins are unchanged; local uncommitted co-op/control changes are excluded. AppRun copies setup to a versioned writable XDG data directory, preserves existing files and selects the locally generated build on repeat launch. Runtime/tool hashes and licenses are included; upstream game screenshots/fixtures are removed and both payloads audited.

Evidence: Windows and Ubuntu 22.04 CI setup-host/analyzer builds, four native tests per platform, publication history/package audits, eight Python tests (two Linux launcher tests skipped on Windows), two AppImage --help launches and synthetic analyzer smoke passed. Extracted Windows and Linux AppImage payloads generated real-ROM code and rebuilt locally using portable toolchain v1.0.14. Linux local environment: Ubuntu 24.04.4 via WSL; launcher initialized using SDL offscreen, with no visual QA or physical devices. Windows setup handed off to the locally rebuilt launcher. The four native tests passed on both local game builds. The unchanged first-stage route completed 3,628 LLE frames on each platform, with dummy video/audio and mods disabled, including map/stage entry, movement and bomb input. Logs had no interp_cap, unresolved-abandon, dispatch_abandon or fatal. The Linux route was invoked through the AppImage after local rebuild, confirming repeat launch uses the generated game. Linux FUSE mounting, full campaign, physical controllers/audio and broad distribution compatibility remain unqualified; extract-and-run was used. Windows gameplay qualification remains limited to the recorded routes. No ROM or generated code is published.

A v0.4.4 acrescenta AppImage Linux x86_64 experimental ao ZIP Windows e mantém os quatro mods da v0.4.3. Submódulos publicados preservados; mudanças locais do cooperativo não incluídas. Setup copiado para uma pasta XDG gravável, sem sobrescrever arquivos existentes; próximas aberturas usam o jogo gerado. Auditorias, licenças e hashes dos pacotes/ferramentas preservados.

Builds CI Windows/Ubuntu 22.04, quatro testes nativos por plataforma, auditorias e oito testes Python passaram (dois testes Linux pulados no Windows). Analisador validado com ROM sintética; AppImage abriu duas vezes. Pacotes extraídos geraram e compilaram o jogo localmente. No Ubuntu 24.04.4/WSL, o launcher inicializou com SDL offscreen; sem inspeção visual ou dispositivos físicos. O setup Windows encaminhou para o launcher recompilado. A rota de 3.628 frames LLE passou nas duas plataformas, com vídeo/áudio dummy e mods desligados, incluindo entrada na fase, movimento e bomba; logs sem os erros conhecidos. A rota Linux foi iniciada pelo AppImage após recompilar, validando a reabertura. Montagem FUSE, campanha completa, controles/áudio físicos e outras distribuições Linux não qualificados. ROM e código gerado permanecem locais.


## v0.4.3 infinite time / Tempo infinito — 2026-10-14

Release scope: optional, default-disabled infinite Normal timer, packaged with
PT-BR menus, character selection and L/R colors. Existing dependency pins stay
unchanged. Local five-player/control changes are not part of this release.
Native WRAM sentinel 99 seconds/9 minutes selects the stock `-:--` HUD and
skips timeout. Interpreter hooks at `$C5:0400/$C5:0460/$C5:0BEF` chain the
previous hook; reset restores it. No cartridge file, SRAM or boss flags change.
No extra gameplay state/save-state extension. These routines must remain interpreted.

Initial Windows/LLE Zone 1-1 evidence: native dash HUD inspected; 1P/2P
forced 0:01 expiration remains unlimited after 780 frames; disabled control
retains countdown and timeout. A two-player route combines PT-BR, characters
and colors. Full campaign, real bosses, rewind and physical controls remain
unqualified. The clean pinned-source setup pack passed publication audit. Its extracted
analyzer passed a synthetic-ROM test without Rust/toolchain PATH; the extracted
source generated five AOT units from the verified local ROM and rebuilt on
Windows/LLE. The root launcher logged GUI startup and delegation to the local
rebuilt host. Four native tests, six Python tests and dependency pin checks
passed. Extracted-package routes passed 1P (4,198 frames), 2P with PT-BR,
characters/colors (5,964 frames), plus a disabled-timeout control (3,598 frames).
No interp_cap, unresolved-abandon, architectural BRK, dispatch_abandon or fatal
was found. Code generation/rebuild was exercised by the shipped CLI pipeline;
the full interactive wizard was not exercised in this verification.

Escopo: tempo infinito opcional no Normal, junto dos três mods anteriores,
sem publicar as alterações locais de cinco jogadores/controles. O relógio usa
os valores nativos 99/9 e HUD `-:--`; não altera ROM em disco, SRAM ou flags de
boss. Evidência inicial Windows/LLE na Zone 1-1 confirma HUD, ausência de
expiração após forçar 0:01 e contagem/morte normais com o mod desligado.
Pacote limpo auditado; analisador extraído funcionou sem Rust/PATH do toolchain,
e as fontes extraídas geraram o código da ROM local e recompilaram no Windows.
Launcher registrou abertura da GUI e encaminhamento ao jogo recompilado.
Rotas do pacote passaram em 1P/2P e no controle com mod desligado, com logs
limpos. A geração usou o pipeline CLI incluído, sem percorrer todo o assistente
interativo. Combinação com PT-BR/personagens/cores em dois jogadores testada. Campanha
completa, bosses reais, rewind e controles físicos ainda não qualificados.

## Launcher and local controls for five players / Launcher e controles para cinco jogadores — 2026-10-14

The launcher showed only PLAYER 1/2 and the desktop host routed local input
only to seats 1–2, so P3–P5 could not be played outside the test fixture. At
the maintainer's request the pinned submodules were changed locally (pending
upstream): recomp-ui's SNES profile ceiling is now 8 seats (the title's
`num_players = 5` decides), keybinds.ini gains `[player3]`..`[player8]`, and
snesrecomp's desktop host reads `SourceP3`.., `EnableGamepad3`.. and
`[GamepadMap] ControlsP3`.., seats extra gamepads and pushes seats 3–5 through
`RtlSetPadState` each frame. Seats 3.. start with no device; two-player
titles keep their config.ini unchanged.

Evidence (Linux build, same sources): recomp-ui's 41 tests pass, with new
checks for five SNES controller cards and per-seat `ControlsP5`; snesrecomp
config round-trip and keybinds tests pass with new seat 3–5 cases; a virtual
gamepad set to P3 reached seat 3 as Start (`RtlGetPadState(2)=0008`) while
P1 stayed idle. Not built on Windows yet; physical controllers and the
interactive launcher page were not exercised here.

O launcher mostrava só PLAYER 1/2 e o host só lia controles locais para dois
jogadores. Com autorização do mantenedor, os submódulos foram alterados
localmente (falta enviar upstream): o launcher do SNES aceita até 8 jogadores
(o jogo declara 5), keybinds.ini ganhou `[player3]`.., e o host lê
`SourceP3`.., `EnableGamepad3`.. e `ControlsP3`.., enviando P3–P5 ao multitap.
Testes do recomp-ui e do snesrecomp passaram em build Linux; um gamepad
virtual em P3 chegou ao assento 3. Falta validar no Windows e com controles
físicos.

## Five-player HUD, animation and rejoin / HUD, animação e retorno com cinco jogadores — 2026-10-14

Interactive five-player play showed four regressions: the HUD frame and icons
were missing, characters did not animate, Start appeared not to revive a dead
player, and P3 drew as a cut sprite. Causes, verified on the JPN image with
scripted LLE runs: the adapter replaced the stock HUD selector `$C5:066C` with
`RTS`; five resident players filled the five campaign sprite pages, so the
stock allocator `$C5:6AF5` refused every animation change and a rejoined
player was created without graphics (the shared life was still spent); P3
requested OBJ palette slot 3, which Zone 1-1 enemies own. The HUD selector now
runs again with the mask restored at `$C5:060E`; an authored allocator at
`$C5:7100` refills the requester's own page when no page is free; seats 3–5
use palette slots 7, 6 and 5.

Evidence (Linux build of the same sources, LLE, scripted routes): Zone 1-1
with five players shows the full HUD; P1 OAM tiles cycle while walking; P3
draws with its own palette; a real bomb death of P1 followed by Start spends a
life and P1 reappears and walks (also with `NewRenderer=1`, characters, colors
and PT-BR enabled); real bomb deaths of P2/P3 rejoin through the fixture. The
four native tests and `tests/normal_coop_runtime.py --fixture` (counts 1–5,
five skins/rejoins/Continue, three-player lifecycle) pass. Not yet run on the
Windows toolchain; bosses, mounts and later zones remain unqualified, and
mount/item palettes may fall back to another palette with five players.

A sessão com cinco jogadores mostrou HUD sem moldura/ícones, personagens sem
animação, Start sem reviver (a vida era gasta, mas o jogador voltava sem
gráficos) e P3 cortado. O seletor de HUD original voltou a rodar; um alocador
próprio reaproveita a página de sprites do jogador quando não há página livre;
P3–P5 usam as paletas 7, 6 e 5. Rotas roteirizadas na Zone 1-1 confirmam HUD,
animação, retorno com Start e paleta de P3; os testes nativos e de execução
passaram em build Linux. Falta validar no Windows, em bosses, montarias e
zonas seguintes.

## Normal menu recognition / Identificação do menu Normal — 2026-10-14

An interactive launcher session reproduced the original two-row menu while
the co-op feature was enabled. Local diagnostics recorded title mode with
`$2000` retaining `$C7:1B3B` from attract mode, instead of the fast scripted
route's `$C6:F717`. The feature now recognizes both native BG3 player rows
before overlays are applied and caches that context for the frame. Unit
checks cover stale return history and rejection of unrelated text. The local
executable is rebuilt; the accurate menu compositor remains scoped to this
screen. This does not qualify the rest of the campaign or physical controllers.

Post-fix Windows/LLE scripted runs with `NewRenderer=1` passed counts 1–5,
initial lives 3–7 and safe Zone 1-1 spawns; PT-BR was enabled for counts 2 and
4. Both player-menu languages were visually inspected. Four native tests,
six Python tests, publication history audit and whitespace checks passed.

A abertura interativa pelo launcher reproduziu as duas opções originais com
o mod ativado. O diagnóstico local registrou o modo de título com `$2000`
conservando `$C7:1B3B` da demonstração, em vez de `$C6:F717` da rota rápida de
teste. O mod agora reconhece as duas linhas originais em BG3 antes das
substituições visuais e mantém esse contexto durante o quadro. Testes nativos
cobrem o retorno antigo e rejeitam textos de outras telas. O executável local
foi recompilado. Campanha completa e controles físicos continuam não qualificados.

Após a correção, execuções Windows/LLE com `NewRenderer=1` passaram para 1–5
jogadores, vidas iniciais 3–7 e spawns seguros na Zone 1-1, com PT-BR nos casos
de dois e quatro jogadores. Os menus nos dois idiomas foram inspecionados.
Passaram os quatro testes nativos, seis testes Python, auditoria e verificação
de espaços em branco.

## Source Normal co-op / Cooperativo Normal — 2026-10-14

Current source adds the optional, default-disabled `superbomberman5.normal-coop`
v0.1.0. The 1–5 player menu works in English/PT-BR; initial shared lives are
3–7. Additional actors use safe-floor searches and boss offsets. The native
living check, Start rejoin and participating roster cover five seats. Normal
character/color selection waits for every chosen seat and preserves choices
through Continue. Dead Start is withheld from the stock pause task until
release. Mount backup slots are separated from P3/P4; NPC capacity is fourteen.

Source build and focused state/collision/rendering/selection tests pass.
Scripted Windows/LLE checks cover 1–5 players in Zone 1-1 and P3 as the sole
survivor, P1 rejoin without pause, Game Over and Continue. Extra-pad scenarios
exercise five independent character/color choices, each Start rejoin and
last-life arbitration through the real multitap path in an opt-in fixture.
The fixture is excluded from the normal build. Full campaign, real bosses,
mount/item combinations, dense NPC stages, endings, populated battery saves
and five physical controllers remain unqualified. No release was published.
See [NORMAL_COOP.md](NORMAL_COOP.md) for scope and reproducible checks.

Os fontes incluem o mod opcional para 1–5 jogadores no Normal, com menu
PT-BR, vidas iniciais 3–7, spawns seguros, retorno com Start e seleção de skins
e cores para todos. Os testes roteirizados abrangem a primeira fase, P3 como
único sobrevivente, retorno sem pausa e Continue. Há testes específicos das
posições de boss e de restauração de snapshots. Campanha completa, bosses reais,
montarias/itens, alta ocupação de NPCs e controles físicos ainda precisam de
validação. A build normal não inclui as caixas de entrada da fixture de teste.

Base criada pelo scaffolder oficial do SNESRecomp em 2026-10-13.

## ROM de referência

- Arquivo local: `Super Bomberman 5 (USA).sfc`.
- Cabeçalho: `Hu SUPER BOMBERMAN 5`, região JPN. O nome do arquivo não determina a região.
- Fast HiROM, 4 MiB, SRAM de 8 KiB, sem coprocessador especial detectado.
- Checksum interno válido; isso não comprova sozinho que a imagem é um dump original sem modificações.
- CRC32: `c25ab56a`.
- SHA-256: `5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4`.
- Identidade usada pela geração e pelo executável: `rom_identity.txt`.

## Base disponível

v0.4.2 consolida a publicação em um único ZIP Windows, com executável de
instalação, recursos do launcher, analisador, fontes e os três mods em
`mods/preloaded/packages`. Não há downloads de mods separados nessa release.
`FILE_SHA256SUMS` lista os hashes de todos os arquivos dentro do pacote; o
teste de integridade cobre conteúdo e caminhos relativos. A referência
MegaManXSNESRecomp v2.2.2-alpha foi conferida: seu ZIP Windows reúne
executável, recursos e catálogos de mods, sem compilador portátil embutido.
O Bomberman mantém a geração/rebuild local necessária ao seu setup host.

v0.4.1 atualiza o ícone Windows fornecido pelo mantenedor: dez tamanhos de
16 a 256 pixels, com escala proporcional e transparência, incorporados por
recurso RC no setup host e no jogo gerado localmente. O ícone foi extraído do
executável local para conferência visual. A categoria do mod PT-BR passa a
`Translation`; seu nome e descrições continuam em português. O README reúne
os três mods opcionais, todos v0.1.0, com ativação, controles e requisitos de
host em inglês e português. Essas mudanças não ampliam a validação de gameplay.

Correção antes da publicação v0.4.0: `$1038` é um contador de escolhas
confirmadas, não uma máscara. A tabela `$62B5` indica cursors ativos/confirmados
e `$62AB` associa cada controle ao cursor atual: humanos usam seus controles e,
ao confirmar, passam a escolher CPUs pendentes. L/R acompanha essas tabelas
e preserva os confirmados; Normal de dois jogadores aguarda contador 2.

Prévias de um mesmo personagem podem compartilhar uma paleta no alocador
original. O mod separa as cinco referências dos retratos em slots próprios,
preservando os demais donos/refcounts. A cor da grade acompanha o cursor ativo;
CPUs pendentes não sobrescrevem a prévia do personagem em escolha.

Na saída do seletor, a limpeza original pode restaurar cores padrão dos CPUs.
O mod preserva as escolhas até o inicializador nativo consumir os offsets e
derivar as paletas das fases; snapshots incluem essa transição. Rota de 7.360
frames com dois humanos e três CPUs validou entrada independente, transferência
de controle e as cinco paletas confirmadas na fase do Battle. A inicialização
do seletor reutiliza temporariamente `$1038`; a confirmação do Normal só passa
a ser observada depois que o seletor fica pronto em `$C6:8E68`. Rotas locais
de Normal para um e dois jogadores validaram abertura, confirmação separada e
paletas na Zone 1-1 (segundo personagem verde em 1P; branco/preto e segundo
personagem vermelho em 2P). Campanha e gameplay cooperativo completos continuam
pendentes de validação.

v0.4.0 inclui o mod de cores na seleção: L/R percorrem branco, preto, vermelho, azul,
verde e amarelo nos nove personagens. Cursor com L + R acima e jogador abaixo.
Testes nativos passaram para paletas, entradas, confirmação, snapshots e
restauração de OAM/VRAM; rotas locais com PT-BR validaram as seis cores e o
segundo personagem verde em fases do Normal e Battle. Multiplayer completo
continua pendente. Requer o host atualizado v0.4.0. Detalhes em [SELECTION_COLORS.md](SELECTION_COLORS.md).

Repositório Git local com SNESRecomp e recomp-ui como submódulos fixados em commits, CMake, configuração inicial de análise, launcher, scripts de geração e empacotamento e workflow oficial. A inicialização do jogo ativa o multitap na porta 2 para cinco assentos; multiplayer ainda não foi validado. Netplay está desabilitado.

Ferramentas locais preparadas em `.tools/`: RetroPorting Toolchains v1.0.14 (CMake, Ninja, Clang e SDL3) e Rust 1.97.1 Windows GNU LLVM. Scripts reproduzíveis: `tools/setup-windows.ps1` e `tools/build-windows.ps1`. Instruções em `BUILD_WINDOWS.md`.

Fontes locais gerados com sucesso e executável Release compilado em `build-lle/SuperBomberman5SNESRecomp.exe`, com SDL3 estático e composição de frames LLE. `play.cmd` abre o launcher.

A ROM, fontes gerados, ferramentas locais, builds, saves, dumps e capturas são ignorados pelo Git e sujeitos à auditoria de publicação.

Repositório público: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp. Formato da publicação inicial v0.1.0: pré-release Windows x64 com setup host compilado sem dados da ROM, fontes, analisador nativo independente da ROM e sua DLL/licença. O jogo completo é gerado no computador do jogador. Não distribuímos o executável local `build-lle/`, que contém código derivado da ROM.

README em inglês/português, CONTRIBUTING.md, THIRD_PARTY_NOTICES.md e docs/PUBLISHING.md descrevem instalação, limites, licenças e fluxo de versões. A skill `$super-bomberman5-repository` está instalada localmente e versionada em `.agents/skills/`. Source checks audita o histórico e executa os testes de publicação; Release gera pacotes Windows manualmente, com ferramentas fixadas em v1.0.14.

## Publicação v0.1.0

Pré-release publicada em 2026-10-13: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.1.0.

- Tag anotada `v0.1.0`: commit `6ba1ee0ffea2560d18f65518ddf0ba72cfaf72a1`.
- ZIP Windows x64: `superbomberman5-0.1.0-windows-x64.zip`, 48.814.122 bytes.
- SHA-256: `4656188e1d4176798ef08a73ece8a32f5ab0dc9abdabc9814c79a6c8b26802d1`, acompanhado de `SHA256SUMS`.
- Build Release no GitHub Actions passou: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/actions/runs/38475851829.
- Arquivo final auditado sem ROM, código C gerado, executável derivado da ROM, capturas de jogos das dependências ou fixtures de ROM. Avisos de licença do framework, launcher, Cargo, Rust e LLVM foram preservados.
- ZIP extraído: launcher abriu; analisador executou um programa RTS sintético sem Rust nem PATH das ferramentas; geração com a ROM local, rebuild e execução de 600 frames foram verificados. A build gerada manteve `SNESRECOMP_FRAME_IMPL=LLE`.
- GitHub confirmou pré-release pública com dois assets, tamanhos e digests iguais aos arquivos locais. O executável jogável local e suas capturas não foram enviados.

A validação usa as rotinas de geração/rebuild do pacote e smoke tests do launcher. A sequência completa de cliques no assistente, novas instalações em outros computadores e o download automático de ferramentas ainda precisam de testes adicionais.

## Validação realizada

Probe de cabeçalho e checksum, verificação de CRC32/SHA-256 pelo CLI e conferência dos submódulos após mover o scaffold para a pasta de trabalho.

Teste de 600 frames sem entrada e testes de navegação com entradas roteirizadas. Capturas locais mostram a abertura animada, tela de título, menus de modo normal, seleção de arquivo e mapa da Zone 1. A rota de 3025 frames até o mapa terminou com código zero, sem mensagens `interp_cap`, `unresolved-abandon` ou BRK. Evidências brutas estão em `.bootstrap/validation/`, ignoradas pelo Git. As primeiras capturas usam SDL vídeo/áudio dummy: não comprovam a qualidade sonora.

O script de setup também foi executado novamente para verificar o reaproveitamento das ferramentas, e o script de build foi validado pelo parser do PowerShell e executado na geração e compilação.

A rota final `validation/first-stage.input` chegou à primeira fase da Zone 1 e terminou após 3628 frames com código zero. As capturas mostram cenário, jogador e cronômetro avançando. Sem mensagens `interp_cap`, `unresolved-abandon`, BRK, erro ou fatal nessa execução. Não há comparação independente de colisões, bombas ou precisão de timing ainda.

Um smoke test adicional de 180 frames abriu a janela SDL real do Windows e o dispositivo de áudio real, com callbacks de áudio e saída normal. O launcher ImGui/SDL3 também abriu e encerrou normalmente pelo mecanismo de smoke test de 60 frames. Isso comprova inicialização dos dispositivos, não fidelidade ou qualidade sonora auditiva.

## Correção de encoding e primeiro mod PT-BR

O corpo da release v0.1.0 apresentou caracteres de substituição ao passar pela saída do console do Windows. Foi corrigido via API com bytes UTF-8 explícitos, comparando o texto remoto com o original e verificando a ausência de U+FFFD. `tools/release-notes.py --output <arquivo>` agora grava UTF-8 diretamente; `--version 0.1.0` permite corrigir notas históricas sem mudar a versão do código.

O código atual prepara a versão **0.2.0**, com o primeiro mod `superbomberman5.ptbr` v0.1.0. São sete rótulos de título e modo normal, fonte própria com Ç/Õ, feature desligada por padrão, ativação na aba Mods e `.snesmod` somente de dados. A antiga release v0.1.0 não tem o plugin nativo.

Capturas locais mostram os rótulos traduzidos. O teste nativo de restauração verificou desenho alterado durante o frame, guarda de superfície, restauração integral da VRAM e caminho desativado. Uma rota com o mod ativo terminou em 3628 frames e entrou na primeira fase, sem erros de dispatch/BRK. Fonte e código autorais são públicos; ROM, tiles extraídos e capturas ficam locais. Escopo e instruções: `docs/TRANSLATION_PTBR.md`.

## Ampliação e correções da tradução PT-BR

A prévia de menus agora tem 62 rótulos de background e um cabeçalho de sprites: título, 1/2 jogadores, modo normal, arquivos, opções/senha/teste de som, tipos de batalha, participantes, personagens, regras e cabeçalho de fases. Inclui acentos, algarismos, sombra, traços mais espessos no título e cabeçalhos azuis. Incluída como prévia no host 0.2.0; a release/tag 0.1.0 não foi substituída.

O corte de JOGO NORMAL ocorria porque a nova fonte ocupava a janela HDMA usada pelo jogo para ocultar Start. O título respeita agora a margem superior do original; o aviso ocupa suas duas linhas de tiles. Capturas consecutivas de 1215 a 1551 verificaram Start aceso/apagado, sem cortar JOGO NORMAL. A paleta de textos longos agora vem do início do rótulo, inclusive sobre espaços antigos; slots de tiles são estáveis por tela, separados dos gráficos em uso.

Telas de jogadores e regras reutilizam IDs de tiles com conteúdos diferentes. Guardas de contexto distinguem as telas e são avaliadas antes de desenhar qualquer rótulo. O teste nativo cobre restauração integral da VRAM, ativação/desativação, alinhamento, slots, paletas, contexto compartilhado e cabeçalho de sprites sem alterar OAM. O build Windows LLE e os testes de publicação/UTF-8 passaram. Capturas e rotas de navegação ficam exclusivamente em `.bootstrap/ptbr-*/`; configurações do usuário foram preservadas, com testes do mod em cópias isoladas.

Nomes e descrições das arenas, confirmações de arquivos e telas não verificadas de equipes/Bombermania ainda podem permanecer originais. Navegação em preparação de batalha não qualifica a precisão de partidas nem P2–P5. Escopo detalhado: `docs/TRANSLATION_PTBR.md`.

## Cobertura nativa e limitação atual

A configuração conservadora gera cinco variantes AOT elegíveis e quatro variantes LLE, em nove variantes exatas. A maior parte da lógica do jogo ainda executa no interpretador. Isso é uma primeira integração compilada, não uma recompilação nativa completa do jogo.

Uma tentativa de promover reset e NMI a AOT descobriu 69 variantes elegíveis, mas falhou no teste: limite de interpretação em `$C02346`/`$C00607`, abandono de dispatch no frame 56 e BRK posteriores. Essa promoção foi retirada. `recomp/symbols.toml` mantém reset e NMI com `emit = false` até corrigir e validar a continuidade do código compilado/interpretado.

## Próximas etapas

1. Corrigir e validar a continuidade AOT/LLE antes de promover as rotinas de reset/NMI novamente.
2. Ampliar a cobertura com rotinas curtas comprovadas e perfis de execução vinculados ao hash desta ROM.
3. Comparar frames, WRAM, interrupções e áudio com uma referência independente (bsnes/Snes9x/Mesen).
4. Validar fases, bombas, colisões, bosses, saves e multiplayer local, incluindo entradas de P3–P5 no host desktop.
5. Validar configurações e fluxo completo do launcher, empacotamento e workflow antes da publicação no GitHub.

## Release v0.2.0

Pré-release Windows x64 com o mod PT-BR v0.1.0 incluído no catálogo Mods, mais arquivo `.snesmod` separado. Publicação usa um checkout limpo e setup host LLE, sem código gerado/ROM. SHA256SUMS cobre o ZIP e o mod. A versão 0.1.0 permanece preservada. Tradução e precisão do jogo continuam parciais.
## Release v0.3.0 — Normal Game character selection

The v0.3.0 setup pack includes the optional, default-disabled
`superbomberman5.normal-characters` v0.1.0 package and trusted plugin. Confirming
a Normal file opens the cartridge's Battle character picker, then restores the
suspended campaign and applies the character/palette before stage graphics
load. The second and ninth characters were observed in Zone 1-1; the latter
was exercised with movement/bomb input and the PT-BR overlay enabled. Cancel
returns to file selection. Source build and focused selection/state tests pass.
Full campaign and cooperative stage behavior are not qualified. This Windows prerelease includes a ROM-free setup host and two data-only mod archives. See [NORMAL_CHARACTERS.md](NORMAL_CHARACTERS.md).

O setup pack v0.3.0 inclui o mod opcional, desabilitado por padrão, de seleção de
personagens no Normal Game. A seleção original do Battle abre depois da
confirmação do arquivo e a escolha é aplicada antes de carregar os sprites da
fase. O segundo e o nono personagens foram observados na Zone 1-1; o nono foi
testado com movimento/bomba e tradução PT-BR. B volta aos arquivos. Compilação
e testes de seleção/estado passaram. Campanha completa e fases cooperativas
ainda não foram qualificadas. Pré-release Windows com setup host sem ROM e dois pacotes de mods.

## README v0.13.1 Normal frame — 2026-10-16

Replaced only the bottom-left montage panel with Windows LLE runtime frame 4100. Native picker choices are Subordinate/Honey/Jet/Bazooka/Terrorin, colors green/native/black/red/native. Native egg pickup produced Black Louie (type 6); staging sets native down-facing animation after repositioning. Simultaneous Honey, Jet, Bazooka and Terrorin abilities and the P1/P2-only Normal HUD are visible. Other three panels compared pixel-identical. Poses/positions/protection/corridors were scripted for composition; no artwork was generated or extracted for source. See docs/images/README.md.

Somente o quadro inferior esquerdo foi atualizado: captura real Windows LLE com cinco personagens, habilidades, Subordinado verde no Black Louie e nova HUD. Os outros três quadros são idênticos pixel a pixel; a composição usa posições e poses preparadas por script.
