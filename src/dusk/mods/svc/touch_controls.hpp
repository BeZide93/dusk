#pragma once

#include "dusk/ui/touch_controls_extension.hpp"

#include <cstddef>
#include <string_view>

namespace dusk::mods::svc::touch_controls {

std::string_view extra_rml_fragment() noexcept;
std::size_t extra_control_count() noexcept;
const ui::TouchLayoutControlInfo* extra_control_at(std::size_t index) noexcept;
bool display_override(ui::Control control, ui::TouchControlDisplayOverride* out) noexcept;

}  // namespace dusk::mods::svc::touch_controls
