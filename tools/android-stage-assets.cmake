# Copies the data files the desktop build stages beside the executable (mod
# catalog, launcher fonts/images) into the APK asset folder, plus
# rom_identity.txt (it marks the game root), and writes sb5/files.txt -- the
# list src/android_assets.c extracts into the app's storage on first launch.
#
#   cmake -DSRC=<dir beside libmain.so> -DDST=<gradle asset dir>
#         -DROM_IDENTITY=<rom_identity.txt> -DVERSION=<game version>
#         -P android-stage-assets.cmake
#
# Nothing here comes from the ROM: these are the same files a release zip
# carries.
foreach(_v SRC DST ROM_IDENTITY VERSION)
    if(NOT DEFINED ${_v})
        message(FATAL_ERROR "android-stage-assets: -D${_v}= is required")
    endif()
endforeach()

set(_out "${DST}/sb5")
file(REMOVE_RECURSE "${_out}")
file(MAKE_DIRECTORY "${_out}")
foreach(_dir mods assets)
    if(IS_DIRECTORY "${SRC}/${_dir}")
        file(COPY "${SRC}/${_dir}" DESTINATION "${_out}")
    endif()
endforeach()
file(COPY "${ROM_IDENTITY}" DESTINATION "${_out}")

file(GLOB_RECURSE _files LIST_DIRECTORIES false RELATIVE "${_out}" "${_out}/*")
list(SORT _files)
set(_list "version ${VERSION}\n")
foreach(_f IN LISTS _files)
    string(APPEND _list "${_f}\n")
endforeach()
file(WRITE "${_out}/files.txt" "${_list}")
list(LENGTH _files _n)
message(STATUS "android-stage-assets: ${_n} files -> ${_out}")
