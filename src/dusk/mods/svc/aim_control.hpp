#pragma once

#include "mods/svc/aim_control.h"

#include <cstdint>

namespace dusk::mods::svc::aim_control {

int camera_mode(uint32_t pad, int itemKind, bool scopedAim, bool supportedItemAim);
int input_routing(int itemKind);
bool subject_update(void* player, int itemKind);

}  // namespace dusk::mods::svc::aim_control
