# Português brasileiro — menus e conclusão do Normal (prévia)

Pacote `superbomberman5.ptbr`, versão `0.1.0`, feature `menus_ptbr`, plugin nativo confiável `superbomberman5.ptbr.menus`. Desabilitado por padrão. Só aceita a ROM de referência de `rom_identity.txt`.

## Como testar

Use a release v0.10.0 e conclua Generate & rebuild com sua ROM. Para compilar este checkout, use `./tools/build-windows.ps1` e abra `SuperBomberman5Recomp.exe`. No launcher, entre em **Mods**, encontre **Português brasileiro — menus (prévia)**, ative **Menus em português brasileiro** e selecione Play. Desative a feature para voltar aos textos originais. A release antiga do jogo **v0.1.0 não inclui este plugin**; instalar o pacote nela não adiciona código nativo.

Para gerar o arquivo instalável, sem ler a ROM:

```powershell
python tools/generate-ptbr.py --mod-out dist/superbomberman5-ptbr-menus-0.1.0.snesmod
```

O `.snesmod` contém manifesto, textos/metadados autorais e licença, sem DLL, executável, ROM ou gráficos extraídos. A implementação nativa precisa estar compilada no jogo. A versão 0.1.0 está incluída no catálogo preloaded deste checkout, portanto não é necessário importar o arquivo para testar localmente.

## Escopo

A prévia foi ampliada para título, quantidade de jogadores, modo normal, arquivos, opções, teste de som e menus de batalha, incluindo regras e o cabeçalho de seleção de fases. Os cabeçalhos azuis e o título da tela de senha também são substituídos. Bombermania é um nome próprio e continua como BOMBERMANIA.

| Original | Tradução |
|---|---|
| PUSH START BUTTON | PRESSIONE START |
| NORMAL GAME | JOGO NORMAL |
| BATTLE GAME | BATALHA |
| OPTIONS | OPÇÕES |
| New Game | Novo jogo |
| Continue | Continuar |
| Erase File | Apagar |
| 1 PLAYER GAME | 1 JOGADOR |
| 2 PLAYER GAME | 2 JOGADORES |
| NORMAL MODE | MODO NORMAL |
| SELECT A FILE | ESCOLHA ARQUIVO |
| File 1 | Arquivo 1 |
| File 2 | Arquivo 2 |
| File 3 | Arquivo 3 |
| Password | Senha |
| Sound | Som |
| Sound Test | Teste de som |
| Stereo | Estéreo |
| BATTLE MODE | MODO BATALHA |
| Battle Royale | Batalha livre |
| Bombermania | Bombermania |
| Config Battle Mode | Configuração |
| BATTLE TYPE | TIPO DE BATALHA |
| Free-for-All | Todos contra todos |
| Team Battle | Por equipes |
| 1 Player | Jogador 1 |
| Human | Humano |
| 2 Player | Jogador 2 |
| 3 Player | Jogador 3 |
| 4 Player | Jogador 4 |
| 5 Player | Jogador 5 |
| PLAYER SELECT | JOGADORES |
| SOUND TEST | Teste de som |
| Music | Música |
| SFX | Efeitos |
| Narrator | Narrador |
| CHARACTER SELECT | ESCOLHA O PERSONAGEM |
| PASSWORD | Senha |
| RULE SETTINGS | REGRAS |
| CPU Level | Nível CPU |
| Matches | Rodadas |
| Time | Tempo |
| Sudden Death | Morte súbita |
| Bad Bomber | Revanche |
| Bowling Bomber | Boliche |
| Low | Baixo |
| Random | Aleatório |
| On | Sim |
| Mono | Mono |
| Super | Super |
| Normal | Normal |
| Strong | Forte |
| Off | Não |
| STAGE SELECT | ESCOLHA A FASE |
| BATTLE START | INÍCIO DA BATALHA |

Ainda é uma tradução parcial. Textos de telas não mapeadas, outras confirmações de arquivo, telas ainda não verificadas de equipes/Bombermania e outras mensagens de fases/cenas podem permanecer originais. Tradução de menus não comprova a precisão de partidas de batalha ou multiplayer. Logo, retratos, áudio e mapas permanecem originais.

## Implementação e dados

A prévia contém 71 rótulos de background, dois cabeçalhos de sprites e 33 resultados de conclusão com título e até três linhas de dicas. Os rótulos de uma mesma tela são identificados antes de qualquer substituição, evitando que a alteração do primeiro texto invalide os demais. IDs reutilizados por telas de jogadores e regras recebem guardas adicionais de contexto. Variantes SIM/NÃO, NORMAL/FORTE e MONO/ESTÉREO compartilham slots temporários.

`translations/ptbr/labels.json` contém texto e coordenadas/identificadores de tiles como metadados funcionais. Não contém bitmaps do jogo. `tools/generate-ptbr.py` valida limites e gera um header dentro da pasta de build. `src/ptbr_native_font.h` lê as fontes do cartucho durante a execução: cabeçalhos azuis, texto proporcional e letras brancas do título. `src/ptbr_font_locations.h` contém somente endereços e coordenadas funcionais, sem pixels extraídos. Acentos, extensões ausentes e o fallback são autorais. O plugin permanece em `src/ptbr_mod.c`.

Durante o renderer LLE, o mod identifica rótulos por camada, posição e IDs esperados, desenha novos tiles temporários e restaura toda a VRAM antes de o driver reaplicar escritas pendentes do jogo. A fonte branca nativa do título respeita a margem superior do original, evitando que a janela HDMA usada para piscar Start corte JOGO NORMAL. Slots de tiles são estáveis por tela; textos que ocupam antigos espaços vazios mantêm a paleta do início do rótulo. As áreas reservadas ficam separadas dos tiles de texto originais. O cabeçalho de fases usa uma faixa de nove sprites de 16×16: o mod verifica posições, tamanho, atributos e tiles, substitui apenas os pixels exclusivos dessa faixa e restaura a VRAM. A tradução dos nomes das arenas também empresta OBJ e restaura a OAM após desenhar. Não altera arquivo de ROM nem WRAM; o renderer HLE ainda não está qualificado para essa integração. Não há fallback que declare o jogo inteiro traduzido.

As capturas de validação ficam em `.bootstrap/ptbr-shots/` e nunca são publicadas. O teste `ptbr_render_restore` verifica mod ativo/desativado, guardas, restauração integral da VRAM, margem do título, slots estáveis e paleta de textos maiores. Capturas consecutivas cobrem Start visível/apagado e as telas traduzidas; arquivos de captura ficam apenas em `.bootstrap/`. Execute `./build-lle/ptbr_render_tests.exe` após compilar. Se CTest estiver instalado, também pode usar `ctest --test-dir build-lle --output-on-failure`; o pacote portátil atual não inclui CTest.


## Ampliação de conclusão — 2026-10-15

Esta ampliação requer gerar/recompilar com o host v0.8.2; importar apenas o pacote
0.1.0 numa release anterior não instala o código novo. A feature e o plugin
continuam os mesmos, desabilitados por padrão.

- CONTINUE FROM... → CONTINUAR DE; ERASE WHICH FILE? → APAGAR QUAL ARQUIVO?
- GAME OVER → FIM DE JOGO; CONTINUE → CONTINUAR; END → SAIR.
- HUD: PUSH START → APERTE START; GAME OVER → FIM DE JOGO, nos dois lados,
  preservando o piscar original.
- Aviso de recompensa por 100%: três linhas traduzidas; senha nativa intacta.
- CERTIFICADO, porcentagem de fases concluídas, assinatura, DICAS e PROGRESSO;
  todos os 33 títulos e respectivas dicas, incluindo fases secretas.

A seleção nativa em `$C6:E120-$E449` usa porcentagem, rotas de chefes,
final e fases secretas. Os 33 resultados foram auditados na ROM local:
layouts 4:$66-$86 para títulos e 4:$89-$A9 para dicas. Apenas textos
traduzidos autorais e IDs funcionais de tiles entram em `labels.json`.
Não há cópia de gráficos, paletas ou dumps de texto da ROM.

O reconhecimento confere os 24 tiles da linha do título e os cabeçalhos
da página. Um atlas de caracteres próprio reutiliza tiles temporários
para comportar as páginas longas. A assinatura original está desenhada no
pergaminho do BG2: uma faixa opaca, com a cor do papel da paleta em uso,
cobre só suas letras, sem alterar o selo ou a borda. O Game Over em Mode 7
usa faixas OBJ; posições, tiles, paleta, tamanho e exclusividade são
conferidos antes de substituir os pixels. A OAM permanece intacta.

O teste nativo cobre os 33 resultados nas duas páginas, guarda negativa,
porcentagem preservada, faixa opaca da assinatura, sprites compartilhados,
HUD e restauração integral da VRAM. O roteiro abaixo usa SRAM isolada e
fixtures de morte/participação/progresso, sem modificar o save do usuário:

```powershell
python tests/ptbr_completion_runtime.py --exe build-lle/SuperBomberman5SNESRecomp.exe --rom 'Super Bomberman 5 (USA).sfc'
```

Use `--renderer 1` para o compositor alternativo do LLE. Capturas brutas ficam
em `.bootstrap/ptbr-completion-validation/`. Não representa uma campanha
completa jogada nem qualifica multiplayer físico ou o renderer HLE.

## English — completion coverage

The current checkout adds Continue/Erase file headings, Normal Game Over
(including OBJ lettering), both HUD join/defeat messages, the completion
password reward, and all 33 native certificate ranks and tips, including
secret-stage variants. Native percentages, passwords and blink timing stay
unchanged. This requires rebuilding host v0.8.2; the data-only 0.1.0
package cannot upgrade an older executable. Authored glyphs and temporary
VRAM writes are restored after drawing; ROM, WRAM, OAM and SRAM are unchanged.
The native test exercises both pages for all 33 ranks. The isolated runtime
script above checks presentation against an untranslated run with identical
native memory. Its death/progress fixtures do not establish full-campaign
or physical multiplayer coverage.

## Integrações das fontes atuais / Current-source integrations — 2026-10-15

O menu de pausa acompanha o PT-BR: Continuar, Empatar rodada, Voltar ao mapa,
Escolher arena e Tem certeza / Não / Sim. Gold Battle acrescenta Hiper
à linha Revanche. A seleção traduz Fase e os nomes das arenas 1–13, inclusive
as dez originais sem Gold Battle. Usa a fonte OBJ 8×16 da própria VRAM,
com acentos autorais, preservando cores, contorno e sombra. O número nativo
permanece. A tradução acompanha os sprites efetivamente exibidos durante a
troca, sem depender do PC do script ou do índice antecipado na WRAM.
OAM e tiles emprestados são restaurados após desenhar.

Pause labels and confirmation follow PT-BR. Gold Battle adds HIPER to Revenge.
Stage selection translates Fase and all 13 arena names, including the original
ten without Gold Battle. Letters come from the native 8×16 OBJ font in live
VRAM, with authored accents and native colors, outlines and shadows. The
native number stays visible. Translation follows displayed sprites throughout
transitions; borrowed OAM and VRAM are restored after drawing.

Windows/LLE: `tests/battle_translation_runtime.py` compared the rendered pink
letter pixels against the native-font translation for 109 captured frames
across arenas 1–13 and 82 frames across arenas 1–10 without Gold. Eight
consecutive frames per forward transition were checked; no original-language
flash occurred in those samples. This does not qualify every possible input
sequence or renderer. Native tests cover all numbers and exact restoration.

## Fontes nativas nos menus / Native menu typography — 2026-10-15

Os textos gerados pela tradução agora compartilham três famílias do próprio
cartucho: o relevo azul dos cabeçalhos, as letras proporcionais 8×16 dos
menus e a fonte branca do título. Opções e regras usam maiúsculas/minúsculas
como no original (por exemplo, Nível CPU e Morte súbita). O título mantém
sua margem e o piscar; senhas e títulos grandes mantêm a proporção ampliada.
Textos longos são ajustados à largura disponível. Os nomes das arenas mantêm
a implementação anterior, com fonte da VRAM e proteção das transições.

Somente localizações funcionais dos glifos são versionadas. Pixels são lidos
da ROM do usuário; acentos e as letras J/Q/V ausentes nos cabeçalhos são
completados por desenhos autorais. O título compõe C/D/J/F/Q a partir de
letras nativas quando necessário. Algarismos/sinais ausentes usam o fallback
autoral. Nada altera a ROM, o progresso ou o áudio.

Translated text now uses three cartridge font families: beveled blue headings,
proportional 8×16 menu lettering and native white title lettering. Menu bodies
follow native mixed case. Long labels fit their reserved width; large headings
scale in both axes, and the title retains its original upper margin and blink.
All translation render paths share the font renderer, including file headings,
passwords, completion pages, HUD and Game Over. Battle names retain their
separate native-VRAM renderer and transition guards.

Only functional glyph addresses/crop coordinates are stored in source.
Letters are read from the user's cartridge at runtime; accents, missing blue
J/Q/V and missing digits/punctuation use authored extensions. Some missing
white-title letters are composed from native pieces. Native tests use synthetic
cartridge pixels to verify both glyph halves, white-title margins, proportional
advances, banner-background removal and exact guest restoration. Runtime
validation uses `tests/ptbr_typography_runtime.py` for paired translated/original
menus and `tests/ptbr_completion_runtime.py` for completion/HUD fixtures.
Raw captures stay in ignored `.bootstrap/`. These routes do not qualify HLE,
every menu variant or a complete campaign.

Final build: 22 paired menu frames covered 50 label contexts with identical
native memory; ending/perfect completion fixtures passed. A further 109 arena
name checks across arenas 1–13 passed, including transitions (5,932 frames).
Build final: 22 quadros pareados cobriram 50 contextos de menu com memória
nativa idêntica; conclusão/perfeito passaram. Mais 109 verificações dos nomes
das arenas 1–13 passaram, incluindo transições (5.932 quadros simulados).

## Traços dos cabeçalhos / Heading strokes — 2026-10-15

A separação entre letras azuis e faixa de fundo mantém dois pixels de
contorno em cada lado do recorte. Isso preserva as barras claras centrais de
E/F/H (também usadas no título SENHA), mesmo quando compartilham a cor 11 com
o fundo. Uma linha superior fica reservada ao contorno: ESCOLHA A FASE usa a
mesma correção ao desenhar sobre fundo transparente. A alteração não muda o
renderer separado dos nomes das arenas nem seus guardas de transição.

Blue glyph isolation retains two outline columns on each side before removing
banner backgrounds. This keeps bright E/F/H crossbars that share color 11
with the banner. One row is reserved for the upper outline, including the
transparent stage-selection heading. Native synthetic tests and optional
cartridge checks cover these strokes and the OBJ palette mapping. Paired
Windows/LLE validation passed on 22 frames / 50 label contexts with exact
native-memory parity; the password, Battle type, character and stage headings
were visually inspected. This qualifies those samples rather than every menu
variant. Run `./build-lle/ptbr_render_tests.exe 'Super Bomberman 5 (USA).sfc'`
for the optional local-ROM glyph check. No cartridge pixels are stored in tests.

## Confirmação da arena / Arena confirmation — 2026-10-15

BATTLE START agora aparece como INÍCIO DA BATALHA com o mod PT-BR. A faixa
mantém a paleta rosa e o piscar nativos, com a mesma fonte de relevo dos
cabeçalhos. Fase e o nome traduzido da arena permanecem em português durante
os intervalos visíveis e apagados do aviso. A restauração após desenhar
preserva a memória original e o início normal da partida.

BATTLE START is translated as INÍCIO DA BATALHA with PT-BR enabled. The OBJ
strip retains its native pink palette and blink. The translated arena label
and name remain visible through both blink phases. Metadata supports a strip
starting partway through an OBJ tile row and selects its native palette;
all temporary glyph/OAM writes are restored after rendering. Native tests
cover all 13 arena numbers in selection, visible confirmation and blink-off
confirmation, plus wrong-palette rejection. Runtime checks use
`tests/battle_start_runtime.py` against the user's cartridge, including
arena 1 without Gold and arena 13 with Gold, with translation-off controls.

Runtime evidence: 37 paired frames per arena (74 per language), including
32 consecutive confirmation frames, passed for arenas 1 and 13. Pink-heading
visible/blink-off phases and translated arena-name ink were checked, with exact
guest-memory restoration and identical gameplay after the overlay ended.
Evidência: 37 quadros pareados por arena, incluindo 32 consecutivos, passaram
nas arenas 1 e 13; paleta rosa/piscar e nomes PT-BR preservados, memória
idêntica e partida idêntica após o aviso.

Os números da quantidade de jogadores usam a mesma altura, espessura e sombra
das letras brancas do título: 1/2 vêm da ROM; 3–5 são extensões autorais.
Player-count digits match the white title lettering height, weight and shadow
using cartridge glyphs for 1/2 and authored extensions for 3–5.

No menu de cinco jogadores, o rosto acompanha o início da opção selecionada
em português, mantendo oito pixels entre a célula do cursor e a do número.
A linha singular recebe seu próprio alinhamento; a animação de confirmação
é preservada e OAM/VRAM são restaurados após desenhar.
In the five-player menu, the face follows the selected Portuguese row with
the native eight-pixel cursor/digit cell gap, including the shorter singular
row. Confirmation bounce and exact OAM/VRAM restoration are preserved.

Current source: the five-player English menu uses 1 PLAYER / 2–5 PLAYERS.
Y reads the cartridge's original white-title halves; T drops the neighboring
U's outline below its crossbar. Both languages align the face cursor to the
selected row with the native gap; all writes remain render-only.
Nos builds atuais de fonte, o menu inglês usa 1 PLAYER / 2–5 PLAYERS. Y usa
os glifos originais do título; T remove o contorno vizinho abaixo da barra.
O cursor acompanha a linha nos dois idiomas, sem alterar a memória do jogo.

## Rótulos durante a fase / Labels during a stage — 2026-10-17

Durante uma fase (Normal `$0D`-`$73` ou arena do Battle, com o jogo rodando ou
um bomber em campo) só os textos da HUD (linha 0 do BG3) são traduzidos. A
arena do chefe do ZONE2 tinha, por acaso, os tiles do "Continue" do menu de
arquivos e mostrava "Continuar" sobre os blocos.

While a stage is played (Normal `$0D`-`$73` or a Battle arena, with gameplay
running or a bomber on the field) only the HUD labels (BG3 row 0) are
translated. ZONE2's boss arena held the file menu's "Continue" tiles by chance
and showed "Continuar" over its blocks.
