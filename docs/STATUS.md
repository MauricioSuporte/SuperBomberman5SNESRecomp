# Super Bomberman 5: estado do protótipo

Base criada pelo scaffolder oficial do SNESRecomp em 2026-10-13.

## ROM de referência

- Arquivo local: `Super Bomberman 5 (USA).sfc`.
- Cabeçalho: `Hu SUPER BOMBERMAN 5`, região JPN. O nome do arquivo não determina a região.
- Fast HiROM, 4 MiB, SRAM de 8 KiB, sem coprocessador especial detectado.
- Checksum interno válido; isso não comprova sozinho que a imagem é um dump original sem modificações.
- CRC32: `c25ab56a`.
- SHA-256: `5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4`.
- Identidade usada pela geração e pelo executável: `rom_identity.txt`.

## Base disponível

Repositório Git local com SNESRecomp e recomp-ui como submódulos fixados em commits, CMake, configuração inicial de análise, launcher, scripts de geração e empacotamento e workflow oficial. A inicialização do jogo ativa o multitap na porta 2 para cinco assentos; multiplayer ainda não foi validado. Netplay está desabilitado.

Ferramentas locais preparadas em `.tools/`: RetroPorting Toolchains v1.0.14 (CMake, Ninja, Clang e SDL3) e Rust 1.97.1 Windows GNU LLVM. Scripts reproduzíveis: `tools/setup-windows.ps1` e `tools/build-windows.ps1`. Instruções em `BUILD_WINDOWS.md`.

Fontes locais gerados com sucesso e executável Release compilado em `build-lle/SuperBomberman5SNESRecomp.exe`, com SDL3 estático e composição de frames LLE. `play.cmd` abre o launcher.

A ROM, fontes gerados, ferramentas locais, builds, saves, dumps e capturas são ignorados pelo Git e sujeitos à auditoria de publicação.

Repositório público: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp. Formato da publicação inicial v0.1.0: pré-release Windows x64 com setup host compilado sem dados da ROM, fontes, analisador nativo independente da ROM e sua DLL/licença. O jogo completo é gerado no computador do jogador. Não distribuímos o executável local `build-lle/`, que contém código derivado da ROM.

README em inglês/português, CONTRIBUTING.md, THIRD_PARTY_NOTICES.md e docs/PUBLISHING.md descrevem instalação, limites, licenças e fluxo de versões. A skill `$super-bomberman5-repository` está instalada localmente e versionada em `.agents/skills/`. Source checks audita o histórico e executa os testes de publicação; Release gera pacotes Windows manualmente, com ferramentas fixadas em v1.0.14.

## Publicação v0.1.0

Pré-release publicada em 2026-10-13: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases/tag/v0.1.0.

- Tag anotada `v0.1.0`: commit `6ba1ee0ffea2560d18f65518ddf0ba72cfaf72a1`.
- ZIP Windows x64: `superbomberman5-0.1.0-windows-x64.zip`, 48.814.122 bytes.
- SHA-256: `4656188e1d4176798ef08a73ece8a32f5ab0dc9abdabc9814c79a6c8b26802d1`, acompanhado de `SHA256SUMS`.
- Build Release no GitHub Actions passou: https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/actions/runs/38475851829.
- Arquivo final auditado sem ROM, código C gerado, executável derivado da ROM, capturas de jogos das dependências ou fixtures de ROM. Avisos de licença do framework, launcher, Cargo, Rust e LLVM foram preservados.
- ZIP extraído: launcher abriu; analisador executou um programa RTS sintético sem Rust nem PATH das ferramentas; geração com a ROM local, rebuild e execução de 600 frames foram verificados. A build gerada manteve `SNESRECOMP_FRAME_IMPL=LLE`.
- GitHub confirmou pré-release pública com dois assets, tamanhos e digests iguais aos arquivos locais. O executável jogável local e suas capturas não foram enviados.

A validação usa as rotinas de geração/rebuild do pacote e smoke tests do launcher. A sequência completa de cliques no assistente, novas instalações em outros computadores e o download automático de ferramentas ainda precisam de testes adicionais.

## Validação realizada

Probe de cabeçalho e checksum, verificação de CRC32/SHA-256 pelo CLI e conferência dos submódulos após mover o scaffold para a pasta de trabalho.

Teste de 600 frames sem entrada e testes de navegação com entradas roteirizadas. Capturas locais mostram a abertura animada, tela de título, menus de modo normal, seleção de arquivo e mapa da Zone 1. A rota de 3025 frames até o mapa terminou com código zero, sem mensagens `interp_cap`, `unresolved-abandon` ou BRK. Evidências brutas estão em `.bootstrap/validation/`, ignoradas pelo Git. As primeiras capturas usam SDL vídeo/áudio dummy: não comprovam a qualidade sonora.

O script de setup também foi executado novamente para verificar o reaproveitamento das ferramentas, e o script de build foi validado pelo parser do PowerShell e executado na geração e compilação.

A rota final `validation/first-stage.input` chegou à primeira fase da Zone 1 e terminou após 3628 frames com código zero. As capturas mostram cenário, jogador e cronômetro avançando. Sem mensagens `interp_cap`, `unresolved-abandon`, BRK, erro ou fatal nessa execução. Não há comparação independente de colisões, bombas ou precisão de timing ainda.

Um smoke test adicional de 180 frames abriu a janela SDL real do Windows e o dispositivo de áudio real, com callbacks de áudio e saída normal. O launcher ImGui/SDL3 também abriu e encerrou normalmente pelo mecanismo de smoke test de 60 frames. Isso comprova inicialização dos dispositivos, não fidelidade ou qualidade sonora auditiva.

## Cobertura nativa e limitação atual

A configuração conservadora gera cinco variantes AOT elegíveis e quatro variantes LLE, em nove variantes exatas. A maior parte da lógica do jogo ainda executa no interpretador. Isso é uma primeira integração compilada, não uma recompilação nativa completa do jogo.

Uma tentativa de promover reset e NMI a AOT descobriu 69 variantes elegíveis, mas falhou no teste: limite de interpretação em `$C02346`/`$C00607`, abandono de dispatch no frame 56 e BRK posteriores. Essa promoção foi retirada. `recomp/symbols.toml` mantém reset e NMI com `emit = false` até corrigir e validar a continuidade do código compilado/interpretado.

## Próximas etapas

1. Corrigir e validar a continuidade AOT/LLE antes de promover as rotinas de reset/NMI novamente.
2. Ampliar a cobertura com rotinas curtas comprovadas e perfis de execução vinculados ao hash desta ROM.
3. Comparar frames, WRAM, interrupções e áudio com uma referência independente (bsnes/Snes9x/Mesen).
4. Validar fases, bombas, colisões, bosses, saves e multiplayer local, incluindo entradas de P3–P5 no host desktop.
5. Validar configurações e fluxo completo do launcher, empacotamento e workflow antes da publicação no GitHub.
