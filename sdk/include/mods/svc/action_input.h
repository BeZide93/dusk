#pragma once

#include <mods/api.h>

#define ACTION_INPUT_SERVICE_ID "dev.twilitrealm.dusklight.action_input"
#define ACTION_INPUT_SERVICE_MAJOR 1u
#define ACTION_INPUT_SERVICE_MINOR 0u

typedef uint64_t ActionInputProviderHandle;

typedef enum DuskModActionInputState {
    /* Keep the host's original input state. */
    DUSK_MOD_ACTION_INPUT_DEFAULT = -1,
    /* Force the action to be inactive for this frame. */
    DUSK_MOD_ACTION_INPUT_RELEASED = 0,
    /* Force the action to be active for this frame. */
    DUSK_MOD_ACTION_INPUT_PRESSED = 1,
} DuskModActionInputState;

typedef enum DuskModPlayerAction {
    /* Player guard/shield state. */
    DUSK_MOD_PLAYER_ACTION_GUARD = 1,
} DuskModPlayerAction;

/* Return a DuskModActionInputState for the requested player action. */
typedef int (*ActionInputPlayerActionFn)(
    ModContext* ctx, void* player, int32_t action, int32_t default_state, void* user_data);

typedef struct ActionInputProviderDesc {
    uint32_t struct_size;
    ActionInputPlayerActionFn player_action;
    void* user_data;
} ActionInputProviderDesc;

#define ACTION_INPUT_PROVIDER_DESC_INIT {sizeof(ActionInputProviderDesc), NULL, NULL}

typedef struct ActionInputService {
    ServiceHeader header;

    /* Register an action-input provider. The latest active provider for a callback wins. */
    ModResult (*register_provider)(
        ModContext* ctx, const ActionInputProviderDesc* desc, ActionInputProviderHandle* out_handle);
    /* Unregister a provider owned by the calling mod. Providers are also removed on unload. */
    ModResult (*unregister_provider)(ModContext* ctx, ActionInputProviderHandle handle);
} ActionInputService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct mods::ServiceTraits<ActionInputService> {
    static constexpr const char* id = ACTION_INPUT_SERVICE_ID;
    static constexpr uint16_t major_version = ACTION_INPUT_SERVICE_MAJOR;
    static constexpr uint16_t minor_version = ACTION_INPUT_SERVICE_MINOR;
};
#endif
