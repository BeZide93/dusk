#pragma once

#include "config.hpp"

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
    if (wii_u_hud_enabled()) {
        return {
            .offset_x = -100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_dpad_transform() {
    if (wii_u_hud_enabled()) {
        return {
            .offset_x = 0.0f,
            .offset_y = -15.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_midna_transform() {
    return {};
}

inline DuskModHudButtonLayout hud_layout_z_button_layout() {
    if (wii_u_hud_enabled()) {
        return {
            .item_scale = 1.0f,
            .item_offset_x = 0.0f,
            .item_offset_y = 0.0f,
            .ammo_scale = 1.0f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = 0.0f,
            .text_scale = 1.0f,
        };
    }
    return {};
}

}  // namespace dawnlight
