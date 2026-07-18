#pragma once

#include "mods/svc/stage_flow.h"

namespace dusk::mods::svc::stage_flow {

int transition_actor_update(void* actor, int actorKind);
bool sequence_complete(int sequenceKind);

}  // namespace dusk::mods::svc::stage_flow
