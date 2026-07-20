#pragma once

#include <mods/api.h>

#define FILE_SELECT_SERVICE_ID "dev.twilitrealm.dusklight.file_select"
#define FILE_SELECT_SERVICE_MAJOR 1u
#define FILE_SELECT_SERVICE_MINOR 0u

typedef uint64_t FileSelectProviderHandle;

/* Called each file-select frame. Return non-zero after fully handling the frame. */
typedef int (*FileSelectUpdateFn)(ModContext* ctx, void* file_select, void* user_data);
/* Called before the host opens an empty slot. Return non-zero to replace the host flow. */
typedef int (*FileSelectOpenNewSlotFn)(ModContext* ctx, void* file_select, void* user_data);
/* Called before the host starts an existing slot. Return non-zero to replace the host flow. */
typedef int (*FileSelectStartExistingSlotFn)(ModContext* ctx, void* file_select, void* user_data);
/* Called after the player and companion names are confirmed for a new file. */
typedef void (*FileSelectNamesConfirmedFn)(ModContext* ctx, void* file_select, void* user_data);
/* Called when the active file-select scene is destroyed. */
typedef void (*FileSelectDestroyedFn)(ModContext* ctx, void* file_select, void* user_data);
/* Called before the name-entry scene starts gameplay. Return non-zero to replace the host start. */
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

    /* Register a file-select provider. The latest active provider for a callback wins. */
    ModResult (*register_provider)(
        ModContext* ctx, const FileSelectProviderDesc* desc, FileSelectProviderHandle* out_handle);
    /* Unregister a provider owned by the calling mod. Providers are also removed on unload. */
    ModResult (*unregister_provider)(ModContext* ctx, FileSelectProviderHandle handle);
} FileSelectService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct mods::ServiceTraits<FileSelectService> {
    static constexpr const char* id = FILE_SELECT_SERVICE_ID;
    static constexpr uint16_t major_version = FILE_SELECT_SERVICE_MAJOR;
    static constexpr uint16_t minor_version = FILE_SELECT_SERVICE_MINOR;
};
#endif
