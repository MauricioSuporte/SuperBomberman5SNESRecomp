# Normal Game — portal destinations / Destinos dos portais

## Português

Compile as fontes atuais e ative **Mods → Gameplay → Normal Game — portal
destinations → Show portal destinations**. O pacote `superbomberman5.portal-labels`
0.1.0 vem desativado. Incluído nos pacotes v0.8.0; exige host v0.8.0 ou superior.
O executável v0.7.0 não contém este plugin. Importar o manifesto
não acrescenta código nativo a um host antigo.

A dica aparece centralizada abaixo de cada portal **somente quando ele está
visível**, acompanhando também os quadros invisíveis da animação original.
Não revela a posição de uma saída ainda escondida sob um bloco. Na ZONE1-1,
os rótulos são esquerda `Z1-2`, centro `Z1-5` e direita `Z1-7`. As fases da
Zone 5 usam numeração contínua de 1 a 30, incluindo as duas partes dos mapas.

Fases especiais usam `Z1-S`, `Z2-S`, `Z3-S`. Chefes mantêm as letras dos mapas:
`Z1-A1`/`Z1-A2`/`Z1-A3`, `Z2-B1`/`Z2-B2`, `Z2-C`, `Z3-D`,
`Z3-E1`/`Z3-E2`, `Z4-F`/`Z4-G`/`Z4-H`, `Z5-I`, `Z5-J1`/`Z5-J2`.
Os sufixos vêm das flags de rota que o próprio portal aplica antes da saída.

É um overlay de apresentação, independente do mod de pausa. Não modifica
WRAM, ROM, SRAM, destinos, cores dos portais, combate ou regras de conclusão.
Some durante a pausa e fora das fases do Normal. Não precisa de extensão
de save state: não guarda estado de campanha ou listas de portais entre quadros.

### Implementação

Os mapas fornecidos são referência; as ligações efetivas são lidas da ROM
do usuário em execução. `$C6:0000` contém os ponteiros das listas por fase
(índice fase−1). As entradas de portal têm handlers nativos 13/11/12:
criação, saída e teste de contato. O comando `$11` fornece coordenadas; `$0F`
fornece o destino, depois de eventuais comandos `$05/$06` de flags.
O parser tem limites de banco, quantidade, coordenadas e formato; entradas
incompatíveis não geram dicas.

As fases de chefe guardam as listas no banco `$C7` e criam os portais só
para a rota usada para chegar lá: antes de `$11`/`$12` vêm condições nativas
`$08 f` (flag de rota f em `$7E2900`), `$83` (inverte), `$5B` (valor) e
`$49 n`/`$47 n` (pula n bytes se falso/verdadeiro). O mod segue essas
condições com as flags atuais, então os rótulos aparecem nos portais que o
chefe abre ao ser derrotado (por exemplo, ZONE1-A pela rota A1: `Z2-1` e
`Z2-5`; ZONE4-G: `Z5-1`, `Z5-2`, `Z5-3`).

Cada dica exige o objeto nativo correspondente, frame não nulo em `$049F`,
slot de paleta válido e sprite da mesma paleta na posição do portal no OAM
da composição. Usa os deslocamentos de câmera `$1D3B/$1D3D`, como `$C0:1F78`.
Assim, a fila de criação ou a existência de um destino na tabela não bastam
para revelar uma dica. Os três portais são verificados separadamente.

As letras e números reutilizam a fonte autoral da tela de título PT-BR,
compartilhada em `src/menu_font.h`, incluindo todos os chefes e variantes.
Os traços não são compactados: as dicas têm 14 pixels de altura, com contorno
escuro; na última linha da fase, usam 10 pixels para caber abaixo do portal.
Essa é a fonte mostrada na referência em português, não um recorte de pixels
do cartucho. Funciona também com a tradução desativada.
Tiles BG3 livres são emprestados antes das páginas OBJ; VRAM e registros de
tela são restaurados na linha 224. A ordem de restauração inverte a de desenho,
em conjunto com cores, Subordinate e pausa. CGRAM e OAM não são alterados.

### Validação reproduzível

```powershell
./tools/build-windows.ps1
./build-lle/portal_labels_tests.exe 'Super Bomberman 5 (USA).sfc'
python tests/portal_labels_runtime.py --exe build-lle/SuperBomberman5SNESRecomp.exe --rom 'Super Bomberman 5 (USA).sfc'
python tools/package-portal-labels.py
```

O teste nativo cobre parsing, limites, variantes, contexto, visibilidade
individual, esgotamento de tiles e restauração. Com a ROM local, audita os
192 portais das 90 fases numeradas e três especiais; isso verifica dados,
não uma campanha jogada. A rota Windows/LLE usa SRAM novo e conclusão de
inimigos/remoção de blocos forçadas numa fixture isolada; compara o estado
nativo com o mod desligado. Capturas e dados brutos ficam em `.bootstrap/`.
Veja [STATUS.md](STATUS.md) para resultados e limites. Campanha completa,
combate natural até cada portal, todos os chefes, controles físicos e Linux
não são qualificados por essa rota.

## English

Build current sources and enable **Mods → Gameplay → Normal Game — portal
destinations → Show portal destinations**. Optional package
`superbomberman5.portal-labels` 0.1.0 is disabled by default. Included in v0.8.0 setup packs; requires host v0.8.0 or newer.
The published v0.7.0 host does not contain its native plugin. A data-only manifest cannot update an older host.

Each hint is centered below its own **visible** native portal, following
the original animation's hidden frames too. Covered exits stay undisclosed.
ZONE1-1 reads `Z1-2` left, `Z1-5` center, `Z1-7` right. Zone 5 is numbered
continuously from 1 to 30. Special stages use `Z1-S`/`Z2-S`/`Z3-S`;
bosses retain map letters and native route variants, including
`Z1-A1`/`Z1-A2`/`Z1-A3`, B1/B2, E1/E2 and J1/J2.

The presentation plugin is independent of pause. It changes no WRAM, ROM,
SRAM, routes, gate colors or completion rules, and hides during pause and
outside Normal stages. It retains no campaign state and needs no save-state
extension. Native script pointers at `$C6:0000` supply gate coordinates,
destination commands and route flags; a bounded, format-checked parser skips
unknown entries. Every hint also requires its native gate object, nonzero
animation frame, palette and corresponding OAM sprite at the camera-adjusted
position. A queued gate alone cannot reveal a hint.

Letters and digits share the authored PT-BR title-screen font in
`src/menu_font.h`, including every boss letter and variant. Strokes are not
compressed: hints are 14 pixels high, or 10 on the bottom stage row so they
fit below the gate. This is the font in the supplied Portuguese reference;
no cartridge pixels are embedded. It also works with translation disabled.
White text with a dark outline borrows unused BG3 tiles before OBJ memory; VRAM and screen
registers are restored at line 224, in reverse overlay order. OAM/CGRAM are
untouched.

Boss stages keep their lists in bank `$C7` and create only the exits of
the route taken: native conditions `$08 f` (route flag f at `$7E2900`),
`$83` (negate), `$5B` (value) and `$49 n`/`$47 n` (skip n bytes when
false/true) precede `$11`/`$12`. The mod follows them with the live flags,
so the gates a defeated boss opens are labelled (ZONE1-A on route A1:
`Z2-1`/`Z2-5`; ZONE4-G: `Z5-1`/`Z5-2`/`Z5-3`).

Run the commands above. Native tests cover context/visibility, bounded
parsing, variants, tile exhaustion and full restoration. The optional local
ROM audit checks 192 exits across 90 numbered and three special stages;
this is a script-data audit, not a full playthrough. The isolated Windows/LLE
fixture forces enemy completion and removes covering blocks, comparing
native state against a disabled-mod run. See [STATUS.md](STATUS.md) for
results and limitations. Full campaign, natural combat through every exit,
all bosses, physical controllers and Linux remain unqualified.
