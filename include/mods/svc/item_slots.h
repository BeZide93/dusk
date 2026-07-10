#pragma once

#include "mods/api.h"

#define ITEM_SLOTS_SERVICE_ID "dev.twilitrealm.dusklight.item_slots"
#define ITEM_SLOTS_SERVICE_MAJOR 1u
#define ITEM_SLOTS_SERVICE_MINOR 0u

typedef uint64_t ItemSlotsProviderHandle;

typedef int (*ItemSlotsEnableZSlotFn)(ModContext* ctx, void* user_data);

typedef struct ItemSlotsProviderDesc {
    uint32_t struct_size;
    ItemSlotsEnableZSlotFn enable_z_slot;
    void* user_data;
} ItemSlotsProviderDesc;

#define ITEM_SLOTS_PROVIDER_DESC_INIT {sizeof(ItemSlotsProviderDesc), NULL, NULL}

typedef struct ItemSlotsService {
    ServiceHeader header;

    ModResult (*register_provider)(
        ModContext* ctx, const ItemSlotsProviderDesc* desc, ItemSlotsProviderHandle* out_handle);
    ModResult (*unregister_provider)(ModContext* ctx, ItemSlotsProviderHandle handle);
} ItemSlotsService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct dusk::mods::ServiceTraits<ItemSlotsService> {
    static constexpr const char* id = ITEM_SLOTS_SERVICE_ID;
    static constexpr uint16_t major_version = ITEM_SLOTS_SERVICE_MAJOR;
};
#endif
