#pragma once

#include "mods/api.h"

#define MIDNA_DIALOG_SERVICE_ID "dev.twilitrealm.dusklight.midna_dialog"
#define MIDNA_DIALOG_SERVICE_MAJOR 1u
#define MIDNA_DIALOG_SERVICE_MINOR 0u

typedef uint64_t MidnaDialogProviderHandle;

typedef const char* (*MidnaDialogPromptTextFn)(ModContext* ctx, void* user_data);
typedef int (*MidnaDialogPromptBeginFn)(ModContext* ctx, void* user_data);
typedef int (*MidnaDialogPromptResolveFn)(ModContext* ctx, int choice, void* user_data);
typedef int (*MidnaDialogPromptConsumeResolutionFn)(ModContext* ctx, void* user_data);
typedef const char* (*MidnaDialogMenuOptionFn)(ModContext* ctx, void* user_data);
typedef int (*MidnaDialogMenuBeginFn)(ModContext* ctx, void* user_data);
typedef int (*MidnaDialogMenuResolveFn)(ModContext* ctx, int choice, void* user_data);
typedef int (*MidnaDialogMenuCancelFn)(ModContext* ctx, void* user_data);
typedef int (*MidnaDialogMenuExecuteWarpFn)(ModContext* ctx, void* player, void* user_data);

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
    MidnaDialogMenuExecuteWarpFn menu_execute_warp;
    void* user_data;
} MidnaDialogProviderDesc;

#define MIDNA_DIALOG_PROVIDER_DESC_INIT \
    {sizeof(MidnaDialogProviderDesc), NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}

typedef struct MidnaDialogService {
    ServiceHeader header;

    ModResult (*register_provider)(ModContext* ctx, const MidnaDialogProviderDesc* desc,
        MidnaDialogProviderHandle* out_handle);
    ModResult (*unregister_provider)(ModContext* ctx, MidnaDialogProviderHandle handle);
} MidnaDialogService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct dusk::mods::ServiceTraits<MidnaDialogService> {
    static constexpr const char* id = MIDNA_DIALOG_SERVICE_ID;
    static constexpr uint16_t major_version = MIDNA_DIALOG_SERVICE_MAJOR;
};
#endif
