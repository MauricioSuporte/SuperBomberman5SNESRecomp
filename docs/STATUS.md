# Super Bomberman 5: estado do protótipo

## v0.6.0 Subordinate Bomber playable / Subordinate Bomber jogável — 2026-10-14

Release scope: the optional **Subordinate Bomber — playable character** mod
(`superbomberman5.subordinate-bomber` 0.1.0, host v0.6.0+), default-disabled.
The Zone 3-D rival is cartridge character index 9: every per-character table
read through `$16C2` has a tenth entry, its sprite set is `$19`, palettes
`$48-$4D` are its six colors, `$4E` the shared effect palette and `$4F` the
rival palette (`$C4:3CCF` uses base + 7). In-memory patches: `$C6:8B20/$8B2E`
(the picker script's locked X cell becomes character 10 in the rival color; the
unlocked secret-character branch is untouched), `$C2:1276` → `$C2:6000` (no X
arrangement for it) and ten-entry HUD face tables at `$C2:6020/$6030`.
Render-only overlays: grid figure from its standing-frame tiles, authored seat
portraits (one shape, nine colors per selectable color) and HUD faces (alive
from the standing frame, defeated from an authored palette-indexed sad face).
Native portrait/HUD frames for index 9 are Bomberman's, and the object system
shares VRAM tiles between objects on the same frame, so overlays draw into
borrowed free OBJ tile blocks and repoint only their own sprites. Selection
colors (palettes up to `$4F`) and the Normal picker hand-off accept character 10.

Evidence (Linux LLE build of these sources, JPN ROM): six native test
executables pass (new `subordinate_tests`: patch bytes and restore, refusal on
unexpected bytes, grid figure, every portrait pixel and color set, shared tiles
with Bomberman, palette conflicts, HUD alive/defeated faces, exact
VRAM/OAM/CGRAM restoration). Scripted routes: Battle picker cycling the six
portraits with PT-BR; P1 Bomberman with P2-P5 on Subordinate Bomber in four
colors (picker and Battle HUD); defeated HUD faces for P3-P5; five-seat Battle
through the score board; Battle without the colors mod; Normal Zone 1-1 with
five players (Subordinate red as P1); Normal Zone 3-D next to the rival
Subordinate Bomber. Logs had no BRK, `interp_cap`, unresolved abandon or fatal.
Not qualified: Windows runtime (CI builds/tests only), physical controllers,
full campaign, a save with the secret X-cell character unlocked, Continue path.
The README showcase was recaptured for v0.6.0 (see `docs/images/README.md`).

O Subordinate Bomber pode ser escolhido no lugar do X, no Battle e no Normal,
com as seis cores do cartucho via L/R. Figura, retratos e rostos do HUD (vivo e
derrotado) acompanham a cor; tudo é desfeito ao desativar o mod.

## v0.5.0 five-player Normal Game demo / Demo com cinco jogadores — 2026-10-14

Release scope: the optional **Normal Game — five players** mod (package 0.1.0,
host v0.5.0+) joins the four existing mods in the Windows ZIP and Linux
AppImage. The launcher's PLAYER 1–5 tabs and seat 3–5 input come from the
framework forks (`MauricioSuporte/snesrecomp`, `MauricioSuporte/recomp-ui`,
branch `superbomberman5`) based on upstream `e54427b`/`41c88ff`. The README
gains the maintainer-requested showcase (`docs/images/five-player-showcase.png`:
five-player title menu, Zone 1-1 and Zone 1-A with distinct characters/colors,
five-player Battle Mode), captured from this project's Linux LLE build.

Evidence before tagging (Linux LLE, JPN ROM, local): native tests for all five
mods, Python publication tests, fixture runtime suite with both renderers, the
co-op scenarios in the sections below and the showcase routes. The Windows
package/build evidence comes from the Release workflow; Windows playtesting of
the final changes was done by the maintainer. Known open issue: Back (B) in the
Normal character picker can leave a black screen (pre-existing).

Escopo: mod opcional de cinco jogadores no Normal Game incluído nos pacotes
Windows e Linux, abas PLAYER 1–5 no launcher (forks dos frameworks) e imagem de
apresentação no README. Problema conhecido: voltar com B na seleção de
personagens do Normal pode deixar a tela preta.

## Five-player menu confirmation / Confirmação no menu de cinco jogadores — 2026-10-14

The overlay moved the title cursor only for its idle tile, so the confirmation
bounce (tiles `$1EA-$1EE`) jumped to the stock 2P row, and the fade (mode 3)
showed the stock two rows. The bounce now keeps its offset on the chosen row
and the five rows stay through the fade; menu input remains mode 2 only.
Evidence (Linux LLE): frame captures of 1P/EN, 3P/PT-BR and 5P/PT-BR
confirmations; native/overlay tests; fixture suite with both renderers.

O cursor faz a animação de confirmação ao lado da opção escolhida e as cinco
opções ficam na tela até o fade-out.

## P3-P5 enemy contact and spawns in smaller arenas / Dano por inimigos e spawns em arenas menores — 2026-10-14

Enemy contact used per-player distance arrays with two entries (`$1FF4-$1FFF`,
loops `CPY #$02` at `$C3:47FB` and four other routines), so P3-P5 never took
contact damage. Co-op moves them to five-entry arrays at `$1FC0-$1FDD` and
fills seats 3-5 via `$C3:FF70`. Spawns ran before `$C5:3D3F` built the stage
map, so P3/P4 appeared outside smaller arenas; seats are re-checked at
`$C5:3D5D`. Evidence (Linux LLE): an enemy pinned on P3 sets status `$41` then
`$C1` like P1; P1 control unchanged; Zone 2-4 (`$1F`) now places P3/P4 at
(5,11)/(9,11), `$6A` P3 at (3,9), `$6B` P3/P4 at (2,9)/(12,9); unit tests,
fixture suite, P3-P5 exit warp and boss continues pass with no BRK. Enemies
still choose P1/P2 as chase targets. Windows build and physical play pending.

Inimigos agora causam dano a P3-P5 ao encostar, e P3-P5 não nascem mais fora
de arenas menores (ZONE2-4, ZONE3-D, ZONE3-E). Os inimigos continuam
escolhendo P1/P2 para perseguir, como no original.

## Five-player stage exits, boss stages, menu and Continue picker / Saídas, chefes, menu e seleção no Continue — 2026-10-14

Exit warps tested only P1/P2: script op `$12` (`$C2:0C67`) loops `CPX #$02`
at `$C2:0C9F`; the same bound sits at `$C5:6195/$C5:61C6` (floor gimmicks),
`$C1:6BA1` (cell hold) and `$C0:373F` (point damage). All follow the roster.
Boss stages `$67-$6E` create rival bombers in actor slots 2/3, which replaced
P3 and pushed P5 to slot 5; five-entry bomber tables (`$0FEA/$0FEF`) then sent
`$C4:06C7` into data and the game froze. Rivals now take the slots after the
players (trampoline `$C2:4340` for `$C2:16B8/$C2:16EE`, 110 literal operands in
`$C5:2600-$C5:39FF`), boss rosters are `min(players, 5 - rivals)` and benched
seats keep character/items. The player menu uses native colors (heading color
3, rows color 2), follows the W34SEL blink and passes Up/Down to the guest for
its cursor sound. Continue -> File (`$C7:0BEA/$C7:0E2B`) opens the picker.

Evidence (Linux LLE, JPN ROM): five native tests and the fixture runtime
suite passed; P3-P5 warps from Zone 1-1 to 1-5 with every seat's bombs/fire
kept; Continue picker applied five characters; Continue into `ZONE1 - A`
(5P: four fight, rival in slot 4) and `$6A` (three fight, two rivals) ran
without the freeze, 2P/3P stock comparisons unchanged. Not qualified: boss
defeat and choreography, full campaign, Windows build, physical controls.
Picker Back (B) leaves a black screen in headless runs for New Game and
Continue, also with the previous version; unresolved.

Portais agora funcionam para P3-P5 e os itens de todos seguem para a fase
seguinte. Nas fases de chefe Bomber o rival fica depois dos jogadores; com
cinco slots, lutam até quatro (três na fase com dois rivais) e os demais
voltam na fase seguinte com personagem e itens. Menu com opções brancas,
PUSH START piscando e som do cursor. Continue -> File abre a seleção de
personagens. Voltar com B na seleção ainda deixa a tela preta nos testes.

## v0.4.4 Linux AppImage + Windows ZIP — 2026-10-14

Experimental Linux x86_64 ROM-independent setup AppImage added alongside the Windows x64 ZIP, retaining v0.4.3's four optional mods. Clean published framework pins are unchanged; local uncommitted co-op/control changes are excluded. AppRun copies setup to a versioned writable XDG data directory, preserves existing files and selects the locally generated build on repeat launch. Runtime/tool hashes and licenses are included; upstream game screenshots/fixtures are removed and both payloads audited.

Evidence: Windows and Ubuntu 22.04 CI setup-host/analyzer builds, four native tests per platform, publication history/package audits, eight Python tests (two Linux launcher tests skipped on Windows), two AppImage --help launches and synthetic analyzer smoke passed. Extracted Windows and Linux AppImage payloads generated real-ROM code and rebuilt locally using portable toolchain v1.0.14. Linux local environment: Ubuntu 24.04.4 via WSL; launcher initialized using SDL offscreen, with no visual QA or physical devices. Windows setup handed off to the locally rebuilt launcher. The four native tests passed on both local game builds. The unchanged first-stage route completed 3,628 LLE frames on each platform, with dummy video/audio and mods disabled, including map/stage entry, movement and bomb input. Logs had no interp_cap, unresolved-abandon, dispatch_abandon or fatal. The Linux route was invoked through the AppImage after local rebuild, confirming repeat launch uses the generated game. Linux FUSE mounting, full campaign, physical controllers/audio and broad distribution compatibility remain unqualified; extract-and-run was used. Windows gameplay qualification remains limited to the recorded routes. No ROM or generated code is published.

A v0.4.4 acrescenta AppImage Linux x86_64 experimental ao ZIP Windows e mantém os quatro mods da v0.4.3. Submódulos publicados preservados; mudanças locais do cooperativo não incluídas. Setup copiado para uma pasta XDG gravável, sem sobrescrever arquivos existentes; próximas aberturas usam o jogo gerado. Auditorias, licenças e hashes dos pacotes/ferramentas preservados.

Builds CI Windows/Ubuntu 22.04, quatro testes nativos por plataforma, auditorias e oito testes Python passaram (dois testes Linux pulados no Windows). Analisador validado com ROM sintética; AppImage abriu duas vezes. Pacotes extraídos geraram e compilaram o jogo localmente. No Ubuntu 24.04.4/WSL, o launcher inicializou com SDL offscreen; sem inspeção visual ou dispositivos físicos. O setup Windows encaminhou para o launcher recompilado. A rota de 3.628 frames LLE passou nas duas plataformas, com vídeo/áudio dummy e mods desligados, incluindo entrada na fase, movimento e bomba; logs sem os erros conhecidos. A rota Linux foi iniciada pelo AppImage após recompilar, validando a reabertura. Montagem FUSE, campanha completa, controles/áudio físicos e outras distribuições Linux não qualificados. ROM e código gerado permanecem locais.


## v0.4.3 infinite time / Tempo infinito — 2026-10-14

Release scope: optional, default-disabled infinite Normal timer, packaged with
PT-BR menus, character selection and L/R colors. Existing dependency pins stay
unchanged. Local five-player/control changes are not part of this release.
Native WRAM sentinel 99 seconds/9 minutes selects the stock `-:--` HUD and
skips timeout. Interpreter hooks at `$C5:0400/$C5:0460/$C5:0BEF` chain the
previous hook; reset restores it. No cartridge file, SRAM or boss flags change.
No extra gameplay state/save-state extension. These routines must remain interpreted.

Initial Windows/LLE Zone 1-1 evidence: native dash HUD inspected; 1P/2P
forced 0:01 expiration remains unlimited after 780 frames; disabled control
retains countdown and timeout. A two-player route combines PT-BR, characters
and colors. Full campaign, real bosses, rewind and physical controls remain
unqualified. The clean pinned-source setup pack passed publication audit. Its extracted
analyzer passed a synthetic-ROM test without Rust/toolchain PATH; the extracted
source generated five AOT units from the verified local ROM and rebuilt on
Windows/LLE. The root launcher logged GUI startup and delegation to the local
rebuilt host. Four native tests, six Python tests and dependency pin checks
passed. Extracted-package routes passed 1P (4,198 frames), 2P with PT-BR,
characters/colors (5,964 frames), plus a disabled-timeout control (3,598 frames).
No interp_cap, unresolved-abandon, architectural BRK, dispatch_abandon or fatal
was found. Code generation/rebuild was exercised by the shipped CLI pipeline;
the full interactive wizard was not exercised in this verification.

Escopo: tempo infinito opcional no Normal, junto dos três mods anteriores,
sem publicar as alterações locais de cinco jogadores/controles. O relógio usa
os valores nativos 99/9 e HUD `-:--`; não altera ROM em disco, SRAM ou flags de
boss. Evidência inicial Windows/LLE na Zone 1-1 confirma HUD, ausência de
expiração após forçar 0:01 e contagem/morte normais com o mod desligado.
Pacote limpo auditado; analisador extraído funcionou sem Rust/PATH do toolchain,
e as fontes extraídas geraram o código da ROM local e recompilaram no Windows.
Launcher registrou abertura da GUI e encaminhamento ao jogo recompilado.
Rotas do pacote passaram em 1P/2P e no controle com mod desligado, com logs
limpos. A geração usou o pipeline CLI incluído, sem percorrer todo o assistente
interativo. Combinação com PT-BR/personagens/cores em dois jogadores testada. Campanha
completa, bosses reais, rewind e controles físicos ainda não qualificados.

## Launcher and local controls for five players / Launcher e controles para cinco jogadores — 2026-10-14

The launcher showed only PLAYER 1/2 and the desktop host routed local input
only to seats 1–2, so P3–P5 could not be played outside the test fixture. At
the maintainer's request the pinned submodules were changed locally (pending
upstream): recomp-ui's SNES profile ceiling is now 8 seats (the title's
`num_players = 5` decides), keybinds.ini gains `[player3]`..`[player8]`, and
snesrecomp's desktop host reads `SourceP3`.., `EnableGamepad3`.. and
`[GamepadMap] ControlsP3`.., seats extra gamepads and pushes seats 3–5 through
`RtlSetPadState` each frame. Seats 3.. start with no device; two-player
titles keep their config.ini unchanged.

Evidence (Linux build, same sources): recomp-ui's 41 tests pass, with new
checks for five SNES controller cards and per-seat `ControlsP5`; snesrecomp
config round-trip and keybinds tests pass with new seat 3–5 cases; a virtual
gamepad set to P3 reached seat 3 as Start (`RtlGetPadState(2)=0008`) while
P1 stayed idle. Not built on Windows yet; physical controllers and the
interactive launcher page were not exercised here.

O launcher mostrava só PLAYER 1/2 e o host só lia controles locais para dois
jogadores. Com autorização do mantenedor, os submódulos foram alterados
localmente (falta enviar upstream): o launcher do SNES aceita até 8 jogadores
(o jogo declara 5), keybinds.ini ganhou `[player3]`.., e o host lê
`SourceP3`.., `EnableGamepad3`.. e `ControlsP3`.., enviando P3–P5 ao multitap.
Testes do recomp-ui e do snesrecomp passaram em build Linux; um gamepad
virtual em P3 chegou ao assento 3. Falta validar no Windows e com controles
físicos.

## Five-player HUD, animation and rejoin / HUD, animação e retorno com cinco jogadores — 2026-10-14

Interactive five-player play showed four regressions: the HUD frame and icons
were missing, characters did not animate, Start appeared not to revive a dead
player, and P3 drew as a cut sprite. Causes, verified on the JPN image with
scripted LLE runs: the adapter replaced the stock HUD selector `$C5:066C` with
`RTS`; five resident players filled the five campaign sprite pages, so the
stock allocator `$C5:6AF5` refused every animation change and a rejoined
player was created without graphics (the shared life was still spent); P3
requested OBJ palette slot 3, which Zone 1-1 enemies own. The HUD selector now
runs again with the mask restored at `$C5:060E`; an authored allocator at
`$C5:7100` refills the requester's own page when no page is free; seats 3–5
use palette slots 7, 6 and 5.

Evidence (Linux build of the same sources, LLE, scripted routes): Zone 1-1
with five players shows the full HUD; P1 OAM tiles cycle while walking; P3
draws with its own palette; a real bomb death of P1 followed by Start spends a
life and P1 reappears and walks (also with `NewRenderer=1`, characters, colors
and PT-BR enabled); real bomb deaths of P2/P3 rejoin through the fixture. The
four native tests and `tests/normal_coop_runtime.py --fixture` (counts 1–5,
five skins/rejoins/Continue, three-player lifecycle) pass. Not yet run on the
Windows toolchain; bosses, mounts and later zones remain unqualified, and
mount/item palettes may fall back to another palette with five players.

A sessão com cinco jogadores mostrou HUD sem moldura/ícones, personagens sem
animação, Start sem reviver (a vida era gasta, mas o jogador voltava sem
gráficos) e P3 cortado. O seletor de HUD original voltou a rodar; um alocador
próprio reaproveita a página de sprites do jogador quando não há página livre;
P3–P5 usam as paletas 7, 6 e 5. Rotas roteirizadas na Zone 1-1 confirmam HUD,
animação, retorno com Start e paleta de P3; os testes nativos e de execução
passaram em build Linux. Falta validar no Windows, em bosses, montarias e
zonas seguintes.

## Normal menu recognition / Identificação do menu Normal — 2026-10-14

An interactive launcher session reproduced the original two-row menu while
the co-op feature was enabled. Local diagnostics recorded title mode with
`$2000` retaining `$C7:1B3B` from attract mode, instead of the fast scripted
route's `$C6:F717`. The feature now recognizes both native BG3 player rows
before overlays are applied and caches that context for the frame. Unit
checks cover stale return history and rejection of unrelated text. The local
executable is rebuilt; the accurate menu compositor remains scoped to this
screen. This does not qualify the rest of the campaign or physical controllers.

Post-fix Windows/LLE scripted runs with `NewRenderer=1` passed counts 1–5,
initial lives 3–7 and safe Zone 1-1 spawns; PT-BR was enabled for counts 2 and
4. Both player-menu languages were visually inspected. Four native tests,
six Python tests, publication history audit and whitespace checks passed.

A abertura interativa pelo launcher reproduziu as duas opções originais com
o mod ativado. O diagnóstico local registrou o modo de título com `$2000`
conservando `$C7:1B3B` da demonstração, em vez de `$C6:F717` da rota rápida de
teste. O mod agora reconhece as duas linhas originais em BG3 antes das
substituições visuais e mantém esse contexto durante o quadro. Testes nativos
cobrem o retorno antigo e rejeitam textos de outras telas. O executável local
foi recompilado. Campanha completa e controles físicos continuam não qualificados.

Após a correção, execuções Windows/LLE com `NewRenderer=1` passaram para 1–5
jogadores, vidas iniciais 3–7 e spawns seguros na Zone 1-1, com PT-BR nos casos
de dois e quatro jogadores. Os menus nos dois idiomas foram inspecionados.
Passaram os quatro testes nativos, seis testes Python, auditoria e verificação
de espaços em branco.

## Source Normal co-op / Cooperativo Normal — 2026-10-14

Current source adds the optional, default-disabled `superbomberman5.normal-coop`
v0.1.0. The 1–5 player menu works in English/PT-BR; initial shared lives are
3–7. Additional actors use safe-floor searches and boss offsets. The native
living check, Start rejoin and participating roster cover five seats. Normal
character/color selection waits for every chosen seat and preserves choices
through Continue. Dead Start is withheld from the stock pause task until
release. Mount backup slots are separated from P3/P4; NPC capacity is fourteen.

Source build and focused state/collision/rendering/selection tests pass.
Scripted Windows/LLE checks cover 1–5 players in Zone 1-1 and P3 as the sole
survivor, P1 rejoin without pause, Game Over and Continue. Extra-pad scenarios
exercise five independent character/color choices, each Start rejoin and
last-life arbitration through the real multitap path in an opt-in fixture.
The fixture is excluded from the normal build. Full campaign, real bosses,
mount/item combinations, dense NPC stages, endings, populated battery saves
and five physical controllers remain unqualified. No release was published.
See [NORMAL_COOP.md](NORMAL_COOP.md) for scope and reproducible checks.

Os fontes incluem o mod opcional para 1–5 jogadores no Normal, com menu
PT-BR, vidas iniciais 3–7, spawns seguros, retorno com Start e seleção de skins
e cores para todos. Os testes roteirizados abrangem a primeira fase, P3 como
único sobrevivente, retorno sem pausa e Continue. Há testes específicos das
posições de boss e de restauração de snapshots. Campanha completa, bosses reais,
montarias/itens, alta ocupação de NPCs e controles físicos ainda precisam de
validação. A build normal não inclui as caixas de entrada da fixture de teste.

Base criada pelo scaffolder oficial do SNESRecomp em 2026-10-13.

## ROM de referência

- Arquivo local: `Super Bomberman 5 (USA).sfc`.
- Cabeçalho: `Hu SUPER BOMBERMAN 5`, região JPN. O nome do arquivo não determina a região.
- Fast HiROM, 4 MiB, SRAM de 8 KiB, sem coprocessador especial detectado.
- Checksum interno válido; isso não comprova sozinho que a imagem é um dump original sem modificações.
- CRC32: `c25ab56a`.
- SHA-256: `5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4`.
- Identidade usada pela geração e pelo executável: `rom_identity.txt`.

## Base disponível

v0.4.2 consolida a publicação em um único ZIP Windows, com executável de
instalação, recursos do launcher, analisador, fontes e os três mods em
`mods/preloaded/packages`. Não há downloads de mods separados nessa release.
`FILE_SHA256SUMS` lista os hashes de todos os arquivos dentro do pacote; o
teste de integridade cobre conteúdo e caminhos relativos. A referência
MegaManXSNESRecomp v2.2.2-alpha foi conferida: seu ZIP Windows reúne
executável, recursos e catálogos de mods, sem compilador portátil embutido.
O Bomberman mantém a geração/rebuild local necessária ao seu setup host.

v0.4.1 atualiza o ícone Windows fornecido pelo mantenedor: dez tamanhos de
16 a 256 pixels, com escala proporcional e transparência, incorporados por
recurso RC no setup host e no jogo gerado localmente. O ícone foi extraído do
executável local para conferência visual. A categoria do mod PT-BR passa a
`Translation`; seu nome e descrições continuam em português. O README reúne
os três mods opcionais, todos v0.1.0, com ativação, controles e requisitos de
host em inglês e português. Essas mudanças não ampliam a validação de gameplay.

Correção antes da publicação v0.4.0: `$1038` é um contador de escolhas
confirmadas, não uma máscara. A tabela `$62B5` indica cursors ativos/confirmados
e `$62AB` associa cada controle ao cursor atual: humanos usam seus controles e,
ao confirmar, passam a escolher CPUs pendentes. L/R acompanha essas tabelas
e preserva os confirmados; Normal de dois jogadores aguarda contador 2.

Prévias de um mesmo personagem podem compartilhar uma paleta no alocador
original. O mod separa as cinco referências dos retratos em slots próprios,
preservando os demais donos/refcounts. A cor da grade acompanha o cursor ativo;
CPUs pendentes não sobrescrevem a prévia do personagem em escolha.

Na saída do seletor, a limpeza original pode restaurar cores padrão dos CPUs.
O mod preserva as escolhas até o inicializador nativo consumir os offsets e
derivar as paletas das fases; snapshots incluem essa transição. Rota de 7.360
frames com dois humanos e três CPUs validou entrada independente, transferência
de controle e as cinco paletas confirmadas na fase do Battle. A inicialização
do seletor reutiliza temporariamente `$1038`; a confirmação do Normal só passa
a ser observada depois que o seletor fica pronto em `$C6:8E68`. Rotas locais
de Normal para um e dois jogadores validaram abertura, confirmação separada e
paletas na Zone 1-1 (segundo personagem verde em 1P; branco/preto e segundo
personagem vermelho em 2P). Campanha e gameplay cooperativo completos continuam
pendentes de validação.

v0.4.0 inclui o mod de cores na seleção: L/R percorrem branco, preto, vermelho, azul,
verde e amarelo nos nove personagens. Cursor com L + R acima e jogador abaixo.
Testes nativos passaram para paletas, entradas, confirmação, snapshots e
restauração de OAM/VRAM; rotas locais com PT-BR validaram as seis cores e o
segundo personagem verde em fases do Normal e Battle. Multiplayer completo
continua pendente. Requer o host atualizado v0.4.0. Detalhes em [SELECTION_COLORS.md](SELECTION_COLORS.md).

Repositório Git local com SNESRecomp e recomp-ui como submódulos fixados em commits, CMake, configuração inicial de análise, launcher, scripts de geração e empacotamento e workflow oficial. A inicialização do jogo ativa o multitap na porta 2 para cinco assentos; multiplayer ainda não foi validado. Netplay está desabilitado.

Ferramentas locais preparadas em `.tools/`: RetroPorting Toolchains v1.0.14 (CMake, Ninja, Clang e SDL3) e Rust 1.97.1 Windows GNU LLVM. Scripts reproduzíveis: `tools/setup-windows.ps1` e `tools/build-windows.ps1`. Instruções em `BUILD_WINDOWS.md`.

Fontes locais gerados com sucesso e executável Release compilado em `build-lle/SuperBomberman5SNESRecomp.exe`, com SDL3 estático e composição de frames LLE. `play.cmd` abre o launcher.

A ROM, fontes gerados, ferramentas locais, builds, saves, dumps e capturas são ignorados pelo Git e sujeitos à auditoria de publicação.

Repositório público: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp. Formato da publicação inicial v0.1.0: pré-release Windows x64 com setup host compilado sem dados da ROM, fontes, analisador nativo independente da ROM e sua DLL/licença. O jogo completo é gerado no computador do jogador. Não distribuímos o executável local `build-lle/`, que contém código derivado da ROM.

README em inglês/português, CONTRIBUTING.md, THIRD_PARTY_NOTICES.md e docs/PUBLISHING.md descrevem instalação, limites, licenças e fluxo de versões. A skill `$super-bomberman5-repository` está instalada localmente e versionada em `.agents/skills/`. Source checks audita o histórico e executa os testes de publicação; Release gera pacotes Windows manualmente, com ferramentas fixadas em v1.0.14.

## Publicação v0.1.0

Pré-release publicada em 2026-10-13: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.1.0.

- Tag anotada `v0.1.0`: commit `6ba1ee0ffea2560d18f65518ddf0ba72cfaf72a1`.
- ZIP Windows x64: `superbomberman5-0.1.0-windows-x64.zip`, 48.814.122 bytes.
- SHA-256: `4656188e1d4176798ef08a73ece8a32f5ab0dc9abdabc9814c79a6c8b26802d1`, acompanhado de `SHA256SUMS`.
- Build Release no GitHub Actions passou: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/actions/runs/38475851829.
- Arquivo final auditado sem ROM, código C gerado, executável derivado da ROM, capturas de jogos das dependências ou fixtures de ROM. Avisos de licença do framework, launcher, Cargo, Rust e LLVM foram preservados.
- ZIP extraído: launcher abriu; analisador executou um programa RTS sintético sem Rust nem PATH das ferramentas; geração com a ROM local, rebuild e execução de 600 frames foram verificados. A build gerada manteve `SNESRECOMP_FRAME_IMPL=LLE`.
- GitHub confirmou pré-release pública com dois assets, tamanhos e digests iguais aos arquivos locais. O executável jogável local e suas capturas não foram enviados.

A validação usa as rotinas de geração/rebuild do pacote e smoke tests do launcher. A sequência completa de cliques no assistente, novas instalações em outros computadores e o download automático de ferramentas ainda precisam de testes adicionais.

## Validação realizada

Probe de cabeçalho e checksum, verificação de CRC32/SHA-256 pelo CLI e conferência dos submódulos após mover o scaffold para a pasta de trabalho.

Teste de 600 frames sem entrada e testes de navegação com entradas roteirizadas. Capturas locais mostram a abertura animada, tela de título, menus de modo normal, seleção de arquivo e mapa da Zone 1. A rota de 3025 frames até o mapa terminou com código zero, sem mensagens `interp_cap`, `unresolved-abandon` ou BRK. Evidências brutas estão em `.bootstrap/validation/`, ignoradas pelo Git. As primeiras capturas usam SDL vídeo/áudio dummy: não comprovam a qualidade sonora.

O script de setup também foi executado novamente para verificar o reaproveitamento das ferramentas, e o script de build foi validado pelo parser do PowerShell e executado na geração e compilação.

A rota final `validation/first-stage.input` chegou à primeira fase da Zone 1 e terminou após 3628 frames com código zero. As capturas mostram cenário, jogador e cronômetro avançando. Sem mensagens `interp_cap`, `unresolved-abandon`, BRK, erro ou fatal nessa execução. Não há comparação independente de colisões, bombas ou precisão de timing ainda.

Um smoke test adicional de 180 frames abriu a janela SDL real do Windows e o dispositivo de áudio real, com callbacks de áudio e saída normal. O launcher ImGui/SDL3 também abriu e encerrou normalmente pelo mecanismo de smoke test de 60 frames. Isso comprova inicialização dos dispositivos, não fidelidade ou qualidade sonora auditiva.

## Correção de encoding e primeiro mod PT-BR

O corpo da release v0.1.0 apresentou caracteres de substituição ao passar pela saída do console do Windows. Foi corrigido via API com bytes UTF-8 explícitos, comparando o texto remoto com o original e verificando a ausência de U+FFFD. `tools/release-notes.py --output <arquivo>` agora grava UTF-8 diretamente; `--version 0.1.0` permite corrigir notas históricas sem mudar a versão do código.

O código atual prepara a versão **0.2.0**, com o primeiro mod `superbomberman5.ptbr` v0.1.0. São sete rótulos de título e modo normal, fonte própria com Ç/Õ, feature desligada por padrão, ativação na aba Mods e `.snesmod` somente de dados. A antiga release v0.1.0 não tem o plugin nativo.

Capturas locais mostram os rótulos traduzidos. O teste nativo de restauração verificou desenho alterado durante o frame, guarda de superfície, restauração integral da VRAM e caminho desativado. Uma rota com o mod ativo terminou em 3628 frames e entrou na primeira fase, sem erros de dispatch/BRK. Fonte e código autorais são públicos; ROM, tiles extraídos e capturas ficam locais. Escopo e instruções: `docs/TRANSLATION_PTBR.md`.

## Ampliação e correções da tradução PT-BR

A prévia de menus agora tem 62 rótulos de background e um cabeçalho de sprites: título, 1/2 jogadores, modo normal, arquivos, opções/senha/teste de som, tipos de batalha, participantes, personagens, regras e cabeçalho de fases. Inclui acentos, algarismos, sombra, traços mais espessos no título e cabeçalhos azuis. Incluída como prévia no host 0.2.0; a release/tag 0.1.0 não foi substituída.

O corte de JOGO NORMAL ocorria porque a nova fonte ocupava a janela HDMA usada pelo jogo para ocultar Start. O título respeita agora a margem superior do original; o aviso ocupa suas duas linhas de tiles. Capturas consecutivas de 1215 a 1551 verificaram Start aceso/apagado, sem cortar JOGO NORMAL. A paleta de textos longos agora vem do início do rótulo, inclusive sobre espaços antigos; slots de tiles são estáveis por tela, separados dos gráficos em uso.

Telas de jogadores e regras reutilizam IDs de tiles com conteúdos diferentes. Guardas de contexto distinguem as telas e são avaliadas antes de desenhar qualquer rótulo. O teste nativo cobre restauração integral da VRAM, ativação/desativação, alinhamento, slots, paletas, contexto compartilhado e cabeçalho de sprites sem alterar OAM. O build Windows LLE e os testes de publicação/UTF-8 passaram. Capturas e rotas de navegação ficam exclusivamente em `.bootstrap/ptbr-*/`; configurações do usuário foram preservadas, com testes do mod em cópias isoladas.

Nomes e descrições das arenas, confirmações de arquivos e telas não verificadas de equipes/Bombermania ainda podem permanecer originais. Navegação em preparação de batalha não qualifica a precisão de partidas nem P2–P5. Escopo detalhado: `docs/TRANSLATION_PTBR.md`.

## Cobertura nativa e limitação atual

A configuração conservadora gera cinco variantes AOT elegíveis e quatro variantes LLE, em nove variantes exatas. A maior parte da lógica do jogo ainda executa no interpretador. Isso é uma primeira integração compilada, não uma recompilação nativa completa do jogo.

Uma tentativa de promover reset e NMI a AOT descobriu 69 variantes elegíveis, mas falhou no teste: limite de interpretação em `$C02346`/`$C00607`, abandono de dispatch no frame 56 e BRK posteriores. Essa promoção foi retirada. `recomp/symbols.toml` mantém reset e NMI com `emit = false` até corrigir e validar a continuidade do código compilado/interpretado.

## Próximas etapas

1. Corrigir e validar a continuidade AOT/LLE antes de promover as rotinas de reset/NMI novamente.
2. Ampliar a cobertura com rotinas curtas comprovadas e perfis de execução vinculados ao hash desta ROM.
3. Comparar frames, WRAM, interrupções e áudio com uma referência independente (bsnes/Snes9x/Mesen).
4. Validar fases, bombas, colisões, bosses, saves e multiplayer local, incluindo entradas de P3–P5 no host desktop.
5. Validar configurações e fluxo completo do launcher, empacotamento e workflow antes da publicação no GitHub.

## Release v0.2.0

Pré-release Windows x64 com o mod PT-BR v0.1.0 incluído no catálogo Mods, mais arquivo `.snesmod` separado. Publicação usa um checkout limpo e setup host LLE, sem código gerado/ROM. SHA256SUMS cobre o ZIP e o mod. A versão 0.1.0 permanece preservada. Tradução e precisão do jogo continuam parciais.
## Release v0.3.0 — Normal Game character selection

The v0.3.0 setup pack includes the optional, default-disabled
`superbomberman5.normal-characters` v0.1.0 package and trusted plugin. Confirming
a Normal file opens the cartridge's Battle character picker, then restores the
suspended campaign and applies the character/palette before stage graphics
load. The second and ninth characters were observed in Zone 1-1; the latter
was exercised with movement/bomb input and the PT-BR overlay enabled. Cancel
returns to file selection. Source build and focused selection/state tests pass.
Full campaign and cooperative stage behavior are not qualified. This Windows prerelease includes a ROM-free setup host and two data-only mod archives. See [NORMAL_CHARACTERS.md](NORMAL_CHARACTERS.md).

O setup pack v0.3.0 inclui o mod opcional, desabilitado por padrão, de seleção de
personagens no Normal Game. A seleção original do Battle abre depois da
confirmação do arquivo e a escolha é aplicada antes de carregar os sprites da
fase. O segundo e o nono personagens foram observados na Zone 1-1; o nono foi
testado com movimento/bomba e tradução PT-BR. B volta aos arquivos. Compilação
e testes de seleção/estado passaram. Campanha completa e fases cooperativas
ainda não foram qualificadas. Pré-release Windows com setup host sem ROM e dois pacotes de mods.
