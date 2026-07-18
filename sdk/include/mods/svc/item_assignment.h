#pragma once

#include <mods/api.h>

#define ITEM_ASSIGNMENT_SERVICE_ID "dev.twilitrealm.dusklight.item_assignment"
#define ITEM_ASSIGNMENT_SERVICE_MAJOR 1u
#define ITEM_ASSIGNMENT_SERVICE_MINOR 0u

typedef uint64_t ItemAssignmentProviderHandle;

/* Return the number of direct item-assignment slots to expose in the item wheel. */
typedef int (*ItemAssignmentSelectItemSlotCountFn)(ModContext* ctx, void* user_data);

typedef struct ItemAssignmentProviderDesc {
    uint32_t struct_size;
    ItemAssignmentSelectItemSlotCountFn select_item_slot_count;
    void* user_data;
} ItemAssignmentProviderDesc;

#define ITEM_ASSIGNMENT_PROVIDER_DESC_INIT {sizeof(ItemAssignmentProviderDesc), NULL, NULL}

typedef struct ItemAssignmentService {
    ServiceHeader header;

    /* Register an item-assignment provider. The latest active provider for a callback wins. */
    ModResult (*register_provider)(
        ModContext* ctx, const ItemAssignmentProviderDesc* desc, ItemAssignmentProviderHandle* out_handle);
    /* Unregister a provider owned by the calling mod. Providers are also removed on unload. */
    ModResult (*unregister_provider)(ModContext* ctx, ItemAssignmentProviderHandle handle);
} ItemAssignmentService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct mods::ServiceTraits<ItemAssignmentService> {
    static constexpr const char* id = ITEM_ASSIGNMENT_SERVICE_ID;
    static constexpr uint16_t major_version = ITEM_ASSIGNMENT_SERVICE_MAJOR;
    static constexpr uint16_t minor_version = ITEM_ASSIGNMENT_SERVICE_MINOR;
};
#endif
