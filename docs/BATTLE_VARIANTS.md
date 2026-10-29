# Battle — stage variants / Variantes das fases

## Português

Em **Mods → Gameplay → Battle — stage variants**, ative **Choose Battle stage
variants**. Vem desligado por padrão. Com o mod ligado, apertar **A** ou
**Start** numa arena do carrossel do Battle abre a tela **Escolha a variação** em vez de
começar a partida:

- **Original**: o layout de sempre da arena;
- **Variante 1**: o layout da senha **4622**;
- **Variante 2**: o layout da senha **0413**;
- **Variante 3**: o layout da senha **0926**;
- **Variante 4**: a regra da senha **0110**, em que todos os blocos macios
  viram itens. Só aparece na arena 1, a única cujo script tem essa regra.

À esquerda, uma miniatura da arena mostra os blocos fixos e onde cada jogador
começa (P1 a P5). Abaixo dela aparece a senha correspondente. **↑/↓** escolhe,
**A** ou **Start** confirma e segue a troca de cena e o início de partida normais do jogo,
**B** volta para o carrossel. Qualquer jogador pode usar a tela. Com o mod
**Português brasileiro — menus** ligado, a tela fica em PT-BR; sem ele, em
inglês. O que cada variante muda depende da arena (spawns no centro, outro
layout de blocos fixos, outro grupo de itens); os blocos macios continuam
sorteados pelo jogo a cada partida.

Como funciona:

- A fase é o modo `$74 + 4×(arena−1) + variante`. No início da partida, o
  script `$C6:A4A9` escolhe a variante pelos bits de senha da sessão
  `$7E6274` (`$01` → 1, `$02` → 2, `$04` → 3); sem eles, pode usar os bits
  salvos `$7E6273` (e sortear), senão fica com a 0. A regra de itens é a flag
  de script 3 (`$7E2900` bit 3, ligada pela senha 0110 via `$C2:033A`), que o
  mesmo script desliga se `$6274` não tiver o bit `$20`.
- Ao confirmar, o mod escreve o bit da variante (e, na Variante 4, o bit
  `$20` e a flag 3), esconde temporariamente os bits salvos para o jogo não
  sortear outro layout e devolve o **A** ao seletor no quadro seguinte. Quando
  a fase começa, os bits de senha originais voltam. Nada na ROM ou no SRAM é
  alterado.
- A entrada passa pelo fim do leitor de controles (`$C0:0AF9`, vetores
  `D+$2C/$31/$36/$3B` por jogador), com o menu aberto só no seletor
  interativo (tarefa 9 entre `$C6:A1AC` e `$C6:A33D`). O estado do menu é uma
  extensão do save state, então todos os peers do netplay veem a mesma
  escolha.
- As miniaturas são montadas **uma vez**, quando o mod é ativado, a partir da
  ROM do próprio jogador, e ficam prontas na memória (13 arenas × 5 imagens).
  Nenhum mapa extraído é guardado no repositório ou no pacote. Fontes: script
  da fase (`$C6:0000 + (modo−1)×3`), operações de mapa `0a`/`0b`/`0c` e camada
  de colisão `FF.79`, stream comprimido em `$D0:A000 + (mapa−1)×3`
  (descompressor `$C0:1CB0`), atributos sólidos em
  `$D0:8000 + (tileset−1)×2` e spawns `22 P x y`.
- O desenho é feito sobre a imagem final, depois que o PPU a compôs; VRAM,
  OAM, CGRAM e WRAM não são tocados para desenhar.

Validação (Linux LLE, ROM USA): as 52 combinações arena × variante
decodificadas da ROM batem com o mapa de colisão `$1052` e os spawns de
P1/P2 do jogo rodando; as 52 miniaturas da tela batem com esses mapas célula
a célula. Rotas headless: abrir, mover, **B** voltando ao carrossel e
reabrindo em outra arena, confirmar a Variante 2 (modo `$76`) e a Variante 4
(0 blocos macios na arena 1), PT-BR e inglês. Sonda de rollback com o menu
usado: só as divergências de PPU já conhecidas, iguais às do jogo sem o mod.
Teste nativo `battle_variants_tests`. Não validado no Windows nem numa
partida de netplay real.

## English

Enable **Choose Battle stage variants** under **Mods → Gameplay → Battle —
stage variants**. Disabled by default. With the mod on, pressing **A** or
**Start** on an arena in the Battle carousel opens **Choose a variant** instead of starting
the match: **Original**, **Variant 1** (password 4622 layout), **Variant 2**
(0413), **Variant 3** (0926) and **Variant 4** (the 0110 rule: every soft
block is an item; arena 1 only, the one stage whose script has the rule).
A minimap of the arena shows its hard blocks and each player's start (P1-P5),
with the password under it. **Up/Down** chooses, **A** or **Start** confirms and continues
with the game's own scene change and match start, **B** returns to the
carousel. Any player can use it. The screen is in PT-BR when the PT-BR menus
mod is on, English otherwise. What a variant changes depends on the arena;
soft blocks are still placed at random by the game.

The stage is mode `$74 + 4*(arena-1) + variant`; match-start script
`$C6:A4A9` reads the session password bits `$7E6274` (`$01/$02/$04`), else the
saved bits `$7E6273`, else 0; the item rule is script flag 3 (`$7E2900` bit 3)
kept only while `$6274` has bit `$20`. On confirm the mod writes the bits,
masks the saved ones until the stage has started, then restores both, and
hands the A back to the selector on the next pad read. Input goes through
`$C0:0AF9`; the menu state is a save-state extension, so netplay peers agree.
Minimaps are built once, when the feature is enabled, from the player's own
ROM (stage scripts, map ops `0a/0b/0c`, collision layer `FF.79`, map streams at
`$D0:A000`, solid attributes at `$D0:8000`, spawns `22 P x y`) and drawn over
the finished frame. Nothing derived from the ROM is stored in the repository
or the package.

Validated on Linux LLE: all 52 arena/variant layouts from the ROM match the
running game's `$1052` and P1/P2 spawns, and all 52 on-screen minimaps match
them cell by cell; menu routes (open, move, B back, confirm Variant 2 and
Variant 4) in PT-BR and English; rollback probe with the menu in use shows
only the known PPU divergences, same as without the mod. Not run on Windows
or in a real netplay match.
