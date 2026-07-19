#include "bossrush.hpp"
#include "config.hpp"
#include "save_compat.hpp"

#include "mods/service.hpp"
#include "mods/svc/game.h"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"

DEFINE_MOD();
IMPORT_SERVICE(GameService, svc_game);
IMPORT_SERVICE(LogService, svc_log);

namespace dawnlight {
ModResult install_aim_hooks(ModError* error);
ModResult install_bossrush_hooks(ModError* error);
ModResult install_enemy_scaling_hooks(ModError* error);
ModResult install_file_select_hooks(ModError* error);
ModResult install_item_integrity_hooks(ModError* error);
ModResult install_item_slot_hooks(ModError* error);
ModResult install_jump_hooks(ModError* error);
ModResult install_manual_shield_hooks(ModError* error);
ModResult register_ui(ModError* error);
}

extern "C" {

MOD_EXPORT ModResult mod_initialize(ModError* error) {
    if (const ModResult result = dawnlight::register_config(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::register_ui(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_save_compat_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_file_select_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_bossrush_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_aim_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_item_integrity_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_item_slot_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_manual_shield_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_jump_hooks(error); result != MOD_OK) {
        return result;
    }
    if (const ModResult result = dawnlight::install_enemy_scaling_hooks(error); result != MOD_OK) {
        return result;
    }

    svc_log->info(mod_ctx, "Dawnlight portable feature pack initialized");
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    return MOD_OK;
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    svc_log->info(mod_ctx, "Dawnlight portable feature pack stopped");
    return MOD_OK;
}

}  // extern "C"
