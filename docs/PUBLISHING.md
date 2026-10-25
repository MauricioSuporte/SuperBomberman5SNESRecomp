# Versions and publication / Versões e publicação

Public repository: `MauricioSuporte/SuperBomberman5SNESRecomp`. VERSION is **2.2.9**. Since v1.0.1 releases are full (non-prerelease) releases, marked latest: the maintainer completed the campaign on Windows. Keep describing the partial recompilation and the unqualified areas. Preserve existing published tags and assets.

## Release contents

Publish **superbomberman5-2.2.9-windows-x64.zip**, **superbomberman5-2.2.9-linux-x86_64.AppImage** and **SHA256SUMS**. Both are ROM-independent setup hosts with source, native analyzer, launcher resources, licenses, the updater (`updater/SuperBomberman5Updater[.exe]`, built from `src/updater/` and added by `tools/finalize-release.py --updater` / `tools/package-appimage.py`) and fourteen optional, default-disabled mods: PT-BR menus and completion screens, Normal character selection, Normal HUD faces, L/R colors, infinite Normal timer and infinite lives (Cheats group), five-player Normal Game, its optional corner-spawn layout, Super Bomberman 5 Extra Characters (Honey, Kotetsu, Subordinate Bomber and Emperor Terrorin; replaces the separate Honey and Kotetsu and Subordinate Bomber packages since v0.13.0), Pause — exit & draw, Normal portal destinations, Gold Battle, Louie improvements and Bomber Kings. Extra Characters renders Terrorin's barrier sound at runtime from the player's own SB5 ROM in a private APU; no audio is stored. Black Louie uses native cartridge sound effects; no external WAVs are included. Former audio hashes are grandfathered for auditing historical commits only, never current source or new packages. Bomber Kings reads Super Bomberman 4 graphics, palettes and sound driver at runtime from the player's own SB4 ROM (selected on the Mods page, SHA-256 checked); no SB4 data is in the repository or the packages — audit for it. The playable executable is generated only on the player's machine. Linux is experimental; see STATUS.md for exact evidence.

Build from a separate clean checkout with pinned submodules, no src/gen/*.c or recomp/funcs.h, `SNESRECOMP_SETUP_HOST=ON`, `SNESRECOMP_FRAME_IMPL=LLE` and the test input fixture OFF. Never package the development game executable or local ROM/save/capture data.

## Before every publication

Update README.md in **both English and Portuguese** with useful current download links/formats, installation or upgrade steps, features/mods, requirements and validation limits. Review obsolete version/platform claims. Update VERSION, STATUS.md, these instructions and tools/release-notes.py to match the final artifacts. Keep dependency pins unchanged unless intentionally reviewed.

## Forked framework submodules

Since v0.5.0 `snesrecomp` and `recomp-ui` point to the maintainer's forks
(`MauricioSuporte/snesrecomp`, `MauricioSuporte/recomp-ui`), branch
`superbomberman5-v1.2` since v2.0.0 (rebased onto Mega Man X Recompiled's pins),
which carry the host rebuild fixes, the rollback exec-state of the beam driver,
the netplay test-input knobs and the launcher's `game_links`/`game_action` on
top of the upstream commits recorded in docs/STATUS.md. Commit framework changes in
the fork first, push that branch, then update the gitlink and run
`bash snesrecomp/tools/ci/record_pins.sh framework_pins.txt`. Never publish a
gitlink that exists only locally: CI checks out submodules recursively. Offer
the same changes upstream as pull requests and return to the upstream URLs
once merged.

The README screenshots `docs/images/five-player-showcase.png` and `portal-destinations.png` are the tracked
screenshots (maintainer-requested exception, provenance in docs/images/README.md).
Release packagers strip `docs/images/` and the ZIP audit rejects it.

```powershell
python tools/audit-publication.py --history
python -m unittest discover -s tests -v
git diff --check
```

Run all sixteen native tests in the release source (`ptbr_render_tests`, `normal_character_tests`, `normal_hud_tests`, `selection_color_tests`, `infinite_time_tests`, `infinite_lives_tests`, `updater_tests`, `normal_coop_tests`, `subordinate_tests`, `pause_menu_tests`, `portal_labels_tests`, `extra_characters_tests`, `special_abilities_tests`, `sb4_rom_tests`, `gold_battle_tests`, `louie_tests`). Test package extraction, the native analyzer, first/repeat launcher startup and local ROM generation/rebuild. Exercise a real-ROM route locally and record its platform/frames and limitations in STATUS.md. No ROM enters CI or public artifacts.

## Build and package

The manual **Release** workflow builds on Windows and Ubuntu 22.04 using portable toolchain v1.0.14, Rust 1.97.1 for the analyzer and Python 3.12 for Linux checks. Both platform jobs must pass before the publish job runs. Dispatch with the version matching VERSION; `publish_release=false` produces test artifacts, `true` publishes after the same checks. Releases are published as full releases (latest) and existing assets cannot be overwritten.

The Windows packager remains scripts/package_release.sh, followed by tools/finalize-release.py with the analyzer, libunwind DLL, toolchain and Rust license paths. The Linux job first builds the same ROM-independent setup ZIP, then invokes:

```bash
python3 tools/package-appimage.py \
  --zip dist/superbomberman5-2.0.0-linux-x64.zip \
  --build-dir build-ci \
  --analyzer snesrecomp/recompiler-rs/target/release/snesrecomp-analyze \
  --rust-sysroot "$(rustup run 1.97.1 rustc --print sysroot)" \
  --appimagetool /path/to/appimagetool --runtime /path/to/runtime-x86_64
```

The workflow verifies pinned SHA-256 hashes for appimagetool and the type2 runtime. Updating these pins requires reviewing their source/notice changes. The Linux finalizer removes upstream game screenshots/test fixtures, adds Cargo/Rust notices and the native analyzer, audits the ZIP and wraps it in AppRun plus an authored SVG icon. The intermediate Linux ZIP is a CI artifact; the public Linux download is the AppImage. Preserve SOURCE_REVISIONS, PACKAGE_MANIFEST.json, FILE_SHA256SUMS and notices.

AppRun installs a versioned writable copy under `${XDG_DATA_HOME:-$HOME/.local/share}/superbomberman5/<version>`. It never overwrites an existing generated build. Subsequent launches use `build/SuperBomberman5SNESRecomp` when present. Test `--appimage-extract-and-run` for systems without FUSE. Linux requires x86_64, glibc 2.35+ and desktop graphics/audio libraries; Windows requires Windows 10+ x64. Both need several GB free and internet for the first portable tools download. Rust is not required by players.

## Publish and verify

Generate bilingual notes with `python tools/release-notes.py --output .bootstrap/notes.md`; read/send UTF-8 bytes, then verify the remote body exactly matches and has no U+FFFD. Stage explicit paths and preserve the user's working tree. Push reviewed commits, create a new version tag at the release commit and publish only audited assets. Never move published tags or overwrite assets without explicit authorization.

Verify the default branch, tag target, successful workflow, release state (not prerelease, latest), UTF-8 notes, asset sizes and SHA-256 digests. SHA256SUMS covers both public packages; FILE_SHA256SUMS covers the setup payload. GitHub also exposes each uploaded asset digest. Check remote state before retrying an API failure to avoid duplicates.

The Release workflow then announces the release in the Discord #patch-notes channel (`tools/discord-release-post.py`, link + only this version's changes from `tools/release-notes.py --whats-new`, English/Português embeds). The webhook URL is only in the Actions secret `DISCORD_PATCH_NOTES_WEBHOOK`; never commit or print it. Without the secret the step is skipped.

## Português

Toda nova release exige README.md atualizado nas duas línguas, com informações úteis: downloads, formatos, instalação/atualização, recursos, requisitos e limites reais de validação. Atualize VERSION, STATUS.md, este documento e o gerador de notas.

A v2.0.0 distribui **ZIP Windows x64**, **AppImage Linux x86_64 experimental** e **SHA256SUMS**. Ambos incluem setup sem ROM, fontes, analisador, licenças e o atualizador em `updater/` e os quatorze mods opcionais desligados por padrão (incluindo Super Bomberman 5 Extra Characters, que substitui Honey and Kotetsu e Subordinate Bomber, e Bomber Kings, que lê o Super Bomberman 4 da ROM do próprio jogador; nada do SB4 entra nos pacotes). O jogo é gerado apenas na máquina do jogador. Use checkout limpo, LLE e setup host; nunca publique o executável de desenvolvimento, ROM, código gerado, saves ou capturas.

A workflow Release é manual e só publica quando os dois builds e verificações passam. Valide os pacotes extraídos, analisador, primeira abertura/reabertura e geração/rebuild local. O AppImage copia o setup para a pasta XDG gravável da versão e reutiliza o jogo gerado; sem FUSE, use --appimage-extract-and-run. Registre plataforma, rota/frames e limitações no STATUS.md. Preserve fontes, licenças, revisões e checksums. Não reescreva tags ou assets públicos automaticamente. Confirme notas UTF-8, estado de release completa (latest) e hashes dos arquivos no GitHub.

A skill instalada é `$super-bomberman5-repository`, com cópia versionada em `.agents/skills/super-bomberman5-repository/`.

Gameplay screenshots under `docs/images/` are authorized for README/docs/releases. Screenshots are optional; no per-mod image is required. This does not allow extracted sprites/tiles/audio or raw dumps.

Capturas são opcionais; não é exigida uma imagem por mod.

Windows v0.13.1 includes SuperBomberman5Recomp.exe, a ROM-independent entry with the existing “5” icon. It locates the sibling setup/game executable or the source tree build-lle executable, forwarding arguments and exit status. It replaced play.cmd, removed after v0.13.1. Since v1.0.1 the audited Windows ZIP has a single root executable: tools/finalize-release.py ships the setup host itself as SuperBomberman5Recomp.exe (built with SNESRECOMP_GAME_TARGET, so Generate & rebuild still targets and relaunches build/SuperBomberman5SNESRecomp.exe). The forwarding entry remains for source trees.

O Windows v0.13.1 inclui SuperBomberman5Recomp.exe com o ícone atual do “5”. A entrada localiza o setup/jogo ao lado ou o build local em build-lle e repassa argumentos e código de saída. Ele substituiu o play.cmd, removido depois da v0.13.1. Desde a v1.0.1 o ZIP Windows tem um único executável na raiz: o próprio setup, publicado como SuperBomberman5Recomp.exe; o Generate & rebuild continua gerando e abrindo build/SuperBomberman5SNESRecomp.exe.


## Netplay, Discord, updater and Quiver (v2.0.0)

Since v2.1.0 the snesrecomp fork carries the in-game room chat and name-tag overlay
(`runner/src/desktop/host_chat.c`) and the recomp-net fork the spectator-BYE fix; push recomp-net,
then snesrecomp, then the game gitlink.

Since v2.0.4 the snesrecomp fork also points `lib/recomp-net` at the fork `MauricioSuporte/recomp-net` (branch
`superbomberman5`: 64 KiB lobby frames). Push recomp-net fork commits before the snesrecomp gitlink.

The game target links recomp-net with ICE (`SNESRECOMP_NET_ICE`); configure needs network for libjuice
(FetchContent) unless `RNET_LIBJUICE_ROOT` points to a local copy. Lobby matching uses the stamped build
version, so every release must keep `VERSION` = tag. The updater reads `releases/latest` and needs the
asset names `superbomberman5-<v>-windows-x64.zip`, `superbomberman5-<v>-linux-x86_64.AppImage` and
`SHA256SUMS` (covering both). Quiver relies on the same names and on a single root executable; keep the
updater in `updater/`. See NETPLAY.md, UPDATER.md and QUIVER.md.

Netplay, Discord, atualizador e Quiver dependem dos mesmos nomes de assets, do `SHA256SUMS` cobrindo os dois
pacotes, de `VERSION` igual à tag e de um único executável na raiz do ZIP.

Cada release publicada é anunciada no canal #patch-notes do Discord pelo último passo do workflow
(`tools/discord-release-post.py`). A URL do webhook fica só no secret `DISCORD_PATCH_NOTES_WEBHOOK` do
Actions; nunca entra no Git nem em logs.
