# Android (experimental) / Android (experimental)

Branch `android`. Not part of any release yet.

## English

### What it is

An arm64 APK of the same game, mods and netplay code as the PC build, built from `android/` (Gradle + NDK,
SDL2, based on SuperMetroidRecomp's Android project). The game is `libmain.so`, loaded by SDL's `SDLActivity`.

### Interpreter only

A phone cannot run the launcher's Generate & rebuild (no compiler, no Rust analyzer), and an APK must never carry
`src/gen` (it is derived from the ROM). The Android build therefore sets `SB5_INTERPRETER_ONLY=ON`: instead of
generated code it links empty dispatch tables (`src/interp_only/interp_only_dispatch.c`), so every routine runs on
the framework's 65816 interpreter. On PC only about 9 routine variants are AOT; everything else, reset and NMI
included, already runs on the interpreter, so the behaviour is the same code path the mods are written against
(the interpreter PC hooks fire for every instruction). Performance on phones is not measured yet.

The option also works on desktop for testing: `cmake -S . -B build-io -DSB5_INTERPRETER_ONLY=ON` (no `src/gen`,
not combined with `SNESRECOMP_SETUP_HOST`).

### Files on the phone

The game folder is the app's external files folder, `Android/data/io.github.mauriciosuporte.sb5recomp/files`. On
first launch (and after each APK update) `src/android_assets.c` copies the bundled mod catalog, launcher fonts and
images and `rom_identity.txt` there; `config.ini`, `rom.cfg`, saves, mod state and ROMs are never overwritten.
Copy your own ROM (and, for Bomber Kings, your Super Bomberman 4 ROM) into that folder with a file manager or
`adb push`; the game finds ROMs by SHA-256 in the game folder and in `roms/`.

The runner anchors its game folder to `$APPIMAGE` when set; `Sb5AndroidPrepare` points it inside the files folder,
so the framework needs no Android-specific path code.

### Build

CI: `.github/workflows/android.yml` builds `superbomberman5-<version>-android-arm64-experimental.apk` on every push to
`android` (artifact, not a release). It also checks the APK lists no ROM, save or generated file.

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

### Not done yet / limits

- No runtime evidence on a phone yet: build only.
- Touch controls (Bluetooth/USB controllers work through SDL), ROM picker through Android's file chooser, the
  Update button (should open the releases page), Discord presence.
- Netplay: the code is in the APK (LAN, Direct IP, ICE). Cross-play with PC needs the same version, mods and ROM
  hashes, and must be checked with `tests/netplay_pair_runtime.py`-style hash comparison between an interpreter-only
  peer and an AOT PC peer; a slow phone as guest hits the same catch-up limits as a slow PC (docs/NETPLAY.md).
- Signed with the debug key; a release key belongs in a CI secret.

## Português

### O que é

Um APK arm64 com o mesmo jogo, mods e netplay da versão de PC, gerado por `android/` (Gradle + NDK, SDL2, baseado no
projeto Android do SuperMetroidRecomp). O jogo é a `libmain.so`, carregada pelo `SDLActivity` do SDL.

### Só interpretador

O celular não consegue rodar o "Generate & rebuild" do launcher (sem compilador nem analisador Rust), e o APK nunca
pode levar `src/gen` (é derivado da ROM). Por isso o Android usa `SB5_INTERPRETER_ONLY=ON`: no lugar do código
gerado entram tabelas de dispatch vazias (`src/interp_only/interp_only_dispatch.c`) e todas as rotinas rodam no
interpretador 65816 do framework. No PC só ~9 variantes de rotina são AOT; o resto, inclusive reset e NMI, já roda
no interpretador, então é o mesmo caminho para o qual os mods foram escritos. O desempenho em celulares ainda não
foi medido.

A opção também funciona no PC para testes: `cmake -S . -B build-io -DSB5_INTERPRETER_ONLY=ON` (sem `src/gen` e sem
`SNESRECOMP_SETUP_HOST`).

### Arquivos no celular

A pasta do jogo é `Android/data/io.github.mauriciosuporte.sb5recomp/files`. Na primeira abertura (e depois de cada
atualização do APK) o `src/android_assets.c` copia para lá o catálogo de mods, fontes e imagens do launcher e o
`rom_identity.txt`; `config.ini`, `rom.cfg`, saves, estado dos mods e ROMs nunca são sobrescritos. Copie a sua ROM
(e, para os Bomber Kings, a ROM do Super Bomberman 4) para essa pasta com um gerenciador de arquivos ou `adb push`;
o jogo acha as ROMs pelo SHA-256 na pasta do jogo e em `roms/`.

### Build

CI: `.github/workflows/android.yml` gera `superbomberman5-<versão>-android-arm64-experimental.apk` a cada push no
branch `android` (artefato, não release) e confere que o APK não lista ROM, save nem código gerado. Build local:
comandos na seção em inglês.

### Falta / limites

- Ainda sem teste rodando num celular: só compilação.
- Controles de toque (controles Bluetooth/USB funcionam pelo SDL), seletor de ROM do Android, botão Atualizar
  (deve abrir a página de releases), Discord.
- Netplay: o código está no APK (LAN, IP direto, ICE). Crossplay com PC exige mesma versão, mods e hashes de ROM e
  precisa ser conferido comparando hashes entre um peer só-interpretador e um peer de PC com AOT; celular lento como
  convidado sofre os mesmos limites de recuperação de um PC lento (docs/NETPLAY.md).
- Assinado com a chave de debug; a chave de release deve ficar num secret do CI.
