#pragma once

#include "mods/api.h"

#define ITEM_AIM_SERVICE_ID "dev.twilitrealm.dusklight.item_aim"
#define ITEM_AIM_SERVICE_MAJOR 1u
#define ITEM_AIM_SERVICE_MINOR 0u

typedef uint64_t ItemAimProviderHandle;

typedef enum DuskModItemAimKind {
    DUSK_MOD_ITEM_AIM_BOW = 0,
    DUSK_MOD_ITEM_AIM_BOOMERANG = 1,
    DUSK_MOD_ITEM_AIM_HOOKSHOT = 2,
    DUSK_MOD_ITEM_AIM_IRON_BALL = 3,
    DUSK_MOD_ITEM_AIM_COPY_ROD = 4,
} DuskModItemAimKind;

typedef int (*ItemAimUseCinemaCameraFn)(
    ModContext* ctx, uint32_t pad, int scoped_aim, int supported_item_aim, void* user_data);
typedef int (*ItemAimUseThirdPersonFn)(
    ModContext* ctx, uint32_t pad, int supported_item_aim, void* user_data);
typedef int (*ItemAimMovementEnabledFn)(ModContext* ctx, void* user_data);
typedef int (*ItemAimSubjectUpdateFn)(
    ModContext* ctx, void* player, int item_kind, void* user_data);

typedef struct ItemAimProviderDesc {
    uint32_t struct_size;
    ItemAimUseCinemaCameraFn use_cinema_camera;
    ItemAimUseThirdPersonFn use_third_person;
    ItemAimMovementEnabledFn movement_enabled;
    ItemAimSubjectUpdateFn subject_update;
    void* user_data;
} ItemAimProviderDesc;

#define ITEM_AIM_PROVIDER_DESC_INIT {sizeof(ItemAimProviderDesc), NULL, NULL, NULL, NULL, NULL}

typedef struct ItemAimService {
    ServiceHeader header;

    ModResult (*register_provider)(
        ModContext* ctx, const ItemAimProviderDesc* desc, ItemAimProviderHandle* out_handle);
    ModResult (*unregister_provider)(ModContext* ctx, ItemAimProviderHandle handle);
} ItemAimService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct dusk::mods::ServiceTraits<ItemAimService> {
    static constexpr const char* id = ITEM_AIM_SERVICE_ID;
    static constexpr uint16_t major_version = ITEM_AIM_SERVICE_MAJOR;
};
#endif
