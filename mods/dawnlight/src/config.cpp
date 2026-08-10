#include "config.hpp"
#include "service_imports.hpp"

#include "mods/service.hpp"
#include "mods/svc/config.h"

#include <algorithm>
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

bool get_bool(ConfigVarHandle handle, bool fallback) {
    bool value = fallback;
    if (handle != 0) {
        svc_config->get_bool(mod_ctx, handle, &value);
    }
    return value;
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
    return static_cast<HudLayout>(std::clamp<int64_t>(value, 0, 2));
}

bool hardcoded_hud_layout_enabled() {
    return hud_layout() != HudLayout::GameCube;
}

bool round_xy_buttons_enabled() {
    return get_bool(s_roundXYButtons, false);
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

}  // namespace dawnlight
