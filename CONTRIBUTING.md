# Contributing / Como contribuir

Use a legally obtained local ROM matching `rom_identity.txt`. Clone recursively and follow `docs/BUILD_WINDOWS.md`. Never commit the ROM or `src/gen/`, never edit generated C by hand, and do not copy extracted graphics, music, saves or gameplay captures into tracked files.

Change analysis inputs in `recomp/` or integration code in `src/`, regenerate locally, and test the affected route. Distinguish compiled AOT coverage from interpreter execution; do not describe a successful boot as full-game validation. Record the current evidence and limitations in `docs/STATUS.md`, and update both README languages when player-facing behavior changes.

Before a commit or publication, run:

```powershell
python tools/audit-publication.py --history
git diff --check
```

The guard checks the top-level project and its history. Submodules retain their own upstream history and licenses. A `.gitignore` entry does not remove files already tracked or committed. If the audit finds restricted material in existing history, stop publication and decide how to remove it without silently rewriting shared history.

For releases, keep `VERSION`, the tag, package names and release notes consistent. Publish setup packs built from a clean source checkout without generated game code; validate the final archive and its SHA-256. Release notes belong on GitHub Releases, rather than a tracked `RELEASE_NOTES*.md` file. See `docs/PUBLISHING.md`.

## Português

Use sua própria ROM local, conforme os hashes de `rom_identity.txt`. Altere a configuração de análise ou os fontes de integração, regenere o código e teste o comportamento afetado. Não versione ROM, código gerado, executável derivado da ROM, sprites, música extraída, saves ou capturas do jogo.

Atualize `docs/STATUS.md` com testes e limitações reais e mantenha as duas partes do README coerentes. Antes de publicar, execute a auditoria acima. Releases devem conter o programa de instalação e fontes sem dados do jogo; o código do jogo é gerado no computador do jogador. Preserve as licenças das dependências.
