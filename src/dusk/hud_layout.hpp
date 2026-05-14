#pragma once

#include "dusk/settings.h"

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
    Oil,
    DPad,
    Minimap,
    ButtonBackground,
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

const char* LayoutName(ControllerOverlayLayout layout) noexcept;
const char* ItemAnchorName(ItemAnchor anchor) noexcept;
const char* SideAnchorName(SideAnchor anchor) noexcept;
ItemAnchor NormalizeItemAnchor(int anchor) noexcept;
SideAnchor NormalizeSideAnchor(int anchor) noexcept;
ItemAnchor ButtonItemAnchor(Button button) noexcept;
ItemAnchor DefaultItemAnchor(Button button) noexcept;
SideAnchor ButtonTextAnchor(Button button) noexcept;
SideAnchor DefaultTextAnchor(Button button) noexcept;
float ButtonItemScale(Button button) noexcept;
float ButtonTextScale(Button button) noexcept;
bool HasItemAnchor(Button button) noexcept;
bool HasTextAnchor(Button button) noexcept;
bool HasItemScale(Button button) noexcept;
bool HasTextScale(Button button) noexcept;
Transform ButtonTransform(Button button) noexcept;
Transform ElementTransform(Element element) noexcept;
u32 LayoutStamp() noexcept;

}  // namespace dusk::hud_layout
