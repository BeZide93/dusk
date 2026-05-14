#pragma once

#include "SSystem/SComponent/c_API_controller_pad.h"
#include "dusk/settings.h"
#include "nlohmann/json.hpp"

#include <SDL3/SDL_events.h>
#include <cstddef>

namespace dusk::touch_controls {

bool HandleEvent(const SDL_Event& event) noexcept;
void DrawOverlay() noexcept;
void MergeToPad(interface_of_controller_pad& pad) noexcept;
void ResetInputState() noexcept;
void ApplyPreset(ControllerOverlayLayout preset) noexcept;
size_t ControlCount() noexcept;
const char* ControlDisplayName(size_t index) noexcept;
int ControlScalePercent(size_t index) noexcept;
int DefaultControlScalePercent(size_t index) noexcept;
void SetControlScalePercent(size_t index, int percent) noexcept;
void SaveLayout() noexcept;
nlohmann::json ExportLayout();
bool ImportLayout(const nlohmann::json& root) noexcept;

}  // namespace dusk::touch_controls
