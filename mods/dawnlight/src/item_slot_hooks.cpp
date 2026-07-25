#include "config.hpp"
#include "hud_layout.hpp"
#include "touch_hooks.hpp"

#include "global.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_kantera_icon_meter.h"
#include "d/d_item.h"
#include "d/d_item_data.h"
#include "d/d_meter_HIO.h"
#include "d/d_meter2_info.h"
#include "d/d_menu_window.h"
#include "d/d_menu_item_explain.h"
#include "d/d_pane_class.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#define private public
#include "d/d_menu_ring.h"
#include "d/d_meter2_draw.h"
#undef private
#include "m_Do/m_Do_controller_pad.h"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <algorithm>
#include <array>
#include <cstring>

IMPORT_SERVICE(HookService, svc_hook);

namespace dawnlight {
namespace {

constexpr u8 kZItemSlot = SELECT_ITEM_DOWN;
constexpr int kExtendedSelectItemCount = 3;
constexpr int kSelectItemNotFound = 3;

DEFINE_HOOK(&dComIfGp_getSelectItem, GetSelectItemHook);
DEFINE_HOOK(&dComIfGp_setSelectItem, SetSelectItemHook);
DEFINE_HOOK(&dMenu_Ring_c::_create, RingCreateHook);
DEFINE_HOOK(&dMenu_Ring_c::_delete, RingDeleteHook);
DEFINE_HOOK(&dMenu_Ring_c::_draw, RingDrawHook);
DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor, RingSetActiveCursorHook);
DEFINE_HOOK(&dMw_DOWN_TRIGGER, MenuDownTriggerHook);
DEFINE_HOOK(&dMeter2Draw_c::draw, MeterDrawHook);
DEFINE_HOOK(&dMeter2Draw_c::setButtonIconMidonaAlpha, MeterMidnaAlphaHook);
DEFINE_HOOK(&daAlink_c::midnaTalkTrigger, MidnaTalkTriggerHook);
DEFINE_HOOK(&daAlink_c::checkItemButtonChange, CheckItemButtonChangeHook);
DEFINE_HOOK(&daAlink_c::checkItemChangeFromButton, CheckItemChangeFromButtonHook);
DEFINE_HOOK(&daAlink_c::checkSetItemTrigger, CheckSetItemTriggerHook);
DEFINE_HOOK(&daAlink_c::checkItemSetButton, CheckItemSetButtonHook);
DEFINE_HOOK(&daAlink_c::setHeavyBoots, SetHeavyBootsHook);
DEFINE_HOOK(&daAlink_c::execute, PlayerExecuteHook);

struct PendingAssign {
    dMenu_Ring_c* ring = nullptr;
    u8 targetSlot = dItemNo_NONE_e;
    u8 selectedSlot = dItemNo_NONE_e;
    u8 oldTargetSlot = dItemNo_NONE_e;
    u8 oldTargetMix = dItemNo_NONE_e;
    bool active = false;
};

PendingAssign s_pendingAssign;

struct RingZButtonPrompt {
    dMenu_Ring_c* ring = nullptr;
    J2DScreen* screen = nullptr;
    CPaneMgr* button = nullptr;
};

RingZButtonPrompt s_ringZPrompt;
alignas(32) u8 s_zHudItemTexBuf[2][2][0xC00];
u8 s_zHudItemTexPage = 0;
u8 s_zHudLastItem = dItemNo_NONE_e;
J2DPicture* s_zItemNumTex[3] = {};
dKantera_icon_c* s_zOilMeter = nullptr;
daAlink_c* s_zHeavyBootsGuardLink = nullptr;
bool s_zHeavyBootsManualToggleOff = false;
bool s_zHeavyBootsWaitRelease = false;

ResTIMG* z_hud_item_tex(const u8 page, const u8 layer) {
    return reinterpret_cast<ResTIMG*>(s_zHudItemTexBuf[page][layer]);
}

u8 hud_texture_item(u8 itemNo) {
    return itemNo == dItemNo_LIGHT_ARROW_e ? dItemNo_BOW_e : itemNo;
}

u8 hud_layout_item(u8 itemNo) {
    return itemNo == dItemNo_HAWK_ARROW_e ? dItemNo_BOW_e : hud_texture_item(itemNo);
}

u8 clamp_hud_alpha(const f32 alpha) {
    if (alpha <= 0.0f) {
        return 0;
    }
    if (alpha >= 255.0f) {
        return 255;
    }
    return static_cast<u8>(alpha);
}

void hide_pane_tree(J2DPane* pane) {
    if (pane == nullptr) {
        return;
    }

    pane->hide();
    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        hide_pane_tree(child);
    }
}

void show_pane_tree(J2DPane* pane) {
    if (pane == nullptr) {
        return;
    }

    pane->show();
    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        show_pane_tree(child);
    }
}

void show_pane_parents(J2DPane* pane) {
    for (J2DPane* parent = pane; parent != nullptr; parent = parent->getParentPane()) {
        parent->show();
    }
}

J2DPane* item_wheel_z_anchor(J2DScreen* screen) {
    return screen != nullptr ? screen->search(MULTI_CHAR('r_btn_n')) : nullptr;
}

void apply_item_wheel_z_offset(Vec& pos) {
    pos.x += 5.0f;
    pos.y -= 5.0f;
}

void destroy_ring_z_prompt(dMenu_Ring_c* ring) {
    if (s_ringZPrompt.ring != ring) {
        return;
    }

    JKR_DELETE(s_ringZPrompt.button);
    s_ringZPrompt.button = nullptr;
    JKR_DELETE(s_ringZPrompt.screen);
    s_ringZPrompt.screen = nullptr;
    s_ringZPrompt.ring = nullptr;
}

void create_ring_z_prompt(dMenu_Ring_c* ring) {
    destroy_ring_z_prompt(s_ringZPrompt.ring);
    if (!z_item_slot_enabled() || ring == nullptr || ring->mPlayerIsWolf || ring->mpScreen == nullptr) {
        return;
    }

    J2DPane* anchor = item_wheel_z_anchor(ring->mpScreen);
    if (anchor != nullptr) {
        anchor->translate(anchor->getTranslateX() + 64.0f, anchor->getTranslateY());
        anchor->hide();
    }

    J2DScreen* screen = JKR_NEW J2DScreen();
    if (screen == nullptr) {
        return;
    }
    if (!screen->setPriority("zelda_game_image.blo", 0x20000, dComIfGp_getMain2DArchive())) {
        JKR_DELETE(screen);
        return;
    }

    dPaneClass_showNullPane(screen);
    hide_pane_tree(screen->search('ROOT'));

    J2DPane* zButtonPane = screen->search(MULTI_CHAR('zbtn_n'));
    if (zButtonPane == nullptr) {
        JKR_DELETE(screen);
        return;
    }

    show_pane_parents(zButtonPane);
    show_pane_tree(zButtonPane);

    CPaneMgr* button = JKR_NEW CPaneMgr(screen, MULTI_CHAR('zbtn_n'), 2, nullptr);
    if (button == nullptr) {
        JKR_DELETE(screen);
        return;
    }

    button->setAlphaRate(1.0f);
    button->show();
    s_ringZPrompt = {.ring = ring, .screen = screen, .button = button};
}

void draw_ring_z_prompt(dMenu_Ring_c* ring) {
    if (!z_item_slot_enabled() || s_ringZPrompt.ring != ring ||
        s_ringZPrompt.screen == nullptr || s_ringZPrompt.button == nullptr ||
        ring == nullptr || ring->mpScreen == nullptr || ring->mPlayerIsWolf)
    {
        return;
    }

    J2DPane* anchor = item_wheel_z_anchor(ring->mpScreen);
    if (anchor == nullptr) {
        return;
    }

    CPaneMgr paneMgr;
    Vec pos = paneMgr.getGlobalVtxCenter(anchor, true, 0);
    pos.x += ring->mCenterPosX;
    pos.y += ring->mCenterPosY;
    apply_item_wheel_z_offset(pos);

    s_ringZPrompt.button->scale(0.9f, 0.9f);
    s_ringZPrompt.button->paneTrans(pos.x - s_ringZPrompt.button->getInitGlobalCenterPosX(),
                                    pos.y - s_ringZPrompt.button->getInitGlobalCenterPosY());
    s_ringZPrompt.button->setAlphaRate(ring->mAlphaRate);
    s_ringZPrompt.screen->draw(0.0f, 0.0f, dComIfGp_getCurrentGrafPort());
}

bool pane_current_global_bounds(CPaneMgr* pane, f32& left, f32& top, f32& right, f32& bottom) {
    if (pane == nullptr) {
        return false;
    }

    Mtx mtx;
    for (u8 i = 0; i < 4; ++i) {
        Vec vtx = pane->getGlobalVtx(&mtx, i, false, 0);
        if (i == 0) {
            left = right = vtx.x;
            top = bottom = vtx.y;
            continue;
        }
        if (vtx.x < left) {
            left = vtx.x;
        }
        if (vtx.x > right) {
            right = vtx.x;
        }
        if (vtx.y < top) {
            top = vtx.y;
        }
        if (vtx.y > bottom) {
            bottom = vtx.y;
        }
    }
    return true;
}

bool add_pane_current_global_bounds(CPaneMgr* pane, f32& left, f32& top, f32& right,
    f32& bottom, bool& hasBounds) {
    f32 paneLeft;
    f32 paneTop;
    f32 paneRight;
    f32 paneBottom;
    if (!pane_current_global_bounds(pane, paneLeft, paneTop, paneRight, paneBottom)) {
        return false;
    }

    if (!hasBounds) {
        left = paneLeft;
        top = paneTop;
        right = paneRight;
        bottom = paneBottom;
        hasBounds = true;
        return true;
    }

    if (paneLeft < left) left = paneLeft;
    if (paneRight > right) right = paneRight;
    if (paneTop < top) top = paneTop;
    if (paneBottom > bottom) bottom = paneBottom;
    return true;
}

void pane_trans_to_global_center(CPaneMgr* pane, const f32 targetX, const f32 targetY) {
    f32 transX = targetX - pane->getInitGlobalCenterPosX();
    f32 transY = targetY - pane->getInitGlobalCenterPosY();
    pane->paneTrans(transX, transY);

    f32 left;
    f32 top;
    f32 right;
    f32 bottom;
    if (!pane_current_global_bounds(pane, left, top, right, bottom)) {
        return;
    }

    const f32 centerX = (left + right) * 0.5f;
    const f32 centerY = (top + bottom) * 0.5f;
    const f32 localWidth = pane->getSizeX();
    const f32 localHeight = pane->getSizeY();
    const f32 globalScaleX = localWidth != 0.0f ? (right - left) / localWidth : 1.0f;
    const f32 globalScaleY = localHeight != 0.0f ? (bottom - top) / localHeight : 1.0f;

    if (globalScaleX != 0.0f) {
        transX += (targetX - centerX) / globalScaleX;
    }
    if (globalScaleY != 0.0f) {
        transY += (targetY - centerY) / globalScaleY;
    }
    pane->paneTrans(transX, transY);
}

void change_z_hud_item_texture(dMeter2Draw_c* meter, const u8 itemNo) {
    const u8 textureItem = hud_texture_item(itemNo);
    if (s_zHudLastItem == textureItem) {
        return;
    }

    s_zHudItemTexPage ^= 1;
    ResTIMG* primary = z_hud_item_tex(s_zHudItemTexPage, 0);
    ResTIMG* secondary = z_hud_item_tex(s_zHudItemTexPage, 1);
    const s32 textureCount =
        dMeter2Info_readItemTexture(textureItem, primary,
            static_cast<J2DPicture*>(meter->mpItemR->getPanePtr()), secondary,
            meter->mpItemXYPane[2], nullptr, nullptr, nullptr, nullptr, -1);
    if (textureCount <= 1) {
        meter->mpItemXYPane[2]->hide();
    } else {
        meter->mpItemXYPane[2]->show();
    }

    const f32 textureScale = g_drawHIO.mItemScaleAdjustON ?
        g_drawHIO.mItemScalePercent / 100.0f :
        dItem_data::getTexScale(textureItem) / 100.0f;
    meter->field_0x6c4[2] =
        textureScale * ((primary->width * meter->mpItemR->getInitSizeX()) / 48.0f);
    meter->field_0x6d0[2] =
        textureScale * ((primary->height * meter->mpItemR->getInitSizeY()) / 48.0f);
    meter->field_0x6ac[2] = (meter->mpItemR->getInitSizeX() - meter->field_0x6c4[2]) * 0.5f;
    meter->field_0x6b8[2] = (meter->mpItemR->getInitSizeY() - meter->field_0x6d0[2]) * 0.5f;
    meter->mpItemR->resize(meter->field_0x6c4[2], meter->field_0x6d0[2]);
    meter->mpItemXYPane[2]->resize(meter->field_0x6c4[2], meter->field_0x6d0[2]);
    s_zHudLastItem = textureItem;
}

void layout_z_hud_item(dMeter2Draw_c* meter, const u8 itemNo) {
    meter->setItemParamZ(hud_layout_item(itemNo));
    meter->mpItemR->getPanePtr()->rotate(meter->mpItemR->getSizeX() * 0.5f,
        meter->mpItemR->getSizeY() * 0.5f, ROTATE_Z,
        meter->mItemParams[dMeter2Draw_c::SELECT_Z_e].rotation);

    const DuskModHudTransform hudTransform = hud_layout_z_transform();
    const DuskModHudButtonLayout buttonLayout = hud_layout_z_button_layout();
    const f32 hudScale = hudTransform.scale;
    const f32 itemScale = buttonLayout.item_scale > 0.0f ? buttonLayout.item_scale : 1.0f;
    const f32 itemOffsetX = buttonLayout.item_offset_x;
    const f32 itemOffsetY = buttonLayout.item_offset_y;

    meter->mpItemR->scale(g_drawHIO.mButtonZItemScale * hudScale * itemScale,
        g_drawHIO.mButtonZItemScale * hudScale * itemScale);
    meter->mpItemR->paneTrans(g_drawHIO.mButtonZItemPosX + meter->field_0x6ac[2] +
            itemOffsetX + hudTransform.offset_x,
        g_drawHIO.mButtonZItemPosY + meter->field_0x6b8[2] + itemOffsetY +
            hudTransform.offset_y);

    meter->mpLightXY[2]->scale(g_drawHIO.mButtonZItemBaseScale * hudScale * itemScale,
        g_drawHIO.mButtonZItemBaseScale * hudScale * itemScale);
    meter->mpLightXY[2]->paneTrans(g_drawHIO.mButtonZItemBasePosX + itemOffsetX +
            hudTransform.offset_x,
        g_drawHIO.mButtonZItemBasePosY + itemOffsetY + hudTransform.offset_y);
}

bool is_z_lantern_item(const u8 itemNo) {
    return itemNo == dItemNo_KANTERA_e || itemNo == dItemNo_KANTERA2_e;
}

bool z_item_has_ammo(const u8 itemNo) {
    switch (itemNo) {
    case dItemNo_NORMAL_BOMB_e:
    case dItemNo_WATER_BOMB_e:
    case dItemNo_POKE_BOMB_e:
    case dItemNo_BOMB_ARROW_e:
    case dItemNo_BOW_e:
    case dItemNo_LIGHT_ARROW_e:
    case dItemNo_ARROW_LV1_e:
    case dItemNo_ARROW_LV2_e:
    case dItemNo_ARROW_LV3_e:
    case dItemNo_HAWK_ARROW_e:
    case dItemNo_PACHINKO_e:
    case dItemNo_BEE_CHILD_e:
        return true;
    default:
        return false;
    }
}

bool z_item_ammo_values(const u8 itemNo, u8& itemNum, u8& itemMax) {
    if (!z_item_has_ammo(itemNo)) {
        return false;
    }

    switch (itemNo) {
    case dItemNo_BOW_e:
    case dItemNo_LIGHT_ARROW_e:
    case dItemNo_ARROW_LV1_e:
    case dItemNo_ARROW_LV2_e:
    case dItemNo_ARROW_LV3_e:
    case dItemNo_HAWK_ARROW_e:
        itemNum = static_cast<u8>(dComIfGs_getArrowNum());
        itemMax = static_cast<u8>(dComIfGs_getArrowMax());
        return true;
    case dItemNo_BOMB_ARROW_e: {
        itemNum = static_cast<u8>(std::max<s16>(0, dComIfGp_getSelectItemNum(kZItemSlot)));
        itemMax = static_cast<u8>(std::max(0, dComIfGp_getSelectItemMaxNum(kZItemSlot)));
        itemNum = std::min(itemNum, static_cast<u8>(dComIfGs_getArrowNum()));
        itemMax = std::max(itemMax, static_cast<u8>(dComIfGs_getArrowMax()));
        return true;
    }
    case dItemNo_PACHINKO_e:
        itemNum = static_cast<u8>(dComIfGs_getPachinkoNum());
        itemMax = static_cast<u8>(dComIfGs_getPachinkoMax());
        return true;
    default:
        itemNum = static_cast<u8>(std::max<s16>(0, dComIfGp_getSelectItemNum(kZItemSlot)));
        itemMax = static_cast<u8>(std::max(0, dComIfGp_getSelectItemMaxNum(kZItemSlot)));
        return true;
    }
}

bool ensure_z_item_num_textures() {
    if (s_zItemNumTex[0] != nullptr && s_zItemNumTex[1] != nullptr &&
        s_zItemNumTex[2] != nullptr)
    {
        return true;
    }

    ResTIMG* timg = static_cast<ResTIMG*>(dComIfGp_getMain2DArchive()->getResource(
        'TIMG', dMeter2Info_getNumberTextureName(0)));
    if (timg == nullptr) {
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        if (s_zItemNumTex[i] == nullptr) {
            s_zItemNumTex[i] = JKR_NEW J2DPicture(timg);
        }
        if (s_zItemNumTex[i] == nullptr) {
            return false;
        }
    }
    return true;
}

void set_z_item_num_textures(u8 itemNum, const u8 itemMax) {
    if (!ensure_z_item_num_textures()) {
        return;
    }

    if (itemNum > itemMax) {
        itemNum = itemMax;
    }

    JUtility::TColor black;
    JUtility::TColor white;
    if (itemNum == itemMax) {
        black.set(30, 30, 30, 0);
        white.set(255, 200, 50, 255);
    } else if (itemNum == 0) {
        black.set(30, 30, 30, 0);
        white.set(180, 180, 180, 255);
    } else {
        black.set(0, 0, 0, 0);
        white.set(255, 255, 255, 255);
    }

    for (J2DPicture* digit : s_zItemNumTex) {
        digit->setBlackWhite(black, white);
    }

    auto set_digit = [](const int index, const int digit) {
        ResTIMG* timg = static_cast<ResTIMG*>(dComIfGp_getMain2DArchive()->getResource(
            'TIMG', dMeter2Info_getNumberTextureName(digit)));
        if (timg != nullptr) {
            s_zItemNumTex[index]->changeTexture(timg, 0);
        }
    };

    if (itemNum < 100) {
        set_digit(0, itemNum / 10);
        set_digit(1, itemNum % 10);
        s_zItemNumTex[2]->hide();
    } else {
        set_digit(0, itemNum / 100);
        itemNum %= 100;
        set_digit(1, itemNum / 10);
        set_digit(2, itemNum % 10);
        s_zItemNumTex[2]->show();
    }
}

void update_z_hud_item_alpha(dMeter2Draw_c* meter) {
    const f32 buttonAlpha =
        g_drawHIO.mButtonZAlpha * (g_drawHIO.mParentAlpha * g_drawHIO.mMainHUDButtonsAlpha);
    const f32 parentAlpha = meter->mpButtonParent->getAlphaRate();
    u8 itemAlpha = meter->mpItemR->getInitAlpha();
    u8 itemBaseAlpha = clamp_hud_alpha(
        g_drawHIO.mButtonZItemBaseAlpha * (buttonAlpha * meter->mpLightXY[2]->getInitAlpha()));
    u8 buttonBaseAlpha = clamp_hud_alpha(255.0f * buttonAlpha);

    if (dComIfGp_getSelectItem(kZItemSlot) == dItemNo_NONE_e ||
        dComIfGp_getSelectItem(kZItemSlot) == 0)
    {
        itemAlpha = g_drawHIO.mButtonXYItemDimAlpha;
        itemBaseAlpha = g_drawHIO.mButtonXYItemDimAlpha;
        buttonBaseAlpha = g_drawHIO.mButtonXYBaseDimAlpha;
    }

    meter->mpItemR->setAlpha(clamp_hud_alpha(static_cast<f32>(itemAlpha) * parentAlpha));
    meter->mpLightXY[2]->setAlpha(clamp_hud_alpha(static_cast<f32>(itemBaseAlpha) * parentAlpha));
    meter->mpButtonXY[2]->setAlpha(clamp_hud_alpha(static_cast<f32>(buttonBaseAlpha) * parentAlpha));
}

void draw_z_ammo(dMeter2Draw_c* meter, const u8 itemNo, const f32 itemAlphaRate) {
    u8 itemNum = 0;
    u8 itemMax = 0;
    if (!z_item_ammo_values(itemNo, itemNum, itemMax) || itemMax == 0 ||
        !ensure_z_item_num_textures())
    {
        return;
    }

    set_z_item_num_textures(itemNum, itemMax);

    const DuskModHudTransform hudTransform = hud_layout_z_transform();
    const DuskModHudButtonLayout buttonLayout = hud_layout_z_button_layout();
    const f32 itemScale = buttonLayout.item_scale > 0.0f ? buttonLayout.item_scale : 1.0f;
    const f32 ammoScale =
        hudTransform.scale * itemScale * (buttonLayout.ammo_scale > 0.0f ? buttonLayout.ammo_scale : 1.0f);
    const f32 digitSize = meter->mItemParams[dMeter2Draw_c::SELECT_Z_e].num_scale * 16.0f * ammoScale;

    Vec vtx0 = meter->mpItemR->getPanePtr()->getGlbVtx(0);
    Vec vtx3 = meter->mpItemR->getPanePtr()->getGlbVtx(3);
    const f32 centerX = (vtx0.x + vtx3.x) * 0.5f;
    const f32 centerY = (vtx0.y + vtx3.y) * 0.5f;
    const u8 alpha = clamp_hud_alpha(itemAlphaRate * 255.0f);

    for (int i = 0; i < 3; ++i) {
        if (i == 2 && itemNum < 100) {
            continue;
        }
        s_zItemNumTex[i]->setAlpha(alpha);
        s_zItemNumTex[i]->draw(meter->mItemParams[dMeter2Draw_c::SELECT_Z_e].num_pos_x +
                buttonLayout.ammo_offset_x + centerX + digitSize * i,
            meter->mItemParams[dMeter2Draw_c::SELECT_Z_e].num_pos_y +
                buttonLayout.ammo_offset_y + centerY + meter->mpItemR->getSizeY(),
            digitSize, digitSize, false, false, false);
    }
}

void draw_z_oil_meter(dMeter2Draw_c* meter, const u8 itemNo, const f32 itemAlphaRate) {
    if (!is_z_lantern_item(itemNo) || dComIfGs_getMaxOil() == 0) {
        return;
    }

    if (s_zOilMeter == nullptr) {
        s_zOilMeter = JKR_NEW dKantera_icon_c();
    }
    if (s_zOilMeter == nullptr) {
        return;
    }

    const DuskModHudTransform hudTransform = hud_layout_z_transform();
    const DuskModHudButtonLayout buttonLayout = hud_layout_z_button_layout();
    const f32 itemScale = buttonLayout.item_scale > 0.0f ? buttonLayout.item_scale : 1.0f;
    Vec vtx0 = meter->mpItemR->getPanePtr()->getGlbVtx(0);
    Vec vtx3 = meter->mpItemR->getPanePtr()->getGlbVtx(3);

    s_zOilMeter->setPos(((vtx0.x + vtx3.x) * 0.5f) + 9.0f * hudTransform.scale * itemScale,
        vtx3.y);
    s_zOilMeter->setScale(0.6f * hudTransform.scale * itemScale,
        0.6f * hudTransform.scale * itemScale);
    s_zOilMeter->setNowGauge(dComIfGs_getMaxOil(), dComIfGs_getOil());
    s_zOilMeter->setAlphaRate(itemAlphaRate);
    s_zOilMeter->drawSelf();
}

void draw_z_hud_item_meters(dMeter2Draw_c* meter) {
    if (!z_item_slot_enabled() || meter == nullptr || meter->mpItemR == nullptr ||
        meter->mpButtonParent == nullptr || daPy_py_c::checkNowWolf())
    {
        return;
    }
    if (!meter->mpButtonParent->getPanePtr()->isVisible()) {
        return;
    }

    const u8 itemNo = dComIfGp_getSelectItem(kZItemSlot);
    if (itemNo == dItemNo_NONE_e || itemNo == 0 || !meter->mpItemR->isVisible()) {
        return;
    }

    const f32 itemAlphaRate = static_cast<f32>(meter->mpItemR->getAlpha()) / 255.0f;
    if (itemAlphaRate <= 0.0f) {
        return;
    }

    draw_z_ammo(meter, itemNo, itemAlphaRate);
    draw_z_oil_meter(meter, itemNo, itemAlphaRate);
}

void update_z_hud_item(dMeter2Draw_c* meter) {
    if (!z_item_slot_enabled() || meter == nullptr || meter->mpItemR == nullptr ||
        meter->mpLightXY[2] == nullptr || meter->mpButtonXY[2] == nullptr ||
        meter->mpItemXYPane[2] == nullptr || daPy_py_c::checkNowWolf())
    {
        return;
    }

    if (meter->mpTextXY[2] != nullptr) {
        meter->mpTextXY[2]->hide();
    }

    const u8 itemNo = dComIfGp_getSelectItem(kZItemSlot);
    J2DPane* itemParent = meter->mpScreen != nullptr ?
        meter->mpScreen->search(MULTI_CHAR('item_r_n')) : nullptr;
    if (itemNo == dItemNo_NONE_e || itemNo == 0) {
        if (itemParent != nullptr) itemParent->hide();
        meter->mpItemR->hide();
        meter->mpLightXY[2]->hide();
        return;
    }

    if (itemParent != nullptr) itemParent->show();
    meter->mpItemR->show();
    meter->mpLightXY[2]->show();
    dMeter2Info_onUseButton(METER2_USEBUTTON_Z);
    change_z_hud_item_texture(meter, itemNo);
    layout_z_hud_item(meter, itemNo);
    update_z_hud_item_alpha(meter);
}

void move_midna_hud_to_dpad(dMeter2Draw_c* meter) {
    if (!z_item_slot_enabled() || meter == nullptr || meter->mpButtonMidona == nullptr) {
        return;
    }

    const DuskModHudTransform dpadTransform = hud_layout_dpad_transform();
    const DuskModHudTransform midnaTransform = hud_layout_midna_transform();
    const f32 dpadScale = g_drawHIO.mButtonCrossScale * dpadTransform.scale;
    const f32 midnaScale = g_drawHIO.mMidnaIconScale * dpadScale * midnaTransform.scale;

    f32 left = 0.0f;
    f32 top = 0.0f;
    f32 right = 0.0f;
    f32 bottom = 0.0f;
    bool hasBounds = false;
    for (int i = 0; i < 5; ++i) {
        add_pane_current_global_bounds(meter->mpJujiI[i], left, top, right, bottom, hasBounds);
        add_pane_current_global_bounds(meter->mpJujiM[i], left, top, right, bottom, hasBounds);
    }
    if (!hasBounds) {
        add_pane_current_global_bounds(meter->mpButtonCrossParent, left, top, right, bottom,
            hasBounds);
    }
    if (!hasBounds) {
        return;
    }

    const f32 midnaHalfHeight = meter->mpButtonMidona->getInitSizeY() * midnaScale * 0.5f;
    const f32 targetX = (left + right) * 0.5f + midnaTransform.offset_x;
    const f32 targetY = bottom + midnaHalfHeight + midnaTransform.offset_y;
    meter->mpButtonMidona->scale(midnaScale, midnaScale);
    pane_trans_to_global_center(meter->mpButtonMidona, targetX, targetY);
}

u8 combine_select_item(u8 playItem, u8 mixSlot) {
    if (mixSlot == dItemNo_NONE_e) {
        return playItem;
    }

    u8 saveItem = dComIfGs_getItem(mixSlot, false);
    if (saveItem == dItemNo_BOW_e) {
        saveItem = playItem;
        playItem = dItemNo_BOW_e;
    } else if (saveItem == dItemNo_FISHING_ROD_1_e) {
        saveItem = playItem;
        playItem = dItemNo_FISHING_ROD_1_e;
    }

    if (playItem == dItemNo_BOW_e) {
        switch (saveItem) {
        case dItemNo_NORMAL_BOMB_e:
        case dItemNo_WATER_BOMB_e:
        case dItemNo_POKE_BOMB_e:
            return dItemNo_BOMB_ARROW_e;
        case dItemNo_HAWK_EYE_e:
            return dItemNo_HAWK_ARROW_e;
        default:
            break;
        }
    } else if (playItem == dItemNo_FISHING_ROD_1_e) {
        switch (saveItem) {
        case dItemNo_BEE_CHILD_e:
            return dItemNo_BEE_ROD_e;
        case dItemNo_WORM_e:
            return dItemNo_WORM_ROD_e;
        case dItemNo_ZORAS_JEWEL_e:
            return dItemNo_JEWEL_ROD_e;
        default:
            break;
        }
    }

    return playItem;
}

u8 resolved_select_item(int index) {
    const u8 slot = dComIfGs_getSelectItemIndex(index);
    if (slot == dItemNo_NONE_e) {
        return dItemNo_NONE_e;
    }

    return combine_select_item(dComIfGs_getItem(slot, false), dComIfGs_getMixItemIndex(index));
}

void sync_play_select_item(int index) {
    if (!z_item_slot_enabled() || index != kZItemSlot) {
        return;
    }

    g_dComIfG_gameInfo.play.setSelectItem(index, resolved_select_item(index));
}

int find_select_button(daAlink_c* link, int itemNo) {
    if (link == nullptr) {
        return kSelectItemNotFound;
    }

    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        if (link->checkGroupItem(itemNo, resolved_select_item(i))) {
            return i;
        }
    }
    return kSelectItemNotFound;
}

bool item_needs_z_valid_button(int itemNo) {
    return itemNo == dItemNo_HVY_BOOTS_e || itemNo == dItemNo_SPINNER_e;
}

bool z_heavy_boots_selected(daAlink_c* link) {
    return link != nullptr &&
           link->checkGroupItem(dItemNo_HVY_BOOTS_e, resolved_select_item(kZItemSlot));
}

bool z_heavy_boots_held(daAlink_c* link) {
    return link != nullptr && (link->mItemButton & daAlink_c::BTN_Z) != 0;
}

bool z_heavy_boots_input_locked(daAlink_c* link) {
    return s_zHeavyBootsGuardLink == link && s_zHeavyBootsWaitRelease;
}

bool z_heavy_boots_forced_off_context(daAlink_c* link) {
    if (link == nullptr) {
        return true;
    }

    if (link->checkWolf() || link->checkEventRun() || link->checkDeadHP() ||
        link->checkCanoeRide() || link->checkHorseRide() || link->checkBoardRide() ||
        link->checkSpinnerRide())
    {
        return true;
    }

    switch (link->mProcID) {
    case daAlink_c::PROC_DIVE_JUMP:
    case daAlink_c::PROC_SMALL_JUMP:
    case daAlink_c::PROC_CANOE_RIDE:
    case daAlink_c::PROC_CANOE_JUMP_RIDE:
    case daAlink_c::PROC_CANOE_GETOFF:
    case daAlink_c::PROC_HORSE_RIDE:
    case daAlink_c::PROC_HORSE_GETOFF:
    case daAlink_c::PROC_BOARD_RIDE:
    case daAlink_c::PROC_SPINNER_READY:
        return true;
    default:
        return false;
    }
}

void clear_z_heavy_boots_input_lock() {
    s_zHeavyBootsGuardLink = nullptr;
    s_zHeavyBootsManualToggleOff = false;
    s_zHeavyBootsWaitRelease = false;
}

void lock_z_heavy_boots_input(daAlink_c* link, bool manualToggleOff) {
    s_zHeavyBootsGuardLink = link;
    s_zHeavyBootsManualToggleOff = manualToggleOff;
    s_zHeavyBootsWaitRelease = true;
}

void tick_z_heavy_boots_guard(daAlink_c* link) {
    if (s_zHeavyBootsGuardLink == nullptr) {
        s_zHeavyBootsWaitRelease = false;
    }

    if (s_zHeavyBootsGuardLink == link && s_zHeavyBootsWaitRelease &&
        !z_heavy_boots_held(link))
    {
        clear_z_heavy_boots_input_lock();
    }
}

u8 cursor_for_slot(dMenu_Ring_c* ring, u8 slot) {
    return slot == dItemNo_NONE_e ? dItemNo_NONE_e : ring->getCursorPos(slot);
}

void sync_ring_fields(dMenu_Ring_c* ring) {
    ring->mXButtonSlot = cursor_for_slot(ring, dComIfGs_getSelectItemIndex(SELECT_ITEM_X));
    ring->mYButtonSlot = cursor_for_slot(ring, dComIfGs_getSelectItemIndex(SELECT_ITEM_Y));
    ring->field_0x6ac = cursor_for_slot(ring, dComIfGs_getSelectItemIndex(kZItemSlot));
    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        ring->field_0x6b4[i] = dComIfGs_getSelectItemIndex(i);
        ring->field_0x6b8[i] = dComIfGs_getMixItemIndex(i);
    }
}

void store_select_slots(const std::array<u8, kExtendedSelectItemCount>& slots,
    const std::array<u8, kExtendedSelectItemCount>& mixes) {
    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        dComIfGs_setMixItemIndex(i, mixes[i]);
        dComIfGs_setSelectItemIndex(i, slots[i]);
        sync_play_select_item(i);
    }
}

void assign_current_item(dMenu_Ring_c* ring, u8 targetSlot) {
    const u8 selectedSlot = ring->mItemSlots[ring->mCurrentSlot];
    std::array<u8, kExtendedSelectItemCount> slots = {
        dComIfGs_getSelectItemIndex(SELECT_ITEM_X),
        dComIfGs_getSelectItemIndex(SELECT_ITEM_Y),
        dComIfGs_getSelectItemIndex(kZItemSlot),
    };
    std::array<u8, kExtendedSelectItemCount> mixes = {
        dComIfGs_getMixItemIndex(SELECT_ITEM_X),
        dComIfGs_getMixItemIndex(SELECT_ITEM_Y),
        dComIfGs_getMixItemIndex(kZItemSlot),
    };

    u8 sourceSlot = dItemNo_NONE_e;
    bool selectedWasMixItem = false;
    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        if (i == targetSlot) {
            continue;
        }
        if (slots[i] == selectedSlot) {
            sourceSlot = i;
            break;
        }
        if (mixes[i] == selectedSlot) {
            sourceSlot = i;
            selectedWasMixItem = true;
            break;
        }
    }

    const u8 oldTargetSlot = slots[targetSlot];
    const u8 oldTargetMix = mixes[targetSlot];
    const bool targetAlreadyHeldSelected = oldTargetSlot == selectedSlot;

    slots[targetSlot] = selectedSlot;
    mixes[targetSlot] = dItemNo_NONE_e;

    if (sourceSlot != dItemNo_NONE_e) {
        if (targetAlreadyHeldSelected) {
            if (selectedWasMixItem) {
                mixes[sourceSlot] = dItemNo_NONE_e;
            } else {
                slots[sourceSlot] = dItemNo_NONE_e;
                mixes[sourceSlot] = dItemNo_NONE_e;
            }
        } else {
            slots[sourceSlot] = oldTargetSlot;
            mixes[sourceSlot] = oldTargetSlot == dItemNo_NONE_e ? dItemNo_NONE_e : oldTargetMix;
        }
    }

    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        if (i == targetSlot || i == sourceSlot) {
            continue;
        }
        if (slots[i] == selectedSlot) {
            slots[i] = dItemNo_NONE_e;
            mixes[i] = dItemNo_NONE_e;
        } else if (mixes[i] == selectedSlot) {
            mixes[i] = dItemNo_NONE_e;
        }
    }

    store_select_slots(slots, mixes);
    sync_ring_fields(ring);
    ring->field_0x6b3 = targetSlot;
    ring->field_0x674[targetSlot] = 1;
    ring->setJumpItem(true);
}

bool item_assign_allowed(dMenu_Ring_c* ring) {
    if (ring == nullptr) {
        return false;
    }

    const u8 item = dComIfGs_getItem(ring->mItemSlots[ring->mCurrentSlot], false);
    return ring->mStatus == dMenu_Ring_c::STATUS_WAIT &&
           ring->mOldStatus != dMenu_Ring_c::STATUS_EXPLAIN_FORCE &&
           ring->mOldStatus != dMenu_Ring_c::STATUS_EXPLAIN &&
           ring->mpItemExplain->getStatus() == 0 &&
           !ring->mPlayerIsWolf &&
           item != dItemNo_NONE_e;
}

u8 vanilla_assign_target() {
    if (mDoCPd_c::getTrigX(PAD_1)) {
        return SELECT_ITEM_X;
    }
    if (mDoCPd_c::getTrigY(PAD_1)) {
        return SELECT_ITEM_Y;
    }
    return dItemNo_NONE_e;
}

void capture_vanilla_assign(dMenu_Ring_c* ring) {
    s_pendingAssign = {};
    const u8 targetSlot = vanilla_assign_target();
    if (!item_assign_allowed(ring) || targetSlot == dItemNo_NONE_e) {
        return;
    }

    s_pendingAssign = {
        .ring = ring,
        .targetSlot = targetSlot,
        .selectedSlot = ring->mItemSlots[ring->mCurrentSlot],
        .oldTargetSlot = dComIfGs_getSelectItemIndex(targetSlot),
        .oldTargetMix = dComIfGs_getMixItemIndex(targetSlot),
        .active = true,
    };
}

void rotate_pending_duplicate(dMenu_Ring_c* ring) {
    if (!s_pendingAssign.active || s_pendingAssign.ring != ring) {
        s_pendingAssign = {};
        return;
    }

    std::array<u8, kExtendedSelectItemCount> slots = {
        ring->field_0x6b4[SELECT_ITEM_X],
        ring->field_0x6b4[SELECT_ITEM_Y],
        dComIfGs_getSelectItemIndex(kZItemSlot),
    };
    std::array<u8, kExtendedSelectItemCount> mixes = {
        ring->field_0x6b8[SELECT_ITEM_X],
        ring->field_0x6b8[SELECT_ITEM_Y],
        dComIfGs_getMixItemIndex(kZItemSlot),
    };

    const u8 targetSlot = s_pendingAssign.targetSlot;
    const u8 selectedSlot = s_pendingAssign.selectedSlot;
    u8 sourceSlot = dItemNo_NONE_e;
    bool selectedWasMixItem = false;
    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        if (i == targetSlot) {
            continue;
        }
        if (slots[i] == selectedSlot) {
            sourceSlot = i;
            break;
        }
        if (mixes[i] == selectedSlot) {
            sourceSlot = i;
            selectedWasMixItem = true;
            break;
        }
    }

    if (sourceSlot != dItemNo_NONE_e) {
        if (s_pendingAssign.oldTargetSlot == selectedSlot) {
            if (selectedWasMixItem) {
                mixes[sourceSlot] = dItemNo_NONE_e;
            } else {
                slots[sourceSlot] = dItemNo_NONE_e;
                mixes[sourceSlot] = dItemNo_NONE_e;
            }
        } else {
            slots[sourceSlot] = s_pendingAssign.oldTargetSlot;
            mixes[sourceSlot] =
                s_pendingAssign.oldTargetSlot == dItemNo_NONE_e ? dItemNo_NONE_e :
                                                                  s_pendingAssign.oldTargetMix;
        }
    }

    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        if (i == targetSlot || i == sourceSlot) {
            continue;
        }
        if (slots[i] == selectedSlot) {
            slots[i] = dItemNo_NONE_e;
            mixes[i] = dItemNo_NONE_e;
        } else if (mixes[i] == selectedSlot) {
            mixes[i] = dItemNo_NONE_e;
        }
    }

    store_select_slots(slots, mixes);
    sync_ring_fields(ring);
    ring->field_0x674[targetSlot] = 1;
    if (sourceSlot != dItemNo_NONE_e) {
        ring->field_0x674[sourceSlot] = 1;
    }
    s_pendingAssign = {};
}

HookAction before_get_select_item(ModContext*, void* args, void* retval, void*) {
    const int index = mods::arg<int>(args, 0);
    if (!z_item_slot_enabled() || index != kZItemSlot) {
        return HOOK_CONTINUE;
    }

    *static_cast<u8*>(retval) = resolved_select_item(index);
    return HOOK_SKIP_ORIGINAL;
}

void after_set_select_item(ModContext*, void* args, void*, void*) {
    sync_play_select_item(mods::arg<int>(args, 0));
}

void after_ring_create(ModContext*, void* args, void*, void*) {
    create_ring_z_prompt(mods::arg<dMenu_Ring_c*>(args, 0));
}

HookAction before_ring_delete(ModContext*, void* args, void*, void*) {
    destroy_ring_z_prompt(mods::arg<dMenu_Ring_c*>(args, 0));
    return HOOK_CONTINUE;
}

void after_ring_draw(ModContext*, void* args, void*, void*) {
    draw_ring_z_prompt(mods::arg<dMenu_Ring_c*>(args, 0));
}

HookAction before_meter_draw(ModContext*, void* args, void*, void*) {
    update_z_hud_item(mods::arg<dMeter2Draw_c*>(args, 0));
    return HOOK_CONTINUE;
}

void after_meter_draw(ModContext*, void* args, void*, void*) {
    draw_z_hud_item_meters(mods::arg<dMeter2Draw_c*>(args, 0));
}

void after_meter_midna_alpha(ModContext*, void* args, void*, void*) {
    move_midna_hud_to_dpad(mods::arg<dMeter2Draw_c*>(args, 0));
}

HookAction before_ring_set_active_cursor(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!z_item_slot_enabled() || ring == nullptr) {
        s_pendingAssign = {};
        return HOOK_CONTINUE;
    }

    if (!mDoCPd_c::getTrigZ(PAD_1)) {
        capture_vanilla_assign(ring);
        return HOOK_CONTINUE;
    }

    s_pendingAssign = {};
    if (item_assign_allowed(ring)) {
        assign_current_item(ring, kZItemSlot);
        if (ring->mpItemExplain->getStatus() == 0) {
            ring->setStatus(dMenu_Ring_c::STATUS_WAIT);
            ring->stick_wait_init();
        }
    } else {
        Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, nullptr, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    }

    return HOOK_SKIP_ORIGINAL;
}

void after_ring_set_active_cursor(ModContext*, void* args, void*, void*) {
    rotate_pending_duplicate(mods::arg<dMenu_Ring_c*>(args, 0));
}

HookAction before_midna_talk_trigger(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<const daAlink_c*>(args, 0);
    if (!z_item_slot_enabled() || link == nullptr) {
        return HOOK_CONTINUE;
    }

    *static_cast<BOOL*>(retval) =
        mDoCPd_c::getTrigDown(PAD_1) || consume_touch_midna_trigger();
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_menu_down_trigger(ModContext*, void*, void* retval, void*) {
    if (!z_item_slot_enabled()) {
        return HOOK_CONTINUE;
    }

    *static_cast<BOOL*>(retval) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_check_item_button_change(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!z_item_slot_enabled() || link == nullptr) {
        return HOOK_CONTINUE;
    }

    if (link->mProcID != daAlink_c::PROC_CANOE_PADDLE_PUT &&
        link->mEquipItem != dItemNo_NONE_e &&
        !link->checkEquipAnime())
    {
        for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
            const u8 next = (i + 1) % kExtendedSelectItemCount;
            if (link->mEquipItem == resolved_select_item(i) &&
                (link->mEquipItem != resolved_select_item(next) || link->mSelectItemId != next))
            {
                link->mSelectItemId = i;
            }
        }
    }
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_check_item_change_from_button(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!z_item_slot_enabled() || link == nullptr) {
        return HOOK_CONTINUE;
    }

    BOOL result = FALSE;
    if (link->checkModeFlg(4) &&
        !link->checkEquipAnime() &&
        !link->checkBoomerangThrowAnime() &&
        !link->checkCopyRodThrowAnime() &&
        !link->checkKandelaarSwingAnime() &&
        !link->checkKandelaarSwingAnime())
    {
        if (
#if PLATFORM_GCN
            dComIfGs_getSelectEquipSword() != dItemNo_NONE_e &&
#endif
            !link->checkNotBattleStage() &&
            !link->checkCanoeRide() &&
            (!link->checkModeFlg(0x40000) || link->checkEquipHeavyBoots()) &&
            link->mEquipItem != 0x103 &&
            link->swordTrigger())
        {
            if (!link->checkEndResetFlg1(daPy_py_c::ERFLG1_SWORD_TRIGGER_NON)) {
                link->swordEquip(TRUE);
            }
        } else if (link->checkCanoeRide() &&
                   !link->checkStageName("F_SP103") &&
                   !link->checkCanoeSlider() &&
                   !link->checkFisingRodLure() &&
                   link->swordTrigger())
        {
            link->itemEquip(0x105);
        } else {
            for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
                const int procType = link->checkNewItemChange(i);
                if (procType != 0 && link->itemTriggerCheck(1 << i)) {
                    if (i == kZItemSlot &&
                        link->checkGroupItem(dItemNo_HVY_BOOTS_e, resolved_select_item(i)))
                    {
                        if (z_heavy_boots_input_locked(link)) {
                            continue;
                        }
                        lock_z_heavy_boots_input(link, link->checkEquipHeavyBoots());
                    }
                    result = link->changeItemTriggerKeepProc(i, procType);
                    *static_cast<BOOL*>(retval) = result;
                    return HOOK_SKIP_ORIGINAL;
                }
            }

            if (link->doTrigger() && dComIfGp_getDoStatus() == BUTTON_STATUS_PUT_AWAY) {
                if (link->mEquipItem != dItemNo_KANTERA_e && link->checkNoResetFlg2(daPy_py_c::FLG2_UNK_1)) {
                    link->offKandelaarModel();
                } else if (link->mSwordFlourishTimer != 0 && link->mEquipItem == 0x103 &&
                           !link->checkWoodSwordEquip() && !link->checkModeFlg(0x402))
                {
                    result = link->procSwordUnequipSpInit();
                } else {
                    link->allUnequip(TRUE);
                }
            } else if (link->mEquipItem == dItemNo_NONE_e &&
                       link->mThrowBoomerangAcKeep.getActor() == nullptr &&
                       !link->checkCanoeRide() &&
                       link->checkNoUpperAnime() &&
                       link->checkNoResetFlg2(daPy_py_c::FLG2_UNK_1))
            {
                for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
                    if (resolved_select_item(i) == dItemNo_KANTERA_e) {
                        link->mSelectItemId = i;
                    }
                }
                link->itemEquip(dItemNo_KANTERA_e);
                link->onNoResetFlg1(daPy_py_c::FLG1_UNK_40);
            } else if (link->mEquipItem != 0x103 &&
                       link->mEquipItem != dItemNo_NONE_e &&
                       link->mEquipItem != 0x10B &&
                       link->mEquipItem != 0x102 &&
                       (!link->checkCanoeRide() || !link->checkFisingRodLure()))
            {
                if (!link->checkEventRun() ||
                    std::strcmp(dComIfGp_getEventManager().getRunEventName(), "ANGER") != 0)
                {
                    if (std::strcmp(dComIfGp_getEventManager().getRunEventName(), "ANGER2") != 0 &&
                        find_select_button(link, link->mEquipItem) == kSelectItemNotFound)
                    {
                        link->allUnequip(TRUE);
                    }
                }
            }
        }
    }

    *static_cast<BOOL*>(retval) = result;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_check_set_item_trigger(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const int itemNo = mods::arg<int>(args, 1);
    if (!z_item_slot_enabled() || link == nullptr) {
        return HOOK_CONTINUE;
    }

    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        if (!link->checkGroupItem(itemNo, resolved_select_item(i)) || !link->itemTriggerCheck(1 << i)) {
            continue;
        }

        if (itemNo == dItemNo_HVY_BOOTS_e) {
            if (i == kZItemSlot) {
                if (z_heavy_boots_input_locked(link)) {
                    *static_cast<int*>(retval) = 0;
                    return HOOK_SKIP_ORIGINAL;
                }
                lock_z_heavy_boots_input(link, link->checkEquipHeavyBoots());
            }
        } else {
            link->mSelectItemId = i;
        }

        *static_cast<int*>(retval) = 1;
        return HOOK_SKIP_ORIGINAL;
    }

    *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_check_item_set_button(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const int itemNo = mods::arg<int>(args, 1);
    if (!z_item_slot_enabled() || link == nullptr || !item_needs_z_valid_button(itemNo)) {
        return HOOK_CONTINUE;
    }

    if (!link->checkGroupItem(itemNo, resolved_select_item(kZItemSlot))) {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = SELECT_ITEM_X;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_set_heavy_boots(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const int enable = mods::arg<int>(args, 1);
    if (!z_item_slot_enabled() || link == nullptr || !link->checkEquipHeavyBoots() ||
        link->checkNotHeavyBootsStage() || !z_heavy_boots_selected(link))
    {
        return HOOK_CONTINUE;
    }

    if (enable != 0 && s_zHeavyBootsGuardLink == link && s_zHeavyBootsManualToggleOff) {
        clear_z_heavy_boots_input_lock();
        return HOOK_CONTINUE;
    }

    if (enable != 0 && z_heavy_boots_input_locked(link)) {
        *static_cast<int*>(retval) = 0;
        return HOOK_SKIP_ORIGINAL;
    }

    if (enable == 0 && z_heavy_boots_forced_off_context(link)) {
        clear_z_heavy_boots_input_lock();
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

void after_player_execute(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!z_item_slot_enabled() || link == nullptr || link->checkWolf()) {
        return;
    }

    tick_z_heavy_boots_guard(link);
    sync_play_select_item(kZItemSlot);
    if (resolved_select_item(kZItemSlot) != dItemNo_NONE_e) {
        dMeter2Info_onUseButton(METER2_USEBUTTON_Z);
    }
}

ModResult add_hook(ModResult result, ModError* error) {
    return result == MOD_OK ? MOD_OK :
        mods::set_error(error, result, "failed to install Dawnlight Z item slot hooks");
}

}  // namespace

ModResult install_item_slot_hooks(ModError* error) {
    ModResult result = mods::hook_add_pre<GetSelectItemHook>(svc_hook, before_get_select_item);
    if (result == MOD_OK) {
        result = mods::hook_add_post<SetSelectItemHook>(svc_hook, after_set_select_item);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<RingCreateHook>(svc_hook, after_ring_create);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<RingDeleteHook>(svc_hook, before_ring_delete);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<RingDrawHook>(svc_hook, after_ring_draw);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterDrawHook>(svc_hook, before_meter_draw);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterDrawHook>(svc_hook, after_meter_draw);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterMidnaAlphaHook>(svc_hook, after_meter_midna_alpha);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<RingSetActiveCursorHook>(svc_hook, before_ring_set_active_cursor);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<RingSetActiveCursorHook>(svc_hook, after_ring_set_active_cursor);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MidnaTalkTriggerHook>(svc_hook, before_midna_talk_trigger);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MenuDownTriggerHook>(svc_hook, before_menu_down_trigger);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<CheckItemButtonChangeHook>(svc_hook, before_check_item_button_change);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<CheckItemChangeFromButtonHook>(svc_hook, before_check_item_change_from_button);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<CheckSetItemTriggerHook>(svc_hook, before_check_set_item_trigger);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<CheckItemSetButtonHook>(svc_hook, before_check_item_set_button);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<SetHeavyBootsHook>(svc_hook, before_set_heavy_boots);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<PlayerExecuteHook>(svc_hook, after_player_execute);
    }
    return add_hook(result, error);
}

}  // namespace dawnlight
