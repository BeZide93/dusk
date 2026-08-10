#include "config.hpp"
#include "service_imports.hpp"

#include "mods/service.hpp"
#include "mods/svc/config.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdint>

namespace dawnlight {
namespace {

ConfigVarHandle s_healthScale = 0;
ConfigVarHandle s_automaticHealthScale = 0;
ConfigVarHandle s_saveCompatibility = 0;
ConfigVarHandle s_itemIntegrity = 0;
ConfigVarHandle s_newSaveMode = 0;
ConfigVarHandle s_aimMode = 0;
ConfigVarHandle s_aimMovement = 0;
ConfigVarHandle s_manualShielding = 0;
ConfigVarHandle s_rJump = 0;
ConfigVarHandle s_zItemSlot = 0;
ConfigVarHandle s_hudLayout = 0;
ConfigVarHandle s_legacyWiiUHud = 0;
ConfigVarHandle s_roundXYButtons = 0;
ConfigVarHandle s_aimDefaultsMigrated = 0;
ConfigVarHandle s_hudLayoutMigrated = 0;

constexpr size_t kHudElementCount = static_cast<size_t>(HudElement::Count);
constexpr size_t kHudButtonCount = static_cast<size_t>(HudButton::Count);

std::array<ConfigVarHandle, kHudElementCount> s_hudElementX = {};
std::array<ConfigVarHandle, kHudElementCount> s_hudElementY = {};
std::array<ConfigVarHandle, kHudElementCount> s_hudElementScale = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonItemOffsetX = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonItemOffsetY = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonItemScale = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonAmmoOffsetX = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonAmmoOffsetY = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonAmmoScale = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonTextOffsetX = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonTextOffsetY = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonTextScale = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonItemAnchor = {};
std::array<ConfigVarHandle, kHudButtonCount> s_hudButtonTextAnchor = {};
ConfigVarHandle s_hudDpadFollowsMinimap = 0;
ConfigVarHandle s_hudMinimapSlideDirection = 0;

struct HudElementDefaults {
    const char* name;
    int x;
    int y;
    int scale;
};

struct HudButtonDefaults {
    const char* name;
    int itemOffsetX;
    int itemOffsetY;
    int itemScale;
    int ammoOffsetX;
    int ammoOffsetY;
    int ammoScale;
    int textOffsetX;
    int textOffsetY;
    int textScale;
    int itemAnchor;
    int textAnchor;
};

constexpr std::array<HudElementDefaults, kHudElementCount> kHudElementDefaults = {{
    {"a", -35, 25, 100},
    {"b", 20, -27, 150},
    {"x", -102, -1, 170},
    {"y", -22, 0, 170},
    {"z", 0, 0, 100},
    {"button-backing", -100, 0, 100},
    {"dpad", 0, -280, 100},
    {"midna", -6, 0, 100},
    {"hearts", 0, 0, 100},
    {"rupees", 0, 0, 100},
    {"keys", 0, 0, 100},
    {"oil", 0, 0, 100},
    {"oxygen", 0, 0, 100},
    {"minimap", 0, 50, 70},
}};

constexpr std::array<HudButtonDefaults, kHudButtonCount> kHudButtonDefaults = {{
    {"a", 0, 0, 100, 0, 0, 100, 0, 0, 100, 1, 1},
    {"b", 30, 0, 50, 0, 0, 100, 10, 0, 50, 2, 1},
    {"x", -15, -15, 50, 0, 0, 100, -15, -15, 50, 0, 0},
    {"y", 0, 0, 50, 0, 0, 100, 15, 0, 50, 2, 0},
    {"z", 0, 0, 100, 0, 0, 100, 0, 0, 100, 1, 0},
}};

size_t hud_element_index(HudElement element) {
    return std::clamp<size_t>(static_cast<size_t>(element), 0, kHudElementCount - 1);
}

size_t hud_button_index(HudButton button) {
    return std::clamp<size_t>(static_cast<size_t>(button), 0, kHudButtonCount - 1);
}

ModResult register_bool(const char* name, bool defaultValue, ConfigVarHandle& handle) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = name;
    desc.type = CONFIG_VAR_BOOL;
    desc.default_bool = defaultValue;
    return svc_config->register_var(mod_ctx, &desc, &handle);
}

ModResult register_int(const char* name, int64_t defaultValue, ConfigVarHandle& handle) {
    ConfigVarDesc desc = CONFIG_VAR_DESC_INIT;
    desc.name = name;
    desc.type = CONFIG_VAR_INT;
    desc.default_int = defaultValue;
    return svc_config->register_var(mod_ctx, &desc, &handle);
}

ModResult register_custom_int(
    const char* category, const char* name, const char* field, int defaultValue,
    ConfigVarHandle& handle) {
    char key[64];
    std::snprintf(key, sizeof(key), "hud-custom-%s-%s-%s", category, name, field);
    return register_int(key, defaultValue, handle);
}

bool get_bool(ConfigVarHandle handle, bool fallback) {
    bool value = fallback;
    if (handle != 0) {
        svc_config->get_bool(mod_ctx, handle, &value);
    }
    return value;
}

int get_int(ConfigVarHandle handle, int fallback, int min, int max) {
    int64_t value = fallback;
    if (handle != 0) {
        svc_config->get_int(mod_ctx, handle, &value);
    }
    return static_cast<int>(std::clamp<int64_t>(value, min, max));
}

ModResult register_custom_hud_config() {
    for (size_t i = 0; i < kHudElementCount; ++i) {
        const auto& defaults = kHudElementDefaults[i];
        if (register_custom_int("element", defaults.name, "x", defaults.x, s_hudElementX[i]) !=
                MOD_OK ||
            register_custom_int("element", defaults.name, "y", defaults.y, s_hudElementY[i]) !=
                MOD_OK ||
            register_custom_int(
                "element", defaults.name, "scale", defaults.scale, s_hudElementScale[i]) !=
                MOD_OK)
        {
            return MOD_ERROR;
        }
    }

    for (size_t i = 0; i < kHudButtonCount; ++i) {
        const auto& defaults = kHudButtonDefaults[i];
        if (register_custom_int("button", defaults.name, "item-x", defaults.itemOffsetX,
                s_hudButtonItemOffsetX[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "item-y", defaults.itemOffsetY,
                s_hudButtonItemOffsetY[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "item-scale", defaults.itemScale,
                s_hudButtonItemScale[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "ammo-x", defaults.ammoOffsetX,
                s_hudButtonAmmoOffsetX[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "ammo-y", defaults.ammoOffsetY,
                s_hudButtonAmmoOffsetY[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "ammo-scale", defaults.ammoScale,
                s_hudButtonAmmoScale[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "text-x", defaults.textOffsetX,
                s_hudButtonTextOffsetX[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "text-y", defaults.textOffsetY,
                s_hudButtonTextOffsetY[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "text-scale", defaults.textScale,
                s_hudButtonTextScale[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "item-anchor", defaults.itemAnchor,
                s_hudButtonItemAnchor[i]) != MOD_OK ||
            register_custom_int("button", defaults.name, "text-anchor", defaults.textAnchor,
                s_hudButtonTextAnchor[i]) != MOD_OK)
        {
            return MOD_ERROR;
        }
    }

    if (register_bool("hud-custom-dpad-follows-minimap", false, s_hudDpadFollowsMinimap) !=
            MOD_OK ||
        register_int("hud-custom-minimap-slide-direction", 0, s_hudMinimapSlideDirection) !=
            MOD_OK)
    {
        return MOD_ERROR;
    }

    return MOD_OK;
}

}  // namespace

ModResult register_config(ModError* error) {
    if (register_int("hp-scale-percent", 100, s_healthScale) != MOD_OK ||
        register_bool("ngplus-auto-hp-scaling", true, s_automaticHealthScale) != MOD_OK ||
        register_bool("save-compatibility", true, s_saveCompatibility) != MOD_OK ||
        register_bool("item-integrity-fixes", true, s_itemIntegrity) != MOD_OK ||
        register_int("new-save-mode", 0, s_newSaveMode) != MOD_OK ||
        register_int("aim-mode", 2, s_aimMode) != MOD_OK ||
        register_bool("aim-movement", true, s_aimMovement) != MOD_OK ||
        register_bool("manual-shielding", true, s_manualShielding) != MOD_OK ||
        register_bool("r-jump", true, s_rJump) != MOD_OK ||
        register_bool("z-item-slot", true, s_zItemSlot) != MOD_OK ||
        register_int("hud-layout", static_cast<int64_t>(HudLayout::GameCube), s_hudLayout) !=
            MOD_OK ||
        register_bool("wii-u-hud", false, s_legacyWiiUHud) != MOD_OK ||
        register_bool("round-xy-buttons", false, s_roundXYButtons) != MOD_OK ||
        register_bool("aim-defaults-v2", false, s_aimDefaultsMigrated) != MOD_OK ||
        register_bool("hud-layout-migrated-v1", false, s_hudLayoutMigrated) != MOD_OK)
    {
        return mods::set_error(error, MOD_ERROR, "failed to register Dawnlight config variables");
    }
    if (register_custom_hud_config() != MOD_OK) {
        return mods::set_error(
            error, MOD_ERROR, "failed to register Dawnlight custom HUD variables");
    }

    if (!get_bool(s_aimDefaultsMigrated, false)) {
        if (svc_config->set_int(mod_ctx, s_aimMode, static_cast<int64_t>(AimMode::Cinema)) != MOD_OK ||
            svc_config->set_bool(mod_ctx, s_aimMovement, true) != MOD_OK ||
            svc_config->set_bool(mod_ctx, s_aimDefaultsMigrated, true) != MOD_OK)
        {
            return mods::set_error(
                error, MOD_ERROR, "failed to migrate Dawnlight aim defaults");
        }
    }

    if (!get_bool(s_hudLayoutMigrated, false)) {
        if (get_bool(s_legacyWiiUHud, false) &&
            svc_config->set_int(mod_ctx, s_hudLayout,
                static_cast<int64_t>(HudLayout::Dawnlight)) != MOD_OK)
        {
            return mods::set_error(
                error, MOD_ERROR, "failed to migrate Dawnlight HUD layout");
        }
        if (svc_config->set_bool(mod_ctx, s_hudLayoutMigrated, true) != MOD_OK) {
            return mods::set_error(
                error, MOD_ERROR, "failed to finish Dawnlight HUD layout migration");
        }
    }
    return MOD_OK;
}

int health_scale_percent() {
    int64_t value = 100;
    if (s_healthScale != 0) {
        svc_config->get_int(mod_ctx, s_healthScale, &value);
    }
    return static_cast<int>(std::clamp<int64_t>(value, 1, 9999));
}

bool automatic_ngplus_health_scaling() {
    return get_bool(s_automaticHealthScale, true);
}

bool save_compatibility_enabled() {
    return get_bool(s_saveCompatibility, true);
}

bool item_integrity_fixes_enabled() {
    return get_bool(s_itemIntegrity, true);
}

NewSaveMode new_save_mode() {
    int64_t value = static_cast<int64_t>(NewSaveMode::Vanilla);
    if (s_newSaveMode != 0) {
        svc_config->get_int(mod_ctx, s_newSaveMode, &value);
    }
    return static_cast<NewSaveMode>(std::clamp<int64_t>(value, 0, 2));
}

AimMode aim_mode() {
    int64_t value = static_cast<int64_t>(AimMode::Cinema);
    if (s_aimMode != 0) {
        svc_config->get_int(mod_ctx, s_aimMode, &value);
    }
    return static_cast<AimMode>(std::clamp<int64_t>(value, 0, 2));
}

bool aim_movement_enabled() {
    return get_bool(s_aimMovement, true);
}

bool manual_shielding_enabled() {
    return get_bool(s_manualShielding, true);
}

bool r_jump_enabled() {
    return get_bool(s_rJump, true);
}

bool z_item_slot_enabled() {
    return get_bool(s_zItemSlot, true);
}

HudLayout hud_layout() {
    int64_t value = static_cast<int64_t>(HudLayout::GameCube);
    if (s_hudLayout != 0) {
        svc_config->get_int(mod_ctx, s_hudLayout, &value);
    }
    return static_cast<HudLayout>(std::clamp<int64_t>(value, 0, 3));
}

bool hardcoded_hud_layout_enabled() {
    return hud_layout() != HudLayout::GameCube;
}

bool custom_hud_layout_enabled() {
    return hud_layout() == HudLayout::Custom;
}

bool round_xy_buttons_enabled() {
    return get_bool(s_roundXYButtons, false);
}

int hud_custom_element_x(HudElement element) {
    const size_t index = hud_element_index(element);
    return get_int(s_hudElementX[index], kHudElementDefaults[index].x, -9999, 9999);
}

int hud_custom_element_y(HudElement element) {
    const size_t index = hud_element_index(element);
    return get_int(s_hudElementY[index], kHudElementDefaults[index].y, -9999, 9999);
}

int hud_custom_element_scale_percent(HudElement element) {
    const size_t index = hud_element_index(element);
    return get_int(s_hudElementScale[index], kHudElementDefaults[index].scale, 1, 9999);
}

int hud_custom_button_item_offset_x(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonItemOffsetX[index], kHudButtonDefaults[index].itemOffsetX, -9999, 9999);
}

int hud_custom_button_item_offset_y(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonItemOffsetY[index], kHudButtonDefaults[index].itemOffsetY, -9999, 9999);
}

int hud_custom_button_item_scale_percent(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonItemScale[index], kHudButtonDefaults[index].itemScale, 1, 9999);
}

int hud_custom_button_ammo_offset_x(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonAmmoOffsetX[index], kHudButtonDefaults[index].ammoOffsetX, -9999, 9999);
}

int hud_custom_button_ammo_offset_y(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonAmmoOffsetY[index], kHudButtonDefaults[index].ammoOffsetY, -9999, 9999);
}

int hud_custom_button_ammo_scale_percent(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonAmmoScale[index], kHudButtonDefaults[index].ammoScale, 1, 9999);
}

int hud_custom_button_text_offset_x(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonTextOffsetX[index], kHudButtonDefaults[index].textOffsetX, -9999, 9999);
}

int hud_custom_button_text_offset_y(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonTextOffsetY[index], kHudButtonDefaults[index].textOffsetY, -9999, 9999);
}

int hud_custom_button_text_scale_percent(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(
        s_hudButtonTextScale[index], kHudButtonDefaults[index].textScale, 1, 9999);
}

int hud_custom_button_item_anchor(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(s_hudButtonItemAnchor[index], kHudButtonDefaults[index].itemAnchor, 0, 3);
}

int hud_custom_button_text_anchor(HudButton button) {
    const size_t index = hud_button_index(button);
    return get_int(s_hudButtonTextAnchor[index], kHudButtonDefaults[index].textAnchor, 0, 1);
}

bool hud_custom_dpad_follows_minimap() {
    return get_bool(s_hudDpadFollowsMinimap, false);
}

int hud_custom_minimap_slide_direction() {
    return get_int(s_hudMinimapSlideDirection, 0, 0, 1);
}

ConfigVarHandle health_scale_config_var() {
    return s_healthScale;
}

ConfigVarHandle automatic_health_scale_config_var() {
    return s_automaticHealthScale;
}

ConfigVarHandle save_compatibility_config_var() {
    return s_saveCompatibility;
}

ConfigVarHandle item_integrity_config_var() {
    return s_itemIntegrity;
}

ConfigVarHandle new_save_mode_config_var() {
    return s_newSaveMode;
}

ConfigVarHandle aim_mode_config_var() {
    return s_aimMode;
}

ConfigVarHandle aim_movement_config_var() {
    return s_aimMovement;
}

ConfigVarHandle manual_shielding_config_var() {
    return s_manualShielding;
}

ConfigVarHandle r_jump_config_var() {
    return s_rJump;
}

ConfigVarHandle z_item_slot_config_var() {
    return s_zItemSlot;
}

ConfigVarHandle hud_layout_config_var() {
    return s_hudLayout;
}

ConfigVarHandle round_xy_buttons_config_var() {
    return s_roundXYButtons;
}

ConfigVarHandle hud_custom_element_x_config_var(HudElement element) {
    return s_hudElementX[hud_element_index(element)];
}

ConfigVarHandle hud_custom_element_y_config_var(HudElement element) {
    return s_hudElementY[hud_element_index(element)];
}

ConfigVarHandle hud_custom_element_scale_config_var(HudElement element) {
    return s_hudElementScale[hud_element_index(element)];
}

ConfigVarHandle hud_custom_button_item_offset_x_config_var(HudButton button) {
    return s_hudButtonItemOffsetX[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_item_offset_y_config_var(HudButton button) {
    return s_hudButtonItemOffsetY[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_item_scale_config_var(HudButton button) {
    return s_hudButtonItemScale[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_ammo_offset_x_config_var(HudButton button) {
    return s_hudButtonAmmoOffsetX[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_ammo_offset_y_config_var(HudButton button) {
    return s_hudButtonAmmoOffsetY[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_ammo_scale_config_var(HudButton button) {
    return s_hudButtonAmmoScale[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_text_offset_x_config_var(HudButton button) {
    return s_hudButtonTextOffsetX[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_text_offset_y_config_var(HudButton button) {
    return s_hudButtonTextOffsetY[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_text_scale_config_var(HudButton button) {
    return s_hudButtonTextScale[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_item_anchor_config_var(HudButton button) {
    return s_hudButtonItemAnchor[hud_button_index(button)];
}

ConfigVarHandle hud_custom_button_text_anchor_config_var(HudButton button) {
    return s_hudButtonTextAnchor[hud_button_index(button)];
}

ConfigVarHandle hud_custom_dpad_follows_minimap_config_var() {
    return s_hudDpadFollowsMinimap;
}

ConfigVarHandle hud_custom_minimap_slide_direction_config_var() {
    return s_hudMinimapSlideDirection;
}

}  // namespace dawnlight
