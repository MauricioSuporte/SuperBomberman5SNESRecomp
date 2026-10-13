# Super Bomberman 5: estado inicial

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

Repositório Git local com SNESRecomp e recomp-ui como submódulos fixados em commits, CMake, configuração inicial de análise, launcher, scripts de geração e empacotamento e workflow oficial. Cinco assentos com multitap na porta 2 estão configurados; multiplayer ainda não foi validado. Netplay está desabilitado.

A ROM, fontes gerados, ferramentas locais e builds são ignorados pelo Git. Nenhum repositório remoto deste jogo foi criado e nada foi publicado.

## Validação realizada

Probe de cabeçalho e checksum, verificação de CRC32/SHA-256 pelo CLI e conferência dos submódulos após mover o scaffold para a pasta de trabalho.

## Próximas etapas

1. Disponibilizar CMake, Ninja, compilador C/C++, Rust/Cargo e SDL3 compatível. Git e Python 3.12 já estão acessíveis. Os demais não foram encontrados no PATH durante a preparação.
2. Executar `bash tools/regen.sh` no Git Bash para gerar os fontes locais a partir da ROM verificada. O analisador atual exige seu backend nativo.
3. Configurar e compilar com CMake.
4. Validar e ajustar `src/game_rtl.c`: reset, frames, NMI/IRQ e execução híbrida.
5. Comparar abertura, menus, áudio, partida e multiplayer com uma referência independente; ampliar a análise em `recomp/` conforme os resultados.

Não há executável compilado, geração de C concluída ou inicialização do jogo comprovada nesta etapa. O scaffold é o ponto de partida do port.
