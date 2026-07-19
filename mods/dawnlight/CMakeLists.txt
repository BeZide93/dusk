add_mod(dawnlight_mod
    FEATURES game
    SOURCES
        src/aim_hooks.cpp
        src/bossrush.cpp
        src/bossrush_hooks.cpp
        src/config.cpp
        src/enemy_scaling.cpp
        src/file_select.cpp
        src/game_modes.cpp
        src/item_integrity.cpp
        src/item_slot_hooks.cpp
        src/jump_hooks.cpp
        src/manual_shield_hooks.cpp
        src/mod.cpp
        src/save_compat.cpp
        src/ui.cpp
    MOD_JSON mod.json
    RES_DIR res
    OUTPUT_DIR "${CMAKE_BINARY_DIR}/mods"
)

target_include_directories(dawnlight_mod PRIVATE src)
target_link_libraries(dawnlight_mod PRIVATE nlohmann_json::nlohmann_json)
