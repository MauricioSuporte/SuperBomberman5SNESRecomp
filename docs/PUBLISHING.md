# Versions and publication / Versões e publicação

Public repository: `MauricioSuporte/SuperBomberman5SNESRecomp`. The initial release is **v0.1.0**, marked as a prerelease. Windows x64 is the qualified platform; Linux/macOS are not release targets yet.

Current VERSION is **0.2.0**, the Windows prerelease with the optional PT-BR menu mod v0.1.0. Its setup pack includes the mod; a separate data-only `.snesmod` is also provided. Preserve the older v0.1.0 tag and assets.

Generate notes with `python tools/release-notes.py --output .bootstrap/notes.md`; read that file as UTF-8 and send UTF-8 JSON bytes through the API. Avoid Windows console pipelines that may replace accented characters. Verify the returned body equals the authored text and contains no U+FFFD. Use `--version 0.1.0` when repairing the already published release.

## Before committing

Keep `README.md` English/Portuguese sections, `docs/STATUS.md`, build instructions, VERSION and dependency pins consistent with the actual change. Run:

```powershell
python tools/audit-publication.py --history
python -m unittest discover -s tests -v
git diff --check
```

The audit checks this project's tracked files/history, not the upstream submodule histories. Ignore patterns do not remove already committed files. New binary/media assets require a provenance review and a deliberate guard update. No local ROM, generated C, game build, extracts, saves, dumps, captures or personal settings are published.

## Release contents

A **setup pack**, built from a clean checkout with `SNESRECOMP_SETUP_HOST=ON` and `SNESRECOMP_FRAME_IMPL=LLE`. It includes a setup host, launcher assets with upstream notices, project/framework sources, ROM identity hashes and the native analyzer with its libunwind DLL/license. Portable compiler tools are downloaded from the official toolchain release on first use. The game C and playable executable are generated only on the user's machine.

Do not package the development `build-lle/` executable: it includes locally generated ROM-derived code. The framework rejects a setup build when `src/gen/*.c` exists. Keep the user's development build intact and use a separate clean clone for packaging.

The official packager is `scripts/package_release.sh`; its output is completed and audited by:

```powershell
python tools/finalize-release.py --zip dist/superbomberman5-0.2.0-windows-x64.zip --build-dir build-ci --analyzer snesrecomp/recompiler-rs/target/release/snesrecomp-analyze.exe --toolchain C:/path/to/toolchain --cargo-home C:/path/to/cargo --rust-sysroot C:/path/to/rust-toolchain
python tools/audit-publication.py --zip dist/superbomberman5-0.2.0-windows-x64.zip
```

Use the exact package version in the filename. The finalizer creates `dist/SHA256SUMS`. Preserve `SOURCE_REVISIONS`, `PACKAGE_MANIFEST.json` and licenses in the archive.

## GitHub workflow

The **Release** workflow is manual, Windows only and pins the portable toolchain to v1.0.14. It checks source/history, builds the setup host and native analyzer, packages/audits the ZIP, and uploads an artifact. Set `publish_release` to true to publish only after successful checks. The version input must match VERSION. Releases default to prerelease; changing that status requires broader runtime validation.

For local publication, complete the same checks, test extraction and local Generate → rebuild, commit, push, then create the version tag at that commit and upload the audited ZIP and SHA256SUMS. Verify the remote tag target and uploaded asset digest. Do not move a public tag or overwrite published assets without an explicit replacement decision.

Release notes belong on GitHub Releases, not tracked `RELEASE_NOTES*.md` files. `tools/release-notes.py` generates bilingual prototype notes to an ignored temporary path. Update that generator when the release scope changes. Do not blindly repeat prototype limitations in future releases after they are resolved.

## Português

Antes de publicar, atualize versão, README nas duas línguas, observações e testes em STATUS.md, instruções e licenças. Execute auditoria, testes e revisão do diff. A primeira versão é uma **pré-release v0.1.0 para Windows x64**, com recompilação parcial.

Prepare o pacote a partir de um clone limpo, sem código gerado. Ele contém o instalador, fontes e analisador independente da ROM; o jogador fornece sua própria ROM e gera o jogo localmente. Não publique o executável de desenvolvimento, que contém código derivado da ROM.

A workflow Release é manual. Ela só publica quando `publish_release` está ativo e todas as verificações passam. Valide também o pacote extraído e a geração/compilação local. Preserve licenças, arquivos de revisão e checksums. Não reescreva tags públicas automaticamente.

A skill instalada é `$super-bomberman5-repository`, com cópia versionada em `.agents/skills/super-bomberman5-repository/`. Ela gerencia Git, versões, documentação, exclusões, anotações, auditorias e releases, respeitando a autorização de cada pedido.
