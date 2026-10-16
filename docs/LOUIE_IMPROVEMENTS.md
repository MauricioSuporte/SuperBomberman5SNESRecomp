# Louie improvements / Melhorias dos Louies

## English

Optional native gameplay mod, package `superbomberman5.louie-improvements`
0.1.0, included in host v0.11.0. Complete Generate & rebuild, then enable **Yellow, Red and Black Louies**
in the launcher's Gameplay category. Disabled by default. Hosts older than v0.11.0
do not contain this plugin; importing the manifest cannot add native code.

Press Y while mounted:
- Yellow (native type 3): kicks a bomb in the facing cell with native rolling
  motion. Motion can transfer between bombs and native moving soft blocks.
  Native fuse, explosion, wall and allocation rules remain authoritative.
- Red (type 0): keeps its native player punch and additionally freezes enemies
  in the facing cell in Normal Game for 180 simulation frames (about 3 seconds).
  The native Clock helper pauses the affected enemy's AI and animation while
  retaining damage checks. Enemy hits play the native player-hit SFX (group 2,
  command `$38`) once; misses are silent. Pause does not consume the timer; Battle enemies
  are excluded.
- Black (type 6): joins the native egg lottery, now inclusive 0–6. Y starts a
  forward dash of up to three tiles over 30 simulation frames. The first player
  or Normal enemy hit is pushed up to two tiles, stopping at solid obstacles.
  The dash holds on impact and protects its rider during the ability. Native
  mount-loss/death handling and sounds are retained.

Behavior is informed by the three ability classes in the maintainer's SB6
Unity project. Sprite sheets are references only: all displayed mount art,
palettes and animations come from the player's SB5 cartridge at runtime.

### Native Black Louie sounds

Current source uses the cartridge SPC driver. Acquisition retains the original
`$C4:5168` call (group 2 / `$2E`); dash calls group 2 / `$35` once per accepted
ability. Simultaneous starts share one command in the same input poll. No WAVs,
host PCM voices or audio path overrides are used. Sound data comes from the
player's ROM at runtime. Previously published v0.11.0/v0.11.1 packages contained
external audio; this correction requires rebuilding and does not alter those releases.

### Implementation and validation

`src/louie_mod.c` registers additive interpreter pre-opcode hooks. The original
ROM padding at `$C4:5B80-$5C1F` holds authored native-call trampolines; original
bytes are saved and restored on reset. These hooks require the relevant
routines to remain interpreted. Save states append a fixed-size validated
extension after Gold Battle; older states without it start with empty effects.
Native audio follows the game audio state. Five seats share the same code;
Normal co-op's `$1CBF` determines mount and enemy slot boundaries.

Native tests cover activation/restore, P1–P5, native Clock timer borrowing,
180-frame duration, pause, Battle exclusion, bomb ownership, obstacle clipping,
input flags, saved state round trips and rejection of corrupt fields.
Windows/LLE runtime evidence and remaining limits are recorded in
[STATUS.md](STATUS.md). The deterministic egg-choice mailbox in
`tests/louie_fixture.c` is compiled only with `SB5_COOP_INPUT_FIXTURE=ON`;
normal/release builds must keep that option OFF.

## Português brasileiro

Mod opcional de gameplay nativo, pacote `superbomberman5.louie-improvements`
0.1.0, incluído no host v0.11.0. Faça Generate & rebuild e ative **Yellow, Red and Black Louies** na
categoria Gameplay do launcher. Desativado por padrão. Hosts anteriores à v0.11.0
não contêm este plugin; importar o manifesto não acrescenta código.

Pressione Y enquanto estiver montado:
- Yellow (tipo nativo 3): chuta a bomba na célula à frente com o movimento
  nativo de rolagem. O movimento pode passar entre bombas e soft blocks móveis.
  Pavios, explosões, paredes e alocação continuam sob controle do jogo.
- Red (tipo 0): mantém o soco nativo contra jogadores e congela inimigos
  na célula à frente no Normal por 180 frames de simulação (cerca de 3 segundos).
  A rotina nativa do Clock pausa IA e animação do alvo, preservando os testes
  de dano. O acerto toca uma vez o mesmo SFX nativo usado contra jogadores
  (grupo 2, comando `$38`); errar não toca esse som. A pausa não consome esse
  tempo; não afeta inimigos no Battle.
- Black (tipo 6): passa a participar do sorteio nativo dos ovos, agora de 0 a 6.
  Y inicia uma arrancada de até três tiles em 30 frames. O primeiro jogador
  ou inimigo do Normal atingido é empurrado até dois tiles, respeitando
  obstáculos sólidos. A arrancada para no impacto e protege o cavaleiro
  durante a habilidade. A perda da montaria e o som de morte continuam nativos.

As três classes de habilidades do projeto SB6 Unity orientam o comportamento.
As imagens anexadas servem de referência; gráficos, paletas e animações são
lidos da ROM de SB5 do jogador durante a execução.

O código atual usa o driver SPC do cartucho. A montagem preserva a chamada
original `$C4:5168` (grupo 2 / `$2E`); a arrancada chama o grupo 2 / `$35` uma
vez por habilidade aceita. Ativações simultaneas compartilham um comando na
mesma leitura de entrada. Não usa WAVs, vozes PCM ou variáveis de caminho de
áudio. Os dados dos sons vêm da ROM do jogador durante a execução. Os pacotes
publicados v0.11.0/v0.11.1 continham áudio externo; esta correção exige recompilar
o host e não modifica essas releases.

A implementação usa hooks aditivos do interpretador e trampolins autorais em
`$C4:5B80-$5C1F`, com restauração dos bytes originais no reset. Estados salvos
recebem uma extensão validada de tamanho fixo; estados antigos começam sem
os efeitos. O áudio nativo acompanha o estado de áudio do jogo. O mesmo
código atende P1–P5, usando `$1CBF` para separar jogadores, montarias e inimigos.
Desativar restaura o sorteio e os patches em memória, sem alterar ROM ou SRAM.

Os testes nativos cobrem ativação/restauração, P1–P5, empréstimo temporário do
contador do Clock, duração de 180 frames, pausa, exclusão do Battle, bombas,
colisões, flags de entrada e estados salvos válidos/corrompidos. Evidências
Windows/LLE e limites estão em [STATUS.md](STATUS.md). A escolha determinística
do ovo em `tests/louie_fixture.c` existe apenas com
`SB5_COOP_INPUT_FIXTURE=ON`; builds normais/de release devem deixá-la OFF.


Run the local fixtures / Executar as fixtures locais:

```powershell
.tools/toolchain/bin/cmake.exe -S . -B build-lle -DSB5_COOP_INPUT_FIXTURE=ON
./tools/build-windows.ps1
python tests/louie_runtime.py --exe build-lle/SuperBomberman5SNESRecomp.exe --rom 'Super Bomberman 5 (USA).sfc' --case red
# Other cases / Outros cenários: yellow, black, chain, five
.tools/toolchain/bin/cmake.exe -S . -B build-lle -DSB5_COOP_INPUT_FIXTURE=OFF
./tools/build-windows.ps1
```
