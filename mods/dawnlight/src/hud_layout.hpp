#pragma once

#include "mods/api.h"
#include "mods/svc/hud_layout.h"
#include "mods/svc/ui.h"

namespace dawnlight {

DuskModHudTransform hud_layout_z_transform();
DuskModHudTransform hud_layout_dpad_transform();
DuskModHudTransform hud_layout_midna_transform();
DuskModHudButtonLayout hud_layout_z_button_layout();
ModResult register_hud_layout(ModError* error);
ModResult build_hud_layout_settings_tab(
    ModContext* ctx, UiWindowHandle window, UiElementHandle left, UiElementHandle right,
    void* userData, ModError* error);
ModResult build_hud_layout_files_tab(
    ModContext* ctx, UiWindowHandle window, UiElementHandle left, UiElementHandle right,
    void* userData, ModError* error);
void update_hud_layout();

}  // namespace dawnlight
