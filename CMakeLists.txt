cmake_minimum_required(VERSION 3.20)
project(SuperBomberman5SNESRecomp C CXX)

set(CMAKE_C_STANDARD 11)

# Release pin for lobby matching. Release builds stamp the VERSION file so two
# peers can prove they are the same build; everything else stamps "dev".
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _GAME_VER_RAW)
string(STRIP "${_GAME_VER_RAW}" GAME_RELEASE_VERSION)
set(SNESRECOMP_BUILD_VERSION "" CACHE STRING
    "Lobby release pin (empty = Release uses VERSION, else dev)")
if(SNESRECOMP_BUILD_VERSION STREQUAL "")
    get_property(_is_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(_is_multi)
        set(SNESRECOMP_BUILD_VERSION "$<IF:$<CONFIG:Release>,${GAME_RELEASE_VERSION},dev>")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(SNESRECOMP_BUILD_VERSION "${GAME_RELEASE_VERSION}")
    else()
        set(SNESRECOMP_BUILD_VERSION "dev")
    endif()
endif()

set(SNESRECOMP_ROOT "${CMAKE_SOURCE_DIR}/snesrecomp" CACHE PATH
    "Path to the snesrecomp checkout used by this game")
if(NOT EXISTS "${SNESRECOMP_ROOT}/runner/runner.cmake")
    message(FATAL_ERROR
        "snesrecomp submodule missing at ${SNESRECOMP_ROOT}.\n"
        "Run: git submodule update --init --recursive")
endif()
# Mod packages are part of every project, not an option. The launcher's Mods
# page, the .snesmod loader, the trusted-plugin registry and netplay's mod-set
# gate all hang off this one switch; a build without it cannot exchange mods
# with a peer that has them, and the framework says so at every lobby init.
# Must precede the include: runner.cmake reads it to add the loader sources.
set(SNESRECOMP_ENABLE_MODS ON CACHE BOOL
    "Mod package loader + trusted plugins (required for every title)" FORCE)
# The first Bomberman release is qualified against LLE composition. Keep the
# launcher's first-run rebuild on that same path unless explicitly overridden.
# Online netplay like Mega Man X Recompiled: ICE (libjuice) transport for the
# shared recomp-net lobby, plus LAN / Direct IP. Must precede the include.
set(SNESRECOMP_NET_ICE ON CACHE BOOL "Internet netplay transport" FORCE)
set(SNESRECOMP_FRAME_IMPL "LLE" CACHE STRING
    "Frame compositor validated for this game (LLE; HLE is experimental)")
include(${SNESRECOMP_ROOT}/runner/runner.cmake)

add_executable(SuperBomberman5SNESRecomp
    ${SNESRECOMP_RUNNER_SOURCES}
    src/main.c
    src/host_contract.c
    src/game_rtl.c
    src/ptbr_mod.c
    src/normal_character_mod.c
    # Register infinite-time cleanup before coop; its hook wraps the coop hook.
    # Hook reset callbacks follow this order, opposite to frame installation.
    src/portal_labels_mod.c
    src/name_tags.c
    src/normal_hud_mod.c
    src/louie_mod.c
    src/gold_battle_mod.c
    src/pause_menu_mod.c
    src/infinite_lives_mod.c
    src/infinite_time_mod.c
    src/normal_coop_mod.c
    src/coop_corner_spawns_mod.c
    src/selection_color_mod.c
    src/subordinate_mod.c
    src/extra_characters_mod.c
    src/bomber_kings_mod.c
    src/sb4_rom.c
    src/sb4_sound.c
    src/sb5_sound.c
    src/special_abilities.c
    src/sb5_launcher.c
    src/discord_presence.c
    src/gen_stubs.c
)

# ROM-independent updater shipped beside the setup host (src/updater/).
add_executable(SuperBomberman5Updater src/updater/sb5_updater.c ${SNESRECOMP_ROOT}/runner/src/sha256.c)
target_include_directories(SuperBomberman5Updater PRIVATE ${SNESRECOMP_ROOT}/runner/src)
if(NOT WIN32)
    target_compile_definitions(SuperBomberman5Updater PRIVATE _DEFAULT_SOURCE)
endif()
add_dependencies(SuperBomberman5SNESRecomp SuperBomberman5Updater)

if(WIN32)
    enable_language(RC)
    # ROM-independent Windows entry point for source and setup packages.
    add_executable(SuperBomberman5Recomp WIN32 src/windows_entry.c assets/app.rc)
    target_include_directories(SuperBomberman5Recomp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    target_sources(SuperBomberman5SNESRecomp PRIVATE assets/app.rc)
    set_source_files_properties(assets/app.rc PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico")
    target_include_directories(SuperBomberman5SNESRecomp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    # Discord invite launch registration (RegSetKeyValueW).
    target_link_libraries(SuperBomberman5SNESRecomp PRIVATE advapi32)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(PTBR_HEADER "${CMAKE_CURRENT_BINARY_DIR}/translation/ptbr_labels.h")
add_custom_command(OUTPUT "${PTBR_HEADER}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/generate-ptbr.py" --header "${PTBR_HEADER}"
    DEPENDS tools/generate-ptbr.py translations/ptbr/labels.json
    VERBATIM)
target_sources(SuperBomberman5SNESRecomp PRIVATE "${PTBR_HEADER}")
target_include_directories(SuperBomberman5SNESRecomp PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/translation")
include(CTest)
option(SB5_COOP_INPUT_FIXTURE "Test-only WRAM input mailbox for seats 3–5" OFF)
if(SB5_COOP_INPUT_FIXTURE)
    target_sources(SuperBomberman5SNESRecomp PRIVATE tests/coop_input_fixture.c tests/louie_fixture.c)
    target_compile_definitions(SuperBomberman5SNESRecomp PRIVATE SB5_COOP_INPUT_FIXTURE=1)
endif()
if(BUILD_TESTING)
    add_executable(ptbr_render_tests tests/ptbr_render_tests.c "${PTBR_HEADER}")
    target_include_directories(ptbr_render_tests PRIVATE
        "${SNESRECOMP_ROOT}/runner/src" "${CMAKE_CURRENT_BINARY_DIR}/translation")
    add_test(NAME ptbr_render_restore COMMAND ptbr_render_tests)
    add_executable(normal_character_tests tests/normal_character_tests.c)
    target_include_directories(normal_character_tests PRIVATE
        "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME normal_character_selection COMMAND normal_character_tests)
    add_executable(name_tags_tests tests/name_tags_tests.c)
    target_include_directories(name_tags_tests PRIVATE "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME netplay_name_tags COMMAND name_tags_tests)
    if(UNIX)
        # Fake Discord IPC socket; the Windows named-pipe path is the same code
        # above the transport.
        add_executable(discord_presence_tests tests/discord_presence_tests.c)
        target_include_directories(discord_presence_tests PRIVATE
            "${SNESRECOMP_ROOT}/runner/src" "${SNESRECOMP_ROOT}/runner/src/netplay"
            "${SNESRECOMP_ROOT}/lib/recomp-net/include" "${CMAKE_SOURCE_DIR}/recomp-ui/src")
        add_test(NAME discord_presence COMMAND discord_presence_tests)
    endif()
    add_executable(ingame_chat_tests tests/ingame_chat_tests.c)
    target_include_directories(ingame_chat_tests PRIVATE
        "${SNESRECOMP_ROOT}/runner/src" "${CMAKE_SOURCE_DIR}/recomp-ui/src")
    if(UNIX)
        target_link_libraries(ingame_chat_tests PRIVATE m)
    endif()
    add_test(NAME ingame_chat COMMAND ingame_chat_tests
        "${CMAKE_SOURCE_DIR}/recomp-ui/assets/common/fonts/LatoLatin-Bold.ttf")
    add_executable(portal_labels_tests tests/portal_labels_tests.c)
    target_include_directories(portal_labels_tests PRIVATE "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME normal_portal_labels COMMAND portal_labels_tests)
    add_executable(louie_tests tests/louie_tests.c)
    target_include_directories(louie_tests PRIVATE "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME louie_improvements COMMAND louie_tests)
    add_executable(gold_battle_tests tests/gold_battle_tests.c)
    target_include_directories(gold_battle_tests PRIVATE "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME gold_battle_rules COMMAND gold_battle_tests)
    add_executable(pause_menu_tests tests/pause_menu_tests.c)
    target_include_directories(pause_menu_tests PRIVATE "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME pause_menu_actions COMMAND pause_menu_tests)
    add_executable(normal_hud_tests tests/normal_hud_tests.c)
    target_include_directories(normal_hud_tests PRIVATE "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME normal_hud_faces COMMAND normal_hud_tests)
    add_executable(infinite_time_tests tests/infinite_time_tests.c)
    target_include_directories(infinite_time_tests PRIVATE "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME normal_infinite_time COMMAND infinite_time_tests)
    add_executable(infinite_lives_tests tests/infinite_lives_tests.c)
    target_include_directories(infinite_lives_tests PRIVATE "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME normal_infinite_lives COMMAND infinite_lives_tests)
    add_executable(updater_tests tests/updater_tests.c ${SNESRECOMP_ROOT}/runner/src/sha256.c)
    target_include_directories(updater_tests PRIVATE "${SNESRECOMP_ROOT}/runner/src")
    if(NOT WIN32)
        target_compile_definitions(updater_tests PRIVATE _DEFAULT_SOURCE)
    endif()
    add_test(NAME updater COMMAND updater_tests)
    add_executable(normal_coop_tests tests/normal_coop_tests.c)
    target_include_directories(normal_coop_tests PRIVATE "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME normal_five_player_coop COMMAND normal_coop_tests)
    add_executable(selection_color_tests tests/selection_color_tests.c)
    target_include_directories(selection_color_tests PRIVATE
        "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME selection_color_controls COMMAND selection_color_tests)
    add_executable(subordinate_tests tests/subordinate_tests.c)
    target_include_directories(subordinate_tests PRIVATE
        "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME subordinate_bomber COMMAND subordinate_tests)
    add_executable(extra_characters_tests tests/extra_characters_tests.c)
    target_include_directories(extra_characters_tests PRIVATE
        "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME extra_characters COMMAND extra_characters_tests)
    add_executable(special_abilities_tests tests/special_abilities_tests.c)
    target_include_directories(special_abilities_tests PRIVATE
        "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME special_abilities COMMAND special_abilities_tests)
    add_executable(sb4_rom_tests tests/sb4_rom_tests.c)
    target_include_directories(sb4_rom_tests PRIVATE
        "${SNESRECOMP_ROOT}/runner/src")
    add_test(NAME sb4_rom COMMAND sb4_rom_tests)
endif()

# ROM identity. rom_identity.txt is the one place the digests live: the build
# turns it into snesrecomp_rom_identity.h (SNESRECOMP_ROM_EXPECTED_SHA256 and
# friends) for src/main.c, and tools/regen.sh, scripts/package_release.sh and
# the release workflow read the same file. A revision bump is one edit there.
snesrecomp_rom_identity(SuperBomberman5SNESRecomp ${CMAKE_SOURCE_DIR}/rom_identity.txt)

# Generated recompiler output. Not committed: it is derived from a copyrighted
# ROM, so every developer regenerates it locally from their own copy -- and a
# release does not ship it either. With -DSNESRECOMP_SETUP_HOST=ON and an empty
# src/gen/, this builds the SETUP HOST instead: the same executable minus the
# recompiled code, whose only path is the launcher's Generate & rebuild wizard.
# That is what release zips contain; the player's machine produces the
# playable binary from their own ROM. Without the option an empty src/gen/ is
# still a hard error (the framework owns the message).
snesrecomp_target_generated_code(SuperBomberman5SNESRecomp ${CMAKE_SOURCE_DIR}/src/gen)

# The mod catalog. This names WHERE the packages are; the framework owns the
# layout they are staged into beside the executable, and stages them on every
# build OF THE EXECUTABLE -- including the launcher's own Generate & rebuild on
# a player's machine, which builds only this target and would skip a sibling
# ALL target. mods/preloaded/packages/ holds reviewed, default-disabled
# packages; players install .snesmod archives through the launcher.
# snesrecomp/docs/MOD_PACKAGES.md.
#
# It used to be `snesrecomp_target_stage_dir(... ${CMAKE_SOURCE_DIR}/mods mods)`
# -- the destination spelled per title, which is how three ports in the same
# workspace ended up shipping an empty Mods page and nothing said so. Declaring
# it is now also enforced: a project with packages that never calls this fails
# to configure.
snesrecomp_target_mod_catalog(SuperBomberman5SNESRecomp ${CMAKE_SOURCE_DIR}/mods/preloaded)
# Staging only copies, so a reused build directory still listed packages that
# v0.13.0 merged into Extra Characters (Honey/Kotetsu, Subordinate Bomber).
add_custom_command(TARGET SuperBomberman5SNESRecomp POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        "-DSNES_MOD_DIR=$<TARGET_FILE_DIR:SuperBomberman5SNESRecomp>/${SNESRECOMP_MOD_CATALOG_DEST}/.."
        -P "${CMAKE_SOURCE_DIR}/tools/prune-retired-mods.cmake"
    COMMENT "SuperBomberman5SNESRecomp: removing retired mod packages"
    VERBATIM)
# Any other directory the game reads exe-relative (a translations/ table, ...)
# is staged with snesrecomp_target_stage_dir, never with a separate ALL target.

target_include_directories(SuperBomberman5SNESRecomp PRIVATE
    ${SNESRECOMP_RUNNER_INCLUDE_DIRS}
    # common_rtl.c includes "config.h": the desktop key-map / Config contract
    # the framework host supplies (snesrecomp_target_desktop_host below).
    ${SNESRECOMP_ROOT}/runner/src/desktop
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/recomp
)
target_link_libraries(SuperBomberman5SNESRecomp PRIVATE ${SNESRECOMP_RUNNER_LIBRARIES})
target_compile_definitions(SuperBomberman5SNESRecomp PRIVATE
    SNES_GAME_VERSION="${SNESRECOMP_BUILD_VERSION}"
    SNES_GAME_TITLE="Super Bomberman 5"
    # Release ZIPs ship this setup host as SuperBomberman5Recomp.exe; rebuilds
    # still target and relaunch the game under the target's own name.
    SNESRECOMP_GAME_TARGET="SuperBomberman5SNESRecomp"
)
snesrecomp_target_sdl(SuperBomberman5SNESRecomp)

# The desktop host: launcher flow, ROM resolution, config.ini + [KeyMap],
# window + SDL/OpenGL presenters, audio, gamepads, the save-state browser and
# rewind filmstrip, the pacing clock, crash handlers and the post-mortem
# report. src/main.c is the shim that names this title and supplies its
# hooks; the host is the framework's and improves on a submodule pull.
# Add TIER2 once this port reads the interpreter-gap manifest after each run.
snesrecomp_target_desktop_host(SuperBomberman5SNESRecomp)
# rom.cfg (the ROM the wizard set this project up from; git-ignored) is
# staged beside the executable when none is there yet, so the first launch
# opens on that dump rather than asking for one.
snesrecomp_target_rom_cache(SuperBomberman5SNESRecomp)

# Shared Dear ImGui pre-boot launcher (ROM picker, verification, display /
# audio / input settings). Self-contained: it brings its own ImGui, GL link and
# staged assets, and defines RECOMP_LAUNCHER so src/main.c compiles its GUI path.
set(RECOMP_UI_ROOT "${CMAKE_SOURCE_DIR}/recomp-ui" CACHE PATH
    "Root directory of the recomp-ui launcher repo" FORCE)
if(NOT EXISTS "${RECOMP_UI_ROOT}/recomp_ui.cmake")
    message(FATAL_ERROR
        "recomp-ui missing at ${RECOMP_UI_ROOT}.\n"
        "Run: git submodule update --init --recursive recomp-ui")
endif()
include(${RECOMP_UI_ROOT}/recomp_ui.cmake)
# BOXART stages launcher_assets/img/boxart.tga beside the exe; the launcher
# shows assets/img/boxart.tga by default. EXISTS-guarded in recomp_ui.cmake,
# so this configures cleanly before any art is fetched.
recomp_target_launcher_ui(SuperBomberman5SNESRecomp CONSOLE snes
    BOXART "${CMAKE_SOURCE_DIR}/launcher_assets/img/boxart.tga")
# Netplay (rollback, up to five seats on the multitap) after the launcher:
# the host lobby adapter uses recomp-ui's shared netplay backend.
snesrecomp_enable_recomp_net(SuperBomberman5SNESRecomp)

# Generate & rebuild from inside the launcher: pick ROM -> recompile locally
# -> cmake --build -> relaunch. The implementation is the framework's; this
# project contributes no configuration (snesrecomp_codegen_host_autowire in
# src/main.c). recomp-ui/src is on the include path because the host speaks
# RecompLauncherCGameInfo.
target_sources(SuperBomberman5SNESRecomp PRIVATE
    ${SNESRECOMP_ROOT}/host/snesrecomp_codegen_host.c)
target_include_directories(SuperBomberman5SNESRecomp PRIVATE
    ${SNESRECOMP_ROOT}/host
    ${RECOMP_UI_ROOT}/src)

# Netplay is not built into this target.
# Seats: 5. Call RtlSetMultitap() at startup, or launch with
# SNES_MULTITAP=port2. See snesrecomp/docs/MULTITAP.md.
