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

if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(notes, encoding='utf-8', newline='\n')
else:
    sys.stdout.reconfigure(encoding='utf-8')
    print(notes, end='')
