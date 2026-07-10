#pragma once

#include "global.h"

namespace dusk::hud_layout {

enum class Button {
    A,
    B,
    X,
    Y,
    Z,
};

enum class Element {
    A,
    B,
    X,
    Y,
    Z,
    Hearts,
    Rupees,
    Keys,
    Oil,
    Oxygen,
    DPad,
    Minimap,
    ButtonBackground,
    Midna,
};

enum class ItemAnchor : int {
    Left = 0,
    Right = 1,
    Top = 2,
    Bottom = 3,
};

enum class SideAnchor : int {
    Left = 0,
    Right = 1,
};

struct Transform {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float scale = 1.0f;
};

ItemAnchor ButtonItemAnchor(Button button) noexcept;
ItemAnchor DefaultItemAnchor(Button button) noexcept;
SideAnchor ButtonTextAnchor(Button button) noexcept;
float ButtonItemScale(Button button) noexcept;
float ButtonTextScale(Button button) noexcept;
float ButtonTextOffsetX(Button button) noexcept;
float ButtonTextOffsetY(Button button) noexcept;
float ButtonItemOffsetX(Button button) noexcept;
float ButtonItemOffsetY(Button button) noexcept;
float ButtonAmmoOffsetX(Button button) noexcept;
float ButtonAmmoOffsetY(Button button) noexcept;
float ButtonAmmoScale(Button button) noexcept;
Transform ButtonTransform(Button button) noexcept;
Transform ElementTransform(Element element) noexcept;
bool ButtonBackgroundEnabled() noexcept;
bool RoundXYButtons() noexcept;
bool DPadFollowsMinimap() noexcept;
bool MinimapSlidesRightToLeft() noexcept;
u32 LayoutStamp() noexcept;

}  // namespace dusk::hud_layout
