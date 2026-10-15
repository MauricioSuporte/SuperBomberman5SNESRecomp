---
name: super-bomberman5-repository
description: Maintain Super Bomberman 5 SNESRecomp Git history, versions, bilingual documentation, publication exclusions, validation notes and GitHub releases. Use for repository maintenance and publishing this project, rather than gameplay implementation alone.
---

# Super Bomberman 5 repository management

Maintain the project as a separate game repository with pinned SNESRecomp/recomp-ui submodules and ROM-free setup releases, following the Mega Man X Recompiled architecture without copying its game-specific code or claiming its feature set.

## Resolve the repository and authorization

Find the checkout from the current workspace and `rom_identity.txt` (`game_id = superbomberman5-jpn`). The expected public remote is `MauricioSuporte/SuperBomberman5SNESRecomp`; verify `git remote -v` and the authenticated account before external mutations. Do not silently replace another remote or publish to another account. Read applicable AGENTS.md, README.md, docs/STATUS.md, docs/PUBLISHING.md, VERSION, framework_pins.txt and current Git status as needed. Respect existing user changes.

Use the user's current request and earlier authorization for commits, pushing, repository creation and releases. This skill is not standing authorization to publish unrelated future work. When the user has already asked to publish this version, complete the checks and publication without asking them to approve the same action again. Never print credentials, store tokens in tracked files or place tokens in remote URLs. Prefer an authenticated CLI/API; Git Credential Manager credentials may be used in memory.

## Maintain Git and versions

- Inspect the diff and relevant tests before committing. Commits describe the resulting behavior, not the chat history. Stage explicit paths; exclude unrelated changes.
- Keep submodules pinned by committed gitlinks. Update framework_pins.txt with the framework's record_pins.sh after an intentional dependency change and verify the pins before release. Branch names in .gitmodules are not version pins.
- VERSION is the project version. Coordinate it with tag names, package names, binary version and GitHub release title. Until accuracy and broader gameplay are validated, keep releases marked as prereleases and explain the remaining interpreter coverage. Do not invent full-game, multiplayer, netplay or platform support.
- Create a tag only after its package and publication audit pass. Never move an existing published tag, rewrite shared history, force-push or overwrite a release asset without explicit authorization for that particular replacement. For an existing version, inspect its tag/release before deciding whether to finish an incomplete release or create a new version.

## Keep documentation and observations accurate

Update English and Portuguese README sections together when installation steps, controls, supported ROM identity, features, compatibility or release availability change. Use the Mega Man X README's organization (download, status/features, quick start, controls, source build, diagnostics, architecture, credits/license), but describe only this project's implemented and validated behavior.

Record concrete evidence, unresolved failures, workarounds and next engineering steps in docs/STATUS.md. Keep instructions in docs/BUILD_WINDOWS.md and publication mechanics in docs/PUBLISHING.md. Update THIRD_PARTY_NOTICES.md for bundled components. A local filename containing USA does not override the verified JPN header or rom_identity.txt hashes. A checksum-valid image is not proof of unmodified original-dump provenance.

Release notes belong in GitHub Releases; prepare transient notes under ignored output directories. Preserve existing licenses and notices. Gameplay screenshots are allowed in README, documentation and releases; store selected images under `docs/images/`. Do not embed extracted sprites, tiles, palettes, music or other cartridge assets. Treat source/build/packaging success and runtime accuracy as separate claims.

Generate release notes with `tools/release-notes.py --output <path>` instead of a Windows console pipeline. Read the file explicitly as UTF-8, send UTF-8 JSON bytes to the API, then verify the remote body equals the source and contains no U+FFFD. Use `--version` for a historical release; do not change its assets or tag to repair text. For the PT-BR mod, read docs/TRANSLATION_PTBR.md and distinguish package version from host version: the v0.1.0 host predates the plugin. Translation metadata and authored glyph code are permitted source; extracted glyphs stay local; selected gameplay screenshots may be published under `docs/images/`.

## Exclude game data and local material

Review `.gitignore`, tracked files and history using `python tools/audit-publication.py --history`. Ignore files are not removed from existing commits automatically. Never publish ROMs, generated game C, game executables containing ROM-derived compiled code, extracted graphics/audio, raw memory captures, saves, save states, ROM paths, private diagnostics or secrets. Analysis configuration, symbol metadata and hashes are allowed; raw memory/audio execution captures stay local. Gameplay screenshots under `docs/images/` are explicitly permitted, including the existing `five-player-showcase.png`; their presence in history alone must not block publication or trigger history cleanup.

Gameplay screenshot permission was explicitly granted by the maintainer on 2026-10-15; it does not permit extracted assets or change third-party rights. For other new original or licensed assets, review and document provenance before intentionally changing the guard's allowlist. Do not use `git add -f` to bypass the rule. Audit top-level project history; leave dependency histories intact and preserve their licenses. If restricted content already exists in shared history, stop that publication and explain the specific finding; history cleanup needs a deliberate plan, not an automatic destructive rewrite.

## Build, package and publish

Read docs/PUBLISHING.md for current commands. Release a clean-source **setup host** with SNESRECOMP_SETUP_HOST=ON, no src/gen/*.c or recomp/funcs.h, and the validated LLE frame implementation. The normal development game executable is not a public release artifact. Use the framework's packager, then tools/finalize-release.py to include the ROM-independent native analyzer and its DLL/license, omit dependency gameplay screenshots/test ROM fixtures, audit the final ZIP and produce SHA256SUMS.

Test the extracted package: its root setup host and launcher open; the packaged analyzer runs with its bundled DLL; ROM selection/code generation/rebuild produces a local playable executable. Keep the generated C and resulting game build local. New packaging logic warrants meaningful tests with synthetic forbidden files; do not treat a green filename audit as a guarantee about copyright ownership or every binary's contents.

When authorized, create or reuse the public game repository, push reviewed commits and the verified tag, then publish a prerelease with the audited Windows ZIP, Linux AppImage, SHA256SUMS, bilingual release notes, precise platform coverage and known limitations. Verify the remote default branch, tag target, release state, uploaded asset sizes/digests and workflow result. Do not advertise Linux/macOS packages until built and checked. On an API/auth failure, inspect state before retrying to avoid duplicate repositories, tags or releases; stop after repeated identical failures and report the remaining step.

End with links to the repository/release, version, installed skill and material limitations or outstanding work. Never say publication completed unless GitHub confirms it.


## Every release updates the README

Before publishing any new release, update README.md in both English and Portuguese with useful information for that version: current downloads and platform formats, installation or upgrade steps, included features/mods, required host versions and concrete validation limits. Review the full README for obsolete version/platform claims, keep still-valid instructions, and use precise release links. Do not publish a new version with an outdated README or replace useful instructions with release-note boilerplate. Update docs/PUBLISHING.md and the release-notes generator to match the final artifacts.


## Linux AppImage and Windows ZIP

Publish both the audited Linux x86_64 AppImage and Windows x64 setup ZIP for each release once both builds and package checks pass. AppImage is Linux-only; never rename a Windows executable or ZIP to .AppImage. Preserve the ROM-independent setup-host requirement on both platforms. Use tools/package-appimage.py after the framework Linux setup packager: it includes the native analyzer and licenses, removes upstream game screenshots/test fixtures, audits the payload and wraps it in scripts/AppRun. AppRun installs a versioned writable copy under ${XDG_DATA_HOME:-$HOME/.local/share}/superbomberman5/<version> so Generate & rebuild can run without writing to the AppImage mount. Subsequent launches use the locally rebuilt game when present. Never overwrite the player's generated files.

Build and test on the target OS, check extracted package contents and analyzer, and exercise first and repeat launches plus local ROM generation/rebuild. Record actual runtime coverage and untested distributions in docs/STATUS.md; an AppImage build does not establish full-game Linux support. The Release workflow builds both platforms before publication; pin and verify downloaded packaging tools and publish SHA256SUMS for both artifacts. Preserve public tags/assets.
