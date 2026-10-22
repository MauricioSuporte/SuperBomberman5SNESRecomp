# v2.0.0 — diário de progresso

STATUS: DONE
Heartbeat: 2026-10-22 11:15 (America/Cuiaba)

## Decisões (opção recomendada, sem perguntar ao mantenedor)

- Base: `main` v1.1.2 (`2474613`); o clone local do mantenedor estava em v1.1.1 e não foi alterado.
- Capa no launcher: a imagem enviada pelo mantenedor entra como exceção autorizada
  (`launcher_assets/img/boxart.tga`), como o MMX Recompiled faz; registrada na auditoria.
- Forks do framework: novos commits vão para a branch `superbomberman5-v1.2` dos forks
  (sem force-push na `superbomberman5`).

## Fases

- [x] F0 — branch `release/v2.0.0`, plano, diário (2026-10-22 00:55)
- [x] F1 — aba Cheats + vidas ilimitadas (`12bb72f`; rota headless 1/2/5 jogadores OK)
- [x] F2 — capa do SB5 no launcher (`12bb72f`; staged em `assets/img/boxart.tga`; falta ver o launcher na tela)
- [x] F3 — framework nos pins do MMX (`2c64277`; forks em `superbomberman5-v1.2`)
- [x] F4 — netplay (delay-sync; 2 peers loopback validados; 3–5 via lobby não validados)
- [x] F5 — Discord (login do lobby + botões do servidor; capturas do launcher em Xvfb)
- [~] F6 — atualizador (Linux/AppImage testado de ponta a ponta; Windows só compila no CI)
- [x] F7 — Quiver (layout + docs/QUIVER.md; cadastro no site depende do login do mantenedor)
- [x] F8 — docs, versão, release (v2.0.0 publicada 2026-10-22)

## Registro

### 2026-10-22 05:50
- Ambiente: código no container da nuvem (`/home/claude/superbomberman5snesrecomp`, build do setup host
  em `/home/claude/build-setup`). Testes com ROM na VM Linux do computador do mantenedor
  (`$HOME/sb5lab/repo`, ferramenta `device_bash`, chamadas de no máximo 180 s; processos em segundo
  plano morrem ao fim de cada chamada → `ninja` em partes). Toolchain: `snesrecomp/tools/ci/fetch_toolchain.sh
  --artifact linux-x64 --tag v1.0.14` (+ `source .cache/toolchain-pack/env.sh`). Sem Rust na VM: o
  analisador nativo foi copiado do AppImage v1.1.2 (`recompiler-rs/target/release/snesrecomp-analyze`;
  fontes diferem um pouco — só para testes locais; o CI compila o novo). SDL configurado sem X11/áudio.
- `tests/infinite_lives_runtime.py --players 1|2|5`: sem o mod, vida 0 + tempo esgotado → Game Over
  (modo 8); com o mod, vidas continuam 0 e a fase reinicia (modo `$0D`). Passou nos três.
- Forks rebaseados sem conflito; o commit de multitap já está no upstream.

### 2026-10-22 06:35
- Netplay: `snesrecomp_enable_recomp_net` + ICE. Rollback com o beam driver diverge (PPU/VRAM) quando há
  reprevisão forçada (`RNET_RB_FORCE_MISPREDICT=20`), mesmo sem mods; corrigi no fork o ponto de retomada
  do driver e o diário raster (exec state), mas o `rb_probe` ainda acusa VRAM diferente (DMA fora do
  vblank?). Decisão: o jogo força `SNES_NET_MODE=delay` (lockstep), igual em todos os peers.
  `tests/netplay_pair_runtime.py --scene battle --inputs --mods all`: 7600 quadros, ~1700 em Battle,
  253 amostras de WRAM idênticas nos dois peers. LAN/IP direto só tem 2 assentos neste build; online 4.
- Knobs de validação no fork: `SNESRECOMP_NET_TEST_INPUT(_SCRIPT)`; jogo: `SB5_STATE_TRACE`.
- recomp-ui fork: `GameInfo.game_links` + `game_action` (botões sob o painel da ROM, na escolha de
  netplay e no painel online). Capturas: capa, botões Atualizar/Discord/Releases, Netplay, Discord.
- Atualizador `SuperBomberman5Updater` em `updater/` do pacote; teste Linux real (1.1.1→1.1.2 AppImage
  com SHA256SUMS). Windows: compila e tem teste nativo; extração por `tar` e cópia em lugar não testadas.
- Pendentes: F7 Quiver, F8 docs/versão/release; Windows real fica para o CI/mantenedor.

### 2026-10-22 06:40 — versão
- O mantenedor pediu para publicar como **v2.0.0** (não 1.2.0). Branch continua `release/v1.2.0`;
  plano/diário renomeados para `V2_0_0_*`.
- F7 Quiver: catálogo não lista o SB5; entrada pronta em `docs/QUIVER.md` (formato copiado da do Mega Man X
  via API do catálogo). Cadastro exige o login do mantenedor em quiverlauncher.com/submit (não posso criar
  conta/entrar). Atualizador respeita `installed-build.txt` do Quiver.
- 06:50 Cadastro enviado em quiverlauncher.com/submit com a conta do mantenedor (ele entrou no navegador
  embutido): Game port, repo, Super Bomberman 5 (1996, SNES), recompilação estática, Windows+Linux,
  desenvolvedor, "mostly AI-generated", captura de Battle (Kotetsu/Bazooka/Hammer/Honey/Terrorin).
  Status: "Submission received" — aguarda revisão.

### 2026-10-22 10:35
- Região do launcher: `rom_identity.txt` `region = USA` (pedido do mantenedor: a versão suportada é a USA;
  o cabeçalho mantém o byte de região japonês). Hashes e `game_id` inalterados. README, notas e
  `docs/images/launcher-v2.png` atualizados (`9c4499c`).
- CI de teste da Release (run 39766928140, sem publicar): windows OK, linux OK.
- Bloqueio: o fast-forward de `main` e o disparo da Release 2.0.0 (publish) foram negados pela checagem de
  permissões da sessão. Falta o mantenedor autorizar ou rodar:
  `git push origin release/v1.2.0:main` e Actions → Release → version `2.0.0`, publish ✔.
  Depois: testar o atualizador no Windows com a release publicada.

### 2026-10-22 11:15 — v2.0.0 publicada
- Mantenedor autorizou. `main` avançada por fast-forward até `258756e`; Release run 39798621872 (windows, linux,
  publish OK). Tag `v2.0.0` → `258756e`, release latest, não pré-release; assets: Windows ZIP 18.814.553 B,
  AppImage 14.637.560 B, SHA256SUMS conferido com os digests do GitHub; notas UTF-8 idênticas à fonte.
- Notas publicadas ainda dizem "ROM JPN" num parágrafo (gerador corrigido depois; esta sessão não pode editar
  releases — o mantenedor pode trocar o texto pela saída de `tools/release-notes.py`).
- Discord: novo passo do workflow `tools/discord-release-post.py` (secret `DISCORD_PATCH_NOTES_WEBHOOK`). O secret
  não pôde ser gravado desta sessão (proxy bloqueia a API de secrets); falta o mantenedor criar. O anúncio da
  v2.0.0 foi enviado uma vez pelo navegador embutido (webhook "Mecha Bomber", #patch-notes, HTTP 200).
- Pendente fora do escopo da sessão: testar o atualizador numa instalação real de Windows.
- Lembretes de retomada desativados.
