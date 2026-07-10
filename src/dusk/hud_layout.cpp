#include "dusk/hud_layout.hpp"

#include "dusk/io.hpp"
#include "dusk/main.h"
#include "dusk/mods/svc/hud_layout.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

namespace dusk::hud_layout {
namespace {

const DuskModHudLayoutSnapshot* snapshot() noexcept {
    static std::filesystem::path sLastConfigPath;
    static std::string dataPath;
    if (sLastConfigPath != dusk::ConfigPath) {
        sLastConfigPath = dusk::ConfigPath;
        dataPath = io::fs_path_to_string(sLastConfigPath);
    }
    const auto* layout = dusk::mods::svc::hud_layout_snapshot(dataPath.c_str());
    if (layout == nullptr || layout->struct_size < sizeof(DuskModHudLayoutSnapshot)) {
        return nullptr;
    }
    return layout;
}

size_t button_index(Button button) noexcept {
    return std::min(static_cast<size_t>(button),
        static_cast<size_t>(DUSK_MOD_HUD_BUTTON_COUNT - 1));
}

size_t element_index(Element element) noexcept {
    return std::min(static_cast<size_t>(element),
        static_cast<size_t>(DUSK_MOD_HUD_ELEMENT_COUNT - 1));
}

const DuskModHudButtonLayout* button_layout(Button button) noexcept {
    const auto* layout = snapshot();
    return layout != nullptr ? &layout->buttons[button_index(button)] : nullptr;
}

}  // namespace

ItemAnchor DefaultItemAnchor(Button button) noexcept {
    return button == Button::Y ? ItemAnchor::Left : ItemAnchor::Right;
}

ItemAnchor ButtonItemAnchor(Button button) noexcept {
    const auto* layout = button_layout(button);
    if (layout == nullptr || layout->item_anchor < static_cast<int>(ItemAnchor::Left) ||
        layout->item_anchor > static_cast<int>(ItemAnchor::Bottom))
    {
        return DefaultItemAnchor(button);
    }
    return static_cast<ItemAnchor>(layout->item_anchor);
}

SideAnchor ButtonTextAnchor(Button button) noexcept {
    const auto* layout = button_layout(button);
    if (layout == nullptr || layout->text_anchor != static_cast<int>(SideAnchor::Right)) {
        return SideAnchor::Left;
    }
    return SideAnchor::Right;
}

float ButtonItemScale(Button button) noexcept {
    const auto* layout = button_layout(button);
    return layout != nullptr ? layout->item_scale : 1.0f;
}

float ButtonTextScale(Button button) noexcept {
    const auto* layout = button_layout(button);
    return layout != nullptr ? layout->text_scale : 1.0f;
}

float ButtonTextOffsetX(Button button) noexcept {
    const auto* layout = button_layout(button);
    return layout != nullptr ? layout->text_offset_x : 0.0f;
}

float ButtonTextOffsetY(Button button) noexcept {
    const auto* layout = button_layout(button);
    return layout != nullptr ? layout->text_offset_y : 0.0f;
}

float ButtonItemOffsetX(Button button) noexcept {
    const auto* layout = button_layout(button);
    return layout != nullptr ? layout->item_offset_x : 0.0f;
}

float ButtonItemOffsetY(Button button) noexcept {
    const auto* layout = button_layout(button);
    return layout != nullptr ? layout->item_offset_y : 0.0f;
}

float ButtonAmmoOffsetX(Button button) noexcept {
    const auto* layout = button_layout(button);
    return layout != nullptr ? layout->ammo_offset_x : 0.0f;
}

float ButtonAmmoOffsetY(Button button) noexcept {
    const auto* layout = button_layout(button);
    return layout != nullptr ? layout->ammo_offset_y : 0.0f;
}

float ButtonAmmoScale(Button button) noexcept {
    const auto* layout = button_layout(button);
    return layout != nullptr ? layout->ammo_scale : 1.0f;
}

Transform ButtonTransform(Button button) noexcept {
    return ElementTransform(static_cast<Element>(button));
}

Transform ElementTransform(Element element) noexcept {
    const auto* layout = snapshot();
    if (layout == nullptr) {
        return {};
    }
    const auto& transform = layout->elements[element_index(element)];
    return {transform.offset_x, transform.offset_y, transform.scale};
}

bool ButtonBackgroundEnabled() noexcept {
    const auto* layout = snapshot();
    return layout == nullptr || layout->button_background != 0;
}

bool RoundXYButtons() noexcept {
    const auto* layout = snapshot();
    return layout != nullptr && layout->round_xy_buttons != 0;
}

bool DPadFollowsMinimap() noexcept {
    const auto* layout = snapshot();
    return layout == nullptr || layout->dpad_follows_minimap != 0;
}

bool MinimapSlidesRightToLeft() noexcept {
    const auto* layout = snapshot();
    return layout != nullptr && layout->minimap_slide_direction == 1;
}

u32 LayoutStamp() noexcept {
    const auto* layout = snapshot();
    return layout != nullptr ? layout->revision : 0;
}

}  // namespace dusk::hud_layout
