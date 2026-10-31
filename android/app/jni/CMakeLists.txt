cmake_minimum_required(VERSION 3.20)

project(SuperBomberman5RecompAndroid C CXX)

# SDL2 source lives in ./SDL (pinned copy fetched by android/fetch_sdl.sh; it
# is not committed). Builds libSDL2.so and exports the SDL2::SDL2 target the
# game links against.
add_subdirectory(SDL)
set(SDL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdl2-config" CACHE PATH "" FORCE)

# The game itself: the repo-root CMakeLists builds the runner, the mods and the
# host as libmain.so when ANDROID is set (SDLActivity loads "libmain.so" and
# calls its SDL_main). Android is always interpreter-only (SB5_INTERPRETER_ONLY):
# the APK carries no code generated from the ROM.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../.. sb5)
