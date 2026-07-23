#pragma once

#include <mods/api.h>

#include <stddef.h>
#include <stdint.h>

#define TOUCH_CONTROLS_SERVICE_ID "dev.twilitrealm.dusklight.touch_controls"
#define TOUCH_CONTROLS_SERVICE_MAJOR 1u
#define TOUCH_CONTROLS_SERVICE_MINOR 1u

typedef uint64_t TouchControlsProviderHandle;

typedef enum DuskModTouchControl {
    DUSK_MOD_TOUCH_CONTROL_A = 0,
    DUSK_MOD_TOUCH_CONTROL_B = 1,
    DUSK_MOD_TOUCH_CONTROL_X = 2,
    DUSK_MOD_TOUCH_CONTROL_Y = 3,
    DUSK_MOD_TOUCH_CONTROL_Z = 4,
    DUSK_MOD_TOUCH_CONTROL_L = 5,
    DUSK_MOD_TOUCH_CONTROL_R = 6,
    DUSK_MOD_TOUCH_CONTROL_FIRST_PERSON = 7,
    DUSK_MOD_TOUCH_CONTROL_ITEMS = 8,
    DUSK_MOD_TOUCH_CONTROL_COLLECTIONS = 9,
    DUSK_MOD_TOUCH_CONTROL_MAP = 10,
    DUSK_MOD_TOUCH_CONTROL_SKIP = 11,
    DUSK_MOD_TOUCH_CONTROL_DPAD_UP = 12,
    DUSK_MOD_TOUCH_CONTROL_DPAD_DOWN = 13,
    DUSK_MOD_TOUCH_CONTROL_DPAD_LEFT = 14,
    DUSK_MOD_TOUCH_CONTROL_DPAD_RIGHT = 15,
} DuskModTouchControl;

typedef enum DuskModTouchControlAnchor {
    DUSK_MOD_TOUCH_ANCHOR_NONE = 0,
    DUSK_MOD_TOUCH_ANCHOR_TOP = 1,
    DUSK_MOD_TOUCH_ANCHOR_LEFT = 2,
    DUSK_MOD_TOUCH_ANCHOR_BOTTOM = 3,
    DUSK_MOD_TOUCH_ANCHOR_RIGHT = 4,
    DUSK_MOD_TOUCH_ANCHOR_TOP_LEFT = 5,
    DUSK_MOD_TOUCH_ANCHOR_TOP_RIGHT = 6,
    DUSK_MOD_TOUCH_ANCHOR_BOTTOM_LEFT = 7,
    DUSK_MOD_TOUCH_ANCHOR_BOTTOM_RIGHT = 8,
} DuskModTouchControlAnchor;

typedef enum DuskModTouchAimInputMode {
    DUSK_MOD_TOUCH_AIM_INPUT_DEFAULT = 0,
    DUSK_MOD_TOUCH_AIM_INPUT_SPLIT_STICKS = 1,
} DuskModTouchAimInputMode;

typedef struct TouchControlsControlDesc {
    uint32_t struct_size;
    const char* layout_id;
    const char* element_id;
    float x;
    float y;
    float w;
    float h;
    float scale;
    int32_t anchor;
    int32_t control;
    int has_control;
} TouchControlsControlDesc;

#define TOUCH_CONTROLS_CONTROL_DESC_INIT \
    {sizeof(TouchControlsControlDesc), NULL, NULL, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0, 0, 0}

typedef struct TouchControlsDisplayOverride {
    uint32_t struct_size;
    const char* icon_source;
    uint64_t icon_revision;
    int visible;
    int show_icon;
} TouchControlsDisplayOverride;

#define TOUCH_CONTROLS_DISPLAY_OVERRIDE_INIT \
    {sizeof(TouchControlsDisplayOverride), NULL, 0u, 0, 0}

typedef const char* (*TouchControlsExtraRmlFn)(ModContext* ctx, void* user_data);
typedef size_t (*TouchControlsExtraControlCountFn)(ModContext* ctx, void* user_data);
typedef int (*TouchControlsExtraControlAtFn)(
    ModContext* ctx, size_t index, TouchControlsControlDesc* out_control, void* user_data);
typedef int (*TouchControlsDisplayOverrideFn)(ModContext* ctx, int32_t control,
    TouchControlsDisplayOverride* out_override, void* user_data);
typedef uint16_t (*TouchControlsPadButtonFn)(
    ModContext* ctx, int32_t control, uint16_t fallback_button, void* user_data);
typedef void (*TouchControlsControlEventFn)(
    ModContext* ctx, int32_t control, int pressed, void* user_data);
typedef int32_t (*TouchControlsAimInputModeFn)(ModContext* ctx, void* user_data);

typedef struct TouchControlsProviderDesc {
    uint32_t struct_size;
    TouchControlsExtraRmlFn extra_rml;
    TouchControlsExtraControlCountFn extra_control_count;
    TouchControlsExtraControlAtFn extra_control_at;
    TouchControlsDisplayOverrideFn display_override;
    TouchControlsPadButtonFn pad_button;
    TouchControlsControlEventFn control_event;
    void* user_data;
    TouchControlsAimInputModeFn aim_input_mode;
} TouchControlsProviderDesc;

#define TOUCH_CONTROLS_PROVIDER_DESC_INIT \
    {sizeof(TouchControlsProviderDesc), NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}

typedef struct TouchControlsService {
    ServiceHeader header;

    ModResult (*register_provider)(ModContext* ctx, const TouchControlsProviderDesc* desc,
        TouchControlsProviderHandle* out_handle);
    ModResult (*unregister_provider)(ModContext* ctx, TouchControlsProviderHandle handle);
} TouchControlsService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct mods::ServiceTraits<TouchControlsService> {
    static constexpr const char* id = TOUCH_CONTROLS_SERVICE_ID;
    static constexpr uint16_t major_version = TOUCH_CONTROLS_SERVICE_MAJOR;
    static constexpr uint16_t minor_version = TOUCH_CONTROLS_SERVICE_MINOR;
};
#endif
