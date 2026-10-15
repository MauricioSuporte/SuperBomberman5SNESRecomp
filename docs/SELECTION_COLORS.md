# Character selection colors / Cores na seleção de personagens

## Português

Em **Mods**, ative **Choose character colors with L and R** e inicie o jogo.
Na seleção de personagens, R avança na sequência **branco, preto, vermelho,
azul, verde e amarelo**; L percorre o sentido contrário. A sequência dá a volta
nas extremidades. Todos os nove personagens jogáveis usam as seis paletas.
P1 começa em branco, P2 em preto, P3 em vermelho, P4 em azul e P5 em verde.
A cor inicial acompanha o assento atual, inclusive na passagem do controle para CPUs.
Como no seletor original, humanos usam seus próprios controles e, após
confirmar, passam a escolher os CPUs pendentes. L/R acompanha o cursor
associado a cada controle e preserva as escolhas já confirmadas.
O cursor mostra **L + R** acima e o número do jogador abaixo, com as cores originais.
L/R são os botões do controle SNES. No teclado padrão, use **C para L** e
**V para R**. No controle padrão, use **LB/RB (L1/R1)**. Confira o mapeamento
no launcher se você personalizou os controles.

Funciona no Battle Mode e, com **Choose a character in Normal Game** também
ativado, na seleção após escolher o save do Normal Game. A cor confirmada é
usada durante as fases. A tradução PT-BR pode permanecer ativada.
O mod fica desativado por padrão; save states e rewind preservam sua seleção.

Este recurso está incluído no pacote v0.4.0. Releases anteriores não contêm este plugin. Importar o pacote em um executável antigo
não instala a implementação nativa: é necessário recompilar o host atualizado.
O pacote contém apenas manifesto e licença e pode ser gerado com:

```powershell
python tools/package-selection-colors.py
```

## English

Enable **Choose character colors with L and R** in **Mods**, then launch.
During character selection, R cycles **white, black, red, blue, green, yellow**;
L cycles backward. Both directions wrap. All nine playable characters support
the six palettes. P1 starts white, P2 black, P3 red, P4 blue and P5 green.
Defaults follow the current seat, including controller handoff to CPUs. As in the original picker, humans use their own controllers
and take over pending CPU selections after confirming. L/R follows the cursor
assigned to each controller and preserves confirmed choices.
The cursor displays **L + R** above and the player number
below, retaining the original cursor colors. L/R refer to the SNES buttons:
the default keyboard bindings are **C for L** and **V for R**, and the default
gamepad bindings are **LB/RB (L1/R1)**. Check the launcher for custom bindings.

Battle Mode is supported. Enable **Choose a character in Normal Game** as well
to use these controls after choosing a Normal Game save. The confirmed color
is used in stages. PT-BR translation is compatible. The mod is disabled by
default; save states and rewind preserve the selection.

This feature is included in the v0.4.0 setup pack; previous releases do not contain its native plugin. Importing the manifest-only `.snesmod`
archive into an older executable does not install native code.

## Validation

Native tests cover all nine characters and six palettes, cycling and wrapping,
independent player state, sequential human/CPU selection, simultaneous R+A,
snapshot compatibility, and restoration of cursor OAM/VRAM after rendering.
Tests also cover native controller-to-cursor handoff and preserving CPU colors
through final picker cleanup until the stage initializer consumes them.
Same-character previews use independent native palette slots, and pending CPU
previews cannot overwrite the active grid palette.
Local runtime routes verified the six-color preview with PT-BR enabled and
the second character in green in actual Normal and Battle stages. Five-seat
input routing is covered by native tests; a two-human/three-CPU Battle route
verified separate controller inputs, CPU handoff and all five chosen stage palettes.
One- and two-player Normal routes also verified picker opening, confirmation
and chosen palettes in Zone 1-1.
Five-seat
cursor rendering is covered with synthetic font data; full multiplayer
gameplay remains unvalidated. No ROM graphics or palette bytes are packaged.

## Shared grid palette correction / Correção da paleta compartilhada

Upper-row BG strips begin at index 8 within a shared 16-color palette.
Their first entry is also used by the figure below. Color changes preserve
that entry and update only the remaining seven entries; OBJ portraits retain
all nine cartridge colors. This prevents white (and other variants) from
recoloring the lower figure. No save-state format change.

As faixas da primeira fila começam no índice 8 de uma paleta BG compartilhada.
A troca de cor preserva essa entrada, também usada pelo personagem abaixo,
e atualiza as outras sete; os retratos OBJ mantêm as nove cores do cartucho.
O formato de save states permanece igual.

2026-10-15: Windows/LLE isolated fresh-SRAM Normal and Battle routes with PT-BR,
all six colors across the five stock columns, initial seat colors 00/01/02/03/04,
and shared-entry CGRAM checks passed. Evidence is local under
`.bootstrap/selection-color-fix/`. Physical five-controller play and a full
campaign were not exercised by this check.

Repeat the isolated regression with:

```powershell
python tests/selection_color_runtime.py --exe build-lle/SuperBomberman5SNESRecomp.exe --rom 'Super Bomberman 5 (USA).sfc'
```
