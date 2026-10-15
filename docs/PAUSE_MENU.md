# Pause — exit & draw / Pausa — sair e empatar

## Português

Nas fontes atuais, compile o host e ative **Mods → Gameplay → Pause — exit & draw →
Stage exits and Battle Draw**. O mod `superbomberman5.pause-menu` 0.1.0 vem desativado.
O pacote `.snesmod` contém somente manifesto e licença; exige um executável
v0.7.0 ou superior com este plugin; importar o pacote não atualiza hosts antigos.
Incluído nos pacotes de instalação v0.7.0, desativado por padrão.

Start abre a pausa original durante uma fase. Cima/baixo movimentam a mão
apontando para a direita; Start confirma a opção selecionada e B volta.
No menu principal, B retoma a partida; A não confirma opções. Manter um botão pressionado não repete a ação.

- **Normal Game:** Resume e Return to world map. O cursor abre em Resume.
  Retornar abre Are you sure, com **No** selecionado. Start em No ou B volta
  ao menu; Start em Yes confirma a saída.
- **Battle:** Resume, Force Draw e Return to stage select. Force Draw usa o
  resultado de empate original, sem vencedor, e a rodada reinicia normalmente.
  Retornar às arenas abre a mesma confirmação, inicialmente em No, e usa o
  sinal nativo que leva à seleção de arenas após o fim de uma disputa.

O retorno ao mapa preserva personagens, paletas, equipamentos e vidas
compartilhadas. Não concede uma fase concluída. O salvamento nativo continua
ativo e pode normalizar contador/checksum de um arquivo novo; o layout do SRAM
é o original. Seats guardados pelo co-op durante chefes são recuperados antes
da saída. O menu novo está em inglês, inclusive com a tradução PT-BR ativa.
Os efeitos de mover, confirmar e voltar são os comandos 1, 4 e 3 do grupo 2
usados na configuração do Battle, executados pelo código/APU do cartucho.

### Implementação

A pausa e seu congelamento continuam nativos (`$1D1D`, tarefa em
`$C5:009E`). O teste de Start em `$C5:00B7` passa por um pequeno trampoline
próprio em padding `$C5:7180-$719A`; ele toca o SFX pendente preservando
A/X/Y/P e retorna ao teste original. Em `$C5:00C8`, o hook permite retomar
apenas após uma ação explícita. O beep padrão de despausa em `$C5:0096` é
suprimido para não substituir o som escolhido. Bytes originais são guardados,
assinaturas/padding são conferidos e tudo é restaurado no reset/desativação.
As rotinas precisam continuar interpretadas; os hooks encadeiam o anterior.

As saídas aguardam o envio do SFX antes de mudar a cena. Normal limpa somente
o bit `$40` de `$1D1A`, que autoriza registrar a fase concluída em
`$C5:3D19`, e solicita o encerramento nativo por `$1D1E` bit 2. O ator do menu
Normal entra diretamente no estado 4 (mapa) pelo store da VM em `$C2:1AEC`,
restrito aos comandos de inicialização `$C7:0B8B/$0B97`. A reinicialização de
jogadores em `$C4:28CE` restaura os campos de campanha guardados antes da
saída. Battle retorna por `$35B2=1`, modo 4, como `$C0:3D8E`. Draw marca os
cinco status `$15A2+i` como mortos e retira o bit de sobrevivente Revenge
`$1BA3+i & 2`; `$C5:0307` resolve zero sobreviventes pelo fluxo original.

Texto, mão e moldura são bitmaps autorais. Um painel compacto mantém a fase
visível e preserva o título PAUSE original, com as opções logo abaixo. O
overlay BG3 usa tiles livres antes das páginas OBJ, sem alterar CGRAM. VRAM e
registros de tela são restaurados após o desenho. O fundo mantém a fase
escurecida por uma janela de color math, excluindo BG3: PAUSE, ZONE, opções e mão ficam totalmente opacos. Quando a cena já
usa color math ou janelas de camada, o painel usa preenchimento próprio.
Não incorpora gráficos extraídos. O estado do menu, bordas de entrada, ação pendente e preservação da campanha têm uma
extensão de save state de tamanho constante, com magic e validação. Estados
anteriores sem a extensão continuam aceitos. A extensão vem depois do co-op.

### Validação

Veja a seção datada em [STATUS.md](STATUS.md). O teste reproduzível é:

```powershell
python tests/pause_menu_runtime.py --exe build-lle/SuperBomberman5SNESRecomp.exe --rom 'Super Bomberman 5 (USA).sfc' --new-renderer
```

A rota usa diretórios isolados e SRAM novo em `.bootstrap/`, verifica mapa,
empate, arenas, reentrada, flags de conclusão, personagens/vidas e os comandos
APU dos três sons. Não equivale a validar campanha inteira, todas as arenas,
chefes, Super/Tag Battle, controles físicos ou rewind durante a transição.
Save/load do menu e de sua preservação são cobertos por testes nativos.

## English

Build current sources and enable **Mods → Gameplay → Pause — exit & draw → Stage exits and Battle
Draw**. Optional `superbomberman5.pause-menu` 0.1.0 is disabled by default.
Its data-only `.snesmod` requires host v0.7.0 or newer with this native plugin;
importing it cannot update older hosts. Included in the v0.7.0 setup packages, disabled by default.

Start opens the native in-stage pause. Up/down move the authored right-pointing
glove, Start accepts the selected option and B goes back. B on the main menu
resumes play; A does not accept options.
Held buttons never repeat an action. The new labels remain English with PT-BR.

- **Normal:** Resume / Return to world map, initially Resume. Return opens
  Are you sure / No / Yes, initially **No**. Start on No or B cancels; Start
  on Yes leaves the stage.
- **Battle:** Resume / Force Draw / Return to stage select. Draw uses the
  original zero-survivor result and round restart. Returning to arenas uses
  the same native return signal as the end-of-match flow, with the confirmation
  initially on No.

Map return keeps campaign characters, colors, equipment and shared lives,
including co-op seats benched for bosses, without awarding a cleared stage.
Native saving may normalize a new file's count/checksum; SRAM layout is
unchanged. Move/accept/back are native Battle setup SFX commands 1/4/3,
executed by the cartridge's audio routine and APU before a scene transition.

The native pause task still freezes gameplay. An authored trampoline at
`$C5:7180-$719A` wraps `$C5:00B7`; the chained instruction hook handles
`$C5:00C8`, native Normal map entry (`$C2:1AEC`, initialization commands only),
and campaign-field restoration (`$C4:28CE`). Normal clears the completion
permission bit `$1D1A & $40` before native teardown. Battle arena return uses
`$35B2=1`, mode 4; Draw clears Revenge survivor flags as well as marking all
five statuses dead. These routines must remain interpreted. All in-memory
ROM patches are signature-checked, reserved and restored on reset/disable.

A compact authored BG3 panel keeps the field and native PAUSE heading visible,
with options below the heading. It borrows unused glyph tiles before live OBJ
pages, leaves CGRAM unchanged and restores VRAM/screen registers after drawing.
A color-math window dims the field behind the transparent background while
excluding BG3: PAUSE, ZONE, options and glove remain fully opaque. Scenes
with existing color math or layer windows use a solid fill instead. No
extracted artwork is embedded. A fixed-size validated state extension follows co-op's extension and accepts older states without it.

Run the command above for isolated runtime checks and see [STATUS.md](STATUS.md)
for dated evidence. Full campaign, all arenas, bosses, Super/Tag Battle,
physical controls and rewind across transitions are not qualified. Native
tests cover menu/save/load state and rendering restoration.
