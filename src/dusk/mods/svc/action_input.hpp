#pragma once

#include "mods/svc/action_input.h"

namespace dusk::mods::svc::action_input {

int player_action_state(void* player, int action, bool defaultPressed);
bool player_action_pressed(void* player, int action, bool defaultPressed);

}  // namespace dusk::mods::svc::action_input
