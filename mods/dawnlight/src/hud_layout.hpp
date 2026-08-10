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
constexpr int kHudParentIndependent = 1;
constexpr int kHudSlideLeftToRight = 1;
constexpr int kHudSlideRightToLeft = 2;

inline DuskModHudTransform hud_layout_a_transform() {
    switch (hud_layout()) {
    case HudLayout::WiiU:
        return {
            .offset_x = -35.0f,
            .offset_y = 25.0f,
            .scale = 1.0f,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = -135.0f,
            .offset_y = 25.0f,
            .scale = 1.0f,
        };
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudButtonLayout hud_layout_a_button_layout() {
    if (hardcoded_hud_layout_enabled()) {
        return {
            .text_scale = 1.0f,
            .text_anchor = kHudTextAnchorRight,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_b_transform() {
    switch (hud_layout()) {
    case HudLayout::WiiU:
        return {
            .offset_x = 20.0f,
            .offset_y = -27.0f,
            .scale = 1.5f,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = -80.0f,
            .offset_y = -27.0f,
            .scale = 1.5f,
        };
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudButtonLayout hud_layout_b_button_layout() {
    if (hardcoded_hud_layout_enabled()) {
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
    switch (hud_layout()) {
    case HudLayout::WiiU:
        return {
            .offset_x = -102.0f,
            .offset_y = -1.0f,
            .scale = 1.7000000476837158f,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = -202.0f,
            .offset_y = -1.0f,
            .scale = 1.7000000476837158f,
        };
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudButtonLayout hud_layout_x_button_layout() {
    switch (hud_layout()) {
    case HudLayout::WiiU:
        return {
            .item_scale = 0.5f,
            .item_offset_x = -15.0f,
            .item_offset_y = -15.0f,
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
    case HudLayout::Dawnlight:
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
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudTransform hud_layout_y_transform() {
    switch (hud_layout()) {
    case HudLayout::WiiU:
        return {
            .offset_x = -22.0f,
            .offset_y = 0.0f,
            .scale = 1.7000000476837158f,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = -122.0f,
            .offset_y = 0.0f,
            .scale = 1.7000000476837158f,
        };
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudButtonLayout hud_layout_y_button_layout() {
    if (hardcoded_hud_layout_enabled()) {
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
    switch (hud_layout()) {
    case HudLayout::WiiU:
        return {
            .offset_x = 0.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = -100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudTransform hud_layout_dpad_transform() {
    switch (hud_layout()) {
    case HudLayout::WiiU:
        return {
            .offset_x = 0.0f,
            .offset_y = -280.0f,
            .scale = 1.0f,
            .parent_mode = kHudParentIndependent,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = 0.0f,
            .offset_y = -15.0f,
            .scale = 1.0f,
            .parent_mode = kHudParentIndependent,
        };
    case HudLayout::GameCube:
    default:
        return {};
    }
}

inline DuskModHudTransform hud_layout_midna_transform() {
    if (hud_layout() == HudLayout::WiiU) {
        return {
            .offset_x = -6.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudButtonLayout hud_layout_z_button_layout() {
    if (hardcoded_hud_layout_enabled()) {
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

inline DuskModHudTransform hud_layout_backing_transform() {
    if (hardcoded_hud_layout_enabled()) {
        return {
            .offset_x = -100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_hearts_transform() {
    if (hud_layout() == HudLayout::Dawnlight) {
        return {
            .offset_x = 100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_rupees_transform() {
    if (hud_layout() == HudLayout::Dawnlight) {
        return {
            .offset_x = 40.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_oil_transform() {
    if (hud_layout() == HudLayout::Dawnlight) {
        return {
            .offset_x = 100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_oxygen_transform() {
    if (hud_layout() == HudLayout::Dawnlight) {
        return {
            .offset_x = 100.0f,
            .offset_y = 0.0f,
            .scale = 1.0f,
        };
    }
    return {};
}

inline DuskModHudTransform hud_layout_minimap_transform() {
    switch (hud_layout()) {
    case HudLayout::WiiU:
        return {
            .offset_x = 0.0f,
            .offset_y = 50.0f,
            .scale = 0.699999988079071f,
            .slide_direction = kHudSlideLeftToRight,
        };
    case HudLayout::Dawnlight:
        return {
            .offset_x = 730.0f,
            .offset_y = -190.0f,
            .scale = 0.699999988079071f,
            .slide_direction = kHudSlideRightToLeft,
        };
    case HudLayout::GameCube:
    default:
        return {};
    }
}

}  // namespace dawnlight
