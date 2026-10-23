# Política de Privacidade / Privacy Policy — Super Bomberman 5 Recomp

Última atualização / Last updated: 2026-10-22

## Português

O **Super Bomberman 5 Recomp** é um projeto voluntário e sem fins comerciais. Os mantenedores **não** têm servidor
próprio, não coletam, não vendem e não guardam dados pessoais dos jogadores. Esta política explica quais dados saem do
seu PC quando você usa cada recurso e para quem vão.

**No seu PC.** ROM, saves, configurações, mods escolhidos e logs de diagnóstico (`saves\netplay`, só quando você liga o
diagnóstico) ficam apenas no seu computador. Eles só saem dele se você mesmo os enviar (por exemplo, ao reportar um
problema).

**Integração com o Discord** (aplicativo "Super Bomberman 5 Recomp", só com o app do Discord aberto). O jogo envia ao
app do Discord do seu PC o seu status (por exemplo "Battle", "Online room", tempo de jogo, versão do jogo) e, quando
você está numa sala online, o identificador da sala e quantos jogadores ela tem, para que amigos possam entrar. O
Discord mostra isso no seu perfil e trata esses dados conforme a
[Política de Privacidade do Discord](https://discord.com/privacy). Quando um amigo pede para entrar ou aceita um
convite, o Discord informa ao jogo o identificador da sala (e, no pedido, o id da conta do amigo, usado só para aceitar
o pedido e não guardado). No Windows o jogo grava uma chave no registro do seu usuário (`discord-<id>`) para que o
Discord possa abri-lo a partir de um convite. Você pode desligar tudo com `SB5_DISCORD=0` ou com a opção de atividade
do próprio Discord.

**Jogo online.** O netplay usa um servidor de lobby/relay de terceiros (o padrão é o do framework,
`netplay.retcomm.net`). Ele recebe o seu endereço IP, o nome que você escolhe, o país estimado pelo IP, as salas e as
mensagens de chat, e, se você entrar com Discord, a identificação da sua conta Discord. As partidas trocam controles e
dados do jogo diretamente entre os jogadores ou pelo relay, e os outros jogadores podem ver o seu nome e as suas
mensagens. Esses dados são tratados pelo operador desse servidor.

**Atualizações.** O botão **Update game / Atualizar jogo** e a verificação de versão consultam a API e os downloads do
GitHub, que recebe o seu endereço IP como em qualquer acesso (ver a
[Declaração de Privacidade do GitHub](https://docs.github.com/site-policy/privacy-policies/github-general-privacy-statement)).

**Crianças.** O projeto não é direcionado a menores de 13 anos e não coleta dados deles de propósito.

**Seus direitos e contato.** Como os mantenedores não guardam dados seus, pedidos sobre dados do Discord, do GitHub
ou do servidor de lobby devem ir para esses serviços. Dúvidas sobre o projeto:
<https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/issues> ou <https://discord.gg/kH54NCDQB4>.

**Mudanças.** Esta política pode ser atualizada; a versão atual fica sempre neste arquivo, com a data acima.

## English

**Super Bomberman 5 Recomp** is a volunteer, noncommercial project. Its maintainers run **no** server of their own and
do not collect, sell or keep players' personal data. This policy explains what leaves your PC for each feature and
where it goes.

**On your PC.** ROM, saves, settings, chosen mods and diagnostic logs (`saves\netplay`, only when you turn diagnostics
on) stay on your computer unless you send them yourself (for example, with a bug report).

**Discord integration** (the "Super Bomberman 5 Recomp" application, only while the Discord app runs). The game sends
the Discord app on your PC your status (for example "Battle", "Online room", play time, game version) and, while you
are in an online room, the room's id and player count so friends can join. Discord shows this on your profile and
handles it under the [Discord Privacy Policy](https://discord.com/privacy). When a friend asks to join or accepts an
invite, Discord tells the game the room id (and, for a request, the friend's account id, used only to accept the
request and not stored). On Windows the game writes a per-user registry key (`discord-<id>`) so Discord can start it
from an invite. Turn it off with `SB5_DISCORD=0` or Discord's activity setting.

**Online play.** Netplay uses a third-party lobby/relay server (by default the framework's, `netplay.retcomm.net`). It
receives your IP address, the name you choose, a country estimated from your IP, rooms and chat messages and, if you
sign in with Discord, your Discord account identity. Matches exchange controller and game data directly between
players or through the relay, and other players see your name and messages. That data is handled by the server's
operator.

**Updates.** **Update game** and the version check query GitHub's API and downloads, which receive your IP address as
with any request (see the
[GitHub General Privacy Statement](https://docs.github.com/site-policy/privacy-policies/github-general-privacy-statement)).

**Children.** The project is not directed at children under 13 and does not knowingly collect their data.

**Your rights and contact.** The maintainers hold no data about you; requests about data held by Discord, GitHub or
the lobby server go to those services. Questions about the project:
<https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/issues> or <https://discord.gg/kH54NCDQB4>.

**Changes.** This policy may be updated; the current version is always this file, dated above.
