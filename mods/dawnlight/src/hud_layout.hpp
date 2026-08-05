#pragma once

#include <cstdint>

namespace dawnlight {

struct DuskModHudTransform {
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    float scale = 1.0f;
    std::uint32_t flags = 0;
    int parent_mode = 0;
    int slide_direction = 0;
};

struct DuskModHudButtonLayout {
    float item_scale = 1.0f;
    float item_offset_x = 0.0f;
    float item_offset_y = 0.0f;
    float ammo_scale = 1.0f;
    float ammo_offset_x = 0.0f;
    float ammo_offset_y = 0.0f;
    float text_scale = 1.0f;
    float text_offset_x = 0.0f;
    float text_offset_y = 0.0f;
    int item_anchor = 0;
    int text_anchor = 0;
    std::uint32_t style_flags = 0;
};

inline DuskModHudTransform hud_layout_z_transform() {
    return {};
}

inline DuskModHudTransform hud_layout_dpad_transform() {
    return {};
}

inline DuskModHudTransform hud_layout_midna_transform() {
    return {};
}

inline DuskModHudButtonLayout hud_layout_z_button_layout() {
    return {};
}

}  // namespace dawnlight
