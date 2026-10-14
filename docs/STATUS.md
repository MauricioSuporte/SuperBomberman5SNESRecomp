# Super Bomberman 5: estado do protótipo

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
