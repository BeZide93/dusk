#pragma once

#include "mods/svc/item_aim.h"

#include <cstdint>

namespace dusk::mods::svc::item_aim {

bool use_cinema_camera(uint32_t pad, bool scopedAim, bool supportedItemAim);
bool use_third_person(uint32_t pad, bool supportedItemAim);
bool movement_enabled();
bool subject_update(void* player, int itemKind);

}  // namespace dusk::mods::svc::item_aim
