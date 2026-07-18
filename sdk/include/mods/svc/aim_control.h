#pragma once

#include <mods/api.h>

#define AIM_CONTROL_SERVICE_ID "dev.twilitrealm.dusklight.aim_control"
#define AIM_CONTROL_SERVICE_MAJOR 1u
#define AIM_CONTROL_SERVICE_MINOR 0u

typedef uint64_t AimControlProviderHandle;

typedef enum DuskModAimItemKind {
    /* Used by host camera code when it is not tied to one item implementation. */
    DUSK_MOD_AIM_ITEM_ANY = -1,
    DUSK_MOD_AIM_ITEM_BOW = 0,
    DUSK_MOD_AIM_ITEM_BOOMERANG = 1,
    DUSK_MOD_AIM_ITEM_HOOKSHOT = 2,
    DUSK_MOD_AIM_ITEM_IRON_BALL = 3,
    DUSK_MOD_AIM_ITEM_COPY_ROD = 4,
} DuskModAimItemKind;

typedef enum DuskModAimCameraMode {
    /* Keep the host's default camera mode. */
    DUSK_MOD_AIM_CAMERA_DEFAULT = 0,
    DUSK_MOD_AIM_CAMERA_FIRST_PERSON = 1,
    DUSK_MOD_AIM_CAMERA_THIRD_PERSON = 2,
    DUSK_MOD_AIM_CAMERA_OVER_SHOULDER = 3,
} DuskModAimCameraMode;

typedef enum DuskModAimInputRouting {
    /* Keep the host's default stick/touch routing for the active aim mode. */
    DUSK_MOD_AIM_INPUT_DEFAULT = 0,
    /* Route movement through the left stick while aiming through C-stick/right touch/gyro. */
    DUSK_MOD_AIM_INPUT_LEFT_MOVE_RIGHT_AIM = 1,
} DuskModAimInputRouting;

typedef struct DuskModAimRequest {
    uint32_t struct_size;
    /* Controller pad index used by the host callsite, when available. */
    uint32_t pad;
    int32_t item_kind;
    /* Non-zero when the host is already in a scoped/first-person item view. */
    int32_t scoped;
    /* Non-zero when the host callsite supports alternate aim behavior. */
    int32_t supported;
} DuskModAimRequest;

/* Return a DuskModAimCameraMode for this item-aim request. */
typedef int (*AimControlCameraModeFn)(
    ModContext* ctx, const DuskModAimRequest* request, void* user_data);
/* Return a DuskModAimInputRouting for this item-aim request. */
typedef int (*AimControlInputRoutingFn)(
    ModContext* ctx, const DuskModAimRequest* request, void* user_data);
/* Return non-zero after replacing the host's item-specific subject-aim update. */
typedef int (*AimControlSubjectUpdateFn)(
    ModContext* ctx, void* player, int item_kind, void* user_data);

typedef struct AimControlProviderDesc {
    uint32_t struct_size;
    AimControlCameraModeFn camera_mode;
    AimControlInputRoutingFn input_routing;
    AimControlSubjectUpdateFn subject_update;
    void* user_data;
} AimControlProviderDesc;

#define AIM_CONTROL_PROVIDER_DESC_INIT {sizeof(AimControlProviderDesc), NULL, NULL, NULL, NULL}

typedef struct AimControlService {
    ServiceHeader header;

    /* Register an aim-control provider. The latest active provider for a callback wins. */
    ModResult (*register_provider)(
        ModContext* ctx, const AimControlProviderDesc* desc, AimControlProviderHandle* out_handle);
    /* Unregister a provider owned by the calling mod. Providers are also removed on unload. */
    ModResult (*unregister_provider)(ModContext* ctx, AimControlProviderHandle handle);
} AimControlService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct mods::ServiceTraits<AimControlService> {
    static constexpr const char* id = AIM_CONTROL_SERVICE_ID;
    static constexpr uint16_t major_version = AIM_CONTROL_SERVICE_MAJOR;
    static constexpr uint16_t minor_version = AIM_CONTROL_SERVICE_MINOR;
};
#endif
