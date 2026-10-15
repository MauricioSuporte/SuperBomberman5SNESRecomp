# Subordinate Bomber / Subordinate Bomber jogável

## Português

Em **Mods**, ative **Play as Subordinate Bomber** e inicie o jogo. Na seleção de
personagens, o **Subordinate Bomber** (o rival do ZONE3-D) ocupa a célula do
**X**, no canto inferior direito da grade. Escolha-o com o direcional e confirme
com A, como os outros nove personagens.

- **Battle Mode:** funciona direto na seleção de personagens.
- **Normal Game:** ative também **Choose a character in Normal Game**; ele aparece
  na seleção aberta depois de escolher o save.
- **Cores:** com **Choose character colors with L and R** ativo, L/R alternam as
  seis paletas que o cartucho já tem para ele: branco, preto, vermelho, azul,
  verde e amarelo. Sem esse mod, o jogo usa a cor padrão do assento.
- Na grade ele aparece na cor vermelha de chefe até um cursor passar por ele,
  como os outros vilões. O HUD do Battle mostra o rosto dele no lugar do ícone e,
  quando ele é derrotado, o rosto triste (arte do mantenedor baseada na pose de
  Time Over), na cor escolhida.
- Retrato no topo: arte própria do projeto (fornecida pelo mantenedor), 24x24,
  com uma versão para cada uma das seis cores. Ela troca junto com a cor escolhida
  por L/R; a moldura e o fundo de cada jogador continuam os originais.

Jogabilidade, animações, placar e as seis paletas do Subordinate Bomber já
existem na ROM (personagem índice 9). Como a ROM não tem figura na grade, retrato
nem ícone de HUD para ele, o mod desenha a figura e o ícone durante a renderização
a partir dos tiles do próprio sprite dele, lidos da ROM, e usa os retratos
autorais. Nenhum gráfico ou paleta do cartucho é embutido no código.

Limitações: se o personagem secreto da célula do X já tiver sido desbloqueado no
seu save, a célula continua mostrando esse personagem e o Subordinate Bomber não
aparece. No HUD do Normal Game o ícone continua sendo o do Bomberman, como acontece
com os outros personagens escolhidos pelo mod. O mod é desativado por padrão,
vem pré-instalado nos pacotes v0.6.0, exige host v0.6.0 ou superior e o pacote
de dados pode ser gerado com:

```powershell
python tools/package-subordinate-bomber.py
```

## English

Enable **Play as Subordinate Bomber** in **Mods**, then launch. In character
selection, **Subordinate Bomber** (the Zone 3-D rival) takes the **X** cell at the
bottom right of the grid. Pick it with the D-pad and confirm with A like the
other nine characters.

- **Battle Mode:** available in the character picker.
- **Normal Game:** also enable **Choose a character in Normal Game**; it appears
  in the picker opened after choosing a save.
- **Colors:** with **Choose character colors with L and R** enabled, L/R cycle the
  six palettes the cartridge already has for it: white, black, red, blue, green
  and yellow. Without that mod the game uses the seat's default color.
- On the grid it shows the red boss palette until a cursor passes over it, like
  the other villains. The Battle HUD shows its face and, once defeated, a sad
  face (maintainer art based on its Time Over pose) in the chosen color.
- Top portrait: project-authored 24x24 art supplied by the maintainer, one
  version per selectable color; it follows the L/R choice while the frame and
  per-player background stay native.

Gameplay, animations, score board and Subordinate Bomber's six palettes already
exist in the cartridge (character index 9). The ROM has no grid figure, portrait
or HUD face for it: the figure and face are built while rendering from its own
sprite tiles read from the ROM, and the portraits are authored art. No
cartridge graphics or palettes are embedded in source.

Limits: if your save has already unlocked the secret character of the X cell,
that cell keeps showing it and Subordinate Bomber does not appear. The Normal
Game HUD keeps the Bomberman icon, as it does for every character chosen by the
mod. The mod is disabled by default, is preinstalled in the v0.6.0 packages, needs
host v0.6.0 or newer, and its data package can be built with
`python tools/package-subordinate-bomber.py`.

## Implementation

Research on the verified JPN image (Linux LLE build of the same sources):

- Every table read through `$16C2` has a tenth entry (`$C0:BB30/BB3A/BB44/BB4E`,
  `$C0:B9C0/B9CA`, `$C0:BB66`, `$C4:551B`, `$C5:242C`); sprite set `$19`, palettes
  `$48-$4D` plus `$4E` and rival `$4F` (`$C4:3CCF` uses base + 7). Zone 3-D
  (`$1031 = $6A`) creates rivals type 4 (purple, palette `$27`) and type 9
  (Subordinate Bomber, `$4F`) through `$C4:3CB5`.
- The picker script at `$C6:8AF4` fills grid cell 9 with character 1 color 5 when
  the secret slot is unlocked and with 0 (the X) otherwise (`$C6:8B17`). The grid
  drawer `$C2:122A` draws arrangement `$90` for characters above 9; arrangement
  set 6 has figures only for the nine stock characters.
- HUD faces are set-6 animations from 9-byte tables `$C2:1470` (normal) and
  `$C5:12FE` (alternate), read at `$C5:129B/$C5:1319/$C5:12EE`; index 9 read code.

In-memory patches (installed while enabled, restored on reset, guarded by the
expected stock bytes): `$C6:8B20` = 10, `$C6:8B2E` = 6, `$C2:1276` JSR to
`$C2:6000` (skip the X for character 10), ten-entry face tables at
`$C2:6020/$C2:6030` (index 9 → `$1D/$27`). The free area `$C2:6000-$603F` is
inside the `$C2:4207-$7FFF` `$FF` padding, apart from co-op's `$C2:4300`.

Render-only overlays (observer chained after the color cursor hint, restored
in reverse order at line 224): the standing frame (`$C9:5800`, `$C9:6340`,
`$C9:6360` left half, mirrored) is drawn at grid tile (25, 21) with borrowed OBJ
tiles `$1C0-$1EF`, an unused OBJ palette (7, else 5) loaded from palette
`$48 + color` (`$4F` for color 6); seat portraits keep the BG frame and native
OBJ palette and get the authored portrait: one 24x24 shape (`kPortrait`, 0
transparent, classes 1-9) at (4..27, 4..27), where native faces sit, so the BG
frame and per-player background show around it, and nine authored BGR555
colors per selectable color (`kPortraitColors[$6264]`) written render-only into
colors 1-9 of the seat's palette (skipped if any other visible sprite uses that
palette). The native object system shows the same VRAM tiles for every
portrait running the same animation (two seats on this bomber, or Bomberman,
whose portrait animation index 9 reuses), so the mod never writes native
tiles: it draws one shared copy into borrowed free 16x16 OBJ blocks
(`$100-$1EF`, none shown by a visible sprite that frame) and points the
seat's portrait sprites at them; each seat keeps its own palette and color.
HUD faces work the same way (alive `$1D` block from the standing frame,
defeated `$27` block from the authored 16x16 `kDefeatedFace`, indices into the
player's palette so it takes the selected color); HUD faces get the head over the face
object's private 16x16 sprite. Tiles shared with any other visible sprite are
left alone. Selection colors accept character 10 (palettes up to `$4F`); the
Normal picker hand-off accepts choice 10.

## Validation

- Native tests (`subordinate_tests`): patch bytes, idempotent install, restore
  on reset, refusal on unexpected bytes/no cartridge, grid figure position,
  mirror, palette and L/R color, every portrait pixel, its color set per seat
  color and shared-palette refusal, shared-tile
  refusal, palette exhaustion, HUD face, exact VRAM/OAM/CGRAM/high-OAM
  restoration. `selection_color_tests` covers character 10's six palettes and
  `normal_character_tests` its index-9 hand-off. All six native test
  executables pass on the Linux build.
- Runtime (Linux LLE, scripted, JPN ROM): Battle picker with colors and PT-BR
  (grid figure red then white/red by L/R, portrait), 5-seat Battle stage with
  P2 Subordinate Bomber in red (HUD face, sprite) through the score board win;
  Battle without the colors mod (default yellow); Normal Zone 1-1 in blue; Normal
  Zone 3-D with player and rival Subordinate Bomber together; Battle picker
  portrait cycled through all six colors with PT-BR. Logs had no BRK,
  `interp_cap`, unresolved abandon or fatal. Disabled control keeps the X.
- Not qualified: Windows build, physical controllers, full campaign, secret
  slot unlocked, every Battle stage/rule.
