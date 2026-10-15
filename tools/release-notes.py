"""Generate initial prototype release notes; keep output outside tracked source."""
from pathlib import Path
import argparse
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path)
parser.add_argument('--version', help='Version for correcting an existing release without changing VERSION.')
args = parser.parse_args()

version = args.version or (Path(__file__).resolve().parents[1] / 'VERSION').read_text().strip()
notes = f'''# Super Bomberman 5 v{version} — Windows prototype

Unofficial noncommercial prototype. **Partial recompilation:** most game logic still uses SNESRecomp's interpreter. The opening, title, menus, Zone 1 map and first-stage entry were verified locally over a scripted 3,628-frame route. Windows display/audio initialization and launcher smoke tests passed. Full-game accuracy, audio fidelity, battle mode and P2–P5 are not qualified; netplay and widescreen enhancements are not implemented.

Download the Windows x64 ZIP and extract the entire archive into a writable folder. Start the root SuperBomberman5SNESRecomp.exe, let the launcher obtain official build tools, select your own legally obtained ROM and use Generate & rebuild. The native analyzer is bundled; release users do not need Rust. The first tool download needs internet. Requires Windows 10+ x64 and several GB of disk space.

The release contains **a setup host and source, not the game**: no ROM, ROM-derived C, ROM-derived game executable, extracted sprites/music, saves or gameplay captures. Runtime licensed launcher assets and dependency notices are preserved. The playable build is generated locally under build/.

Supported image: header region JPN, CRC32 c25ab56a, SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4. The developer filename contains USA, but hashes determine compatibility. No other revision is qualified.

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

if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(notes, encoding='utf-8', newline='\n')
else:
    sys.stdout.reconfigure(encoding='utf-8')
    print(notes, end='')
