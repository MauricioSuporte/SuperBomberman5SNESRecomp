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

Desde a v2.3.1 a janela mostra o andamento: o download tem barra, porcentagem, MB baixados, velocidade e tempo
estimado (`ETA`), calculados pelo tamanho do pacote informado pela release; a extração mostra o tempo
decorrido; a instalação tem barra com os arquivos copiados e o tempo estimado. Cada etapa termina com uma linha
`done in m:ss`, que também vai para o `updater.log`. Fora de um console (saída redirecionada) só essas linhas
finais são escritas. A barra só aparece a partir da atualização seguinte à instalação da v2.3.1, porque é o
atualizador já instalado que baixa a versão nova.

Os arquivos temporários (download, pacote extraído e a cópia do próprio atualizador, em
`%TEMP%\sb5-update-*` / `$TMPDIR/sb5-update-*`) são apagados ao terminar, e sobras de execuções antigas são
apagadas ao começar (desde a v2.0.2). Não baixa ROM nem dados do jogo. Recusa pastas que são checkout Git (use `git pull`) e instalações do Quiver
Launcher (`installed-build.txt`: atualize pelo Quiver). `--check` só informa (`0` atualizado, `10` há
atualização, `1` sem conexão). Registro em `updater.log`.

O netplay exige a mesma versão em todos os jogadores. Desde a v2.0.3 o launcher mostra a versão instalada no
cabeçalho e roda `--check` em segundo plano ao abrir: com versão nova, avisa acima do botão, explica nas telas de
Netplay por que as salas dos outros não aparecem e desativa o **Host Lobby** até atualizar
(`SB5_NO_UPDATE_CHECK=1` desliga a consulta).

**Validação:** teste nativo (`updater_tests`: JSON da release com tamanhos, versões, `SHA256SUMS`, arquivos do
jogador, formatação da barra/ETA); download real do AppImage da v2.3.0 com a barra num terminal Linux; e
atualização real no Linux de um AppImage falso para a v1.1.2 publicada, com checksum conferido. A extração
no Windows compila no CI, mas ainda não foi executada numa instalação real.

## English

Each package ships `updater/SuperBomberman5Updater(.exe)`; the launcher's **Update game** button closes the
game and starts it. It reads the latest GitHub release, compares with `VERSION`, downloads the platform
package and `SHA256SUMS`, verifies the digest, then on Windows unpacks the ZIP over the folder without
touching ROMs, `rom.cfg`, config, saves, `build/`, generated sources or the Mods selection, and on Linux
replaces the AppImage (AppRun carries `rom.cfg`, config, saves and the Mods selection into the new
version folder). The launcher then asks for one **Generate & rebuild**. Since v2.3.1 the window shows progress: a
download bar with percentage, MB, speed and estimated time left (from the asset size in the release metadata),
the elapsed time while unpacking, and an install bar counting copied files with its own estimate; each step ends
with a `done in m:ss` line, also written to `updater.log` (only those lines when the output is not a console).
The installed updater downloads the next version, so the bars appear from the update after v2.3.1. Its temporary files (`sb5-update-*` in the temp folder) are deleted when it finishes, and leftovers of
earlier runs when it starts (since v2.0.2). It never downloads a ROM, refuses
Git checkouts and defers to Quiver Launcher installs. Netplay needs the same version on every player; since v2.0.3 the launcher shows the installed version, checks
GitHub in the background (`--check`), warns when a newer release exists and disables Host Lobby until you update
(`SB5_NO_UPDATE_CHECK=1` turns the check off).
Validated by a native test and a real Linux AppImage update; the Windows extraction is built in CI but not
yet run on a real install.
