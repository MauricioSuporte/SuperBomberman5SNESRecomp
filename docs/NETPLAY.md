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
4. O anfitrião escolhe os mods; a sala adota os mods e as versões do anfitrião e recusa quem não tiver os
   mesmos.
5. O botão **Discord: online matches & news** abre o servidor do Super Bomberman 5 para achar partidas e
   novidades: <https://discord.gg/kH54NCDQB4>.

**Modo de sincronização:** o jogo força *delay-sync* (lockstep, com atraso de entrada configurável na sala).
O *rollback* do framework divergiu com este port LLE quando havia reprevisão (ver Validação); uma variável
`SNES_NET_MODE=rollback` explícita ainda o liga para testes.

**Assentos:** LAN / IP direto tem dois assentos neste build; salas online têm **cinco** assentos, um
para cada jogador do multitap (desde a v2.0.2; antes o framework limitava a quatro). Mais de dois peers foi construído no framework, mas
**não foi exercitado**.

Partidas começam do boot frio. Save states, rewind e pausa local ficam desligados online (regras do
framework); o menu de estados sincronizado do anfitrião é **Select+R**.

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

Ferramentas: `SNESRECOMP_NET_TEST_INPUT=<semente>` / `SNESRECOMP_NET_TEST_INPUT_SCRIPT=<arquivo>` (fork do
snesrecomp) dão entradas determinísticas a peers sem janela; `SB5_STATE_TRACE=<período>` imprime o hash da
WRAM.

## English

Since v2.0.0 the launcher offers **NETPLAY** like Mega Man X Recompiled: transport, lobby, Discord sign-in
and synchronization are the framework's (snesrecomp + recomp-net + recomp-ui). Every player needs the
same version (rooms only match the same version — use **Update game** in the launcher) and their own ROM
(plus their own Super Bomberman 4 ROM when Bomber Kings is on). Configure local **PLAYER 1**; choose
**Online Netplay** (Discord sign-in) or **LAN / Direct IP**. The room adopts the host's mods. The
**Discord: online matches & news** button opens the Super Bomberman 5 server,
<https://discord.gg/kH54NCDQB4>.

The game forces **delay-sync** (lockstep with a room input delay); the framework's rollback forked on this
LLE port under forced mispredictions. LAN / Direct IP has two seats; online rooms have five seats, one per multitap player (since v2.0.2; four before); more
than two peers is built in the framework but not exercised. Two real executables stayed byte-identical in
WRAM over 7,600 loopback frames including ~1,700 frames of Battle with every gameplay mod on. No real
two-machine internet match has been played yet.
