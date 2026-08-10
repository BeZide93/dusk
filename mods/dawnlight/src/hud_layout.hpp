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
    int default_item_anchor = 0;
    int text_anchor = 0;
    std::uint32_t style_flags = 0;
};

constexpr int kHudItemAnchorLeft = 0;
constexpr int kHudItemAnchorRight = 1;
constexpr int kHudItemAnchorTop = 2;
constexpr int kHudItemAnchorBottom = 3;
constexpr int kHudTextAnchorLeft = 0;
constexpr int kHudTextAnchorRight = 1;

inline DuskModHudTransform hud_layout_a_transform() {
    if (wii_u_hud_enabled()) {
        return {
            .offset_x = -135.0f,
            .offset_y = 25.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudButtonLayout hud_layout_a_button_layout() {
    if (wii_u_hud_enabled()) {
        return {
            .text_scale = 1.0f,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_b_transform() {
    if (wii_u_hud_enabled()) {
        return {
            .offset_x = -80.0f,
            .offset_y = -27.0f,
            .scale = 1.5f,
        };
    }
    return {};
}

inline DuskModHudButtonLayout hud_layout_b_button_layout() {
    if (wii_u_hud_enabled()) {
        return {
            .item_scale = 0.5f,
            .item_offset_x = 30.0f,
            .item_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 10.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorTop,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_x_transform() {
    if (wii_u_hud_enabled()) {
        return {
            .offset_x = -202.0f,
            .offset_y = -1.0f,
            .scale = 1.7000000476837158f,
        };
    }
    return {};
}

inline DuskModHudButtonLayout hud_layout_x_button_layout() {
    if (wii_u_hud_enabled()) {
        return {
            .item_scale = 0.5f,
            .item_offset_x = 0.0f,
            .item_offset_y = 0.0f,
            .ammo_scale = 1.0f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = -15.0f,
            .text_offset_y = -15.0f,
            .item_anchor = kHudItemAnchorLeft,
            .default_item_anchor = kHudItemAnchorRight,
            .text_anchor = kHudTextAnchorLeft,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_y_transform() {
    if (wii_u_hud_enabled()) {
        return {
            .offset_x = -122.0f,
            .offset_y = 0.0f,
            .scale = 1.7000000476837158f,
        };
    }
    return {};
}

inline DuskModHudButtonLayout hud_layout_y_button_layout() {
    if (wii_u_hud_enabled()) {
        return {
            .item_scale = 0.5f,
            .item_offset_x = 0.0f,
            .item_offset_y = 0.0f,
            .ammo_scale = 1.0f,
            .ammo_offset_x = 0.0f,
            .ammo_offset_y = 0.0f,
            .text_scale = 0.5f,
            .text_offset_x = 15.0f,
            .text_offset_y = 0.0f,
            .item_anchor = kHudItemAnchorTop,
            .default_item_anchor = kHudItemAnchorLeft,
            .text_anchor = kHudTextAnchorLeft,
        };
    }
    return {};
}

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
            .item_anchor = kHudItemAnchorRight,
            .default_item_anchor = kHudItemAnchorRight,
        };
    }
    return {};
}

}  // namespace dawnlight
