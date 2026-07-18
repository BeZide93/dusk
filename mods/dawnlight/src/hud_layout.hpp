#pragma once

#include "mods/api.h"

namespace dawnlight {

ModResult register_hud_layout_provider(ModError* error);
void update_hud_layout();

}  // namespace dawnlight
