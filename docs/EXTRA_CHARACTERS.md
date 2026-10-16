# Personagens extras do Super Bomberman 5 / Super Bomberman 5 Extra Characters

## Português

Em **Mods**, ative **Play as the Super Bomberman 5 Extra Characters**
(pacote `superbomberman5.extra-characters`). Ele reúne num só mod o que antes
eram os mods **Honey and Kotetsu** e **Subordinate Bomber**, as habilidades
B + direção da Honey e do Kotetsu (que antes vinham com o Bomber Kings) e um
personagem novo, o **imperador Terrorin**.

A seleção de personagens passa a ter **sete colunas**:

```
 Bomberman ... (5 originais)            Honey     Terrorin
 (5 originais de baixo, Subordinate no X) Kotetsu   -
 Bomberman SB4 Hammer Jet Lady Bazooka Great       -      (com o Bomber Kings)
```

- As colunas originais vão para a esquerda (colunas de BG 3/7/11/15/19);
  Honey e Kotetsu ficam na coluna 23 e o Terrorin na 27.
- O **cursor pula as células vazias**: direita/esquerda dão a volta na
  fileira; cima/baixo na coluna do Terrorin não saem dele.
- Com o **Bomber Kings** sem este mod, a grade continua com seis colunas e a
  fileira do SB4 embaixo (a coluna da Honey/Kotetsu fica vazia).
- **Subordinate Bomber**: fica na célula do X (com seis cores do cartucho
  pelo mod de cores L/R), como no mod antigo.

### Emperor Terrorin

O Terrorin é o chefe final do **ZONE5 - J** (fase `$6F`). Na terceira etapa
ele sai da nave e luta a pé, com bombas. É essa versão a pé que vira
jogável: andar e parar nas quatro direções, erguer, carregar e arremessar
bombas (Power Glove), comemorar, ser atingido, Time Up, montar no Louie, a
**morte do chefe** (a cabeça voa para a direita e o corpo cai), retrato na
seleção e rosto no HUD, tudo **decodificado da sua ROM**
em tempo de execução (nenhum gráfico do cartucho fica no código).

Ele joga como o Bomberman (mesmas regras, hitbox e itens). Tem uma única
paleta no cartucho (L/R não mudam a cor dele).

### Habilidades (B + direção)

Segure **B** e aperte uma **direção** (ou aperte B segurando uma direção).
B sozinho continua detonando a bomba-controle; montado no Louie, o B é o
normal.

| Personagem | Habilidade | Depois |
|---|---|---|
| Honey | tiro da luta contra ela (0,9 s parada), o projétil segue até um bloco e mata quem acertar | debuff |
| Kotetsu | estocada de espada (1 s) no bloco da frente | debuff |
| **Terrorin** | a **barreira** da luta no ZONE5 - J (abaixo) | debuff |

**Barreira do Terrorin** (a mesma sequência do chefe, `$C3:3F2D`-`$C3:4090`):

1. Ele **gira parado por 64 quadros**, intocável (explosões, inimigos e a
   própria barreira não o atingem).
2. Sobe a **barreira azul** nas células do quadrado 5 x 5 em volta dele, sem
   os quatro pilares de dentro: ela cerca o quadrado **3 x 3** em que ele
   está. Uma célula da barreira mata quem estiver nela ou entrar nela, como
   uma explosão. Ela só fere quem luta contra ele: nunca o próprio
   Terrorin, os aliados do Normal Game nem o time dele no Team Battle. O
   azul é a janela de cor somando a cor fixa azul, aparecendo aos poucos
   com o mesmo fade de 16 quadros do cartucho, e toca o **som da barreira**
   do chefe (efeito `$0288`).
3. Depois de 80 quadros o **Clock** (item `8`) sai **voando** dele para um
   dos lados, como o item que um jogador perde, e cai numa célula livre; a
   barreira some ao fim de 192 quadros.
4. **Clock no Battle** (a regra do chefe): se o **Terrorin** pegar o Clock
   dele, **todos os outros jogadores** (só o time adversário no Team
   Battle) ficam **paralisados** por 144 quadros; se um **adversário**
   pegar, o **Terrorin** fica paralisado (parado, de pé). No
   **Normal Game** o Clock tem o efeito normal do item (congela os inimigos).

O **debuff** (como no Super Bomberman 4): cerca de 10,7 s contados do uso,
com metade da velocidade e a habilidade bloqueada.

### Limitações

- O som da barreira não existe no conjunto de sons carregado no Battle;
  ele é tocado por um APU privado (veja as notas técnicas), no mixer de
  mods do executável, e não pelo driver do jogo.
- No Battle o efeito do Clock pego por quem não é o dono nem adversário (um
  colega de time) é nenhum.
- Jogadores da CPU não usam habilidades.
- A barreira não é desenhada quando a fase já usa a janela de cor ou o
  color math naquele quadro (ela continua valendo).
- Validado em rotas Linux LLE (ver docs/STATUS.md); controles físicos e o
  build Windows não foram testados por mim.

## English

Enable **Play as the Super Bomberman 5 Extra Characters** in **Mods**
(package `superbomberman5.extra-characters`). It merges the former **Honey and
Kotetsu** and **Subordinate Bomber** mods, Honey's and Kotetsu's B + direction
abilities (previously shipped with Bomber Kings) and a new character,
**Emperor Terrorin**.

The character selection gets **seven columns**: the stock columns move left
(BG columns 3/7/11/15/19), Honey/Kotetsu take column 23 and Terrorin column
27 (top row). The cursor skips empty cells. Subordinate Bomber takes the X
cell (six cartridge colors with the L/R color mod). With Bomber Kings alone
the grid keeps six columns.

**Terrorin** is ZONE5 - J's final boss (stage `$6F`); in his last phase he
leaves his ship and fights on foot with bombs. That on-foot Terrorin is
playable, with walking/standing, Power Glove lift/carry/throw, cheer, hurt,
dazed, Time Up, riding, picker portrait and HUD face, all decoded from your
ROM at runtime, including the boss's defeat (his head flies off to the right
while the body falls). He plays as Bomberman and has one cartridge palette.

**B + direction**: Honey's shot, Kotetsu's sword, and **Terrorin's barrier**,
the boss's own sequence: he spins in place for 64 frames, untouchable; a blue
barrier rises on the 5 x 5 square around him minus its four inner pillars,
fencing the 3 x 3 square he stands in (its cells kill like an explosion, but
never Terrorin, Normal Game allies or his Team Battle teammates), fading in
over 16 frames with the boss's barrier sound; a **Clock** (item `8`) then
flies off him to one side like an item a player loses and lands on a free
cell; the barrier drops at frame 192.
In **Battle**, Terrorin taking his Clock paralyzes every other player (only
the other team in Team Battle) for 144 frames, and an opponent taking it
paralyzes Terrorin (frozen standing). In **Normal Game** the Clock keeps its stock
effect (it freezes enemies). Afterwards Honey, Kotetsu and Terrorin move at
half speed for about 10.7 s.

Limits: the barrier sound is missing from the Battle sound set, so it is
rendered in a private APU and played through the executable's mod mixer;
CPU players do not use abilities; the tint is skipped on frames that
already use the color window or color math. Linux LLE scripted routes only
(see docs/STATUS.md).

## Technical notes

- Plugin `superbomberman5.extra-characters.characters`, registered by
  `src/extra_characters_mod.c` (the extra-character core, formerly
  `honey_kotetsu_mod.c`), which also turns on `src/subordinate_mod.c`
  (`SubordinateActivate`).
- Picker: cell arrays relocated to `$7F:F800` (characters, 20) and
  `$7F:F814` (colors, 20); seat choice `$7F:F828`, seat color `$7F:F82D`;
  movement/cursor tables at `$C2:60C0-$6135` (20-entry tables; empty cells
  never reached); stage init stores the choice to `$7F:F828` and the portrait
  clamp masks `AND #$1F` (choice 19). Choices: 11 Honey, 12 Kotetsu, 13-18
  Bomber Kings, 19 Terrorin.
- Terrorin art: object set `$0E`, archive `$CC:657D` (`$1E80` bytes, the
  decompressor's last control read is `$CC:8000`, so the stream end checked
  is `$CC:8001`), palette `$C1:906E` (palette id `$37`, equal to phase-3
  CGRAM slot 4). Animations (R/D/L/U): walk `$39-$3C`, lift `$3D-$40`, carry
  walk `$41-$44`, throw `$45-$48`, spin `$49`, cheer `$4E`, defeat `$50`
  (frames 152-169; in 153-165 piece 0 is the head, drawn apart moving
  right), hurt `$51`, guarded spin `$52`, front `$53`. Standing uses the
  walk's first step (`$4A-$4D` are crouch frames, shorter than walking). Found by forcing stage `$6F`, killing phases 1-2 (`$1667=0`,
  `$15A7=$40`) and dumping phase 3 every 8 frames.
- Barrier: `$C3:3F2D` spins `$40` frames (`$15A2` bit 2), then ORs `$0C` into
  `$1052` at the 21 offsets of `$C3:4091` (bit 2 lethal: player death via
  `$1CCA` at `$C4:2CF0`; verified by poking `$0C` under P1 in Zone 1-1),
  windows the screen ±`$28` px (`$C1:313C`), fades `$0317` 0→16, holds `$40`,
  creates the Clock script `$C2:2629` (A = `$72+$1632`), fades out and clears
  the cells (`$C3:4084`). The boss's barrier sound is `$0288` (command `$74`
  via `$C0:86D0`), which the Battle sound set lacks: sending it froze the
  game, so `src/sb5_sound.c` renders it in a private APU (streams via
  `$C5:F69B` like `$C0:0E31`, sets via `$DA:C2E0` like `$C0:0E9C`, command
  sent like `$C0:0E7F`). The mod raises only free cells and clears only its
  own; `$C4:2CFE` (`AND #$04` on `$1CCA`) is hooked to spare the owner and
  his allies. Clock: thrown with `$C4:398C`, the lost-item flight (A = 8,
  `$8D` = seat, `$1CC7` = 0; both saved and restored); pickup
  effect `$C5:42D3` (picker in `$8D`) is hooked to record who took whose
  Clock; paralysis is `$C4:4503` (X = seat, 144 frames; the boss's
  `$C4:3A94` only does P1/P2). The native pickup also stops the Battle clock
  (`$0E20`, 960 frames); the mod clears it for Terrorin's Clock.
- Ability state in `$7F:F840-$F8F0` (per seat 32 bytes, pending B, speed
  accumulator, dazed counters `$7F:F8EA`, Clock event `$7F:F8EF`), zero in
  453 research captures; no ROM operand points there.
- Package: `python tools/package-extra-characters.py`.
