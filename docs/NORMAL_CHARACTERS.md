# Normal Game character selection / Seleção de personagens

## Português

Mod opcional `superbomberman5.normal-characters` v0.1.0, incluído no setup pack v0.3.0 e nos builds
com `src/normal_character_mod.c`. Em **Mods**, ative **Choose a character in
Normal Game** e inicie o jogo. Após confirmar o arquivo, aparece a seleção
original do Battle Mode. Use o direcional e confirme com A (teclado X). B
(teclado Z) volta à seleção de arquivo. A escolha é aplicada aos sprites e à
paleta do jogador antes do carregamento das fases. A tela funciona também com
o mod de tradução PT-BR habilitado.

A seleção vale para a campanha atual; ao escolher um arquivo em outra sessão,
escolha novamente. O formato do SRAM permanece original. Save states e rewind
incluem a escolha e o estado do Normal Game suspenso durante a seleção. O mod
fica desabilitado por padrão. Ele não adiciona personagens ou recursos à ROM.

O pacote `.snesmod` contém somente o manifesto e a licença. Exige um executável
recompilado com o novo plugin; executáveis de releases anteriores não passam a
suportá-lo apenas importando o pacote. Para gerar o arquivo:

```powershell
python tools/package-normal-characters.py
```

## English

Optional `superbomberman5.normal-characters` v0.1.0 is included in the v0.3.0 setup pack and builds
that compile `src/normal_character_mod.c`. Enable **Choose a character in
Normal Game** in **Mods**, then launch. Confirming a Normal Game file opens the
original Battle character picker. Move with the D-pad, confirm with A (keyboard
X), or return to file selection with B (keyboard Z). The selected character and
palette are applied before stage graphics load. The PT-BR menu overlay can be
enabled alongside this mod.

The choice lasts for the current campaign and is selected again in a new
session. Battery SRAM retains the stock format. Save states and rewind include
the choice and the suspended Normal machine while the picker is open. The mod
is disabled by default and adds no cartridge artwork. The `.snesmod` archive
contains only a manifest and license; it requires a rebuilt host containing the
new trusted plugin. Importing it into a previously released executable cannot
install that native implementation.

## Implementation

The verified JPN revision uses script task 9, with its program counter split
across WRAM `$0C90/$0CB8/$0CE0`. The confirmed-file fade at `$C7:1257` is guarded
by the Normal mode and its file-menu return stack. The mod snapshots that
machine in memory, runs the cartridge's picker at `$C6:89CF`, and restores the
Normal snapshot after confirmation. Menu state changes cannot replace campaign
state. No ROM bytes are modified.

Battle choices at `$7E:625F` are one based. Normal's character fields at `$16C2`
are zero based and initialized before the file menu. Confirmation applies this
conversion and the original palette table at ROM `$80:BB4E`, before native
stage routines load graphics. Two-player confirmation waits for both selection
bits; broader cooperative gameplay requires runtime validation.

The title's snapshot extension includes execution position, selection state and
the suspended snapshot, with a bounded allocation. Tests cover disabled mode,
campaign restoration, character/palette mapping, both-player confirmation,
cancel, picker-state serialization and invalid snapshot lengths.

Public snapshots reserve a fixed 384 KiB for the suspended machine, keeping
their size stable for the framework's rewind ring. The private checkpoint omits
that reserve, so it does not recursively include another checkpoint.

Local runtime checks verified the original picker, the second and ninth
characters in Zone 1-1, movement/bomb input with the ninth character, and the
PT-BR overlay enabled alongside the mod. Cancel returns to file selection.
Two-player confirmation was exercised and the Normal map resumed, but full
cooperative stages and a populated Continue file have not been qualified.
The full campaign, deaths, bosses and endings remain unverified.
