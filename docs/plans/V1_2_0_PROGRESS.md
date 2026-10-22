# v1.2.0 — diário de progresso

STATUS: EM ANDAMENTO
Heartbeat: 2026-10-22 05:50 (America/Cuiaba)

## Decisões (opção recomendada, sem perguntar ao mantenedor)

- Base: `main` v1.1.2 (`2474613`); o clone local do mantenedor estava em v1.1.1 e não foi alterado.
- Capa no launcher: a imagem enviada pelo mantenedor entra como exceção autorizada
  (`launcher_assets/img/boxart.tga`), como o MMX Recompiled faz; registrada na auditoria.
- Forks do framework: novos commits vão para a branch `superbomberman5-v1.2` dos forks
  (sem force-push na `superbomberman5`).

## Fases

- [x] F0 — branch `release/v1.2.0`, plano, diário (2026-10-22 00:55)
- [x] F1 — aba Cheats + vidas ilimitadas (`12bb72f`; rota headless 1/2/5 jogadores OK)
- [x] F2 — capa do SB5 no launcher (`12bb72f`; staged em `assets/img/boxart.tga`; falta ver o launcher na tela)
- [x] F3 — framework nos pins do MMX (`2c64277`; forks em `superbomberman5-v1.2`)
- [ ] F4 — netplay
- [ ] F5 — Discord
- [ ] F6 — atualizador
- [ ] F7 — Quiver
- [ ] F8 — docs, versão, release

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
