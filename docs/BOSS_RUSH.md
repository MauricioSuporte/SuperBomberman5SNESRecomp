# Boss Rush / Corrida de chefes

## Português

Em **Mods → Gameplay → Boss Rush**, ative **Boss Rush on the title screen**.
Vem desligado por padrão e exige um host compilado com este mod.

- **Tela de título:** o menu ganha **BOSS RUSH** entre **BATTLE GAME** e
  **OPTIONS**. Para caber sem mexer no copyright, a faixa de texto sobe uma
  linha de 16 pixels: **PUSH START BUTTON** continua acima de **NORMAL GAME**
  e continua piscando como no original, e a mão do cursor acompanha as quatro
  opções. Com o mod **Português brasileiro — menus**: **PRESSIONE START**,
  **JOGO NORMAL**, **MODO BATALHA**, **CORRIDA DE CHEFES** e **OPÇÕES**
  (em PT-BR a entrada BATTLE GAME passa a se chamar **MODO BATALHA** também
  sem este mod).
- **Ao escolher BOSS RUSH:** aparece a escolha de jogadores do Normal Game
  (1 ou 2; até 5 com o mod **Normal Game — five players**). Com o mod
  **Normal Game — character selection** ligado, abre em seguida a tela de
  seleção de personagens (com as cores, se o mod de cores estiver ligado),
  como no Normal Game; **B** nela volta para a tela de título. Os personagens e
  as cores escolhidos valem para todo o boss rush.
- **O boss rush** é o mesmo da senha **3879**: começa no chefe da ZONA 1 e cada
  portal leva ao chefe seguinte; nada é salvo no arquivo de jogo.
- **Cronômetro:** no canto superior direito, logo abaixo do HUD, aparece o tempo
  do boss rush (`M:SS.CC`). Ele para enquanto o jogo está pausado e nas
  transições com a tela preta.
- **Pausa:** Start abre o menu de pausa com **Continuar** e **Voltar ao título**
  (com confirmação, inicialmente em **Não**), mesmo sem o mod de pausa ligado.

### Como funciona

- O menu de título é o script da tarefa 11 (`$C7:1831-`). O limite do cursor
  do menu 0 (`$1037`, `5b 02` em `$C7:19DA`) passa a 3 (só na memória). Quando
  a confirmação (`$C7:18A8`) lê o cursor `$1032`, a entrada 2 (Boss Rush) vira
  0 (o caminho do Normal, que abre a escolha de jogadores, `$1036 = 1`) com o
  boss rush marcado, e a entrada 3 vira 2 (Options).
- A escolha de jogadores grava `$1D25/$1D24` (1P, 2P ou o grupo do co-op) e
  muda de cena com `0F 03` (`$C7:1918` 1P, `$C7:193A` 2P). Para o boss rush
  esse op vira a largada da senha: o tratador de 3879 (`$C5:695E`) é refeito
  com as rotinas do próprio cartucho chamadas a partir do despacho do
  interpretador de scripts (`$C2:0230`, um `JSL` cujo quadro de retorno as
  chamadas reaproveitam): `$C0:499C` (limpa as flags 16-115), `$C2:033A` com 4
  (flag 4: portais de chefe levam ao próximo chefe), `$6274` bit 7 (sem salvar,
  `$C0:48D6`), `$6248 = $67`, `$6254 = 0` e `$C4:286F` (reinicia os jogadores),
  com a máscara de jogadores escolhida em vez da máscara fixa de 1 jogador.
  Depois `$1031 = $67`, `$1D1E = 4` como o op `0F`, e o script de título termina
  como no caminho de Options; o gerente de cena (`$C5:3AA0`) faz o fade e
  carrega a ZONA 1 - A.
- Com a seleção de personagens: o script de título fica parado nesse op (um
  programa de espera de 1 quadro + salto em espaço `$FF` em `$C7:3100`) e o mod
  de personagens guarda um snapshot do título. O op segue nativo (`0F 03`), a
  cena do menu de arquivos começa, o script dela fica parado no primeiro op
  (`$C7:0B82`) e a tela de personagens abre ali, como para um arquivo do Normal.
  Ao fechar, o snapshot do título volta: o boss rush começa com as escolhas
  aplicadas, ou B leva para a tela de título (`0F 02`). Começar direto da cena
  do menu de arquivos não funciona: o executor de scripts da cena passa para os
  scripts seguintes e o gerente nunca vê o pedido de troca de cena.
- O layout da tela de título é só de desenho (`ptbr_mod.c`): linhas BG3 14-23,
  copyright intacto nas linhas 25-26, janela 2 do BG3 desligada e a mesma fase
  de piscar do original (`W34SEL` bit 11), cursor (OAM 0) 16 linhas acima. Em
  inglês reaproveita as palavras de tile da própria tela (BOSS RUSH é montado
  com letras que já estão nela); em PT-BR desenha as cinco linhas com a fonte
  do cartucho. Tudo é restaurado na linha 224.
- O cronômetro conta quadros dentro da simulação (extensão de save state) e é
  desenhado sobre o quadro pronto, com a fonte autoral dos menus, seguindo o
  brilho da tela.
- Nada na ROM em disco nem no SRAM é alterado; os patches na memória são
  desfeitos ao desligar/resetar.

### Validação e limites

Veja a seção datada em [STATUS.md](STATUS.md). Validado no Linux (LLE,
headless) com rotas roteirizadas: título em inglês e PT-BR (quatro entradas,
cursor, piscar), Options/Battle/Normal ainda funcionam, 1P sem seleção, 1P e 2P
com seleção + cores (+ todos os mods), B na seleção volta ao título, 5
jogadores do co-op, pausa → voltar ao título (com e sem o mod de pausa), a
senha 3879 nativa continua funcionando. **Não** validado: boss rush completo
até o fim (só a primeira luta foi jogada), fim de jogo e créditos depois do
último chefe, netplay, Windows.

## English

Enable **Mods → Gameplay → Boss Rush → Boss Rush on the title screen**. It is
off by default and needs a host built with this mod.

- **Title screen:** **BOSS RUSH** sits between **BATTLE GAME** and **OPTIONS**.
  The text block moves up one 16-pixel row so the copyright stays where it is:
  **PUSH START BUTTON** is still above **NORMAL GAME** and still blinks, and the
  cursor hand follows the four entries. With **Português brasileiro — menus**:
  **PRESSIONE START**, **JOGO NORMAL**, **MODO BATALHA**, **CORRIDA DE
  CHEFES**, **OPÇÕES** (in PT-BR the BATTLE GAME entry now reads
  **MODO BATALHA** also without this mod).
- **Choosing BOSS RUSH** opens Normal Game's player-count menu (1 or 2; up to
  5 with **Normal Game — five players**). With **Normal Game — character
  selection** enabled the character picker follows (with colors when that mod
  is on), as in Normal Game; **B** there returns to the title screen. The
  characters and colors chosen are used for the whole rush.
- **The rush** is the cartridge's own, the one password **3879** starts: ZONE 1's
  boss first, each gate leads to the next boss, nothing is saved.
- **Timer** at the top right, just under the HUD (`M:SS.CC`); it stops while
  paused and during black transitions.
- **Pause:** Start opens **Resume** / **Return to title** (with a confirmation
  that starts on **No**), even when the pause-menu mod is off.

### How it works

See the Portuguese section for addresses: the title menu limit and cursor
remap (`$C7:19DA`, `$C7:18A8`), the 3879 handler rebuilt from the cartridge's
own routines (`$C0:499C`, `$C2:033A`, `$C4:286F`) injected from the script
interpreter's dispatch (`$C2:0230`) at the player-count menu's `0F 03` op with
the chosen player mask, the scene change as op `0F` does, the picker hosted by
the file-menu scene with a title snapshot as its return point, the render-only
title layout and the frame-counted timer (save-state extension).

### Validation and limits

Linux LLE headless routes only (see STATUS.md). Not validated: a complete rush
to the end, the ending after the last boss, netplay and Windows.
