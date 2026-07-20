#pragma once

#include "mods/svc/stage_flow.h"

namespace dusk::mods::svc::stage_flow {

bool transition_actor_update(void* actor, int actorKind);
bool sequence_complete(int sequenceKind);
bool final_battle_sequence_complete(void* camera, void* boss);

}  // namespace dusk::mods::svc::stage_flow
