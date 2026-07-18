#include "config.hpp"

#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/service.hpp"
#include "mods/svc/action_input.h"

IMPORT_SERVICE(ActionInputService, svc_action_input);

namespace dawnlight {
namespace {

ActionInputProviderHandle s_provider = 0;

bool switch_target_active(daAlink_c* link) {
    return dComIfGs_getOptAttentionType() == 1 &&
           link != nullptr &&
           link->mAttention != nullptr &&
           link->mAttention->LockonTruth() &&
           (link->mTargetedActor != nullptr || link->mAttention->LockonTarget(0) != nullptr);
}

bool manual_shield_button(daAlink_c* link) {
    if (!manual_shielding_enabled() || link == nullptr || !mDoCPd_c::getHoldLockR(PAD_1)) {
        return false;
    }

    return mDoCPd_c::getHoldLockL(PAD_1) != 0 || switch_target_active(link);
}

int player_action(ModContext*, void* player, int action, int, void*) {
    if (action != DUSK_MOD_PLAYER_ACTION_GUARD || !manual_shielding_enabled()) {
        return DUSK_MOD_ACTION_INPUT_DEFAULT;
    }
    return manual_shield_button(static_cast<daAlink_c*>(player)) ?
        DUSK_MOD_ACTION_INPUT_PRESSED :
        DUSK_MOD_ACTION_INPUT_RELEASED;
}

}  // namespace

ModResult install_manual_shield_hooks(ModError* error) {
    ActionInputProviderDesc desc = ACTION_INPUT_PROVIDER_DESC_INIT;
    desc.player_action = player_action;

    const ModResult result = svc_action_input->register_provider(mod_ctx, &desc, &s_provider);
    if (result != MOD_OK) {
        return mods::set_error(
            error, result, "failed to register Dawnlight action-input provider");
    }
    return MOD_OK;
}

}  // namespace dawnlight
