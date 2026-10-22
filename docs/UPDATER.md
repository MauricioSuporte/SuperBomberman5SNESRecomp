# Updater / Atualizador

## Português

Desde a v2.0.0 cada pacote traz `updater/SuperBomberman5Updater(.exe)`. No launcher, o botão
**Update game / Atualizar jogo** (abaixo de *Browse For ROM*) fecha o jogo e abre o atualizador; ele também
pode ser aberto direto pela pasta `updater/`.

O atualizador:

1. lê a última release em <https://github.com/MauricioSuporte/SuperBomberman5SNESRecomp/releases>
   (API do GitHub) e compara com o arquivo `VERSION` da instalação;
2. baixa o pacote da plataforma e o `SHA256SUMS` da mesma release e só continua se o SHA-256 bater;
3. **Windows:** extrai o ZIP (com o `tar` do Windows) sobre a pasta, **sem tocar** nas ROMs, `rom.cfg`,
   `config.ini`, `keybinds.ini`, `saves/`, `build/`, `src/gen/`, `recomp/funcs.h` e na seleção de mods
   (`mods/preloaded/state.toml`); **Linux:** substitui o AppImage (`$APPIMAGE`), e o AppRun leva `rom.cfg`,
   configurações, saves e a seleção de mods para a pasta da nova versão;
4. abre o launcher, que pede **um Generate & rebuild** para compilar a nova versão a partir da sua ROM.

Não baixa ROM nem dados do jogo. Recusa pastas que são checkout Git (use `git pull`) e instalações do Quiver
Launcher (`installed-build.txt`: atualize pelo Quiver). `--check` só informa (`0` atualizado, `10` há
atualização, `1` sem conexão). Registro em `updater.log`.

O netplay exige a mesma versão em todos os jogadores, por isso o launcher lembra de atualizar.

**Validação:** teste nativo (`updater_tests`: JSON da release, versões, `SHA256SUMS`, arquivos do jogador) e
atualização real no Linux de um AppImage falso para a v1.1.2 publicada, com checksum conferido. A extração
no Windows compila no CI, mas ainda não foi executada numa instalação real.

## English

Each package ships `updater/SuperBomberman5Updater(.exe)`; the launcher's **Update game** button closes the
game and starts it. It reads the latest GitHub release, compares with `VERSION`, downloads the platform
package and `SHA256SUMS`, verifies the digest, then on Windows unpacks the ZIP over the folder without
touching ROMs, `rom.cfg`, config, saves, `build/`, generated sources or the Mods selection, and on Linux
replaces the AppImage (AppRun carries `rom.cfg`, config, saves and the Mods selection into the new
version folder). The launcher then asks for one **Generate & rebuild**. It never downloads a ROM, refuses
Git checkouts and defers to Quiver Launcher installs. Netplay needs the same version on every player.
Validated by a native test and a real Linux AppImage update; the Windows extraction is built in CI but not
yet run on a real install.
