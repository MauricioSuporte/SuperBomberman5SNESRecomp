# Lets the staged mod catalog apply to the ROM this build was generated from. Whether
# each mod then runs is decided from the ROM's contents (src/rom_probe.c). Manifests
# in the source tree keep naming only the supported ROM.
#
#   cmake -DMODULE=<src/gen/module_v2.c> -DIDENTITY=<rom_identity.txt>
#         -DCATALOG=<staged packages dir> -P stage-mod-targets.cmake
if(NOT EXISTS "${MODULE}")
    return()  # Setup host: no generated code, no game to apply mods to.
endif()
file(READ "${MODULE}" _module)
if(NOT _module MATCHES "digest\\[32\\] = {([^}]*)}")
    message(FATAL_ERROR "${MODULE}: no ROM digest found")
endif()
string(REGEX REPLACE "[^0-9a-fA-Fx]" "" _bytes "${CMAKE_MATCH_1}")
string(REPLACE "0x" "" _sha256 "${_bytes}")
string(TOLOWER "${_sha256}" _sha256)
file(READ "${IDENTITY}" _identity)
if(NOT _identity MATCHES "expected_sha256[ \t]*=[ \t]*([0-9a-fA-F]+)")
    message(FATAL_ERROR "${IDENTITY}: no expected_sha256")
endif()
string(TOLOWER "${CMAKE_MATCH_1}" _supported)
if(_sha256 STREQUAL _supported)
    return()
endif()
file(GLOB _manifests "${CATALOG}/*/*/manifest.toml")
foreach(_manifest IN LISTS _manifests)
    file(READ "${_manifest}" _text)
    if(NOT _text MATCHES "${_sha256}")
        file(APPEND "${_manifest}" "\n[[target]]\ngame_id = \"superbomberman5-jpn\"\nrom_sha256 = \"${_sha256}\"\n")
    endif()
endforeach()
