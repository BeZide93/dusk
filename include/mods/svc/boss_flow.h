#pragma once

#include "mods/api.h"

#define BOSS_FLOW_SERVICE_ID "dev.twilitrealm.dusklight.boss_flow"
#define BOSS_FLOW_SERVICE_MAJOR 1u
#define BOSS_FLOW_SERVICE_MINOR 0u

typedef uint64_t BossFlowProviderHandle;

typedef int (*BossFlowWarpExecuteFn)(ModContext* ctx, void* boss_warp, void* user_data);
typedef int (*BossFlowFinalBattleCompleteFn)(ModContext* ctx, void* user_data);

typedef struct BossFlowProviderDesc {
    uint32_t struct_size;
    BossFlowWarpExecuteFn boss_warp_execute;
    BossFlowFinalBattleCompleteFn final_battle_complete;
    void* user_data;
} BossFlowProviderDesc;

#define BOSS_FLOW_PROVIDER_DESC_INIT {sizeof(BossFlowProviderDesc), NULL, NULL, NULL}

typedef struct BossFlowService {
    ServiceHeader header;

    ModResult (*register_provider)(
        ModContext* ctx, const BossFlowProviderDesc* desc, BossFlowProviderHandle* out_handle);
    ModResult (*unregister_provider)(ModContext* ctx, BossFlowProviderHandle handle);
} BossFlowService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct dusk::mods::ServiceTraits<BossFlowService> {
    static constexpr const char* id = BOSS_FLOW_SERVICE_ID;
    static constexpr uint16_t major_version = BOSS_FLOW_SERVICE_MAJOR;
};
#endif
