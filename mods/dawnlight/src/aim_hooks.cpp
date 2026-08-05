#include "config.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/d_camera.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_camera_mng.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"
#include "SSystem/SComponent/c_math.h"

#include <cmath>

namespace dawnlight {
namespace {

DEFINE_HOOK(&daAlink_c::procBowSubject, BowSubjectHook);
DEFINE_HOOK(&daAlink_c::procBoomerangSubject, BoomerangSubjectHook);
DEFINE_HOOK(&daAlink_c::procHookshotSubject, HookshotSubjectHook);
DEFINE_HOOK(&daAlink_c::procIronBallSubject, IronBallSubjectHook);
DEFINE_HOOK(&daAlink_c::procCopyRodSubject, CopyRodSubjectHook);
DEFINE_HOOK(&dCamera_c::nextMode, CameraNextModeHook);
DEFINE_HOOK(&dCamera_c::nextType, CameraNextTypeHook);

enum class AimItem {
    Bow,
    Boomerang,
    Hookshot,
    IronBall,
    CopyRod,
};

bool use_custom_aim_movement() {
    return aim_mode() != AimMode::Vanilla || aim_movement_enabled();
}

bool use_third_person_camera() {
    return aim_mode() == AimMode::ThirdPerson;
}

bool use_cinema_camera() {
    return aim_mode() == AimMode::Cinema;
}

bool use_scope_suppress_camera() {
    return aim_mode() == AimMode::ThirdPerson || aim_mode() == AimMode::Cinema;
}

bool is_hawkeye_bow(daAlink_c* link) {
    return link != nullptr && link->mEquipItem == dItemNo_HAWK_ARROW_e;
}

BOOL face_camera_view_yaw(daAlink_c* link) {
    if (link == nullptr) {
        return FALSE;
    }

    auto* camera = dComIfGp_getCamera(link->field_0x317c);
    if (camera == nullptr) {
        return FALSE;
    }

    const cXyz direction = *fopCamM_GetCenter_p(camera) - *fopCamM_GetEye_p(camera);
    const f32 horizontal = JMAFastSqrt(SQUARE(direction.x) + SQUARE(direction.z));
    if (horizontal <= 0.001f) {
        return FALSE;
    }

    link->shape_angle.y = cM_atan2s(direction.x, direction.z);
    link->field_0x310c = link->shape_angle.y;
    return TRUE;
}

BOOL aim_with_c_stick(daAlink_c* link) {
    const f32 stickValue = link->mStickValue;
    const f32 moveValue = link->mMoveValue;
    const s16 stickAngle = link->mStickAngle;
    const s16 moveAngle = link->mMoveAngle;

    const f32 cStickValue = cLib_minMaxLimit<f32>(mDoCPd_c::getSubStickValue(PAD_1), 0.0f, 1.0f);
    if (cStickValue <= 0.05f) {
        link->mStickValue = 0.0f;
        link->mMoveValue = 0.0f;
    } else {
        link->mStickValue = cStickValue;
        link->mMoveValue = cStickValue;
        link->mStickAngle = mDoCPd_c::getSubStickAngle(PAD_1) + 0x8000;
        link->mMoveAngle = link->mStickAngle +
            dCam_getControledAngleY(dComIfGp_getCamera(link->field_0x317c));
    }

    const BOOL result = link->setBodyAngleToCamera();
    link->mStickValue = stickValue;
    link->mMoveValue = moveValue;
    link->mStickAngle = stickAngle;
    link->mMoveAngle = moveAngle;
    return result;
}

void update_move_animation(daAlink_c* link, u8 waitDirection, bool rightWait, bool ironBall) {
    f32 morph = -1.0f;
    if (link->checkZeroSpeedF()) {
        link->onModeFlg(1);
        if (link->field_0x2f98 != waitDirection) {
            link->field_0x2f98 = waitDirection;
        }
        if (rightWait) {
            link->current.angle.y = link->shape_angle.y - 0x4000;
        }
    } else {
        link->offModeFlg(1);
    }

    if (ironBall && link->checkModeFlg(1)) {
        link->setIronBallBaseAnime();
    } else {
        link->setBlendAtnMoveAnime(morph);
    }
}

void draw_iron_ball_sight(daAlink_c* link) {
    cXyz position;
    link->checkSightLine(10000.0f, &position);
    link->mSight.setPos(&position);
    link->mSight.onDrawFlg();
    link->mSight.offLockFlg();
}

void draw_subject_sight(daAlink_c* link, AimItem item) {
    switch (item) {
    case AimItem::Bow:
        if (link->mEquipItem != dItemNo_HAWK_ARROW_e) {
            link->setBowSight();
            link->mSight.onDrawFlg();
        }
        break;
    case AimItem::Boomerang:
        link->setBoomerangSight();
        link->mSight.onDrawFlg();
        break;
    case AimItem::Hookshot:
        link->setHookshotSight();
        link->mSight.onDrawFlg();
        break;
    case AimItem::IronBall:
        draw_iron_ball_sight(link);
        break;
    case AimItem::CopyRod:
        link->setCopyRodSight();
        link->mSight.onDrawFlg();
        break;
    }
}

bool update_subject_aim(daAlink_c* link, AimItem item) {
    if (link == nullptr) {
        return false;
    }

    const bool hawkeyeBow = item == AimItem::Bow && is_hawkeye_bow(link);
    if ((hawkeyeBow && !aim_movement_enabled()) ||
        (!hawkeyeBow && !use_custom_aim_movement()))
    {
        return false;
    }

    if (!hawkeyeBow && use_cinema_camera()) {
        face_camera_view_yaw(link);
    }

    const s16 shapeYaw = link->shape_angle.y;
    link->setSpeedAndAngleAtn();
    link->shape_angle.y = shapeYaw;

    switch (item) {
    case AimItem::Bow:
        update_move_animation(link, 3, true, false);
        if (aim_with_c_stick(link)) {
            draw_subject_sight(link, item);
        }
        break;
    case AimItem::Boomerang:
        update_move_animation(link, 3, false, false);
        if (aim_with_c_stick(link)) {
            draw_subject_sight(link, item);
        }
        break;
    case AimItem::Hookshot:
        if (link->checkHookshotWait()) {
            update_move_animation(link, 2, false, false);
            if (aim_with_c_stick(link)) {
                draw_subject_sight(link, item);
            }
        }
        break;
    case AimItem::IronBall:
        if (link->checkIronBallPreSwingAnime()) {
            link->mNormalSpeed = 0.0f;
        }
        update_move_animation(link, 2, false, true);
        if (link->itemButton() && link->mItemVar0.field_0x3018 == 2 && aim_with_c_stick(link)) {
            draw_iron_ball_sight(link);
        }
        break;
    case AimItem::CopyRod:
        update_move_animation(link, 3, false, false);
        if (aim_with_c_stick(link)) {
            draw_subject_sight(link, item);
        }
        break;
    }
    return true;
}

HookAction replace_bow_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    link->setDoStatus(BUTTON_STATUS_BACK);
    if (!link->checkNextAction(0) &&
        (!update_subject_aim(link, AimItem::Bow) && link->setBodyAngleToCamera()))
    {
        link->setBowSight();
    }
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

HookAction replace_boomerang_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link->checkItemActorPointer()) {
        *static_cast<int*>(retval) = 1;
        return HOOK_SKIP_ORIGINAL;
    }

    if (link->checkBoomerangReadyAnime()) {
        link->setDoStatus(BUTTON_STATUS_BACK);
    }
    link->setShapeAngleToAtnActor(0);

    if (!link->checkNextAction(0)) {
        if (!update_subject_aim(link, AimItem::Boomerang) && link->setBodyAngleToCamera()) {
            link->setBoomerangSight();
        }
    } else {
        link->mSight.offDrawFlg();
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

HookAction replace_hookshot_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link->checkHookshotWait()) {
        link->setDoStatus(BUTTON_STATUS_BACK);
    }

    link->setShapeAngleToAtnActor(0);
    link->mSight.offDrawFlg();

    if (!link->checkNextAction(0)) {
        if (link->checkHookshotWait()) {
            if (!update_subject_aim(link, AimItem::Hookshot) && link->setBodyAngleToCamera()) {
                link->setHookshotSight();
            }
            dComIfGp_clearPlayerStatus0(0, 0x40000);
        } else {
            dComIfGp_setPlayerStatus0(0, 0x40000);
        }
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

HookAction replace_iron_ball_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link->checkIronBallAnime()) {
        link->setDoStatus(BUTTON_STATUS_BACK);
    }
    link->setShapeAngleToAtnActor(0);

    if (!link->checkNextAction(0) && link->itemButton() && link->mItemVar0.field_0x3018 == 2) {
        if (!update_subject_aim(link, AimItem::IronBall)) {
            link->setBodyAngleToCamera();
        }
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

HookAction replace_copy_rod_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!link->checkItemActorPointer()) {
        *static_cast<int*>(retval) = 1;
        return HOOK_SKIP_ORIGINAL;
    }

    if (link->checkCopyRodReadyAnime()) {
        link->setDoStatus(BUTTON_STATUS_BACK);
    }
    link->setShapeAngleToAtnActor(0);

    if (!link->checkNextAction(0)) {
        if (!update_subject_aim(link, AimItem::CopyRod) && link->setBodyAngleToCamera()) {
            link->setCopyRodSight();
        }
    } else {
        link->mSight.offDrawFlg();
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

bool player_in_supported_aim_state(dCamera_c* camera) {
    auto* link = daAlink_getAlinkActorClass();
    if (link == nullptr || camera == nullptr) {
        return false;
    }

    const u32 pad = camera->mPadID;
    return dComIfGp_checkPlayerStatus0(pad, 0x1040) ||
           dComIfGp_checkPlayerStatus0(pad, 0x80000) ||
           dComIfGp_checkPlayerStatus0(pad, 0x80) ||
           dComIfGp_checkPlayerStatus0(pad, 0x4000) ||
           dComIfGp_checkPlayerStatus0(pad, 0x400);
}

void after_camera_next_mode(ModContext*, void* args, void* retval, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    auto* result = static_cast<s32*>(retval);
    if (camera == nullptr || result == nullptr || !use_scope_suppress_camera() ||
        !player_in_supported_aim_state(camera))
    {
        return;
    }
    if (is_hawkeye_bow(daAlink_getAlinkActorClass())) {
        return;
    }

    if (*result == 7 || *result == 8) {
        *result = use_third_person_camera() ? 0 : 8;
    }
}

void after_camera_next_type(ModContext*, void* args, void* retval, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    auto* result = static_cast<s32*>(retval);
    if (camera == nullptr || result == nullptr || !use_scope_suppress_camera() ||
        !player_in_supported_aim_state(camera))
    {
        return;
    }
    if (is_hawkeye_bow(daAlink_getAlinkActorClass())) {
        return;
    }

    const int scopeType = camera->GetCameraTypeFromCameraName("Scope");
    if (*result == scopeType) {
        *result = camera->mMapToolType;
    }
}

ModResult add_aim_hooks(ModError* error, ModResult result) {
    if (result == MOD_OK) {
        result = mods::hook_add_pre<BoomerangSubjectHook>(svc_hook, replace_boomerang_subject);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<HookshotSubjectHook>(svc_hook, replace_hookshot_subject);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<IronBallSubjectHook>(svc_hook, replace_iron_ball_subject);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<CopyRodSubjectHook>(svc_hook, replace_copy_rod_subject);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<CameraNextModeHook>(svc_hook, after_camera_next_mode);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<CameraNextTypeHook>(svc_hook, after_camera_next_type);
    }
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to install Dawnlight aim hooks");
    }
    return MOD_OK;
}

}  // namespace

ModResult install_aim_hooks(ModError* error) {
    return add_aim_hooks(error, mods::hook_add_pre<BowSubjectHook>(svc_hook, replace_bow_subject));
}

}  // namespace dawnlight
