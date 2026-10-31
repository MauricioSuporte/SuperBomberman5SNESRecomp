# Android / Android

Since v3.0.0: `superbomberman5-<version>-android.apk` (Android 9+, arm64) in every release.

## English

### What it is

An arm64 APK of the same game, mods, Boss Rush ranking and netplay code as the PC build, built from `android/`
(Gradle + NDK, SDL2, based on SuperMetroidRecomp's Android project). The game is `libmain.so`, loaded by
`SB5Activity` (SDL's `SDLActivity` plus the Android services below).

### Install and update

Open the APK on the phone and allow installing it. In the launcher, **Browse For ROM** opens the system document
picker and copies the chosen file into `files/roms/`; then **Play**. **Update game / Atualizar jogo** asks GitHub
for the newest release, downloads its `-android.apk` and installs it over the app through the system installer
(`PackageInstaller`): Android asks once to allow "install unknown apps" for the game, then shows its own update
confirmation. Saves, ROMs, settings and mods stay, because the package name and signing key do not change.

Releases are signed with the project's release key (Actions secrets `ANDROID_KEYSTORE_BASE64`,
`ANDROID_KEYSTORE_PASSWORD`; certificate SHA-256 `3ffbdae9...d321d2b4`, checked by the workflow). Test APKs built
before v3.0.0 used throwaway debug keys and cannot be updated in place: uninstall once (copy `files/saves` first).
Losing the key means no installed copy can update in place again; keep it outside the repository.

### Interpreter only

A phone cannot run the launcher's Generate & rebuild (no compiler, no Rust analyzer), and an APK must never carry
`src/gen` (it is derived from the ROM). The Android build therefore sets `SB5_INTERPRETER_ONLY=ON`: instead of
generated code it links empty dispatch tables (`src/interp_only/interp_only_dispatch.c`), so every routine runs on
the framework's 65816 interpreter. On PC only about 9 routine variants are AOT, so this is the same path the mods
are written against (the interpreter PC hooks fire for every instruction).

Speed: on the x86_64 CI container the title screen runs at about 150 frames/s with every mod on (about 15% slower
than without mods; the mods themselves are about 5% of the time, the rest is the interpreter and the PPU). Phones
are slower, so busy scenes can drop below 60 on a slow phone. `libmain.so` exports only `SDL_main` (hidden
visibility, `-fno-semantic-interposition`) so the runner's internal calls can be inlined.

The option also works on desktop for testing: `cmake -S . -B build-io -DSB5_INTERPRETER_ONLY=ON` (no `src/gen`,
not combined with `SNESRECOMP_SETUP_HOST`).

### Netplay and crossplay

The APK carries the same lobby version as the PC build of that release (no `-android` suffix), so Android,
Windows and Linux players see each other's rooms. Every netplay session selects an empty AOT program
(`Sb5NetplayInterpreterOnly`, `cpu_select_program`) on every platform: an AOT PC peer and an interpreter-only peer
desynced from frame 330 before that, and stay identical with it (see docs/STATUS.md). **Sign in with Discord**
opens the system browser (recomp-net `rnet_open_url` through `SDL_OpenURL`).

### Boss Rush ranking

Requests go through `SB5Activity.sb5Http` (HttpURLConnection, the same User-Agent and JSON bodies as the PC's
curl), called from the ranking worker thread. The release workflow bundles `boss_rush_ranking.cfg` from the
`BOSS_RUSH_*` secrets into the APK assets (never tracked).

### Files on the phone

The game folder is the app's external files folder, `Android/data/io.github.mauriciosuporte.sb5recomp/files`. On
first launch (and after each APK update) `src/android_assets.c` copies the bundled mod catalog, launcher fonts and
images, `rom_identity.txt` and the ranking configuration there; `config.ini`, `rom.cfg`, saves, mod state and ROMs
are never overwritten. A ROM copied into that folder or `roms/` by USB is found by SHA-256 too.
`joysticks.txt` there lists the input devices SDL saw at start-up.

The runner anchors its game folder to `$APPIMAGE` when set; `Sb5AndroidPrepare` points it inside the files folder,
so the framework needs no Android-specific path code.

### Touch controls

An on-screen SNES pad is drawn over the picture (framework `touch_pad.c`): in landscape the 8-way d-pad bottom
left, A/B/X/Y bottom right, L/R in the top corners and Select/Start at the bottom; in portrait the picture moves to
the top and L/R and Select/Start stack above the d-pad and buttons. Multitouch, fingers can slide between buttons.
It hides when a controller or keyboard is used and returns on the next touch. It drives player 1. Motion sensors
are never opened as controllers (`SDL_HINT_ACCELEROMETER_AS_JOYSTICK=0`, sensor joysticks refused).

### Build and CI

`.github/workflows/android.yml` builds the APK (arm64 + x86_64) on every push to `main`/`android` and, called by
`release.yml`, the release APK. It checks the APK holds no ROM, save or generated file, exports `SDL_main` and is
signed with the release key, then runs `tools/android-emulator-test.sh` on an API 34 emulator (no ROM): launcher
start-up, the system file picker and import, HTTP GET/PATCH/POST through the Activity against
`tools/android-test-server.py`, the GitHub release check, the sign-in URL handoff and an in-place update.
Screenshots and logs go to the `android-ci-shots` branch.

Local (Linux/macOS/WSL, Android SDK with NDK 27.2.12479018, JDK 17, CMake 3.24+, Python with Pillow):

```bash
git submodule update --init --recursive
bash android/fetch_sdl.sh
python android/make_icons.py
cd android
echo "cmake.dir=/path/to/cmake" > local.properties   # CMake 3.24 or newer
./gradlew assembleRelease
adb install -r app/build/outputs/apk/release/app-release.apk
```

### Limits

- Game runtime is checked on the maintainer's phone and on Linux x86_64 interpreter-only runs; CI emulator tests run
  without a ROM. Speed and compatibility across phones are not qualified.
- Discord Rich Presence (the PC's status and invites through the Discord app) is not available on Android.
- The Android launcher keeps the PC's layout, scaled to fit; touch works on it, it is not redesigned for phones.

## Português

### O que é

Um APK arm64 com o mesmo jogo, mods, ranking do Boss Rush e netplay da versão de PC, gerado por `android/` (Gradle
+ NDK, SDL2, baseado no projeto Android do SuperMetroidRecomp). O jogo é a `libmain.so`, carregada pela
`SB5Activity` (o `SDLActivity` do SDL mais os serviços do Android descritos abaixo).

### Instalar e atualizar

Abra o APK no celular e permita a instalação. No launcher, **Browse For ROM** abre o seletor de arquivos do sistema
e copia o arquivo escolhido para `files/roms/`; depois **Play**. **Update game / Atualizar jogo** consulta a release
mais nova no GitHub, baixa o `-android.apk` dela e instala por cima do app pelo instalador do sistema: o Android pede
uma vez para permitir "instalar apps desconhecidos" para o jogo e depois mostra a própria confirmação. Saves, ROMs,
configurações e mods continuam, porque o pacote e a chave de assinatura não mudam.

As releases são assinadas com a chave do projeto (secrets `ANDROID_KEYSTORE_BASE64` e `ANDROID_KEYSTORE_PASSWORD`;
certificado SHA-256 `3ffbdae9...d321d2b4`, conferido pelo workflow). APKs de teste anteriores à v3.0.0 usavam chaves
de debug descartáveis e não atualizam por cima: desinstale uma vez (copie antes `files/saves`). Se a chave se
perder, nenhuma cópia instalada consegue mais atualizar por cima; guarde-a fora do repositório.

### Só interpretador

O celular não consegue rodar o "Generate & rebuild" (sem compilador nem analisador Rust), e o APK nunca pode levar
`src/gen` (é derivado da ROM). Por isso o Android usa `SB5_INTERPRETER_ONLY=ON`: no lugar do código gerado entram
tabelas de dispatch vazias e todas as rotinas rodam no interpretador 65816. No PC só ~9 variantes de rotina são
AOT, então é o mesmo caminho para o qual os mods foram escritos.

Velocidade: no contêiner x86_64 do CI a tela de título roda a uns 150 quadros/s com todos os mods (cerca de 15% mais
lento que sem mods; os mods em si são ~5% do tempo, o resto é o interpretador e o PPU). Celulares são mais lentos,
então em cenas cheias um celular fraco pode ficar abaixo de 60. A `libmain.so` exporta só o `SDL_main` (visibilidade
oculta) para o compilador poder otimizar as chamadas internas.

### Netplay e crossplay

O APK usa a mesma versão de lobby da versão de PC da release (sem sufixo `-android`), então jogadores de Android,
Windows e Linux veem as salas uns dos outros. Toda sessão de netplay desliga o código AOT em todas as plataformas
(`Sb5NetplayInterpreterOnly`): sem isso um PC com AOT e um peer só-interpretador dessincronizavam a partir do quadro
330; com isso ficam idênticos (ver docs/STATUS.md). **Sign in with Discord** abre o navegador do sistema.

### Ranking do Boss Rush

As requisições passam pela `SB5Activity.sb5Http` (HttpURLConnection, mesmo User-Agent e corpo JSON do curl do PC). O
workflow de release coloca o `boss_rush_ranking.cfg` dos secrets `BOSS_RUSH_*` nos assets do APK (nunca versionado).

### Arquivos no celular

A pasta do jogo é `Android/data/io.github.mauriciosuporte.sb5recomp/files`. Na primeira abertura (e depois de cada
atualização do APK) o `src/android_assets.c` copia para lá o catálogo de mods, fontes e imagens do launcher, o
`rom_identity.txt` e a configuração do ranking; `config.ini`, `rom.cfg`, saves, estado dos mods e ROMs nunca são
sobrescritos. Uma ROM copiada pelo USB para essa pasta ou para `roms/` também é encontrada pelo SHA-256. O
`joysticks.txt` dessa pasta lista os dispositivos de entrada que o SDL viu ao abrir.

### Controles de toque

Um controle de SNES translúcido aparece sobre a imagem: na horizontal, direcional de 8 direções embaixo à esquerda,
A/B/X/Y embaixo à direita, L/R nos cantos de cima e Select/Start embaixo; na vertical a imagem vai para o topo e
L/R e Select/Start ficam acima do direcional e dos botões. Multitoque, dá para deslizar o dedo entre botões. Some
quando um controle ou teclado é usado e volta no próximo toque. Controla o jogador 1. Sensores de movimento nunca
são abertos como controle.

### Build e CI

O `.github/workflows/android.yml` gera o APK (arm64 + x86_64) a cada push em `main`/`android` e, chamado pelo
`release.yml`, o APK da release. Ele confere que o APK não tem ROM, save nem código gerado, exporta o `SDL_main` e
está assinado com a chave de release, e roda o `tools/android-emulator-test.sh` num emulador API 34 (sem ROM):
abertura do launcher, seletor de arquivos e importação, HTTP GET/PATCH/POST pela Activity contra o
`tools/android-test-server.py`, consulta de versão no GitHub, abertura da URL de login e atualização por cima. Prints
e logs vão para o branch `android-ci-shots`. Build local: comandos na seção em inglês.

### Limites

- O jogo rodando foi conferido no celular do mantenedor e em execuções só-interpretador no Linux x86_64; os testes de
  emulador do CI rodam sem ROM. Velocidade e compatibilidade entre celulares não foram qualificadas.
- O Rich Presence do Discord (status e convites pelo app do Discord no PC) não existe no Android.
- O launcher no Android mantém o layout do PC, reduzido para caber; o toque funciona nele, mas ele não foi
  redesenhado para celular.
