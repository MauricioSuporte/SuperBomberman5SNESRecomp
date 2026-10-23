# Discord: status e convites / Discord status and invites

## Português

Com o app do Discord aberto no PC, o perfil mostra que você está jogando **Super Bomberman 5 Recomp** e o que está
fazendo: no launcher, numa sala online (com quantos jogadores tem), no Normal Game ou no Battle, jogando online ou
offline, e há quanto tempo.

**Convidar amigos:** enquanto você está numa **sala online** (anfitrião ou convidado), a sala aparece no Discord como
um grupo com vagas:

- No chat com um amigo, clique no **+** e em **Convidar para jogar** (Invite to play). Quando ele clicar em **Entrar**
  (Join), o launcher dele abre o **Online Netplay**, conecta no servidor e entra na sua sala.
- O amigo também pode clicar em **Pedir para entrar** (Ask to Join) no seu perfil; o jogo aceita sozinho enquanto a
  sala tem vaga.
- Se o jogo do amigo estiver fechado, o Discord abre o jogo (no Windows) e ele entra na sala assim que o launcher
  abrir.

O convite não aparece em salas com senha, salas cheias, salas LAN / IP direto, nem para quem está na galeria de
espectadores. Cada jogador precisa da mesma versão do jogo (o servidor recusa versões diferentes). Para desligar:
`set SB5_DISCORD=0` antes de abrir o jogo.

**Copiar convite (qualquer chat):** na sala online, o botão **Copy Invite** no rodapé copia uma mensagem com o código
da sala (`sb5:<versão>:<sala>`). Cole no Discord, WhatsApp ou onde quiser. O amigo copia a mensagem inteira, abre o
**Online Netplay** e clica em **Join from Invite**: o launcher lê a área de transferência, conecta e entra na sala
(pede a senha, se houver). Funciona mesmo com o Discord fechado ou `SB5_DISCORD=0`. Salas LAN / IP direto não têm
esse botão.

Não precisa de login nem de nenhum programa extra: o jogo fala com o app do Discord do próprio PC (pipe local
`discord-ipc`). Com o Discord fechado, nada acontece.

## English

With the Discord app running, your profile shows **Super Bomberman 5 Recomp** and what you are doing: in the
launcher, in an online room (with its player count), in Normal Game or Battle, online or offline, and for how long.

**Inviting friends:** while you are in an **online room** (host or guest) the room is published as a party with free
seats. In a chat, use **+ → Invite to play**; when your friend clicks **Join**, their launcher opens Online Netplay,
connects and joins your room. **Ask to Join** on your profile is accepted automatically while the room has a free
seat. If the friend's game is closed, Discord starts it (Windows) and it joins once the launcher opens. No invite is
published for password rooms, full rooms, LAN / Direct IP rooms or spectators. Everyone needs the same game version.
`SB5_DISCORD=0` turns the integration off.

**Copy Invite (any chat):** in an online room the lobby footer's **Copy Invite** button copies a message holding the
room code (`sb5:<version>:<lobby id>`). Paste it in Discord or any chat; the friend copies the whole message, opens
Online Netplay and presses **Join from Invite**, which reads the clipboard and joins (asking for the password if the
room has one). Works with Discord closed or `SB5_DISCORD=0`. Not offered for LAN / Direct IP rooms.

It talks to the local Discord app over its `discord-ipc` pipe; no sign-in,
no SDK.

### Implementation

- `src/discord_presence.c`: IPC framing (handshake, `SET_ACTIVITY`, `SUBSCRIBE` to `ACTIVITY_JOIN` and
  `ACTIVITY_JOIN_REQUEST`, `SEND_ACTIVITY_JOIN_INVITE` / `CLOSE_ACTIVITY_REQUEST`, PING/PONG), non-blocking on the main
  thread, at most one activity update per 4 s. Ticks: the launcher's `poll_external_join` (recomp-ui fork) and
  `GameRunOneFrame` (WRAM read only: `$1031`, `$1D1A`). Join secret `sb5:<version>:<lobby id>`, party
  `sb5-<lobby id>` sized by the room's members and seats.
- Copy Invite / Join from Invite: `room_invite_text` / `room_invite_accept` (recomp-ui fork,
  `RECOMP_LAUNCHER_HAS_ROOM_INVITE`); `Sb5RoomInviteParse` finds the same `sb5:<version>:<lobby id>` token anywhere in
  the pasted text and queues it for `poll_external_join`, so it reuses the Discord join path.
- Application id `1562965215291904100` (public). Large image asset key `logo` (Developer Portal → Rich Presence → Art
  Assets).
- Windows: on first connection it registers `HKCU\Software\Classes\discord-<id>\shell\open\command` to the install's
  `SuperBomberman5Recomp.exe`, so Discord can start the game for an invite.
- recomp-ui fork: `RecompLauncherCGameInfo.poll_external_join` (`RECOMP_LAUNCHER_HAS_EXTERNAL_JOIN`) opens Online
  Netplay, connects and joins the room id; password rooms open the password prompt.
- Test: `discord_presence_tests` (POSIX) plays Discord on a fake `discord-ipc-0` socket.
