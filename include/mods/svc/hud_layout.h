#pragma once

#include "mods/api.h"

#define HUD_LAYOUT_SERVICE_ID "dev.twilitrealm.dusklight.hud_layout"
#define HUD_LAYOUT_SERVICE_MAJOR 1u
#define HUD_LAYOUT_SERVICE_MINOR 0u

enum {
    DUSK_MOD_HUD_BUTTON_COUNT = 5,
    DUSK_MOD_HUD_ELEMENT_COUNT = 14,
};

typedef uint64_t HudLayoutProviderHandle;

typedef struct DuskModHudTransform {
    float offset_x;
    float offset_y;
    float scale;
} DuskModHudTransform;

typedef struct DuskModHudButtonLayout {
    int32_t item_anchor;
    int32_t text_anchor;
    float item_scale;
    float item_offset_x;
    float item_offset_y;
    float ammo_offset_x;
    float ammo_offset_y;
    float ammo_scale;
    float text_scale;
    float text_offset_x;
    float text_offset_y;
} DuskModHudButtonLayout;

typedef struct DuskModHudLayoutSnapshot {
    uint32_t struct_size;
    uint32_t revision;
    int32_t button_background;
    int32_t round_xy_buttons;
    int32_t dpad_follows_minimap;
    int32_t minimap_slide_direction;
    DuskModHudTransform elements[DUSK_MOD_HUD_ELEMENT_COUNT];
    DuskModHudButtonLayout buttons[DUSK_MOD_HUD_BUTTON_COUNT];
} DuskModHudLayoutSnapshot;

typedef const DuskModHudLayoutSnapshot* (*HudLayoutProviderFn)(
    ModContext* ctx, const char* data_path, void* user_data);

typedef struct HudLayoutProviderDesc {
    uint32_t struct_size;
    HudLayoutProviderFn get_layout;
    void* user_data;
} HudLayoutProviderDesc;

#define HUD_LAYOUT_PROVIDER_DESC_INIT {sizeof(HudLayoutProviderDesc), NULL, NULL}

typedef struct HudLayoutService {
    ServiceHeader header;

    /*
     * Registers a render-time HUD layout provider for the calling mod. The latest active
     * registration wins. The provider is removed automatically when the mod is disabled,
     * reloaded, or fails.
     */
    ModResult (*register_provider)(
        ModContext* ctx, const HudLayoutProviderDesc* desc, HudLayoutProviderHandle* out_handle);

    ModResult (*unregister_provider)(ModContext* ctx, HudLayoutProviderHandle handle);
} HudLayoutService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct dusk::mods::ServiceTraits<HudLayoutService> {
    static constexpr const char* id = HUD_LAYOUT_SERVICE_ID;
    static constexpr uint16_t major_version = HUD_LAYOUT_SERVICE_MAJOR;
};
#endif
