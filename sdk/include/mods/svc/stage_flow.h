#pragma once

#include <mods/api.h>

#define STAGE_FLOW_SERVICE_ID "dev.twilitrealm.dusklight.stage_flow"
#define STAGE_FLOW_SERVICE_MAJOR 1u
#define STAGE_FLOW_SERVICE_MINOR 0u

typedef uint64_t StageFlowProviderHandle;

typedef enum DuskModStageFlowTransitionActorKind {
    /* Boss-warp actors that can be left static or used as stage transitions. */
    DUSK_MOD_STAGE_FLOW_TRANSITION_ACTOR_BOSS_WARP = 1,
} DuskModStageFlowTransitionActorKind;

typedef enum DuskModStageFlowTransitionMode {
    /* Keep the host actor behavior. */
    DUSK_MOD_STAGE_FLOW_TRANSITION_DEFAULT = 0,
    /* Keep the actor visible but inactive as a destination marker. */
    DUSK_MOD_STAGE_FLOW_TRANSITION_STATIC = 1,
    /* Allow the actor to run as an active transition. */
    DUSK_MOD_STAGE_FLOW_TRANSITION_ACTIVE = 2,
} DuskModStageFlowTransitionMode;

typedef enum DuskModStageFlowSequenceKind {
    /* The multi-stage final battle sequence. */
    DUSK_MOD_STAGE_FLOW_SEQUENCE_FINAL_BATTLE = 1,
} DuskModStageFlowSequenceKind;

/* Return a DuskModStageFlowTransitionMode for the requested transition actor. */
typedef int (*StageFlowTransitionActorFn)(
    ModContext* ctx, void* actor, int32_t actor_kind, void* user_data);
/* Called when a supported scripted sequence completes. Return non-zero after handling it. */
typedef int (*StageFlowSequenceCompleteFn)(
    ModContext* ctx, int32_t sequence_kind, void* user_data);

typedef struct StageFlowProviderDesc {
    uint32_t struct_size;
    StageFlowTransitionActorFn transition_actor_update;
    StageFlowSequenceCompleteFn sequence_complete;
    void* user_data;
} StageFlowProviderDesc;

#define STAGE_FLOW_PROVIDER_DESC_INIT {sizeof(StageFlowProviderDesc), NULL, NULL, NULL}

typedef struct StageFlowService {
    ServiceHeader header;

    /* Register a stage-flow provider. The latest active provider for a callback wins. */
    ModResult (*register_provider)(
        ModContext* ctx, const StageFlowProviderDesc* desc, StageFlowProviderHandle* out_handle);
    /* Unregister a provider owned by the calling mod. Providers are also removed on unload. */
    ModResult (*unregister_provider)(ModContext* ctx, StageFlowProviderHandle handle);
} StageFlowService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct mods::ServiceTraits<StageFlowService> {
    static constexpr const char* id = STAGE_FLOW_SERVICE_ID;
    static constexpr uint16_t major_version = STAGE_FLOW_SERVICE_MAJOR;
    static constexpr uint16_t minor_version = STAGE_FLOW_SERVICE_MINOR;
};
#endif
