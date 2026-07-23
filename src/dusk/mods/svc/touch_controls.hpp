#pragma once

#include "dusk/ui/touch_controls_extension.hpp"

#include <cstddef>
#include <string_view>

namespace dusk::mods::svc::touch_controls {

std::string_view extra_rml_fragment() noexcept;
std::size_t extra_control_count() noexcept;
const ui::TouchLayoutControlInfo* extra_control_at(std::size_t index) noexcept;
bool display_override(ui::Control control, ui::TouchControlDisplayOverride* out) noexcept;
std::uint16_t pad_button(ui::Control control, std::uint16_t fallback) noexcept;
void control_event(ui::Control control, bool pressed) noexcept;
int aim_input_mode() noexcept;

}  // namespace dusk::mods::svc::touch_controls
