# Compilação local no Windows

Pré-requisitos: Windows x64, Git for Windows e Python 3.11 ou superior acessíveis no PATH. O script de build usa o Git Bash em `C:/Program Files/Git/bin/bash.exe`; ajuste esse caminho se sua instalação estiver em outro diretório.

No PowerShell, a partir da raiz do projeto:

```powershell
git submodule update --init --recursive
./tools/setup-windows.ps1
./tools/build-windows.ps1 -Regenerate
./tools/build-windows.ps1 -Run
```

Se a política local bloquear scripts, invoque-os com `powershell -ExecutionPolicy Bypass -File ./tools/setup-windows.ps1` (e o mesmo prefixo para o build). Isso vale só para esse processo.

`setup-windows.ps1` baixa o pacote oficial RetroPorting Toolchains v1.0.14 para `.tools/toolchain/`, verifica o SHA-256 do arquivo e instala Rust 1.97.1 para Windows GNU LLVM em `.tools/`. Não exige administrador nem altera o PATH permanente. O pacote traz CMake 3.31.12, Ninja 1.13.2, Clang 22.1.8 e SDL3 3.4.10. Git e Python não são instalados pelo script.

`build-windows.ps1` configura essas ferramentas no ambiente do processo, verifica a ROM durante a regeneração e compila em Release. O arquivo local padrão é `Super Bomberman 5 (USA).sfc`; outra localização pode ser informada com `-Rom 'C:/roms/game.sfc'`. A identidade aceita é a registrada em `rom_identity.txt`.

A primeira build usa composição de frames LLE, para validação. O executável fica em `build-lle/SuperBomberman5SNESRecomp.exe`. `-Run` passa a ROM diretamente e inicia o jogo. Abrir o executável sem argumentos permite usar o launcher.

Depois de compilar, clique em `play.cmd` na raiz para abrir o launcher com a ROM deste diretório. As teclas padrão de P1 são setas para direção, Enter para Start, X para A e Z para B. Os controles podem ser alterados no launcher.

Para builds incrementais, omita `-Regenerate`. Use regeneração depois de alterar `recomp/`. A opção `-FrameImpl HLE` cria uma build separada em `build-hle/`; essa composição ainda não foi validada para este jogo.

Esta versão é um protótipo de integração: a cobertura AOT ainda é pequena e as rotinas de reset/NMI permanecem no interpretador. Compilar o executável não prova que o port inteiro está correto. Veja `STATUS.md`.

Windows v0.13.1 includes SuperBomberman5Recomp.exe, a ROM-independent entry with the existing “5” icon. It locates the sibling setup/game executable or the source tree build-lle executable, forwarding arguments and exit status. play.cmd remains a compatibility shortcut. The audited Windows ZIP includes this native entry.

O Windows v0.13.1 inclui SuperBomberman5Recomp.exe com o ícone atual do “5”. A entrada localiza o setup/jogo ao lado ou o build local em build-lle e repassa argumentos e código de saída. play.cmd continua como atalho de compatibilidade.
