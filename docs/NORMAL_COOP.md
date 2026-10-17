# Five-player Normal Game / Normal Game com cinco jogadores

## Português

O menu é identificado pelas duas linhas originais de jogadores em BG3 e pelo
modo de título. Endereços de retorno dos scripts podem conservar o histórico
da demonstração e não são usados para decidir se essa tela está aberta.

O mod opcional `superbomberman5.normal-coop` v0.1.0 está incluído nos pacotes
Windows e Linux a partir da v0.5.0. Em **Mods → 5 Players**, ative **Play Normal Game
with up to five players**. O menu oferece um a cinco jogadores; use cima/baixo
e Start. A tradução PT-BR funciona junto. As opções, o aviso de Start, o cursor
e o copyright cabem na tela. Como no menu original, as opções são brancas, o
aviso PUSH START BUTTON pisca e mover o cursor toca o efeito sonoro. Ao
confirmar, a animação do cursor acontece ao lado da opção escolhida e as cinco
opções continuam na tela até o fade-out.

| Jogadores | Vidas iniciais compartilhadas |
|---|---|
| 1 | 3 |
| 2 | 4 |
| 3 | 5 |
| 4 | 6 |
| 5 | 7 |

Nas fases comuns, P3 e P5 aparecem junto do P1 e P4 junto do P2: P3 e P4
um tile ao lado do spawn original de P1/P2 e P5 um tile acima ou abaixo de P1,
sempre na direção do meio do mapa. O mod opcional
`superbomberman5.coop-corner-spawns` (**Mods → 5 Players → Spawn P3-P5 in
the corners and centre**) troca esse padrão pelo layout anterior: P3 no canto
inferior esquerdo, P4 no inferior direito e P5 no centro. Ele só funciona
junto com o mod de cinco jogadores. Nos bosses, P3 usa dois tiles à esquerda do spawn
original de P1, P4 dois à direita de P2 e P5 o ponto intermediário. P1/P2
mantêm os spawns originais. Um ponto bloqueado é substituído pelo chão seguro
mais próximo, sem soft/hard blocks, com uma saída e sem outro jogador.
Os blocos não são removidos. A posição é conferida de novo depois que o mapa
da fase é montado, então arenas menores (como ZONE2-4, ZONE3-D e ZONE3-E) não
deixam P3-P5 fora da arena ou dentro de paredes. Encostar em inimigos causa
dano a P3-P5 como a P1/P2.

Qualquer jogador que entra em um portal aberto conclui a fase. Bombas, fogo,
velocidade e demais itens de cada um dos cinco jogadores seguem para a fase
seguinte como os de P1/P2 no original.

Os chefes Bomber das fases de chefe (`ZONE1 - A` etc.) ocupam slots de bomber
logo depois dos jogadores. O jogo tem cinco slots de bomber, então nessas
fases jogam até quatro pessoas (três na fase com dois chefes). Os assentos
excedentes, a partir de P5, ficam de fora daquela luta, não podem voltar com
Start nela e retornam na fase seguinte com personagem, cor e itens que tinham.

O HUD original (moldura, ícones, coração e placares de P1/P2) continua sendo
desenhado. As animações usam cinco páginas de sprites; um alocador próprio
reaproveita a página do próprio jogador quando não há página livre, então os
personagens animam e quem volta com Start reaparece com gráficos. P3, P4 e P5
usam as paletas de sprite 7, 6 e 5; os inimigos ficam com as paletas 3 e 4.

Depois da animação de morte, Start no controle do jogador morto consome uma
vida para voltar, desde que haja um companheiro vivo. Esse Start não pausa a
partida. Com zero vidas, jogadores vivos continuam jogando, mas mortos não
podem retornar. A morte de todos usa retry/Game Over, Continue e End originais.
Mortos permanecem ausentes nas transições de fase até voltarem com Start; a
última participação é mantida para o retry original.

Ative também **Choose a character in Normal Game** e **Choose character
colors with L and R** para escolher personagem e cor em todos os controles
selecionados. A seleção aguarda todos; as escolhas são mantidas nas mortes e
no Continue. Os controles são configurados no launcher, que mostra as abas
PLAYER 1–5 (P2–P5 ficam no multitap da porta 2). P3–P5 começam sem
dispositivo: escolha Gamepad ou Teclado e configure as teclas de cada um.

O `.snesmod` contém manifesto e licença, sem gráficos ou bytes da ROM. Exige
o host v0.5.0 ou superior; importar em uma release antiga não atualiza o
executável. O mod fica desabilitado
por padrão. Para empacotar: `python tools/package-normal-coop.py` e
`python tools/package-coop-corner-spawns.py`.

Com 5 jogadores, os Louies aparecem normalmente e as animações de quem está
montado não travam mais.

Na ZONE3 - D com 3 a 5 jogadores, o Pirate Bomber volta a montar o Louie
Preto; antes o P3 recebia um Louie Vermelho, o P1 voltava montado e travado
depois do recomeço da fase e o jogo congelava quando ele morria.

Com 1 ou 2 jogadores, entrar montado no Louie numa fase de chefe mantém o
deslocamento de montaria do próprio jogo (`$1CBF`); antes o mod forçava o
valor de fase comum e o Louie ocupava o lugar do chefe (na ZONE4 - G o chefe
não aparecia e o jogador ficava parado).

## English

The menu is identified by both original player rows on BG3 and title mode.
Script return addresses can retain attract-mode history and are not used to
decide whether this screen is open.

Optional `superbomberman5.normal-coop` v0.1.0 is preloaded in the v0.5.0 and
newer Windows/Linux setup packs. Enable **Play Normal Game with up to five players** under **Mods →
5 Players**, select 1–5 players with Up/Down and confirm with Start. Initial
shared lives equal player count + 2. PT-BR supports the new layout. The
launcher shows PLAYER 1–5 tabs; players 3–5 start with no device, so assign
each a gamepad or its own keyboard keys (`keybinds.ini` `[player3]`..).

In regular stages P3 and P5 start next to P1 and P4 next to P2: P3/P4 one
tile beside the original P1/P2 spawn and P5 one tile above or below P1, always
toward the middle of the field. The optional
`superbomberman5.coop-corner-spawns` mod (**Mods → 5 Players → Spawn P3-P5 in
the corners and centre**) replaces this with the earlier layout: P3 in the
lower-left corner, P4 in the lower-right corner and P5 in the centre. It only
works together with the five-player mod. Boss spawns use the
original P1/P2 coordinates: P3 two tiles left of P1, P4 two right of P2 and
P5 between them. Blocked or enclosed positions move to the nearest safe floor;
blocks and the original P1/P2 spawns are preserved. Positions are checked
again once the stage collision map exists, so smaller arenas (Zone 2-4,
Zone 3-D/E) no longer place P3-P5 outside the arena. Enemy contact damages
P3-P5 like P1/P2.

Any player entering an open exit warp completes the stage, and every seat
keeps its bombs, fire, speed and other items into the next stage as P1/P2 do
in the original. The menu keeps the original white rows, blinking PUSH START
BUTTON and cursor sound. The confirmation bounce stays beside the chosen row
and the five rows remain through the fade-out.

Rival bombers in boss stages take the bomber slots after the players. There
are five bomber slots, so up to four people fight those bosses (three in the
two-rival stage). Higher seats sit that fight out, cannot rejoin it with
Start, and return in the next stage with their character, color and items.

The original HUD frame, icons and score boxes remain visible. Animated
sprites, Start rejoin graphics and P3's palette work with all five seats.

A dead player can press Start after cleanup to spend one shared life and
rejoin while a teammate is alive. Rejoin does not pause. Simultaneous requests
cannot overspend the last life. Stock retry, Game Over, Continue and End remain
in use. Enable the Normal character picker and L/R color mod to select for
every chosen seat; choices survive death and Continue. The package needs a
rebuilt native host and does not update previously published executables.

## Implementation and validation

The adapter uses the existing interpreted-PC hook and authored 65816
trampolines in unused in-memory cartridge space. The ROM file is unchanged.
The common campaign spawn command calls the original `$C4:2A69` allocator for
participating seats. Collision maps at `$1052/$1152` supply the safe-floor
search. Living checks and death/rejoin logic cover all five seats. Story
scripts retain their one/two-player choreography.

Several cartridge loops visit only P1/P2 with `CPX #$02`: script op `$12` at
`$C2:0C9F` (players on a tile, used by exit warps and floor switches), the
floor gimmicks at `$C5:6195/$C5:61C6`, the cell hold at `$C1:6BA1` and point
damage at `$C0:373F`. Each bound is raised to the stage roster; the point
damage enemy bound at `$C0:374D` starts at actor 10. A warp entered by P3-P5
previously never finished the stage.

Campaign spawns (`$C2:4300`) run before `$C5:3D3F` copies the stage map to
`$1052`; the safe-floor search then saw an empty map. At that routine's return
(`$C5:3D5D`) seats 3-5 standing on an unsafe tile move to the nearest safe
floor, updating `$16AA`, `$4E8D/$4EA5`, `$182A/$1872` and the object position
`$055F/$05BF`.

Enemy code keeps per-player distances in six interleaved two-entry arrays at
`$1FF4-$1FFF` (filled by `$C3:46CC`, cleared by `$C3:475C`). Contact damage
(`$C3:47C9`, `$C3:4800`) and the large enemies' area hits (`$C3:1120`,
`$C3:1330`, `$C3:2C20`) looped over two entries. In co-op the 58 operands move
to five-entry arrays at `$1FC0-$1FDD` (never written in any recorded campaign
dump), the loops cover the roster and `$C3:FF70` fills seats 3-5 before the
stock P1/P2 pass. The nearest-player chooser `$C3:4641` keeps its two-entry
direct-page buffers, so enemies still steer toward P1/P2.

Five seats own the campaign's five OBJ pages, so a Louie gets no page of its
own. The stock page service (`$C5:6AE3`) kept retrying that request first in
its queue, freezing every later animation request (riders included). With
five seats it now runs through `$C5:7140`: the failed request starts its
animation without tiles and the queue moves on, and the extra-character
overlay draws the Louie from the cartridge.

Boss start `$C4:3C60` also names the rivals by literal slot: it gives them
equipment through `$C4:2906` (slots 2/3, operands `$C4:3C83/$3C8C`) and seats
ZONE3 - D's Pirate Bomber on its Black Louie (`$C0:B9B7` type 6) with slot 2
(operands `$C4:3C98/$3C9F`), after loading `$1CBF` from `$C0:B9AE` at
`$C4:3C75`. In co-op those literals now follow the rival move and `$C4:3C75`
loads the five-seat 5. Before, P3 was marked as riding a Red Louie, the
rival's Black Louie type landed in P2's mount slot, and the sprite object list
lost its end marker (object `$62`), so the game froze in `$C0:12E3` when the next
object was created (often P1's death after a stage restart).

With one or two players the adapter leaves the mount offset `$1CBF`
(mount actor = rider + `$1CBF`) to the cartridge: boss stages set 3 or 4
from `$C0:B9AE` at `$C4:3C78` because their rivals own slot 2. The adapter
previously wrote 2 every campaign frame, so a mount carried into a boss stage
took the boss's slot (ZONE4 - G never started and the rider stood still).
Now it only undoes its own five-seat value 5.

Boss stages `$67-$6E` create their rival bomber in actor slot 2 (`$6A` adds
slot 3) through `$C2:16B8/$C2:16EE` and `$C4:3CCF`. Five-entry bomber tables
(`$0FEA`, `$0FEF`, `$101F`, ...) indexed by `$8D` cannot hold a sixth bomber:
the rival previously replaced P3 and P5 was created in slot 5, whose indirect
jump through `$0FEF/$0FF4` ran data in bank `$C4`. In those stages a trampoline
at `$C2:4340` moves rivals to the slots after the players, 110 literal slot
operands in the boss choreography (`$C5:2600-$C5:39FF`) follow, and the stage
roster is `min(players, 5 - rivals)`. Benched seats keep their bomber fields
(`$16C2`, `$16DA`, `$1662`, `$167A`, `$1782`, `$15D2`, `$15EA`, `$17CA`,
`$4EBD`, `$4ED5`) in the snapshot extension and get them back at the next
regular stage.

Player actors occupy 0–4; mount companions and character backups move to
5–9; NPC allocation starts at 10. This prevents original P1/P2 mount backups
from overwriting P3/P4 characters. Fourteen NPC actor slots remain; unusually
large NPC/companion workloads need campaign validation. The original two-score
HUD remains. No five-score HUD or enemy difficulty scaling is added.

The stock HUD selector at `$C5:066C` keeps running, so the frame, icons, heart
and score boxes are drawn from the P1/P2 state as in the original game; the
adapter rewrites `$1D25/$1D24` at the `$C5:060E` hook right after it. Player
sprites use five 1 KiB OBJ pages (`$1D00`, VRAM `$6000-$6BFF`). The stock
allocator at `$C5:6AF5` needs a spare page before it releases the requester's
current page, so with five resident players every animation change failed:
characters froze, and a rejoined player was revived without graphics. An
authored allocator at `$C5:7100` takes a free page when one exists and
otherwise refills the page the requesting object already owns. Seats 3–5
request OBJ palette slots 7, 6 and 5 (`$0348`), because slot 3 belongs to
campaign enemies and P3 previously drew with an unrelated palette. Enemies keep
slots 3–4; mount or item palettes that need slots 5–7 can fall back to another
palette in five-player games.

The fixed-size snapshot extension saves selected seats, participation, input
edges, held-Start suppression, pending native operations and Continue choices.
Guest trampoline code is reconstructed from bounded state. SRAM retains its
original layout; campaign records store a compatible one/two-seat mask.

Focused tests cover navigation, 3–7 initial lives, collision maps, enclosed and
occupied cells, boss offsets, life/teammate gating, held Start, participation,
Continue choices, state bounds and full patch restoration. Character tests
cover three/four/five confirmations. Rendering tests cover both languages,
five rows, copyright glyphs, untouched OBJ graphics and restoration of
VRAM/OAM/window state.

Runtime scenarios use the verified JPN image, Windows and LLE: one through five
players in Zone 1-1; five independent character/color choices; Start rejoin for
every seat; last-life arbitration; P3 as sole survivor; Game Over and Continue.
Extra controllers use the real multitap/NMI path in an opt-in test build.
Synthetic deaths enter native cleanup atomically. Captures stay in ignored
`.bootstrap/`.

Additional runtime checks: P3-P5 entering an exit warp with five players and
items carried to the next stage; Continue -> File opening the character
picker; five players entering `ZONE1 - A` (four fight, the rival in slot 4)
and `$6A` (three fight, rivals in slots 3/4) from Continue without the former
freeze. Defeating a boss, boss intro/ending choreography, the full campaign,
all mount/item interactions and five physical controllers are not qualified.
Back (B) in the Normal character picker leaves a black screen in the headless
harness for both New Game and Continue, also with the previous version of the
picker; this is not resolved.

## Reproduce checks

With `BUILD_TESTING=ON`, run the four focused executables:

```powershell
./build-lle/normal_coop_tests.exe
./build-lle/normal_character_tests.exe
./build-lle/selection_color_tests.exe
./build-lle/ptbr_render_tests.exe
```

Counts can be checked with a standard host. Independent extra-pad and lifecycle
checks need the fixture and a supplied local ROM:

```powershell
./.tools/toolchain/bin/cmake.exe -S . -B build-lle -DSB5_COOP_INPUT_FIXTURE=ON
./tools/build-windows.ps1
python tests/normal_coop_runtime.py --exe build-lle/SuperBomberman5SNESRecomp.exe --rom 'YOUR_ROM.sfc' --fixture
./.tools/toolchain/bin/cmake.exe -S . -B build-lle -DSB5_COOP_INPUT_FIXTURE=OFF
./tools/build-windows.ps1
```

The fixture is off by default and must stay off in normal/setup packages.

The overlay also runs during the confirmation fade (mode 3, native BG3 rows
still in VRAM; input is ignored there). The native cursor is OAM 0 with tiles
`$1E8-$1EE` on y `$7F`/`$8F`. The idle tile is pinned to the selected row
(Up/Down briefly toggles the stock two-row y); only the confirmation bounce
(-5..+1) keeps its offset. Previously only the idle tile was moved, so the
bounce jumped to the stock 2P row and the fade showed the stock two rows.

The five-player title menu temporarily uses the accurate compositor, including
when `NewRenderer=1` is selected. The renderer setting is restored after the
menu frame. The optimized renderer previously displayed only the original two
rows. Use `--new-renderer` with the runtime check to reproduce this configuration.

O menu de cinco jogadores usa temporariamente a composição precisa, inclusive
com `NewRenderer=1`. A configuração do renderizador é restaurada após o quadro
do menu. O renderizador otimizado anteriormente mostrava apenas as duas opções
originais. Use `--new-renderer` no teste de execução para verificar essa configuração.
