# Super Bomberman 5 Recompiled

An unofficial, noncommercial **Super Bomberman 5** PC port prototype built with [SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp) and [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui). It follows the separate-game-repository architecture used by [Mega Man X Recompiled](https://github.com/mstan/MegaManXSNESRecomp).

[Download / Baixar](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases) · [English](#english) · [Português](#português) · [Technical status](docs/STATUS.md) · [Contributing](CONTRIBUTING.md)

> **Early prototype, partial recompilation.** Most game logic currently uses the framework's interpreter. First-stage entry has been verified locally; full-game accuracy and multiplayer have not been established. Supply your own legally obtained ROM. No ROM or extracted runtime assets are included; documentation contains gameplay screenshots.

<p align="center"><img src="docs/images/five-player-showcase.png" width="820" alt="Five-player title menu, six-column character selection with Honey, Kotetsu, Subordinate Bomber and colors, Zone 1-1 with five different characters and Battle Mode stage 1 with five characters and palettes"></p>

<p align="center"><b>New in v0.10.0:</b> Gold Battle arenas 11–13 and Hyper Revenge, native-style PT-BR menus, corrected pause sounds and the Subordinate secret slot.<br><b>Novidades da v0.10.0:</b> arenas 11–13 e Revanche Hiper, menus PT-BR com fontes nativas, sons do pause corrigidos e Subordinado na célula secreta.</p>

## English

### Features and current validation

| Feature | Status |
|---|---|
| Windows x64 executable and launcher | Release build and launcher smoke test passed locally |
| Opening, title, normal-mode menus, Zone 1 map and first-stage entry | Verified with captures; scripted route completed 3,628 frames |
| Windows video and audio devices | Initialization and callbacks tested; audio accuracy not independently compared |
| Static CPU recompilation | Small AOT coverage; reset/NMI and most game logic remain interpreted |
| Five-player multitap | Port 2 configured; launcher PLAYER 1–5 tabs; Normal co-op and five-player Battle exercised with scripted inputs; full campaign/physical controllers unqualified |
| Normal Game five players (v0.5.0) | Optional 1–5 player campaign with shared lives, Start rejoin, item carry-over, per-seat characters/colors; boss stages with rival bombers seat up to four |
| PT-BR mod (v0.2.0) | Optional menu translation; v0.8.2 adds file headings, Game Over, HUD, password reward and all 33 completion ranks/tips |
| Normal Game character selection (v0.3.0) | Optional Battle picker after file confirmation; second/ninth characters verified in Zone 1-1 |
| Character selection colors (v0.4.0) | Six palettes with L/R for all ten characters; Normal and Battle stage routes verified |
| Subordinate Bomber (v0.6.0) | Optional tenth character in the X cell with six colors, authored portraits and HUD faces; Battle, Normal and Zone 3-D scripted routes verified on Linux LLE |
| Pause — exit & draw (v0.7.0) | Normal map return, Battle Draw/restart and arena return; Windows LLE routes with 1/2/5 campaign seats and 2-human/3-CPU Battle |
| Portal destinations (v0.8.0) | Optional hints from native exit scripts; all 192 destinations audited; Windows/LLE visibility, both PPUs and PT-BR/pause fixtures passed |
| Honey and Kotetsu (v0.10.0) | Optional sixth picker column with the Zone 1-3 secret-stage bosses, own frames/poses/Louie riding from the ROM; Battle, Normal and secret-stage routes verified on Linux LLE |
| Bomber Kings (unreleased) | Optional third picker row with Super Bomberman 4's Bomberman and Bomber Kings read from the player's own SB4 ROM, five SB4 colors, B + direction abilities with debuff (also Honey/Kotetsu); Battle (FFA/Team) and Normal scripted routes on Linux LLE |
| Netplay / widescreen enhancements | Not implemented for this title |
| Linux / macOS | Experimental x86_64 AppImage: Ubuntu 22.04 CI; Ubuntu 24.04/WSL local rebuild and 3,628-frame LLE route; macOS unavailable |

### Quick start — Windows setup release

1. Download the Windows x64 ZIP from [Releases](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases). Extract the **entire archive** into a writable folder.
2. Open `SuperBomberman5SNESRecomp.exe` at the package root. This is a setup program; the package contains no recompiled game code.
3. Let the launcher obtain the official portable build tools, then select your own ROM matching the hashes below. Internet is needed for the first tool download.
4. Select **Generate & rebuild**. The native analyzer is included; installing Rust is unnecessary for release users. Game code is generated and compiled locally.
5. Run the resulting game under `build/`; configure controls in the launcher.

Upgrading: extract v0.10.0 into a new folder and run Generate & rebuild again with your own ROM; an older generated executable cannot load the new native mod code.

Requires Windows 10 or newer, x64. Leave several GB free for extraction, tools and build output. Open the root executable to start release setup; the repository's developer `play.cmd` is for local source builds.

### Linux AppImage

Download **superbomberman5-0.10.0-linux-x86_64.AppImage** from [v0.10.0](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.10.0), make it executable (`chmod +x superbomberman5-0.10.0-linux-x86_64.AppImage`) and run it. The AppImage copies its ROM-free setup into `${XDG_DATA_HOME:-~/.local/share}/superbomberman5/0.10.0` on first launch. Select your own supported ROM and use **Generate & rebuild**; subsequent launches open the locally rebuilt game. Rust is not required. Internet is needed for the first portable tool download. Linux x86_64, glibc 2.35+ (Ubuntu 22.04 baseline), desktop graphics/audio libraries and several GB free are required. If FUSE is unavailable, run with `--appimage-extract-and-run`. Keep the writable version folder to retain local builds and settings; ROM and generated game data are never part of the download. Linux distribution and full-campaign coverage remain limited; see [validation](docs/STATUS.md).

Windows users download the **superbomberman5-0.10.0-windows-x64.zip** setup pack. Both formats include the same ten optional mods, disabled by default. Upgrading from an earlier version: extract v0.10.0 into a new folder (or the AppImage creates a new 0.10.0 folder) and run **Generate & rebuild** again so the new native plugins are compiled. Check the release's **SHA256SUMS** before use.

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

### Included mods

The **v0.10.0 setup packs** include the ten optional mods below, each at mod version **0.1.0** and disabled by default. Complete **Generate & rebuild**, open **Mods**, enable the desired features and select Play. Each setup package contains the executable, launcher resources, analyzer, source and all ten preinstalled mods; no separate mod download or import is needed. `FILE_SHA256SUMS` inside each setup payload covers its packaged files. They require the corresponding native plugins in the host; importing a package cannot update an older executable.

| Mod / category | Feature and use |
|---|---|
| **Português brasileiro — menus (prévia)** / **Translation** | Enable **Menus em português brasileiro** to translate mapped title, player/file selection, options, sound test, battle setup/rules and stage headings. Also translates Continue/Erase headings, Normal Game Over, HUD prompts (APERTE START / FIM DE JOGO), the password reward and all 33 certificate ranks/tips. Native fonts and expanded Battle coverage require host v0.10.0 and Generate & rebuild. v0.10.0 uses native cartridge fonts with preserved heading crossbars and upper outlines, proportional menus and title text, translate BATTLE START as INÍCIO DA BATALHA, and translate all 13 Battle arena names without source-language flashes in checked transitions. Other unmapped text remains original. Disable to restore the original presentation. [Scope and testing](docs/TRANSLATION_PTBR.md). |
| **Normal Game — character selection** / **Gameplay** | Enable **Choose a character in Normal Game** to open the original Battle character picker after confirming a Normal save file. Use the D-pad and A to choose; B returns to file selection. The campaign uses the chosen character. Requires host v0.3.0 or newer. [Instructions and validation](docs/NORMAL_CHARACTERS.md). |
| **Normal Game — infinite time** / **Gameplay** | Enable **Unlimited time in Normal Game** for the native `-:--` timer without timeout deaths. Disabled by default. Requires host v0.4.3 or newer. [Details and validation](docs/INFINITE_TIME.md). |
| **Character selection — color controls** / **Gameplay** | Enable **Choose character colors with L and R** for white, black, red, blue, green and yellow palettes for the native bomber choices, including Subordinate when its mod is enabled. R cycles forward, L backward; A confirms. Starts: P1 white, P2 black, P3 red, P4 blue, P5 green. The v0.8.1 host fixes palette changes affecting the figure below. Works in Battle and in Normal when the character selection mod is enabled. Default keyboard: C/V; gamepad: LB/RB (L1/R1). Requires host v0.4.0 or newer. [Controls and validation](docs/SELECTION_COLORS.md). |
| **Normal Game — five players** / **Gameplay** | Enable **Play Normal Game with up to five players** and choose 1–5 PLAYER GAME on the title. Shared lives start at players + 2 (3–7); a dead player presses Start to spend a life and rejoin while a teammate is alive. Any player finishes a stage through an exit, items carry over for every seat, P3–P5 spawn on safe floor and take enemy contact damage. Boss stages with rival bombers seat up to four players (three with two rivals); the others return in the next stage. With the character and color mods, each seat picks its own character and color from New Game or Continue. Requires host v0.5.0 or newer. [Instructions, tests and limits](docs/NORMAL_COOP.md). |
| **Subordinate Bomber — playable character** / **Gameplay** | Enable **Play as Subordinate Bomber** to pick the Zone 3-D rival from the X/secret yellow Bomberman cell (fifth bottom-row column with Honey/Kotetsu) of the character selection, in Battle and, with the Normal character selection mod, in Normal Game. With the color controls mod, L/R choose its six palettes from the cartridge; its portrait and Battle HUD faces (alive and defeated) follow the chosen color. Requires host v0.6.0 or newer. [Instructions, tests and limits](docs/SUBORDINATE_BOMBER.md). |
| **Honey and Kotetsu — playable characters** / **Gameplay** | Enable **Play as Honey and Kotetsu** for a six-column character selection: the stock columns move closer, Honey (the cowgirl boss of the Zone 1-3 secret stages) ends the top row and Kotetsu the bottom row, next to Subordinate Bomber/X. They play as Bomberman (same rules and items) and use their own walking, attack, victory/draw, death and Louie-riding poses, portrait and HUD face, all decoded from your ROM. Battle and, with the Normal character selection mod, Normal Game. They have a single cartridge palette (L/R do not recolor them). Requires host v0.9.0. [Instructions, tests and limits](docs/HONEY_KOTETSU.md). |
| **Bomber Kings — Super Bomberman 4 characters** / **Gameplay** | Enable **Play as the Bomber Kings** and select **your own Super Bomberman 4 ROM** (Japanese release, checked by SHA-256) in the mod's **Super Bomberman 4 ROM** item on the Mods page. The selection gets three rows: the stock rows move up, Honey/Kotetsu end them and a new row holds Super Bomberman 4's Bomberman, Hammer, Jet, Lady, Bazooka and Great Bomber, drawn from that ROM at runtime with their five SB4 colors (L/R with the color mod). Hold **B** and press a **direction** for each character's special ability (Honey shot, Kotetsu slash, Hammer strike, Jet dash, Lady disc, Bazooka flames, Great shield); Hammer, Jet, Great, Honey and Kotetsu then move at half speed for about 10.7 s, as in Super Bomberman 4. B alone still detonates remote bombs; riding Louie keeps B native. Abilities hit enemies in Normal Game (never other players) and other players or the other team in Battle. Requires host v0.11.0. [Instructions, tests and limits](docs/BOMBER_KINGS.md). |
| **Gold Cartridge — Battle** / **Gameplay** | Enable **Arenas 11–13 and Hyper Revenge** to unlock the three extra arenas and the fourth Revenge setting, **Hyper**. Returning from the cart after defeating an opponent inherits their equipment. Uses arena data already present in the regular ROM; no second ROM is needed. Included in v0.10.0; use Generate & rebuild to install its native code. [Controls and evidence](docs/GOLD_BATTLE.md). |
| **Pause — exit & draw** / **Gameplay** | Enable **Stage exits and Battle Draw**. Normal: Resume / Return to world map. Battle: Resume / Force Draw (round restart) / Return to stage select. Start confirms; B goes back. Menus start on Resume and exit confirmations on No. Compact panel, opaque text/glove and native menu sounds. Requires host v0.7.0. [Controls and validation](docs/PAUSE_MENU.md). v0.10.0 translates these labels with PT-BR and restores separate native pause/resume sounds. |
| **Normal Game — portal destinations** / **Gameplay** | Enable **Show portal destinations** for readable title-screen-font hints beneath each visible gate: `Z1-2`, `Z1-5`, `Z1-7`, special `Z1-S` and boss variants `Z1-A1`–`Z1-A3`. Covered exits remain hidden. Requires host v0.8.0; importing a manifest cannot update an older host. [Instructions and validation](docs/PORTAL_LABELS.md). |

![Normal Game portal destination hints](docs/images/portal-destinations.png)

ZONE1-1: left **Z1-2**, center **Z1-5**, right **Z1-7**. Hints appear only with each visible portal.

The launcher categories use English, including **Translation**. The PT-BR mod's name and descriptions remain in Brazilian Portuguese. All ten can be enabled together. Full campaign and multiplayer coverage remain unqualified.

v0.4.1 also embeds the supplied application icon with ten sizes from 16 to 256 pixels, preserving the whole drawing and transparency for Windows display scaling.

### Controls and starting a game

For a local source build, open `build-lle/SuperBomberman5SNESRecomp.exe`
directly; it includes the game icon. `play.cmd` opens that same executable
with the local ROM selected. The five-player menu recognizes the native
screen independently of script return history from the opening/demo.

| Keyboard | SNES input |
|---|---|
| Arrow keys | Direction |
| Enter | Start |
| X / Z | A / B |
| S / A | X / Y |
| C / V | L / R |
| Right Shift | Select |

Press Enter in the opening/title sequence. Choose **Normal Game → 1 Player Game → New Game → File 1**, confirming with A (keyboard X). On the Zone 1 map, use Start (Enter) to enter the stage. Controls can be changed in the launcher. The optional Normal co-op mod adds choices through five players; full campaign coverage remains unqualified. The launcher shows **PLAYER 1–5** tabs: set each player's input to Keyboard or Gamepad and press **Configure**. Players 3–5 start with no device and no keys; give each a gamepad or its own keys (sharing keys moves both players).

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
| `.agents/skills/super-bomberman5-mod-development/` | Skill for researching, implementing, testing and documenting native mods |
| `AGENTS.md`, `CLAUDE.md` | Guidance for AI agents: rules, architecture, RAM map, workflows |

### Credits and license

Thanks to Matthew Stanley and RetroPorting ToolKit contributors for SNESRecomp and recomp-ui, and to Mega Man X Recompiled for the architecture reference. Project software uses [PolyForm Noncommercial 1.0.0](LICENSE). Dependency notices are preserved; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). This license does not cover Super Bomberman 5 ROM or game content. The project is not affiliated with or endorsed by the game's rights holders.

## Português

### Sobre o projeto

Protótipo não oficial e não comercial de port de **Super Bomberman 5** para PC, usando SNESRecomp e o launcher recomp-ui, com a estrutura de repositório por jogo utilizada pelo Mega Man X Recompiled.

**A recompilação ainda é parcial:** a maior parte da lógica usa o interpretador. Abertura, título, menus, mapa da Zone 1 e entrada na primeira fase foram verificados numa rota de 3.628 frames. Isso não comprova o jogo completo ou precisão de som. A v0.5.0 trouxe o mod opcional de até cinco jogadores no Normal Game, a v0.6.0 o Subordinate Bomber e a v0.10.0 a Honey e o Kotetsu como personagens selecionáveis, todos testados com entradas roteirizadas; campanha completa e cinco controles físicos ainda não foram qualificados. Não há netplay nem melhorias de widescreen neste jogo.

### Como instalar e jogar

1. Baixe o ZIP Windows x64 em [Releases](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases) e extraia **todo o conteúdo** para uma pasta gravável.
2. Abra `SuperBomberman5SNESRecomp.exe` na raiz do pacote. Esse é o instalador, sem código recompilado do jogo.
3. Permita baixar as ferramentas oficiais e selecione sua própria ROM. A primeira instalação requer internet para obter as ferramentas.
4. Clique em **Generate & rebuild** para gerar, compilar e iniciar o jogo localmente. O analisador já está incluído; usuários da release não precisam instalar Rust.
5. Nas próximas execuções, abra o jogo em `build/` e ajuste os controles no launcher.

Requer Windows 10 ou superior, x64, e alguns GB livres. **Nenhuma ROM, código gerado da ROM ou recurso extraído acompanha o download.** O arquivo de desenvolvimento tem “USA” no nome, porém o cabeçalho indica **JPN**. Confira os hashes em [Supported ROM](#supported-rom); o nome do arquivo não garante compatibilidade.

Ao atualizar para a v0.10.0, extraia numa pasta nova e faça **Generate & rebuild** novamente com sua ROM; o executável gerado da versão anterior não recebe o novo código nativo por meio de um manifesto.

### AppImage Linux

Build CI Ubuntu 22.04 e geração/rebuild local no Ubuntu 24.04/WSL passaram; a rota de 3.628 frames LLE até a primeira fase terminou com vídeo/áudio dummy. Launcher inicializado em modo offscreen. Dispositivos físicos e outras distribuições seguem não qualificados.

Baixe **superbomberman5-0.10.0-linux-x86_64.AppImage** na [v0.10.0](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.10.0), dê permissão de execução e execute. Sem FUSE, use --appimage-extract-and-run. O setup é copiado para a pasta XDG de dados da versão (padrão ~/.local/share/superbomberman5/0.10.0), onde você fornece sua ROM e usa **Generate & rebuild**. Nas próximas aberturas, o AppImage usa o jogo gerado localmente. Requer Linux x86_64 com glibc 2.35+, bibliotecas de vídeo/áudio do desktop, internet para as primeiras ferramentas e alguns GB livres. Rust não é necessário. Preserve a pasta gravável para manter builds e configurações. Confira SHA256SUMS na release. Compatibilidade ampla e campanha completa continuam não qualificadas.

No Windows, baixe **superbomberman5-0.10.0-windows-x64.zip**. Para atualizar de uma versão anterior, extraia a v0.10.0 numa pasta nova (o AppImage cria a pasta 0.10.0) e use **Generate & rebuild** de novo para compilar os novos plugins nativos.

### Mods incluídos

Os **pacotes Windows e Linux v0.10.0** incluem os dez mods opcionais abaixo, todos na versão de mod **0.1.0** e desabilitados por padrão. Conclua **Generate & rebuild**, abra **Mods**, ative as funções desejadas e selecione Play. Cada pacote inclui executável, recursos do launcher, analisador, fontes e os dez mods pré-instalados; não é necessário baixar nem importar mods separados. O arquivo `FILE_SHA256SUMS` dentro do setup cobre os arquivos do pacote. Eles contêm apenas dados e dependem dos plugins nativos do host; importar um pacote não atualiza um executável antigo.

| Mod / categoria | Função e uso |
|---|---|
| **Português brasileiro — menus (prévia)** / **Translation** | Ative **Menus em português brasileiro** para traduzir rótulos de título, jogadores/arquivos, opções, teste de som, menus/regras de batalha e cabeçalhos de fases. Também traduz os cabeçalhos Continuar/Apagar, Fim de Jogo do Normal, HUD (APERTE START / FIM DE JOGO), recompensa com senha e os 33 títulos/dicas dos certificados. As fontes nativas e a ampliação do Battle exigem host v0.10.0 e Generate & rebuild. A v0.10.0 usa letras nativas do cartucho, preservando barras centrais e contornos superiores nos cabeçalhos, menus proporcionais e título, além de traduzir BATTLE START para INÍCIO DA BATALHA e os nomes das 13 arenas sem flashes do original nas transições verificadas. Outros textos não mapeados continuam originais. Desative para restaurar a apresentação original. [Escopo e testes](docs/TRANSLATION_PTBR.md). |
| **Normal Game — character selection** / **Gameplay** | Ative **Choose a character in Normal Game** para abrir o seletor original do Battle depois de confirmar um arquivo do Normal. Use direcional e A para escolher; B retorna aos arquivos. A campanha usa o personagem escolhido. Requer host v0.3.0 ou superior. [Instruções e validação](docs/NORMAL_CHARACTERS.md). |
| **Normal Game — infinite time** / **Gameplay** | Ative **Unlimited time in Normal Game** para usar o cronômetro nativo `-:--` sem morte por tempo. Desativado por padrão. Requer host v0.4.3 ou superior. [Detalhes e validação](docs/INFINITE_TIME.md). |
| **Character selection — color controls** / **Gameplay** | Ative **Choose character colors with L and R** para escolher branco, preto, vermelho, azul, verde e amarelo nos dez personagens. R avança, L retorna e A confirma. Início: P1 branco, P2 preto, P3 vermelho, P4 azul, P5 verde. O host v0.8.1 corrige a troca de paleta afetando o personagem abaixo. Funciona no Battle e no Normal com o mod de seleção de personagens ativo. Teclado padrão: C/V; controle: LB/RB (L1/R1). Requer host v0.4.0 ou superior. [Controles e validação](docs/SELECTION_COLORS.md). |
| **Normal Game — five players** / **Gameplay** | Ative **Play Normal Game with up to five players** e escolha de 1 a 5 jogadores no título. As vidas compartilhadas começam em jogadores + 2 (3–7); quem morre aperta Start para gastar uma vida e voltar, se houver alguém vivo. Qualquer jogador conclui a fase pelo portal, os itens seguem para a próxima fase em todos os assentos, P3–P5 nascem em chão seguro e tomam dano dos inimigos. Fases de chefe com rivais Bomber aceitam até quatro jogadores (três com dois rivais); os demais voltam na fase seguinte. Com os mods de personagem e cor, cada jogador escolhe os seus em New Game ou Continue. Requer host v0.5.0 ou superior. [Instruções, testes e limites](docs/NORMAL_COOP.md). |
| **Subordinate Bomber — playable character** / **Gameplay** | Ative **Play as Subordinate Bomber** para escolher o rival do ZONE3-D na célula do X/Bomberman amarelo secreto (quinta coluna inferior com Honey/Kotetsu) da seleção de personagens, no Battle e, com o mod de seleção no Normal, no Normal Game. Com o mod de cores, L/R escolhem as seis paletas dele que já existem no cartucho; o retrato e os rostos do HUD do Battle (vivo e derrotado) acompanham a cor escolhida. Requer host v0.6.0 ou superior. [Instruções, testes e limites](docs/SUBORDINATE_BOMBER.md). |
| **Honey and Kotetsu — playable characters** / **Gameplay** | Ative **Play as Honey and Kotetsu** para ter a seleção com seis colunas: as colunas originais ficam mais próximas, a Honey (a cowgirl chefe das fases secretas das zonas 1 a 3) fecha a fileira de cima e o Kotetsu a de baixo, ao lado do Subordinate Bomber/X. Eles jogam como o Bomberman (mesmas regras e itens) e usam as próprias poses de andar, ataque, vitória/empate, morte e montaria no Louie, retrato e rosto no HUD, tudo lido da sua ROM. Battle e, com o mod de seleção no Normal, Normal Game. Têm uma única paleta no cartucho (L/R não mudam a cor). Exige host v0.9.0. [Instruções, testes e limites](docs/HONEY_KOTETSU.md). |
| **Bomber Kings — Super Bomberman 4 characters** / **Gameplay** | Ative **Play as the Bomber Kings** e escolha **a sua própria ROM do Super Bomberman 4** (lançamento japonês, conferida pelo SHA-256) no item **Super Bomberman 4 ROM** do mod, na página Mods. A seleção ganha três fileiras: as originais sobem, a Honey e o Kotetsu fecham as duas e uma nova fileira traz o Bomberman, o Hammer, o Jet, a Lady, o Bazooka e o Great Bomber do Super Bomberman 4, desenhados a partir dessa ROM durante o jogo, com as cinco cores do SB4 (L/R com o mod de cores). Segure **B** e aperte uma **direção** para a habilidade de cada um (tiro da Honey, espadada do Kotetsu, martelada do Hammer, investida do Jet, disco da Lady, fogo do Bazooka, escudo do Great); Hammer, Jet, Great, Honey e Kotetsu ficam com metade da velocidade por cerca de 10,7 s, como no Super Bomberman 4. B sozinho continua detonando a bomba-controle; montado no Louie o B continua normal. As habilidades acertam inimigos no Normal Game (nunca outros jogadores) e os outros jogadores ou o time adversário no Battle. Exige host v0.11.0. [Instruções, testes e limites](docs/BOMBER_KINGS.md). |
| **Gold Cartridge — Battle** / **Gameplay** | Ative **Arenas 11–13 and Hyper Revenge** para liberar as três arenas extras e a quarta opção de Revanche, **Hiper** com PT-BR. Ao voltar à arena após eliminar alguém do carrinho, recebe os poderes do adversário. Já existe conteúdo das arenas na ROM comum; dispensa carregar a Gold. Incluído na v0.10.0; use Generate & rebuild para instalar o código nativo. [Controles e evidências](docs/GOLD_BATTLE.md). |
| **Pause — exit & draw** / **Gameplay** | Ative **Stage exits and Battle Draw**. Normal: retomar ou voltar ao mapa. Battle: retomar, empatar/reiniciar a rodada ou voltar às arenas. Start confirma; B volta. Com PT-BR: Continuar, Empatar rodada, Voltar ao mapa e Escolher arena; confirmação inicialmente em Não. Painel compacto, texto/mão opacos e sons nativos ao pausar e retomar. Exige host v0.7.0. [Controles e validação](docs/PAUSE_MENU.md). |
| **Normal Game — portal destinations** / **Gameplay** | Ative **Show portal destinations** para mostrar dicas legíveis com a fonte da tela de título sob cada portal visível: `Z1-2`, `Z1-5`, `Z1-7`, especial `Z1-S` e variantes `Z1-A1`–`Z1-A3`. Saídas cobertas continuam ocultas. Exige host v0.8.0; importar o manifesto não atualiza um executável antigo. [Instruções e validação](docs/PORTAL_LABELS.md). |

![Dicas de destino dos portais do Normal Game](docs/images/portal-destinations.png)

ZONE1-1: esquerda **Z1-2**, centro **Z1-5**, direita **Z1-7**. Cada dica aparece somente com o respectivo portal visível.

As categorias do launcher ficam em inglês, incluindo **Translation**. O nome e as descrições do mod PT-BR continuam em português brasileiro. Os dez podem ser ativados juntos. Campanha e multiplayer completos ainda não foram qualificados.

A v0.4.1 também incorpora o ícone fornecido em dez tamanhos, de 16 a 256 pixels, preservando o desenho inteiro e a transparência para escalas de exibição do Windows.

### Controles e início de partida

No build local de fonte, abra diretamente `build-lle/SuperBomberman5SNESRecomp.exe`,
que inclui o ícone do jogo. O `play.cmd` abre esse mesmo executável com a ROM
local selecionada. O menu de cinco jogadores reconhece a tela original sem
depender do histórico de retorno dos scripts da abertura/demonstração.

**Setas** para direção, **Enter** para Start, **X/Z** para A/B e **Shift direito** para Select. Pressione Enter na abertura/título, escolha **Normal Game → 1 Player Game → New Game → File 1**, confirme com **X** e pressione **Enter** no mapa da Zone 1 para entrar na fase. Remapeie os controles no launcher se desejar. O launcher mostra as abas **PLAYER 1–5**: escolha Teclado ou Gamepad para cada jogador e use **Configure**. P3–P5 começam sem dispositivo e sem teclas; dê a cada um um controle ou teclas próprias (teclas iguais movem os dois jogadores).

### Compilar, contribuir e publicar

Instale Git for Windows e Python 3.11+, clone com `--recurse-submodules` e execute os comandos de [Building from source](#building-from-source). Os scripts preparam ferramentas em `.tools/`, sem alterar o PATH global. Depois de compilar, `play.cmd` abre o launcher com a ROM local padrão. Consulte [BUILD_WINDOWS.md](docs/BUILD_WINDOWS.md) e [CONTRIBUTING.md](CONTRIBUTING.md).

A build validada usa composição LLE. Reset/NMI permanecem interpretados devido a falhas de continuidade na tentativa AOT. Precisamos ampliar a cobertura nativa e comparar o comportamento com uma referência independente.

[Abra uma issue](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/issues) informando versão, Windows, hash da ROM e reprodução. Não publique ROM, sprites/músicas extraídos, dumps ou save states. Confira dados pessoais nos relatórios antes de anexá-los.

O estado e as observações ficam em [STATUS.md](docs/STATUS.md); o fluxo de versões e releases está em [PUBLISHING.md](docs/PUBLISHING.md). A skill de manutenção fica em `.agents/skills/super-bomberman5-repository/` e a de desenvolvimento de mods em `.agents/skills/super-bomberman5-mod-development/`; orientações para agentes de IA estão em `AGENTS.md`. ROM, fontes gerados, builds e capturas ficam fora do Git. Releases contêm instalador e fontes sem dados do jogo.

O software usa [PolyForm Noncommercial 1.0.0](LICENSE), preservando as licenças das dependências. Veja [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). A licença não concede direitos sobre a ROM ou conteúdo de Super Bomberman 5. O projeto não tem vínculo ou endosso dos titulares do jogo.
