#pragma once

struct fopAc_ac_c;
class cCcD_Obj;

namespace dusk {

void MarkManualJumpStarted(const void* link);
void ClearManualJump(const void* link);
bool UpdateBulletTime(const void* link, bool isManualJumpAirborne, bool isBowAimRequested,
                      bool cancelRequested);
bool IsBulletTimeActiveForLink(const void* link);
void MarkCombatTimeProjectileActive();
void MarkCombatTimeActorHit(fopAc_ac_c* actor);
void RememberCombatTimeCollider(cCcD_Obj* obj);
void PrimeCombatTimeActorColliders(fopAc_ac_c* actor);
bool ShouldSkipCombatTimeActor(fopAc_ac_c* actor);

} // namespace dusk
