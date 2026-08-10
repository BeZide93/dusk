#pragma once

#include "mods/api.h"
#include "mods/svc/config.h"

namespace dawnlight {

enum class AimMode : int {
    Vanilla = 0,
    ThirdPerson = 1,
    Cinema = 2,
};

enum class NewSaveMode : int {
    Vanilla = 0,
    IntroSkip = 1,
    BossRush = 2,
};

enum class HudLayout : int {
    GameCube = 0,
    WiiU = 1,
    Dawnlight = 2,
};

ModResult register_config(ModError* error);
int health_scale_percent();
bool automatic_ngplus_health_scaling();
bool save_compatibility_enabled();
bool item_integrity_fixes_enabled();
NewSaveMode new_save_mode();
AimMode aim_mode();
bool aim_movement_enabled();
bool manual_shielding_enabled();
bool r_jump_enabled();
bool z_item_slot_enabled();
HudLayout hud_layout();
bool hardcoded_hud_layout_enabled();
bool round_xy_buttons_enabled();

ConfigVarHandle health_scale_config_var();
ConfigVarHandle automatic_health_scale_config_var();
ConfigVarHandle save_compatibility_config_var();
ConfigVarHandle item_integrity_config_var();
ConfigVarHandle new_save_mode_config_var();
ConfigVarHandle aim_mode_config_var();
ConfigVarHandle aim_movement_config_var();
ConfigVarHandle manual_shielding_config_var();
ConfigVarHandle r_jump_config_var();
ConfigVarHandle z_item_slot_config_var();
ConfigVarHandle hud_layout_config_var();
ConfigVarHandle round_xy_buttons_config_var();

}  // namespace dawnlight
