# Netplay / Jogo online

## Português

Desde a v2.0.0 o launcher tem **NETPLAY**, como o Mega Man X Recompiled: o transporte, o lobby, o login
com Discord e a sincronização são os do framework (snesrecomp + recomp-net + recomp-ui).

1. Todos os jogadores usam **a mesma versão** (o lobby só junta salas da mesma versão — use
   **Update game / Atualizar jogo** no launcher) e cada um usa **a própria ROM** do SB5
   (e a do Super Bomberman 4, se o mod Bomber Kings estiver ligado).
2. Configure o **PLAYER 1** local (teclado ou controle). A rede leva essa entrada para o assento da sala.
3. **NETPLAY → Online Netplay**: entre com Discord (ou como convidado, se o servidor deixar), crie ou entre
   numa sala. **LAN / Direct IP**: rede local ou endereço direto, sem conta.
4. O anfitrião escolhe os mods: ao entrar, os seus mods passam a ser exatamente os dele (desde a v2.0.2 também
   com muitos mods ligados, quando o anfitrião não consegue publicar o conjunto inteiro e o jogo o monta pela
   lista de pacotes). Os mods que o anfitrião não usa são desligados, inclusive quando ele joga sem mods
   (desde a v2.0.8), e a escolha é refeita se você religar um mod na sala. A troca fica salva como a sua
   seleção de mods. Bomber Kings ainda exige a sua ROM do Super Bomberman 4.
5. O botão **Discord: online matches & news** abre o servidor do Super Bomberman 5 para achar partidas e
   novidades: <https://discord.gg/kH54NCDQB4>.

**Diagnóstico:** abra o jogo pelo `tools/netplay-diag.cmd` (copie-o para a pasta do jogo, ao lado de
`SuperBomberman5Recomp.exe`) ou rode `set SNES_NET_DIAG=1` antes de abrir. Cada partida grava em `saves\netplay\`
(na pasta do jogo ou em `build\`) um `net_diag_<data>_<hora>_<host|guest>_slot<N>.jsonl` (paradas, atraso, caminho
da conexão, FPS, quadros lentos, quanto o convidado está atrás do anfitrião) e acrescenta o log do jogo em
`netplay_log.txt` (`delay_sync_starvation`, `local_slow`, `host_authority backlog`). Anfitrião **e** convidado devem
mandar a pasta `saves\netplay`; `python tools/netplay-diag-report.py <pastas>` resume tudo.
O atraso automático (desde a v2.0.6) usa a soma das duas piores latências quando há dois ou mais convidados.

**Convidado atrasado (correção da v2.1.1):** com o anfitrião decidindo, um convidado que
parava um instante (um pacote perdido esperando reenvio, um quadro lento) nunca recuperava o atraso: cada parada
somava atraso nos controles, e passando de ~64 quadros (~1 s) ele **congelava para sempre** (tela parada/preta para o
convidado, personagem dele travado para o anfitrião). Agora o convidado roda até 3 quadros extras por quadro para
alcançar o anfitrião (`SNES_NET_CATCHUP_CAP`), o anfitrião reenvia até 120 quadros perdidos, e se mesmo assim o
convidado ficar irrecuperável (conexão parada por mais de ~2 s) ele volta para a sala com uma mensagem em vez de
ficar congelado. Na LAN / IP direto, um pacote perdido do anfitrião também podia congelar o convidado; corrigido.

**Anfitrião decide (desde a v2.0.7):** o anfitrião nunca espera; a cada quadro usa a entrada mais recente de
cada convidado e envia o resultado a todos (`SNES_NET_HOST_AUTHORITY`, ligado pelo jogo). Quem tem latência alta
sente atraso nos próprios controles, sem derrubar o FPS da partida.

**Modo de sincronização:** o jogo força *delay-sync* (lockstep, com atraso de entrada configurável na sala).
O *rollback* do framework divergiu com este port LLE quando havia reprevisão (ver Validação); uma variável
`SNES_NET_MODE=rollback` explícita ainda o liga para testes.

**Assentos:** LAN / IP direto tem dois assentos neste build; salas online têm **cinco** assentos, um
para cada jogador do multitap (desde a v2.0.2; antes o framework limitava a quatro). Mais de dois peers foi construído no framework, mas
**não foi exercitado**.

Partidas começam do boot frio. Save states, rewind e pausa local ficam desligados online (regras do
framework); o menu de estados sincronizado do anfitrião é **Select+R**.

### Chat no jogo

Desde a v2.1.0 o chat da sala do launcher continua funcionando dentro do jogo durante uma partida online:

- As mensagens aparecem no **canto inferior esquerdo** da janela, a mais nova embaixo e as antigas subindo.
  Cada mensagem nova fica visível por **5 segundos** e some com um fade curto.
- **T** abre a barra de texto e mostra todo o histórico que couber na tela (inclusive o que foi dito na sala
  antes da partida). **Enter** envia e fecha; Enter com a barra vazia ou **Esc** só fecham (com o chat aberto,
  Esc não sai da partida). Acentos usam a entrada de texto do sistema.
- Fonte Lato Bold (a do launcher) com fundo preto translúcido; o nome de quem escreveu aparece em cor
  (o seu em dourado), mensagens da sala (entrou/saiu) em verde. Jogadores ignorados no launcher ficam ocultos.
- Enquanto a barra está aberta, e depois até você soltar as teclas, o **teclado** manda controle neutro para a
  partida (o Enter não vira Start); um **controle** continua jogando. Isso é entrada normal da rede: não dessincroniza.
- O chat é desenhado na resolução da janela, por cima do jogo e do shader, e não entra em save states nem em
  capturas de tela (F12).
- **LAN / IP direto:** o socket da sala de espera fecha quando a partida começa, então não há chat durante a
  partida; enviar mostra "Chat is not available in this match.".

**Nomes sobre os personagens:** o nome de cada jogador na sala aparece em cima do personagem que ele controla,
em branco com contorno escuro e 75% de opacidade, só enquanto o personagem está em cena (fase rodando, sem pausa) e vivo e é controlado
por uma pessoa da partida: no Battle os COM não têm nome; no Normal entram só os assentos que estão jogando, nunca os
rivais dos chefes. Some no instante em que o personagem é atingido e volta quando ele renasce e a fase recomeça.
Montado num Louie, o nome sobe junto. O nome vem do assento da sala (assento 1 = P1 e assim por
diante). Se o anfitrião assistir da galeria, os nomes podem ficar deslocados um assento (limitação conhecida).

**Entrar com a partida em andamento:** quem abre o Netplay e entra numa sala cuja partida já começou fica na sala
de espera do launcher (num assento livre ou na galeria de espectadores, se a sala tiver), conversa no chat da sala, que
os jogadores veem dentro do jogo, e joga ou assiste a partir da partida seguinte. Desde a v2.1.0, um espectador que
fecha o jogo não encerra mais a partida dos jogadores (antes, o aviso de saída dele era lido como a saída de um
jogador). Um jogador que sai continua encerrando a partida para todos, como antes.

Implementação no fork do snesrecomp: `runner/src/desktop/host_chat.c` (estado, layout e rasterização com o
stb_truetype do recomp-ui) e ganchos em `host_main.c` (teclas, entrada de texto, controle neutro, renderer SDL)
e `opengl.c` (camada na resolução da janela); nomes em `src/name_tags.c` (jogo) e no fork do recomp-net a correção
do espectador (`rnet_session.c`). Testes nativos: `ingame_chat_tests`, `name_tags_tests` e o caso de espectador do
`session_state_test` do recomp-net.

### Validação (2026-10-22)

- Dois executáveis reais no loopback UDP (`tests/netplay_pair_runtime.py --scene battle --inputs
  --mods all`): 7.600 quadros, rota até o Battle e ~1.700 quadros de Battle com entradas pseudoaleatórias
  nos dois assentos e todos os mods de jogo ligados; 253 amostras de hash da WRAM (`SB5_STATE_TRACE=30`)
  idênticas nos dois peers.
- Rollback: com `RNET_RB_FORCE_MISPREDICT=20` os peers bifurcaram (PPU) mesmo sem mods. No fork do
  snesrecomp o ponto de retomada do beam driver e o diário raster passaram a entrar no snapshot; o
  `SNESRECOMP_RB_PROBE` ainda acusa VRAM diferente em 7 de 207 sondas. Por isso o delay-sync.
- O login com Discord e a lista de salas foram vistos no launcher (servidor de lobby compartilhado);
  uma partida real pela internet entre duas máquinas **não** foi feita.

Chat no jogo (2026-10-22): `ingame_chat_tests` (histórico × mensagens novas, 5 s + fade, ignorados, digitação
UTF-8, Enter/Esc, limite de 255 bytes, rasterização 1920×1080 e 1280×720) passou com AddressSanitizer/UBSan
(gcc, Linux); `host_main.c`/`opengl.c` compilam com e sem o chat. **Ainda não** foi compilado pelo toolchain
Windows nem testado numa partida real; não se sabe ainda se o servidor de lobby repassa o chat da sala durante
a partida (o cliente permite).

Ferramentas: `SNESRECOMP_NET_TEST_INPUT=<semente>` / `SNESRECOMP_NET_TEST_INPUT_SCRIPT=<arquivo>` (fork do
snesrecomp) dão entradas determinísticas a peers sem janela; `SB5_STATE_TRACE=<período>` imprime o hash da
WRAM.

## English

Since v2.0.0 the launcher offers **NETPLAY** like Mega Man X Recompiled: transport, lobby, Discord sign-in
and synchronization are the framework's (snesrecomp + recomp-net + recomp-ui). Every player needs the
same version (rooms only match the same version — use **Update game** in the launcher) and their own ROM
(plus their own Super Bomberman 4 ROM when Bomber Kings is on). Configure local **PLAYER 1**; choose
**Online Netplay** (Discord sign-in) or **LAN / Direct IP**. Joining switches your mods to the host's automatically (since v2.0.2 also with many mods on): mods the
host does not run are switched off, also when the host plays without mods (since v2.0.8), and re-enabling
one in the room is undone. The switch is saved as your Mods selection. The
**Discord: online matches & news** button opens the Super Bomberman 5 server,
<https://discord.gg/kH54NCDQB4>.

**In-game chat (since v2.1.0):** during an online match the room chat stays in the game, bottom-left of the
window: newest line at the bottom, older ones moving up, each new line visible for 5 seconds. **T** opens a text
bar with all the history that fits; **Enter** sends and closes, an empty Enter or **Esc** just closes (Esc does
not leave the match while typing). Lato Bold on a translucent black background, the sender's name in color
(yours in gold), room notices in green; players ignored in the launcher stay hidden. While typing (and until the
keys are released) the keyboard pad sent to the match is neutral; a gamepad keeps playing. Drawn at window
resolution after the frame and shader, not in save states or F12 screenshots. LAN / Direct IP closes the waiting
room socket at launch, so there is no chat during a LAN match. Code: snesrecomp fork `runner/src/desktop/host_chat.c`
plus hooks in `host_main.c` and `opengl.c`; native test `ingame_chat_tests` (ASan/UBSan clean on Linux gcc).
Not yet built with the Windows toolchain nor tried in a real match; whether the lobby server relays room chat
during a match is still to be confirmed.

**Name tags (v2.1.0):** each player's room name (white, dark outline, 75% opacity) floats over the bomber they control while it is on the
field (stage running, not paused), alive and driven by a person in the match: no tags on Battle COMs, on campaign
seats that are not playing or on boss rival bombers. It disappears the moment the bomber is hit and comes back
when it respawns and the stage resumes; a Louie rider's tag rises with it. Tags follow the lobby seat
(seat 1 = P1, ...); with the host watching from the gallery they can be one seat off (known gap).

**Joining a running match (v2.1.0):** someone who enters a room whose match already started waits in the
launcher's room (free seat or spectator gallery), chats with the players (who see it in game) and plays or watches
from the next match. A spectator closing the game no longer ends the players' match (its BYE used to be read as a
player leaving, recomp-net fork `rnet_session.c`); a player leaving still ends it for everyone.

Since v2.0.7 the host is authoritative: it never waits, uses each guest's newest input every frame and sends
the resolved inputs to everyone, so latency shows up as input lag for distant players instead of frame drops.

**Diagnostics:** start the game through `tools/netplay-diag.cmd` (copy it next to `SuperBomberman5Recomp.exe`) or
set `SNES_NET_DIAG=1`. Each match writes `saves\netplay\net_diag_<date>_<time>_<host|guest>_slot<N>.jsonl` (stalls,
delay, connection path, fps, slow frames, how far the guest trails the host) and appends the game's log to
`netplay_log.txt`. Collect both peers' `saves\netplay` folders and run `python tools/netplay-diag-report.py <folders>`.

**Guest catch-up (v2.1.1 fix):** under host authority a guest that stalled briefly (a lost
datagram waiting for its resend, a slow frame) never caught up: each stall added permanent input lag, and past
~64 frames (~1 s) the guest froze for good (frozen/black screen for the guest, its bomber stuck on the host). The
guest now runs up to 3 extra frames per wall frame to catch up (`SNES_NET_CATCHUP_CAP`), the host resends up to
120 missed rows, and a guest that still cannot be fed (stalled more than ~2 s) returns to the room with a reason
instead of freezing. A lost host row could also freeze a LAN / Direct IP guest; fixed.
The game forces **delay-sync** (lockstep with a room input delay); the framework's rollback forked on this
LLE port under forced mispredictions. LAN / Direct IP has two seats; online rooms have five seats, one per multitap player (since v2.0.2; four before); more
than two peers is built in the framework but not exercised. Two real executables stayed byte-identical in
WRAM over 7,600 loopback frames including ~1,700 frames of Battle with every gameplay mod on. No real
two-machine internet match has been played yet.
