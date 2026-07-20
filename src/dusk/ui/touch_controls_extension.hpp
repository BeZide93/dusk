#pragma once

#include "controls.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace dusk::ui {

struct TouchLayoutControlInfo {
    std::string_view layoutId;
    const char* elementId = nullptr;
    ControlProps props;
    Control control = Control::COUNT;
    bool hasControl = false;
};

struct TouchControlDisplayOverride {
    const char* iconSource = nullptr;
    uint64_t iconRevision = 0;
    bool visible = false;
    bool showIcon = false;
};

// TODO(mod-services): Keep this host adapter aligned with any upstream touch-control extension registry.
std::string_view touch_controls_extra_rml_fragment() noexcept;
std::size_t touch_layout_extra_control_count() noexcept;
const TouchLayoutControlInfo* touch_layout_extra_control_at(std::size_t index) noexcept;
bool touch_control_display_override(Control control, TouchControlDisplayOverride* out) noexcept;
std::uint16_t touch_control_pad_button(Control control, std::uint16_t fallback) noexcept;
void touch_control_event(Control control, bool pressed) noexcept;

}  // namespace dusk::ui
