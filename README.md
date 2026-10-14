# Super Bomberman 5 Recompiled

An unofficial, noncommercial **Super Bomberman 5** PC port prototype built with [SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp) and [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui). It follows the separate-game-repository architecture used by [Mega Man X Recompiled](https://github.com/mstan/MegaManXSNESRecomp).

[Download / Baixar](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases) · [English](#english) · [Português](#português) · [Technical status](docs/STATUS.md) · [Contributing](CONTRIBUTING.md)

> **Early prototype, partial recompilation.** Most game logic currently uses the framework's interpreter. First-stage entry has been verified locally; full-game accuracy and multiplayer have not been established. Supply your own legally obtained ROM. No ROM or extracted game assets are included.

## English

### Features and current validation

| Feature | Status |
|---|---|
| Windows x64 executable and launcher | Release build and launcher smoke test passed locally |
| Opening, title, normal-mode menus, Zone 1 map and first-stage entry | Verified with captures; scripted route completed 3,628 frames |
| Windows video and audio devices | Initialization and callbacks tested; audio accuracy not independently compared |
| Static CPU recompilation | Small AOT coverage; reset/NMI and most game logic remain interpreted |
| Five-player multitap | Port 2 configured; P2–P5 and battle mode not validated |
| PT-BR mod (v0.2.0) | Optional title, files, options and battle setup/rules translation preview; included in the v0.2.0 setup pack |
| Normal Game character selection (v0.3.0) | Optional Battle picker after file confirmation; second/ninth characters verified in Zone 1-1 |
| Character selection colors (v0.4.0) | Six palettes with L/R for all nine characters; Normal and Battle stage routes verified |
| Netplay / widescreen enhancements | Not implemented for this title |
| Linux / macOS | Framework build paths exist; this release is Windows only |

### Quick start — Windows setup release

1. Download the Windows x64 ZIP from [Releases](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases). Extract the **entire archive** into a writable folder.
2. Open `SuperBomberman5SNESRecomp.exe` at the package root. This is a setup program; the package contains no recompiled game code.
3. Let the launcher obtain the official portable build tools, then select your own ROM matching the hashes below. Internet is needed for the first tool download.
4. Select **Generate & rebuild**. The native analyzer is included; installing Rust is unnecessary for release users. Game code is generated and compiled locally.
5. Run the resulting game under `build/`; configure controls in the launcher.

Requires Windows 10 or newer, x64. Leave several GB free for extraction, tools and build output. Open the root executable to start release setup; the repository's developer `play.cmd` is for local source builds.

### Supported ROM

The developer's local filename is `Super Bomberman 5 (USA).sfc`, but its header reports **JPN**. Compatibility is determined by hashes, not filenames. A valid internal checksum alone does not establish original-dump provenance. Other revisions and translations have not been qualified.

| Identity | Value |
|---|---|
| Header title | `Hu SUPER BOMBERMAN 5` |
| Mapping | Fast HiROM; no special coprocessor detected |
| ROM / SRAM | 4 MiB / 8 KiB |
| CRC32 | `c25ab56a` |
| SHA-256 | `5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4` |

[rom_identity.txt](rom_identity.txt) is authoritative. No ROM download links are provided.

### Mods — Brazilian Portuguese preview

The v0.2.0 release includes **Português brasileiro — menus (prévia)** in **Mods**. Enable **Menus em português brasileiro** and select Play. It translates mapped title, player/file selection, options, sound test and battle setup/rules labels with authored glyphs and Portuguese accents, without modifying the ROM. Disable it to restore original presentation. This is a partial menu translation; unmapped screens, dialogue/endings and gameplay messages remain original. Title blinking is preserved without clipping the normal-game label. The published host v0.1.0 predates the plugin and cannot run this mod. See [PT-BR implementation and testing](docs/TRANSLATION_PTBR.md).

Build a data-only installable archive with `python tools/generate-ptbr.py --mod-out dist/superbomberman5-ptbr-menus-0.1.0.snesmod`. The trusted plugin must already be linked into the host; the package cannot install native code.

### Controls and starting a game

The v0.4.0 setup pack also includes **Choose character colors with L and R**:
white, black, red, blue, green, yellow for all nine characters, in Battle and
the optional Normal picker. See [controls and build requirements](docs/SELECTION_COLORS.md).

The v0.3.0 setup pack includes an optional **Normal Game — character selection**
mod. Enable **Choose a character in Normal Game** in Mods to open the original
Battle character picker after confirming a Normal file. Complete Generate & rebuild using the v0.3.0 setup host. See [instructions and validation](docs/NORMAL_CHARACTERS.md).

| Keyboard | SNES input |
|---|---|
| Arrow keys | Direction |
| Enter | Start |
| X / Z | A / B |
| S / A | X / Y |
| C / V | L / R |
| Right Shift | Select |

Press Enter in the opening/title sequence. Choose **Normal Game → 1 Player Game → New Game → File 1**, confirming with A (keyboard X). On the Zone 1 map, use Start (Enter) to enter the stage. Controls can be changed in the launcher. Five-player configuration is preparatory, not a claim of tested multiplayer.

### Building from source

Install Git for Windows and Python 3.11+, then:

```powershell
git clone --recurse-submodules https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp.git
cd SuperBomberman5SNESRecomp
./tools/setup-windows.ps1
./tools/build-windows.ps1 -Regenerate -Rom 'C:/roms/your-game.sfc'
./tools/build-windows.ps1 -Run -Rom 'C:/roms/your-game.sfc'
```

With the developer ROM under its default filename at the repository root, `play.cmd` opens the launcher after a successful build. Tools are installed under ignored `.tools/`; the global PATH is unchanged. See [Windows build instructions](docs/BUILD_WINDOWS.md).

The validated build uses `SNESRECOMP_FRAME_IMPL=LLE`. HLE composition has not been qualified for this title. Regenerate after changing `recomp/`; never hand-edit or commit `src/gen/`.

### What recompilation means here

SNESRecomp translates analyzable 65816 CPU routines into C and links them with a SNES hardware runtime. Unresolved paths use an interpreter; PPU, APU/SPC700, DMA and cartridge hardware remain modeled. This is a hybrid port, not recovered original game source. AOT coverage is currently small. Promoting reset/NMI causes continuation/dispatch failures, so those paths remain interpreted pending a validated fix.

### Reporting problems

[Open an issue](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/issues) with the project version, Windows version, ROM hash, settings, stage and steps to reproduce. Inspect local reports for personal paths or memory data before attaching. Do not upload the ROM, extracted assets, audio dumps or save states to public issues. Known limitations are in [STATUS.md](docs/STATUS.md).

### Repository layout

| Path | Purpose |
|---|---|
| `src/` | Game integration and desktop host descriptor |
| `recomp/` | Analysis configuration and symbol metadata |
| `src/gen/` | Locally generated ROM-derived C; ignored |
| `snesrecomp/`, `recomp-ui/` | Framework and launcher pinned as submodules |
| `rom_identity.txt`, `VERSION`, `framework_pins.txt` | ROM identity, version and dependency commits |
| `tools/`, `scripts/` | Build, publication audit and setup packaging |
| `docs/` | Build, validation evidence, limitations and publication workflow |
| `.agents/skills/super-bomberman5-repository/` | Reusable Git/documentation/release skill |

### Credits and license

Thanks to Matthew Stanley and RetroPorting ToolKit contributors for SNESRecomp and recomp-ui, and to Mega Man X Recompiled for the architecture reference. Project software uses [PolyForm Noncommercial 1.0.0](LICENSE). Dependency notices are preserved; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). This license does not cover Super Bomberman 5 ROM or game content. The project is not affiliated with or endorsed by the game's rights holders.

## Português

### Sobre o projeto

Protótipo não oficial e não comercial de port de **Super Bomberman 5** para PC, usando SNESRecomp e o launcher recomp-ui, com a estrutura de repositório por jogo utilizada pelo Mega Man X Recompiled.

**A recompilação ainda é parcial:** a maior parte da lógica usa o interpretador. Abertura, título, menus, mapa da Zone 1 e entrada na primeira fase foram verificados numa rota de 3.628 frames. Isso não comprova o jogo completo, precisão de som, colisões ou multiplayer. O multitap para cinco jogadores está configurado, mas não foi validado. Não há netplay nem melhorias de widescreen neste jogo.

### Como instalar e jogar

1. Baixe o ZIP Windows x64 em [Releases](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases) e extraia **todo o conteúdo** para uma pasta gravável.
2. Abra `SuperBomberman5SNESRecomp.exe` na raiz do pacote. Esse é o instalador, sem código recompilado do jogo.
3. Permita baixar as ferramentas oficiais e selecione sua própria ROM. A primeira instalação requer internet para obter as ferramentas.
4. Clique em **Generate & rebuild** para gerar, compilar e iniciar o jogo localmente. O analisador já está incluído; usuários da release não precisam instalar Rust.
5. Nas próximas execuções, abra o jogo em `build/` e ajuste os controles no launcher.

Requer Windows 10 ou superior, x64, e alguns GB livres. **Nenhuma ROM, código gerado da ROM ou recurso extraído acompanha o download.** O arquivo de desenvolvimento tem “USA” no nome, porém o cabeçalho indica **JPN**. Confira os hashes em [Supported ROM](#supported-rom); o nome do arquivo não garante compatibilidade.

### Mods — tradução PT-BR (prévia)

Na versão v0.2.0, depois de Generate & rebuild, abra **Mods → Português brasileiro — menus (prévia)**, ative **Menus em português brasileiro** e clique em Play. Há traduções para título, seleção de jogadores/arquivos, cabeçalhos, opções, teste de som, menus/regras de batalha e cabeçalho de fases, com acentos numa fonte própria. O aviso de Start pisca sem cortar JOGO NORMAL. É uma tradução parcial; telas não mapeadas e mensagens de fases/finais ainda permanecem originais. Desative para retornar à apresentação original. **A release antiga v0.1.0 não contém o plugin**. Veja [TRANSLATION_PTBR.md](docs/TRANSLATION_PTBR.md) para instruções, escopo e criação do `.snesmod`.

### Controles e início de partida

O pacote v0.4.0 também inclui **Choose character colors with L and R**:
branco, preto, vermelho, azul, verde e amarelo para os nove personagens, no
Battle e na seleção opcional do Normal. Veja [controles e requisitos](docs/SELECTION_COLORS.md).

O pacote v0.3.0 inclui o mod opcional **Normal Game — character selection**.
Ative **Choose a character in Normal Game** em Mods para abrir
a seleção original do Battle depois de confirmar o arquivo. Conclua Generate & rebuild usando o setup host v0.3.0. Veja [instruções e validação](docs/NORMAL_CHARACTERS.md).

**Setas** para direção, **Enter** para Start, **X/Z** para A/B e **Shift direito** para Select. Pressione Enter na abertura/título, escolha **Normal Game → 1 Player Game → New Game → File 1**, confirme com **X** e pressione **Enter** no mapa da Zone 1 para entrar na fase. Remapeie os controles no launcher se desejar.

### Compilar, contribuir e publicar

Instale Git for Windows e Python 3.11+, clone com `--recurse-submodules` e execute os comandos de [Building from source](#building-from-source). Os scripts preparam ferramentas em `.tools/`, sem alterar o PATH global. Depois de compilar, `play.cmd` abre o launcher com a ROM local padrão. Consulte [BUILD_WINDOWS.md](docs/BUILD_WINDOWS.md) e [CONTRIBUTING.md](CONTRIBUTING.md).

A build validada usa composição LLE. Reset/NMI permanecem interpretados devido a falhas de continuidade na tentativa AOT. Precisamos ampliar a cobertura nativa e comparar o comportamento com uma referência independente.

[Abra uma issue](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/issues) informando versão, Windows, hash da ROM e reprodução. Não publique ROM, sprites/músicas extraídos, dumps ou save states. Confira dados pessoais nos relatórios antes de anexá-los.

O estado e as observações ficam em [STATUS.md](docs/STATUS.md); o fluxo de versões e releases está em [PUBLISHING.md](docs/PUBLISHING.md). A skill de manutenção fica em `.agents/skills/super-bomberman5-repository/`. ROM, fontes gerados, builds e capturas ficam fora do Git. Releases contêm instalador e fontes sem dados do jogo.

O software usa [PolyForm Noncommercial 1.0.0](LICENSE), preservando as licenças das dependências. Veja [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). A licença não concede direitos sobre a ROM ou conteúdo de Super Bomberman 5. O projeto não tem vínculo ou endosso dos titulares do jogo.
