# Honey and Kotetsu / Honey e Kotetsu jogáveis

## Português

Em **Mods**, ative **Play as Honey and Kotetsu** e inicie o jogo. A seleção de
personagens passa a ter **seis colunas**: as cinco colunas originais ficam mais
próximas, a **Honey** fecha a fileira de cima e o **Kotetsu** fecha a fileira de
baixo, logo depois do Subordinate Bomber (ou do X). Os dois são os chefes das
fases secretas `ZONE1 - S`, `ZONE2 - S` e `ZONE3 - S` (ids `$71-$73`), que não
são selecionáveis no jogo original.

- **Cursor:** direita na quinta coluna vai para a Honey/Kotetsu; direita de novo
  volta à primeira coluna (e esquerda faz o caminho inverso). Cima/baixo troca
  entre Honey e Kotetsu.
- **Battle Mode:** funciona direto na seleção de personagens.
- **Normal Game:** ative também **Choose a character in Normal Game**; eles
  aparecem na seleção aberta depois de escolher o save (New Game e Continue).
- **Jogabilidade:** Honey e Kotetsu jogam como o Bomberman (mesmas regras,
  hitbox e itens). Na tela eles aparecem com os próprios sprites:
  - andando e parados nas quatro direções (também andando com a bomba erguida
    pela Power Glove);
  - soco da Boxing Glove (Y) e arremesso da Power Glove: Honey atira com a
    pistola e Kotetsu estoca com a lança, na direção do movimento;
  - vitória, comemoração e empate (tela **DRAW GAME**), placar e tela de
    campeão do Battle: alternam as duas poses que eles usam na tela de campeão
    (Honey com a arma atravessada, Kotetsu parado e acenando), no ritmo
    original;
  - montaria: o Louie (e o ovo antes de ser pego) mantém as cores originais e
    a Honey/o Kotetsu aparece montado nele, de frente para onde anda;
  - morte, atordoamento (bomba arremessada na cabeça) e Time Up: giram; no
    fim da morte aparecem as cinzas originais.
- **Retrato e HUD:** retrato no topo da seleção e rosto no HUD do Battle vêm da
  cabeça do quadro de frente de cada um.
- **Cores:** eles têm só uma paleta no cartucho, então L/R não mudam a cor deles.
- **Subordinate Bomber:** compatível. Com os dois mods ativos, o Subordinate fica
  na quinta coluna de baixo e o Kotetsu ao lado.

Quadros, paletas e gráficos vêm da sua ROM em tempo de execução (o arquivo
comprimido de gráficos dos dois é descomprimido na memória). Nenhum gráfico ou
paleta do cartucho fica no código-fonte.

Limitações: montado de frente (andando para baixo), o corpo fica atrás da
cabeça do Louie, como o Bomberman original. Na vitória eles não pulam junto
com o salto do Bomberman. O carrinho do Bad Bomber usa o sprite original.
O fundo da grade continua com o desenho original de cinco
colunas, então os personagens não ficam centralizados nos quadrados do fundo;
os colchetes de cursores vizinhos se encostam. No HUD do Normal
Game o ícone continua sendo o do Bomberman, como para os outros personagens. A
segunda seleção do cartucho (`$C6:9990-$C6:9E60`) continua com dez células. O
mod vem pré-instalado e desativado nos pacotes v0.9.0, exige host v0.9.0 ou superior e o pacote de dados pode ser gerado com:

```powershell
python tools/package-honey-kotetsu.py
```

## English

Enable **Play as Honey and Kotetsu** in **Mods**, then launch. The character
selection gets **six columns**: the five stock columns move closer together,
**Honey** ends the top row and **Kotetsu** ends the bottom row, right after
Subordinate Bomber (or the X). They are the bosses of the secret stages
`ZONE1 - S`, `ZONE2 - S` and `ZONE3 - S` (ids `$71-$73`), not selectable in the
original game.

- **Cursor:** right from column five reaches Honey/Kotetsu; right again wraps to
  column one (left does the reverse). Up/down switches between them.
- **Battle Mode:** available in the character picker.
- **Normal Game:** also enable **Choose a character in Normal Game**; they appear
  in the picker opened after choosing a save (New Game and Continue).
- **Gameplay:** they play as Bomberman (same rules, hitbox and items). On screen
  they use their own sprites:
  - walking and standing in four directions (also walking with a bomb lifted
    by the Power Glove);
  - Boxing Glove punch (Y) and Power Glove throw: Honey fires her gun, Kotetsu
    thrusts his spear, facing the move direction;
  - victory, cheer and draw (the **DRAW GAME** screen), Battle score board and
    champion screen: the two poses they use on the champion screen alternate
    (Honey holding her rifle across, Kotetsu standing and waving) at the
    native pace;
  - mounts: the Louie (and the egg before it is picked up) keeps its own
    colors and Honey/Kotetsu ride it, facing the move direction;
  - death, stun (thrown bomb on the head) and Time Up: spinning; the final
    death frame shows the original ashes.
- **Portrait and HUD:** the picker portrait and Battle HUD face are the head of
  each one's front frame.
- **Colors:** each has a single cartridge palette, so L/R do not recolor them.
- **Subordinate Bomber:** compatible; with both mods Subordinate stays in column
  five of the bottom row and Kotetsu sits next to it.

Frames, palettes and graphics are read from your ROM at runtime (their
compressed graphics archive is decompressed in memory). No cartridge graphics or
palettes are stored in source.

Limits: riding toward the camera (down) hides the body behind the Louie's
head, as for stock Bomberman. In victory they do not follow Bomberman's hop.
The Bad Bomber cart keeps the native sprite. The grid
background keeps its five-column art, so figures are not
centered on its squares and neighbouring cursor brackets touch. The
Normal Game HUD keeps the Bomberman icon, as for every character chosen by the
mod. The cartridge's second picker script (`$C6:9990-$C6:9E60`) still has ten
cells. The mod is preinstalled and disabled by default in the v0.9.0 packages,
needs host v0.9.0 or newer, and its data package can be built
with `python tools/package-honey-kotetsu.py`.

## Implementation

Research on the verified JPN image (Linux LLE build of the same sources):

- Secret stages `$71-$73` create Honey and Kotetsu as boss actors (`$16C7/$16C8`
  = `$3C/$3E`). They are not bombers: object set `$0D` holds their animations
  (`$1E-$22` Honey, `$2B-$2F` Kotetsu: right, down, left, up, spin — the same
  direction order as bomber walk animations 1-4 and stand `$46-$49`).
- Animation tables: `$C0:171D + set*3` (24-bit table; entry = 16-bit pointer to
  `frame, duration` pairs, `$07FF` = byte offset of the current pair); frame
  tables: `$C0:16CC + set*3` (entry = pointer to `count`, then 7-byte pieces
  `dx16, dy16, tile16, attr` with attr bit 0 = 16x16).
- Graphics: archive set 8 (`$0AE7`) entry `$CC:289D`, `$1E00` bytes,
  decompressed by `$C0:1CB0/$C0:1CCC` (control byte: mode in bits 5-7 —
  literal, previous+1, previous, copy 2/4/16/32/64 back in a 256-byte ring —
  and count-1 in bits 0-4). It uploads to VRAM in 16x16 blocks (TL, BL, TR, BR);
  Honey uses tiles `$00-$7F`, Kotetsu `$80-$EF` of it. Palettes: `$C1:8FC6`
  (Honey), `$C1:8FA6` (Kotetsu).
- Picker: script `$C6:89CF`. `$1034` is the cursor value = cell + 1 = choice;
  cells live in `$62C9` (character) / `$62D3` (color), ten each. The exit
  script copies the cell character into `$625F` (`$C6:A575`) and keeps the cell
  in `$6291`. Cursor objects: `$C0:C927` with `$C0:C976/$C0:C980`. Battle stage
  init `$C4:2A3C`: `$16C2 = $625F - 1`. Score board: script `$C6:7736` → FF.59 →
  `$C4:3BDE` stores `$625F - 1` again.

In-memory patches (installed while enabled, verified against the stock bytes,
restored on reset; free `$FF` area `$C2:6040-$60B0`, after the Subordinate
mod's `$C2:6000-$603F`):

- The two cell arrays move to twelve-entry arrays at `$7F:F800`/`$7F:F80C`
  (19 + 15 script operands and native `$C2:1240/$12B5/$12C3`). Cells 10/11 hold
  choices 11 (Honey) and 12 (Kotetsu).
- `$C2:12D2` packs the five figure columns to tiles 5/9/13/17/21; column six is
  tile 25. Cursor positions read twelve-entry copies (`$C2:6070/$607C`).
- Movement blocks `$C6:8C02/$8C78/$8CEE/$8D62` become table lookups
  (`$C2:6040/$6050/$6060`, op `$36`).
- Portrait helpers FF.66/FF.67 (`$C2:1356/$13E7`) clamp indices above the
  ten-entry tables to Bomberman (`$C2:6088`).
- `$C4:2A3C` maps choices 11/12 to Bomberman and stores each seat's choice at
  `$7F:F818+seat` (`$C2:6094`); `$C4:3BDF` (`$C2:60A4`) clamps the score board.
  The Normal picker hand-off records the same markers; they are cleared at the
  title/player menu (`$1031 = 2`). Being WRAM, they follow save states/rewind.
- `$7F:F800-$7F:F81C` was zero in every one of 154 captures (menus, pickers,
  Normal and Battle stages, score board); no ROM operand found pointing there.

Pose mapping (bomber object set `$10`, four-way groups right/down/left/up,
identified with scripted routes and item pokes): walk 1-4 and `$2F-$32`
(carrying); action `$27-$2A` (punch), `$33-$36` (throw), `$3B-$3E`; cheer
`$15` and victory `$20`; stand for the other four-way groups (`$46-$49`,
`$4A-$4D`, lift `$2B-$2E`, hold `$37-$3A`, airborne `$17-$1A`, cart `$42-$45`,
...); spin for the rest (death `$16`, stun `$21`, Time Up `$1F`). Actions use
Honey's `$23-$26` step 2 and Kotetsu's `$30-$33` step 2; victory uses `$24`
step 4 / `$31` step 1. Wide frames get a second column of blocks; a few
Kotetsu weapon tiles (`$F0-$F7`) are outside this archive and are skipped.
Items (`$C5:4195` table, abilities `$7E4ED5`): `$0C` power glove (bit 2),
`$11` boxing glove (bit 3, Y), `$0B` bit 1, `$04` remote (bit 0); the egg is
item `$15` (revealed from a destroyed soft block).

Which sprites are replaced: the pieces of the bomber's current frame (24-bit
pointer `$7E2EEA/$7E2F4A` + object) found in OAM at object + piece offset in
its palette slot. The shown OAM can lag the pointer by one or two steps, so the
last three distinct pointers (sampled each frame) are tried as well. Mount
(set `$01`, its own object), eggs and other sprites sharing the slot are left
alone. The figure uses an OBJ palette no visible sprite uses (7 first), and
recolors the seat's slot only when every palette is in use. `$1782+seat`: bit
7 riding (the figure stands with its feet where the native rider frame ends,
no shadow), bit 1 jumping onto the mount (no shadow; the native jump shadow
stays).

Victory/cheer (`$20`/`$15`, also the DRAW GAME screen, mode `$0A`, and the
champion screen, mode 5): object set 6 animations `$50` (Honey, frames
`$A2/$A3`, 7 frames each) and `$4E` (Kotetsu, `$9E/$9F`, 16 each), drawn from
the champion screen archive `$D6:D871` (`$0D40` bytes, ends `$D6:E0BB`, same
block layout), checked at install; without it the still victory frame is used.

Render-only overlays (observer chained after Subordinate's, restored in
reverse at line 224): stage — for each seat with a marker, the bomber's sprites
(the pieces of its current frame, see above) are replaced by the matching
Honey/Kotetsu frame composed from the decoded archive into 16x16
borrowed OBJ blocks (reusing the bomber's OAM slots first) in a free OBJ
palette (see below); blocks are borrowed before anything is hidden, so a
full OBJ table leaves the native sprite. HUD face: Bomberman's face sprite is
pointed at a block with the head, in the skin's palette. Picker: figures for cells 10/11 at tile
column 25 in free OBJ palettes 7/5 (a 15-color merge of both palettes when only
one is free) and the seat portrait quarters pointed at a crop of the front frame.

## Validation

- Native tests (`honey_kotetsu_tests`): synthetic stock bytes and archive stream
  (ends exactly at `$CC:40E4`), every patch byte, movement tables (inverse and
  reachable), idempotent install, refusal on unexpected bytes or a corrupt
  stream, restore on reset, markers and title clear, stage replacement (pose,
  pixels, palette, untouched far sprite, exact restoration), blinking frame,
  picker figures and portrait; native-piece matching (nearby same-slot
  sprite untouched, free palette, fallback slot), riding lift and champion
  alternation. All nine native test executables
  (including subordinate, selection color and Normal character with the new
  helpers) and the Python tests pass.
- Runtime (Linux LLE, scripted, JPN ROM): Battle picker with PT-BR, colors and
  Subordinate (cursor 5 → Honey → Kotetsu → wrap, portraits); five-seat Battle
  (P1 Honey, seat 2 Kotetsu, CPUs) for ~9,600 frames through the score board and
  the next round; Normal Zone 1-1 as Honey; Normal `ZONE1 - S` as Honey and
  `ZONE3 - S` as Kotetsu next to the real bosses; disabled/Subordinate-only
  controls keep the stock five-column picker. Logs had no BRK, `interp_cap`,
  unresolved abandon or fatal.
- Animation routes (Linux LLE, each beside the same route without the mod):
  power glove lift/carry/throw (item `$0C` placed by poke), boxing glove punch
  (item `$11`, Y), idle, own-bomb death, stun (`$15BA` poke), victory (CPUs
  forced dead) and score board, Time Up, round restart; Honey and Kotetsu for
  glove routes. Picker portraits and Battle HUD faces re-checked.
- Louie/draw/champion routes (Linux LLE): egg `$15` placed by poke, collected,
  ridden right/down/left (kangaroo Louie in native colors, egg colors kept);
  DRAW GAME screen; victory with dumps every 2 frames (60 frames); champion
  screen with Honey and with Kotetsu as winner, dumped every frame (48
  frames); walking dumped every frame (60 frames): no frame fell back to the
  native sprite. Regressions: gloves, stun, death, picker.
- Not qualified: Windows runtime of this mod (the v0.9.0 release CI builds and
  tests it on Windows), physical controllers, full campaign, coop P3-P5
  as Honey/Kotetsu, every Louie type and Battle stage/rule.
