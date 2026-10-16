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

### Included Black Louie sounds

The release includes `audio/MountBlackLouie.wav` and
`audio/BlackLouieDash.wav` inside the selected mod package. No separate
installation is needed. The maintainer supplied them from SB6 and explicitly
authorized publication as additional mod resources, clarifying that they are
not extractions of the original game. See the package's
[AUDIO_PROVENANCE.md](../mods/preloaded/packages/superbomberman5.louie-improvements/0.1.0/AUDIO_PROVENANCE.md)
for exact source paths, hashes and licensing limits. `SB5_BLACK_LOUIE_SFX` and
`SB5_BLACK_LOUIE_DASH_SFX` override the acquisition/dash paths if desired.
The package-root lookup works independently of the working directory.

The supported format is PCM signed 16-bit, mono/stereo, 8–192 kHz, at most
4 MiB per file. The host's PCM overlay mixer plays acquisition at the other
Louies' native sound cue (`$C4:5168`), before attachment animation finishes.
The unused native cry command for type 6 is suppressed. Dash audio starts once
per accepted Y ability, with independent voices for P1–P5. Digital silence at
the start of either WAV is skipped during registration; original files and the
waveform's attack are preserved. Missing files make only their respective
cues silent. Reset/load stops this mod's voices. Disabling restores the egg
lottery and in-memory code; no ROM file or SRAM format is changed.

### Implementation and validation

`src/louie_mod.c` registers additive interpreter pre-opcode hooks. The original
ROM padding at `$C4:5B80-$5C1F` holds authored native-call trampolines; original
bytes are saved and restored on reset. These hooks require the relevant
routines to remain interpreted. Save states append a fixed-size validated
extension after Gold Battle; older states without it start with empty effects.
Host audio voices are stopped on reset/load. Five seats share the same code;
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

Os dois sons acompanham a release em `audio/MountBlackLouie.wav` e
`audio/BlackLouieDash.wav` dentro do pacote selecionado. Não é necessário
instalá-los separadamente. O mantenedor forneceu os arquivos pelo SB6,
autorizou a distribuição e esclareceu que não são extrações do jogo original.
Origem, hashes e limites de licença constam no
[AUDIO_PROVENANCE.md](../mods/preloaded/packages/superbomberman5.louie-improvements/0.1.0/AUDIO_PROVENANCE.md).
O caminho é resolvido a partir do pacote, inclusive ao abrir pelo atalho.
`SB5_BLACK_LOUIE_SFX` e `SB5_BLACK_LOUIE_DASH_SFX` permitem substituir os
caminhos de aquisição/arrancada. Aceita WAV PCM de 16 bits, mono/estéreo,
8–192 kHz, até 4 MiB por arquivo.

O som do ovo começa no mesmo ponto nativo dos demais Louies (`$C4:5168`),
antes do fim da animação de montagem. A arrancada toca uma vez por ativação
válida de Y, com vozes independentes para P1–P5. O silêncio digital inicial
dos dois WAVs é ignorado ao carregá-los, preservando os arquivos e o ataque
da forma de onda. Se um arquivo estiver ausente, somente seu respectivo som fica silencioso.
Reset e carregamento interrompem as vozes deste mod. Os áudios são arquivos
adicionais do pacote, sem incorporação de áudio do cartucho ao código.

A implementação usa hooks aditivos do interpretador e trampolins autorais em
`$C4:5B80-$5C1F`, com restauração dos bytes originais no reset. Estados salvos
recebem uma extensão validada de tamanho fixo; estados antigos começam sem
os efeitos. Reset/carregamento interrompem a voz de áudio do mod. O mesmo
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
