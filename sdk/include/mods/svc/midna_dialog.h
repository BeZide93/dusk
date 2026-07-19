#pragma once

#include <mods/api.h>

#define MIDNA_DIALOG_SERVICE_ID "dev.twilitrealm.dusklight.midna_dialog"
#define MIDNA_DIALOG_SERVICE_MAJOR 1u
#define MIDNA_DIALOG_SERVICE_MINOR 0u

typedef uint64_t MidnaDialogProviderHandle;

/* Return custom prompt text near a world interaction, or NULL to keep the host prompt. */
typedef const char* (*MidnaDialogPromptTextFn)(ModContext* ctx, void* user_data);
/* Called when opening a custom world-interaction prompt. Return non-zero after handling it. */
typedef int (*MidnaDialogPromptBeginFn)(ModContext* ctx, void* user_data);
/* Resolve a custom world-interaction prompt choice. Return non-zero after handling it. */
typedef int (*MidnaDialogPromptResolveFn)(ModContext* ctx, int choice, void* user_data);
/* Consume a completed prompt resolution. Return non-zero when one was consumed. */
typedef int (*MidnaDialogPromptConsumeResolutionFn)(ModContext* ctx, void* user_data);
/* Return an extra Midna menu option, or NULL to keep the host menu unchanged. */
typedef const char* (*MidnaDialogMenuOptionFn)(ModContext* ctx, void* user_data);
/* Called when opening a custom Midna menu option. Return non-zero after handling it. */
typedef int (*MidnaDialogMenuBeginFn)(ModContext* ctx, void* user_data);
/* Resolve a custom Midna menu choice. Return non-zero after handling it. */
typedef int (*MidnaDialogMenuResolveFn)(ModContext* ctx, int choice, void* user_data);
/* Cancel a custom Midna menu choice. Return non-zero after handling it. */
typedef int (*MidnaDialogMenuCancelFn)(ModContext* ctx, void* user_data);
/* Execute a pending custom Midna menu action. Return non-zero after replacing the host action. */
typedef int (*MidnaDialogMenuExecuteActionFn)(ModContext* ctx, void* player, void* user_data);

typedef struct MidnaDialogProviderDesc {
    uint32_t struct_size;
    MidnaDialogPromptTextFn prompt_text;
    MidnaDialogPromptBeginFn prompt_begin;
    MidnaDialogPromptResolveFn prompt_resolve;
    MidnaDialogPromptConsumeResolutionFn prompt_consume_resolution;
    MidnaDialogMenuOptionFn menu_option;
    MidnaDialogMenuBeginFn menu_begin;
    MidnaDialogMenuResolveFn menu_resolve;
    MidnaDialogMenuCancelFn menu_cancel;
    MidnaDialogMenuExecuteActionFn menu_execute_action;
    void* user_data;
} MidnaDialogProviderDesc;

#define MIDNA_DIALOG_PROVIDER_DESC_INIT \
    {sizeof(MidnaDialogProviderDesc), NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}

typedef struct MidnaDialogService {
    ServiceHeader header;

    /* Register a Midna-dialog provider. The latest active provider for a callback wins. */
    ModResult (*register_provider)(ModContext* ctx, const MidnaDialogProviderDesc* desc,
        MidnaDialogProviderHandle* out_handle);
    /* Unregister a provider owned by the calling mod. Providers are also removed on unload. */
    ModResult (*unregister_provider)(ModContext* ctx, MidnaDialogProviderHandle handle);
} MidnaDialogService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct mods::ServiceTraits<MidnaDialogService> {
    static constexpr const char* id = MIDNA_DIALOG_SERVICE_ID;
    static constexpr uint16_t major_version = MIDNA_DIALOG_SERVICE_MAJOR;
    static constexpr uint16_t minor_version = MIDNA_DIALOG_SERVICE_MINOR;
};
#endif
