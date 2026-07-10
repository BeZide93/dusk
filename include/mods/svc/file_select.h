#pragma once

#include "mods/api.h"

#define FILE_SELECT_SERVICE_ID "dev.twilitrealm.dusklight.file_select"
#define FILE_SELECT_SERVICE_MAJOR 1u
#define FILE_SELECT_SERVICE_MINOR 0u

typedef uint64_t FileSelectProviderHandle;

typedef int (*FileSelectUpdateFn)(ModContext* ctx, void* file_select, void* user_data);
typedef int (*FileSelectOpenNewSlotFn)(ModContext* ctx, void* file_select, void* user_data);
typedef int (*FileSelectStartExistingSlotFn)(ModContext* ctx, void* file_select, void* user_data);
typedef void (*FileSelectNamesConfirmedFn)(ModContext* ctx, void* file_select, void* user_data);
typedef void (*FileSelectDestroyedFn)(ModContext* ctx, void* file_select, void* user_data);
typedef int (*FileSelectStartStageFn)(ModContext* ctx, void* name_scene, void* user_data);

typedef struct FileSelectProviderDesc {
    uint32_t struct_size;
    FileSelectUpdateFn update;
    FileSelectOpenNewSlotFn open_new_slot;
    FileSelectStartExistingSlotFn start_existing_slot;
    FileSelectNamesConfirmedFn names_confirmed;
    FileSelectDestroyedFn destroyed;
    FileSelectStartStageFn start_stage;
    void* user_data;
} FileSelectProviderDesc;

#define FILE_SELECT_PROVIDER_DESC_INIT {sizeof(FileSelectProviderDesc), NULL, NULL, NULL, NULL, NULL, NULL, NULL}

typedef struct FileSelectService {
    ServiceHeader header;

    ModResult (*register_provider)(
        ModContext* ctx, const FileSelectProviderDesc* desc, FileSelectProviderHandle* out_handle);
    ModResult (*unregister_provider)(ModContext* ctx, FileSelectProviderHandle handle);
} FileSelectService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct dusk::mods::ServiceTraits<FileSelectService> {
    static constexpr const char* id = FILE_SELECT_SERVICE_ID;
    static constexpr uint16_t major_version = FILE_SELECT_SERVICE_MAJOR;
};
#endif
