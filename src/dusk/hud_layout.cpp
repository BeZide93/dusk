#include "dusk/hud_layout.hpp"

namespace dusk::hud_layout {
namespace {

Element button_element(Button button) noexcept {
    switch (button) {
    case Button::B:
        return Element::B;
    case Button::X:
        return Element::X;
    case Button::Y:
        return Element::Y;
    case Button::Z:
        return Element::Z;
    case Button::A:
    default:
        return Element::A;
    }
}

ConfigVar<float>& offset_x_var(Element element) noexcept {
    auto& settings = getSettings().game;
    switch (element) {
    case Element::B:
        return settings.hudButtonBOffsetX;
    case Element::X:
        return settings.hudButtonXOffsetX;
    case Element::Y:
        return settings.hudButtonYOffsetX;
    case Element::Z:
        return settings.hudButtonZOffsetX;
    case Element::Hearts:
        return settings.hudHeartsOffsetX;
    case Element::Rupees:
        return settings.hudRupeesOffsetX;
    case Element::Keys:
        return settings.hudKeysOffsetX;
    case Element::Oil:
        return settings.hudOilOffsetX;
    case Element::Oxygen:
        return settings.hudOxygenOffsetX;
    case Element::DPad:
        return settings.hudDPadOffsetX;
    case Element::Minimap:
        return settings.hudMinimapOffsetX;
    case Element::ButtonBackground:
        return settings.hudButtonBackgroundOffsetX;
    case Element::Midna:
        return settings.hudMidnaOffsetX;
    case Element::A:
    default:
        return settings.hudButtonAOffsetX;
    }
}

ConfigVar<float>& offset_y_var(Element element) noexcept {
    auto& settings = getSettings().game;
    switch (element) {
    case Element::B:
        return settings.hudButtonBOffsetY;
    case Element::X:
        return settings.hudButtonXOffsetY;
    case Element::Y:
        return settings.hudButtonYOffsetY;
    case Element::Z:
        return settings.hudButtonZOffsetY;
    case Element::Hearts:
        return settings.hudHeartsOffsetY;
    case Element::Rupees:
        return settings.hudRupeesOffsetY;
    case Element::Keys:
        return settings.hudKeysOffsetY;
    case Element::Oil:
        return settings.hudOilOffsetY;
    case Element::Oxygen:
        return settings.hudOxygenOffsetY;
    case Element::DPad:
        return settings.hudDPadOffsetY;
    case Element::Minimap:
        return settings.hudMinimapOffsetY;
    case Element::ButtonBackground:
        return settings.hudButtonBackgroundOffsetY;
    case Element::Midna:
        return settings.hudMidnaOffsetY;
    case Element::A:
    default:
        return settings.hudButtonAOffsetY;
    }
}

ConfigVar<float>& scale_var(Element element) noexcept {
    auto& settings = getSettings().game;
    switch (element) {
    case Element::B:
        return settings.hudButtonBScale;
    case Element::X:
        return settings.hudButtonXScale;
    case Element::Y:
        return settings.hudButtonYScale;
    case Element::Z:
        return settings.hudButtonZScale;
    case Element::Hearts:
        return settings.hudHeartsScale;
    case Element::Rupees:
        return settings.hudRupeesScale;
    case Element::Keys:
        return settings.hudKeysScale;
    case Element::Oil:
        return settings.hudOilScale;
    case Element::Oxygen:
        return settings.hudOxygenScale;
    case Element::DPad:
        return settings.hudDPadScale;
    case Element::Minimap:
        return settings.hudMinimapScale;
    case Element::ButtonBackground:
        return settings.hudButtonBackgroundScale;
    case Element::Midna:
        return settings.hudMidnaScale;
    case Element::A:
    default:
        return settings.hudButtonAScale;
    }
}

ConfigVar<int>& item_anchor_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::B:
        return settings.hudButtonBItemAnchor;
    case Button::Y:
        return settings.hudButtonYItemAnchor;
    case Button::X:
    default:
        return settings.hudButtonXItemAnchor;
    }
}

ConfigVar<int>& text_anchor_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::B:
        return settings.hudButtonBTextAnchor;
    case Button::X:
        return settings.hudButtonXTextAnchor;
    case Button::Y:
        return settings.hudButtonYTextAnchor;
    case Button::A:
    default:
        return settings.hudButtonATextAnchor;
    }
}

ConfigVar<float>& item_scale_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::B:
        return settings.hudButtonBItemScale;
    case Button::Y:
        return settings.hudButtonYItemScale;
    case Button::Z:
        return settings.hudButtonZItemScale;
    case Button::X:
    default:
        return settings.hudButtonXItemScale;
    }
}

ConfigVar<float>& item_offset_x_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::B:
        return settings.hudButtonBItemOffsetX;
    case Button::Y:
        return settings.hudButtonYItemOffsetX;
    case Button::Z:
        return settings.hudButtonZItemOffsetX;
    case Button::X:
    default:
        return settings.hudButtonXItemOffsetX;
    }
}

ConfigVar<float>& item_offset_y_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::B:
        return settings.hudButtonBItemOffsetY;
    case Button::Y:
        return settings.hudButtonYItemOffsetY;
    case Button::Z:
        return settings.hudButtonZItemOffsetY;
    case Button::X:
    default:
        return settings.hudButtonXItemOffsetY;
    }
}

ConfigVar<float>& ammo_offset_x_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::Y:
        return settings.hudButtonYAmmoOffsetX;
    case Button::Z:
        return settings.hudButtonZAmmoOffsetX;
    case Button::X:
    default:
        return settings.hudButtonXAmmoOffsetX;
    }
}

ConfigVar<float>& ammo_offset_y_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::Y:
        return settings.hudButtonYAmmoOffsetY;
    case Button::Z:
        return settings.hudButtonZAmmoOffsetY;
    case Button::X:
    default:
        return settings.hudButtonXAmmoOffsetY;
    }
}

ConfigVar<float>& ammo_scale_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::Y:
        return settings.hudButtonYAmmoScale;
    case Button::Z:
        return settings.hudButtonZAmmoScale;
    case Button::X:
    default:
        return settings.hudButtonXAmmoScale;
    }
}

ConfigVar<float>& text_scale_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::B:
        return settings.hudButtonBTextScale;
    case Button::X:
        return settings.hudButtonXTextScale;
    case Button::Y:
        return settings.hudButtonYTextScale;
    case Button::Z:
        return settings.hudButtonZTextScale;
    case Button::A:
    default:
        return settings.hudButtonATextScale;
    }
}

ConfigVar<float>& text_offset_x_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::B:
        return settings.hudButtonBTextOffsetX;
    case Button::X:
        return settings.hudButtonXTextOffsetX;
    case Button::Y:
        return settings.hudButtonYTextOffsetX;
    case Button::A:
    default:
        return settings.hudButtonATextOffsetX;
    }
}

ConfigVar<float>& text_offset_y_var(Button button) noexcept {
    auto& settings = getSettings().game;
    switch (button) {
    case Button::B:
        return settings.hudButtonBTextOffsetY;
    case Button::X:
        return settings.hudButtonXTextOffsetY;
    case Button::Y:
        return settings.hudButtonYTextOffsetY;
    case Button::A:
    default:
        return settings.hudButtonATextOffsetY;
    }
}

u32 hash_float(u32 hash, float value) noexcept {
    const auto quantized = static_cast<s32>(value * 100.0f + (value >= 0.0f ? 0.5f : -0.5f));
    return (hash ^ static_cast<u32>(quantized)) * 16777619u;
}

u32 hash_int(u32 hash, int value) noexcept {
    return (hash ^ static_cast<u32>(value)) * 16777619u;
}

u32 hash_element(u32 hash, Element element) noexcept {
    const Transform transform = ElementTransform(element);
    hash = hash_float(hash, transform.offsetX);
    hash = hash_float(hash, transform.offsetY);
    return hash_float(hash, transform.scale);
}

}  // namespace

const char* LayoutName(ControllerOverlayLayout layout) noexcept {
    switch (layout) {
    case ControllerOverlayLayout::WiiU:
        return "Wii U";
    case ControllerOverlayLayout::XBox:
        return "XBox";
    case ControllerOverlayLayout::GameCube:
    default:
        return "GameCube";
    }
}

const char* ItemAnchorName(ItemAnchor anchor) noexcept {
    switch (anchor) {
    case ItemAnchor::Left:
        return "Left";
    case ItemAnchor::Right:
        return "Right";
    case ItemAnchor::Top:
        return "Top";
    case ItemAnchor::Bottom:
    default:
        return "Bottom";
    }
}

const char* SideAnchorName(SideAnchor anchor) noexcept {
    switch (anchor) {
    case SideAnchor::Right:
        return "Right";
    case SideAnchor::Left:
    default:
        return "Left";
    }
}

ItemAnchor NormalizeItemAnchor(int anchor) noexcept {
    switch (anchor) {
    case static_cast<int>(ItemAnchor::Left):
        return ItemAnchor::Left;
    case static_cast<int>(ItemAnchor::Right):
        return ItemAnchor::Right;
    case static_cast<int>(ItemAnchor::Top):
        return ItemAnchor::Top;
    case static_cast<int>(ItemAnchor::Bottom):
        return ItemAnchor::Bottom;
    default:
        return ItemAnchor::Right;
    }
}

SideAnchor NormalizeSideAnchor(int anchor) noexcept {
    switch (anchor) {
    case static_cast<int>(SideAnchor::Right):
        return SideAnchor::Right;
    case static_cast<int>(SideAnchor::Left):
    default:
        return SideAnchor::Left;
    }
}

ItemAnchor DefaultItemAnchor(Button button) noexcept {
    return button == Button::Y ? ItemAnchor::Left : ItemAnchor::Right;
}

SideAnchor DefaultTextAnchor(Button) noexcept {
    return SideAnchor::Left;
}

bool HasItemAnchor(Button button) noexcept {
    return button == Button::B || button == Button::X || button == Button::Y;
}

bool HasTextAnchor(Button button) noexcept {
    return button == Button::A || button == Button::B || button == Button::X ||
           button == Button::Y;
}

bool HasItemScale(Button button) noexcept {
    return button == Button::B || button == Button::X || button == Button::Y ||
           button == Button::Z;
}

bool HasItemOffset(Button button) noexcept {
    return HasItemScale(button);
}

bool HasTextScale(Button button) noexcept {
    return button == Button::A || button == Button::B || button == Button::X ||
           button == Button::Y || button == Button::Z;
}

bool HasTextOffset(Button button) noexcept {
    return button == Button::A || button == Button::B || button == Button::X ||
           button == Button::Y;
}

bool HasAmmoLayout(Button button) noexcept {
    return button == Button::X || button == Button::Y || button == Button::Z;
}

ItemAnchor ButtonItemAnchor(Button button) noexcept {
    if (!HasItemAnchor(button)) {
        return DefaultItemAnchor(button);
    }
    const int anchor = item_anchor_var(button).getValue();
    if (anchor < static_cast<int>(ItemAnchor::Left) ||
        anchor > static_cast<int>(ItemAnchor::Bottom))
    {
        return DefaultItemAnchor(button);
    }
    return NormalizeItemAnchor(anchor);
}

SideAnchor ButtonTextAnchor(Button button) noexcept {
    if (!HasTextAnchor(button)) {
        return DefaultTextAnchor(button);
    }
    const int anchor = text_anchor_var(button).getValue();
    if (anchor < static_cast<int>(SideAnchor::Left) ||
        anchor > static_cast<int>(SideAnchor::Right))
    {
        return DefaultTextAnchor(button);
    }
    return NormalizeSideAnchor(anchor);
}

float ButtonItemScale(Button button) noexcept {
    return HasItemScale(button) ? item_scale_var(button).getValue() : 1.0f;
}

float ButtonTextScale(Button button) noexcept {
    return HasTextScale(button) ? text_scale_var(button).getValue() : 1.0f;
}

float ButtonTextOffsetX(Button button) noexcept {
    return HasTextOffset(button) ? text_offset_x_var(button).getValue() : 0.0f;
}

float ButtonTextOffsetY(Button button) noexcept {
    return HasTextOffset(button) ? text_offset_y_var(button).getValue() : 0.0f;
}

float ButtonItemOffsetX(Button button) noexcept {
    return HasItemOffset(button) ? item_offset_x_var(button).getValue() : 0.0f;
}

float ButtonItemOffsetY(Button button) noexcept {
    return HasItemOffset(button) ? item_offset_y_var(button).getValue() : 0.0f;
}

float ButtonAmmoOffsetX(Button button) noexcept {
    return HasAmmoLayout(button) ? ammo_offset_x_var(button).getValue() : 0.0f;
}

float ButtonAmmoOffsetY(Button button) noexcept {
    return HasAmmoLayout(button) ? ammo_offset_y_var(button).getValue() : 0.0f;
}

float ButtonAmmoScale(Button button) noexcept {
    return HasAmmoLayout(button) ? ammo_scale_var(button).getValue() : 1.0f;
}

Transform ButtonTransform(Button button) noexcept {
    return ElementTransform(button_element(button));
}

Transform ElementTransform(Element element) noexcept {
    return {
        .offsetX = offset_x_var(element).getValue(),
        .offsetY = offset_y_var(element).getValue(),
        .scale = scale_var(element).getValue(),
    };
}

u32 LayoutStamp() noexcept {
    u32 hash = 2166136261u;
    hash = hash_element(hash, Element::A);
    hash = hash_element(hash, Element::B);
    hash = hash_element(hash, Element::X);
    hash = hash_element(hash, Element::Y);
    hash = hash_element(hash, Element::Z);
    hash = hash_element(hash, Element::Hearts);
    hash = hash_element(hash, Element::Rupees);
    hash = hash_element(hash, Element::Keys);
    hash = hash_element(hash, Element::Oil);
    hash = hash_element(hash, Element::Oxygen);
    hash = hash_element(hash, Element::DPad);
    hash = hash_element(hash, Element::Minimap);
    hash = hash_element(hash, Element::ButtonBackground);
    hash = hash_element(hash, Element::Midna);
    hash = hash_int(hash, getSettings().game.hudButtonBackground.getValue() ? 1 : 0);
    hash = hash_int(hash, getSettings().game.hudRoundXYButtons.getValue() ? 1 : 0);
    hash = hash_int(hash, getSettings().game.hudDPadFollowMinimap.getValue() ? 1 : 0);
    hash = hash_int(
        hash, static_cast<int>(getSettings().game.hudMinimapSlideDirection.getValue()));
    hash = hash_int(hash, static_cast<int>(ButtonTextAnchor(Button::A)));
    hash = hash_int(hash, static_cast<int>(ButtonTextAnchor(Button::B)));
    hash = hash_int(hash, static_cast<int>(ButtonTextAnchor(Button::X)));
    hash = hash_int(hash, static_cast<int>(ButtonTextAnchor(Button::Y)));
    hash = hash_int(hash, static_cast<int>(ButtonItemAnchor(Button::B)));
    hash = hash_int(hash, static_cast<int>(ButtonItemAnchor(Button::X)));
    hash = hash_int(hash, static_cast<int>(ButtonItemAnchor(Button::Y)));
    hash = hash_float(hash, ButtonTextScale(Button::A));
    hash = hash_float(hash, ButtonTextOffsetX(Button::A));
    hash = hash_float(hash, ButtonTextOffsetY(Button::A));
    hash = hash_float(hash, ButtonItemScale(Button::B));
    hash = hash_float(hash, ButtonItemOffsetX(Button::B));
    hash = hash_float(hash, ButtonItemOffsetY(Button::B));
    hash = hash_float(hash, ButtonTextScale(Button::B));
    hash = hash_float(hash, ButtonTextOffsetX(Button::B));
    hash = hash_float(hash, ButtonTextOffsetY(Button::B));
    hash = hash_float(hash, ButtonItemScale(Button::X));
    hash = hash_float(hash, ButtonItemOffsetX(Button::X));
    hash = hash_float(hash, ButtonItemOffsetY(Button::X));
    hash = hash_float(hash, ButtonAmmoOffsetX(Button::X));
    hash = hash_float(hash, ButtonAmmoOffsetY(Button::X));
    hash = hash_float(hash, ButtonAmmoScale(Button::X));
    hash = hash_float(hash, ButtonTextScale(Button::X));
    hash = hash_float(hash, ButtonTextOffsetX(Button::X));
    hash = hash_float(hash, ButtonTextOffsetY(Button::X));
    hash = hash_float(hash, ButtonItemScale(Button::Y));
    hash = hash_float(hash, ButtonItemOffsetX(Button::Y));
    hash = hash_float(hash, ButtonItemOffsetY(Button::Y));
    hash = hash_float(hash, ButtonAmmoOffsetX(Button::Y));
    hash = hash_float(hash, ButtonAmmoOffsetY(Button::Y));
    hash = hash_float(hash, ButtonAmmoScale(Button::Y));
    hash = hash_float(hash, ButtonTextScale(Button::Y));
    hash = hash_float(hash, ButtonTextOffsetX(Button::Y));
    hash = hash_float(hash, ButtonTextOffsetY(Button::Y));
    hash = hash_float(hash, ButtonItemScale(Button::Z));
    hash = hash_float(hash, ButtonItemOffsetX(Button::Z));
    hash = hash_float(hash, ButtonItemOffsetY(Button::Z));
    hash = hash_float(hash, ButtonAmmoOffsetX(Button::Z));
    hash = hash_float(hash, ButtonAmmoOffsetY(Button::Z));
    hash = hash_float(hash, ButtonAmmoScale(Button::Z));
    return hash_float(hash, ButtonTextScale(Button::Z));
}

}  // namespace dusk::hud_layout
