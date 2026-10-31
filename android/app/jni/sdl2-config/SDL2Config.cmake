# SDL2 is built from source in this tree (add_subdirectory(SDL) in the parent
# CMakeLists). This config only makes find_package(SDL2) -- used by the
# snesrecomp runner and recomp-ui -- resolve to those in-tree targets.
if(NOT TARGET SDL2::SDL2)
    message(FATAL_ERROR "SDL2::SDL2 must be created by add_subdirectory(SDL) first")
endif()
set(SDL2_FOUND TRUE)
get_target_property(SDL2_INCLUDE_DIRS SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
set(SDL2_LIBRARIES SDL2::SDL2)
