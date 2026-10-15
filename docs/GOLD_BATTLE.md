# Gold Cartridge — Battle

## Português

Recompile as fontes atuais e ative **Mods → Gameplay → Gold Cartridge — Battle →
Arenas 11–13 and Hyper Revenge**. O mod `superbomberman5.gold-battle` 0.1.0 é
opcional e desativado por padrão. O pacote contém apenas manifesto e licença;
não adiciona código a um host antigo. Incluído na release v0.10.0; a v0.9.0 não contém este plugin. O executável local em `build-lle/` precisa ser recompilado.

Na seleção de arenas do Battle, esquerda/direita passam a alcançar 11, 12 e 13.
Nas regras, a linha Revanche oferece Não / Sim / Super / Hiper. Hiper inclui o
retorno do Super: eliminar um adversário enquanto está no carrinho permite
voltar ao campo com as bombas, alcance, velocidade e habilidades do eliminado.
Personagem, paleta e montaria não são transferidos. O adversário não espalha
os equipamentos transferidos. As opções acompanham a tradução PT-BR; sem a
tradução, o novo valor é HYPER. As arenas extras aparecem como Fase 11/12/13: Ciclos estranhos,
Esteiras traiçoeiras e Trilhos misteriosos. O cabeçalho também acompanha o PT-BR.

A análise das duas ROMs fornecidas confirmou scripts idênticos das arenas
11–13 na ROM comum: não é necessário carregar a Gold. Nenhuma ROM, patch de
dados extraídos ou recurso do jogo é incluído no mod. A comparação local da
Gold orientou a lógica nativa autoral do Hiper. Os cartuchos no disco e o
layout do SRAM permanecem intactos; o desbloqueio usa apenas WRAM e patches
em memória, restaurados ao desativar/resetar.

### Implementação e evidência — 2026-10-15

O contador volátil `$6245` permite treze arenas; o caminho de boot em
`$C0:48C3` normalmente escolhe dez. As regras preservam os bits externos à
máscara `$58` de `$625D`. Hiper usa `$50`, compartilhando a lógica nativa de
sobrevivência/retorno do Super (`$10`) e reservando `$40` como marcador. Dois
programas VM autorais em padding `$C7:3000-$30FF` substituem os caminhos de
esquerda/direita. A VM rejeita scripts abaixo do banco C6 (`$C2:0254`), por
isso esse padding é utilizado. As assinaturas e todo o espaço são conferidos
antes da instalação; nada é instalado parcialmente.

O hook de instrução encadeado observa o registro de quem eliminou a vítima,
a captura de equipamentos antes da morte, a decisão nativa de soltar itens e
o término do spawn do retorno. Cinco registros de vítima/equipamentos ficam
no host, sem sobrepor os campos do cartucho. Extensão de save state fixa de
35 bytes, magic e validação, acrescentada depois da pausa; estados antigos
sem a extensão continuam aceitos. Estas rotinas precisam ficar interpretadas.

Build Windows/LLE e dez testes nativos passaram. O teste novo cobre as quatro
opções, ambos os sentidos, preservação de outras regras, os vinte pares de
jogadores, restauração dos patches/hooks, estados antigos e dados inválidos.
Rotas Windows/LLE com SRAM isolado: ciclo Não/Sim/Super/Hiper com PT-BR nas
arenas 11, 12 e 13 (7.016 / 7.039 / 7.062 quadros), mais a arena 1 sem PT-BR
(6.786 quadros). As arenas extras foram observadas até 900 quadros após a
confirmação, sem BRK, interpreter cap ou fatal. Um fixture de contato percorre
a morte/carrinho/eliminação/retorno nativos de P1/P2 e verifica os cinco campos
de equipamento, comparado ao Super sem o mod. Com PT-BR e Pause ativos,
Empatar rodada após o retorno produziu o empate e reiniciou a rodada.
Capturas brutas ficam em
`.bootstrap/`. Não qualifica todas as variantes de arena, equipe/Super Battle,
controles físicos, partidas longas ou rewind durante a transferência.

```powershell
python tests/gold_battle_runtime.py --exe build-lle/SuperBomberman5SNESRecomp.exe --rom 'Super Bomberman 5 (USA).sfc'
python tests/gold_battle_runtime.py --exe build-lle/SuperBomberman5SNESRecomp.exe --rom 'Super Bomberman 5 (USA).sfc' --revenge-only
```

## English

Use release v0.10.0 (Generate & rebuild) or build current sources and enable **Gold Cartridge — Battle → Arenas 11–13
and Hyper Revenge** under **Mods → Gameplay**. Optional, default-disabled
`superbomberman5.gold-battle` 0.1.0 needs this native plugin; its data-only
manifest cannot update an older host. Included in v0.10.0; v0.9.0 setup packs lack this native plugin.

Left/right in Battle arena selection now reach arenas 11–13. Revenge rules
offer Off / On / Super / Hyper. Hyper returns a cart bomber who defeats an
opponent with that opponent's bomb capacity, fire range, speed and abilities;
character, color and mount are retained. Transferred equipment is not also
dropped. PT-BR displays HIPER and translates the extra arena numbers/names to Fase
11/12/13, CICLOS ESTRANHOS, ESTEIRAS TRAIÇOEIRAS and TRILHOS MISTERIOSOS.
The stage-selection heading follows PT-BR on every arena.

Both supplied cartridges have identical scripts for the three arenas, so the
regular ROM supplies all arena resources at runtime. Authored native code
implements the additional Hyper behavior researched locally from Gold. No
extracted assets or cartridge bytes are packaged; ROM files and SRAM layout
are unchanged. All patches are verified, reserved and restored. VM programs
occupy verified padding `$C7:3000-$30FF`; the VM rejects banks below C6. Hyper
uses bits `$50` of `$625D` to reuse Super's native reentry/survivor flow. A
validated fixed-size state extension follows the pause extension and accepts
older states without it. Instruction-hook routines must remain interpreted.

Windows LLE build and ten native tests passed, including all twenty ordered
seat pairs. Isolated runs exercised all three arenas with PT-BR, a full rule
cycle, arena 1 without PT-BR, and controlled native damage/cart/reentry with
equipment transfer versus stock Super. See the Portuguese evidence above for
frames and commands. Full arena variants, team/Super Battle, physical controls,
long matches and rewind during transfer remain unqualified.

Arena typography correction / Correção da fonte (2026-10-15): PT-BR now
translates all 13 names using the native OBJ font from live VRAM, with authored
accents. The original ten also work without Gold Battle. Transition checks
covered 109 frames with Gold and 82 without it, without source-language flashes.
PT-BR agora usa a fonte OBJ nativa nos 13 nomes, com acentos autorais; as dez
arenas originais também funcionam sem Gold Battle. Foram verificados 109
quadros com Gold e 82 sem Gold, sem flashes do idioma original nas amostras.
