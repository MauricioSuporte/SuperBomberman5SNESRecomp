# Versions and publication / Versões e publicação

Public repository: `MauricioSuporte/SuperBomberman5SNESRecomp`. VERSION is **0.5.0**. Publish prereleases while full-game accuracy remains unqualified. Preserve existing published tags and assets.

## Release contents

Publish **superbomberman5-0.5.0-windows-x64.zip**, **superbomberman5-0.5.0-linux-x86_64.AppImage** and **SHA256SUMS**. Both are ROM-independent setup hosts with source, native analyzer, launcher resources, licenses and five optional, default-disabled mods: PT-BR menus, Normal character selection, L/R colors, infinite Normal timer and five-player Normal Game. The playable executable is generated only on the player's machine. Linux is experimental; see STATUS.md for exact evidence.

Build from a separate clean checkout with pinned submodules, no src/gen/*.c or recomp/funcs.h, `SNESRECOMP_SETUP_HOST=ON`, `SNESRECOMP_FRAME_IMPL=LLE` and the test input fixture OFF. Never package the development game executable or local ROM/save/capture data.

## Before every publication

Update README.md in **both English and Portuguese** with useful current download links/formats, installation or upgrade steps, features/mods, requirements and validation limits. Review obsolete version/platform claims. Update VERSION, STATUS.md, these instructions and tools/release-notes.py to match the final artifacts. Keep dependency pins unchanged unless intentionally reviewed.

## Forked framework submodules

Since v0.5.0 `snesrecomp` and `recomp-ui` point to the maintainer's forks
(`MauricioSuporte/snesrecomp`, `MauricioSuporte/recomp-ui`), branch
`superbomberman5`, which carry the five-seat launcher/host input changes on top
of the upstream commits recorded in docs/STATUS.md. Commit framework changes in
the fork first, push that branch, then update the gitlink and run
`bash snesrecomp/tools/ci/record_pins.sh framework_pins.txt`. Never publish a
gitlink that exists only locally: CI checks out submodules recursively. Offer
the same changes upstream as pull requests and return to the upstream URLs
once merged.

The README showcase `docs/images/five-player-showcase.png` is the only tracked
screenshot (maintainer-requested exception, provenance in docs/images/README.md).
Release packagers strip `docs/images/` and the ZIP audit rejects it.

```powershell
python tools/audit-publication.py --history
python -m unittest discover -s tests -v
git diff --check
```

Run all five native tests in the release source (`ptbr_render_tests`, `normal_character_tests`, `selection_color_tests`, `infinite_time_tests`, `normal_coop_tests`). Test package extraction, the native analyzer, first/repeat launcher startup and local ROM generation/rebuild. Exercise a real-ROM route locally and record its platform/frames and limitations in STATUS.md. No ROM enters CI or public artifacts.

## Build and package

The manual **Release** workflow builds on Windows and Ubuntu 22.04 using portable toolchain v1.0.14, Rust 1.97.1 for the analyzer and Python 3.12 for Linux checks. Both platform jobs must pass before the publish job runs. Dispatch with the version matching VERSION; `publish_release=false` produces test artifacts, `true` publishes after the same checks. Releases are prereleases and existing assets cannot be overwritten.

The Windows packager remains scripts/package_release.sh, followed by tools/finalize-release.py with the analyzer, libunwind DLL, toolchain and Rust license paths. The Linux job first builds the same ROM-independent setup ZIP, then invokes:

```bash
python3 tools/package-appimage.py \
  --zip dist/superbomberman5-0.5.0-linux-x64.zip \
  --build-dir build-ci \
  --analyzer snesrecomp/recompiler-rs/target/release/snesrecomp-analyze \
  --rust-sysroot "$(rustup run 1.97.1 rustc --print sysroot)" \
  --appimagetool /path/to/appimagetool --runtime /path/to/runtime-x86_64
```

The workflow verifies pinned SHA-256 hashes for appimagetool and the type2 runtime. Updating these pins requires reviewing their source/notice changes. The Linux finalizer removes upstream game screenshots/test fixtures, adds Cargo/Rust notices and the native analyzer, audits the ZIP and wraps it in AppRun plus an authored SVG icon. The intermediate Linux ZIP is a CI artifact; the public Linux download is the AppImage. Preserve SOURCE_REVISIONS, PACKAGE_MANIFEST.json, FILE_SHA256SUMS and notices.

AppRun installs a versioned writable copy under `${XDG_DATA_HOME:-$HOME/.local/share}/superbomberman5/<version>`. It never overwrites an existing generated build. Subsequent launches use `build/SuperBomberman5SNESRecomp` when present. Test `--appimage-extract-and-run` for systems without FUSE. Linux requires x86_64, glibc 2.35+ and desktop graphics/audio libraries; Windows requires Windows 10+ x64. Both need several GB free and internet for the first portable tools download. Rust is not required by players.

## Publish and verify

Generate bilingual notes with `python tools/release-notes.py --output .bootstrap/notes.md`; read/send UTF-8 bytes, then verify the remote body exactly matches and has no U+FFFD. Stage explicit paths and preserve the user's working tree. Push reviewed commits, create a new version tag at the release commit and publish only audited assets. Never move published tags or overwrite assets without explicit authorization.

Verify the default branch, tag target, successful workflow, prerelease state, UTF-8 notes, asset sizes and SHA-256 digests. SHA256SUMS covers both public packages; FILE_SHA256SUMS covers the setup payload. GitHub also exposes each uploaded asset digest. Check remote state before retrying an API failure to avoid duplicates.

## Português

Toda nova release exige README.md atualizado nas duas línguas, com informações úteis: downloads, formatos, instalação/atualização, recursos, requisitos e limites reais de validação. Atualize VERSION, STATUS.md, este documento e o gerador de notas.

A v0.5.0 distribui **ZIP Windows x64**, **AppImage Linux x86_64 experimental** e **SHA256SUMS**. Ambos incluem setup sem ROM, fontes, analisador, licenças e os quatro mods opcionais desligados por padrão. O jogo é gerado apenas na máquina do jogador. Use checkout limpo, LLE e setup host; nunca publique o executável de desenvolvimento, ROM, código gerado, saves ou capturas.

A workflow Release é manual e só publica quando os dois builds e verificações passam. Valide os pacotes extraídos, analisador, primeira abertura/reabertura e geração/rebuild local. O AppImage copia o setup para a pasta XDG gravável da versão e reutiliza o jogo gerado; sem FUSE, use --appimage-extract-and-run. Registre plataforma, rota/frames e limitações no STATUS.md. Preserve fontes, licenças, revisões e checksums. Não reescreva tags ou assets públicos automaticamente. Confirme notas UTF-8, estado de pré-release e hashes dos arquivos no GitHub.

A skill instalada é `$super-bomberman5-repository`, com cópia versionada em `.agents/skills/super-bomberman5-repository/`.
