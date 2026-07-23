#pragma once

#include "mods/api.h"
#include "mods/svc/ui.h"

namespace dawnlight {

ModResult register_hud_layout(ModError* error);
ModResult build_hud_layout_settings_tab(
    ModContext* ctx, UiWindowHandle window, UiElementHandle left, UiElementHandle right,
    void* userData, ModError* error);
ModResult build_hud_layout_files_tab(
    ModContext* ctx, UiWindowHandle window, UiElementHandle left, UiElementHandle right,
    void* userData, ModError* error);
void update_hud_layout();

}  // namespace dawnlight
