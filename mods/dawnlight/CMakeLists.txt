cmake_minimum_required(VERSION 3.26)
project(dawnlight_mod CXX)

set(DAWNLIGHT_STANDALONE OFF)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(DAWNLIGHT_STANDALONE ON)
    if (NOT DUSKLIGHT_VERSION)
        set(DUSKLIGHT_VERSION "d281c34a65568ee7be2319985f3a290c0ef65abc")
    endif ()

    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDusklight.cmake")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)
endif ()

if (DAWNLIGHT_STANDALONE)
    set(DUSK_DIR "${DUSKLIGHT_DIR}")
elseif (NOT DEFINED DUSK_DIR)
    set(DUSK_DIR "${CMAKE_SOURCE_DIR}")
endif ()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" MOD_JSON_CONTENT)
string(JSON DAWNLIGHT_VER GET "${MOD_JSON_CONTENT}" "version")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/mod.json")
set_source_files_properties(src/mod.cpp src/update_check.cpp PROPERTIES
    OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mod.json")

add_mod(dawnlight_mod
    FEATURES game
    SOURCES
        src/aim_hooks.cpp
        src/config.cpp
        src/deferred_features.cpp
        src/enemy_scaling.cpp
        src/item_integrity.cpp
        src/item_slot_hooks.cpp
        src/jump_hooks.cpp
        src/manual_shield_hooks.cpp
        src/mod.cpp
        src/new_save_modes.cpp
        src/save_compat.cpp
        src/ui.cpp
        src/update_check.cpp
    MOD_JSON mod.json
    RES_DIR res
)

if (NOT DAWNLIGHT_STANDALONE)
    target_include_directories(dawnlight_mod PRIVATE src "${DUSK_DIR}/src")
    set(DAWNLIGHT_RMLUI_INCLUDE "" CACHE PATH "Optional RmlUi include directory for Dawnlight mod builds")
    set(DAWNLIGHT_SDL3_INCLUDE "" CACHE PATH "Optional SDL3 include directory for Dawnlight mod builds")
    foreach (_dawnlight_rmlui_include
            "${DAWNLIGHT_RMLUI_INCLUDE}"
            "${CMAKE_BINARY_DIR}/_deps/rmlui-src/Include")
        if (EXISTS "${_dawnlight_rmlui_include}/RmlUi/Core.h")
            target_include_directories(dawnlight_mod PRIVATE "${_dawnlight_rmlui_include}")
        endif ()
    endforeach ()
    foreach (_dawnlight_sdl_include
            "${DAWNLIGHT_SDL3_INCLUDE}"
            "${CMAKE_BINARY_DIR}/_deps/sdl3_prebuilt-src/include"
            "${CMAKE_BINARY_DIR}/_deps/sdl-src/include")
        if (EXISTS "${_dawnlight_sdl_include}/SDL3/SDL_touch.h")
            target_include_directories(dawnlight_mod PRIVATE "${_dawnlight_sdl_include}")
        endif ()
    endforeach ()
    target_link_libraries(dawnlight_mod PRIVATE dusklight_game_headers)
endif ()
