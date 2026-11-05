# Battle — extra rules / Regras extras do Battle

## English

Package `superbomberman5.battle-rules` (feature `battle_rules`, default disabled). It adds a second page to the Battle
**Rule Settings** screen with two rules, both **off** by default and forgotten on a reset:

- **Round Timer** — before every round the field is shown with the screen slightly dimmed, nobody can move or press
  anything (COM bombers included) while a 3-2-1 countdown runs, and the number of each human player blinks over its
  spawn point. Then the dimming goes away and the round starts. The Battle timer does not run during the countdown.
- **Shuffle Spawn** — the players swap start positions among the spawn points of the arena, drawn again every round.

Open the page with **Down on Bowling Bomber** or **Up on CPU Level**; a blinking arrow under the list says there is
more. Left/Right/A toggles a rule, Up/Down moves, and moving past either rule returns to the original rows. The page
reuses the same background and the same hand cursor, and its labels use the native font (translated with PT-BR on:
"Cronômetro", "Embaralhar", Sim/Não).

How it works (JPN image, `docs/STATUS.md` has the evidence): the pad reader tail `$C0:0AF9` drives the page and clears
the arrays the menu scripts read (held, new, and the auto-repeat array `D+$40`); leaving the page hands one native
Down/Up back so the cartridge's own row wraps. The six native rows are blanked in BG1 (after the translation pass has
read its guard tiles) and the two rows are written with the translation mod's native-derived font; OBJ 0 (the hand) is
moved. The countdown starts from the stores that make `$0AEE` = 1 (`$C5:0007`, `$C2:0BC1`, `$C5:0912`), redirects the
actor update loop `$C4:221D` to its `RTL`, and restores the timer bytes `$0E1C-$0E1E`. Shuffle hooks the bomber
creation op at `$C2:16C7` and swaps `$1D28/$1D2A` with another seat's tile from the stage script's `22 P x y` entries,
using a generator saved in the state extension. Nothing is written to the ROM image.

Limits: validated on Linux/LLE with two humans and three COM bombers (see STATUS). Three or more humans on the
countdown marker and netplay with this mod have not been run.

## Português

Pacote `superbomberman5.battle-rules` (recurso `battle_rules`, desativado por padrão). Adiciona uma segunda página à
tela de **regras** do Battle com duas regras, ambas **desligadas** por padrão e esquecidas num reset:

- **Cronômetro** — antes de cada rodada o campo aparece com a tela levemente escurecida, ninguém se mexe nem aperta nada
  (os COM também ficam parados) durante a contagem 3-2-1, e o número de cada jogador humano pisca no seu ponto de
  início. Depois o escurecimento some e a rodada começa. O tempo da partida não corre durante a contagem.
- **Embaralhar** — os jogadores trocam de posição entre os pontos de início da arena, sorteados de novo a cada rodada.

Abra a página com **Baixo em Boliche** ou **Cima em Nível CPU**; uma seta piscando indica que há mais opções.
Esquerda/Direita/A alterna a regra, Cima/Baixo move, e passar de qualquer uma das duas volta às linhas originais. A
página usa o mesmo fundo e a mesma mãozinha, e os textos usam a fonte nativa (traduzidos com o mod de tradução).
Detalhes técnicos e limites: veja a seção em inglês e `docs/STATUS.md`.
