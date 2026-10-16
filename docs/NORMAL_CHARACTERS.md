# Normal Game character selection / Seleção de personagens

## Português

Nos builds atuais de fonte, o mod [Normal co-op](NORMAL_COOP.md) amplia a
confirmação para 3–5 jogadores, incluindo as cinco escolhas de cor. A seleção
aguarda todos os controles escolhidos antes de retomar a campanha.

Mod opcional `superbomberman5.normal-characters` v0.1.0, incluído no setup pack v0.3.0 e nos builds
com `src/normal_character_mod.c`. Em **Mods**, ative **Choose a character in
Normal Game** e inicie o jogo. Após confirmar o arquivo, tanto em New Game
quanto em Continue, aparece a seleção original do Battle Mode. Use o direcional e confirme com A (teclado X). B
(teclado Z) volta à seleção de arquivo. Nos builds atuais de fonte, a volta
reconstrói a lista e permite navegar, reabrir o seletor ou voltar ao menu anterior. A escolha é aplicada aos sprites e à
paleta do jogador antes do carregamento das fases. A tela funciona também com
o mod de tradução PT-BR habilitado.

Após Game Over, **Continue** mantém os personagens e cores da campanha,
incluindo Honey/Kotetsu e Subordinate. Partidas de um/dois jogadores retomam
com os participantes escolhidos, sem exigir o mod de cooperativo. A escolha
também continua nas transições normais; sair para o título encerra essa sessão.

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

After Game Over, **Continue** retains campaign characters/colors, including
Honey/Kotetsu and Subordinate. One/two-player games restore the selected
roster even without co-op enabled. Normal stage transitions retain their
existing participation behavior; returning to the title ends the session.

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
uses a different stack and never opens the picker. Current source cancellation restarts the complete file-list entry: New Game
`$C7:100E`, Continue `$C7:0D59`. Both include the native screen cleanup and
archive loader. The suspended fade call is unwound from the VM byte stack
and task depth, preserving the outer menu return for reentry. Continue/New
origin is retained across snapshot loading, which resets the mod extension. The mod snapshots that
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
snapshot lengths. The former black-screen cancellation jumped into FF.8D operands at
`$C7:1014`; it could corrupt the VM or stop at `$C7:108D` with NMI busy.
Current source uses complete native entries and restores the original call
depth/stack pointer before rebuilding, so repeated cancels can reopen the
picker. No snapshot fields or SRAM layout were added.

Public snapshots reserve a fixed 384 KiB for the suspended machine, keeping
their size stable for the framework's rewind ring. The private checkpoint omits
that reserve, so it does not recursively include another checkpoint.

Local runtime checks verified the original picker, the second and ninth
characters in Zone 1-1, movement/bomb input with the ninth character, and the
PT-BR overlay enabled alongside the mod. Cancel returns to file selection.
Two-player confirmation was exercised and the Normal map resumed, but full
cooperative stages and a populated Continue file have not been qualified.
The full campaign, deaths, bosses and endings remain unverified.

Current cancellation regression: `tests/normal_character_cancel_runtime.py`
uses isolated SRAM and native menu inputs, checks repeated Back/reentry, file
navigation and return to the outer Normal menu, with co-op, colors, Subordinate,
Honey/Kotetsu and PT-BR on/off. Continue records are synthetic fixtures.
A regressão atual usa SRAM isolado, entradas nativas, B/reentrada repetidos,
navegação de arquivos e saída ao menu anterior, com os mods combinados.
Os arquivos Continue são fixtures; não representam uma campanha completa.

### Game Over continuation (2026-10-16)

The outermost character interpreter hook restores `$16C2/$16DA` from the
retained picker choices `$625F/$6264` at `$C4:28B4` and `$C2:1697`. Honey/
Kotetsu keep their WRAM skin markers. A pending restart restores the original
one/two-seat participation mask exactly once before native actor creation;
co-op continues to own larger rosters and boss benches. The pending bit uses
header state 6 in the existing constant-size extension; older states 0–3
remain accepted. No ROM patches or SRAM layout changes are added.

Regression: `python tests/normal_character_continue_runtime.py`, with optional
`--single`, `--english`, `--skins` (Honey/Subordinate, co-op disabled). See
[STATUS.md](STATUS.md) for routes actually exercised.

From v0.13.1 the Normal picker also retains the independent Super Bomberman 4 palette across snapshot restoration (all extra choices 11–19 covered by the native regression).

Desde a v0.13.1 a seleção do Normal também preserva a paleta independente do Super Bomberman 4 ao restaurar a campanha (escolhas extras 11–19 cobertas pelo teste nativo).
