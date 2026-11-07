# Battle — extra rules / Regras extras do Battle

## English

Package `superbomberman5.battle-rules` (feature `battle_rules`, default disabled). It adds a second page to the Battle
**Rule Settings** screen with two rules, forgotten on a reset (back to their defaults):

- **Instant Start** (default **Yes**) — with **No**, every round begins with the field shown, the screen slightly dimmed, nobody
  able to move or press anything (COM bombers included) during a 3-2-1 countdown, and the number of every bomber, human
  or COM, blinking over its spawn point. Then the dimming goes away and the round starts. The Battle timer does not
  run during the countdown.
- **Shuffle Spawn** (default No) — all bombers, humans and COM, swap start positions among the arena's spawn points,
  drawn again every round; nobody keeps its own spawn.
- **Disable Skills** (default No) — with **Yes**, the B + direction abilities of Honey, Kotetsu, Terrorin and the
  Super Bomberman 4 Bomber Kings are not used during the match (B only drops a bomb). Normal characters are unaffected.
- **Domino** (default No) — with **Yes**, a kicked bomb that runs into a bomb lying on the ground stops there and the
  bomb it hit starts rolling in the same direction (and so on down a line of bombs). Suggested by Kurobon '94.
- **Throw Range** (default **3**, native) — how many tiles a bomb thrown with the Power Glove flies, from 1 to 5.
  Left/Right change it (wrapping), A adds one. Bounces off blocked cells stay native (one tile each). Suggested by
  Kurobon '94.

Open the page with **Down on Bowling Bomber** or **Up on CPU Level**; a blinking arrow under the list says there is
more. Left/Right/A toggles a rule, Up/Down moves, and moving past either end returns to the original rows. The page
reuses the same background and the same hand cursor, and its labels use the native font (translated with PT-BR on:
"Início imediato", "Embaralhar", "Sem habilidades", "Dominó", "Alcance", Sim/Não). Moving and toggling play the native menu move sound.

How it works (JPN image, `docs/STATUS.md` has the evidence): the pad reader tail `$C0:0AF9` drives the page and clears
the arrays the menu scripts read (held, new, and the auto-repeat array `D+$40`); leaving the page hands one native
Down/Up back so the cartridge's own row wraps. The six native rows are blanked in BG1 (after the translation pass has
read its guard tiles) and the two rows are written with the translation mod's native-derived font; OBJ 0 (the hand) is
moved. The countdown starts from the stores that make `$0AEE` = 1 (`$C5:0007`, `$C2:0BC1`, `$C5:0912`), redirects the
actor update loop `$C4:221D` to its `RTL`, and restores the timer bytes `$0E1C-$0E1E`. Shuffle hooks the bomber
creation op at `$C2:16C7` and swaps `$1D28/$1D2A` with another seat's tile from the stage script's `22 P x y` entries,
using a generator saved in the state extension. Domino hooks the rolling bomb's blocked-cell check `$C5:2182` (X = bomb,
A = blocked cell, direction `$0E90 & 7`): when that cell holds a resting bomb, a trampoline in `$FF` padding at
`$C5:7540-$7559` calls the native ground kick `$C5:2293` on it (A = direction, `$8A` = the stopping bomb's cell) and
resumes `$C5:2182`, where the first bomb stops natively — the Yellow Louie's chain-kick path. Bombs kicked by a Yellow
Louie keep the Louie mod's own chain. Throw Range: the glove throw `$C5:2436` makes the carried bomb airborne with
`$0F10` = `$2C`, an index into the height arc at `$80:8CDA`; each airborne frame moves it 4 px (`$80:8C9F`) and reads the
arc at `$C5:20E7`, and height 0 (index `$37`) lands it — 12 frames, 48 px, 3 tiles. A hook at `$C5:2436`'s store
(`$C5:245B`) marks the bomb and the hook at `$C5:20E7` walks the same arc in 4 × range frames by rewriting `$0F10`
before the native read; the bounce arc (`$38`) is left alone. The trampoline is written to the in-memory cartridge only after checking that the
range is `$FF`, and restored on reset. Nothing is written to the ROM file.

Limits: validated on Linux/LLE with two humans and three COM bombers (see STATUS). Three or more humans on the
countdown marker and netplay with this mod have not been run. Domino and Throw Range were exercised on Linux/LLE (P1 in arena 1,
see STATUS); not on Windows, Android or netplay. The hooked routines must stay interpreted.

## Português

Pacote `superbomberman5.battle-rules` (recurso `battle_rules`, desativado por padrão). Adiciona uma segunda página à
tela de **regras** do Battle com duas regras, esquecidas num reset (voltam ao padrão):

- **Início imediato** (padrão **Sim**) — com **Não**, cada rodada começa com o campo à mostra, a tela levemente escurecida, ninguém
  se mexe nem aperta nada (os COM também ficam parados) durante a contagem 3-2-1, e o número de cada bomber, humano ou
  COM, pisca no seu ponto de início. Depois o escurecimento some e a rodada começa. O tempo da partida não corre
  durante a contagem.
- **Embaralhar** (padrão Não) — todos os bombers, humanos e COM, trocam de posição entre os pontos de início da arena,
  sorteados de novo a cada rodada; ninguém fica no próprio ponto.
- **Sem habilidades** (padrão Não) — com **Sim**, as habilidades de B + direção de Honey, Kotetsu, Terrorin e dos Bomber
  Kings do Super Bomberman 4 não são usadas na partida (o B só solta bomba). Personagens normais não mudam.
- **Dominó** (padrão Não) — com **Sim**, uma bomba chutada que bate numa bomba parada no chão para ali, e a bomba
  atingida passa a rolar na mesma direção (e assim por diante numa fileira de bombas). Sugestão do Kurobon '94.
- **Alcance** (padrão **3**, o original) — quantos tiles voa a bomba arremessada com a Power Glove (luva azul), de 1 a
  5. Esquerda/Direita mudam (dando a volta), A soma um. Os quiques em células bloqueadas continuam nativos. Sugestão
  do Kurobon '94.

Abra a página com **Baixo em Boliche** ou **Cima em Nível CPU**; uma seta piscando indica que há mais opções.
Esquerda/Direita/A alterna a regra, Cima/Baixo move, e passar de qualquer extremidade volta às linhas originais. A
página usa o mesmo fundo e a mesma mãozinha, e os textos usam a fonte nativa (traduzidos com o mod de tradução).
Detalhes técnicos e limites: veja a seção em inglês e `docs/STATUS.md`.
