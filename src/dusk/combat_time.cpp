#include "dusk/combat_time.hpp"

#include "SSystem/SComponent/c_counter.h"
#include "SSystem/SComponent/c_cc_d.h"
#include "d/d_cc_s.h"
#include "d/d_com_inf_game.h"
#include "dusk/settings.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_name.h"

namespace dusk {
namespace {

enum class CombatTimeMode {
    None,
    BulletTime,
};

struct CombatTimeState {
    CombatTimeMode mode = CombatTimeMode::None;
    const void* owner = nullptr;
    u32 startFrame = 0;
    u32 durationFrames = 0;
    u32 slowFrameInterval = 1;
};

CombatTimeState g_combatTime;
const void* g_manualJumpOwner = nullptr;
u32 g_manualJumpStartFrame = 0;
bool g_bulletTimeUsedForJump = false;
u32 g_projectileActiveFrame = 0;

constexpr u32 BULLET_TIME_MAX_FRAMES = 300;
constexpr u32 BULLET_TIME_SLOW_INTERVAL = 15;
constexpr u32 MANUAL_JUMP_MAX_TRACK_FRAMES = 420;
constexpr u32 HIT_EXECUTE_GRACE_FRAMES = 6;
constexpr u32 COLLIDER_CACHE_ENTRIES = 256;
constexpr u32 COLLIDERS_PER_ACTOR = 64;

struct ColliderCacheEntry {
    fopAc_ac_c* actor = nullptr;
    cCcD_Obj* colliders[COLLIDERS_PER_ACTOR] = {};
    u32 lastFrame = 0;
    u8 count = 0;
};

struct HitExecuteEntry {
    fopAc_ac_c* actor = nullptr;
    u32 frame = 0;
};

ColliderCacheEntry g_colliderCache[COLLIDER_CACHE_ENTRIES];
HitExecuteEntry g_hitExecuteActors[32];

u32 currentFrame() {
    return g_Counter.mCounter0;
}

bool frameElapsed(u32 start, u32 duration) {
    return currentFrame() - start >= duration;
}

bool combatTimeActive() {
    if (g_combatTime.mode == CombatTimeMode::None) {
        return false;
    }

    if (frameElapsed(g_combatTime.startFrame, g_combatTime.durationFrames)) {
        g_combatTime.mode = CombatTimeMode::None;
        g_combatTime.owner = nullptr;
        return false;
    }

    return true;
}

bool bulletTimeOwnedBy(const void* owner) {
    return g_combatTime.mode == CombatTimeMode::BulletTime &&
           g_combatTime.owner == owner;
}

void stopCombatTimeForOwner(const void* owner) {
    if (g_combatTime.owner == owner) {
        g_combatTime.mode = CombatTimeMode::None;
        g_combatTime.owner = nullptr;
    }
}

bool isActorExemptFromCombatTime(fopAc_ac_c* actor) {
    if (actor == nullptr) {
        return true;
    }

    switch (fopAcM_GetName(actor)) {
    case fpcNm_ALINK_e:
    case fpcNm_ARROW_e:
        return true;
    default:
        return false;
    }
}

ColliderCacheEntry* findColliderEntry(fopAc_ac_c* actor, bool create) {
    ColliderCacheEntry* oldest = &g_colliderCache[0];

    for (ColliderCacheEntry& entry : g_colliderCache) {
        if (entry.actor == actor) {
            return &entry;
        }

        if (entry.actor == nullptr && create) {
            entry.actor = actor;
            entry.count = 0;
            entry.lastFrame = currentFrame();
            return &entry;
        }

        if (entry.lastFrame < oldest->lastFrame) {
            oldest = &entry;
        }
    }

    if (!create) {
        return nullptr;
    }

    oldest->actor = actor;
    oldest->count = 0;
    oldest->lastFrame = currentFrame();
    return oldest;
}

bool actorHasHitExecuteGrace(fopAc_ac_c* actor) {
    if (actor == nullptr) {
        return false;
    }

    for (HitExecuteEntry& entry : g_hitExecuteActors) {
        if (entry.actor == actor &&
            currentFrame() - entry.frame <= HIT_EXECUTE_GRACE_FRAMES)
        {
            return true;
        }
    }

    return false;
}

void startBulletTime(const void* link) {
    if (g_bulletTimeUsedForJump) {
        return;
    }

    g_combatTime.mode = CombatTimeMode::BulletTime;
    g_combatTime.owner = link;
    g_combatTime.startFrame = currentFrame();
    g_combatTime.durationFrames = BULLET_TIME_MAX_FRAMES;
    g_combatTime.slowFrameInterval = BULLET_TIME_SLOW_INTERVAL;
    g_bulletTimeUsedForJump = true;
}

} // namespace

void MarkManualJumpStarted(const void* link) {
    g_manualJumpOwner = link;
    g_manualJumpStartFrame = currentFrame();
    g_bulletTimeUsedForJump = false;
}

void ClearManualJump(const void* link) {
    if (g_manualJumpOwner != link) {
        return;
    }

    g_manualJumpOwner = nullptr;
    g_bulletTimeUsedForJump = false;
    stopCombatTimeForOwner(link);
}

bool UpdateBulletTime(const void* link, bool isManualJumpAirborne, bool isBowAimRequested,
                      bool cancelRequested) {
    const bool wasActive = bulletTimeOwnedBy(link);

    if (!getSettings().game.enableBulletTime) {
        ClearManualJump(link);
        return wasActive;
    }

    if (g_manualJumpOwner != link ||
        frameElapsed(g_manualJumpStartFrame, MANUAL_JUMP_MAX_TRACK_FRAMES))
    {
        stopCombatTimeForOwner(link);
        return wasActive;
    }

    if (!isManualJumpAirborne) {
        ClearManualJump(link);
        return wasActive;
    }

    if (cancelRequested) {
        stopCombatTimeForOwner(link);
        return wasActive;
    }

    if (!combatTimeActive() && isBowAimRequested) {
        startBulletTime(link);
    }

    return wasActive && !bulletTimeOwnedBy(link);
}

bool IsBulletTimeActiveForLink(const void* link) {
    return combatTimeActive() &&
           g_combatTime.mode == CombatTimeMode::BulletTime &&
           g_combatTime.owner == link;
}

void MarkCombatTimeProjectileActive() {
    if (combatTimeActive()) {
        g_projectileActiveFrame = currentFrame();
    }
}

void MarkCombatTimeActorHit(fopAc_ac_c* actor) {
    if (!combatTimeActive() || actor == nullptr || isActorExemptFromCombatTime(actor)) {
        return;
    }

    HitExecuteEntry* oldest = &g_hitExecuteActors[0];
    for (HitExecuteEntry& entry : g_hitExecuteActors) {
        if (entry.actor == actor || entry.actor == nullptr) {
            entry.actor = actor;
            entry.frame = currentFrame();
            return;
        }

        if (entry.frame < oldest->frame) {
            oldest = &entry;
        }
    }

    oldest->actor = actor;
    oldest->frame = currentFrame();
}

void RememberCombatTimeCollider(cCcD_Obj* obj) {
    if (obj == nullptr) {
        return;
    }

    fopAc_ac_c* actor = obj->GetAc();
    if (actor == nullptr || isActorExemptFromCombatTime(actor)) {
        return;
    }

    ColliderCacheEntry* entry = findColliderEntry(actor, true);
    if (entry == nullptr) {
        return;
    }

    entry->lastFrame = currentFrame();

    for (u8 i = 0; i < entry->count; i++) {
        if (entry->colliders[i] == obj) {
            return;
        }
    }

    if (entry->count < COLLIDERS_PER_ACTOR) {
        entry->colliders[entry->count++] = obj;
    }
}

void PrimeCombatTimeActorColliders(fopAc_ac_c* actor) {
    ColliderCacheEntry* entry = findColliderEntry(actor, false);
    if (entry == nullptr) {
        return;
    }

    for (u8 i = 0; i < entry->count; i++) {
        cCcD_Obj* obj = entry->colliders[i];
        if (obj != nullptr) {
            dComIfG_Ccsp()->Set(obj);
        }
    }
}

bool ShouldSkipCombatTimeActor(fopAc_ac_c* actor) {
    if (!combatTimeActive() || isActorExemptFromCombatTime(actor)) {
        return false;
    }

    if (actorHasHitExecuteGrace(actor)) {
        return false;
    }

    if (g_combatTime.slowFrameInterval <= 1) {
        return false;
    }

    return currentFrame() % g_combatTime.slowFrameInterval != 0;
}

} // namespace dusk
