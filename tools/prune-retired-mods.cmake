# Removes mod packages that later releases merged into another package from a
# staged catalog (build tree or installed copy), and their entries in the
# launcher's state.toml. The framework's catalog staging only copies packages,
# so a build directory reused across releases kept listing them.
#
#   cmake -DSNES_MOD_DIR=<dir with packages/ and state.toml> -P prune-retired-mods.cmake
#
# superbomberman5.honey-kotetsu and superbomberman5.subordinate-bomber became
# superbomberman5.extra-characters in v0.13.0. When either was enabled and the
# state has no entry for Extra Characters yet, that package is enabled instead.
set(_retired
    superbomberman5.honey-kotetsu
    superbomberman5.subordinate-bomber)

if(NOT SNES_MOD_DIR)
    message(FATAL_ERROR "prune-retired-mods: set SNES_MOD_DIR")
endif()

foreach(_id IN LISTS _retired)
    if(IS_DIRECTORY "${SNES_MOD_DIR}/packages/${_id}")
        file(REMOVE_RECURSE "${SNES_MOD_DIR}/packages/${_id}")
        message(STATUS "Removed retired mod package ${_id} (now part of superbomberman5.extra-characters)")
    endif()
endforeach()

set(_state "${SNES_MOD_DIR}/state.toml")
if(NOT EXISTS "${_state}")
    return()
endif()
file(READ "${_state}" _text)
# Split into blocks that each start with a [[table]] header; the text before
# the first header is kept as is. Blocks naming a retired package go.
string(REPLACE ";" "\\;" _text "${_text}")
string(REGEX REPLACE "\n\\[\\[" "\n;[[" _text "${_text}")
set(_out "")
set(_changed FALSE)
set(_was_enabled FALSE)
foreach(_block IN LISTS _text)
    set(_drop FALSE)
    foreach(_id IN LISTS _retired)
        string(FIND "${_block}" "\"${_id}\"" _at)
        if(_block MATCHES "^\\[\\[" AND NOT _at EQUAL -1)
            set(_drop TRUE)
        endif()
    endforeach()
    if(_drop)
        set(_changed TRUE)
        if(_block MATCHES "^\\[\\[feature\\]\\]" AND _block MATCHES "enabled *= *true")
            set(_was_enabled TRUE)
        endif()
    else()
        string(APPEND _out "${_block}")
    endif()
endforeach()
# A player who had either retired mod on keeps those characters: turn on the
# package that replaced them, unless the state already says something about it.
string(FIND "${_out}" "\"superbomberman5.extra-characters\"" _has_extra)
if(_was_enabled AND _has_extra EQUAL -1)
    string(REGEX REPLACE "[\r\n]+$" "" _out "${_out}")
    string(APPEND _out "\n\n[[package]]\nid = \"superbomberman5.extra-characters\"\nversion = \"0.1.0\"\n"
        "\n[[feature]]\npackage_id = \"superbomberman5.extra-characters\"\nid = \"extra_characters\"\nenabled = true\n")
    message(STATUS "Enabled superbomberman5.extra-characters, which replaces them")
endif()
if(_changed)
    string(REPLACE "\\;" ";" _out "${_out}")
    file(WRITE "${_state}" "${_out}")
    message(STATUS "Removed retired mod entries from ${_state}")
endif()
