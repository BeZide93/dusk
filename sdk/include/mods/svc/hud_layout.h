#pragma once

#include <mods/api.h>

#define HUD_LAYOUT_SERVICE_ID "dev.twilitrealm.dusklight.hud_layout"
#define HUD_LAYOUT_SERVICE_MAJOR 1u
#define HUD_LAYOUT_SERVICE_MINOR 0u

enum {
    DUSK_MOD_HUD_BUTTON_COUNT = 5,
    DUSK_MOD_HUD_ELEMENT_COUNT = 14,
};

typedef uint64_t HudLayoutProviderHandle;

typedef enum DuskModHudElementFlags {
    /* Hide this HUD element while preserving the host's normal update flow. */
    DUSK_MOD_HUD_ELEMENT_HIDDEN = 1u << 0,
} DuskModHudElementFlags;

typedef enum DuskModHudParentMode {
    /* Keep the host's original parent transform relationship. */
    DUSK_MOD_HUD_PARENT_DEFAULT = 0,
    /* Apply this element's transform independently of a related parent element. */
    DUSK_MOD_HUD_PARENT_INDEPENDENT = 1,
} DuskModHudParentMode;

typedef enum DuskModHudSlideDirection {
    /* Keep the host's minimap slide direction. */
    DUSK_MOD_HUD_SLIDE_DEFAULT = 0,
    DUSK_MOD_HUD_SLIDE_LEFT_TO_RIGHT = 1,
    DUSK_MOD_HUD_SLIDE_RIGHT_TO_LEFT = 2,
} DuskModHudSlideDirection;

typedef enum DuskModHudButtonStyleFlags {
    /* Draw a round face-button style when the host has a compatible button asset. */
    DUSK_MOD_HUD_BUTTON_STYLE_ROUND = 1u << 0,
} DuskModHudButtonStyleFlags;

typedef struct DuskModHudTransform {
    float offset_x;
    float offset_y;
    float scale;
    uint32_t flags;
    int32_t parent_mode;
    int32_t slide_direction;
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
    uint32_t style_flags;
} DuskModHudButtonLayout;

typedef struct DuskModHudLayoutSnapshot {
    uint32_t struct_size;
    /* Increment when the layout changes so the host can refresh cached transforms. */
    uint32_t revision;
    DuskModHudTransform elements[DUSK_MOD_HUD_ELEMENT_COUNT];
    DuskModHudButtonLayout buttons[DUSK_MOD_HUD_BUTTON_COUNT];
} DuskModHudLayoutSnapshot;

/* Return the current layout snapshot, or NULL to keep the host layout. */
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

    /* Unregister a provider owned by the calling mod. Providers are also removed on unload. */
    ModResult (*unregister_provider)(ModContext* ctx, HudLayoutProviderHandle handle);
} HudLayoutService;

#ifdef __cplusplus
#include "mods/service.hpp"

template <>
struct mods::ServiceTraits<HudLayoutService> {
    static constexpr const char* id = HUD_LAYOUT_SERVICE_ID;
    static constexpr uint16_t major_version = HUD_LAYOUT_SERVICE_MAJOR;
    static constexpr uint16_t minor_version = HUD_LAYOUT_SERVICE_MINOR;
};
#endif
