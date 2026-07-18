#include "config.hpp"

#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "f_op/f_op_camera_mng.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/service.hpp"
#include "mods/svc/aim_control.h"

#include <cmath>

IMPORT_SERVICE(AimControlService, svc_aim_control);

namespace dawnlight {
namespace {

AimControlProviderHandle s_provider = 0;

enum class AimItem {
    Bow,
    Boomerang,
    Hookshot,
    IronBall,
    CopyRod,
};

bool use_cinema_aim() {
    return aim_mode() == AimMode::Cinema;
}

bool use_custom_aim_movement() {
    return aim_mode() != AimMode::Vanilla || aim_movement_enabled();
}

int camera_mode(ModContext*, const DuskModAimRequest* request, void*) {
    if (request == nullptr || !request->supported) {
        return DUSK_MOD_AIM_CAMERA_DEFAULT;
    }
    if (use_cinema_aim() && !request->scoped) {
        return DUSK_MOD_AIM_CAMERA_OVER_SHOULDER;
    }
    if (aim_mode() == AimMode::ThirdPerson) {
        return DUSK_MOD_AIM_CAMERA_THIRD_PERSON;
    }
    return DUSK_MOD_AIM_CAMERA_DEFAULT;
}

int input_routing(ModContext*, const DuskModAimRequest*, void*) {
    return use_custom_aim_movement() ? DUSK_MOD_AIM_INPUT_LEFT_MOVE_RIGHT_AIM :
                                       DUSK_MOD_AIM_INPUT_DEFAULT;
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
    if (link == nullptr || !use_custom_aim_movement()) {
        return false;
    }
    if (item == AimItem::Bow && link->mEquipItem == dItemNo_HAWK_ARROW_e) {
        return false;
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

bool item_from_kind(int kind, AimItem& item) {
    switch (kind) {
    case DUSK_MOD_AIM_ITEM_BOW:
        item = AimItem::Bow;
        return true;
    case DUSK_MOD_AIM_ITEM_BOOMERANG:
        item = AimItem::Boomerang;
        return true;
    case DUSK_MOD_AIM_ITEM_HOOKSHOT:
        item = AimItem::Hookshot;
        return true;
    case DUSK_MOD_AIM_ITEM_IRON_BALL:
        item = AimItem::IronBall;
        return true;
    case DUSK_MOD_AIM_ITEM_COPY_ROD:
        item = AimItem::CopyRod;
        return true;
    default:
        return false;
    }
}

int subject_update(ModContext*, void* player, int itemKind, void*) {
    AimItem item = AimItem::Bow;
    if (!item_from_kind(itemKind, item)) {
        return 0;
    }
    return update_subject_aim(static_cast<daAlink_c*>(player), item);
}

}  // namespace

ModResult install_aim_hooks(ModError* error) {
    AimControlProviderDesc desc = AIM_CONTROL_PROVIDER_DESC_INIT;
    desc.camera_mode = camera_mode;
    desc.input_routing = input_routing;
    desc.subject_update = subject_update;

    const ModResult result = svc_aim_control->register_provider(mod_ctx, &desc, &s_provider);
    if (result != MOD_OK) {
        return mods::set_error(error, result, "failed to register Dawnlight aim-control provider");
    }
    return MOD_OK;
}

}  // namespace dawnlight
