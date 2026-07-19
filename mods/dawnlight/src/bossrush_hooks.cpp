#include "bossrush.hpp"

#include "SSystem/SComponent/c_math.h"
#include "d/d_com_inf_game.h"
#include "d/d_stage.h"
#include "d/actor/d_a_b_gnd.h"
#include "m_Do/m_Do_ext.h"
class JPABaseEmitter;
#include "d/actor/d_a_alink.h"
#define private public
#include "d/actor/d_a_obj_bosswarp.h"
#undef private
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_name.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

IMPORT_SERVICE(HookService, svc_hook);

namespace dawnlight {
namespace {

DEFINE_HOOK(&daObjBossWarp_c::execute, BossWarpExecuteHook);

bool update_hub_boss_warp(daObjBossWarp_c* warp) {
    if (warp == nullptr || !bossrush::is_hub_stage()) {
        return false;
    }

    const bool active = bossrush::is_hub_center_portal(warp->getSceneListNo());
    warp->appear(0);
    if (active) {
        warp->mpBrkAnm->play();
        warp->mpBtkAnm[0]->play();
        warp->mpBtkAnm[1]->play();
    } else {
        warp->mpBrkAnm->setPlaySpeed(0.0f);
        warp->mpBtkAnm[0]->setPlaySpeed(0.0f);
        warp->mpBtkAnm[1]->setPlaySpeed(0.0f);
    }
    if (warp->mScalingUp) {
        cLib_chaseF(&warp->scale.y, 1.0f, 0.016f);
    }
    if (warp->mpParticle[3] != nullptr) {
        JGeometry::TVec3<f32> particleScale;
        JGeometry::setTVec3f(&warp->scale.x, &particleScale.x);
        warp->mpParticle[3]->setGlobalScale(particleScale);
    }
    if (active && warp->mpBrkAnm != nullptr && warp->mpBrkAnm->getFrame() != 0.0f) {
        mDoAud_seStartLevel(Z2SE_OBJ_MDN_ESCAPE_HOLE, &warp->current.pos, 0, 0);
    }
    warp->setBaseMtx();
    return true;
}

HookAction before_boss_warp_execute(ModContext*, void* args, void* retval, void*) {
    auto* warp = mods::arg<daObjBossWarp_c*>(args, 0);
    if (!update_hub_boss_warp(warp)) {
        return HOOK_CONTINUE;
    }
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

void update_ganondorf_sequence() {
    auto* ganondorf = static_cast<b_gnd_class*>(fopAcM_SearchByName(fpcNm_B_GND_e));
    if (ganondorf == nullptr || ganondorf->mDemoCamMode != 65 ||
        ganondorf->mDemoCamTimer != 330 || !bossrush::complete_ganondorf_sequence()) {
        return;
    }

    camera_process_class* camera = dComIfGp_getCamera(dComIfGp_getPlayerCameraID(0));
    if (camera != nullptr) {
        camera->mCamera.Start();
        camera->mCamera.SetTrimSize(0);
    }
    dComIfGp_event_reset();
    ganondorf->mDemoCamMode = 0;
    ganondorf->mDemoCamTimer = 0;
}

}  // namespace

ModResult install_bossrush_hooks(ModError* error) {
    ModResult result = mods::hook_add_pre<BossWarpExecuteHook>(svc_hook, before_boss_warp_execute);
    if (result == MOD_OK) {
        return MOD_OK;
    }

    return mods::set_error(error, result, "failed to install Dawnlight Boss Rush portal hook");
}

void update_bossrush_hooks() {
    bossrush::update();
    update_ganondorf_sequence();
}

}  // namespace dawnlight
