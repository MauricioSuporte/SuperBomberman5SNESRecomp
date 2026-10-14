# Normal Game character selection / Seleção de personagens

## Português

Nos builds atuais de fonte, o mod [Normal co-op](NORMAL_COOP.md) amplia a
confirmação para 3–5 jogadores, incluindo as cinco escolhas de cor. A seleção
aguarda todos os controles escolhidos antes de retomar a campanha.

Mod opcional `superbomberman5.normal-characters` v0.1.0, incluído no setup pack v0.3.0 e nos builds
com `src/normal_character_mod.c`. Em **Mods**, ative **Choose a character in
Normal Game** e inicie o jogo. Após confirmar o arquivo, tanto em New Game
quanto em Continue, aparece a seleção original do Battle Mode. Use o direcional e confirme com A (teclado X). B
(teclado Z) volta à seleção de arquivo (no ambiente de testes sem janela essa
volta ainda termina em tela preta; veja abaixo). A escolha é aplicada aos sprites e à
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

Current source builds extend confirmation to 3–5 players when the
[Normal co-op mod](NORMAL_COOP.md) is enabled, including all five color choices.
The campaign waits for every selected seat to confirm.

Optional `superbomberman5.normal-characters` v0.1.0 is included in the v0.3.0 setup pack and builds
that compile `src/normal_character_mod.c`. Enable **Choose a character in
Normal Game** in **Mods**, then launch. Confirming a Normal Game file, from
New Game or Continue, opens the original Battle character picker. Move with the D-pad, confirm with A (keyboard
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
by the Normal mode and its file-menu return stack at `$2003-$2008`: New Game
`$C7:0BFF/$C7:10E6` or Continue `$C7:0BEA/$C7:0E2B`. Back from the file list
uses a different stack and never opens the picker. Cancel from Continue resumes
`$C7:0D39` (file list build and fade-in) with its own return stack. The mod snapshots that
machine in memory, runs the cartridge's picker at `$C6:89CF`, and restores the
Normal snapshot after confirmation. Menu state changes cannot replace campaign
state. No ROM bytes are modified.

Battle choices at `$7E:625F` are one based. Normal's character fields at `$16C2`
are zero based and initialized before the file menu. Confirmation applies this
conversion and the original palette table at ROM `$80:BB4E`, before native
stage routines load graphics. Two-player confirmation waits until the native
confirmed-seat count reaches two. Confirmation is gated on picker readiness
because initialization temporarily reuses that counter. Local one- and two-player
routes verified stage characters and palettes; broader cooperative gameplay
requires runtime validation.

The title's snapshot extension includes execution position, selection state and
the suspended snapshot, with a bounded allocation. Tests cover disabled mode,
campaign restoration, character/palette mapping, both-player confirmation,
cancel, Continue confirmation/cancel, picker-state serialization and invalid
snapshot lengths. In headless LLE runs, Back from the picker leaves the screen
black for both New Game and Continue (also with the earlier New Game-only
version: the script stops at `$C7:108D` with the NMI latch `$67` set); this
remains unresolved.

Public snapshots reserve a fixed 384 KiB for the suspended machine, keeping
their size stable for the framework's rewind ring. The private checkpoint omits
that reserve, so it does not recursively include another checkpoint.

Local runtime checks verified the original picker, the second and ninth
characters in Zone 1-1, movement/bomb input with the ninth character, and the
PT-BR overlay enabled alongside the mod. Cancel returns to file selection.
Two-player confirmation was exercised and the Normal map resumed, but full
cooperative stages and a populated Continue file have not been qualified.
The full campaign, deaths, bosses and endings remain unverified.
