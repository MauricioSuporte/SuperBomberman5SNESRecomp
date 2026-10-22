# Plano de execução — v1.2.0

Branch de trabalho: `release/v1.2.0` (a partir de `main` v1.1.2, `2474613`).
Diário de progresso: `docs/plans/V1_2_0_PROGRESS.md` (sempre atualizado e enviado ao GitHub
ao fim de cada etapa). Referência principal: **Mega Man X Recompiled**
(`mstan/MegaManXSNESRecomp`, pins `snesrecomp 7a67b85`, `recomp-ui 7c635c6`).

O mantenedor deu permissão total: não perguntar nada; quando houver dúvida, escolher a
opção recomendada e registrar a decisão no diário.

## Pedidos

1. Aba **Cheats** no launcher: mover o mod de tempo ilimitado para ela e criar
   **vidas ilimitadas** (Normal Game).
2. **Netplay** como no MMX Recompiled (online por lobby + LAN/IP direto).
3. **Discord** no netplay (login do lobby, como no MMX) + botão/convite do servidor do
   SB5: https://discord.gg/kH54NCDQB4 (busca de partidas e novidades).
4. **Capa do SB5** no painel da ROM do launcher (como a capa do MMX).
5. **Atualizador** que lê https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases,
   baixa e aplica a versão mais nova preservando ROM/saves/config; botão no launcher;
   netplay exige a mesma versão.
6. Repositório **listável no Quiver Launcher** (https://github.com/tgeorgiadis/quiver-launcher).

## O que já existe (apurado em 2026-10-22)

- Forks `MauricioSuporte/snesrecomp` e `recomp-ui` (branch `superbomberman5`) = upstream
  RetroPortingToolKit + 6 / 2 commits nossos (assentos multitap 3–8). Já trazem netplay
  (`runner/recomp_net.cmake`, `runner/src/netplay/*`, lobby) e o login Discord da recomp-ui.
  O MMX usa versões mais novas do upstream (snesrecomp +40 commits sobre nossa base).
- `CMakeLists.txt` já chama `recomp_target_launcher_ui(... BOXART launcher_assets/img/boxart.tga)`
  — falta só o arquivo.
- Mods do launcher: campo `group` no `manifest.toml` define a aba (`Gameplay`, `5 Players`).
- Releases: `superbomberman5-<v>-windows-x64.zip`, `...-linux-x86_64.AppImage`, `SHA256SUMS`
  (nomes já reconhecidos pelo `PlatformAssetMatcher` do Quiver).

## Fases (marcar no diário ao concluir)

| Fase | Entrega | Pronto quando |
|---|---|---|
| F0 | Branch, plano, diário, agendamentos de retomada | plano no GitHub + tarefas agendadas |
| F1 | Aba Cheats: `infinite_time` → `group = "Cheats"`; mod novo `superbomberman5.infinite-lives` (`src/infinite_lives_mod.c`, teste nativo, manifesto, `tools/package-infinite-lives.py`, doc `docs/INFINITE_LIVES.md`) | vidas não caem no Normal (1P e co-op 5P), rota headless comprovando; testes |
| F2 | Capa: imagem 2 → `launcher_assets/img/boxart.tga`; exceção revisada na auditoria (`docs/images/README.md`), entra no pacote | launcher mostra a capa; auditoria passa |
| F3 | Framework: rebasear nossos commits dos forks sobre os pins do MMX; publicar em `superbomberman5-v1.2` nos forks; atualizar gitlinks, `.gitmodules`, `framework_pins.txt` | build + 18 testes nativos + rota Zona 1-1 iguais à v1.1.2 |
| F4 | Netplay: `snesrecomp_enable_recomp_net` + ICE; `src/sb5_netplay.c` (política de assentos, mods do host adotados, recursos locais desligados online, versão do jogo = `VERSION`); estado dos mods incluído no rollback (extensões de save state) | dois peers headless (UDP local) com mesmo digest por ≥600 quadros em Battle e Normal; build Windows no CI |
| F5 | Discord: login do lobby (recomp-ui) ligado + botão "Servidor do Discord do SB5" no netplay e no launcher + README | botão abre o convite; doc |
| F6 | Atualizador: `SuperBomberman5Updater` (C, WinHTTP no Windows / curl no Linux): GitHub `releases/latest`, compara `VERSION`, baixa o zip, confere `SHA256SUMS`, troca arquivos preservando ROM, `saves/`, `rom.cfg`, `*.ini`, `mods/preloaded/state.toml`; relança o setup host para regenerar; botão "Atualizar" no launcher; netplay bloqueia versão diferente e aponta o atualizador | teste nativo do comparador/filtro + atualização real v1.1.2 → build local simulado |
| F7 | Quiver: exe na raiz do zip, `docs/QUIVER.md` com a entrada de catálogo (`repository`, `folderName`, filtro de release, arquivo da ROM a adicionar), atualizador detecta instalação do Quiver e delega a ele; tentar cadastrar em quiverlauncher.com | zip passa no `PlatformAssetMatcher` (teste com os nomes reais) |
| F8 | Docs EN/PT (README, STATUS, AGENTS, docs por recurso), `VERSION` 1.2.0, `tools/release-notes.py`, CI, PR para `main`, merge, tag `v1.2.0`, release | release publicada com zip/AppImage/SHA256SUMS |

## Regras (AGENTS.md)

Nada de dados do jogo no Git; não editar código gerado; patches de ROM só em memória;
LLE; afirmações honestas (separar "compila" de "validado em execução"); docs bilíngues;
mensagens de commit em inglês com as linhas de atribuição; nunca `git add -A`.

## Como retomar (sessão nova ou depois do limite)

1. `add_repo MauricioSuporte/SuperBomberman5SNESRecomp` (push) e clonar em
   `/home/claude/superbomberman5snesrecomp`; `git fetch origin release/v1.2.0` e
   `git checkout release/v1.2.0`; `git submodule update --init --recursive`.
2. Ler este plano, `docs/plans/V1_2_0_PROGRESS.md`, `AGENTS.md` e as skills em `.agents/skills/`.
3. Se o diário tiver "batimento" (linha `Heartbeat:`) com menos de 60 min, outra sessão está
   trabalhando: não fazer nada. Senão atualizar o batimento e continuar pela primeira fase
   não concluída.
4. ROMs: no computador do mantenedor em `C:\Users\mauri\source\repos\Super-Bomberman-5-Recomp`
   (`Super Bomberman 5 (USA).sfc`, `Super Bomberman 4 (Japan).sfc`). Copiar para fora do Git
   (`.bootstrap/`) via ponte do dispositivo quando precisar rodar.
5. Ao terminar tudo, escrever `STATUS: DONE` no diário e desativar as tarefas agendadas
   "SB5 v1.2.0 — retomar".
