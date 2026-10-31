"""Generate initial prototype release notes; keep output outside tracked source."""
from pathlib import Path
import argparse
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path)
parser.add_argument('--version', help='Version for correcting an existing release without changing VERSION.')
parser.add_argument('--whats-new', action='store_true', help='Only what changed in this version (Discord #patch-notes).')
args = parser.parse_args()

version = args.version or (Path(__file__).resolve().parents[1] / 'VERSION').read_text().strip()
notes = f'''# Super Bomberman 5 v{version} — Windows prototype

Unofficial noncommercial prototype. **Partial recompilation:** most game logic still uses SNESRecomp's interpreter. The opening, title, menus, Zone 1 map and first-stage entry were verified locally over a scripted 3,628-frame route. Windows display/audio initialization and launcher smoke tests passed. Full-game accuracy, audio fidelity, battle mode and P2–P5 are not qualified; netplay and widescreen enhancements are not implemented.

Download the Windows x64 ZIP and extract the entire archive into a writable folder. Start the root SuperBomberman5SNESRecomp.exe, let the launcher obtain official build tools, select your own legally obtained ROM and use Generate & rebuild. The native analyzer is bundled; release users do not need Rust. The first tool download needs internet. Requires Windows 10+ x64 and several GB of disk space.

The release contains **a setup host and source, not the game**: no ROM, ROM-derived C, ROM-derived game executable, extracted sprites/music, saves or gameplay captures. Runtime licensed launcher assets and dependency notices are preserved. The playable build is generated locally under build/.

Supported image: Super Bomberman 5 (USA), CRC32 c25ab56a, SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4 (its header keeps the Japanese region byte). Hashes determine compatibility. No other revision is qualified.

Reset/NMI remain interpreted after AOT promotion produced continuation/dispatch errors. This is a baseline for improving native coverage, not a completed port. See README.md, docs/STATUS.md, CONTRIBUTING.md and THIRD_PARTY_NOTICES.md. Verify the ZIP using SHA256SUMS.

## Português

Protótipo não oficial e não comercial para Windows x64. **Recompilação parcial:** a maior parte da lógica continua no interpretador. Foram verificados abertura, título, menus, mapa da Zone 1 e entrada na primeira fase numa rota de 3.628 frames, além de janela, inicialização do áudio e launcher. Jogo completo, fidelidade sonora, batalha e P2–P5 não foram validados; não há netplay nem melhorias de widescreen.

Extraia todo o ZIP, abra SuperBomberman5SNESRecomp.exe na raiz, permita baixar as ferramentas oficiais, selecione sua própria ROM e use Generate & rebuild. O analisador já está incluído; o usuário da release não precisa instalar Rust. A primeira instalação exige internet, Windows 10+ x64 e alguns GB livres.

O pacote contém **instalador e fontes, não o jogo**. Não inclui ROM, fontes/executável derivados da ROM, sprites, música, saves ou capturas. O jogo é gerado e compilado localmente em build/. Confira SHA256SUMS, os hashes acima e as limitações em docs/STATUS.md.
'''
if tuple(int(n) for n in version.split('.')) >= (0,2,0):
    notes += '''\n## PT-BR menu mod / Mod de menus em português\n\nNew in v0.2.0: the included optional PT-BR mod v0.1.0 translates 62 background labels and a sprite heading, covering mapped title, player/file selection, options, sound test and battle setup/rules labels with authored glyphs. After Generate & rebuild, open Mods and enable Menus em português brasileiro. The separate .snesmod download contains the same mod; importing it is optional. Title font weight, shadows and accents were improved; JOGO NORMAL no longer clips when Start blinks. It is not a full-game translation; unmapped screens and gameplay/ending messages remain original. Presentation-only, disabled by default; see docs/TRANSLATION_PTBR.md.\n\nNovidade da v0.2.0: mod PT-BR v0.1.0 incluído, com 62 rótulos de background e um cabeçalho de sprites. Traduz rótulos de título, jogadores/arquivos, opções, teste de som e menus/regras de batalha com uma fonte própria. Depois de Generate & rebuild, ative Menus em português brasileiro em Mods. O .snesmod separado contém o mesmo mod; a importação é opcional. Fonte, sombra e acentos foram melhorados; JOGO NORMAL não fica cortado quando Start pisca. Não é tradução completa: telas não mapeadas e mensagens de fases/finais permanecem originais. Mod de apresentação, desabilitado por padrão; veja docs/TRANSLATION_PTBR.md.\n'''
if tuple(int(n) for n in version.split('.')) >= (0,3,0):
    notes += '''\n## Normal Game character selection\n\nNew in v0.3.0: enable **Choose a character in Normal Game** in Mods after Generate & rebuild. Confirming a Normal Game file opens the original Battle character picker; use the D-pad and A to choose, or B to return to file selection. The campaign resumes with the selected character and palette. The mod is optional, disabled by default, and included in the setup pack alongside PT-BR menus. Mod names and descriptions use English; PT-BR translation information remains Portuguese. The separate normal-characters .snesmod requires this updated host and cannot add the native plugin to older executables.\n\nThe second and ninth characters were observed in Zone 1-1; movement/bomb input and the PT-BR overlay were exercised with the ninth character. Selection, campaign restoration, both-player confirmation, bounded snapshot loading and stable rewind snapshot size have focused tests. Full campaign, cooperative stages, deaths, bosses/endings and a populated Continue file remain unqualified. No ROM bytes are patched, and battery SRAM keeps the stock format. See docs/NORMAL_CHARACTERS.md.\n\n## Seleção de personagens no Normal Game\n\nNovidade da v0.3.0: depois de Generate & rebuild, ative **Choose a character in Normal Game** em Mods. Ao confirmar o arquivo, aparece a seleção original do Battle. Use o direcional e A para escolher; B volta aos arquivos. A campanha continua com os sprites e a paleta escolhidos. O mod é opcional, desabilitado por padrão e incluído junto com os menus PT-BR. Os nomes e descrições dos mods usam inglês, preservando as informações de tradução PT-BR em português. O .snesmod separado exige este host atualizado.\n\nO segundo e o nono personagens foram observados na Zone 1-1; o nono foi testado com movimento/bomba e tradução PT-BR. Há testes de seleção, restauração da campanha, confirmação de dois jogadores, limites de snapshots e tamanho estável para rewind. Campanha completa, fases cooperativas, mortes, bosses/finais e Continue com arquivo preenchido ainda não foram qualificados. A ROM não é modificada e o SRAM mantém o formato original.\n'''
if tuple(int(n) for n in version.split('.')) >= (0,4,0):
    notes = notes.replace('battle mode and P2–P5 are not qualified', 'full battle coverage and P2–P5 are not qualified').replace('batalha e P2–P5 não foram validados', 'cobertura completa de batalha e P2–P5 não foram validadas')
    notes += '''\n## Character selection color controls — new in v0.4.0\n\nEnable **Choose character colors with L and R** in Mods after Generate & rebuild. R cycles white, black, red, blue, green, yellow; L cycles backward. All nine playable characters support these six palettes. Cursors display L + R above and the player number below, retaining native font colors. Confirm with A to carry the color into stages. Supported in Battle and, with **Choose a character in Normal Game** enabled, the Normal picker. PT-BR menus remain compatible. Disabled by default.\n\nFocused native tests cover all nine characters, six palettes, input edges, independent seats, confirmation, snapshots and cursor restoration. Runtime routes verified the six-color preview and the second character in green during Normal and Battle stages with PT-BR enabled. Full multiplayer and campaign coverage remain unqualified. The demo is a Windows setup pack: provide your own supported ROM and generate the playable build locally. The separate selection-colors .snesmod is data-only and requires the v0.4.0 host; importing it into an older host does not install the native plugin.\n\n## Cores na seleção — novidade da v0.4.0\n\nAtive **Choose character colors with L and R** em Mods depois de Generate & rebuild. R percorre branco, preto, vermelho, azul, verde e amarelo; L percorre o sentido contrário. Os nove personagens suportam essas seis paletas. O cursor mostra L + R acima e o jogador abaixo, preservando as cores da fonte original. Confirme com A para usar a cor nas fases. Funciona no Battle e na seleção do Normal com **Choose a character in Normal Game** ativado. Compatível com os menus PT-BR; desabilitado por padrão.\n\nTestes nativos cobrem os nove personagens, seis paletas, entradas, jogadores independentes, confirmação, snapshots e restauração do cursor. Rotas locais validaram as seis cores na seleção e o segundo personagem verde em fases do Normal e Battle com PT-BR. Multiplayer e campanha completos continuam pendentes. A demo é um pacote de instalação Windows: forneça sua própria ROM compatível e gere o jogo localmente. O .snesmod separado contém apenas dados e exige o host v0.4.0.\n'''
if tuple(int(n) for n in version.split('.')) >= (0,4,0):
    notes += '''\nThe original picker lets humans use their own controllers, and take over pending CPU selections after confirming. Color changes follow the native active-cursor/controller tables and preserve confirmed choices. The confirmation counter is also handled correctly for two-player Normal selection. Default keyboard: C = L, V = R; default gamepad: LB/RB (L1/R1).\n\nNo seletor original, humanos usam seus próprios controles e passam a escolher CPUs pendentes depois de confirmar. A troca de cor acompanha as tabelas originais de cursor/controle e preserva as escolhas confirmadas. O contador de confirmação também é tratado corretamente no Normal de dois jogadores. Teclado padrão: C = L, V = R; controle padrão: LB/RB (L1/R1).\n'''
if tuple(int(n) for n in version.split('.')) >= (0,4,1):
    notes = notes.replace('# Super Bomberman 5 v' + version + ' — Windows prototype', '# Super Bomberman 5 v' + version + ' — Windows prototype\n\n## New in v0.4.1 / Novidades da v0.4.1\n\nThe Windows setup host and locally rebuilt game embed the supplied application icon in ten sizes (16–256 pixels), scaled to fill the available space without clipping or losing transparency. The Mods category is now **Translation** in English; the PT-BR mod keeps its Portuguese name and descriptions. The bilingual README now documents all three included optional mods: PT-BR menus, Normal Game character selection and character color controls. All remain at mod version 0.1.0 and disabled by default.\n\nO setup host Windows e o jogo recompilado localmente incorporam o ícone fornecido em dez tamanhos (16–256 pixels), preenchendo o espaço sem cortes e preservando a transparência. A categoria de Mods passa a **Translation**, em inglês; o mod PT-BR mantém nome e descrições em português. O README bilíngue descreve os três mods opcionais incluídos: menus PT-BR, seleção de personagens no Normal Game e controles de cores. Todos permanecem na versão de mod 0.1.0 e desabilitados por padrão.')
if tuple(int(n) for n in version.split('.')) >= (0,4,2):
    notes = f'''# Super Bomberman 5 v{version} — Windows setup bundle

## English

Download **superbomberman5-{version}-windows-x64.zip**. This single Windows download includes the setup executable with its scaled icon, launcher resources, native analyzer and required DLL/license, source and documentation, and all three preinstalled optional mods:

- **Português brasileiro — menus (prévia)**: partial Brazilian Portuguese menu translation, under **Translation**. The mod's labels remain Portuguese.
- **Normal Game — character selection**: choose a campaign character using the original Battle picker after confirming a file.
- **Character selection — color controls**: L/R cycles six colors for all nine characters in Battle and the optional Normal picker.

All mods remain at version 0.1.0 and disabled by default. No separate mod download or import is needed. The ZIP includes **FILE_SHA256SUMS**, covering its contents; GitHub also provides the uploaded ZIP's SHA-256 digest.

Extract the entire ZIP into a writable folder, open **SuperBomberman5SNESRecomp.exe**, obtain the official portable build tools, select your own supported ROM and choose **Generate & rebuild**. Configure Mods and select Play in the rebuilt launcher. Requires Windows 10+ x64, several GB free and internet for the first tool download. Rust is not required.

Like the reference Mega Man X Windows release, application resources and mods are bundled together. This prototype still generates the playable executable locally. No ROM, generated game C, ROM-derived game executable, extracted game graphics/audio, saves or gameplay captures are distributed. Partial recompilation: most logic uses the interpreter; full campaign and multiplayer remain unqualified. Linux/macOS releases are not provided.

Supported ROM: JPN header; CRC32 c25ab56a; SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4. See README.md, docs/STATUS.md and THIRD_PARTY_NOTICES.md.

## Português

Baixe **superbomberman5-{version}-windows-x64.zip**. O único download Windows reúne o executável de instalação com ícone corrigido, recursos do launcher, analisador nativo e DLL/licença, fontes, documentação e os três mods opcionais pré-instalados: **menus PT-BR**, **seleção de personagens no Normal Game** e **controles de cores com L/R**.

Os mods permanecem na versão 0.1.0, desligados por padrão. A categoria é **Translation**, mantendo os textos do mod PT-BR em português. Não é necessário baixar nem importar mods separados. O ZIP inclui **FILE_SHA256SUMS** para conferir seus arquivos; o GitHub também informa o SHA-256 do ZIP publicado.

Extraia todo o ZIP, abra **SuperBomberman5SNESRecomp.exe**, obtenha as ferramentas oficiais, selecione sua própria ROM compatível e use **Generate & rebuild**. No launcher recompilado, configure Mods e selecione Play. Requer Windows 10+ x64, alguns GB livres e internet para baixar ferramentas na primeira instalação; Rust não é necessário.

Os recursos e mods ficam juntos, como no pacote Windows de referência do Mega Man X. Este protótipo ainda gera o executável jogável localmente. Não distribuímos ROM, C gerado, executável derivado da ROM, gráficos/áudio extraídos, saves ou capturas. A recompilação continua parcial, com a maior parte da lógica no interpretador; campanha e multiplayer completos não foram qualificados. Sem releases Linux/macOS.
'''
if tuple(int(n) for n in version.split('.')) >= (0,4,3):
    notes = f'''# Super Bomberman 5 v{version} — Windows setup bundle

New optional mod: **Normal Game — infinite time**. After Generate & rebuild, enable **Unlimited time in Normal Game** in **Mods → Gameplay**. Campaign stages show the native `-:--` boss timer and no longer cause timeout deaths. Enemies and bombs still kill players. Battle remains unchanged.

All four optional mods are preinstalled and disabled by default: PT-BR menus, Normal character selection, L/R color controls and infinite time. Their package version remains 0.1.0; infinite time requires native host v0.4.3 or newer. Download and extract the complete Windows x64 ZIP; supply your own supported ROM and choose Generate & rebuild. Windows 10+ x64, several GB free and internet for the initial tool download are required.

Windows/LLE Zone 1-1 tests exercise unlimited time, a forced 0:01 deadline, PT-BR and character/color selection; disabled control preserves countdown and native expiration. Native and Python tests and publication audits pass. Full campaign, real boss encounters, rewind with this mod and physical controls remain unqualified. Partial recompilation: most game logic uses the interpreter. No netplay or Linux/macOS release.

ROM identity: JPN header; CRC32 c25ab56a; SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4. The ZIP contains the ROM-free setup host, analyzer, sources, launcher resources and licenses. It contains no ROM, generated game C, playable ROM-derived executable, extracted graphics/audio, saves or captures. FILE_SHA256SUMS covers packaged files; GitHub supplies the ZIP digest.

## Português

Novo mod opcional: **Normal Game — infinite time**. Depois de Generate & rebuild, ative **Unlimited time in Normal Game** em **Mods → Gameplay**. As fases Normal usam o relógio nativo `-:--`, como nos chefes, sem morte por tempo esgotado. Bombas e inimigos ainda matam; Battle mantém seu comportamento.

O ZIP Windows x64 inclui os quatro mods opcionais, desativados por padrão: menus PT-BR, personagens no Normal, cores com L/R e tempo infinito. Os pacotes continuam em 0.1.0; tempo infinito exige host v0.4.3 ou superior. Extraia o ZIP completo, forneça sua ROM compatível e use Generate & rebuild. Requer Windows 10+ x64, alguns GB livres e internet no primeiro download das ferramentas.

Rotas Windows/LLE na Zone 1-1 verificam tempo ilimitado, tentativa de forçar 0:01, PT-BR e personagens/cores; com o mod desligado, a contagem e a expiração originais permanecem. Testes nativos/Python e auditorias passaram. Campanha completa, bosses reais, rewind com este mod e controles físicos seguem não qualificados. Recompilação parcial; sem release Linux/macOS ou netplay.

Pacote sem ROM ou executável derivado dela: contém instalador, analisador, fontes, recursos do launcher e licenças. FILE_SHA256SUMS verifica o conteúdo; GitHub mostra o digest do ZIP.
'''

if tuple(int(n) for n in version.split('.')) >= (0,4,4):
    notes = f'''# Super Bomberman 5 v{version} — Linux AppImage + Windows ZIP

## English

New: experimental Linux x86_64 AppImage alongside the Windows x64 ZIP. Both are ROM-independent setup packs with the native analyzer, sources, notices and four optional mods (PT-BR menus, Normal character selection, L/R colors and infinite Normal timer), all disabled by default. No ROM, generated game code, playable ROM-derived binary, extracted game assets or saves are distributed.

Windows: extract the entire ZIP and open SuperBomberman5SNESRecomp.exe. Linux: make the AppImage executable and run it; without FUSE, use --appimage-extract-and-run. First launch copies setup into $XDG_DATA_HOME/superbomberman5/{version} (default ~/.local/share). Provide your supported ROM and use Generate & rebuild. Later AppImage launches use the locally rebuilt game. Requires internet for the first portable tools download, several GB free, Windows 10+ x64 or Linux x86_64 with glibc 2.35+ and desktop graphics/audio libraries. Rust is unnecessary. SHA256SUMS covers both downloads.

Windows and Linux setup builds/tests passed. Extracted packages generated and rebuilt locally; both completed the 3,628-frame LLE first-stage route with dummy video/audio and mods disabled. Linux local check used Ubuntu 24.04/WSL and extract-and-run; FUSE and physical devices were not tested.

Partial recompilation with LLE: most game logic remains interpreted. Full campaign, full multiplayer and broad Linux distribution compatibility remain unqualified. See README.md and docs/STATUS.md for concrete package/runtime checks. This release retains the v0.4.3 feature set; local uncommitted co-op work is not included.

Supported ROM: JPN header, CRC32 c25ab56a, SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4.

## Português

Novidade: AppImage Linux x86_64 experimental junto ao ZIP Windows x64. Ambos incluem setup sem ROM, analisador nativo, fontes, licenças e quatro mods opcionais: menus PT-BR, personagens no Normal, cores com L/R e tempo infinito. Todos desligados por padrão. Não distribuímos ROM, código gerado, executável jogável derivado da ROM, recursos extraídos ou saves.

Windows: extraia todo o ZIP e abra SuperBomberman5SNESRecomp.exe. Linux: dê permissão de execução ao AppImage e execute; sem FUSE, use --appimage-extract-and-run. Na primeira abertura, o setup é copiado para $XDG_DATA_HOME/superbomberman5/{version} (padrão ~/.local/share). Forneça sua ROM compatível e use Generate & rebuild. Nas próximas aberturas, o AppImage usa o jogo recompilado localmente. Requer internet no primeiro download de ferramentas, alguns GB livres, Windows 10+ x64 ou Linux x86_64 com glibc 2.35+ e bibliotecas de vídeo/áudio do desktop. Rust não é necessário. SHA256SUMS cobre os dois downloads.

Builds/testes Windows e Linux passaram. Pacotes extraídos geraram e compilaram localmente; ambos concluíram a rota de 3.628 frames LLE até a primeira fase, com vídeo/áudio dummy e mods desligados. Linux validado no Ubuntu 24.04/WSL via extract-and-run; FUSE e dispositivos físicos não testados.

Recompilação parcial com LLE; a maior parte da lógica permanece interpretada. Campanha completa, multiplayer completo e compatibilidade ampla de distribuições Linux continuam não qualificados. README.md e docs/STATUS.md registram as verificações concretas. Mantém as funcionalidades da v0.4.3; alterações locais ainda não commitadas do cooperativo não estão incluídas.
'''
if tuple(int(n) for n in version.split('.')) >= (0,5,0):
    notes = f'''# Super Bomberman 5 v{version} — five-player Normal Game demo

## English

New optional mod: **Normal Game — five players**. After Generate & rebuild, enable **Play Normal Game with up to five players** in **Mods → Gameplay**. The title menu offers 1–5 PLAYER GAME (native colors, blinking Start, cursor sound). Players 3–5 use the multitap on port 2; the launcher now shows **PLAYER 1–5** tabs, and seats 3–5 start with no device, so assign each a gamepad or its own keys.

- Shared lives start at players + 2 (3–7). A dead player presses Start to spend one life and rejoin while a teammate is alive.
- Every player can finish a stage through an exit; bombs, fire, speed and other items carry over for all seats.
- P3–P5 spawn on safe floor, including smaller arenas, take enemy contact damage and animate with their own sprite pages and palettes.
- Boss stages with rival bombers seat up to four players (three when there are two rivals): the game has five bomber slots. Seats left out return in the next stage with their character, colors and items.
- With **Choose a character in Normal Game** and **Choose character colors with L and R**, every seat picks its own character and color, from New Game and from Continue.

All five optional mods are preinstalled and disabled by default: PT-BR menus, Normal character selection, L/R colors, infinite Normal timer and five-player Normal Game. Mod packages remain 0.1.0; the five-player mod requires host v0.5.0 or newer. Download the Windows x64 ZIP or the experimental Linux x86_64 AppImage, supply your own supported ROM and use Generate & rebuild.

Validation: native tests for all mods, Python publication tests and audits; Linux LLE scripted runs with five players in Zone 1-1, exits to the next stage with items, Zone 1-A and the two-rival boss stage from Continue, Zone 2-4 and Zone 3-D/E spawns, enemy contact on P3, Start rejoin, Game Over and Continue. Not qualified: defeating each boss, the full campaign, boss choreography with benched seats, five physical controllers. Back (B) from the Normal character picker can leave a black screen; this predates v0.5.0 and remains open. Partial recompilation: most game logic still runs in the interpreter.

Supported ROM: JPN header; CRC32 c25ab56a; SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4. The downloads contain no ROM, generated game code, playable ROM-derived executable, extracted graphics/audio, saves or captures. SHA256SUMS covers both downloads.

## Português

Novo mod opcional: **Normal Game — five players**. Depois de Generate & rebuild, ative **Play Normal Game with up to five players** em **Mods → Gameplay**. O título oferece de 1 a 5 jogadores, com as cores originais, Start piscando e som do cursor. P3–P5 usam o multitap da porta 2; o launcher mostra as abas **PLAYER 1–5**, e P3–P5 começam sem dispositivo: configure um controle ou teclas próprias para cada um.

- Vidas compartilhadas começam em jogadores + 2 (3–7). Quem morre aperta Start para gastar uma vida e voltar, desde que haja alguém vivo.
- Qualquer jogador conclui a fase pelo portal; bombas, fogo, velocidade e demais itens seguem para a próxima fase em todos os assentos.
- P3–P5 nascem em chão seguro, inclusive em arenas menores, tomam dano ao encostar em inimigos e têm animação e paletas próprias.
- Fases de chefe com rivais Bomber aceitam até quatro jogadores (três com dois rivais), pois o jogo tem cinco vagas de bomber. Quem fica de fora volta na fase seguinte com personagem, cores e itens.
- Com **Choose a character in Normal Game** e **Choose character colors with L and R**, cada jogador escolhe personagem e cor, em New Game e em Continue.

Os cinco mods opcionais vêm pré-instalados e desativados: menus PT-BR, personagens no Normal, cores com L/R, tempo infinito e Normal Game com cinco jogadores. Os pacotes continuam em 0.1.0; o de cinco jogadores exige host v0.5.0 ou superior. Baixe o ZIP Windows x64 ou o AppImage Linux x86_64 experimental, forneça sua ROM compatível e use Generate & rebuild.

Validação: testes nativos de todos os mods, testes Python e auditorias; rotas Linux LLE com cinco jogadores na Zone 1-1, portal para a próxima fase com itens, Zone 1-A e a fase com dois rivais via Continue, spawns na Zone 2-4 e Zone 3-D/E, dano de inimigo no P3, retorno com Start, Game Over e Continue. Não qualificados: derrotar cada chefe, campanha completa, cenas dos chefes com assentos de fora e cinco controles físicos. Voltar com B na seleção de personagens do Normal pode deixar a tela preta; isso já existia e continua aberto. Recompilação parcial: a maior parte da lógica ainda usa o interpretador.
'''
if tuple(int(n) for n in version.split('.')) >= (0,6,0):
    notes = f'''# Super Bomberman 5 v{version} — Subordinate Bomber playable

## English

New optional mod: **Subordinate Bomber — playable character**. After Generate & rebuild, enable **Play as Subordinate Bomber** in **Mods → Gameplay**. The Zone 3-D rival takes the X cell (bottom right) of the character selection in Battle Mode and, with **Choose a character in Normal Game**, in Normal Game after choosing a save.

- Gameplay, animations, score board and its six palettes (white, black, red, blue, green, yellow) come from your cartridge, where it is character index 9. With **Choose character colors with L and R**, L/R cycle those six colors like the other characters.
- The cartridge has no grid figure, portrait or HUD face for it: the grid figure and alive HUD face are drawn at render time from its own sprite tiles; the seat portraits (one per color) and the defeated HUD face are maintainer-authored art that follows the selected color.
- In-memory cartridge patches only, restored when the mod is disabled; the ROM file and SRAM layout are untouched. If your save already unlocked the secret character of the X cell, that cell keeps it.

All six optional mods are preinstalled and disabled by default: PT-BR menus, Normal character selection, L/R colors, infinite Normal timer, five-player Normal Game and Subordinate Bomber. Mod packages remain 0.1.0; Subordinate Bomber requires host v0.6.0 or newer. Download **superbomberman5-{version}-windows-x64.zip** (extract all, run SuperBomberman5SNESRecomp.exe) or **superbomberman5-{version}-linux-x86_64.AppImage** (chmod +x and run; without FUSE use --appimage-extract-and-run), supply your own ROM and use **Generate & rebuild**. Upgrading: use a new folder and rebuild so the new native plugin is compiled.

Validation: six native test executables (new subordinate_tests) and Python publication tests; Linux LLE scripted runs: Battle picker with colors and PT-BR, five-seat Battle through the score board, several seats on Subordinate Bomber next to Bomberman, defeated HUD faces, Normal Zone 1-1 with five players and Zone 3-D beside the rival Subordinate Bomber. Not qualified: physical controllers, full campaign, unlocked secret slot. Partial recompilation: most game logic remains interpreted.

Supported ROM: JPN header; CRC32 c25ab56a; SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4. The downloads contain no ROM, generated game code, playable ROM-derived executable, extracted graphics/audio, saves or captures. SHA256SUMS covers both packages.

## Português

Novo mod opcional: **Subordinate Bomber — playable character**. Depois de Generate & rebuild, ative **Play as Subordinate Bomber** em **Mods → Gameplay**. O rival do ZONE3-D ocupa a célula do X (canto inferior direito) da seleção de personagens no Battle Mode e, com **Choose a character in Normal Game**, no Normal Game, depois de escolher o save.

- Jogabilidade, animações, placar e as seis paletas dele (branco, preto, vermelho, azul, verde, amarelo) vêm do seu cartucho, onde ele é o personagem índice 9. Com **Choose character colors with L and R**, L/R alternam essas seis cores como nos outros personagens.
- O cartucho não tem figura na grade, retrato nem rosto de HUD para ele: a figura e o rosto vivo são desenhados a partir do próprio sprite dele; os retratos (um por cor) e o rosto triste de derrotado são arte do mantenedor e acompanham a cor escolhida.
- Somente patches em memória, desfeitos ao desativar o mod; a ROM e o SRAM não mudam. Se o seu save já desbloqueou o personagem secreto da célula do X, a célula continua com ele.

Os seis mods opcionais vêm pré-instalados e desativados: menus PT-BR, personagens no Normal, cores com L/R, tempo infinito, Normal com cinco jogadores e Subordinate Bomber. Os pacotes de mod seguem em 0.1.0; o Subordinate Bomber exige host v0.6.0 ou superior. Baixe o ZIP Windows (extraia tudo e abra SuperBomberman5SNESRecomp.exe) ou o AppImage Linux (chmod +x; sem FUSE use --appimage-extract-and-run), forneça sua ROM e use **Generate & rebuild**. Para atualizar, use uma pasta nova e recompile.

Validação: seis executáveis de teste nativos (novo subordinate_tests) e testes Python de publicação; rotas roteirizadas Linux LLE: seleção do Battle com cores e PT-BR, Battle com cinco assentos até o placar, vários assentos no Subordinate ao lado do Bomberman, rostos de derrotado no HUD, Normal na Zone 1-1 com cinco jogadores e ZONE3-D ao lado do Subordinate rival. Não qualificados: controles físicos, campanha completa, célula secreta desbloqueada. Recompilação parcial: a maior parte da lógica continua interpretada.

ROM suportada: cabeçalho JPN; CRC32 c25ab56a; SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4. Os downloads não contêm ROM, código gerado, executável derivado da ROM, gráficos/áudio extraídos, saves ou capturas. SHA256SUMS cobre os dois pacotes.
'''

if tuple(int(n) for n in version.split('.')) >= (0,7,0):
    notes = f'''# Super Bomberman 5 v{version} — Pause — exit & draw

## English

New optional mod **Pause — exit & draw**: enable **Stage exits and Battle Draw** under Mods → Gameplay after Generate & rebuild.

- Normal Game: Resume or Return to world map, preserving characters, colors, equipment and shared lives without awarding a cleared stage.
- Battle: Resume, Force Draw (native draw result and round restart), or Return to stage select through the native arena-selection flow.
- Start accepts the highlighted option; B goes back. The initial selection is Resume; exit confirmation starts on No. Held buttons do not repeat actions.
- Compact pause panel keeps gameplay visible, with fully opaque PAUSE/ZONE headings, text and pointing glove. Move/accept/back use native Battle menu sounds.

The bilingual README documents all seven optional mods: PT-BR menus, Normal character selection, L/R colors, infinite Normal time, five-player Normal Game, Subordinate Bomber and Pause — exit & draw. All mod packages remain 0.1.0 and default-disabled. The new pause plugin requires host v0.7.0; importing its data-only manifest cannot update an older host.

Download **superbomberman5-{version}-windows-x64.zip** or **superbomberman5-{version}-linux-x86_64.AppImage**, plus **SHA256SUMS**. Windows: extract the entire ZIP into a writable folder and launch SuperBomberman5SNESRecomp.exe. Linux: chmod +x the AppImage and run it; without FUSE use --appimage-extract-and-run. AppRun creates a versioned writable setup directory. Supply your own compatible ROM and choose Generate & rebuild. For upgrades, use the new version folder and rebuild the game. The native analyzer is included; Rust is not required. First tool download needs internet.

Windows x64/LLE runtime evidence: Normal 1P, 2P with PT-BR/other mods, 5P co-op and Battle 2 humans/3 CPUs; map return/reentry, Draw/restart, arena return/reentry, safe defaults, Start/B, SFX and opaque glyph/glove pixels. Normal 1P was also checked with the accurate PPU. Seven native test executables cover the mods. Linux packaging/native tests are checked by CI; this does not establish full-campaign Linux accuracy. Physical controllers, all arenas, full campaign and rewind across transitions remain unqualified. Partial recompilation: most game logic remains interpreted.

ROM-independent setup downloads contain source, launcher resources, analyzer, licenses, mod manifests and documentation; no ROM, generated game code, ROM-derived playable executable, extracted sprites/audio, saves or raw dumps. Supported ROM: JPN header; CRC32 c25ab56a; SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4.

## Português

Novo mod opcional **Pause — exit & draw**: depois de Generate & rebuild, ative **Stage exits and Battle Draw** em Mods → Gameplay.

- Normal: retomar ou voltar ao mapa, preservando personagens, cores, itens e vidas compartilhadas, sem registrar a fase como concluída.
- Battle: retomar, forçar empate e reiniciar a rodada pelo fluxo nativo, ou retornar à seleção de arenas.
- Start confirma; B volta. O cursor começa em Resume e a confirmação de saída em No. Segurar o botão não repete a ação.
- Painel compacto com a fase visível; PAUSE/ZONE, opções e mão são opacos. Sons de mover, confirmar e voltar vêm do menu Battle.

O README bilíngue documenta os sete mods: menus PT-BR, personagens no Normal, cores com L/R, tempo infinito, Normal com cinco jogadores, Subordinate Bomber e pausa. Todos seguem em 0.1.0, desativados por padrão. O mod de pausa exige host v0.7.0; importar seu manifesto não atualiza executáveis antigos.

Baixe o ZIP Windows ou AppImage Linux e confira SHA256SUMS. No Windows, extraia tudo numa pasta gravável e abra SuperBomberman5SNESRecomp.exe. No Linux, use chmod +x; sem FUSE, --appimage-extract-and-run. Forneça sua ROM e use Generate & rebuild. Para atualizar, use a pasta da nova versão e gere o jogo novamente. O analisador está incluído; Rust não é necessário. O primeiro download das ferramentas exige internet.

Rotas Windows/LLE validaram Normal com 1/2/5 jogadores e Battle com 2 humanos/3 CPUs, retorno ao mapa, empate/reinício, retorno às arenas, reentrada, controles, sons e pixels opacos do texto/cursor. Normal 1P também passou no PPU preciso. Há sete executáveis de teste nativos. CI verifica build/pacote e testes Linux; isso não comprova a campanha inteira. Controles físicos, todas as arenas, campanha completa e rewind nas transições continuam não qualificados. A recompilação permanece parcial.

Os pacotes contêm instalador independente da ROM, fontes, analisador, licenças, manifestos e documentação. Não incluem ROM, código gerado, executável jogável derivado da ROM, sprites/áudio extraídos, saves ou dumps. Confira a identidade da ROM acima e docs/STATUS.md.
'''

if tuple(int(n) for n in version.split('.')) >= (0,8,0):
    notes = f'''# Super Bomberman 5 v{version} — Normal portal destinations

## English

New optional Gameplay mod: Normal Game — portal destinations → Show portal destinations.
Title-screen-font hints appear below each visible portal, with Z1-2 style stage names,
special Z1-S names and boss variants such as Z1-A1/Z1-A2/Z1-A3. Covered portals
remain undisclosed. The presentation plugin reads native exit scripts and changes
no campaign state. It is disabled by default and requires rebuilding this host;
the published v0.7.0 executable does not contain the plugin.

Local data audit covers all 192 native exits. Windows/LLE fixture checks cover
hidden/revealed exits, individual visibility, pause and native-state restoration,
including both PPUs and PT-BR/pause integration. This is not full-campaign or
natural-combat qualification. See docs/PORTAL_LABELS.md and docs/STATUS.md.
All eight optional mods are preinstalled at package version 0.1.0, disabled by
default. Previous character, color, co-op, translation, infinite-time, Subordinate
Bomber and pause features remain included. The README includes the supplied
gameplay screenshot in both languages.

Download superbomberman5-{version}-windows-x64.zip or
superbomberman5-{version}-linux-x86_64.AppImage and verify SHA256SUMS.
Extract Windows into a new writable folder; on Linux chmod +x and run the
AppImage (without FUSE use --appimage-extract-and-run). Supply your own
compatible JPN ROM and choose Generate & rebuild. Rust is not required;
internet is needed for the initial tool download. Linux remains experimental.
No ROM, generated code, extracted assets, saves or raw dumps are packaged.

## Português

Novo mod opcional em Gameplay: Normal Game — portal destinations → Show portal destinations.
Dicas com a fonte da tela de título aparecem sob cada portal visível, no padrão Z1-2,
especiais Z1-S e variantes de chefe Z1-A1/Z1-A2/Z1-A3. Portais cobertos
continuam ocultos. O plugin de apresentação lê os scripts nativos sem alterar
a campanha. Vem desativado e exige recompilar este host; o executável v0.7.0
publicado não contém o plugin.

Auditoria local dos 192 destinos nativos. Fixture Windows/LLE cobre saídas
ocultas/reveladas, visibilidade individual, pausa e restauração do estado nativo,
incluindo os dois PPUs e integração PT-BR/pausa. Não equivale a qualificar
campanha inteira ou combate natural. Veja docs/PORTAL_LABELS.md e docs/STATUS.md.
Os oito mods opcionais vêm pré-instalados na versão de pacote 0.1.0, desligados
por padrão. Personagens, cores, co-op, tradução, tempo infinito, Subordinate
Bomber e pausa continuam incluídos. O README inclui a captura fornecida pelo
mantenedor na explicação do mod nos dois idiomas.

Baixe superbomberman5-{version}-windows-x64.zip ou
superbomberman5-{version}-linux-x86_64.AppImage e confira SHA256SUMS.
No Windows, extraia tudo numa pasta nova e gravável; no Linux, use chmod +x e
execute o AppImage (sem FUSE use --appimage-extract-and-run). Forneça sua
ROM JPN compatível e escolha Generate & rebuild. Rust não é necessário;
o primeiro download das ferramentas exige internet. Linux segue experimental.
Nenhuma ROM, código gerado, assets extraídos, saves ou dumps brutos são empacotados.
'''

if tuple(int(n) for n in version.split('.')) >= (0,8,1):
    notes = f'''# Super Bomberman 5 v{version} — Character selection color fixes

## English

- Initial colors follow the current seat: P1 white, P2 black, P3 red, P4 blue,
  P5 green, including controller handoff to CPU seats.
- Fixed palette changes in the top row brightening the character below,
  especially white on the second and fourth characters. The shared BG palette
  entry is preserved; L/R cycling and confirmed colors remain available.

The maintainer confirmed the correction. Windows/LLE Normal (3,636 frames)
and Battle (4,750 frames) fixtures with PT-BR exercise six colors across five
columns and verify initial seat colors and unchanged lower palette strips.
Full campaign and physical five-controller play remain unqualified.

Download **superbomberman5-{version}-windows-x64.zip** or experimental
**superbomberman5-{version}-linux-x86_64.AppImage**; verify **SHA256SUMS**.
Extract Windows into a new writable folder. On Linux, chmod +x and run the
AppImage (without FUSE use --appimage-extract-and-run). Supply your own
compatible JPN ROM and choose **Generate & rebuild**, including when upgrading:
existing generated games need recompiling to receive the fix. Rust is not
required; internet is needed for the initial portable tool download.

All eight existing optional mods are included, disabled by default, at package
version 0.1.0. No ROM, generated game code, playable ROM-derived executable,
extracted assets, saves or raw dumps are distributed. Partial recompilation:
most logic uses the interpreter. See README.md and docs/STATUS.md for evidence
and platform limits. No netplay or macOS package.

## Português

- Cores iniciais por assento: P1 branco, P2 preto, P3 vermelho, P4 azul e P5
  verde, inclusive quando o controle passa a selecionar um CPU.
- Corrigida a troca de paleta da primeira fila clareando o personagem abaixo,
  especialmente branco no segundo e quarto personagens. A entrada BG
  compartilhada é preservada; L/R e as cores confirmadas continuam disponíveis.

Correção confirmada pelo mantenedor. Fixtures Windows/LLE Normal (3.636
quadros) e Battle (4.750 quadros), com PT-BR, percorrem seis cores nas cinco
colunas e verificam cores iniciais e paletas inferiores intactas. Campanha
completa e cinco controles físicos permanecem não qualificados.

Baixe **superbomberman5-{version}-windows-x64.zip** ou
**superbomberman5-{version}-linux-x86_64.AppImage** experimental e confira
**SHA256SUMS**. Extraia o Windows numa pasta nova e gravável. No Linux, use
chmod +x e execute (sem FUSE: --appimage-extract-and-run). Forneça sua ROM JPN
compatível e use **Generate & rebuild**, inclusive ao atualizar: o jogo já
gerado precisa ser recompilado para receber a correção. Rust não é necessário;
o primeiro download das ferramentas exige internet.

Os oito mods opcionais existentes continuam incluídos, desligados por padrão,
com versão de pacote 0.1.0. Sem ROM, código gerado, executável derivado dela,
assets extraídos, saves ou dumps. Recompilação parcial, com a maior parte da
lógica no interpretador. Veja README.md e docs/STATUS.md. Sem netplay/macOS.
'''

if tuple(int(n) for n in version.split('.')) >= (0,8,2):
    notes = f'''# Super Bomberman 5 v{version} — PT-BR completion and HUD fixes

## English

- Translated Continue/Erase file headings and Normal Game Over title/choices.
- HUD prompts now read **APERTE START** and **FIM DE JOGO** on either side;
  compact spacing fits the original strip and preserves its native blinking.
- Translated the 100% password reward, certificate heading/signature, all
  **33 ranks and tips**, including secret-stage variants. Percentages and
  the native password remain unchanged.

Confirmed by the maintainer. Native tests cover all 33 ranks on both pages.
Isolated Windows/LLE fixtures exercise 0%/100% completion, file headings,
HUD prompts and blink-off frames; translated/original native memory matches.
The 100% and HUD fixtures were checked with both LLE compositors. These are
forced death/progress fixtures, not full natural playthroughs for every rank.
Full campaign and physical multiplayer remain unqualified; Linux is experimental.

Download **superbomberman5-{version}-windows-x64.zip** or
**superbomberman5-{version}-linux-x86_64.AppImage** and verify **SHA256SUMS**.
Extract Windows into a new writable folder. On Linux, chmod +x and run the
AppImage (without FUSE use --appimage-extract-and-run). Supply your own
compatible JPN ROM and use **Generate & rebuild**, including when upgrading;
an old generated executable does not receive native changes automatically.
Rust is not required. Internet is needed for the initial portable tool download.

All eight existing optional mods remain included, disabled by default, at
package version 0.1.0. Honey/Kotetsu and unrelated local work are excluded.
No ROM, generated game code, ROM-derived playable executable, extracted
assets, saves or raw dumps are distributed. Partial recompilation: most
logic still runs on the interpreter. See README.md and docs/STATUS.md.

## Português

- Traduzidos os cabeçalhos de continuar/apagar arquivos e a tela Fim de Jogo
  do Normal, incluindo as opções **CONTINUAR** e **SAIR**.
- HUD: **APERTE START** e **FIM DE JOGO** nos dois lados, com espaçamento
  ajustado à faixa original e preservando o piscar nativo.
- Traduzidos o aviso da senha de 100%, cabeçalho/assinatura dos certificados
  e todas as **33 classificações e dicas**, incluindo fases secretas.
  Porcentagem e senha nativas permanecem intactas.

Confirmado pelo mantenedor. Testes nativos cobrem os 33 resultados nas duas
páginas. Fixtures isoladas Windows/LLE exercitam 0%/100%, arquivos e HUD,
com memória nativa idêntica entre tradução ligada/desligada. As rotas de
100% e HUD foram verificadas nos dois compositores LLE. São cenários com
morte/progresso forçados, sem qualificar campanhas completas naturais para
cada título. Campanha completa e multiplayer físico seguem sem qualificação;
Linux continua experimental.

Baixe **superbomberman5-{version}-windows-x64.zip** ou
**superbomberman5-{version}-linux-x86_64.AppImage** e confira **SHA256SUMS**.
Extraia o Windows numa pasta nova e gravável. No Linux, use chmod +x e execute
(sem FUSE: --appimage-extract-and-run). Forneça sua ROM JPN compatível e use
**Generate & rebuild**, inclusive ao atualizar: o executável antigo não recebe
as mudanças nativas automaticamente. Rust não é necessário; o primeiro
download das ferramentas exige internet.

Os oito mods opcionais existentes permanecem incluídos, desligados por padrão,
com versão de pacote 0.1.0. Honey/Kotetsu e outros trabalhos locais ficam fora.
Sem ROM, código gerado, executável jogável derivado dela, assets extraídos,
saves ou dumps. Recompilação parcial: a maior parte da lógica usa o interpretador.
Veja README.md e docs/STATUS.md para evidências e limites.
'''

if tuple(int(n) for n in version.split('.')) >= (0,9,0):
    notes = f'''# Super Bomberman 5 v{version} — Honey and Kotetsu playable

## English

New optional mod **Honey and Kotetsu — playable characters** (Gameplay,
disabled by default). Enable **Play as Honey and Kotetsu** for a six-column
character selection: the stock columns move closer, Honey (the cowgirl boss
of the Zone 1-3 secret stages) ends the top row and Kotetsu the bottom row,
next to Subordinate Bomber/X.

- Battle and, with the Normal character selection mod, Normal Game
  (including five-player co-op seats).
- They play as Bomberman (same rules, hitbox and items) and are drawn with
  their own cartridge frames, decoded from your ROM at runtime: walking,
  standing, gun/spear attacks for the Boxing and Power Glove, victory and
  DRAW GAME poses taken from the Battle champion screen, death, stun,
  Time Up and riding a Louie (mount and eggs keep their own colors).
- Own picker figures, seat portraits and Battle HUD faces. They have a single
  cartridge palette, so L/R colors do not change them.
- Subordinate Bomber and the color controls keep working with the new grid.

Evidence: native tests for every patch byte, movement tables, archive
decoding, pose mapping, sprite matching, palettes and restoration; Linux LLE
scripted routes for the Battle picker, five-seat Battle through the score
board and champion screen, DRAW GAME, glove/egg/death/stun/Time Up poses,
Normal Zone 1-1 and the ZONE1/3 - S stages. Windows runtime of this mod,
physical controllers and full campaign remain unqualified; Linux is
experimental.

Download **superbomberman5-{version}-windows-x64.zip** or
**superbomberman5-{version}-linux-x86_64.AppImage** and verify **SHA256SUMS**.
Extract Windows into a new writable folder. On Linux, chmod +x and run the
AppImage (without FUSE use --appimage-extract-and-run). Supply your own
compatible JPN ROM and use **Generate & rebuild**, including when upgrading;
an old generated executable does not receive native changes automatically.
Rust is not required. Internet is needed for the initial portable tool download.

All nine optional mods are included, disabled by default, at package version
0.1.0. No ROM, generated game code, ROM-derived playable executable,
extracted assets, saves or raw dumps are distributed; Honey and Kotetsu's
graphics are read from your ROM. Partial recompilation: most logic still runs
on the interpreter. See README.md, docs/HONEY_KOTETSU.md and docs/STATUS.md.

## Português

Novo mod opcional **Honey and Kotetsu — playable characters** (Gameplay,
desligado por padrão). Ative **Play as Honey and Kotetsu** para ter a seleção
com seis colunas: as colunas originais ficam mais próximas, a Honey (a cowgirl
chefe das fases secretas das zonas 1 a 3) fecha a fileira de cima e o Kotetsu
a de baixo, ao lado do Subordinate Bomber/X.

- Battle e, com o mod de seleção no Normal, Normal Game (inclusive nos
  assentos do co-op de cinco jogadores).
- Jogam como o Bomberman (mesmas regras, hitbox e itens) e aparecem com os
  próprios quadros do cartucho, lidos da sua ROM: andando, parados, ataques
  com pistola/lança na Boxing e Power Glove, poses de vitória e DRAW GAME da
  tela de campeão do Battle, morte, atordoamento, Time Up e montados no Louie
  (montaria e ovos mantêm as cores originais).
- Figuras próprias na seleção, retratos e rostos no HUD do Battle. Têm uma
  única paleta no cartucho, então L/R não mudam a cor deles.
- Subordinate Bomber e o controle de cores continuam funcionando na nova grade.

Evidências: testes nativos de cada byte alterado, tabelas do cursor,
decodificação do arquivo gráfico, poses, correspondência de sprites, paletas
e restauração; rotas Linux LLE da seleção, Battle com cinco assentos até o
placar e a tela de campeão, DRAW GAME, luvas/ovo/morte/atordoamento/Time Up,
Normal Zone 1-1 e as fases ZONE1/3 - S. Execução deste mod no Windows,
controles físicos e campanha completa seguem sem qualificação; Linux continua
experimental.

Baixe **superbomberman5-{version}-windows-x64.zip** ou
**superbomberman5-{version}-linux-x86_64.AppImage** e confira **SHA256SUMS**.
Extraia o Windows numa pasta nova e gravável. No Linux, use chmod +x e execute
(sem FUSE: --appimage-extract-and-run). Forneça sua ROM JPN compatível e use
**Generate & rebuild**, inclusive ao atualizar: o executável antigo não recebe
as mudanças nativas automaticamente. Rust não é necessário; o primeiro
download das ferramentas exige internet.

Os nove mods opcionais estão incluídos, desligados por padrão, com versão de
pacote 0.1.0. Sem ROM, código gerado, executável jogável derivado dela, assets
extraídos, saves ou dumps; os gráficos da Honey e do Kotetsu são lidos da sua
ROM. Recompilação parcial: a maior parte da lógica usa o interpretador. Veja
README.md, docs/HONEY_KOTETSU.md e docs/STATUS.md.
'''

if tuple(int(n) for n in version.split('.')) >= (0,10,0):
    notes = f'''# Super Bomberman 5 v{version} — Gold Battle and PT-BR improvements

## English

New optional **Gold Cartridge — Battle** mod: arenas **11–13** and **Hyper
Revenge**. Hyper returns a Revenge-cart bomber after a knockout with the
opponent's equipment. The regular supported ROM already contains the arena
resources; no second ROM is required. Enable **Arenas 11–13 and Hyper Revenge**
in **Mods → Gameplay** after Generate & rebuild.

PT-BR now translates all 13 Battle arena names, **INÍCIO DA BATALHA**, and the
pause options. Headings and menus use cartridge-derived fonts with authored
accents, restored E/F/H crossbars and complete upper outlines. Arena names
stay Portuguese while changing or confirming stages, including the native
blink. Pause plays its native entry sound when pausing and its separate
resume sound when resuming. Subordinate Bomber replaces the secret slot even
when yellow Bomberman is unlocked; L/R still reaches yellow on standard
Bomberman with the color-controls mod.

Download **superbomberman5-{version}-windows-x64.zip** or
**superbomberman5-{version}-linux-x86_64.AppImage** and verify **SHA256SUMS**.
Both contain a ROM-independent setup host, analyzer, sources, notices and ten
optional preinstalled mods, all disabled by default (package version 0.1.0).
Extract Windows into a new writable folder; on Linux chmod +x and run the
AppImage (without FUSE: --appimage-extract-and-run). Supply your own compatible
JPN ROM and use **Generate & rebuild**, including when upgrading. An older
generated executable cannot acquire these native changes from a manifest.
Windows 10+ x64 or Linux x86_64/glibc 2.35+, several GB free and internet for
the initial tool download are required; Rust is not required by players.

Windows/LLE routes exercised arenas 11–13, controlled Hyper cart knockouts,
pause/audio transitions, the unlocked Subordinate slot and paired PT-BR menu
frames against untranslated controls. Native tests cover all 20 ordered
Revenge seat pairs, all 13 arena numbers, save state/patch restoration and
font strokes. Linux remains experimental; packaging and smoke checks do not
establish these gameplay routes on Linux. Full campaign, physical-controller
multiplayer and HLE remain unqualified. Partial recompilation: most game
logic still runs on the interpreter. No netplay. See docs/STATUS.md for the
precise routes and limitations.

No ROM, generated game C, ROM-derived playable executable, extracted graphics
or audio, saves or raw dumps are distributed. Cartridge pixels are read only
from the player's own ROM at runtime. Supported ROM: JPN header, CRC32
c25ab56a, SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4.

## Português

Novo mod opcional **Gold Cartridge — Battle**: arenas **11–13** e **Revanche
Hiper**. Ao eliminar alguém do carrinho de Revanche, o Hiper retorna à arena
com os poderes do adversário. A ROM comum compatível já contém os recursos
das arenas; não é necessário carregar outro cartucho. Depois de Generate &
rebuild, ative **Arenas 11–13 and Hyper Revenge** em **Mods → Gameplay**.

O PT-BR agora traduz os nomes das 13 arenas, **INÍCIO DA BATALHA** e as opções
do pause. Cabeçalhos e menus usam fontes do cartucho com acentos autorais,
barras de E/F/H e contornos superiores preservados. Os nomes permanecem em
português ao trocar ou confirmar fases, inclusive durante o piscar nativo.
O pause reproduz o som de entrada ao pausar e o som separado ao retomar.
O Subordinado ocupa a célula secreta mesmo com o Bomberman amarelo liberado;
o Bomberman padrão continua chegando ao amarelo com L/R no mod de cores.

Baixe **superbomberman5-{version}-windows-x64.zip** ou
**superbomberman5-{version}-linux-x86_64.AppImage** e confira **SHA256SUMS**.
Os dois incluem setup sem ROM, analisador, fontes, licenças e dez mods
opcionais pré-instalados, desligados por padrão (pacotes 0.1.0). Extraia o
Windows numa pasta nova e gravável; no Linux, chmod +x e execute o AppImage
(sem FUSE: --appimage-extract-and-run). Forneça sua ROM JPN compatível e faça
**Generate & rebuild**, inclusive ao atualizar: um executável antigo não
recebe código nativo novo ao importar um manifesto. Requer Windows 10+ x64
ou Linux x86_64/glibc 2.35+, alguns GB livres e internet no primeiro download
das ferramentas. O jogador não precisa instalar Rust.

Rotas Windows/LLE verificaram as arenas extras, eliminações controladas com
Hiper, pause/áudio, célula desbloqueada do Subordinado e menus PT-BR pareados
com o original. Testes nativos cobrem os 20 pares ordenados da Revanche,
13 números de arena, estados/restauração de patches e traços da fonte.
Linux continua experimental; build e smoke test não comprovam essas rotas
de gameplay no Linux. Campanha completa, controles físicos/multiplayer e HLE
seguem sem qualificação. Recompilação parcial, principalmente interpretada;
sem netplay. Evidências e limites em docs/STATUS.md.

Sem ROM, C gerado, executável jogável derivado da ROM, gráficos/áudio
extraídos, saves ou dumps. Os pixels são lidos da ROM do próprio jogador
durante a execução. Confira a identidade acima e as licenças no pacote.
'''

if tuple(int(n) for n in version.split('.')) >= (0,11,0):
    notes = f'''# Super Bomberman 5 v{version} — Louie improvements

## English

New optional **Louie improvements** mod. Enable **Yellow, Red and Black Louies**
in Mods → Gameplay after Generate & rebuild. Press Y while mounted:

- Yellow kicks bombs; motion transfers between bombs and moving soft blocks.
- Red freezes Normal Game enemies for 180 simulation frames (about three
  seconds), with the native player-hit sound, while retaining its player punch.
- Black can hatch from eggs and performs a protected dash that pushes players
  and Normal enemies. It retains the native Louie death sound.

Black acquisition/dash sounds include the maintainer-supplied additional
resources MountBlackLouie.wav and BlackLouieDash.wav, explicitly authorized
for distribution as non-extracted mod resources. See
[audio details and provenance](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/blob/v{version}/docs/LOUIE_IMPROVEMENTS.md).
The acquisition cue matches the native Louie timing; initial digital silence
is skipped in memory. Super Bomberman 4 characters are still in development
and are excluded from this release.

Download **superbomberman5-{version}-windows-x64.zip** or
**superbomberman5-{version}-linux-x86_64.AppImage**, and verify **SHA256SUMS**.
Both include a ROM-independent setup host, analyzer, source, licenses and
**eleven optional mods**, all disabled by default (package version 0.1.0).
Extract Windows into a new writable folder. On Linux chmod +x and run the
AppImage; without FUSE use --appimage-extract-and-run. Supply your own
compatible JPN ROM and use **Generate & rebuild**, also when upgrading.
Windows 10+ x64 or experimental Linux x86_64/glibc 2.35+, several GB free and
internet for the first tool download are required. Players do not need Rust.

Windows/LLE scripted routes cover Yellow bomb/block chains, Red freeze and
native hit command, Black egg/dash/push and P5 with co-op/PT-BR. Native tests
cover all five seats, state validation, patch restoration and audio dispatch.
The maintainer also confirmed the gameplay and sound adjustments locally.
Dummy-audio routes do not measure physical device latency. Full campaign,
all mod combinations, physical-controller multiplayer and HLE remain
unqualified; Linux gameplay coverage remains limited. Partial recompilation:
most game logic runs on the interpreter. No netplay.

No ROM, generated game C, ROM-derived playable executable, extracted graphics
or audio, saves or raw dumps are distributed. Supported ROM: JPN header,
CRC32 c25ab56a, SHA-256
5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4.

## Português

Novo mod opcional **Louie improvements**. Depois de Generate & rebuild, ative
**Yellow, Red and Black Louies** em Mods → Gameplay. Pressione Y montado:

- Yellow chuta bombas; o movimento passa entre bombas e soft blocks móveis.
- Red paralisa inimigos do Normal por 180 frames de simulação (cerca de três
  segundos), com o SFX nativo de acerto, mantendo o soco contra jogadores.
- Black pode sair dos ovos e executa uma arrancada protegida que empurra
  jogadores e inimigos do Normal. Mantém o som nativo de morte dos Louies.

Os sons de aquisição/arrancada do Black incluem MountBlackLouie.wav e
BlackLouieDash.wav como recursos adicionais fornecidos pelo mantenedor, que
autorizou a publicação e esclareceu não serem extrações do jogo original.
Veja os [detalhes e a origem do áudio](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/blob/v{version}/docs/LOUIE_IMPROVEMENTS.md).
A aquisição usa o mesmo instante dos outros Louies e o silêncio digital
inicial é ignorado em memória. Os personagens do Super Bomberman 4 continuam
em desenvolvimento e ficam fora desta release.

Baixe **superbomberman5-{version}-windows-x64.zip** ou
**superbomberman5-{version}-linux-x86_64.AppImage** e confira **SHA256SUMS**.
Ambos incluem setup sem ROM, analisador, fontes, licenças e **onze mods
opcionais**, desligados por padrão (pacotes 0.1.0). Extraia o Windows numa
pasta nova e gravável. No Linux, chmod +x e execute; sem FUSE use
--appimage-extract-and-run. Forneça sua ROM JPN compatível e faça
**Generate & rebuild**, inclusive ao atualizar. Requer Windows 10+ x64 ou
Linux experimental x86_64/glibc 2.35+, alguns GB livres e internet no primeiro
download das ferramentas. O jogador não precisa instalar Rust.

Rotas Windows/LLE verificaram cadeias de bombas/blocos do Yellow, paralisia e
comando sonoro do Red, ovo/arrancada/empurrão do Black e P5 com co-op/PT-BR.
Testes nativos cobrem os cinco assentos, estados, restauração de patches e
disparo de áudio. O mantenedor também confirmou gameplay e ajustes sonoros
localmente. Áudio dummy não mede latência de dispositivos físicos. Campanha
completa, todas as combinações de mods, controles físicos/multiplayer e HLE
seguem sem qualificação. A cobertura Linux é limitada. Recompilação parcial,
principalmente interpretada; sem netplay. Evidências em docs/STATUS.md.

Sem ROM, código gerado, executável jogável derivado da ROM, gráficos/áudio
extraídos, saves ou dumps. Confira a identidade acima e as licenças do pacote.
'''

if tuple(int(n) for n in version.split('.')) >= (0,11,1):
    notes = f'''# Super Bomberman 5 v{version} — PT-BR title font fix

Corrects player-count digits in the PT-BR Normal Game title menu. Counts 1/2
use the original cartridge title glyphs; 3–5 use authored extensions matching
the native weight and full outline, consistent with the copyright lettering.
The Bomberman face cursor now sits beside the selected Portuguese option,
retaining the native eight-pixel cell gap and confirmation bounce.
The eleven optional mods and framework pins from v0.11.0 remain unchanged.
Mods are disabled by default; their package versions remain 0.1.0.

Download **superbomberman5-{version}-windows-x64.zip** or
**superbomberman5-{version}-linux-x86_64.AppImage** and verify **SHA256SUMS**.
Extract Windows into a new writable folder. On Linux, chmod +x and run;
without FUSE use --appimage-extract-and-run. Supply your compatible JPN ROM
and run **Generate & rebuild**, including when upgrading: an old generated
executable cannot receive this native fix by importing a mod manifest.
Requires Windows 10+ x64 or experimental Linux x86_64/glibc 2.35+, several GB
free and internet for the first tool download. Players do not need Rust.

Windows/LLE menu inspection exercised 1,448 frames with PT-BR/co-op enabled;
regression tests cover original glyph reads, all five counts and restoration.
CI builds/tests and package smoke checks cover Windows and Ubuntu 22.04.
Full campaign, physical controllers, all mod combinations and broad Linux
support remain unqualified. Partial recompilation: most game logic runs in
the interpreter. No netplay. See docs/STATUS.md for concrete evidence.

Both downloads are ROM-independent setup hosts with source, analyzer,
licenses and eleven default-disabled mods. No ROM, generated game C,
ROM-derived playable executable, extracted graphics, saves or raw dumps.
The two approved Black Louie add-on WAVs remain included with provenance.
Super Bomberman 4 development is excluded. Supported ROM SHA-256:
5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4.

## Português

Corrige a fonte dos números da quantidade de jogadores no menu Normal em
PT-BR. Os algarismos 1/2 usam os desenhos originais da ROM; 3–5 são extensões
autorais com o mesmo peso e contorno completo da fonte branca do título e
do copyright. O cursor do rosto agora fica junto da opção selecionada, com
o espaçamento original de oito pixels e a animação de confirmação.
Mantém os onze mods opcionais e os pins da v0.11.0. Mods
desligados por padrão; versões dos pacotes continuam 0.1.0.

Baixe o **ZIP Windows x64** ou **AppImage Linux x86_64 experimental** acima e
confira **SHA256SUMS**. Extraia o Windows numa pasta nova e gravável. No Linux,
dê permissão de execução; sem FUSE, use --appimage-extract-and-run. Forneça
sua ROM JPN compatível e faça **Generate & rebuild**, inclusive ao atualizar.
Importar o manifesto do mod não atualiza o executável antigo. Requer Windows
10+ x64 ou Linux experimental x86_64/glibc 2.35+, alguns GB livres e internet
no primeiro download de ferramentas. Rust não é necessário para o jogador.

Inspeção Windows/LLE chegou ao menu com PT-BR/co-op em 1.448 frames. Testes
cobrem leitura dos glifos originais, os cinco números e restauração. CI
verifica builds/testes e smoke dos pacotes Windows/Ubuntu 22.04. Campanha
completa, controles físicos, todas as combinações de mods e suporte amplo a
Linux permanecem não qualificados. Recompilação parcial, principalmente
interpretada; sem netplay. Evidências em docs/STATUS.md.

Pacotes de setup sem ROM, com fontes, analisador, licenças e onze mods.
Sem código/jogo gerado, gráficos extraídos, saves ou dumps. Os dois WAVs
adicionais autorizados do Black Louie seguem incluídos com origem documentada.
Desenvolvimento de personagens do Super Bomberman 4 fica fora da release.
'''

if tuple(int(n) for n in version.split('.')) >= (0,12,0):
    notes = f'''# Super Bomberman 5 v{version} — Bomber Kings

## English

New optional **Bomber Kings — Super Bomberman 4 characters** mod. After
Generate & rebuild, enable **Play as the Bomber Kings** in Mods → Gameplay and
select **your own Super Bomberman 4 ROM** (Japanese release, SHA-256
68df561dba2c47a2b743eb5f960530c7a587646d3e448f68f9937305815c9f44) in the
mod's **Super Bomberman 4 ROM** item. Its graphics, palettes and sound driver
are read from that file at runtime; nothing from Super Bomberman 4 is in the
repository or in these downloads.

- The character selection gets a third row: Super Bomberman 4's Bomberman,
  Hammer, Jet, Lady, Bazooka and Great Bomber, with their own portraits and
  five SB4 colors each (L/R with the color mod). Honey and Kotetsu end the
  two stock rows.
- Hold **B** and press a **direction** for the character's special ability,
  ported from Super Bomberman 4 with its timings, poses, graphics and sound
  effects: Hammer's mace swing (stuns, takes an item), Jet's flight with a
  fire trail (immune to enemy contact), Lady's laser (stuns, takes items),
  Bazooka's fire beam (burns the first soft block, sets off bombs) and
  Great's invulnerability. Honey fires her boss bullet and Kotetsu uses his
  sword thrust. Bazooka, Jet, Great, Honey and Kotetsu then move at half
  speed for about 10.7 s, as in Super Bomberman 4.
- Abilities hit enemies and bosses in Normal Game (never other players) and
  other players or the other team in Battle. B alone still detonates remote
  bombs; riding Louie keeps B native. Riding and Bad Bomber use SB4's poses.

Download **superbomberman5-{version}-windows-x64.zip** or
**superbomberman5-{version}-linux-x86_64.AppImage** and verify **SHA256SUMS**.
Both include a ROM-independent setup host, analyzer, source, licenses and
**twelve optional mods**, all disabled by default (package version 0.1.0).
Extract Windows into a new writable folder. On Linux chmod +x and run the
AppImage; without FUSE use --appimage-extract-and-run. Supply your own
compatible JPN ROM and run **Generate & rebuild**, also when upgrading: an old
generated executable cannot receive the new native code from a manifest.
Requires Windows 10+ x64 or experimental Linux x86_64/glibc 2.35+, several GB
free and internet for the first tool download. Players do not need Rust.

Evidence: thirteen native test programs (including ability input, damage
rules, timelines, picker layout and SB4 ROM reading) and scripted Linux LLE
routes — three-row picker with all picker mods, Battle Free-for-All and Team
Battle with five kings, Normal Game Zone 1-1 with five players where Jet,
Bazooka, Honey and Kotetsu each killed an enemy. The maintainer also played
the abilities in a local Windows build. CPU players do not use abilities;
the Battle champion screen still shows Bomberman. Full campaign, every mod
combination, physical-controller multiplayer and HLE remain unqualified;
Linux gameplay coverage is limited. Partial recompilation: most game logic
runs on the interpreter. No netplay.

No ROM, generated game C, ROM-derived playable executable, extracted graphics
or audio, saves or raw dumps are distributed. The two approved Black Louie
add-on WAVs remain included with documented provenance. Supported SB5 ROM:
JPN header, CRC32 c25ab56a, SHA-256
5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4.

## Português

Novo mod opcional **Bomber Kings — Super Bomberman 4 characters**. Depois de
Generate & rebuild, ative **Play as the Bomber Kings** em Mods → Gameplay e
escolha **a sua própria ROM do Super Bomberman 4** (lançamento japonês,
SHA-256 acima) no item **Super Bomberman 4 ROM** do mod. Gráficos, paletas e
driver de som são lidos desse arquivo durante o jogo; nada do Super
Bomberman 4 fica no repositório ou nestes downloads.

- A seleção de personagens ganha uma terceira fileira: o Bomberman, o
  Hammer, o Jet, a Lady, o Bazooka e o Great Bomber do Super Bomberman 4, com
  retratos próprios e cinco cores do SB4 cada (L/R com o mod de cores). Honey
  e Kotetsu fecham as duas fileiras originais.
- Segure **B** e aperte uma **direção** para a habilidade especial, portada
  do Super Bomberman 4 com os mesmos tempos, poses, gráficos e efeitos
  sonoros: maça do Hammer (atordoa e tira item), voo do Jet com rastro de
  fogo (imune ao contato com inimigos), laser da Lady (atordoa e tira itens),
  raio de fogo do Bazooka (queima o primeiro soft block e detona bombas) e
  invencibilidade do Great. A Honey dispara o tiro da luta de chefe e o
  Kotetsu dá a estocada de espada. Bazooka, Jet, Great, Honey e Kotetsu ficam
  com metade da velocidade por cerca de 10,7 s, como no Super Bomberman 4.
- As habilidades acertam inimigos e chefes no Normal Game (nunca outros
  jogadores) e os outros jogadores ou o time adversário no Battle. B sozinho
  continua detonando a bomba-controle; montado no Louie o B continua normal.
  Montaria e Bad Bomber usam as poses do SB4.

Baixe **superbomberman5-{version}-windows-x64.zip** ou
**superbomberman5-{version}-linux-x86_64.AppImage** e confira **SHA256SUMS**.
Ambos incluem setup sem ROM, analisador, fontes, licenças e **doze mods
opcionais**, desligados por padrão (pacotes 0.1.0). Extraia o Windows numa
pasta nova e gravável. No Linux, chmod +x e execute; sem FUSE use
--appimage-extract-and-run. Forneça sua ROM JPN compatível e faça
**Generate & rebuild**, inclusive ao atualizar. Requer Windows 10+ x64 ou
Linux experimental x86_64/glibc 2.35+, alguns GB livres e internet no primeiro
download das ferramentas. O jogador não precisa instalar Rust.

Evidências: treze programas de testes nativos (entrada das habilidades,
regras de dano, linhas do tempo, layout da seleção e leitura da ROM do SB4) e
rotas roteirizadas no Linux LLE — seleção de três fileiras com todos os mods
de seleção, Battle Free-for-All e Team Battle com cinco reis, Normal Game
Zone 1-1 com cinco jogadores em que Jet, Bazooka, Honey e Kotetsu mataram um
inimigo cada. O mantenedor também jogou as habilidades num build local no
Windows. Jogadores da CPU não usam habilidades; a tela de campeão do Battle
continua com o Bomberman. Campanha completa, todas as combinações de mods,
controles físicos/multiplayer e HLE seguem sem qualificação. A cobertura
Linux é limitada. Recompilação parcial, principalmente interpretada; sem
netplay. Evidências em docs/STATUS.md.

Sem ROM, código gerado, executável jogável derivado da ROM, gráficos/áudio
extraídos, saves ou dumps. Os dois WAVs adicionais autorizados do Black Louie
seguem incluídos com origem documentada.
'''


if tuple(int(n) for n in version.split('.')) >= (0,12,1):
    notes = f'''# Super Bomberman 5 v{version} - Normal menus, Back and native Louie audio

## English

- Normal Game's five-player menu uses **1 PLAYER / 2-5 PLAYERS**, the native
  Y glyph and a corrected T outline in PUSH START BUTTON. The face cursor
  follows the selected row in English and PT-BR. PT-BR digits retain the
  original title-font weight and outline.
- **B** from Normal character selection returns to the previous New Game or
  Continue file list without freezing; repeated selection/cancel works.
- Black Louie's egg/mount and dash sounds now use the cartridge SPC driver
  (native commands $022E and $0235). Both external WAVs and their loader are
  removed. No extracted audio is bundled.
- The README's top-left screenshot now shows the corrected player menu.

Includes all **twelve optional mods**, disabled by default, including Bomber
Kings from v0.12.0. Bomber Kings requires your own supported Super Bomberman 4
ROM selected on the Mods page; its graphics and sound data are read at runtime.
Mod package versions remain 0.1.0; updating a manifest cannot update native code.

Download **superbomberman5-{version}-windows-x64.zip** or experimental
**superbomberman5-{version}-linux-x86_64.AppImage**, and verify **SHA256SUMS**.
Extract Windows to a new writable folder. On Linux use chmod +x and run;
without FUSE use --appimage-extract-and-run. Supply your own supported SB5 ROM
and run **Generate & rebuild**, including when upgrading. Windows 10+ x64 or
Linux x86_64/glibc 2.35+, several GB free and internet for initial tool downloads
are required. Rust is not required by players.

Windows/LLE regression routes covered New Game/Continue cancellation with
1/2/5 seats, PT-BR on/off, repeated entry and stage confirmation. Black Louie
egg/dash native commands and enemy push passed with P1 and P5/co-op/PT-BR;
the maintainer confirmed the audio fix in gameplay. Native and Python tests
and publication audits pass. Broader campaign, all mod combinations, physical
multiplayer and Linux distributions remain unqualified. Partial recompilation:
most logic uses the interpreter; no netplay. See docs/STATUS.md for evidence.

Setup packs contain source, analyzer, launcher, licenses and manifests, never
ROMs, generated game code, playable ROM-derived executables, extracted audio
or graphics, saves or raw dumps. Supported SB5: JPN header, CRC32 c25ab56a,
SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4.

## Português

- Menu Normal de cinco jogadores com **1 PLAYER / 2-5 PLAYERS**, Y original,
  contorno do T corrigido em PUSH START BUTTON e cursor junto da linha
  selecionada, em inglês e PT-BR. Números PT-BR mantêm peso e contorno originais.
- **B** na seleção de personagens do Normal retorna aos arquivos de New Game
  ou Continue sem travar, inclusive ao entrar e cancelar repetidamente.
- Montagem e arrancada do Black Louie usam os SFX nativos da ROM ($022E/$0235).
  Os dois WAVs externos e o carregador foram removidos. Sem áudio extraído no pacote.
- Nova captura da tela corrigida no quadro superior esquerdo do README.

Mantém os **doze mods opcionais**, desligados por padrão, incluindo Bomber Kings
  da v0.12.0. Esse mod usa sua própria ROM compatível do Super Bomberman 4,
selecionada na página Mods; os dados são lidos durante a execução.

Baixe o **ZIP Windows x64** ou **AppImage Linux x86_64 experimental** acima e
confira **SHA256SUMS**. Extraia o Windows numa pasta nova e gravável. No Linux,
chmod +x e execute; sem FUSE, --appimage-extract-and-run. Forneça sua ROM do SB5
e faça **Generate & rebuild**, inclusive ao atualizar: o manifesto 0.1.0 não
atualiza o código nativo. Requer Windows 10+ x64 ou Linux/glibc 2.35+, espaço
livre e internet para as primeiras ferramentas; Rust não é necessário.

Rotas Windows/LLE verificaram retorno de New Game/Continue com 1/2/5 jogadores,
PT-BR ligado/desligado, reentrada e confirmação de fase. Ovo/arrancada/empurrão
do Black foram verificados com P1 e P5/co-op/PT-BR, contando os comandos sonoros;
o mantenedor confirmou o resultado no jogo. Campanha completa, todas as
combinações, multiplayer físico e distribuições Linux seguem sem qualificação.
Recompilação parcial, principalmente interpretada; sem netplay. Sem ROM, código
ou executável gerado do jogo, gráficos/áudio extraídos, saves ou dumps.
'''

if tuple(int(n) for n in version.split('.')) >= (0,13,0):
    notes = f'''# Super Bomberman 5 v{version} - Super Bomberman 5 Extra Characters and Emperor Terrorin

## English

New optional mod **Super Bomberman 5 Extra Characters** (enable **Play as the
Super Bomberman 5 Extra Characters** in Mods). It replaces the separate Honey and
Kotetsu and Subordinate Bomber mods, which are no longer shipped: if you used
them, enable this one after Generate & rebuild.

- Seven-column character selection: the stock columns move left, **Honey** and
  **Emperor Terrorin** end the top row, **Kotetsu** the second, **Subordinate
  Bomber** takes the X cell (six cartridge colors with L/R). The cursor skips
  empty cells. With Bomber Kings, its Super Bomberman 4 row sits below.
- **Emperor Terrorin**, the on-foot final phase of ZONE5 - J, is playable with
  his own walking, standing, Power Glove, cheer, hurt, riding, portrait and HUD
  frames decoded from your ROM, and the boss's defeat (his head flies off).
- **B + direction**: Honey's shot, Kotetsu's sword (both moved here from Bomber
  Kings) and **Terrorin's barrier**: he spins, a blue barrier fades in around
  the 3 x 3 square he stands in with the boss's barrier sound (rendered from
  your ROM in a private APU), and a **Clock** flies off him like a lost item.
  In Battle, Terrorin taking it paralyzes everyone else; an opponent taking it
  paralyzes Terrorin. A half-speed debuff follows, as for the other abilities.
- The README montage shows the new picker and abilities in Zone 1-1 and Battle.

Includes **eleven optional mods**, disabled by default (twelve minus the two
merged mods, plus Extra Characters). Bomber Kings still requires your own
Super Bomberman 4 ROM selected on the Mods page. Mod package versions are
0.1.0; native code arrives only with Generate & rebuild.

Download **superbomberman5-{version}-windows-x64.zip** or experimental
**superbomberman5-{version}-linux-x86_64.AppImage**, and verify **SHA256SUMS**.
Extract Windows to a new writable folder. On Linux use chmod +x and run;
without FUSE use --appimage-extract-and-run. Supply your own supported SB5 ROM
and run **Generate & rebuild**, including when upgrading. Windows 10+ x64 or
Linux x86_64/glibc 2.35+, several GB free and internet for initial tool
downloads are required. Rust is not required by players.

Validation: native tests (picker tables, Terrorin frames/poses/defeat, barrier
timeline, Clock flight and paralysis rules), Python tests and publication
audits; Linux LLE scripted Battle and Normal routes (five seats, picker,
barrier, Clock, defeat); the barrier sound rendered in a headless harness; the
maintainer checked the mod in Windows gameplay. CPU players do not use
abilities; full campaign, all mod combinations, physical multiplayer and
Linux distributions remain unqualified. Partial recompilation: most logic uses
the interpreter; no netplay. See docs/EXTRA_CHARACTERS.md and docs/STATUS.md.

Setup packs contain source, analyzer, launcher, licenses and manifests, never
ROMs, generated game code, playable ROM-derived executables, extracted audio
or graphics, saves or raw dumps. Supported SB5: JPN header, CRC32 c25ab56a,
SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4.

## Português

Novo mod opcional **Super Bomberman 5 Extra Characters** (ative **Play as the
Super Bomberman 5 Extra Characters** em Mods). Ele substitui os mods separados
Honey and Kotetsu e Subordinate Bomber, que deixam de vir nos pacotes: se você
os usava, ative este depois do Generate & rebuild.

- Seleção com sete colunas: as originais vão para a esquerda, **Honey** e
  **imperador Terrorin** fecham a fileira de cima, **Kotetsu** a segunda e o
  **Subordinate Bomber** fica na célula do X (seis cores do cartucho com L/R).
  O cursor pula as células vazias. Com o Bomber Kings, a fileira do SB4 fica
  embaixo.
- O **imperador Terrorin**, a última etapa a pé do ZONE5 - J, é jogável, com
  quadros próprios lidos da sua ROM (andar, parado, Power Glove, comemorar,
  dano, montaria, retrato e HUD) e a derrota do chefe (a cabeça sai voando).
- **B + direção**: tiro da Honey, espada do Kotetsu (vindos do Bomber Kings) e
  a **barreira do Terrorin**: ele gira, a barreira azul aparece aos poucos em
  volta do quadrado 3 x 3 com o som da barreira do chefe (gerado da sua ROM
  num APU privado) e um **Clock** sai voando como item perdido. No Battle, se o
  Terrorin pegar, os outros ficam paralisados; se um adversário pegar, o
  Terrorin fica. Depois vem o debuff de meia velocidade.
- A montagem do README mostra a nova seleção e as habilidades na ZONE1-1 e no
  Battle.

Inclui **onze mods opcionais**, desligados por padrão. O Bomber Kings continua
exigindo sua própria ROM do Super Bomberman 4, selecionada em Mods. Os pacotes
de mod ficam em 0.1.0; o código nativo só chega com Generate & rebuild.

Baixe o **ZIP Windows x64** ou o **AppImage Linux x86_64 experimental** e
confira **SHA256SUMS**. Extraia o Windows numa pasta nova e gravável. No Linux,
chmod +x e execute; sem FUSE, --appimage-extract-and-run. Forneça sua ROM do
SB5 e faça **Generate & rebuild**, inclusive ao atualizar. Requer Windows 10+
x64 ou Linux/glibc 2.35+, espaço livre e internet para as primeiras
ferramentas; Rust não é necessário.

Validação: testes nativos e Python, auditorias de publicação, rotas Linux LLE
de Battle e Normal com cinco jogadores, som da barreira gerado num harness e
verificação do mantenedor no Windows. A CPU não usa habilidades; campanha
completa, todas as combinações, multiplayer físico e distribuições Linux seguem
sem qualificação. Recompilação parcial, principalmente interpretada; sem
netplay. Sem ROM, código ou executável gerado do jogo, gráficos/áudio
extraídos, saves ou dumps.
'''

if version == '0.13.1':
    notes = f'''# Super Bomberman 5 v{version}

## English

- Optional Normal HUD faces: selected characters/colors and native alive/defeated states, without the stock Bomberman behind them or Battle victory counters.
- P1 and active P2 only, even with five Normal players; extra-seat death/reentry cannot overwrite their portraits.
- Game Over → Continue retains characters, colors and the original one/two-player roster. Independent SB4 palettes survive leaving the Normal picker.
- Open Windows through **SuperBomberman5Recomp.exe**, with the existing “5” icon. Extract the complete ZIP. play.cmd remains a compatibility shortcut.
- Updated Normal gameplay README panel with five characters, abilities and green Subordinate Bomber riding Black Louie.

Twelve optional mods, disabled by default. Extra Characters combines Honey, Kotetsu, Subordinate Bomber and Emperor Terrorin. Bomber Kings needs your own verified SB4 ROM. Mod packages remain 0.1.0; **Generate & rebuild** after upgrading.

Download the Windows x64 ZIP or experimental Linux x86_64 AppImage and verify SHA256SUMS. Install in a new writable folder. Linux: chmod +x; without FUSE use --appimage-extract-and-run. Supply your own supported JPN SB5 ROM and run Generate & rebuild. Internet is needed for the first tools; Rust is not required.

Evidence and runtime limits: docs/STATUS.md. Partial recompilation, mostly interpreted gameplay. Full campaign, all mod combinations, physical multiplayer and broad Linux compatibility remain unqualified; no netplay. Setup downloads contain no ROM, generated game code/binary, extracted cartridge assets, saves or raw captures.

## Português

- Mod opcional de rostos na HUD do Normal: personagem/cor selecionados e poses vivo/derrotado, sem o Bomberman original no fundo nem contadores de vitória.
- Apenas P1 e P2 ativo, mesmo com cinco jogadores; morte/reentrada dos demais não sobrescreve seus retratos.
- Game Over → Continue preserva personagens, cores e a formação original de um/dois jogadores. As paletas independentes do SB4 são mantidas ao sair da seleção do Normal.
- No Windows, abra **SuperBomberman5Recomp.exe**, com o ícone atual do “5”. Extraia o ZIP completo. play.cmd continua como atalho de compatibilidade.
- Quadro Normal do README atualizado com cinco personagens, habilidades e Subordinado verde montado no Black Louie.

Doze mods opcionais, desligados por padrão. Extra Characters reúne Honey, Kotetsu, Subordinate Bomber e imperador Terrorin. Bomber Kings exige sua própria ROM conferida do SB4. Pacotes de mod em 0.1.0; faça **Generate & rebuild** ao atualizar.

Baixe o ZIP Windows x64 ou AppImage Linux x86_64 experimental e confira SHA256SUMS. Instale em pasta nova e gravável. Linux: chmod +x; sem FUSE use --appimage-extract-and-run. Forneça a ROM JPN compatível do SB5 e faça Generate & rebuild. Internet para as primeiras ferramentas; Rust não é necessário.

Evidências e limites em docs/STATUS.md. Recompilação parcial, principalmente interpretada. Campanha completa, todas as combinações, multiplayer físico e compatibilidade ampla no Linux não qualificados; sem netplay. Downloads de setup sem ROM, código/executável gerado do jogo, assets extraídos, saves ou capturas brutas.
'''

if version == '1.0.0':
    notes = f'''# Super Bomberman 5 v{version}

## English

- Five-player Normal Game: Louies now hatch, animate and are ridden correctly with every seat, even when all sprite pages are in use.
- ZONE3 - D in co-op: the Pirate Bomber gets its Black Louie again, and a restart after everyone dies no longer leaves P1 on a frozen Red Louie or hangs the game.
- Mounted boss entry (ZONE4 - G and others) no longer corrupts P1 or the boss.
- Portal destinations now also appear on the portals shown after defeating a boss.
- Several identical controllers (for example two DualSense pads) can be assigned to different players in the launcher and keep their seats in game.
- The removed Honey and Kotetsu and Subordinate Bomber packages no longer linger in rebuilt installs; Windows uses only **SuperBomberman5Recomp.exe** (play.cmd and the shortcut were removed).
- Shorter README with a new gameplay panel: green Subordinate Bomber on Black Louie, Jet, Bazooka, Hammer and Terrorin using their abilities.

Twelve optional mods, disabled by default; mod packages remain 0.1.0. Run **Generate & rebuild** after upgrading.

Download the Windows x64 ZIP or experimental Linux x86_64 AppImage and verify SHA256SUMS. Install in a new writable folder. Linux: chmod +x; without FUSE use --appimage-extract-and-run. Supply your own supported JPN SB5 ROM. Internet is needed for the first tools; Rust is not required.

Still a prerelease: partial recompilation with mostly interpreted gameplay. Full campaign, all mod combinations, physical five-player play and broad Linux compatibility remain unqualified; no netplay. Evidence: docs/STATUS.md. Downloads contain no ROM, generated game code/binary, extracted assets, saves or raw captures.

## Português

- Normal Game com cinco jogadores: os Louies nascem, animam e são montados corretamente em todos os assentos, mesmo com todas as páginas de sprites ocupadas.
- ZONE3 - D em co-op: o Pirate Bomber volta a receber o Black Louie, e reiniciar depois que todos morrem não deixa mais o P1 num Red Louie travado nem congela o jogo.
- Entrada montada em fases de chefe (ZONE4 - G e outras) não corrompe mais o P1 nem o chefe.
- Os destinos dos portais também aparecem nos portais mostrados após derrotar um chefe.
- Vários controles iguais (por exemplo dois DualSense) podem ser atribuídos a jogadores diferentes no launcher e mantêm o assento no jogo.
- Os pacotes removidos Honey and Kotetsu e Subordinate Bomber não ficam mais nas instalações recompiladas; o Windows usa apenas **SuperBomberman5Recomp.exe** (play.cmd e o atalho foram removidos).
- README mais curto com novo quadro: Subordinate verde no Black Louie, Jet, Bazooka, Hammer e Terrorin usando habilidades.

Doze mods opcionais, desligados por padrão; pacotes de mod em 0.1.0. Faça **Generate & rebuild** ao atualizar.

Baixe o ZIP Windows x64 ou o AppImage Linux x86_64 experimental e confira SHA256SUMS. Instale em pasta nova e gravável. Linux: chmod +x; sem FUSE use --appimage-extract-and-run. Forneça sua ROM JPN compatível do SB5. Internet para as primeiras ferramentas; Rust não é necessário.

Ainda é pré-release: recompilação parcial, principalmente interpretada. Campanha completa, todas as combinações de mods, cinco controles físicos e compatibilidade ampla no Linux não qualificados; sem netplay. Evidências em docs/STATUS.md. Downloads sem ROM, código/executável gerado do jogo, assets extraídos, saves ou capturas brutas.
'''

if version == '1.0.1':
    notes = f'''# Super Bomberman 5 v{version}

## English

- The Windows ZIP now has a single executable at its root, **SuperBomberman5Recomp.exe**: it is the setup program and, after Generate & rebuild, it opens the rebuilt game.
- Generate & rebuild works in folders whose path has accents or other non-English letters (for example OneDrive's "Área de Trabalho"); it previously stopped at "Configure failed" or built the game without mods. Such folders are built through an ASCII link under C:\\Users\\Public\\retcomm-builds.
- Includes all v1.0.0 changes: five-player Louies, ZONE3 - D co-op, mounted boss entry, portal labels after bosses and identical controllers on different players.

Install into a new writable folder and run **Generate & rebuild** with your own supported JPN SB5 ROM. Download the Windows x64 ZIP or experimental Linux x86_64 AppImage and verify SHA256SUMS. Internet is needed for the first tools; Rust is not required.

First full release: the maintainer has played the campaign to the end on Windows. Partial recompilation with mostly interpreted gameplay; all mod combinations, physical five-player play and broad Linux compatibility remain unqualified; no netplay. Downloads contain no ROM, generated game code/binary, extracted assets, saves or raw captures.

## Português

- O ZIP Windows agora tem um único executável na raiz, **SuperBomberman5Recomp.exe**: ele é o setup e, depois do Generate & rebuild, abre o jogo recompilado.
- O Generate & rebuild funciona em pastas com acento ou outras letras fora do inglês (por exemplo a "Área de Trabalho" do OneDrive); antes parava em "Configure failed" ou gerava o jogo sem mods. Essas pastas são compiladas por um atalho sem acento em C:\\Users\\Public\\retcomm-builds.
- Inclui todas as mudanças da v1.0.0: Louies com cinco jogadores, ZONE3 - D em co-op, entrada montada em chefes, destinos dos portais após chefes e controles iguais em jogadores diferentes.

Instale numa pasta nova e gravável e faça **Generate & rebuild** com sua ROM JPN compatível do SB5. Baixe o ZIP Windows x64 ou o AppImage Linux x86_64 experimental e confira SHA256SUMS. Internet para as primeiras ferramentas; Rust não é necessário.

Primeira versão completa: o mantenedor zerou a campanha no Windows. Recompilação parcial, principalmente interpretada; combinações de mods, cinco controles físicos e compatibilidade ampla no Linux não qualificados; sem netplay. Downloads sem ROM, código/executável gerado do jogo, assets extraídos, saves ou capturas brutas.
'''

if version == '1.0.2':
    notes = f'''# Super Bomberman 5 v{version}

## English

- **ZONE3 - D exit gates:** with **Battle character faces in Normal HUD** enabled, the gates did not appear after defeating the Pirate Bomber and his partner (the face's graphics took the gates' video memory). Boss stages now keep the stock HUD icons.
- **Abilities against bosses:** the B + direction abilities (Bomber Kings, Honey, Kotetsu, Terrorin) now hurt the rival bomber bosses, through the game's own fire hit: first their Louie, then a hit point. A Louie is no longer hit on its own.
- **Jet:** keeps blowing fire behind him while pressed against the obstacle, as in Super Bomberman 4.
- **PT-BR menus:** "Continuar" no longer appears over the blocks of ZONE2's boss arena; during a stage only the HUD texts are translated.

Install into a new writable folder and run **Generate & rebuild** with your own supported JPN SB5 ROM; an older generated game does not get these fixes. Download the Windows x64 ZIP or experimental Linux x86_64 AppImage and verify SHA256SUMS. Internet is needed for the first tools; Rust is not required.

The fixes were verified on Linux with scripted, forced-stage routes (ZONE3 - D and ZONE2's boss) and native tests; not yet in a played Windows campaign. Partial recompilation with mostly interpreted gameplay; all mod combinations, physical five-player play and broad Linux compatibility remain unqualified; no netplay. Downloads contain no ROM, generated game code/binary, extracted assets, saves or raw captures.

## Português

- **Portais do ZONE3 - D:** com **Battle character faces in Normal HUD** ligado, os portais não apareciam depois de vencer o pirata e o parceiro (o gráfico do rosto ocupava a memória de vídeo dos portais). Nas fases de chefe a HUD fica com os ícones originais.
- **Habilidades contra chefes:** as habilidades de B + direção (Bomber Kings, Honey, Kotetsu, Terrorin) agora machucam os chefes bombers, pelo dano de fogo do próprio jogo: primeiro o Louie, depois a vida. O Louie não é mais atingido sozinho.
- **Jet:** continua soltando fogo para trás enquanto está parado no obstáculo, como no Super Bomberman 4.
- **Menus em PT-BR:** o "Continuar" não aparece mais sobre os blocos da arena do chefe do ZONE2; durante a fase só os textos da HUD são traduzidos.

Instale numa pasta nova e gravável e faça **Generate & rebuild** com sua ROM JPN compatível do SB5; um jogo gerado antes não recebe estas correções. Baixe o ZIP Windows x64 ou o AppImage Linux x86_64 experimental e confira SHA256SUMS. Internet para as primeiras ferramentas; Rust não é necessário.

As correções foram verificadas no Linux com rotas roteirizadas de fase forçada (ZONE3 - D e chefe do ZONE2) e testes nativos; ainda não numa campanha jogada no Windows. Recompilação parcial, principalmente interpretada; combinações de mods, cinco controles físicos e compatibilidade ampla no Linux não qualificados; sem netplay. Downloads sem ROM, código/executável gerado do jogo, assets extraídos, saves ou capturas brutas.
'''

if version == '1.0.3':
    notes = f'''# Super Bomberman 5 v{version}

## English

- **Abilities in every Battle arena:** the B + direction abilities (Super Bomberman 4 characters with **Bomber Kings**, Honey, Kotetsu and Emperor Terrorin) only worked in Battle arena 1. They now work in all arenas, including the Gold Cartridge ones.
- **Five-player slowdown:** with **Battle character faces in Normal HUD** enabled, the HUD refreshed the lives display every frame and five-player stages dropped up to 24% of frames. The full refresh now runs once every eight HUD updates (about 4% of frames with every gameplay mod on, against 9% before).

Install into a new writable folder and run **Generate & rebuild** with your own supported JPN SB5 ROM; an older generated game does not get these fixes. Download the Windows x64 ZIP or experimental Linux x86_64 AppImage and verify SHA256SUMS. Internet is needed for the first tools; Rust is not required.

The fixes were verified on Linux with scripted five-player Normal and Battle routes and native tests; not yet in a played Windows campaign or with physical controllers. Partial recompilation with mostly interpreted gameplay; all mod combinations, physical five-player play and broad Linux compatibility remain unqualified; no netplay. Downloads contain no ROM, generated game code/binary, extracted assets, saves or raw captures.

## Português

- **Habilidades em todas as arenas do Battle:** as habilidades de B + direção (personagens do Super Bomberman 4 com o **Bomber Kings**, Honey, Kotetsu e Imperador Terrorin) só funcionavam na arena 1 do Battle. Agora funcionam em todas, inclusive nas do Gold Cartridge.
- **Lentidão com cinco jogadores:** com **Battle character faces in Normal HUD** ligado, a HUD atualizava o contador de vidas a cada quadro e as fases com cinco jogadores perdiam até 24% dos quadros. A atualização completa agora roda uma vez a cada oito atualizações da HUD (cerca de 4% dos quadros com todos os mods de jogo ligados, contra 9% antes).

Instale numa pasta nova e gravável e faça **Generate & rebuild** com sua ROM JPN compatível do SB5; um jogo gerado antes não recebe estas correções. Baixe o ZIP Windows x64 ou o AppImage Linux x86_64 experimental e confira SHA256SUMS. Internet para as primeiras ferramentas; Rust não é necessário.

As correções foram verificadas no Linux com rotas roteirizadas de cinco jogadores no Normal e no Battle e testes nativos; ainda não numa campanha jogada no Windows nem com controles físicos. Recompilação parcial, principalmente interpretada; combinações de mods, cinco controles físicos e compatibilidade ampla no Linux não qualificados; sem netplay. Downloads sem ROM, código/executável gerado do jogo, assets extraídos, saves ou capturas brutas.
'''

if version == '1.1.0':
    notes = f'''# Super Bomberman 5 v{version}

## English

- **Five-player spawns next to P1/P2:** with **Play Normal Game with up to five players**, P3 and P5 now start next to P1 and P4 next to P2 in regular stages (one tile beside the original spawn, toward the middle of the field). Blocked tiles still fall back to the nearest safe floor. Boss stages keep their layout.
- **New optional mod — corner spawns:** **Spawn P3-P5 in the corners and centre** brings back the previous layout (P3 lower left, P4 lower right, P5 centre). It only works together with the five-player mod. Thirteen optional mods are now included, all disabled by default.
- **5 Players group:** both five-player mods are listed in their own **5 Players** section on the Mods page.

Install into a new writable folder and run **Generate & rebuild** with your own supported JPN SB5 ROM; an older generated game does not get these changes. Download the Windows x64 ZIP or experimental Linux x86_64 AppImage and verify SHA256SUMS. Internet is needed for the first tools; Rust is not required.

The spawn change is covered by native tests and was checked by the maintainer in a Windows build; physical five-player play is not qualified. Partial recompilation with mostly interpreted gameplay; all mod combinations and broad Linux compatibility remain unqualified; no netplay. Downloads contain no ROM, generated game code/binary, extracted assets, saves or raw captures.

## Português

- **Spawns de cinco jogadores junto de P1/P2:** com **Play Normal Game with up to five players**, P3 e P5 agora nascem junto do P1 e P4 junto do P2 nas fases comuns (um tile ao lado do spawn original, na direção do meio do mapa). Um tile bloqueado continua indo para o chão seguro mais próximo. As fases de chefe mantêm o layout.
- **Novo mod opcional — spawns nos cantos:** **Spawn P3-P5 in the corners and centre** traz de volta o layout anterior (P3 no canto inferior esquerdo, P4 no inferior direito, P5 no centro). Só funciona junto com o mod de cinco jogadores. Agora são treze mods opcionais, todos desligados por padrão.
- **Grupo 5 Players:** os dois mods de cinco jogadores ficam numa seção própria, **5 Players**, na página Mods.

Instale numa pasta nova e gravável e faça **Generate & rebuild** com sua ROM JPN compatível do SB5; um jogo gerado antes não recebe estas mudanças. Baixe o ZIP Windows x64 ou o AppImage Linux x86_64 experimental e confira SHA256SUMS. Internet para as primeiras ferramentas; Rust não é necessário.

A mudança de spawn tem testes nativos e foi conferida pelo mantenedor num build Windows; cinco controles físicos não foram qualificados. Recompilação parcial, principalmente interpretada; combinações de mods e compatibilidade ampla no Linux não qualificadas; sem netplay. Downloads sem ROM, código/executável gerado do jogo, assets extraídos, saves ou capturas brutas.
'''

if version == '1.1.1':
    notes = f'''# Super Bomberman 5 v{version}

## English

- **ZONE3 - 1 exit gate with HUD faces:** with **Battle character faces in Normal HUD** enabled, the exit gate of ZONE3 - 1 never appeared after every enemy was defeated: the face graphics took the sprite memory the gate needs. When the game cannot load a graphic for lack of sprite memory while faces are shown, the faces are now removed through the game's own routine and the HUD returns to the stock icons until the next stage, so the gate appears. Stages whose gate fits (such as ZONE1 - 1) keep their faces.

Includes everything from v1.1.0 (five-player spawns next to P1/P2 and the optional corner spawn mod). Install into a new writable folder and run **Generate & rebuild** with your own supported JPN SB5 ROM; an older generated game does not get this fix. Download the Windows x64 ZIP or experimental Linux x86_64 AppImage and verify SHA256SUMS. Internet is needed for the first tools; Rust is not required.

The fix was verified on Linux with scripted, forced-stage routes (ZONE3 - 1 and ZONE1 - 1, alone and with most gameplay mods) and native tests; not yet in a played Windows campaign. Other regular stages that hit the same limit are not catalogued. Partial recompilation with mostly interpreted gameplay; all mod combinations, physical five-player play and broad Linux compatibility remain unqualified; no netplay. Downloads contain no ROM, generated game code/binary, extracted assets, saves or raw captures.

## Português

- **Portal da ZONE3 - 1 com rostos na HUD:** com **Battle character faces in Normal HUD** ligado, o portal de saída da ZONE3 - 1 não aparecia depois de derrotar todos os inimigos: o gráfico dos rostos ocupava a memória de sprites de que o portal precisa. Agora, quando o jogo não consegue carregar um gráfico por falta dessa memória enquanto os rostos estão na tela, os rostos são removidos pela rotina do próprio jogo e a HUD volta aos ícones originais até a próxima fase, e o portal aparece. Fases em que o portal cabe (como a ZONE1 - 1) mantêm os rostos.

Inclui tudo da v1.1.0 (spawns de cinco jogadores junto de P1/P2 e o mod opcional de spawns nos cantos). Instale numa pasta nova e gravável e faça **Generate & rebuild** com sua ROM JPN compatível do SB5; um jogo gerado antes não recebe esta correção. Baixe o ZIP Windows x64 ou o AppImage Linux x86_64 experimental e confira SHA256SUMS. Internet para as primeiras ferramentas; Rust não é necessário.

A correção foi verificada no Linux com rotas roteirizadas de fase forçada (ZONE3 - 1 e ZONE1 - 1, sozinha e com a maioria dos mods de jogo) e testes nativos; ainda não numa campanha jogada no Windows. Outras fases comuns que batem no mesmo limite não foram catalogadas. Recompilação parcial, principalmente interpretada; combinações de mods, cinco controles físicos e compatibilidade ampla no Linux não qualificados; sem netplay. Downloads sem ROM, código/executável gerado do jogo, assets extraídos, saves ou capturas brutas.
'''

if version == '1.1.2':
    notes = f'''# Super Bomberman 5 v{version}

## English

Five-player Normal Game fixes (**5 Players** mod) and picker navigation:

- **ZONE5 - J final boss:** no more garbled sprites or broken colors with three to five players; extra seats now share the game's sprite palettes correctly.
- **Zone 5 slowdown** with five players removed, and the **ZONE5 - 3** exit gate appears again.
- **Player colors:** P3-P5 keep their own colors in normal stages; they borrow P1's colors only while a stage uses every sprite palette.
- **No blinking while walking:** P3-P5 no longer flicker or show garbled frames when tapping or holding a direction.
- **ZONE5 - 1 tower shocks** hit every player, and **ZONE5 - 25 moving platforms** carry P3-P5.
- **Lives reset** when leaving the world map for the title or file menu.
- **Start rejoin** no longer shows a garbled sprite while a Louie is on the field.
- **ZONE3 - D:** after returning to the world map, a benched player no longer keeps the Pirate's Black Louie or another player's colors.
- **Back in the Normal character picker:** B from any controller returns to the file menu; B on the world map before entering a stage reopens the character picker (Normal character selection mod).

Install into a new writable folder and run **Generate & rebuild** with your own supported JPN SB5 ROM; an older generated game does not get these fixes. Download the Windows x64 ZIP or experimental Linux x86_64 AppImage and verify SHA256SUMS. Internet is needed for the first tools; Rust is not required.

Verified on Linux LLE with scripted five-seat routes (forced Zone 5 stages, ZONE3 - D, picker and map routes) and native tests, and confirmed by the maintainer on a Windows build. Partial recompilation with mostly interpreted gameplay; all mod combinations, physical five-player play and broad Linux compatibility remain unqualified; no netplay. Downloads contain no ROM, generated game code/binary, extracted assets, saves or raw captures.

## Português

Correções do Normal Game com cinco jogadores (mod **5 Players**) e navegação do seletor:

- **Chefe final da ZONE5 - J:** sem sprites embaralhados ou cores quebradas com três a cinco jogadores; os assentos extras agora dividem corretamente as paletas de sprite do jogo.
- **Lentidão da Zona 5** com cinco jogadores eliminada, e o portal de saída da **ZONE5 - 3** volta a aparecer.
- **Cores dos jogadores:** P3-P5 mantêm as próprias cores nas fases comuns; só usam as cores do P1 enquanto uma fase ocupa todas as paletas de sprite.
- **Sem piscar ao andar:** P3-P5 não piscam nem mostram quadros embaralhados ao tocar ou segurar o direcional.
- **Choques da torre da ZONE5 - 1** atingem todos os jogadores, e as **plataformas móveis da ZONE5 - 25** carregam P3-P5.
- **Vidas reiniciadas** ao sair do mapa para o título ou o menu de arquivos.
- **Volta com Start** sem sprite embaralhado quando há um Louie em campo.
- **ZONE3 - D:** ao voltar ao mapa, um jogador fora da luta não fica mais com o Louie Preto do Pirata nem com as cores de outro jogador.
- **Voltar no seletor de personagem do Normal:** B em qualquer controle volta ao menu de arquivos; B no mapa, antes de entrar numa fase, reabre o seletor (mod de seleção de personagem no Normal).

Instale numa pasta nova e gravável e faça **Generate & rebuild** com sua ROM JPN compatível do SB5; um jogo gerado antes não recebe estas correções. Baixe o ZIP Windows x64 ou o AppImage Linux x86_64 experimental e confira SHA256SUMS. Internet para as primeiras ferramentas; Rust não é necessário.

Verificado no Linux LLE com rotas roteirizadas de cinco assentos (fases da Zona 5 forçadas, ZONE3 - D, seletor e mapa) e testes nativos, e confirmado pelo mantenedor num build Windows. Recompilação parcial, principalmente interpretada; combinações de mods, cinco controles físicos e compatibilidade ampla no Linux não qualificados; sem netplay. Downloads sem ROM, código/executável gerado do jogo, assets extraídos, saves ou capturas brutas.
'''

if version in ('2.0.0', '2.0.1', '2.0.2'):
    notes = f'''# Super Bomberman 5 v{version}

## English

- **Netplay:** **NETPLAY** in the launcher, like Mega Man X Recompiled: **Online Netplay** on the shared lobby server (sign in with Discord) or **LAN / Direct IP**. Every player needs this same version and their own ROM; the room adopts the host's mods. Synchronization is lockstep (delay-sync). Validated between two real executables on one machine (7,600 frames including Battle with every gameplay mod on); a real internet match and three or more peers are not yet qualified. See docs/NETPLAY.md.
- **Discord:** **Discord: online matches & news** opens the Super Bomberman 5 server to find matches and follow updates: https://discord.gg/kH54NCDQB4
- **Updater:** **Update game / Atualizar jogo** in the launcher (or `updater/SuperBomberman5Updater`) installs the newest GitHub release in place after checking `SHA256SUMS`, keeping your ROM, saves, settings and Mods selection; then run **Generate & rebuild** once. Verified end to end on Linux; the Windows path is new. See docs/UPDATER.md.
- **Cheats:** infinite time now lives in a **Cheats** group, joined by the new **Unlimited lives in Normal Game** (no Game Over; works with five players).
- **Box art** on the launcher's ROM panel, and releases ready for **Quiver Launcher** (catalog entry submitted; see docs/QUIVER.md).
- Framework updated to the engine revisions used by Mega Man X Recompiled 2.3.1.

Coming from v1.x: download the Windows x64 ZIP or the experimental Linux x86_64 AppImage, verify SHA256SUMS, install (a new folder or over the old one) and run **Generate & rebuild** with your own Super Bomberman 5 (USA) ROM. Internet is needed for the first tools; Rust is not required.

Partial recompilation with mostly interpreted gameplay; all mod combinations, physical five-player play and broad Linux compatibility remain unqualified. Downloads contain no ROM, generated game code/binary, extracted assets, saves or raw captures.

## Português

- **Netplay:** **NETPLAY** no launcher, como no Mega Man X Recompiled: **Online Netplay** no servidor de lobby compartilhado (entre com o Discord) ou **LAN / Direct IP**. Todos precisam desta mesma versão e da própria ROM; a sala adota os mods do anfitrião. A sincronização é lockstep (delay-sync). Validado entre dois executáveis reais na mesma máquina (7.600 quadros, incluindo Battle com todos os mods de jogo); partida real pela internet e três ou mais peers ainda não foram qualificados. Veja docs/NETPLAY.md.
- **Discord:** **Discord: online matches & news** abre o servidor do Super Bomberman 5 para achar partidas e acompanhar novidades: https://discord.gg/kH54NCDQB4
- **Atualizador:** **Update game / Atualizar jogo** no launcher (ou `updater/SuperBomberman5Updater`) instala a release mais nova do GitHub na mesma pasta depois de conferir o `SHA256SUMS`, mantendo ROM, saves, configurações e mods escolhidos; depois faça **Generate & rebuild** uma vez. Verificado de ponta a ponta no Linux; o caminho do Windows é novo. Veja docs/UPDATER.md.
- **Cheats:** o tempo infinito agora fica no grupo **Cheats**, junto com o novo **Unlimited lives in Normal Game** (sem Game Over; funciona com cinco jogadores).
- **Capa** no painel da ROM do launcher, e releases prontas para o **Quiver Launcher** (cadastro enviado; veja docs/QUIVER.md).
- Framework atualizado para as revisões usadas pelo Mega Man X Recompiled 2.3.1.

Vindo da v1.x: baixe o ZIP Windows x64 ou o AppImage Linux x86_64 experimental, confira o SHA256SUMS, instale (numa pasta nova ou por cima da antiga) e faça **Generate & rebuild** com sua ROM do Super Bomberman 5 (USA). Internet para as primeiras ferramentas; Rust não é necessário.

Recompilação parcial, principalmente interpretada; combinações de mods, cinco controles físicos e compatibilidade ampla no Linux não qualificados. Downloads sem ROM, código/executável gerado do jogo, assets extraídos, saves ou capturas brutas.
'''

if version in ('2.0.1', '2.0.2'):
    fix_en = '''**Fix in v2.0.1:** **Generate & rebuild** failed on Windows when the game folder's path had accents (for example OneDrive's "Área de Trabalho"), stopping at "Configure failed … does not appear to contain CMakeLists.txt". The 2.0 framework runs in UTF-8, and the rebuild script wrote the folder name in UTF-8 while the Windows console reads it in its own code page, so the build shortcut under C:\\Users\\Public\\retcomm-builds pointed at a folder that does not exist. The script now uses the console's code page and replaces a broken shortcut left by v2.0.0. Install v2.0.1 (a new folder or over 2.0.0) and run **Generate & rebuild**. Everything below is unchanged from v2.0.0.

'''
    if version == '2.0.2':
        fix_en = '''**New in v2.0.2:** online netplay rooms now have **five seats**, one for each Super Bomberman 5 player (they stopped at four). Every player needs v2.0.2. LAN / Direct IP rooms stay two seats. Joining an online room now switches your mods to the host's automatically (the room used to close when they differed with many mods on); with Bomber Kings you still need your own Super Bomberman 4 ROM. Matches with more than two peers have not been played yet. The updater now deletes its temporary files (download and unpacked package) when it finishes, plus leftovers of earlier runs.\n\n''' + fix_en.replace('**Fix in v2.0.1:**', '**Also included (v2.0.1):**')
    fix_pt = '''**Correção na v2.0.1:** o **Generate & rebuild** falhava no Windows quando o caminho da pasta do jogo tinha acento (por exemplo a "Área de Trabalho" do OneDrive), parando em "Configure failed … does not appear to contain CMakeLists.txt". O framework da 2.0 roda em UTF-8, e o script de rebuild gravava o nome da pasta em UTF-8 enquanto o console do Windows o lê no próprio code page; o atalho de build em C:\\Users\\Public\\retcomm-builds apontava para uma pasta inexistente. Agora o script usa o code page do console e refaz o atalho quebrado deixado pela v2.0.0. Instale a v2.0.1 (pasta nova ou por cima da 2.0.0) e faça **Generate & rebuild**. O restante é igual à v2.0.0.

'''
    if version == '2.0.2':
        fix_pt = '''**Novo na v2.0.2:** as salas do netplay online agora têm **cinco assentos**, um para cada jogador do Super Bomberman 5 (antes paravam em quatro). Todos precisam da v2.0.2. Salas LAN / IP direto continuam com dois assentos. Ao entrar numa sala online, seus mods passam a ser os do anfitrião automaticamente (antes a sala fechava quando eram diferentes com muitos mods ligados); com Bomber Kings ainda é preciso a sua própria ROM do Super Bomberman 4. Partidas com mais de dois peers ainda não foram jogadas. O atualizador agora apaga os arquivos temporários (download e pacote extraído) ao terminar, além de sobras de execuções antigas.\n\n''' + fix_pt.replace('**Correção na v2.0.1:**', '**Também incluído (v2.0.1):**')
    notes = notes.replace('## English\n\n', '## English\n\n' + fix_en, 1).replace('## Português\n\n', '## Português\n\n' + fix_pt, 1)

# From v2.0.3 on, each version lists only what is new in it. The full notes add
# the install/upgrade paragraph; --whats-new (the Discord announcement) does not.
WHATS_NEW = {
    '2.5.7': (
'''- **Fix: online rooms with every mod on (second part):** v2.5.6 sent the mod configuration, but peers still rebuilt part of it from the mod list, which also names mods that only change menus and text and are not compared; the result never matched, not even on the host, and Play kept failing with `mod_set_mismatch`. The host now sends the exact configuration in a compact form and every peer reads it back unchanged.
- Everyone in an online room needs v2.5.7.''',
'''- **Correção: salas online com todos os mods ligados (segunda parte):** a v2.5.6 enviava a configuração de mods, mas os jogadores ainda remontavam parte dela pela lista de mods, que também traz mods que só mudam menus e textos e não entram na comparação; o resultado nunca batia, nem no anfitrião, e o Play continuava falhando com `mod_set_mismatch`. Agora o anfitrião envia a configuração exata em forma compacta e todos a leem sem mudanças.
- Todos na sala online precisam da v2.5.7.'''),
    '2.5.6': (
'''- **Fix: online rooms with every mod on:** with 16 mods enabled the host's mod configuration did not fit the lobby, so guests rebuilt it without the mods' options and the match never started (`mod_set_mismatch`, guest stuck on "Not ready"). The host now sends the mod list plus only the options, and guests match it exactly.
- Everyone in an online room needs v2.5.6.''',
'''- **Correção: salas online com todos os mods ligados:** com 16 mods ativos a configuração de mods do anfitrião não cabia no lobby, os convidados a montavam sem as opções dos mods e a partida nunca começava (`mod_set_mismatch`, convidado preso em "Not ready"). Agora o anfitrião envia a lista de mods e só as opções, e os convidados ficam exatamente iguais.
- Todos na sala online precisam da v2.5.6.'''),
    '2.5.5': (
'''- **Linux: no more missing-library errors at startup.** The AppImage now carries the PulseAudio sound libraries and uses them only when the system lacks them (WSL and minimal installs). If no sound device can be opened at all, the game starts without sound instead of closing.
- **WSL: full speed:** WSL could draw the game without the graphics card (software rendering), so it ran slow. On WSL the AppImage now asks for the GPU (D3D12) when Windows exposes it. WSL's own sound output (RDP) may still crackle even when the game keeps its sound flowing; this is a WSLg limitation.
- **Slow PCs keep full speed:** when a frame takes too long, the next one is not drawn (never two in a row) so the game and its sound keep their speed instead of slowing down and crackling. The sound buffer also grows on its own when the sound device asks for large chunks. The log now shows the renderer in use and, while the game runs slow, where each frame's time goes.
- On Windows only the frame skip on slow PCs changes. Everyone in an online room needs v2.5.5.''',
'''- **Linux: sem erros de biblioteca faltando ao abrir.** O AppImage agora traz as bibliotecas de som do PulseAudio e só as usa quando o sistema não tem (WSL e instalações mínimas). Se nenhum dispositivo de som abrir, o jogo inicia sem som em vez de fechar.
- **WSL: velocidade normal:** o WSL podia desenhar o jogo sem a placa de vídeo (renderização por software), deixando-o lento. No WSL o AppImage agora pede a GPU (D3D12) quando o Windows a disponibiliza. A saída de som do próprio WSL (RDP) ainda pode chiar mesmo com o jogo entregando o som em dia; é uma limitação do WSLg.
- **PCs lentos mantêm a velocidade:** quando um quadro demora demais, o seguinte não é desenhado (nunca dois seguidos), e o jogo e o som mantêm a velocidade em vez de ficarem lentos e chiados. A reserva de som também cresce sozinha quando o dispositivo de som pede blocos grandes. O log agora mostra o renderizador em uso e, quando o jogo fica lento, onde vai o tempo de cada quadro.
- No Windows só muda o pulo de quadros em PCs lentos. Todos na sala online precisam da v2.5.5.'''),
    '2.5.4': (
'''- **Fix: Linux setup on Ubuntu, Debian, Mint and WSL:** **Generate & rebuild** stopped with "CMAKE_CXX_COMPILER not set". The portable toolchain's setup script needs bash, but the launcher ran it with `/bin/sh`, which on these systems is dash; the compiler path came out wrong. The launcher now uses bash.
- **Fix: Linux toolchain download:** on a fresh Ubuntu (no `unzip`) the launcher showed "Failed to extract the toolchain zip". It now also extracts with `bsdtar` or `python3`.
- Windows is unchanged. Everyone in an online room needs v2.5.4.''',
'''- **Correção: setup no Linux (Ubuntu, Debian, Mint e WSL):** o **Generate & rebuild** parava com "CMAKE_CXX_COMPILER not set". O script do toolchain portátil precisa do bash, mas o launcher o rodava com `/bin/sh`, que nesses sistemas é o dash; o caminho do compilador saía errado. Agora o launcher usa o bash.
- **Correção: download do toolchain no Linux:** num Ubuntu recém-instalado (sem `unzip`) o launcher mostrava "Failed to extract the toolchain zip". Agora ele também extrai com `bsdtar` ou `python3`.
- O Windows não muda. Todos na sala online precisam da v2.5.4.'''),
    '2.5.3': (
'''- **Fix: Boss Rush ranking with the 5 Players mod:** a rush with 3, 4 or 5 players was sent to the Discord ranking as a 2-player time (with only two names). It now goes to its own category with every player. The wrong 2-player time sent this way was removed from the ranking.
- Everyone in an online room needs v2.5.3.''',
'''- **Correção: ranking do Boss Rush com o mod de 5 jogadores:** um boss rush com 3, 4 ou 5 jogadores ia para o ranking do Discord como tempo de 2 jogadores (com só dois nomes). Agora vai para a categoria certa, com todos os jogadores. O tempo de 2 jogadores enviado errado dessa forma foi removido do ranking.
- Todos na sala online precisam da v2.5.3.'''),
    '2.5.2': (
'''- **Boss Rush:** save and load states (hotkeys, slot menu and rewind) are off from the start of a rush until the title screen, so ranked times are played straight; a notice shows when the rush starts.
- **Fix: Boss Rush top 5 with the 5 Players mod:** on the player-count menu, entries 3, 4 and 5 showed the 2P ranking; each now shows its own.
- **Fix: Clock and Louies:** picking up a Clock froze the players' Louies (mostly with the 5 Players mod, and with Terrorin's Clock in Battle); it now freezes only enemies.
- **Abilities:** while a B + direction ability recovers (debuff), that player's bombs reach one tile. Terrorin's debuff lasts twice as long (about 21 s).
- **Hammer and Lady:** the mace and the laser never deal damage. In Battle they stun and take items as before; in Normal Game and the Boss Rush they leave enemies and bosses alone and only stun the rival bombers of boss fights (they used to damage bosses).
- Everyone in an online room needs v2.5.2.''',
'''- **Boss Rush:** salvar e carregar estados (atalhos, menu de slots e rewind) ficam bloqueados do início do boss rush até a tela de título, para os tempos do ranking serem jogados de verdade; um aviso aparece quando ele começa.
- **Correção: top 5 do Boss Rush com o mod de 5 jogadores:** no menu de jogadores, as opções 3, 4 e 5 mostravam o ranking de 2P; agora cada uma mostra o seu.
- **Correção: Clock e Louies:** pegar um Clock congelava os Louies dos jogadores (principalmente com o mod de 5 jogadores, e com o Clock do Terrorin no Battle); agora ele congela só os inimigos.
- **Habilidades:** enquanto uma habilidade B + direção se recupera (debuff), as bombas desse jogador alcançam só 1 tile. O debuff do Terrorin dura o dobro (cerca de 21 s).
- **Hammer e Lady:** a maça e o laser nunca causam dano. No Battle continuam atordoando e tirando itens; no Normal Game e no Boss Rush não afetam inimigos nem chefes e só atordoam os bombers rivais das lutas de chefe (antes causavam dano nos chefes).
- Todos na sala online precisam da v2.5.2.'''),
    '2.5.1': (
'''- **Boss Rush online ranking (Discord):** the five best times for each player count (1-5) are kept in a message of the project's Discord channel, and every new top-5 time posts a notice there. When connected, the Boss Rush player-count menu shows the top 5 of the count under the cursor. At the end of an online rush (beating Terrorin) the room host sends the time with every player's name and character; offline, when the time makes the top 5, each player types a name and chooses whether to send it. Without internet only the time is shown, and the game never waits.
- The Boss Rush timer stops for good when Terrorin is beaten (it used to keep counting through the ending).
- Everyone in an online room needs v2.5.1.''',
'''- **Ranking online do Boss Rush (Discord):** os cinco melhores tempos de cada quantidade de jogadores (1 a 5) ficam numa mensagem do canal do Discord do projeto, e cada novo tempo no top 5 gera um aviso lá. Conectado, o menu de jogadores do Boss Rush mostra o top 5 da quantidade sob o cursor. No fim de um boss rush online (ao vencer o Terrorin) o anfitrião da sala envia o tempo com o nome e o personagem de cada jogador; offline, quando o tempo entra no top 5, cada jogador digita o nome e escolhe se quer enviar. Sem internet aparece só o tempo, e o jogo nunca espera.
- O cronômetro do Boss Rush para de vez ao vencer o Terrorin (antes continuava contando durante o final).
- Todos na sala online precisam da v2.5.1.'''),
    '2.5.0': (
'''- **New mod: Boss Rush** (Mods → Gameplay): **BOSS RUSH** on the title menu, between BATTLE GAME and OPTIONS. It is the cartridge's own boss rush (password **3879**): the player-count menu first (up to five players with the 5 Players mod), then, with **Normal Game — character selection** on, the character picker (with colors when that mod is on; **B** returns to the title). The characters and colors chosen stay for the whole rush. A run timer (`MM:SS.CC`) sits at the top right, and the pause menu offers **Return to title**. In PT-BR: **CORRIDA DE CHEFES**, and the title menu reads JOGO NORMAL / MODO BATALHA / OPÇÕES. Disabled by default. [Details](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/blob/main/docs/BOSS_RUSH.md)
- **Launcher on Direct3D 11 (Windows):** the launcher now renders with Direct3D 11 instead of OpenGL, falling back to SDL's other renderers when D3D11 is unavailable. The game window is unchanged.
- Everyone in an online room needs v2.5.0.''',
'''- **Novo mod: Boss Rush** (Mods → Gameplay): **BOSS RUSH** no menu da tela de título, entre BATTLE GAME e OPTIONS. É o boss rush do próprio cartucho (senha **3879**): primeiro a escolha de jogadores (até cinco com o mod de 5 jogadores) e, com o **Normal Game — character selection** ligado, a tela de personagens (com as cores quando esse mod está ligado; **B** volta ao título). Os personagens e cores escolhidos valem para todo o boss rush. Um cronômetro (`MM:SS.CC`) fica no canto superior direito e o menu de pausa oferece **Voltar ao título**. Em PT-BR: **CORRIDA DE CHEFES**, e o menu de título mostra JOGO NORMAL / MODO BATALHA / OPÇÕES. Desligado por padrão. [Detalhes](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/blob/main/docs/BOSS_RUSH.md)
- **Launcher em Direct3D 11 (Windows):** o launcher agora desenha com Direct3D 11 em vez de OpenGL, usando os outros renderizadores do SDL quando o D3D11 não está disponível. A janela do jogo não muda.
- Todos na sala online precisam da v2.5.0.'''),
    '2.4.3': (
'''- **Battle — stage variants: the minimap is now the arena itself**, drawn from your cartridge when the mod is enabled: rails (stage 8), conveyor belts (stage 2), ice, tunnels and the other stage gimmicks appear where each variant puts them, with the P1–P5 starts on top.
- **Menu sounds on the variant screen:** opening it plays the Battle menu's confirm sound, moving plays the cursor sound and **B** plays the back sound as it returns to the arena carousel.
- Includes v2.4.1–v2.4.2 (Start opens the list; held keys no longer confirm it). Everyone in an online room needs v2.4.3.''',
'''- **Battle — stage variants: a miniatura agora é a própria arena**, desenhada a partir do seu cartucho quando o mod é ativado: trilhos (fase 8), esteiras (fase 2), gelo, túneis e as outras mecânicas das fases aparecem onde cada variante as coloca, com os pontos de início de P1–P5 por cima.
- **Sons do menu na tela de variantes:** abrir toca o som de confirmação do menu do Battle, mover toca o som do cursor e o **B** toca o som de voltar ao retornar ao carrossel de arenas.
- Inclui a v2.4.1–v2.4.2 (Start abre a lista; tecla segurada não confirma mais). Todos na sala online precisam da v2.4.3.'''),
    '2.4.2': (
'''- **Fix: the Battle variant list confirmed itself:** with **Battle — stage variants** on, the list appeared for a few frames and the match started before you could choose, because a key still held (a normal keyboard press lasts several frames) was read as a new press. Holding A, Start or B no longer does anything extra: the list waits for a new press, and after it closes the arena carousel ignores keys until they are released.
- Includes v2.4.1 (Start also opens the list). Everyone in an online room needs v2.4.2.''',
'''- **Correção: a lista de variantes do Battle se confirmava sozinha:** com o **Battle — stage variants** ligado, a lista aparecia por poucos quadros e a partida começava antes da escolha, porque uma tecla ainda segurada (um toque normal no teclado dura vários quadros) era lida como um novo toque. Segurar A, Start ou B não faz mais nada a mais: a lista espera um novo toque e, depois de fechar, o carrossel de arenas ignora as teclas até elas serem soltas.
- Inclui a v2.4.1 (Start também abre a lista). Todos na sala online precisam da v2.4.2.'''),
    '2.4.1': (
'''- **Fix: Battle — stage variants with Start:** confirming a Battle arena with **Start** (Enter on the keyboard) skipped the variant screen and started the match. **Start** now opens the list as **A** does, and also confirms a variant.
- Includes everything from v2.4.0 (the Battle — stage variants mod). Everyone in an online room needs v2.4.1.''',
'''- **Correção: Battle — stage variants com Start:** confirmar a arena do Battle com **Start** (Enter no teclado) pulava a tela de variantes e começava a partida. Agora o **Start** abre a lista como o **A** e também confirma a variante.
- Inclui tudo da v2.4.0 (o mod Battle — stage variants). Todos na sala online precisam da v2.4.1.'''),
    '2.4.0': (
'''- **New mod: Battle — stage variants** (Mods → Gameplay): after choosing an arena, a screen lets you play its **Original** layout or **Variant 1–4**, the layouts the cartridge only unlocks with passwords **4622**, **0413** and **0926**, plus the **0110** rule that turns every soft block into an item (arena 1, the only stage with that rule). A minimap shows the hard blocks and where P1–P5 start; **B** goes back to the arena carousel, **A** starts the match as usual. In PT-BR with the translation mod. Disabled by default. [Details](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/blob/main/docs/BATTLE_VARIANTS.md)
- **COM level 4** (v2.3.0) is now confirmed in game: with the mod on, the Battle rules reach level 4.
- Online rooms adopt the host's mods as before; everyone in a room needs v2.4.0.''',
'''- **Novo mod: Battle — stage variants** (Mods → Gameplay): depois de escolher a arena, uma tela deixa jogar o layout **Original** ou as **Variantes 1–4**, os layouts que o cartucho só libera com as senhas **4622**, **0413** e **0926**, além da regra **0110**, que transforma todos os blocos macios em itens (arena 1, a única fase com essa regra). Uma miniatura mostra os blocos fixos e onde P1–P5 começam; **B** volta ao carrossel de arenas, **A** começa a partida normalmente. Em PT-BR com o mod de tradução. Desligado por padrão. [Detalhes](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/blob/main/docs/BATTLE_VARIANTS.md)
- O **nível 4 do COM** (v2.3.0) agora foi confirmado no jogo: com o mod ligado, as regras do Battle chegam ao nível 4.
- As salas online continuam adotando os mods do anfitrião; todos na sala precisam da v2.4.0.'''),
    '2.3.3': (
'''- **Fix: a failing rebuild started again on every launch:** when the automatic Generate & rebuild failed on Windows, opening the game started it (and failed) again each time, so the launcher, and its Update button, could not be reached. After a failed rebuild the launcher now opens normally with the Generate & rebuild button; a successful rebuild turns the automatic start back on.
- Includes v2.3.2's fix for rebuilding after renaming or moving the game folder ("CMake step for libjuice failed").
- Same online netplay as v2.3.2; everyone in a room needs v2.3.3.''',
'''- **Correção: rebuild com falha recomeçava a cada abertura:** quando o Generate & rebuild automático falhava no Windows, abrir o jogo começava (e falhava) de novo toda vez, sem chegar ao launcher e ao botão de atualizar. Depois de um rebuild com falha o launcher agora abre normalmente com o botão Generate & rebuild; um rebuild bem-sucedido religa o início automático.
- Inclui a correção da v2.3.2 para o rebuild depois de renomear ou mover a pasta do jogo ("CMake step for libjuice failed").
- Mesmo jogo online da v2.3.2; todos na sala precisam da v2.3.3.'''),
    '2.3.2': (
'''- **Fix: Generate & rebuild after renaming or moving the game folder:** v2.3.1 configured the build again but then stopped at "CMake step for libjuice failed" ("The current CMakeCache.txt directory ... is different"), because a downloaded library kept its own build settings from the old folder. Those are now reset too, and the rebuild finishes. If v2.3.1 showed that error, update and run Generate & rebuild again (it starts by itself).
- Same online netplay as v2.3.1; everyone in a room needs v2.3.2.''',
'''- **Correção: Generate & rebuild depois de renomear ou mover a pasta do jogo:** a v2.3.1 configurava o build de novo, mas parava em "CMake step for libjuice failed" ("The current CMakeCache.txt directory ... is different"), porque uma biblioteca baixada guardava as configurações de build da pasta antiga. Agora elas também são refeitas e o rebuild termina. Se a v2.3.1 mostrou esse erro, atualize e rode o Generate & rebuild de novo (ele começa sozinho).
- Mesmo jogo online da v2.3.1; todos na sala precisam da v2.3.2.'''),
    '2.3.1': (
'''- **Fix: Generate & rebuild silently kept the old game:** in a game folder that had been renamed or moved, every rebuild stopped at "Error: could not load cache", and the rebuild window then started the previously built game as if it had worked, so new versions (and the v2.3.0 **Battle — COM level 4** mod) never appeared. The rebuild now configures again in that case, and a failed build stops with "Build failed" instead of opening the old game.
- **Generate & rebuild starts by itself after an update** when the game was already built and your ROM is known; if it fails, the button is still there.
- **Updater progress:** download bar with percentage, speed and time left, unpacking timer and an install bar counting files. It shows from the next update on, because the updater already installed is the one that downloads a new version.
- Same online netplay as v2.3.0; everyone in a room needs v2.3.1.''',
'''- **Correção: Generate & rebuild mantinha o jogo antigo sem avisar:** numa pasta do jogo que foi renomeada ou movida, todo rebuild parava em "Error: could not load cache" e a janela do rebuild abria o jogo compilado antes como se tivesse dado certo, então versões novas (e o mod **Battle — COM level 4** da v2.3.0) não apareciam. Agora o rebuild reconfigura nesse caso, e um build com falha para em "Build failed" em vez de abrir o jogo antigo.
- **O Generate & rebuild começa sozinho depois de atualizar** quando o jogo já estava compilado e a sua ROM é conhecida; se falhar, o botão continua lá.
- **Progresso no atualizador:** barra de download com porcentagem, velocidade e tempo restante, cronômetro da extração e barra de instalação contando os arquivos. Aparece a partir da próxima atualização, porque é o atualizador já instalado que baixa a versão nova.
- Mesmo jogo online da v2.3.0; todos na sala precisam da v2.3.1.'''),
    '2.3.0': (
'''- **New mod: Battle — COM level 4:** in **Mods → Cheats**, turn on **COM level 4 (Super) in Battle** and the Battle settings let you choose the Super COM level, the one the cartridge only unlocks with password **1103**. Your save file is not changed: turn the mod off and the stock three levels are back. Disabled by default. [Details](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/blob/main/docs/COM_LEVEL_4.md)
- Online rooms adopt the host's mods as before; everyone in a room needs v2.3.0.''',
'''- **Novo mod: Battle — COM level 4:** em **Mods → Cheats**, ligue **COM level 4 (Super) in Battle** e a configuração do Battle passa a oferecer o nível Super do COM, o mesmo que o cartucho só libera com a senha **1103**. O seu save não é alterado: desligando o mod, voltam os três níveis normais. Desligado por padrão. [Detalhes](https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/blob/main/docs/COM_LEVEL_4.md)
- As salas online continuam adotando os mods do anfitrião; todos na sala precisam da v2.3.0.'''),
    '2.2.10': (
'''- **Fix: "ninja: error: loading 'build.ninja'" on Generate & rebuild:** if the first build setup was interrupted or failed, every later **Generate & rebuild** skipped that step and stopped with this error, hiding the real cause. The rebuild now notices the incomplete build folder, sets it up again and, if something is still missing, shows the actual error. Linux and Windows.
- Same online netplay as v2.2.9; everyone in a room needs v2.2.10.''',
'''- **Correção: "ninja: error: loading 'build.ninja'" no Generate & rebuild:** se a primeira preparação do build fosse interrompida ou falhasse, todo **Generate & rebuild** seguinte pulava essa etapa e parava com esse erro, escondendo a causa real. Agora o rebuild percebe a pasta de build incompleta, prepara de novo e, se ainda faltar algo, mostra o erro de verdade. Linux e Windows.
- Mesmo jogo online da v2.2.9; todos na sala precisam da v2.2.10.'''),
    '2.2.9': (
'''- **Infinite lives stays at the maximum:** with **Normal Game — infinite lives** on, the shared counter now stays at 9 instead of only keeping its value. Players who press Start to rejoin the five-player game spend a life that is refilled right away, so they can always come back (the counter could sit at 0 and block rejoins).
- Same online netplay as v2.2.8; everyone in a room needs v2.2.9.''',
'''- **Vidas infinitas no máximo:** com **Normal Game — infinite lives** ligado, o contador compartilhado agora fica em 9 em vez de só manter o valor. Quem aperta Start para voltar no modo de cinco jogadores gasta uma vida que é reposta na hora, então sempre dá para renascer (antes o contador podia ficar em 0 e impedir a volta).
- Mesmo jogo online da v2.2.8; todos na sala precisam da v2.2.9.'''),
    '2.2.8': (
'''- **Fix: Battle stuck on the VICTORY screen:** when an extra character (for example Honey, Emperor Terrorin or a Bomber King) won a Battle, the champion screen never ended. It now goes on to the item roulette and the next match as usual (the champion pose stays Bomberman's, as before).
- **Fix: Super Bomberman 4 losers on the VICTORY screen:** the Bomber Kings who lost now show Super Bomberman 4's dejected loser pose instead of being electrocuted.
- Same online netplay as v2.2.7; everyone in a room needs v2.2.8.''',
'''- **Correção: Battle travado na tela VICTORY:** quando um personagem extra (por exemplo Honey, o Imperador Terrorin ou um Bomber King) vencia uma Battle, a tela do campeão nunca terminava. Agora ela segue para a roleta de itens e a próxima partida normalmente (a pose do campeão continua a do Bomberman, como antes).
- **Correção: perdedores do Super Bomberman 4 na tela VICTORY:** os Bomber Kings que perderam agora aparecem com a pose de derrota do Super Bomberman 4 em vez de eletrocutados.
- Mesmo jogo online da v2.2.7; todos na sala precisam da v2.2.8.'''),
    '2.2.7': (
'''- **Fix: "This is a setup build" on Play (Linux):** the AppImage kept its settings, ROM choice and mods beside the `.AppImage` file instead of in its game folder (`~/.local/share/superbomberman5/<version>`). The launcher then opened without the setup wizard and **Play** ended in "This is a setup build with no recompiled game code". The AppImage now uses its game folder again, so the launcher asks for your ROM and **Generate & rebuild** as it should. Settings, controls, saves and the netplay identity already written beside the AppImage are copied into the game folder once.
- Same online netplay as v2.2.6; everyone in a room needs v2.2.7.''',
'''- **Correção: "This is a setup build" no Play (Linux):** o AppImage guardava as configurações, a ROM escolhida e os mods ao lado do arquivo `.AppImage` em vez da pasta do jogo (`~/.local/share/superbomberman5/<versão>`). O launcher abria sem o assistente de instalação e o **Play** terminava em "This is a setup build with no recompiled game code". Agora o AppImage volta a usar a pasta do jogo, então o launcher pede a ROM e o **Generate & rebuild** como deveria. Configurações, controles, saves e a identidade do netplay já gravados ao lado do AppImage são copiados para a pasta do jogo uma vez.
- Mesmo jogo online da v2.2.6; todos na sala precisam da v2.2.7.'''),
    '2.2.6': (
'''- **Portable game folder:** the ROM paths saved by the launcher (rom.cfg) and by the Mods page (the Super Bomberman 4 ROM of Bomber Kings) are now kept relative to the game folder. A folder with the game already generated can be moved or copied to another drive or PC and opens straight on **Play**: it no longer asks where the ROMs are or sends you to **Generate & rebuild** when everything is in place.
- **ROMs found in the folder:** a valid Super Bomberman 5 or Super Bomberman 4 ROM in the game folder or its `roms/` subfolder is recognized by its checksum under any file name, and a path saved on another PC is looked up by its file name in the folder.
- **Rebuild after moving:** **Generate & rebuild** in a folder that was moved reconfigures the build instead of failing on the old folder's paths. The setup decides whether the generated game is current by its version, not by file dates that change when files are copied.
- Same online netplay as v2.2.5; everyone in a room needs v2.2.6.''',
'''- **Pasta do jogo portátil:** os caminhos das ROMs salvos pelo launcher (rom.cfg) e pela página Mods (a ROM do Super Bomberman 4 do Bomber Kings) agora ficam relativos à pasta do jogo. Uma pasta com o jogo já gerado pode ser movida ou copiada para outro disco ou PC e abre direto no **Play**: não pergunta mais onde estão as ROMs nem manda para o **Generate & rebuild** quando está tudo certo.
- **ROMs achadas na pasta:** uma ROM válida do Super Bomberman 5 ou do Super Bomberman 4 na pasta do jogo ou na subpasta `roms/` é reconhecida pelo checksum com qualquer nome de arquivo, e um caminho salvo em outro PC é procurado pelo nome do arquivo na pasta.
- **Rebuild depois de mudar de pasta:** o **Generate & rebuild** numa pasta que foi movida reconfigura o build em vez de falhar com os caminhos da pasta antiga. O setup decide se o jogo gerado está atualizado pela versão, não pelas datas dos arquivos, que mudam ao copiar.
- Mesmo jogo online da v2.2.5; todos na sala precisam da v2.2.6.'''),
    '2.2.5': (
'''- **Fix: "This is a setup build" on Linux:** after updating the AppImage, a player who had turned on skipping the launcher (or who opened the AppImage with a ROM) got the error "This is a setup build with no recompiled game code" instead of the game. The setup now always opens the launcher; run **Generate & rebuild** once and the AppImage opens the rebuilt game as usual.
- Same online netplay as v2.2.4; everyone in a room needs v2.2.5.''',
'''- **Correção: "This is a setup build" no Linux:** depois de atualizar o AppImage, quem tinha ligado a opção de pular o launcher (ou abria o AppImage com uma ROM) recebia o erro "This is a setup build with no recompiled game code" em vez do jogo. Agora o setup sempre abre o launcher; rode **Generate & rebuild** uma vez e o AppImage passa a abrir o jogo gerado normalmente.
- Mesmo jogo online da v2.2.4; todos na sala precisam da v2.2.5.'''),
    '2.2.4': (
'''- **Fix: choppy picture for online guests:** in v2.2.3 a guest repaid any delay at once, so when the host's frames arrived unevenly almost every other frame became a double step: the picture juddered and objects the game blinks on alternate frames (gates, labels, bombers) disappeared. The guest now catches up gradually, one extra frame every few frames, and only runs bursts when it is far behind. It also keeps one frame in reserve (about 16 ms more input lag for guests) so it stops less often.
- The "Slow PC" message now only appears when the PC itself is slow, not when the host's frames simply arrive unevenly.
- Everyone in a room needs v2.2.4. Not yet tried in a real internet match.''',
'''- **Correção: imagem tremida para o convidado online:** na v2.2.3 o convidado recuperava qualquer atraso de uma vez, então quando os quadros do anfitrião chegavam irregulares quase todo segundo quadro virava um passo duplo: a imagem tremia e objetos que o jogo pisca em quadros alternados (portais, nomes, bombers) sumiam. Agora o convidado recupera aos poucos, um quadro extra a cada alguns quadros, e só faz rajadas quando está bem atrás. Ele também guarda um quadro de reserva (cerca de 16 ms a mais de atraso nos controles do convidado) para parar menos.
- O aviso "Slow PC" agora só aparece quando o próprio PC está lento, não quando os quadros do anfitrião só chegam irregulares.
- Todos na sala precisam da v2.2.4. Ainda não testado numa partida real pela internet.'''),
    '2.2.3': (
'''- **Fix: online guest on a slow PC could not move:** a guest whose PC cannot hold 60 fps fell a little further behind the host every second, until the host stopped receiving its inputs and its bomber froze with no message. The guest now skips drawing the frames it uses to catch up (only the picture is skipped, the game runs the same) and catches up faster when far behind. The picture is choppier on that PC, but the match keeps up; the game shows "Slow PC: skipping frames to keep up with the host".
- **Black screen while waiting for players:** a match starts only when every player has loaded. Instead of a black screen, the game now says who it is waiting for ("Waiting for P3..."), and the netplay log lists each player's state.
- Everyone in a room needs v2.2.3. Not yet tried in a real internet match.''',
'''- **Correção: convidado online com PC lento não conseguia se mexer:** um convidado cujo PC não segura 60 fps ficava um pouco mais atrás do anfitrião a cada segundo, até o anfitrião deixar de receber os comandos dele e o personagem parar, sem aviso. Agora o convidado pula o desenho dos quadros que usa para alcançar o anfitrião (só a imagem é pulada, o jogo roda igual) e alcança mais rápido quando está muito atrás. A imagem fica menos fluida nesse PC, mas a partida acompanha; o jogo mostra "Slow PC: skipping frames to keep up with the host".
- **Tela preta esperando jogadores:** a partida só começa quando todos carregaram. Em vez da tela preta, o jogo agora diz quem falta ("Waiting for P3..."), e o log do netplay mostra a situação de cada jogador.
- Todos na sala precisam da v2.2.3. Ainda não testado numa partida real pela internet.'''),
    '2.2.2': (
'''- **Copy Invite:** in an online room, the **Copy Invite** button in the footer copies a message with the room code. Paste it in Discord, WhatsApp or any chat; your friend copies the whole message, opens **Online Netplay** and presses **Join from Invite** to enter the room (the password is asked if the room has one). Works even with Discord closed.
- **Fix: the host's save online:** the host now plays online on the same save as offline play. Every player who joins receives that save, and progress made online (cleared stages, Continue) is saved to the same file when the game closes. Before, the host entered online play with an empty save and online progress was lost. Guests keep their own save untouched.
- Online play was tested successfully in a real internet match, together with the Discord integration (status, invites and joining rooms). Find players on the Super Bomberman 5 Discord: https://discord.gg/kH54NCDQB4. Everyone in a room needs v2.2.2.''',
'''- **Copiar convite:** numa sala online, o botão **Copy Invite** no rodapé copia uma mensagem com o código da sala. Cole no Discord, WhatsApp ou qualquer chat; o amigo copia a mensagem inteira, abre o **Online Netplay** e clica em **Join from Invite** para entrar na sala (a senha é pedida se a sala tiver). Funciona mesmo com o Discord fechado.
- **Correção: o save do anfitrião no online:** o anfitrião agora joga online com o mesmo save do jogo offline. Todo jogador que entra recebe esse save, e o progresso feito online (fases liberadas, Continue) é gravado no mesmo arquivo ao fechar o jogo. Antes o anfitrião entrava online com o save vazio e o progresso online se perdia. O save dos convidados continua intocado.
- O jogo online foi testado com sucesso numa partida real pela internet, junto com a integração com o Discord (status, convites e entrada nas salas). Ache jogadores no Discord do Super Bomberman 5: https://discord.gg/kH54NCDQB4. Todos na sala precisam da v2.2.2.'''),
    '2.2.1': (
'''- **Less input lag for online guests:** a guest used to feel the round trip to the host plus the room's input delay, because after the first network hiccup their game kept that many frames queued. Guests now keep a small buffer that adjusts itself (one frame, more only while the connection jitters), send their controls the moment they change and play each frame as soon as the host's frame arrives. Quick taps are no longer lost when two inputs arrive together. The host still never waits for anyone, so the frame rate is not affected. Everyone in a room needs v2.2.1.
- **Fix: no exit gate in ZONE3 - 1 with HUD faces:** with the HUD faces mod, removing the faces to make room for the gate also took P1's and P2's colors away from them in the game's palette table. Other objects could then end up with a bomber's colors (the bridge in the middle of ZONE3 - 1) and the exit gate did not show, especially with the invincibility vest or five players. The faces now leave the players' colors alone.
- Not yet tried in a real internet match or a played Windows campaign.''',
'''- **Menos atraso nos controles do convidado online:** o convidado sentia a ida e volta até o anfitrião mais o atraso da sala, porque depois do primeiro soluço da rede o jogo dele guardava essa quantidade de quadros. Agora o convidado guarda um buffer pequeno que se ajusta sozinho (um quadro, mais só enquanto a conexão oscila), manda o controle assim que ele muda e roda cada quadro assim que o do anfitrião chega. Toques rápidos não se perdem mais quando duas entradas chegam juntas. O anfitrião continua sem esperar ninguém, então o FPS não muda. Todos na sala precisam da v2.2.1.
- **Correção: sem portal de saída na ZONE3 - 1 com os rostos na HUD:** com o mod de rostos na HUD, ao remover os rostos para abrir espaço para o portal, o jogo também tirava as cores do P1 e do P2 da tabela de paletas. Outros objetos podiam ficar com as cores de um bomber (a ponte no meio da ZONE3 - 1) e o portal não aparecia, principalmente com o colete de invencibilidade ou cinco jogadores. Agora os rostos não mexem nas cores dos jogadores.
- Ainda não testado numa partida real pela internet nem numa campanha jogada no Windows.'''),
    '2.2.0': (
'''- **Discord status:** with the Discord app open, your profile shows that you are playing Super Bomberman 5 Recomp and what you are doing: in the launcher, in an online room, in Normal Game or Battle, online or offline.
- **Invite friends from Discord:** while you are in an online room it shows in Discord as a party with free seats. Invite a friend with **+ → Invite to play** in a chat; when they click **Join**, their launcher opens Online Netplay and enters your room. **Ask to Join** on your profile is accepted while a seat is free, and on Windows Discord starts the game if it is closed. Password rooms, full rooms and LAN / Direct IP rooms are not published.
- Not yet tried with the real Discord client. Turn it off with `SB5_DISCORD=0`. Everyone in a room needs v2.2.0.''',
'''- **Status no Discord:** com o app do Discord aberto, o seu perfil mostra que você está jogando Super Bomberman 5 Recomp e o que está fazendo: no launcher, numa sala online, no Normal Game ou no Battle, online ou offline.
- **Convide amigos pelo Discord:** enquanto você está numa sala online, ela aparece no Discord como um grupo com vagas. Convide um amigo pelo **+ → Convidar para jogar** no chat; quando ele clicar em **Entrar**, o launcher dele abre o Online Netplay e entra na sua sala. O **Pedir para entrar** no seu perfil é aceito enquanto houver vaga, e no Windows o Discord abre o jogo se ele estiver fechado. Salas com senha, cheias e LAN / IP direto não são publicadas.
- Ainda não testado com o app do Discord de verdade. Para desligar, use `SB5_DISCORD=0`. Todos na sala precisam da v2.2.0.'''),
    '2.1.1': (
'''- **Fix: the guest's screen froze online:** when the host's friend had a short hiccup (a lost packet, a slow frame), their game never caught up with the host again. Each hiccup added lag to their controls, and after about one second behind, their screen froze for good (stuck or black) while their bomber stopped on the host's screen. The guest now runs a few extra frames to catch up with the host, the host resends what was lost, and a guest whose connection stops for more than about two seconds goes back to the room with a message instead of staying frozen. A single lost packet could also freeze a LAN / Direct IP guest; fixed too.
- **Netplay logs:** open the game through `tools/netplay-diag.cmd` (copy it next to `SuperBomberman5Recomp.exe`) to save, for each match, a report in `saves\\netplay` with stalls, frame rate, slow frames and how far the guest is behind the host. Send that folder from both players when reporting a problem.
- Not yet tried in a real internet match. Every online player needs v2.1.1.''',
'''- **Correção: a tela do convidado congelava no online:** quando o amigo do anfitrião tinha um engasgo rápido (um pacote perdido, um quadro lento), o jogo dele nunca mais alcançava o anfitrião. Cada engasgo somava atraso nos controles, e depois de cerca de um segundo atrás a tela dele congelava de vez (parada ou preta), enquanto o personagem dele parava na tela do anfitrião. Agora o convidado roda alguns quadros a mais para alcançar o anfitrião, o anfitrião reenvia o que se perdeu, e um convidado cuja conexão para por mais de uns dois segundos volta para a sala com uma mensagem em vez de ficar congelado. Um único pacote perdido também podia congelar o convidado na LAN / IP direto; corrigido também.
- **Logs do netplay:** abra o jogo pelo `tools/netplay-diag.cmd` (copie-o para o lado de `SuperBomberman5Recomp.exe`) para salvar, a cada partida, um relatório em `saves\\netplay` com paradas, FPS, quadros lentos e quanto o convidado está atrás do anfitrião. Ao relatar um problema, mande essa pasta dos dois jogadores.
- Ainda não testado numa partida real pela internet. Todos os jogadores online precisam da v2.1.1.'''),
    '2.1.0': (
'''- **Chat inside the match:** during an online match the room's chat stays on screen, bottom-left. New messages show for 5 seconds; press **T** to open the text bar with the recent history, **Enter** to send, **Esc** to cancel. While you type the keyboard does not move your bomber (a gamepad keeps playing). Not available in LAN / Direct IP matches.
- **Player names over the bombers:** each player's room name floats over the bomber they control while it is on the field and alive: it disappears when the bomber is hit and comes back when it respawns. COMs, players who are out and boss rivals have none.
- **Join a room while a match is running:** you wait in the room, chat with the players (they read it in game) and play from the next match.
- **Fix:** a spectator closing the game ended the match for every player. Now only a player leaving ends it.
- Not yet tried in a real internet match. Every online player needs v2.1.0.''',
'''- **Chat dentro da partida:** durante uma partida online o chat da sala continua na tela, no canto inferior esquerdo. Mensagens novas aparecem por 5 segundos; aperte **T** para abrir a barra de texto com o histórico recente, **Enter** para enviar e **Esc** para cancelar. Enquanto você digita, o teclado não move o seu personagem (o controle continua jogando). Não funciona em partidas LAN / IP direto.
- **Nome dos jogadores em cima dos personagens:** o nome de cada jogador na sala aparece em cima do personagem que ele controla, enquanto estiver em cena e vivo: some quando o personagem é atingido e volta quando ele renasce. COMs, jogadores eliminados e rivais dos chefes não têm nome.
- **Entrar na sala com a partida em andamento:** você espera na sala, conversa com os jogadores (eles leem dentro do jogo) e joga a partir da partida seguinte.
- **Correção:** um espectador que fechava o jogo encerrava a partida de todos os jogadores. Agora só a saída de um jogador encerra.
- Ainda não testado numa partida real pela internet. Todos os jogadores online precisam da v2.1.0.'''),
    '2.0.8': (
'''- **Launcher on older graphics cards:** the launcher crashed right after opening on PCs whose graphics driver has no OpenGL 3.3, such as Intel HD Graphics 2000/3000 (OpenGL 3.1). It now opens with OpenGL 3.0 or newer. On a PC without an OpenGL 3.0 driver (for example Windows' generic "Microsoft Basic Display Adapter"), it shows a message asking to install the graphics card driver instead of crashing.
- **Same mods for every online player:** when the host played without mods, players who joined kept their own mods on, so the games could differ. Now everyone starts with exactly the host's mods, and the ones the host does not use are switched off. If a player turns a mod back on while waiting in the room, it is switched off again before the match. Every player needs v2.0.8.''',
'''- **Launcher em placas de vídeo antigas:** o launcher fechava logo ao abrir em PCs cujo driver de vídeo não tem OpenGL 3.3, como Intel HD Graphics 2000/3000 (OpenGL 3.1). Agora ele abre com OpenGL 3.0 ou mais novo. Num PC sem driver com OpenGL 3.0 (por exemplo o "Adaptador de Vídeo Básico da Microsoft" do Windows), aparece uma mensagem pedindo para instalar o driver da placa de vídeo, em vez de fechar.
- **Mesmos mods para todos no online:** quando o anfitrião jogava sem mods, quem entrava na sala continuava com os próprios mods ligados, e os jogos podiam ficar diferentes. Agora todos começam exatamente com os mods do anfitrião, e os que ele não usa são desligados. Se um jogador religar um mod enquanto espera na sala, ele é desligado de novo antes da partida. Todos os jogadores precisam da v2.0.8.'''),
    '2.0.7': (
'''- **Online no longer slows down to wait for players:** the host now runs the match at full speed and never waits. Each frame it takes the newest input it has from every player and sends the result to everyone, so all games stay identical. Players far from the host feel their controls respond later (their own latency), instead of the whole match dropping frames. Every player needs v2.0.7.''',
'''- **O online não fica mais lento esperando jogadores:** o anfitrião roda a partida na velocidade normal e nunca espera. A cada quadro ele usa a entrada mais recente de cada jogador e envia o resultado para todos, então os jogos continuam idênticos. Quem está longe do anfitrião sente os controles responderem mais tarde (a própria latência), em vez de a partida inteira perder quadros. Todos os jogadores precisam da v2.0.7.'''),
    '2.0.6': (
'''- **Smoother online matches with 3+ players:** the automatic input delay was too short, so the game kept pausing to wait for the slowest player (a three-player test ran at about 29 fps). It now accounts for inputs that travel through the host and for the lockstep mode this game uses. With a high-latency player the match stays smooth but controls respond later for everyone; a wired connection helps the most.''',
'''- **Partidas online com 3+ jogadores mais fluidas:** o atraso de entrada automático era curto demais e o jogo ficava parando para esperar o jogador mais lento (um teste com três jogadores rodou a uns 29 fps). Agora ele considera que as entradas passam pelo anfitrião e o modo lockstep que este jogo usa. Com um jogador de latência alta a partida fica fluida, mas os controles respondem mais tarde para todos; usar cabo de rede ajuda bastante.'''),
    '2.0.5': (
'''- **Fix:** with three or more players in an online room, pressing PLAY did not start the match: the host's game did not open and the others stayed on a black screen. The match was being opened for two players only; it now uses every seated player. Matches with 3-5 players over the internet are still being tested.''',
'''- **Correção:** com três ou mais jogadores numa sala online, apertar PLAY não começava a partida: o jogo do anfitrião não abria e os outros ficavam com a tela preta. A partida era aberta só para dois jogadores; agora usa todos os que estão sentados. Partidas pela internet com 3 a 5 jogadores ainda estão em teste.'''),
    '2.0.4': (
'''- **Fix:** an online room closed as soon as a third player joined. With Super Bomberman 5's 14 mod packages, the room update sent to each player grew past the 4 KB the netplay library read at once, and the host was disconnected from the lobby. The lobby now reads updates for all five players; full 3-5 player matches have not been played yet.''',
'''- **Correção:** a sala online fechava assim que entrava o terceiro jogador. Com os 14 pacotes de mods do Super Bomberman 5, a atualização da sala enviada a cada jogador passava dos 4 KB que a biblioteca de netplay lia de uma vez, e o anfitrião era desconectado do lobby. Agora o lobby lê as atualizações dos cinco jogadores; partidas completas com 3 a 5 jogadores ainda não foram jogadas.'''),
    '2.0.3': (
'''- **Version in the launcher:** the header shows the installed version (for example "v2.0.3").
- **Update check:** on start the launcher asks GitHub for the newest release. When there is one, it says so above **Update game / Atualizar jogo**, explains on the Netplay pages why other players' rooms are missing (rooms only match the same version) and disables **Host Lobby** until you update.''',
'''- **Versão no launcher:** o cabeçalho mostra a versão instalada (por exemplo "v2.0.3").
- **Aviso de atualização:** ao abrir, o launcher consulta o GitHub. Se houver versão nova, avisa acima de **Update game / Atualizar jogo**, explica nas telas de Netplay por que as salas dos outros não aparecem (as salas só juntam a mesma versão) e desativa o **Host Lobby** até você atualizar.'''),
}

if version in WHATS_NEW:
    en, pt = WHATS_NEW[version]
    if args.whats_new:
        notes = f'''# Super Bomberman 5 v{version}

## English

{en}

## Português

{pt}
'''
    else:
        notes = f'''# Super Bomberman 5 v{version}

## English

{en}

Update with **Update game / Atualizar jogo** in the launcher (or download the Windows x64 ZIP / Linux x86_64 AppImage and verify SHA256SUMS), then run **Generate & rebuild** once with your own Super Bomberman 5 (USA) ROM. Every online player needs this version. Partial recompilation; see docs/STATUS.md for validation limits. Downloads contain no ROM, generated game code, extracted assets or saves.

## Português

{pt}

Atualize com **Update game / Atualizar jogo** no launcher (ou baixe o ZIP Windows x64 / AppImage Linux x86_64 e confira o SHA256SUMS) e faça **Generate & rebuild** uma vez com sua ROM do Super Bomberman 5 (USA). Todos os jogadores online precisam desta versão. Recompilação parcial; limites de validação em docs/STATUS.md. Os downloads não trazem ROM, código gerado do jogo, assets extraídos nem saves.
'''

if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(notes, encoding='utf-8', newline='\n')
else:
    sys.stdout.reconfigure(encoding='utf-8')
    print(notes, end='')
