# Normal HUD character faces / Rostos dos personagens na HUD do Normal

## Português

No launcher, em **Mods → Gameplay**, ative **Battle character faces in Normal HUD**.
O mod opcional `superbomberman5.hud-faces` 0.1.0 exige um host recompilado com
host v0.13.1 ou superior; importar apenas o pacote não atualiza executáveis antigos.

P1/P2 usam o rosto do personagem atual e a paleta escolhida, nas posições dos
ícones fixos branco/preto. A derrota usa a animação nativa do Battle. Coração,
vidas, tempo e pontos continuam sendo os do Normal; não há placar de vitórias.
O aviso de retorno continua piscando, ajustado ao espaço ao lado do rosto.
Com três a cinco jogadores, somente P1/P2 têm retratos; P3–P5 não
criam nem atualizam rostos na HUD. Sem participante P2, seu retrato não é
criado. O retorno dos jogadores extras não altera os dois retratos. Combine com seleção de personagem
e cores para escolher outros bombers; Subordinate e os personagens do Extra Characters/Bomber Kings reutilizam
seus overlays existentes. O mod começa desativado.

Nas fases de chefe (ZONE1 - A a ZONE5 - J e as três fases secretas, `$67`-`$73`)
a HUD fica com os ícones originais: o gráfico do rosto ocupava a VRAM de que os
portais de saída precisam e, no ZONE3 - D (pirata e parceiro), os portais não
apareciam depois de vencer o chefe.

Algumas fases comuns batem no mesmo limite: na ZONE3 - 1 o portal não aparecia
depois de derrotar todos os inimigos. Quando o jogo não consegue carregar um
gráfico (como o do portal) por falta de VRAM enquanto os rostos estão na tela,
os rostos são removidos pelo próprio jogo e a HUD volta aos ícones originais
até a próxima fase; o portal então aparece normalmente. Nas fases em que o
portal cabe (como a ZONE1 - 1) os rostos continuam. Na ZONE5 - 3 o gráfico do
portal é carregado uma única vez, no início da fase, e o jogo não tenta de novo:
nesse caso os rostos são removidos na hora e o carregamento é refeito, então o
portal aparece.

As rotinas nativas criam somente os objetos dos rostos, sem carregar o layout
do Battle. O desenho limpa o rosto fixo nos seis tiles BG2 de cada ícone ocupado,
preservando as linhas da moldura e o fundo azul,
restaurando VRAM ao fim do quadro. As imagens e paletas vêm da ROM em memória.
Patches são reversíveis e não alteram a ROM em disco nem o formato da SRAM.
Não há extensão de save state: os objetos e sua animação já pertencem à WRAM.
Os pontos de integração precisam continuar interpretados, não AOT.

## English

Enable **Battle character faces in Normal HUD** under **Mods → Gameplay**.
The optional `superbomberman5.hud-faces` 0.1.0 package requires a host rebuilt
with this source; importing the manifest cannot update an older executable.

P1/P2 show their current character and selected color at the original fixed
white/black icon positions, using Battle's native defeated animation. Normal
lives, heart, timer and points remain; no Battle wins counter is added.
The blinking rejoin prompt is fitted beside the face using its existing pixels.
With three to five players only P1/P2 own HUD faces. Extra seats cannot
create, update or repaint these portraits; without a P2 participant, no P2
portrait is created. Character/color selection and the existing
Subordinate, Honey and Kotetsu overlays can be combined. Disabled by default.

Boss stages (ZONE1 - A to ZONE5 - J and the three secret stages, `$67`-`$73`)
keep the stock HUD icons: the face's graphics took the OBJ VRAM the exit gates
need, and in ZONE3 - D (Pirate Bomber and partner) the gates never appeared
after the boss was defeated.

Some regular stages hit the same limit: in ZONE3 - 1 the exit gate did not
appear after every enemy was defeated. When the game cannot load a graphic
(such as the gate's) for lack of VRAM while faces are shown, the faces are
deleted through the game's own object routine and the HUD returns to the stock
icons until the next stage; the gate then appears. Stages whose gate fits (such
as ZONE1 - 1) keep their faces. ZONE5 - 3 loads its gate archive (`$3A`) once,
at stage start, and never retries; on a failure with a live face the
allocator's failure exit (`$C0:193A/$193C`) now runs `$C5:7360`, which deletes
the faces like `$C5:7300` and re-enters `$C0:18FC` with the same archive and
registers, so the gate is loaded (static archive table identical to a run
without the mod).

Native routines create only the face objects. Render observers clear each
occupied fixed BG2 head across all three tile rows, preserving the blue
background and border scanlines and restore VRAM after drawing. All cartridge art is
read at runtime; reversible memory patches leave the ROM file and SRAM layout
untouched. Face objects and animation are already serialized as guest WRAM.
The patched routines must remain interpreted.

## Validation / Validação

Reproduce the combined death/rejoin route with
`python tests/normal_hud_runtime.py` (add `--english` for PT-BR off).
See the dated entry in [STATUS.md](STATUS.md). Unit tests cover activation,
Normal/Battle guards, patch restoration, refusal of occupied trampoline space,
and exact VRAM restoration. Full campaign and Linux runtime are not qualified.

### Co-op HUD regression (2026-10-16)

Normal face maintenance now runs from the native HUD task, allowing late
actors/restored scenes to acquire P1/P2 faces without waiting for a life
change. Unexpected living-face animations are repaired through the native
character table. Battle reentry's face updater is skipped for extra seats
in Normal. Honey/Kotetsu and Subordinate render only P1/P2 HUD portraits in
campaign stages; Battle retains all five.

`tests/normal_hud_coop_runtime.py --exe <fixture-host> --players 3|4|5`
uses the test-only pad mailbox, NewRenderer=1 and character/color/HUD/co-op,
Honey/Subordinate, Gold, pause, Louie and infinite-time mods. It models a
stale P3 pointer and wrong P1 animation, tests P3 reentry plus P2 death/reentry,
and checks one OAM face at each P1/P2 position. `--players 1` works with the
normal fixture-free executable and checks that P2 has no portrait. Captures
stay ignored. See STATUS.md for counts/languages actually exercised.
