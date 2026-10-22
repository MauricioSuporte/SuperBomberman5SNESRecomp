# Quiver Launcher

## Português

[Quiver Launcher](https://github.com/tgeorgiadis/quiver-launcher) instala e atualiza apps a partir das
releases do GitHub e lista os projetos do catálogo de [quiverlauncher.com](https://quiverlauncher.com).
Desde a v2.0.0 este repositório segue o que o Quiver espera:

- cada release tem `superbomberman5-<versão>-windows-x64.zip` e
  `superbomberman5-<versão>-linux-x86_64.AppImage` (nomes reconhecidos pelo `PlatformAssetMatcher`
  do Quiver) e `SHA256SUMS`;
- o ZIP tem uma única pasta, que o Quiver achata, e **um só executável na raiz**:
  `SuperBomberman5Recomp.exe` (o atualizador fica em `updater/`, para não aparecer como opção);
- a ROM é fornecida pelo jogador: o Quiver a copia para `Super Bomberman 5 (USA).sfc` ao lado do
  executável, que o host procura sozinho; a ROM é conferida pelo SHA-256 de `rom_identity.txt`;
- o atualizador do jogo reconhece uma instalação do Quiver (`installed-build.txt`) e manda atualizar
  pelo Quiver, para a biblioteca dele não ficar numa versão antiga. Saves, `rom.cfg`, configurações e a
  pasta `build/` continuam na pasta do app.

### Cadastro no catálogo

O cadastro é feito pelo mantenedor em <https://quiverlauncher.com/submit> (precisa entrar com a conta
dele). Dados para o formulário:

| Campo | Valor |
|---|---|
| Repositório | `MauricioSuporte/SuperBomberman5SNESRecomp` (GitHub) |
| Nome / jogo | Super Bomberman 5 — Super Nintendo (`snes`) |
| Tipo | port (recompilação SNESRecomp) |
| Sistemas | Windows, Linux |
| Pasta | `SuperBomberman5-SuperBomberman5SNESRecomp` |
| Executável preferido | Windows `SuperBomberman5Recomp.exe`; Linux `superbomberman5-*-linux-x86_64.AppImage` |
| Arquivo de setup (ROM) | destino `Super Bomberman 5 (USA).sfc`, extensões `sfc`/`smc`, SHA-256 `5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4` |
| Uso de IA | gerado com assistência de IA (como os outros recomps SNES do catálogo) |
| Tags | recomp, snes, bomberman, multiplayer, netplay, multitap, mod support, controller support |

Entrada equivalente no formato que o Quiver lê (`launcher`):

```json
{
  "folderName": "SuperBomberman5-SuperBomberman5SNESRecomp",
  "filesToAdd": [],
  "preferredExecutables": {
    "windows": ["SuperBomberman5Recomp.exe"],
    "linux": ["superbomberman5-*-linux-x86_64.AppImage"]
  },
  "setupFiles": [{
    "id": "sb5",
    "label": "Super Bomberman 5 ROM",
    "destination": "Super Bomberman 5 (USA).sfc",
    "extensions": ["sfc", "smc"],
    "accepted": [{"algo": "sha256", "value": "5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4"}]
  }]
}
```

Para usar antes do cadastro: no Quiver, **+ Add New Entry** com o repositório acima e a pasta da tabela.

## English

Since v2.0.0 releases follow Quiver Launcher's conventions: platform-labelled assets
(`-windows-x64.zip`, `-linux-x86_64.AppImage`, `SHA256SUMS`), a ZIP with one wrapper folder and a single
root executable (`SuperBomberman5Recomp.exe`; the updater lives in `updater/`), and a player-supplied
ROM copied by Quiver to `Super Bomberman 5 (USA).sfc` beside the executable and verified by the
SHA-256 in `rom_identity.txt`. The in-game updater detects a Quiver install (`installed-build.txt`) and
defers to Quiver. The maintainer submits the catalog entry above at <https://quiverlauncher.com/submit>
(sign-in required); until then, add it in Quiver with **+ Add New Entry** using the repository above.
