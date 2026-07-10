#pragma once

#include "mods/api.h"

#define COMBAT_INPUT_SERVICE_ID "dev.twilitrealm.dusklight.combat_input"
#define COMBAT_INPUT_SERVICE_MAJOR 1u
#define COMBAT_INPUT_SERVICE_MINOR 0u

typedef uint64_t CombatInputProviderHandle;

typedef int (*CombatInputManualShieldingEnabledFn)(ModContext* ctx, void* user_data);
typedef int (*CombatInputManualShieldButtonFn)(ModContext* ctx, void* player, void* user_data);

typedef struct CombatInputProviderDesc {
    uint32_t struct_size;
    CombatInputManualShieldingEnabledFn manual_shielding_enabled;
    CombatInputManualShieldButtonFn manual_shield_button;
    void* user_data;
} CombatInputProviderDesc;

#define COMBAT_INPUT_PROVIDER_DESC_INIT {sizeof(CombatInputProviderDesc), NULL, NULL, NULL}

typedef struct CombatInputService {
    ServiceHeader header;

    ModResult (*register_provider)(
        ModContext* ctx, const CombatInputProviderDesc* desc, CombatInputProviderHandle* out_handle);
    ModResult (*unregister_provider)(ModContext* ctx, CombatInputProviderHandle handle);
} CombatInputService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct dusk::mods::ServiceTraits<CombatInputService> {
    static constexpr const char* id = COMBAT_INPUT_SERVICE_ID;
    static constexpr uint16_t major_version = COMBAT_INPUT_SERVICE_MAJOR;
};
#endif
