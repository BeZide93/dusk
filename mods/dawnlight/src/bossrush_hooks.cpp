#include "bossrush.hpp"

#include "m_Do/m_Do_ext.h"
class JPABaseEmitter;
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_obj_bosswarp.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "mods/svc/midna_dialog.h"
#include "mods/svc/stage_flow.h"

IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(MidnaDialogService, svc_midna_dialog);
IMPORT_SERVICE(StageFlowService, svc_stage_flow);

namespace dawnlight {
namespace {

MidnaDialogProviderHandle s_midna_dialog_provider = 0;
StageFlowProviderHandle s_stage_flow_provider = 0;

DEFINE_HOOK_SYMBOL("dScnPly_Execute", int(void*), PlaySceneUpdateHook);

void update_bossrush(ModContext*, void*, void*, void*) {
    bossrush::update();
}

int transition_actor_update(ModContext*, void* actor, int actor_kind, void*) {
    if (actor_kind != DUSK_MOD_STAGE_FLOW_TRANSITION_ACTOR_BOSS_WARP) {
        return DUSK_MOD_STAGE_FLOW_TRANSITION_DEFAULT;
    }

    auto* warp = static_cast<daObjBossWarp_c*>(actor);
    if (warp == nullptr || !bossrush::is_hub_stage()) {
        return DUSK_MOD_STAGE_FLOW_TRANSITION_DEFAULT;
    }
    return bossrush::is_hub_center_portal(warp->getSceneListNo()) ?
        DUSK_MOD_STAGE_FLOW_TRANSITION_ACTIVE :
        DUSK_MOD_STAGE_FLOW_TRANSITION_STATIC;
}

int sequence_complete(ModContext*, int sequence_kind, void*) {
    if (sequence_kind != DUSK_MOD_STAGE_FLOW_SEQUENCE_FINAL_BATTLE) {
        return 0;
    }
    return bossrush::complete_ganondorf_sequence();
}

const char* prompt_text(ModContext*, void*) {
    return bossrush::has_hub_midna_prompt() ? bossrush::hub_midna_prompt_text() : nullptr;
}

int prompt_begin(ModContext*, void*) {
    return bossrush::begin_hub_midna_prompt();
}

int prompt_resolve(ModContext*, int choice, void*) {
    return bossrush::resolve_hub_midna_prompt(choice);
}

int prompt_consume_resolution(ModContext*, void*) {
    const bool hubPromptResolved = bossrush::consume_hub_midna_prompt_resolution();
    const bool hubWarpPromptResolved = bossrush::consume_midna_hub_warp_prompt_resolution();
    return hubPromptResolved || hubWarpPromptResolved;
}

const char* menu_option(ModContext*, void*) {
    return bossrush::has_midna_hub_warp_prompt() ? bossrush::midna_hub_warp_option_text() : nullptr;
}

int menu_begin(ModContext*, void*) {
    return bossrush::begin_midna_hub_warp_prompt();
}

int menu_resolve(ModContext*, int choice, void*) {
    return bossrush::resolve_midna_hub_warp_prompt(choice);
}

int menu_cancel(ModContext*, void*) {
    return bossrush::cancel_midna_hub_warp_prompt();
}

int menu_execute_warp(ModContext*, void* player_ptr, void*) {
    if (!bossrush::consume_midna_hub_warp_request()) {
        return 0;
    }

    auto* player = static_cast<daAlink_c*>(player_ptr);
    if (bossrush::prepare_midna_hub_warp() &&
        (player == nullptr || !player->procDungeonWarpReadyInit()))
    {
        bossrush::warp_to_hub_now();
    }
    return 1;
}

}  // namespace

ModResult install_bossrush_hooks(ModError* error) {
    StageFlowProviderDesc stageFlow = STAGE_FLOW_PROVIDER_DESC_INIT;
    stageFlow.transition_actor_update = transition_actor_update;
    stageFlow.sequence_complete = sequence_complete;

    ModResult result =
        svc_stage_flow->register_provider(mod_ctx, &stageFlow, &s_stage_flow_provider);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to register Dawnlight stage-flow provider");
    }

    MidnaDialogProviderDesc midnaDialog = MIDNA_DIALOG_PROVIDER_DESC_INIT;
    midnaDialog.prompt_text = prompt_text;
    midnaDialog.prompt_begin = prompt_begin;
    midnaDialog.prompt_resolve = prompt_resolve;
    midnaDialog.prompt_consume_resolution = prompt_consume_resolution;
    midnaDialog.menu_option = menu_option;
    midnaDialog.menu_begin = menu_begin;
    midnaDialog.menu_resolve = menu_resolve;
    midnaDialog.menu_cancel = menu_cancel;
    midnaDialog.menu_execute_warp = menu_execute_warp;

    result = svc_midna_dialog->register_provider(
        mod_ctx, &midnaDialog, &s_midna_dialog_provider);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to register Dawnlight Midna-dialog provider");
    }

    result = mods::hook_add_post<PlaySceneUpdateHook>(svc_hook, update_bossrush);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight Boss Rush update hook");
    }

    return MOD_OK;
}

}  // namespace dawnlight
