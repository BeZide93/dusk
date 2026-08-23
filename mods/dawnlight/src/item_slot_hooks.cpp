#include "config.hpp"
#include "hud_layout.hpp"
#include "service_imports.hpp"

#include "global.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_kantera_icon_meter.h"
#include "d/d_item.h"
#include "d/d_item_data.h"
#include "d/d_meter_button.h"
#include "d/d_meter_HIO.h"
#include "d/d_meter2_info.h"
#include "d/d_menu_window.h"
#include "d/d_menu_item_explain.h"
#include "d/d_pane_class.h"
#include "d/d_msg_object.h"
#include "d/d_save.h"
#include "JSystem/J2DGraph/J2DPane.h"
#include "JSystem/J2DGraph/J2DOrthoGraph.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JKernel/JKRMemArchive.h"
#define private public
#include "d/d_meter2.h"
#include "d/d_menu_ring.h"
#include "d/d_meter_map.h"
#include "d/d_meter2_draw.h"
#undef private
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_dvd_thread.h"
#include "m_Do/m_Do_ext.h"
#include "dusk/ui/controls.hpp"
#include "mods/hook.hpp"
#include "mods/service.hpp"
#include "mods/svc/hook.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

namespace Rml {
using String = std::string;
class Element;
}  // namespace Rml

namespace dusk::ui {
class TouchControls;
}  // namespace dusk::ui

namespace dawnlight {
namespace {

constexpr u8 kZItemSlot = SELECT_ITEM_DOWN;
constexpr u8 kDpadDownItemSlot = SELECT_ITEM_B;
constexpr u8 kExtraItemProcSlot = kZItemSlot;
constexpr int kExtendedSelectItemCount = MAX_SELECT_ITEM;
constexpr int kSelectItemNotFound = MAX_SELECT_ITEM;
constexpr int kItemProcBootsEquip = 1;
constexpr size_t kDawnlightReserveOffset = 0x8F0;
constexpr size_t kBossRushMarkerOffset = 32;
constexpr size_t kItemSlotStateOffset = 64;
constexpr size_t kItemSlotStateMagicSize = 8;
constexpr char kBossRushMarker[] = "DUSKBR1";
constexpr char kItemSlotStateMagic[] = "DUSKITM1";

DEFINE_HOOK(&dComIfGp_getSelectItem, GetSelectItemHook);
DEFINE_HOOK(&dComIfGp_setSelectItem, SetSelectItemHook);
DEFINE_HOOK(&dComIfGp_getSelectItemNum, GetSelectItemNumHook);
DEFINE_HOOK(&dComIfGp_getSelectItemMaxNum, GetSelectItemMaxNumHook);
DEFINE_HOOK(&dComIfGp_setSelectItemNum, SetSelectItemNumHook);
DEFINE_HOOK(&dComIfGp_addSelectItemNum, AddSelectItemNumHook);
DEFINE_HOOK(&dSv_player_item_c::setEquipBottleItemIn, SetEquipBottleItemInHook);
DEFINE_HOOK(&dMenu_Ring_c::_create, RingCreateHook);
DEFINE_HOOK(&dMenu_Ring_c::_delete, RingDeleteHook);
DEFINE_HOOK(&dMenu_Ring_c::_draw, RingDrawHook);
DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor, RingSetActiveCursorHook);
DEFINE_HOOK(&dMenu_Ring_c::isMixItemOn, RingIsMixItemOnHook);
DEFINE_HOOK(&dMenu_Ring_c::isMixItemOff, RingIsMixItemOffHook);
DEFINE_HOOK(&dMeter2Draw_c::draw, MeterDrawHook);
DEFINE_HOOK(&dMeter2Draw_c::drawKantera, MeterDrawKanteraHook);
DEFINE_HOOK(&dMeter2Draw_c::drawOxygen, MeterDrawOxygenHook);
DEFINE_HOOK(&dMeter2Draw_c::setButtonIconMidonaAlpha, MeterMidnaAlphaHook);
DEFINE_HOOK(&dMeter2Draw_c::drawButtonCross, MeterDrawButtonCrossHook);
DEFINE_HOOK(&dMeter2_c::moveButtonCross, MeterMoveButtonCrossHook);
DEFINE_HOOK(&dMeterButton_c::setString, MeterButtonSetStringHook);
DEFINE_HOOK(&dMeterButton_c::_execute, MeterButtonExecuteHook);
DEFINE_HOOK(&dMeterMap_c::draw, MeterMapDrawHook);
DEFINE_HOOK(&daAlink_c::midnaTalkTrigger, MidnaTalkTriggerHook);
DEFINE_HOOK(&mDoCPd_c::read, PadReadHook);
DEFINE_HOOK(&daAlink_c::checkItemButtonChange, CheckItemButtonChangeHook);
DEFINE_HOOK(&daAlink_c::checkItemChangeFromButton, CheckItemChangeFromButtonHook);
DEFINE_HOOK(&daAlink_c::checkSetItemTrigger, CheckSetItemTriggerHook);
DEFINE_HOOK(&daAlink_c::checkItemSetButton, CheckItemSetButtonHook);
DEFINE_HOOK(&daAlink_c::setHeavyBoots, SetHeavyBootsHook);
DEFINE_HOOK(&daAlink_c::execute, PlayerExecuteHook);
#if defined(__ANDROID__)
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls21sync_action_bar_stateEv",
    void(dusk::ui::TouchControls*), TouchSyncActionBarHook);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui13TouchControls19set_control_pressedENS0_7ControlEb",
    void(dusk::ui::TouchControls*, dusk::ui::Control, bool), TouchSetControlPressedHook);
DEFINE_HOOK_SYMBOL("_ZN3Rml7Element14SetPseudoClassERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEb",
    void(Rml::Element*, const Rml::String*, bool), RmlSetPseudoClassHook);
DEFINE_HOOK_SYMBOL("_ZN3Rml7Element11SetInnerRMLERKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE",
    void(Rml::Element*, const Rml::String*), RmlSetInnerRMLHook);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui17midna_icon_sourceEv", std::string(), MidnaIconSourceHook);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui19midna_icon_revisionEv", uint64_t(), MidnaIconRevisionHook);
DEFINE_HOOK_SYMBOL("_ZN4dusk2ui25update_midna_icon_textureEP7J2DPane",
    void(J2DPane*), UpdateMidnaIconTextureHook);
#endif

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
struct RingDpadDownPrompt {
    dMenu_Ring_c* ring = nullptr;
    J2DPicture* picture = nullptr;
    J2DPicture* overlay = nullptr;
};

RingDpadDownPrompt s_ringDpadDownPrompt;
alignas(32) u8 s_zHudItemTexBuf[2][2][0xC00];
u8 s_zHudItemTexPage = 0;
u8 s_zHudLastItem = dItemNo_NONE_e;
J2DPicture* s_zHudLastPicture = nullptr;
J2DPicture* s_zItemNumTex[3] = {};
dKantera_icon_c* s_zOilMeter = nullptr;
alignas(32) u8 s_dpadDownHudItemTexBuf[2][2][0xC00];
u8 s_dpadDownHudItemTexPage = 0;
u8 s_dpadDownHudLastItem = dItemNo_NONE_e;
J2DPicture* s_dpadDownHudItemPane[2] = {};
J2DPicture* s_dpadDownItemNumTex[3] = {};
dKantera_icon_c* s_dpadDownOilMeter = nullptr;
daAlink_c* s_zHeavyBootsGuardLink = nullptr;
bool s_zHeavyBootsManualToggleOff = false;
bool s_zHeavyBootsWaitRelease = false;
u8 s_zHeavyBootsGuardFrames = 0;
u8 s_zHeavyBootsGuardSlot = kZItemSlot;
bool s_dpadLeftHeld = false;
bool s_dpadLeftTrig = false;
bool s_dpadDownHeld = false;
bool s_dpadDownTrig = false;
bool s_touchMidnaTrig = false;
u8 s_touchMidnaBlockStartFrames = 0;
bool s_inTouchActionBarSync = false;
u8 s_touchActionBarHiddenCall = 0;
Rml::Element* s_skipTouchElement = nullptr;
bool s_skipTouchMidnaMode = false;
bool s_skipTouchMidnaPressed = false;
std::string s_skipTouchMidnaSource;
bool s_zPromptAsDpadLeftThisFrame = false;
bool s_zPromptCustomVisualsActive = false;
bool s_applyZPromptDpadIcon = false;
bool s_hideZPromptAfterExecute = false;
bool s_dpadDownProcSelectOverride = false;
u8 s_procSelectOverrideSlot = dItemNo_NONE_e;
u8 s_activeExtraProcSlot = dItemNo_NONE_e;
bool s_zHudSelectLookup = false;
bool s_dpadDownBottleSlotRedirect = false;
u8 s_dpadDownBottleOldSelectSlot = dItemNo_NONE_e;

struct ZPromptVisualState {
    J2DPicture* icon = nullptr;
    J2DPicture* overlay = nullptr;
    ResTIMG const* textures[2] = {};
    ResTIMG const* overlayTextures[2] = {};
    JUtility::TColor black = 0;
    JUtility::TColor white = 0xFFFFFFFF;
    JUtility::TColor cornerColors[4] = {};
    JUtility::TColor overlayBlack = 0;
    JUtility::TColor overlayWhite = 0xFFFFFFFF;
    JUtility::TColor overlayCornerColors[4] = {};
    JGeometry::TVec2<s16> overlayTexCoords[4] = {};
    JGeometry::TBox2<f32> overlayBounds = {};
    f32 overlayScaleX = 1.0f;
    f32 overlayScaleY = 1.0f;
    f32 overlayTranslateX = 0.0f;
    f32 overlayTranslateY = 0.0f;
    f32 overlayRotateX = 0.0f;
    f32 overlayRotateY = 0.0f;
    f32 overlayRotateZ = 0.0f;
    f32 overlayRotateOffsetX = 0.0f;
    f32 overlayRotateOffsetY = 0.0f;
    char overlayRotAxis = ROTATE_Z;
    u8 overlayBasePosition = 0;
    J2DPane* hiddenPanes[16] = {};
    bool hiddenVisible[16] = {};
    u8 textureCount = 0;
    u8 overlayTextureCount = 0;
    u8 overlayAlpha = 0xFF;
    u8 hiddenCount = 0;
    bool iconVisible = false;
    bool overlayVisible = false;
    bool overlayActive = false;
    bool labelVisible = false;
};

ZPromptVisualState s_zPromptVisualState;
mDoDvdThd_mountArchive_c* s_dpadPromptArchiveMount = nullptr;
JKRArchive* s_dpadPromptArchive = nullptr;
u8* s_dpadPromptTextureBuffer = nullptr;
u32 s_dpadPromptTextureSize = 0;
u8 s_dpadPromptArchivePathIndex = 0;
bool s_dpadPromptArchiveUnavailable = false;

void reset_z_prompt_visual_state() {
    s_zPromptVisualState = ZPromptVisualState();
}

void clear_z_prompt_custom_visuals() {
    reset_z_prompt_visual_state();
    s_zPromptCustomVisualsActive = false;
}

struct J2DPictureTexCoordAccess : J2DPicture {
    static JGeometry::TVec2<s16>* coords(J2DPicture* picture) {
        return reinterpret_cast<J2DPictureTexCoordAccess*>(picture)->field_0x10a;
    }
};

struct ScopedBool {
    bool& ref;
    bool old;

    ScopedBool(bool& value, bool next) : ref(value), old(value) {
        ref = next;
    }

    ~ScopedBool() {
        ref = old;
    }
};

struct ScopedU8 {
    u8& ref;
    u8 old;

    ScopedU8(u8& value, u8 next) : ref(value), old(value) {
        ref = next;
    }

    ~ScopedU8() {
        ref = old;
    }
};

struct HudPaneTransformState {
    J2DPane* pane = nullptr;
    f32 offsetX = 0.0f;
    f32 offsetY = 0.0f;
    f32 scale = 1.0f;
    f32 appliedX = 0.0f;
    f32 appliedY = 0.0f;
    f32 appliedScaleX = 1.0f;
    f32 appliedScaleY = 1.0f;
    u8 originalTextFlags = 0;
    bool hasOriginalTextFlags = false;
    bool active = false;
};

struct HudTextBoxFlagState {
    J2DTextBox* textBox = nullptr;
    u8 originalFlags = 0;
    bool active = false;
};

enum class HudPaneSlot : std::size_t {
    ButtonA,
    TextA,
    ButtonB,
    ItemB,
    LightB,
    TextB,
    ButtonX,
    ItemX,
    LightX,
    TextX,
    ButtonY,
    ItemY,
    LightY,
    TextY,
    ButtonZ,
    TextZ,
    Backing,
    DPad,
    Hearts,
    Rupee0,
    Rupee1,
    Rupee2,
    Keys,
    Count,
};

std::array<HudPaneTransformState, static_cast<std::size_t>(HudPaneSlot::Count)>
    s_wiiUHudPaneTransforms;
std::array<std::array<HudTextBoxFlagState, 5>, static_cast<std::size_t>(HudPaneSlot::Count)>
    s_hudTextBoxFlags;
std::array<dMeter2Draw_c::item_params, 2> s_xyAmmoOriginalParams = {};
std::array<bool, 2> s_xyAmmoOriginalValid = {};

struct MinimapTransformState {
    dMeterMap_c* map = nullptr;
    f32 drawPosX = 0.0f;
    f32 drawPosY = 0.0f;
    f32 sizeW = 0.0f;
    f32 sizeH = 0.0f;
    bool active = false;
};

MinimapTransformState s_wiiUMinimapTransform;

struct RoundPictureState {
    J2DPicture* picture = nullptr;
    ResTIMG const* textures[2] = {};
    u8 textureCount = 0;
    f32 left = 0.0f;
    f32 top = 0.0f;
    f32 width = 0.0f;
    f32 height = 0.0f;
    bool active = false;
};

std::array<RoundPictureState, 32> s_roundPictureStates;
dMeter2Draw_c* s_roundHudMeter = nullptr;

bool consume_touch_midna_trigger() {
    const bool triggered = s_touchMidnaTrig;
    s_touchMidnaTrig = false;
    return triggered;
}

J2DPane* prompt_pane(dMeterButton_c* meter, u64 tag) {
    return meter != nullptr && meter->mpButtonScreen != nullptr ? meter->mpButtonScreen->search(tag) :
                                                                  nullptr;
}

J2DPicture* prompt_picture(dMeterButton_c* meter, u64 tag) {
    J2DPane* pane = prompt_pane(meter, tag);
    if (pane == nullptr || pane->getTypeID() != 18) {
        return nullptr;
    }

    return static_cast<J2DPicture*>(pane);
}

void set_prompt_pane_visible(dMeterButton_c* meter, u64 tag, bool visible) {
    J2DPane* pane = prompt_pane(meter, tag);
    if (pane == nullptr) {
        return;
    }

    if (visible) {
        pane->show();
    } else {
        pane->hide();
    }
}

void hide_z_prompt_panes(dMeterButton_c* meter) {
    set_prompt_pane_visible(meter, MULTI_CHAR('zbtn_n'), false);
    set_prompt_pane_visible(meter, 'zbtn', false);
    set_prompt_pane_visible(meter, MULTI_CHAR('z_btnl'), false);
}

bool pane_tree_contains(J2DPane* root, J2DPane* pane) {
    if (root == nullptr || pane == nullptr) {
        return false;
    }

    if (root == pane) {
        return true;
    }

    for (J2DPane* child = root->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        if (pane_tree_contains(child, pane)) {
            return true;
        }
    }

    return false;
}

void save_and_hide_z_prompt_pane(J2DPane* pane) {
    if (pane == nullptr) {
        return;
    }

    for (u8 i = 0; i < s_zPromptVisualState.hiddenCount; ++i) {
        if (s_zPromptVisualState.hiddenPanes[i] == pane) {
            pane->hide();
            return;
        }
    }

    if (s_zPromptVisualState.hiddenCount >= 16) {
        pane->hide();
        return;
    }

    const u8 index = s_zPromptVisualState.hiddenCount++;
    s_zPromptVisualState.hiddenPanes[index] = pane;
    s_zPromptVisualState.hiddenVisible[index] = pane->isVisible();
    pane->hide();
}

void hide_z_prompt_extra_layers(J2DPane* pane, J2DPane* icon, J2DPane* overlay, J2DPane* midna) {
    if (pane == nullptr || pane == icon || pane == overlay || pane == midna) {
        return;
    }

    if (pane_tree_contains(pane, midna)) {
        for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
             child = child->getNextChildPane())
        {
            hide_z_prompt_extra_layers(child, icon, overlay, midna);
        }
        return;
    }

    const u16 type = pane->getTypeID();
    if (type != 16 && type != 17) {
        save_and_hide_z_prompt_pane(pane);
        return;
    }

    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        hide_z_prompt_extra_layers(child, icon, overlay, midna);
    }
}

JKRArchive* dpad_prompt_archive() {
    if (s_dpadPromptArchive != nullptr || s_dpadPromptArchiveUnavailable) {
        return s_dpadPromptArchive;
    }

    if (s_dpadPromptArchiveMount != nullptr) {
        if (!s_dpadPromptArchiveMount->sync()) {
            return nullptr;
        }

        s_dpadPromptArchive = static_cast<JKRArchive*>(s_dpadPromptArchiveMount->getArchive());
        s_dpadPromptArchiveMount->destroy();
        s_dpadPromptArchiveMount = nullptr;
        if (s_dpadPromptArchive == nullptr) {
            s_dpadPromptArchiveUnavailable = true;
        }
        return s_dpadPromptArchive;
    }

    constexpr const char* paths[] = {
        "/res/Layout/clctresR.arc",
        "/res/LayoutRevo/clctresR.arc",
    };

    while (s_dpadPromptArchivePathIndex < sizeof(paths) / sizeof(paths[0])) {
        s_dpadPromptArchiveMount =
            mDoDvdThd_mountArchive_c::create(paths[s_dpadPromptArchivePathIndex++], 0,
                                             static_cast<JKRHeap*>(mDoExt_getJ2dHeap()));
        if (s_dpadPromptArchiveMount != nullptr) {
            return nullptr;
        }
    }

    s_dpadPromptArchiveUnavailable = true;
    return nullptr;
}

ResTIMG const* load_dpad_prompt_texture_from_archive() {
    if (s_dpadPromptTextureBuffer != nullptr) {
        return reinterpret_cast<ResTIMG const*>(s_dpadPromptTextureBuffer);
    }

    JKRArchive* archive = dpad_prompt_archive();
    if (archive == nullptr) {
        return nullptr;
    }

    constexpr const char* dpadNames[] = {
        "im_juji_key_03.bti",
        "im_juji_key.bti",
    };

    for (const char* name : dpadNames) {
        void* resource = archive->getResource('TIMG', name);
        if (resource == nullptr) {
            continue;
        }

        const u32 size = archive->getExpandedResSize(resource);
        if (size == 0 || size == 0xFFFFFFFF) {
            archive->removeResource(resource);
            continue;
        }
        archive->removeResource(resource);

        auto* buffer = static_cast<u8*>(
            JKRAllocFromHeap(static_cast<JKRHeap*>(mDoExt_getJ2dHeap()), size, 0x20));
        if (buffer == nullptr) {
            continue;
        }

        const u32 readSize = archive->readResource(buffer, size, 'TIMG', name);
        if (readSize == 0) {
            JKRFreeToHeap(mDoExt_getJ2dHeap(), buffer);
            continue;
        }

        s_dpadPromptTextureBuffer = buffer;
        s_dpadPromptTextureSize = readSize;
        return reinterpret_cast<ResTIMG const*>(s_dpadPromptTextureBuffer);
    }

    return nullptr;
}

ResTIMG const* z_prompt_dpad_texture(dMeterButton_c*) {
    constexpr const char* dpadNames[] = {
        "im_juji_key_03.bti",
        "im_juji_key.bti",
    };

    JKRArchive* archives[] = {
        dComIfGp_getCollectResArchive(),
        dComIfGp_getMain2DArchive(),
        dComIfGp_getMeterButtonArchive(),
    };

    for (JKRArchive* archive : archives) {
        if (archive == nullptr) {
            continue;
        }

        for (const char* name : dpadNames) {
            if (auto* dpadTexture =
                    static_cast<ResTIMG const*>(archive->getResource('TIMG', name)))
            {
                return dpadTexture;
            }
        }
    }

    return load_dpad_prompt_texture_from_archive();
}

void restore_z_prompt_visuals(dMeterButton_c* meter) {
    if (!s_zPromptCustomVisualsActive || meter == nullptr) {
        return;
    }

    if (meter->mpButtonScreen == nullptr ||
        prompt_picture(meter, 'zbtn') != s_zPromptVisualState.icon ||
        prompt_picture(meter, MULTI_CHAR('z_btnl')) != s_zPromptVisualState.overlay)
    {
        clear_z_prompt_custom_visuals();
        return;
    }

    if (s_zPromptVisualState.icon != nullptr) {
        for (u8 i = 0; i < s_zPromptVisualState.textureCount; ++i) {
            if (s_zPromptVisualState.textures[i] != nullptr) {
                s_zPromptVisualState.icon->changeTexture(s_zPromptVisualState.textures[i], i);
            }
        }
        if (JUTTexture* texture = s_zPromptVisualState.icon->getTexture(0)) {
            s_zPromptVisualState.icon->setTexCoord(texture, BIND15, MIRROR0, false);
        }
        s_zPromptVisualState.icon->setBlackWhite(s_zPromptVisualState.black,
                                                 s_zPromptVisualState.white);
        s_zPromptVisualState.icon->setCornerColor(s_zPromptVisualState.cornerColors[0],
                                                  s_zPromptVisualState.cornerColors[1],
                                                  s_zPromptVisualState.cornerColors[2],
                                                  s_zPromptVisualState.cornerColors[3]);
        if (s_zPromptVisualState.iconVisible) {
            s_zPromptVisualState.icon->show();
        } else {
            s_zPromptVisualState.icon->hide();
        }
    }

    if (J2DPane* label = prompt_pane(meter, MULTI_CHAR('z_btnl'))) {
        if (s_zPromptVisualState.labelVisible) {
            label->show();
        } else {
            label->hide();
        }
    }

    if (s_zPromptVisualState.overlayActive && s_zPromptVisualState.overlay != nullptr) {
        for (u8 i = 0; i < s_zPromptVisualState.overlayTextureCount; ++i) {
            if (s_zPromptVisualState.overlayTextures[i] != nullptr) {
                s_zPromptVisualState.overlay->changeTexture(
                    s_zPromptVisualState.overlayTextures[i], i);
            }
        }
        s_zPromptVisualState.overlay->setBlackWhite(s_zPromptVisualState.overlayBlack,
                                                    s_zPromptVisualState.overlayWhite);
        s_zPromptVisualState.overlay->setCornerColor(
            s_zPromptVisualState.overlayCornerColors[0],
            s_zPromptVisualState.overlayCornerColors[1],
            s_zPromptVisualState.overlayCornerColors[2],
            s_zPromptVisualState.overlayCornerColors[3]);
        s_zPromptVisualState.overlay->place(s_zPromptVisualState.overlayBounds);
        s_zPromptVisualState.overlay->scale(s_zPromptVisualState.overlayScaleX,
                                            s_zPromptVisualState.overlayScaleY);
        s_zPromptVisualState.overlay->translate(s_zPromptVisualState.overlayTranslateX,
                                                s_zPromptVisualState.overlayTranslateY);
        s_zPromptVisualState.overlay->mRotateX = s_zPromptVisualState.overlayRotateX;
        s_zPromptVisualState.overlay->mRotateY = s_zPromptVisualState.overlayRotateY;
        s_zPromptVisualState.overlay->mRotateZ = s_zPromptVisualState.overlayRotateZ;
        s_zPromptVisualState.overlay->mRotateOffsetX =
            s_zPromptVisualState.overlayRotateOffsetX;
        s_zPromptVisualState.overlay->mRotateOffsetY =
            s_zPromptVisualState.overlayRotateOffsetY;
        s_zPromptVisualState.overlay->mRotAxis = s_zPromptVisualState.overlayRotAxis;
        s_zPromptVisualState.overlay->mBasePosition = s_zPromptVisualState.overlayBasePosition;
        s_zPromptVisualState.overlay->calcMtx();
        s_zPromptVisualState.overlay->setAlpha(s_zPromptVisualState.overlayAlpha);
        JGeometry::TVec2<s16>* overlayCoords =
            J2DPictureTexCoordAccess::coords(s_zPromptVisualState.overlay);
        for (u8 i = 0; i < 4; ++i) {
            overlayCoords[i] = s_zPromptVisualState.overlayTexCoords[i];
        }
        if (s_zPromptVisualState.overlayVisible) {
            s_zPromptVisualState.overlay->show();
        } else {
            s_zPromptVisualState.overlay->hide();
        }
    }

    for (u8 i = 0; i < s_zPromptVisualState.hiddenCount; ++i) {
        J2DPane* pane = s_zPromptVisualState.hiddenPanes[i];
        if (pane == nullptr) {
            continue;
        }

        if (s_zPromptVisualState.hiddenVisible[i]) {
            pane->show();
        } else {
            pane->hide();
        }
    }

    clear_z_prompt_custom_visuals();
}

void apply_z_prompt_visuals(dMeterButton_c* meter) {
    if (meter == nullptr || meter->mpButtonScreen == nullptr) {
        return;
    }

    J2DPicture* icon = prompt_picture(meter, 'zbtn');
    J2DPicture* overlay = prompt_picture(meter, MULTI_CHAR('z_btnl'));
    ResTIMG const* dpadTexture = z_prompt_dpad_texture(meter);
    if (icon == nullptr || dpadTexture == nullptr) {
        return;
    }

    if (!s_zPromptCustomVisualsActive || s_zPromptVisualState.icon != icon ||
        s_zPromptVisualState.overlay != overlay)
    {
        reset_z_prompt_visual_state();
        s_zPromptVisualState.icon = icon;
        s_zPromptVisualState.overlay = overlay;
        s_zPromptVisualState.textureCount = std::min<u8>(icon->getTextureCount(), 2);
        for (u8 i = 0; i < s_zPromptVisualState.textureCount; ++i) {
            if (JUTTexture* texture = icon->getTexture(i)) {
                s_zPromptVisualState.textures[i] = texture->getTexInfo();
            }
        }
        s_zPromptVisualState.black = icon->getBlack();
        s_zPromptVisualState.white = icon->getWhite();
        for (u8 i = 0; i < 4; ++i) {
            s_zPromptVisualState.cornerColors[i] = icon->corner(i);
        }
        s_zPromptVisualState.iconVisible = icon->isVisible();
        if (J2DPane* label = prompt_pane(meter, MULTI_CHAR('z_btnl'))) {
            s_zPromptVisualState.labelVisible = label->isVisible();
        }
        if (overlay != nullptr && overlay != icon) {
            s_zPromptVisualState.overlayTextureCount =
                std::min<u8>(overlay->getTextureCount(), 2);
            for (u8 i = 0; i < s_zPromptVisualState.overlayTextureCount; ++i) {
                if (JUTTexture* texture = overlay->getTexture(i)) {
                    s_zPromptVisualState.overlayTextures[i] = texture->getTexInfo();
                }
            }
            s_zPromptVisualState.overlayBlack = overlay->getBlack();
            s_zPromptVisualState.overlayWhite = overlay->getWhite();
            JGeometry::TVec2<s16>* overlayCoords = J2DPictureTexCoordAccess::coords(overlay);
            for (u8 i = 0; i < 4; ++i) {
                s_zPromptVisualState.overlayCornerColors[i] = overlay->corner(i);
                s_zPromptVisualState.overlayTexCoords[i] = overlayCoords[i];
            }
            s_zPromptVisualState.overlayBounds = overlay->getBounds();
            s_zPromptVisualState.overlayScaleX = overlay->getScaleX();
            s_zPromptVisualState.overlayScaleY = overlay->getScaleY();
            s_zPromptVisualState.overlayTranslateX = overlay->getTranslateX();
            s_zPromptVisualState.overlayTranslateY = overlay->getTranslateY();
            s_zPromptVisualState.overlayRotateX = overlay->mRotateX;
            s_zPromptVisualState.overlayRotateY = overlay->mRotateY;
            s_zPromptVisualState.overlayRotateZ = overlay->mRotateZ;
            s_zPromptVisualState.overlayRotateOffsetX = overlay->mRotateOffsetX;
            s_zPromptVisualState.overlayRotateOffsetY = overlay->mRotateOffsetY;
            s_zPromptVisualState.overlayRotAxis = overlay->mRotAxis;
            s_zPromptVisualState.overlayBasePosition = overlay->mBasePosition;
            s_zPromptVisualState.overlayAlpha = overlay->getAlpha();
            s_zPromptVisualState.overlayVisible = overlay->isVisible();
            s_zPromptVisualState.overlayActive =
                s_zPromptVisualState.overlayTextureCount > 0;
        }
    }

    for (u8 i = 0; i < s_zPromptVisualState.textureCount; ++i) {
        icon->changeTexture(dpadTexture, i);
    }
    if (JUTTexture* texture = icon->getTexture(0)) {
        icon->setTexCoord(texture, BIND15, J2DMirror_X, false);
    }
    icon->setBlackWhite(JUtility::TColor(0x00000000), JUtility::TColor(0xFFFFFFFF));
    icon->setCornerColor(JUtility::TColor(0xFFFFFFFF));
    icon->show();
    if (s_zPromptVisualState.overlayActive && overlay != nullptr && overlay != icon) {
        for (u8 i = 0; i < s_zPromptVisualState.overlayTextureCount; ++i) {
            overlay->changeTexture(dpadTexture, i);
        }

        overlay->mBounds = icon->mBounds;
        overlay->mBounds.f.x = overlay->mBounds.i.x + icon->mBounds.getWidth() * (1.0f / 3.0f);
        overlay->mScaleX = icon->mScaleX;
        overlay->mScaleY = icon->mScaleY;
        overlay->mTranslateX = icon->mTranslateX;
        overlay->mTranslateY = icon->mTranslateY;
        overlay->mRotateX = icon->mRotateX;
        overlay->mRotateY = icon->mRotateY;
        overlay->mRotateZ = icon->mRotateZ;
        overlay->mRotateOffsetX = icon->mRotateOffsetX;
        overlay->mRotateOffsetY = icon->mRotateOffsetY;
        overlay->mRotAxis = icon->mRotAxis;
        overlay->mBasePosition = icon->mBasePosition;
        overlay->calcMtx();
        overlay->setBlackWhite(JUtility::TColor(0x00000000), JUtility::TColor(0xFFFFFFFF));
        overlay->setCornerColor(JUtility::TColor(0xFF000090));
        overlay->setAlpha(0x90);
        JGeometry::TVec2<s16>* overlayCoords = J2DPictureTexCoordAccess::coords(overlay);
        overlayCoords[0].set(256, 0);
        overlayCoords[1].set(171, 0);
        overlayCoords[2].set(256, 256);
        overlayCoords[3].set(171, 256);
        overlay->show();
    } else {
        set_prompt_pane_visible(meter, MULTI_CHAR('z_btnl'), false);
    }
    hide_z_prompt_extra_layers(prompt_pane(meter, MULTI_CHAR('zbtn_n')), icon, overlay,
                               prompt_pane(meter, MULTI_CHAR('midona')));
    s_zPromptCustomVisualsActive = true;
}

bool z_item_menu_or_pause_context();
bool midna_unlocked();
u8 resolved_select_item(int index);
bool z_item_ammo_values_for_slot(int slotIndex, u8 itemNo, u8& itemNum, u8& itemMax);

bool any_extra_item_slot_enabled() {
    return z_item_slot_enabled() || dpad_down_item_slot_enabled();
}

bool extra_item_slot_enabled(const int index) {
    switch (index) {
    case kZItemSlot:
        return z_item_slot_enabled();
    case kDpadDownItemSlot:
        return dpad_down_item_slot_enabled();
    default:
        return false;
    }
}

u8* item_slot_state_bytes() {
    dSv_save_c* save = dComIfGs_getSaveData();
    u8* reserve = save == nullptr ? nullptr :
        reinterpret_cast<u8*>(save) + kDawnlightReserveOffset;
    if (reserve == nullptr) {
        return nullptr;
    }

    u8* state = reserve + kItemSlotStateOffset;
    if (std::memcmp(state, kItemSlotStateMagic, kItemSlotStateMagicSize) != 0) {
        const u8 oldZSlot = dComIfGs_getSelectItemIndex(kZItemSlot);
        const u8 oldZMix = dComIfGs_getMixItemIndex(kZItemSlot);
        std::memcpy(state, kItemSlotStateMagic, kItemSlotStateMagicSize);
        state[kItemSlotStateMagicSize + 0] = oldZSlot;
        state[kItemSlotStateMagicSize + 1] = oldZMix;
        state[kItemSlotStateMagicSize + 2] = dItemNo_NONE_e;
        state[kItemSlotStateMagicSize + 3] = dItemNo_NONE_e;
    }
    return state;
}

int dawnlight_extra_slot_index(const int index) {
    switch (index) {
    case kZItemSlot:
        return 0;
    case kDpadDownItemSlot:
        return 1;
    default:
        return -1;
    }
}

bool is_dawnlight_item_slot(const int index) {
    return dawnlight_extra_slot_index(index) >= 0;
}

bool is_active_dawnlight_item_slot(const int index) {
    return is_dawnlight_item_slot(index) && extra_item_slot_enabled(index);
}

bool logical_item_slot_enabled(const int index) {
    if (index == SELECT_ITEM_X || index == SELECT_ITEM_Y) {
        return true;
    }
    return is_active_dawnlight_item_slot(index);
}

u8 stored_select_slot(const int index) {
    const int extra = dawnlight_extra_slot_index(index);
    if (extra < 0) {
        return dComIfGs_getSelectItemIndex(index);
    }

    u8* state = item_slot_state_bytes();
    return state != nullptr ? state[kItemSlotStateMagicSize + extra * 2] : dItemNo_NONE_e;
}

u8 stored_mix_slot(const int index) {
    const int extra = dawnlight_extra_slot_index(index);
    if (extra < 0) {
        return dComIfGs_getMixItemIndex(index);
    }

    u8* state = item_slot_state_bytes();
    return state != nullptr ? state[kItemSlotStateMagicSize + extra * 2 + 1] : dItemNo_NONE_e;
}

void set_stored_select_slot(const int index, const u8 slot) {
    const int extra = dawnlight_extra_slot_index(index);
    if (extra < 0) {
        dComIfGs_setSelectItemIndex(index, slot);
        return;
    }

    u8* state = item_slot_state_bytes();
    if (state != nullptr) {
        state[kItemSlotStateMagicSize + extra * 2] = slot;
    }
}

void set_stored_mix_slot(const int index, const u8 slot) {
    const int extra = dawnlight_extra_slot_index(index);
    if (extra < 0) {
        dComIfGs_setMixItemIndex(index, slot);
        return;
    }

    u8* state = item_slot_state_bytes();
    if (state != nullptr) {
        state[kItemSlotStateMagicSize + extra * 2 + 1] = slot;
    }
}

struct PaneRenderState {
    J2DPane* pane = nullptr;
    u8 alpha = 0;
    bool visible = false;
};

ResTIMG* z_hud_item_tex(const u8 page, const u8 layer) {
    return reinterpret_cast<ResTIMG*>(s_zHudItemTexBuf[page][layer]);
}

u8 hud_texture_item(u8 itemNo) {
    return itemNo == dItemNo_LIGHT_ARROW_e ? dItemNo_BOW_e : itemNo;
}

std::string z_touch_item_source() {
    const u8 itemNo = resolved_select_item(kZItemSlot);
    if (itemNo == dItemNo_NONE_e || itemNo == 0 || daPy_py_c::checkNowWolf()) {
        return {};
    }

    char source[48] = {};
    const u8 textureItem = hud_texture_item(itemNo);
    std::snprintf(source, sizeof(source), "item://item/%02x?dawnlight_z=%02x", textureItem, itemNo);
    return source;
}

uint64_t z_touch_item_revision() {
    const u8 itemNo = resolved_select_item(kZItemSlot);
    return itemNo == dItemNo_NONE_e ? 0 : (0xDA000000ull | static_cast<uint64_t>(itemNo));
}

bool cutscene_skip_touch_visible() {
    auto* event = dComIfGp_getEvent();
    return event != nullptr && event->mEventStatus == 1 && event->mSkipFunc != nullptr &&
           !event->chkFlag2(2);
}

bool touch_midna_controls_suppressed() {
    return dComIfGp_event_runCheck() ||
           (dComIfGp_getMsgObjectClass() != nullptr && dMsgObject_isTalkNowCheck()) ||
           z_item_menu_or_pause_context();
}

bool boss_rush_save_active() {
    dSv_save_c* save = dComIfGs_getSaveData();
    const u8* reserve = save == nullptr ? nullptr :
        reinterpret_cast<const u8*>(save) + kDawnlightReserveOffset;
    return reserve != nullptr &&
           std::memcmp(
               reserve + kBossRushMarkerOffset, kBossRushMarker, sizeof(kBossRushMarker) - 1) == 0;
}

bool midna_touch_available() {
    return midna_unlocked() || boss_rush_save_active();
}

bool skip_touch_can_be_midna() {
    return z_item_slot_enabled() && !cutscene_skip_touch_visible() &&
           !touch_midna_controls_suppressed() && midna_touch_available() &&
           dComIfGp_getLinkPlayer() != nullptr;
}

#if defined(__ANDROID__)
std::string true_midna_icon_source() {
    if (MidnaIconSourceHook::g_orig == nullptr) {
        return {};
    }
    return MidnaIconSourceHook::g_orig();
}

void set_skip_touch_rml(Rml::Element* element, const std::string& rml) {
    if (element == nullptr || RmlSetInnerRMLHook::g_orig == nullptr) {
        return;
    }
    RmlSetInnerRMLHook::g_orig(element, &rml);
}

void set_skip_touch_hidden(Rml::Element* element, const bool hidden) {
    if (element == nullptr || RmlSetPseudoClassHook::g_orig == nullptr) {
        return;
    }

    const std::string hiddenClass = "hidden";
    RmlSetPseudoClassHook::g_orig(element, &hiddenClass, hidden);
}

void sync_skip_touch_midna_button(Rml::Element* element) {
    const std::string source = true_midna_icon_source();
    if (element == s_skipTouchElement && s_skipTouchMidnaMode &&
        (!s_skipTouchMidnaSource.empty() || source == s_skipTouchMidnaSource))
    {
        return;
    }

    s_skipTouchElement = element;
    s_skipTouchMidnaMode = true;

    if (source.empty()) {
        s_skipTouchMidnaSource.clear();
        set_skip_touch_rml(element, "<span>Midna</span>");
        return;
    }

    s_skipTouchMidnaSource = source;
    set_skip_touch_rml(element,
        "<img class=\"midna-icon visible\" src=\"" + source + "\" /><span></span>");
}

void restore_skip_touch_button(Rml::Element* element) {
    if (element == s_skipTouchElement && !s_skipTouchMidnaMode) {
        return;
    }

    s_skipTouchElement = element;
    s_skipTouchMidnaMode = false;
    s_skipTouchMidnaPressed = false;
    s_skipTouchMidnaSource.clear();
    set_skip_touch_rml(element, "<icon><glyph>&#xe044;</glyph></icon>");
}

void collect_pane_render_state(
    J2DPane* pane, std::array<PaneRenderState, 64>& states, size_t& count) {
    if (pane == nullptr || count >= states.size()) {
        return;
    }

    states[count++] = {
        .pane = pane,
        .alpha = pane->getAlpha(),
        .visible = pane->isVisible(),
    };
    pane->show();
    pane->setAlpha(255);

    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        collect_pane_render_state(child, states, count);
    }
}

void restore_pane_render_state(const std::array<PaneRenderState, 64>& states, size_t count) {
    while (count > 0) {
        const PaneRenderState& state = states[--count];
        if (state.pane == nullptr) {
            continue;
        }

        state.pane->setAlpha(state.alpha);
        if (state.visible) {
            state.pane->show();
        } else {
            state.pane->hide();
        }
    }
}

void refresh_midna_touch_icon_texture(J2DPane* midnaPane) {
    if (midnaPane == nullptr || UpdateMidnaIconTextureHook::g_orig == nullptr) {
        return;
    }

    std::array<PaneRenderState, 64> states{};
    size_t count = 0;
    collect_pane_render_state(midnaPane, states, count);
    UpdateMidnaIconTextureHook::g_orig(midnaPane);
    restore_pane_render_state(states, count);
}
#else
void set_skip_touch_hidden(Rml::Element*, bool) {}
void sync_skip_touch_midna_button(Rml::Element*) {}
void restore_skip_touch_button(Rml::Element*) {}
void refresh_midna_touch_icon_texture(J2DPane*) {}
#endif

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

void set_pane_tree_alpha_visible(J2DPane* pane, const bool visible, const u8 alpha) {
    if (pane == nullptr) {
        return;
    }

    pane->setAlpha(alpha);
    if (visible) {
        pane->show();
    } else {
        pane->hide();
    }

    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        set_pane_tree_alpha_visible(child, visible, alpha);
    }
}

void set_pane_influenced_alpha_tree(J2DPane* pane, const bool influenced) {
    if (pane == nullptr) {
        return;
    }

    pane->setInfluencedAlpha(influenced, true);
    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        set_pane_influenced_alpha_tree(child, influenced);
    }
}

void show_pane_parents(J2DPane* pane) {
    for (J2DPane* parent = pane; parent != nullptr; parent = parent->getParentPane()) {
        parent->show();
    }
}

J2DPicture* as_picture(J2DPane* pane) {
    if (pane == nullptr || pane->getTypeID() != 18) {
        return nullptr;
    }

    return static_cast<J2DPicture*>(pane);
}

J2DPicture* first_picture_pane(J2DPane* pane) {
    if (J2DPicture* picture = as_picture(pane)) {
        return picture;
    }

    if (pane == nullptr) {
        return nullptr;
    }

    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        if (J2DPicture* picture = first_picture_pane(child)) {
            return picture;
        }
    }

    return nullptr;
}

ResTIMG const* round_hud_button_texture(CPaneMgr* roundSource) {
    auto* archive = dComIfGp_getMain2DArchive();
    if (archive != nullptr) {
        auto* texture = static_cast<ResTIMG const*>(
            archive->getResource('TIMG', "tt_zelda_button_ab_maru.bti"));
        if (texture != nullptr) {
            return texture;
        }
    }

    if (roundSource == nullptr) {
        return nullptr;
    }

    J2DPicture* source = first_picture_pane(roundSource->getPanePtr());
    if (source == nullptr || source->getTexture(0) == nullptr) {
        return nullptr;
    }

    return source->getTexture(0)->getTexInfo();
}

void resize_pane_around_center(J2DPane* pane, const f32 width, const f32 height) {
    JGeometry::TBox2<f32> bounds = pane->getBounds();
    const f32 centerX = bounds.i.x + bounds.getWidth() * 0.5f;
    const f32 centerY = bounds.i.y + bounds.getHeight() * 0.5f;

    pane->resize(width, height);
    pane->move(centerX - width * 0.5f, centerY - height * 0.5f);
}

void make_hud_button_picture_square(J2DPicture* picture) {
    const f32 width = picture->getWidth();
    const f32 height = picture->getHeight();
    if (width <= 0.0f || height <= 0.0f || std::fabs(width - height) < 0.01f) {
        return;
    }

    const f32 size = width < height ? width : height;
    resize_pane_around_center(picture, size, size);
}

RoundPictureState* round_picture_state(J2DPicture* picture) {
    for (auto& state : s_roundPictureStates) {
        if (state.active && state.picture == picture) {
            return &state;
        }
    }

    for (auto& state : s_roundPictureStates) {
        if (!state.active) {
            return &state;
        }
    }

    return nullptr;
}

void capture_round_picture_state(RoundPictureState& state, J2DPicture* picture) {
    if (state.active) {
        return;
    }

    const JGeometry::TBox2<f32> bounds = picture->getBounds();
    state.picture = picture;
    state.textureCount = std::min<u8>(picture->getTextureCount(), 2);
    for (u8 i = 0; i < state.textureCount; ++i) {
        auto* texture = picture->getTexture(i);
        state.textures[i] = texture != nullptr ? texture->getTexInfo() : nullptr;
    }
    state.left = bounds.i.x;
    state.top = bounds.i.y;
    state.width = bounds.getWidth();
    state.height = bounds.getHeight();
    state.active = true;
}

void restore_round_picture_state(RoundPictureState& state) {
    if (state.active && state.picture != nullptr) {
        for (u8 i = 0; i < state.textureCount; ++i) {
            if (state.textures[i] != nullptr) {
                state.picture->changeTexture(state.textures[i], i);
            }
        }
        if (state.picture->getTexture(0) != nullptr) {
            state.picture->setTexCoord(state.picture->getTexture(0), BIND15, MIRROR0, false);
        }
        state.picture->resize(state.width, state.height);
        state.picture->move(state.left, state.top);
    }
    state = {};
}

void restore_round_button_pictures() {
    for (auto& state : s_roundPictureStates) {
        restore_round_picture_state(state);
    }
}

bool apply_round_hud_picture(J2DPicture* picture, ResTIMG const* texture) {
    if (picture == nullptr || texture == nullptr) {
        return false;
    }

    RoundPictureState* state = round_picture_state(picture);
    if (state == nullptr) {
        return false;
    }
    capture_round_picture_state(*state, picture);

    const u8 textureCount = picture->getTextureCount();
    for (u8 i = 0; i < textureCount; ++i) {
        picture->changeTexture(texture, i);
    }
    if (picture->getTexture(0) != nullptr) {
        picture->setTexCoord(picture->getTexture(0), BIND15, MIRROR0, false);
    }
    make_hud_button_picture_square(picture);
    return true;
}

void apply_round_hud_button_base(CPaneMgr* button, ResTIMG const* texture) {
    if (button == nullptr) {
        return;
    }

    apply_round_hud_picture(first_picture_pane(button->getPanePtr()), texture);
}

void apply_round_hud_button_layers(J2DPane* pane, ResTIMG const* texture) {
    if (pane == nullptr || texture == nullptr) {
        return;
    }

    apply_round_hud_picture(as_picture(pane), texture);

    for (J2DPane* child = pane->getFirstChildPane(); child != nullptr;
         child = child->getNextChildPane())
    {
        apply_round_hud_button_layers(child, texture);
    }
}

void apply_round_xy_buttons(dMeter2Draw_c* meter) {
    if (meter == nullptr) {
        restore_round_button_pictures();
        s_roundHudMeter = nullptr;
        return;
    }

    if (s_roundHudMeter != meter) {
        s_roundPictureStates = {};
        s_roundHudMeter = meter;
    }

    if (!round_xy_buttons_enabled()) {
        restore_round_button_pictures();
        return;
    }

    ResTIMG const* texture = round_hud_button_texture(meter->mpButtonA);
    if (texture == nullptr) {
        return;
    }

    apply_round_hud_button_base(meter->mpButtonXY[0], texture);
    apply_round_hud_button_base(meter->mpButtonXY[1], texture);
    if (meter->mpLightXY[0] != nullptr) {
        apply_round_hud_button_layers(meter->mpLightXY[0]->getPanePtr(), texture);
    }
    if (meter->mpLightXY[1] != nullptr) {
        apply_round_hud_button_layers(meter->mpLightXY[1]->getPanePtr(), texture);
    }
}

bool nearly_equal(const f32 lhs, const f32 rhs) {
    return std::fabs(lhs - rhs) < 0.01f;
}

struct HudLayoutOffset {
    f32 x = 0.0f;
    f32 y = 0.0f;
};

HudLayoutOffset hud_item_anchor_position(const int anchor, const f32 defaultX,
    const f32 defaultY) {
    const f32 distance = std::fabs(defaultX) > 14.0f ? std::fabs(defaultX) : 14.0f;

    switch (anchor) {
    case kHudItemAnchorLeft:
        return {-distance, defaultY};
    case kHudItemAnchorTop:
        return {0.0f, defaultY - distance};
    case kHudItemAnchorBottom:
        return {0.0f, defaultY + distance};
    case kHudItemAnchorRight:
    default:
        return {distance, defaultY};
    }
}

HudLayoutOffset hud_item_anchor_delta(const DuskModHudButtonLayout& layout,
    const f32 defaultX, const f32 defaultY) {
    const HudLayoutOffset selected =
        hud_item_anchor_position(layout.item_anchor, defaultX, defaultY);
    const HudLayoutOffset fallback =
        hud_item_anchor_position(layout.default_item_anchor, defaultX, defaultY);
    return {
        .x = selected.x - fallback.x,
        .y = selected.y - fallback.y,
    };
}

f32 hud_item_scale(const DuskModHudTransform& transform,
    const DuskModHudButtonLayout& layout) {
    const f32 itemScale = layout.item_scale > 0.0f ? layout.item_scale : 1.0f;
    return transform.scale * itemScale;
}

f32 hud_text_scale(const DuskModHudTransform& transform,
    const DuskModHudButtonLayout& layout) {
    const f32 textScale = layout.text_scale > 0.0f ? layout.text_scale : 1.0f;
    return transform.scale * textScale;
}

f32 hud_ammo_scale(const DuskModHudTransform& transform,
    const DuskModHudButtonLayout& layout) {
    const f32 itemScale = layout.item_scale > 0.0f ? layout.item_scale : 1.0f;
    const f32 ammoScale = layout.ammo_scale > 0.0f ? layout.ammo_scale : 1.0f;
    return transform.scale * itemScale * ammoScale;
}

DuskModHudTransform hud_layout_xy_transform(const int slot) {
    return slot == dMeter2Draw_c::SELECT_Y_e ? hud_layout_y_transform() :
                                               hud_layout_x_transform();
}

DuskModHudButtonLayout hud_layout_xy_button_layout(const int slot) {
    return slot == dMeter2Draw_c::SELECT_Y_e ? hud_layout_y_button_layout() :
                                               hud_layout_x_button_layout();
}

int hud_button_b_item_variant(dMeter2Draw_c* meter) {
    if (meter == nullptr) {
        return 0;
    }

    switch (meter->mButtonBItem) {
    case dItemNo_LURE_ROD_e:
        return 2;
    case dItemNo_WOOD_STICK_e:
    case dItemNo_SWORD_e:
    case dItemNo_MASTER_SWORD_e:
    case dItemNo_LIGHT_SWORD_e:
        return 1;
    default:
        return 0;
    }
}

HudPaneTransformState& hud_pane_state(const HudPaneSlot slot) {
    return s_wiiUHudPaneTransforms[static_cast<std::size_t>(slot)];
}

J2DPane* pane_ptr(CPaneMgrAlpha* pane) {
    return pane != nullptr ? pane->getPanePtr() : nullptr;
}

J2DTextBox* text_box_ptr(CPaneMgr* pane) {
    J2DPane* j2dPane = pane_ptr(pane);
    if (j2dPane == nullptr || j2dPane->getTypeID() != 19) {
        return nullptr;
    }
    return static_cast<J2DTextBox*>(j2dPane);
}

void set_text_box_h_binding(J2DTextBox* textBox, const J2DTextBoxHBinding binding) {
    if (textBox == nullptr) {
        return;
    }
    textBox->mFlags = (textBox->mFlags & ~0x0C) | ((static_cast<u8>(binding) & 0x03) << 2);
}

J2DTextBoxHBinding hud_text_anchor_binding(const int textAnchor) {
    return textAnchor == kHudTextAnchorRight ? HBIND_LEFT : HBIND_RIGHT;
}

void apply_hud_text_box_binding(const HudPaneSlot slot, const std::size_t index,
    CPaneMgr* pane, const bool enabled, const int textAnchor) {
    if (index >= s_hudTextBoxFlags[static_cast<std::size_t>(slot)].size()) {
        return;
    }

    HudTextBoxFlagState& state =
        s_hudTextBoxFlags[static_cast<std::size_t>(slot)][index];
    J2DTextBox* textBox = text_box_ptr(pane);
    if (textBox == nullptr) {
        state = {};
        return;
    }

    if (!enabled) {
        if (state.active && state.textBox == textBox) {
            textBox->mFlags = state.originalFlags;
        }
        state = {};
        return;
    }

    if (!state.active || state.textBox != textBox) {
        state = {
            .textBox = textBox,
            .originalFlags = textBox->mFlags,
            .active = true,
        };
    }

    set_text_box_h_binding(textBox, hud_text_anchor_binding(textAnchor));
}

void apply_hud_text_box_group_binding(const HudPaneSlot slot, CPaneMgr* const* panes,
    const std::size_t count, const bool enabled, const int textAnchor) {
    for (std::size_t i = 0; i < count; ++i) {
        apply_hud_text_box_binding(slot, i, panes[i], enabled, textAnchor);
    }
}

void apply_hud_xy_text_box_group_binding(const HudPaneSlot slot, CPaneMgr* panes[5][3],
    const std::size_t xySlot, const bool enabled, const int textAnchor) {
    for (std::size_t i = 0; i < 5; ++i) {
        apply_hud_text_box_binding(slot, i, panes[i][xySlot], enabled, textAnchor);
    }
}

void apply_hud_pane_transform(HudPaneTransformState& state, J2DPane* pane, const bool enabled,
    const f32 offsetX, const f32 offsetY, const f32 scale) {
    if (pane == nullptr || scale <= 0.0f) {
        state = {};
        return;
    }

    if (state.active && state.pane == pane &&
        nearly_equal(pane->getTranslateX(), state.appliedX) &&
        nearly_equal(pane->getTranslateY(), state.appliedY) &&
        nearly_equal(pane->getScaleX(), state.appliedScaleX) &&
        nearly_equal(pane->getScaleY(), state.appliedScaleY))
    {
        const f32 appliedScale = state.scale > 0.0f ? state.scale : 1.0f;
        pane->translate(pane->getTranslateX() - state.offsetX,
            pane->getTranslateY() - state.offsetY);
        pane->scale(pane->getScaleX() / appliedScale, pane->getScaleY() / appliedScale);
    }

    if (!enabled) {
        state = {};
        return;
    }

    const u8 originalTextFlags = state.originalTextFlags;
    const bool hasOriginalTextFlags = state.hasOriginalTextFlags;
    const f32 baseX = pane->getTranslateX();
    const f32 baseY = pane->getTranslateY();
    const f32 baseScaleX = pane->getScaleX();
    const f32 baseScaleY = pane->getScaleY();
    pane->translate(baseX + offsetX, baseY + offsetY);
    pane->scale(baseScaleX * scale, baseScaleY * scale);

    state = {
        .pane = pane,
        .offsetX = offsetX,
        .offsetY = offsetY,
        .scale = scale,
        .appliedX = pane->getTranslateX(),
        .appliedY = pane->getTranslateY(),
        .appliedScaleX = pane->getScaleX(),
        .appliedScaleY = pane->getScaleY(),
        .originalTextFlags = originalTextFlags,
        .hasOriginalTextFlags = hasOriginalTextFlags,
        .active = true,
    };
}

void apply_hud_pane_transform(const HudPaneSlot slot, CPaneMgr* pane, const bool enabled,
    const f32 offsetX, const f32 offsetY, const f32 scale) {
    apply_hud_pane_transform(hud_pane_state(slot), pane_ptr(pane), enabled, offsetX, offsetY,
        scale);
}

void apply_hud_pane_transform(const HudPaneSlot slot, CPaneMgrAlpha* pane, const bool enabled,
    const f32 offsetX, const f32 offsetY, const f32 scale) {
    apply_hud_pane_transform(hud_pane_state(slot), pane_ptr(pane), enabled, offsetX, offsetY,
        scale);
}

void apply_hud_text_pane_transform(const HudPaneSlot slot, CPaneMgr* pane, const bool enabled,
    const f32 offsetX, const f32 offsetY, const f32 scale, const int) {
    HudPaneTransformState& state = hud_pane_state(slot);
    apply_hud_pane_transform(state, pane_ptr(pane), enabled, offsetX, offsetY, scale);
}

void apply_wii_u_hud_layout(dMeter2Draw_c* meter) {
    if (meter == nullptr) {
        return;
    }

    const bool enabled = hardcoded_hud_layout_enabled();

    const DuskModHudTransform aTransform = hud_layout_a_transform();
    const DuskModHudButtonLayout aLayout = hud_layout_a_button_layout();
    apply_hud_pane_transform(HudPaneSlot::ButtonA, meter->mpButtonA, enabled,
        aTransform.offset_x, aTransform.offset_y, aTransform.scale);
    apply_hud_text_pane_transform(HudPaneSlot::TextA, meter->mpTextA, enabled,
        aTransform.offset_x + aLayout.text_offset_x,
        aTransform.offset_y + aLayout.text_offset_y, hud_text_scale(aTransform, aLayout),
        aLayout.text_anchor);
    apply_hud_text_box_group_binding(
        HudPaneSlot::TextA, meter->mpAText, 5, enabled, aLayout.text_anchor);

    const DuskModHudTransform bTransform = hud_layout_b_transform();
    const DuskModHudButtonLayout bLayout = hud_layout_b_button_layout();
    const int bVariant = hud_button_b_item_variant(meter);
    const HudLayoutOffset bItemAnchor =
        hud_item_anchor_delta(bLayout, g_drawHIO.mButtonBItemPosX[bVariant],
            g_drawHIO.mButtonBItemPosY[bVariant]);
    const f32 bItemOffsetX = bTransform.offset_x + bLayout.item_offset_x + bItemAnchor.x;
    const f32 bItemOffsetY = bTransform.offset_y + bLayout.item_offset_y + bItemAnchor.y;
    apply_hud_pane_transform(HudPaneSlot::ButtonB, meter->mpButtonB, enabled,
        bTransform.offset_x, bTransform.offset_y, bTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::ItemB, meter->mpItemB, enabled, bItemOffsetX,
        bItemOffsetY, hud_item_scale(bTransform, bLayout));
    apply_hud_pane_transform(HudPaneSlot::LightB, meter->mpLightB, enabled, bItemOffsetX,
        bItemOffsetY, hud_item_scale(bTransform, bLayout));
    apply_hud_text_pane_transform(HudPaneSlot::TextB, meter->mpTextB, enabled,
        bTransform.offset_x + bLayout.text_offset_x,
        bTransform.offset_y + bLayout.text_offset_y, hud_text_scale(bTransform, bLayout),
        bLayout.text_anchor);
    apply_hud_text_box_group_binding(
        HudPaneSlot::TextB, meter->mpBText, 5, enabled, bLayout.text_anchor);

    const DuskModHudTransform xTransform = hud_layout_x_transform();
    const DuskModHudButtonLayout xLayout = hud_layout_x_button_layout();
    const HudLayoutOffset xItemAnchor =
        hud_item_anchor_delta(xLayout, g_drawHIO.mButtonXItemBasePosX[0],
            g_drawHIO.mButtonXItemBasePosY[0]);
    const f32 xItemOffsetX = xTransform.offset_x + xLayout.item_offset_x + xItemAnchor.x;
    const f32 xItemOffsetY = xTransform.offset_y + xLayout.item_offset_y + xItemAnchor.y;
    apply_hud_pane_transform(HudPaneSlot::ButtonX, meter->mpButtonXY[0], enabled,
        xTransform.offset_x, xTransform.offset_y, xTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::ItemX, meter->mpItemXY[0], enabled, xItemOffsetX,
        xItemOffsetY, hud_item_scale(xTransform, xLayout));
    apply_hud_pane_transform(HudPaneSlot::LightX, meter->mpLightXY[0], enabled, xItemOffsetX,
        xItemOffsetY, hud_item_scale(xTransform, xLayout));
    apply_hud_text_pane_transform(HudPaneSlot::TextX, meter->mpTextXY[0], enabled,
        xTransform.offset_x + xLayout.text_offset_x,
        xTransform.offset_y + xLayout.text_offset_y, hud_text_scale(xTransform, xLayout),
        xLayout.text_anchor);
    apply_hud_xy_text_box_group_binding(
        HudPaneSlot::TextX, meter->mpXYText, 0, enabled, xLayout.text_anchor);

    const DuskModHudTransform yTransform = hud_layout_y_transform();
    const DuskModHudButtonLayout yLayout = hud_layout_y_button_layout();
    const HudLayoutOffset yItemAnchor =
        hud_item_anchor_delta(yLayout, g_drawHIO.mButtonYItemBasePosX[0],
            g_drawHIO.mButtonYItemBasePosY[0]);
    const f32 yItemOffsetX = yTransform.offset_x + yLayout.item_offset_x + yItemAnchor.x;
    const f32 yItemOffsetY = yTransform.offset_y + yLayout.item_offset_y + yItemAnchor.y;
    apply_hud_pane_transform(HudPaneSlot::ButtonY, meter->mpButtonXY[1], enabled,
        yTransform.offset_x, yTransform.offset_y, yTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::ItemY, meter->mpItemXY[1], enabled, yItemOffsetX,
        yItemOffsetY, hud_item_scale(yTransform, yLayout));
    apply_hud_pane_transform(HudPaneSlot::LightY, meter->mpLightXY[1], enabled, yItemOffsetX,
        yItemOffsetY, hud_item_scale(yTransform, yLayout));
    apply_hud_text_pane_transform(HudPaneSlot::TextY, meter->mpTextXY[1], enabled,
        yTransform.offset_x + yLayout.text_offset_x,
        yTransform.offset_y + yLayout.text_offset_y, hud_text_scale(yTransform, yLayout),
        yLayout.text_anchor);
    apply_hud_xy_text_box_group_binding(
        HudPaneSlot::TextY, meter->mpXYText, 1, enabled, yLayout.text_anchor);

    const DuskModHudTransform zTransform = hud_layout_z_transform();
    const DuskModHudButtonLayout zLayout = hud_layout_z_button_layout();
    apply_hud_pane_transform(HudPaneSlot::ButtonZ, meter->mpButtonXY[2], enabled,
        zTransform.offset_x, zTransform.offset_y, zTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::TextZ, meter->mpTextXY[2], enabled,
        zTransform.offset_x + zLayout.text_offset_x,
        zTransform.offset_y + zLayout.text_offset_y, hud_text_scale(zTransform, zLayout));

    const DuskModHudTransform backingTransform = hud_layout_backing_transform();
    apply_hud_pane_transform(HudPaneSlot::Backing, meter->mpUzu, enabled,
        backingTransform.offset_x, backingTransform.offset_y, backingTransform.scale);
    const DuskModHudTransform dpadTransform = hud_layout_dpad_transform();
    apply_hud_pane_transform(HudPaneSlot::DPad, meter->mpButtonCrossParent, enabled,
        dpadTransform.offset_x, dpadTransform.offset_y, dpadTransform.scale);
    const DuskModHudTransform heartsTransform = hud_layout_hearts_transform();
    apply_hud_pane_transform(HudPaneSlot::Hearts, meter->mpLifeParent, enabled,
        heartsTransform.offset_x, heartsTransform.offset_y, heartsTransform.scale);
    const DuskModHudTransform rupeesTransform = hud_layout_rupees_transform();
    apply_hud_pane_transform(HudPaneSlot::Rupee0, meter->mpRupeeParent[0], enabled,
        rupeesTransform.offset_x, rupeesTransform.offset_y, rupeesTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::Rupee1, meter->mpRupeeParent[1], enabled,
        rupeesTransform.offset_x, rupeesTransform.offset_y, rupeesTransform.scale);
    apply_hud_pane_transform(HudPaneSlot::Rupee2, meter->mpRupeeParent[2], enabled,
        rupeesTransform.offset_x, rupeesTransform.offset_y, rupeesTransform.scale);
    const DuskModHudTransform keysTransform = hud_layout_keys_transform();
    apply_hud_pane_transform(HudPaneSlot::Keys, meter->mpKeyParent, enabled,
        keysTransform.offset_x, keysTransform.offset_y, keysTransform.scale);
}

void apply_xy_ammo_layout(dMeter2Draw_c* meter) {
    s_xyAmmoOriginalValid.fill(false);
    if (!hardcoded_hud_layout_enabled() || meter == nullptr) {
        return;
    }

    for (int slot = dMeter2Draw_c::SELECT_X_e; slot <= dMeter2Draw_c::SELECT_Y_e; ++slot) {
        if (meter->mpItemXY[slot] == nullptr) {
            continue;
        }

        s_xyAmmoOriginalParams[slot] = meter->mItemParams[slot];
        s_xyAmmoOriginalValid[slot] = true;

        const DuskModHudTransform transform = hud_layout_xy_transform(slot);
        const DuskModHudButtonLayout layout = hud_layout_xy_button_layout(slot);
        meter->mItemParams[slot].num_pos_x += layout.ammo_offset_x;
        meter->mItemParams[slot].num_pos_y += layout.ammo_offset_y;
        meter->mItemParams[slot].num_scale *= hud_ammo_scale(transform, layout);
    }
}

void restore_xy_ammo_layout(dMeter2Draw_c* meter) {
    if (meter != nullptr) {
        for (int slot = dMeter2Draw_c::SELECT_X_e; slot <= dMeter2Draw_c::SELECT_Y_e; ++slot) {
            if (s_xyAmmoOriginalValid[slot]) {
                meter->mItemParams[slot] = s_xyAmmoOriginalParams[slot];
            }
        }
    }
    s_xyAmmoOriginalValid.fill(false);
}

void apply_hud_backing_visibility(dMeter2Draw_c* meter) {
    if (meter == nullptr || meter->mpUzu == nullptr) {
        return;
    }

    if (!hud_button_backing_visible()) {
        meter->mpUzu->setAlpha(0);
        meter->mpUzu->setAlphaRate(0.0f);
        return;
    }

    meter->mpUzu->setAlpha(meter->mpUzu->getInitAlpha());
    meter->mpUzu->setAlphaRate(1.0f);
}

void apply_wii_u_minimap_layout(dMeterMap_c* map) {
    if (!hardcoded_hud_layout_enabled() || map == nullptr) {
        return;
    }

    s_wiiUMinimapTransform = {
        .map = map,
        .drawPosX = map->mDrawPosX,
        .drawPosY = map->mDrawPosY,
        .sizeW = map->mSizeW,
        .sizeH = map->mSizeH,
        .active = true,
    };

    const DuskModHudTransform transform = hud_layout_minimap_transform();
    if (transform.slide_direction == kHudSlideRightToLeft) {
        const f32 insidePosX =
            map->mDrawPosX - (static_cast<f32>(map->mSlidePositionOffset) * 2.0f);
        map->mDrawPosX = insidePosX - (map->mDrawPosX - insidePosX);
    }
    map->mDrawPosX += transform.offset_x;
    map->mDrawPosY += transform.offset_y;
    map->mSizeW *= transform.scale;
    map->mSizeH *= transform.scale;
}

void restore_wii_u_minimap_layout(dMeterMap_c* map) {
    if (!s_wiiUMinimapTransform.active || s_wiiUMinimapTransform.map != map) {
        s_wiiUMinimapTransform = {};
        return;
    }

    map->mDrawPosX = s_wiiUMinimapTransform.drawPosX;
    map->mDrawPosY = s_wiiUMinimapTransform.drawPosY;
    map->mSizeW = s_wiiUMinimapTransform.sizeW;
    map->mSizeH = s_wiiUMinimapTransform.sizeH;
    s_wiiUMinimapTransform = {};
}

J2DPane* item_wheel_z_anchor(J2DScreen* screen) {
    return screen != nullptr ? screen->search(MULTI_CHAR('r_btn_n')) : nullptr;
}

void apply_item_wheel_z_offset(Vec& pos) {
    pos.x += 5.0f;
    pos.y -= 5.0f;
}

void apply_item_wheel_dpad_down_offset(Vec& pos) {
    pos.x += 40.0f;
    pos.y -= 5.0f;
}

void clear_ring_z_prompt_refs() {
    s_ringZPrompt.button = nullptr;
    s_ringZPrompt.screen = nullptr;
    s_ringZPrompt.ring = nullptr;
}

void clear_ring_dpad_down_prompt_refs() {
    s_ringDpadDownPrompt.picture = nullptr;
    s_ringDpadDownPrompt.overlay = nullptr;
    s_ringDpadDownPrompt.ring = nullptr;
}

void destroy_ring_z_prompt(dMenu_Ring_c* ring) {
    if (s_ringZPrompt.ring != ring) {
        return;
    }

    // The ring menu owns this heap lifetime; keep only per-menu references here.
    clear_ring_z_prompt_refs();
}

void destroy_ring_dpad_down_prompt(dMenu_Ring_c* ring) {
    if (s_ringDpadDownPrompt.ring != ring) {
        return;
    }

    clear_ring_dpad_down_prompt_refs();
}

void create_ring_z_prompt(dMenu_Ring_c* ring) {
    clear_ring_z_prompt_refs();
    if (!z_item_slot_enabled() || ring == nullptr || ring->mPlayerIsWolf || ring->mpScreen == nullptr) {
        return;
    }

    auto* archive = dComIfGp_getMain2DArchive();
    if (archive == nullptr) {
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
    if (!screen->setPriority("zelda_game_image.blo", 0x20000, archive)) {
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

bool ring_extra_item_prompts_visible(dMenu_Ring_c* ring) {
    if (ring == nullptr || ring->mPlayerIsWolf ||
        dMeter2Info_getItemExplainWindowStatus() != 0)
    {
        return false;
    }

    return ring->mStatus == dMenu_Ring_c::STATUS_WAIT ||
           ring->mStatus == dMenu_Ring_c::STATUS_MOVE;
}

void create_ring_dpad_down_prompt(dMenu_Ring_c* ring) {
    clear_ring_dpad_down_prompt_refs();
    if (!dpad_down_item_slot_enabled() || ring == nullptr || ring->mPlayerIsWolf) {
        return;
    }

    ResTIMG const* texture = z_prompt_dpad_texture(nullptr);
    if (texture == nullptr) {
        s_ringDpadDownPrompt = {.ring = ring, .picture = nullptr, .overlay = nullptr};
        return;
    }

    J2DPicture* picture = JKR_NEW J2DPicture(texture);
    J2DPicture* overlay = JKR_NEW J2DPicture(texture);
    if (picture == nullptr || overlay == nullptr) {
        return;
    }

    picture->setBasePosition(J2DBasePosition_4);
    overlay->setBasePosition(J2DBasePosition_4);
    overlay->setBlackWhite(JUtility::TColor(0x00000000), JUtility::TColor(0xFFFFFFFF));
    overlay->setCornerColor(JUtility::TColor(0xFF0000C0));
    overlay->show();
    s_ringDpadDownPrompt = {.ring = ring, .picture = picture, .overlay = overlay};
}

void draw_ring_z_prompt(dMenu_Ring_c* ring) {
    if (!z_item_slot_enabled() || s_ringZPrompt.ring != ring ||
        s_ringZPrompt.screen == nullptr || s_ringZPrompt.button == nullptr ||
        ring == nullptr || ring->mpScreen == nullptr || !ring_extra_item_prompts_visible(ring))
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

void draw_ring_dpad_down_prompt(dMenu_Ring_c* ring) {
    if (!dpad_down_item_slot_enabled() || s_ringDpadDownPrompt.ring != ring ||
        ring == nullptr || ring->mpScreen == nullptr || !ring_extra_item_prompts_visible(ring))
    {
        return;
    }

    if (s_ringDpadDownPrompt.picture == nullptr) {
        ResTIMG const* texture = z_prompt_dpad_texture(nullptr);
        if (texture == nullptr) {
            return;
        }
        s_ringDpadDownPrompt.picture = JKR_NEW J2DPicture(texture);
        s_ringDpadDownPrompt.overlay = JKR_NEW J2DPicture(texture);
        if (s_ringDpadDownPrompt.picture == nullptr || s_ringDpadDownPrompt.overlay == nullptr) {
            return;
        }
        s_ringDpadDownPrompt.picture->setBasePosition(J2DBasePosition_4);
        s_ringDpadDownPrompt.overlay->setBasePosition(J2DBasePosition_4);
        s_ringDpadDownPrompt.overlay->setBlackWhite(
            JUtility::TColor(0x00000000), JUtility::TColor(0xFFFFFFFF));
        s_ringDpadDownPrompt.overlay->setCornerColor(JUtility::TColor(0xFF0000C0));
        s_ringDpadDownPrompt.overlay->show();
    }

    J2DPane* anchor = item_wheel_z_anchor(ring->mpScreen);
    if (anchor == nullptr) {
        return;
    }

    CPaneMgr paneMgr;
    Vec pos = paneMgr.getGlobalVtxCenter(anchor, true, 0);
    pos.x += ring->mCenterPosX;
    pos.y += ring->mCenterPosY;
    apply_item_wheel_dpad_down_offset(pos);

    const u8 alpha = clamp_hud_alpha(ring->mAlphaRate * 255.0f);
    s_ringDpadDownPrompt.picture->setAlpha(alpha);
    s_ringDpadDownPrompt.picture->draw(pos.x - 14.0f, pos.y - 14.0f, 28.0f, 28.0f,
        false, false, false);

    if (s_ringDpadDownPrompt.overlay != nullptr) {
        const f32 overlayWidth = 28.0f / 3.0f;
        const f32 overlayHeight = 28.0f / 3.0f;
        J2DFillBox(pos.x - overlayWidth * 0.5f, pos.y + 14.0f - overlayHeight,
            overlayWidth, overlayHeight, JUtility::TColor(255, 0, 0,
                clamp_hud_alpha(ring->mAlphaRate * 128.0f)));
    }
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
    auto* picture = static_cast<J2DPicture*>(meter->mpItemR->getPanePtr());
    if (picture == nullptr) {
        return;
    }

    const ResTIMG* activeTexture = nullptr;
    if (auto* texture = picture->getTexture(0)) {
        activeTexture = texture->getTexInfo();
    }
    if (s_zHudLastItem == textureItem && s_zHudLastPicture == picture &&
        activeTexture == z_hud_item_tex(s_zHudItemTexPage, 0))
    {
        return;
    }

    s_zHudItemTexPage ^= 1;
    ResTIMG* primary = z_hud_item_tex(s_zHudItemTexPage, 0);
    ResTIMG* secondary = z_hud_item_tex(s_zHudItemTexPage, 1);
    const s32 textureCount =
        dMeter2Info_readItemTexture(textureItem, primary,
            picture, secondary, meter->mpItemXYPane[2], nullptr, nullptr, nullptr, nullptr, -1);
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
    s_zHudLastPicture = picture;
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

u8 stored_select_mix_no_arrow_slot(const int slotIndex) {
    const u8 selectSlot = stored_select_slot(slotIndex);
    const u8 mixSlot = stored_mix_slot(slotIndex);
    if (selectSlot >= SLOT_15 && selectSlot < SLOT_18) {
        return selectSlot;
    }
    if (mixSlot != dItemNo_NONE_e && mixSlot >= SLOT_15 && mixSlot < SLOT_18) {
        return mixSlot;
    }
    return dItemNo_NONE_e;
}

bool z_item_ammo_values(const u8 itemNo, u8& itemNum, u8& itemMax) {
    return z_item_ammo_values_for_slot(kZItemSlot, itemNo, itemNum, itemMax);
}

bool z_item_ammo_values_for_slot(const int slotIndex, const u8 itemNo, u8& itemNum, u8& itemMax) {
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
        const u8 bombSlot = stored_select_mix_no_arrow_slot(slotIndex);
        if (bombSlot == dItemNo_NONE_e) {
            return false;
        }
        itemNum = static_cast<u8>(std::max<s16>(0, dComIfGs_getBombNum(bombSlot - SLOT_15)));
        itemMax = static_cast<u8>(std::max<int>(0, dComIfGs_getBombMax(itemNo)));
        itemNum = std::min(itemNum, static_cast<u8>(dComIfGs_getArrowNum()));
        itemMax = std::max(itemMax, static_cast<u8>(dComIfGs_getArrowMax()));
        return true;
    }
    case dItemNo_PACHINKO_e:
        itemNum = static_cast<u8>(dComIfGs_getPachinkoNum());
        itemMax = static_cast<u8>(dComIfGs_getPachinkoMax());
        return true;
    case dItemNo_BEE_CHILD_e: {
        const u8 bottleSlot = stored_select_slot(slotIndex);
        if (bottleSlot < SLOT_11 || bottleSlot >= SLOT_15) {
            return false;
        }
        itemNum = static_cast<u8>(dComIfGs_getBottleNum(bottleSlot - SLOT_11));
        itemMax = static_cast<u8>(dComIfGs_getBottleMax());
        return true;
    }
    default:
        if (itemNo == dItemNo_NORMAL_BOMB_e || itemNo == dItemNo_WATER_BOMB_e ||
            itemNo == dItemNo_POKE_BOMB_e)
        {
            const u8 bombSlot = stored_select_mix_no_arrow_slot(slotIndex);
            if (bombSlot == dItemNo_NONE_e) {
                return false;
            }
            itemNum = static_cast<u8>(std::max<s16>(0, dComIfGs_getBombNum(bombSlot - SLOT_15)));
            itemMax = static_cast<u8>(std::max<int>(0, dComIfGs_getBombMax(itemNo)));
            return true;
        }
        itemNum = 0;
        itemMax = 0;
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
    f32 numPosX = meter->mItemParams[dMeter2Draw_c::SELECT_Z_e].num_pos_x;
    f32 numPosY = meter->mItemParams[dMeter2Draw_c::SELECT_Z_e].num_pos_y;
    f32 numScale = meter->mItemParams[dMeter2Draw_c::SELECT_Z_e].num_scale;
    if (hardcoded_hud_layout_enabled()) {
        numPosX = g_drawHIO.field_0x1f8;
        numPosY = g_drawHIO.field_0x208;
        numScale = g_drawHIO.field_0x218;
    }
    const f32 digitSize = numScale * 16.0f * hud_ammo_scale(hudTransform, buttonLayout);

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
        s_zItemNumTex[i]->draw(numPosX + buttonLayout.ammo_offset_x + centerX + digitSize * i,
            numPosY + buttonLayout.ammo_offset_y + centerY + meter->mpItemR->getSizeY(),
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

ResTIMG* dpad_down_hud_item_tex(const u8 page, const u8 layer) {
    return reinterpret_cast<ResTIMG*>(s_dpadDownHudItemTexBuf[page][layer]);
}

bool ensure_dpad_down_hud_item_panes() {
    for (int i = 0; i < 2; ++i) {
        if (s_dpadDownHudItemPane[i] != nullptr) {
            continue;
        }
        s_dpadDownHudItemPane[i] = JKR_NEW J2DPicture(dpad_down_hud_item_tex(0, i));
        if (s_dpadDownHudItemPane[i] == nullptr) {
            return false;
        }
        s_dpadDownHudItemPane[i]->setBasePosition(J2DBasePosition_4);
    }
    return true;
}

bool change_dpad_down_hud_item_texture(const u8 itemNo, u8& textureCount) {
    if (!ensure_dpad_down_hud_item_panes()) {
        return false;
    }

    const u8 textureItem = hud_texture_item(itemNo);
    if (s_dpadDownHudLastItem == textureItem) {
        textureCount = s_dpadDownHudItemPane[1]->isVisible() ? 2 : 1;
        return true;
    }

    s_dpadDownHudItemTexPage ^= 1;
    ResTIMG* primary = dpad_down_hud_item_tex(s_dpadDownHudItemTexPage, 0);
    ResTIMG* secondary = dpad_down_hud_item_tex(s_dpadDownHudItemTexPage, 1);
    const s32 readCount = dMeter2Info_readItemTexture(textureItem, primary,
        s_dpadDownHudItemPane[0], secondary, s_dpadDownHudItemPane[1],
        nullptr, nullptr, nullptr, nullptr, -1);
    if (readCount <= 0) {
        return false;
    }

    s_dpadDownHudItemPane[0]->changeTexture(primary, 0);
    if (readCount > 1) {
        s_dpadDownHudItemPane[1]->changeTexture(secondary, 0);
        s_dpadDownHudItemPane[1]->show();
    } else {
        s_dpadDownHudItemPane[1]->hide();
    }
    dMeter2Info_setItemColor(textureItem, s_dpadDownHudItemPane[0],
        s_dpadDownHudItemPane[1], nullptr, nullptr);
    s_dpadDownHudLastItem = textureItem;
    textureCount = static_cast<u8>(readCount);
    return true;
}

bool dpad_down_hud_position(dMeter2Draw_c* meter, f32& centerX, f32& centerY, f32& iconSize,
    u8& alpha) {
    if (meter == nullptr || meter->mpScreen == nullptr || meter->mpButtonCrossParent == nullptr) {
        return false;
    }

    J2DPane* dpadPane = meter->mpScreen->search(MULTI_CHAR('juji_n'));
    if (dpadPane == nullptr || !dpadPane->isVisible()) {
        return false;
    }

    Vec vtx0 = dpadPane->getGlbVtx(0);
    Vec vtx3 = dpadPane->getGlbVtx(3);
    const f32 width = std::fabs(vtx3.x - vtx0.x);
    const f32 height = std::fabs(vtx3.y - vtx0.y);
    if (width <= 0.0f || height <= 0.0f) {
        return false;
    }

    iconSize = std::max(20.0f, std::min(width, height) * 0.22f);
    centerX = (vtx0.x + vtx3.x) * 0.5f - iconSize * 0.5f;
    centerY = (vtx0.y + vtx3.y) * 0.5f + height * 0.24f + iconSize * 0.5f;
    alpha = dpadPane->getAlpha();
    return alpha != 0;
}

bool ensure_dpad_down_item_num_textures() {
    if (s_dpadDownItemNumTex[0] != nullptr && s_dpadDownItemNumTex[1] != nullptr &&
        s_dpadDownItemNumTex[2] != nullptr)
    {
        return true;
    }

    ResTIMG* timg = static_cast<ResTIMG*>(dComIfGp_getMain2DArchive()->getResource(
        'TIMG', dMeter2Info_getNumberTextureName(0)));
    if (timg == nullptr) {
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        if (s_dpadDownItemNumTex[i] == nullptr) {
            s_dpadDownItemNumTex[i] = JKR_NEW J2DPicture(timg);
        }
        if (s_dpadDownItemNumTex[i] == nullptr) {
            return false;
        }
    }
    return true;
}

void set_dpad_down_item_num_textures(u8 itemNum, const u8 itemMax) {
    if (!ensure_dpad_down_item_num_textures()) {
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

    for (J2DPicture* digit : s_dpadDownItemNumTex) {
        digit->setBlackWhite(black, white);
    }

    auto set_digit = [](const int index, const int digit) {
        ResTIMG* timg = static_cast<ResTIMG*>(dComIfGp_getMain2DArchive()->getResource(
            'TIMG', dMeter2Info_getNumberTextureName(digit)));
        if (timg != nullptr) {
            s_dpadDownItemNumTex[index]->changeTexture(timg, 0);
        }
    };

    if (itemNum < 100) {
        set_digit(0, itemNum / 10);
        set_digit(1, itemNum % 10);
        s_dpadDownItemNumTex[2]->hide();
    } else {
        set_digit(0, itemNum / 100);
        itemNum %= 100;
        set_digit(1, itemNum / 10);
        set_digit(2, itemNum % 10);
        s_dpadDownItemNumTex[2]->show();
    }
}

void draw_dpad_down_ammo(const u8 itemNo, const f32 centerX, const f32 centerY,
    const f32 iconSize, const u8 alpha) {
    u8 itemNum = 0;
    u8 itemMax = 0;
    if (!z_item_ammo_values_for_slot(kDpadDownItemSlot, itemNo, itemNum, itemMax) ||
        itemMax == 0 || !ensure_dpad_down_item_num_textures())
    {
        return;
    }

    set_dpad_down_item_num_textures(itemNum, itemMax);
    const f32 digitSize = iconSize * 0.45f;
    const f32 startX = centerX + iconSize * 0.05f;
    const f32 startY = centerY + iconSize * 0.15f;
    for (int i = 0; i < 3; ++i) {
        if (i == 2 && itemNum < 100) {
            continue;
        }
        s_dpadDownItemNumTex[i]->setAlpha(alpha);
        s_dpadDownItemNumTex[i]->draw(startX + digitSize * i, startY,
            digitSize, digitSize, false, false, false);
    }
}

void draw_dpad_down_oil_meter(const u8 itemNo, const f32 centerX, const f32 centerY,
    const f32 iconSize, const f32 alphaRate) {
    if (!is_z_lantern_item(itemNo) || dComIfGs_getMaxOil() == 0) {
        return;
    }

    if (s_dpadDownOilMeter == nullptr) {
        s_dpadDownOilMeter = JKR_NEW dKantera_icon_c();
    }
    if (s_dpadDownOilMeter == nullptr) {
        return;
    }

    const f32 scale = std::max(0.35f, iconSize / 48.0f) * 0.6f;
    s_dpadDownOilMeter->setPos(centerX + iconSize * 0.15f, centerY + iconSize * 0.35f);
    s_dpadDownOilMeter->setScale(scale, scale);
    s_dpadDownOilMeter->setNowGauge(dComIfGs_getMaxOil(), dComIfGs_getOil());
    s_dpadDownOilMeter->setAlphaRate(alphaRate);
    s_dpadDownOilMeter->drawSelf();
}

void draw_dpad_down_hud_item(dMeter2Draw_c* meter) {
    if (!dpad_down_item_slot_enabled() || meter == nullptr || daPy_py_c::checkNowWolf())
    {
        return;
    }
    if (z_item_menu_or_pause_context() && dMeter2Info_getWindowStatus() != 2) {
        return;
    }

    const u8 itemNo = resolved_select_item(kDpadDownItemSlot);
    if (itemNo == dItemNo_NONE_e || itemNo == 0) {
        return;
    }

    f32 centerX = 0.0f;
    f32 centerY = 0.0f;
    f32 iconSize = 0.0f;
    u8 alpha = 0;
    if (!dpad_down_hud_position(meter, centerX, centerY, iconSize, alpha)) {
        return;
    }

    u8 textureCount = 0;
    if (!change_dpad_down_hud_item_texture(itemNo, textureCount)) {
        return;
    }

    const f32 alphaRate = static_cast<f32>(alpha) / 255.0f;
    for (u8 i = 0; i < std::min<u8>(textureCount, 2); ++i) {
        J2DPicture* picture = s_dpadDownHudItemPane[i];
        if (picture == nullptr || (i != 0 && !picture->isVisible())) {
            continue;
        }
        picture->setAlpha(alpha);
        picture->draw(centerX - iconSize * 0.5f, centerY - iconSize * 0.5f,
            iconSize, iconSize, false, false, false);
    }

    draw_dpad_down_ammo(itemNo, centerX, centerY, iconSize, alpha);
    draw_dpad_down_oil_meter(itemNo, centerX, centerY, iconSize, alphaRate);
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
    if (!z_item_slot_enabled() || meter == nullptr || meter->mpScreen == nullptr) {
        return;
    }

    const DuskModHudTransform midnaTransform = hud_layout_midna_transform();
    J2DPane* midnaPane = meter->mpScreen->search(MULTI_CHAR('midona_n'));
    if (midnaPane == nullptr) {
        return;
    }

    refresh_midna_touch_icon_texture(midnaPane);

    if (z_item_menu_or_pause_context() || !midna_unlocked()) {
        set_pane_tree_alpha_visible(midnaPane, false, 0);
        return;
    }

    J2DPane* dpadPane = meter->mpScreen->search(MULTI_CHAR('juji_n'));
    if (dpadPane == nullptr) {
        return;
    }

    if (midnaPane->getParentPane() != dpadPane) {
        dpadPane->appendChild(midnaPane);
        set_pane_influenced_alpha_tree(midnaPane, true);
    }

    const f32 scale = g_drawHIO.mMidnaIconScale * midnaTransform.scale;
    midnaPane->scale(scale, scale);
    midnaPane->move(-18.0f + midnaTransform.offset_x, midnaTransform.offset_y);

    if (boss_rush_save_active() && meter->isEmphasisZ() &&
        dComIfGp_getZStatus() == BUTTON_STATUS_CHECK && dComIfGp_isZSetFlag(BUTTON_STATUS_FLAG_EMPHASIS))
    {
        if (meter->field_0x738 == 0.0f) {
            meter->field_0x738 = 18.0f;
        }
        const u8 midnaInitAlpha =
            meter->mpButtonMidona != nullptr ? meter->mpButtonMidona->getInitAlpha() : 255;
        meter->mButtonZAlpha = static_cast<f32>(midnaInitAlpha) / 255.0f;
    }

    const u8 dpadAlpha = dpadPane->getAlpha();
    set_pane_tree_alpha_visible(midnaPane, dpadPane->isVisible() && dpadAlpha > 0, dpadAlpha);
}

bool z_item_menu_or_pause_context() {
    const u8 windowStatus = dMeter2Info_getWindowStatus();
    return windowStatus != 0 || dMeter2Info_getPauseStatus() != 0 || dComIfGp_isPauseFlag() ||
           dComIfGp_event_runCheck() || dMeter2Info_isShopTalkFlag() ||
           dMsgObject_isTalkNowCheck();
}

bool midna_unlocked() {
    if (boss_rush_save_active()) {
        return true;
    }

    const daAlink_c* link = daAlink_getAlinkActorClass();
    return (link != nullptr && link->checkWolf()) || dComIfGs_getTransformStatus() != 0 ||
           dComIfGs_isEventBit(0x0520) || dComIfGs_isEventBit(0x0510) ||
           dComIfGs_isEventBit(0x0501) || dComIfGs_isEventBit(0x0640) ||
           dComIfGs_isEventBit(0x0504) || dComIfGs_isEventBit(0x0502);
}

bool bow_mix_item(u8 itemNo) {
    switch (itemNo) {
    case dItemNo_NORMAL_BOMB_e:
    case dItemNo_WATER_BOMB_e:
    case dItemNo_POKE_BOMB_e:
    case dItemNo_HAWK_EYE_e:
        return true;
    default:
        return false;
    }
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
    const u8 slot = stored_select_slot(index);
    if (slot == dItemNo_NONE_e) {
        return dItemNo_NONE_e;
    }

    return combine_select_item(dComIfGs_getItem(slot, false), stored_mix_slot(index));
}

void sync_play_select_item(int index) {
    if (!z_item_slot_enabled() || index != kZItemSlot) {
        return;
    }

    g_dComIfG_gameInfo.play.setSelectItem(index, resolved_select_item(index));
}

u8 item_proc_slot(const u8 slot) {
    return is_dawnlight_item_slot(slot) ? kExtraItemProcSlot : slot;
}

bool dpad_down_item_input_active() {
    return dpad_down_item_slot_enabled() && (s_dpadDownHeld || s_dpadDownTrig) &&
           resolved_select_item(kDpadDownItemSlot) != dItemNo_NONE_e;
}

void remember_active_extra_proc_slot(const u8 slot) {
    if (is_active_dawnlight_item_slot(slot)) {
        s_activeExtraProcSlot = slot;
    }
}

void refresh_active_extra_proc_slot(daAlink_c* link) {
    if (link == nullptr || link->mSelectItemId != kExtraItemProcSlot ||
        link->mEquipItem == dItemNo_NONE_e)
    {
        s_activeExtraProcSlot = dItemNo_NONE_e;
        return;
    }

    if (is_active_dawnlight_item_slot(s_activeExtraProcSlot) &&
        link->checkGroupItem(link->mEquipItem, resolved_select_item(s_activeExtraProcSlot)))
    {
        return;
    }

    s_activeExtraProcSlot = dItemNo_NONE_e;
    for (u8 slot : std::array<u8, 2>{kZItemSlot, kDpadDownItemSlot}) {
        if (extra_item_slot_enabled(slot) &&
            link->checkGroupItem(link->mEquipItem, resolved_select_item(slot)))
        {
            s_activeExtraProcSlot = slot;
            return;
        }
    }
}

bool dpad_down_proc_item_context() {
    if (!dpad_down_item_slot_enabled()) {
        return false;
    }

    daAlink_c* link = daAlink_getAlinkActorClass();
    const u8 item = resolved_select_item(kDpadDownItemSlot);
    return link != nullptr && item != dItemNo_NONE_e &&
           link->mSelectItemId == kExtraItemProcSlot &&
           link->checkGroupItem(link->mEquipItem, item);
}

bool dpad_down_proc_select_context(const int index) {
    if (index != kExtraItemProcSlot || !dpad_down_item_slot_enabled() || s_zHudSelectLookup) {
        return false;
    }
    if (s_procSelectOverrideSlot != dItemNo_NONE_e) {
        return s_procSelectOverrideSlot == kDpadDownItemSlot;
    }
    return s_dpadDownProcSelectOverride || s_activeExtraProcSlot == kDpadDownItemSlot ||
           dpad_down_proc_item_context();
}

bool dpad_down_proc_stored_select_slot(const int index, u8* slot) {
    if (slot == nullptr || !dpad_down_proc_select_context(index)) {
        return false;
    }

    const u8 storedSlot = stored_select_slot(kDpadDownItemSlot);
    if (storedSlot == dItemNo_NONE_e) {
        return false;
    }

    *slot = storedSlot;
    return true;
}

bool extra_select_item_num_values(const int index, s16& itemNum, int* itemMax = nullptr) {
    if (!dpad_down_proc_select_context(index)) {
        return false;
    }

    const u8 itemNo = resolved_select_item(kDpadDownItemSlot);
    switch (itemNo) {
    case dItemNo_NORMAL_BOMB_e:
    case dItemNo_WATER_BOMB_e:
    case dItemNo_POKE_BOMB_e:
    case dItemNo_BOMB_ARROW_e: {
        const u8 bombSlot = stored_select_mix_no_arrow_slot(kDpadDownItemSlot);
        if (bombSlot < SLOT_15 || bombSlot >= SLOT_18) {
            itemNum = 0;
            if (itemMax != nullptr) {
                *itemMax = 0;
            }
            return true;
        }
        itemNum = dComIfGs_getBombNum(bombSlot - SLOT_15);
        if (itemMax != nullptr) {
            *itemMax = dComIfGs_getBombMax(itemNo);
        }
        return true;
    }
    case dItemNo_PACHINKO_e:
        itemNum = dComIfGs_getPachinkoNum();
        if (itemMax != nullptr) {
            *itemMax = dComIfGs_getPachinkoMax();
        }
        return true;
    case dItemNo_BEE_CHILD_e: {
        const u8 bottleSlot = stored_select_slot(kDpadDownItemSlot);
        if (bottleSlot < SLOT_11 || bottleSlot >= SLOT_15) {
            itemNum = 0;
            if (itemMax != nullptr) {
                *itemMax = 0;
            }
            return true;
        }
        itemNum = dComIfGs_getBottleNum(bottleSlot - SLOT_11);
        if (itemMax != nullptr) {
            *itemMax = dComIfGs_getBottleMax();
        }
        return true;
    }
    case dItemNo_BOMB_BAG_LV1_e:
        itemNum = 1;
        if (itemMax != nullptr) {
            *itemMax = 1;
        }
        return true;
    default:
        return false;
    }
}

bool set_extra_select_item_num(const int index, s16 itemNum) {
    if (!dpad_down_proc_select_context(index)) {
        return false;
    }

    const u8 itemNo = resolved_select_item(kDpadDownItemSlot);
    switch (itemNo) {
    case dItemNo_NORMAL_BOMB_e:
    case dItemNo_WATER_BOMB_e:
    case dItemNo_POKE_BOMB_e:
    case dItemNo_BOMB_ARROW_e: {
        const u8 bombSlot = stored_select_mix_no_arrow_slot(kDpadDownItemSlot);
        if (bombSlot < SLOT_15 || bombSlot >= SLOT_18) {
            return true;
        }
        itemNum = std::max<s16>(0, std::min<s16>(itemNum, dComIfGs_getBombMax(itemNo)));
        dComIfGs_setBombNum(bombSlot - SLOT_15, static_cast<u8>(itemNum));
        return true;
    }
    case dItemNo_PACHINKO_e:
        itemNum = std::max<s16>(0, std::min<s16>(itemNum, dComIfGs_getPachinkoMax()));
        dComIfGs_setPachinkoNum(static_cast<u8>(itemNum));
        return true;
    case dItemNo_BEE_CHILD_e: {
        const u8 bottleSlot = stored_select_slot(kDpadDownItemSlot);
        if (bottleSlot < SLOT_11 || bottleSlot >= SLOT_15) {
            return true;
        }
        itemNum = std::max<s16>(0, std::min<s16>(itemNum, dComIfGs_getBottleMax()));
        dComIfGs_setBottleNum(bottleSlot - SLOT_11, static_cast<u8>(itemNum));
        return true;
    }
    default:
        return false;
    }
}

bool add_extra_select_item_num(const int index, const s16 itemNum) {
    if (!dpad_down_proc_select_context(index)) {
        return false;
    }

    const u8 itemNo = resolved_select_item(kDpadDownItemSlot);
    switch (itemNo) {
    case dItemNo_NORMAL_BOMB_e:
    case dItemNo_WATER_BOMB_e:
    case dItemNo_POKE_BOMB_e:
    case dItemNo_BOMB_ARROW_e: {
        const u8 bombSlot = stored_select_mix_no_arrow_slot(kDpadDownItemSlot);
        if (bombSlot < SLOT_15 || bombSlot >= SLOT_18) {
            return true;
        }
        dComIfGp_setItemBombNumCount(bombSlot - SLOT_15, itemNum);
        return true;
    }
    case dItemNo_PACHINKO_e:
        dComIfGp_setItemPachinkoNumCount(itemNum);
        return true;
    case dItemNo_BEE_CHILD_e: {
        const u8 bottleSlot = stored_select_slot(kDpadDownItemSlot);
        if (bottleSlot < SLOT_11 || bottleSlot >= SLOT_15) {
            return true;
        }
        dComIfGs_addBottleNum(bottleSlot - SLOT_11, itemNum);
        return true;
    }
    default:
        return false;
    }
}

int find_select_button(daAlink_c* link, int itemNo) {
    if (link == nullptr) {
        return kSelectItemNotFound;
    }

    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        if (!logical_item_slot_enabled(i)) {
            continue;
        }
        if (link->checkGroupItem(itemNo, resolved_select_item(i))) {
            return item_proc_slot(i);
        }
    }
    return kSelectItemNotFound;
}

bool item_needs_z_valid_button(int itemNo) {
    return itemNo == dItemNo_HVY_BOOTS_e || itemNo == dItemNo_SPINNER_e;
}

bool select_slot_trigger(daAlink_c* link, const u8 slot) {
    if (slot == kDpadDownItemSlot) {
        return s_dpadDownTrig && resolved_select_item(kDpadDownItemSlot) != dItemNo_NONE_e;
    }
    if (slot == kZItemSlot && dpad_down_item_input_active()) {
        return false;
    }
    return link != nullptr && link->itemTriggerCheck(1 << slot);
}

bool select_slot_button(daAlink_c* link, const u8 slot) {
    if (slot == kDpadDownItemSlot) {
        return s_dpadDownHeld && resolved_select_item(kDpadDownItemSlot) != dItemNo_NONE_e;
    }
    if (slot == kZItemSlot && dpad_down_item_input_active()) {
        return false;
    }
    return link != nullptr && link->itemButtonCheck(1 << slot);
}

void sync_dpad_down_item_button_state(daAlink_c* link) {
    if (link == nullptr) {
        return;
    }

    sync_play_select_item(kZItemSlot);

    if (!dpad_down_item_input_active()) {
        return;
    }

    if (s_dpadDownHeld) {
        link->mItemButton |= 1 << kExtraItemProcSlot;
    }
    if (s_dpadDownTrig) {
        link->mItemTrigger |= 1 << kExtraItemProcSlot;
    }
}

u8 extra_heavy_boots_slot(daAlink_c* link) {
    if (link == nullptr) {
        return dItemNo_NONE_e;
    }

    for (u8 slot : std::array<u8, 2>{kZItemSlot, kDpadDownItemSlot}) {
        if (!extra_item_slot_enabled(slot)) {
            continue;
        }
        if (link->checkGroupItem(dItemNo_HVY_BOOTS_e, resolved_select_item(slot))) {
            return slot;
        }
    }
    return dItemNo_NONE_e;
}

bool z_heavy_boots_held(daAlink_c* link) {
    return select_slot_button(link, s_zHeavyBootsGuardSlot);
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
    s_zHeavyBootsGuardFrames = 0;
    s_zHeavyBootsGuardSlot = kZItemSlot;
}

void lock_z_heavy_boots_input(daAlink_c* link, bool manualToggleOff,
    const u8 slot = kZItemSlot) {
    s_zHeavyBootsGuardLink = link;
    s_zHeavyBootsManualToggleOff = manualToggleOff;
    s_zHeavyBootsWaitRelease = true;
    s_zHeavyBootsGuardFrames = manualToggleOff ? 24 : 0;
    s_zHeavyBootsGuardSlot = slot;
}

void tick_z_heavy_boots_guard(daAlink_c* link) {
    if (s_zHeavyBootsGuardLink == nullptr) {
        s_zHeavyBootsWaitRelease = false;
        s_zHeavyBootsGuardFrames = 0;
        return;
    }

    if (s_zHeavyBootsGuardLink != link || !s_zHeavyBootsWaitRelease) {
        return;
    }

    if (s_zHeavyBootsManualToggleOff) {
        if (s_zHeavyBootsGuardFrames != 0) {
            --s_zHeavyBootsGuardFrames;
        } else {
            clear_z_heavy_boots_input_lock();
        }
        return;
    }

    if (!z_heavy_boots_held(link)) {
        clear_z_heavy_boots_input_lock();
    }
}

u8 cursor_for_slot(dMenu_Ring_c* ring, u8 slot) {
    return slot == dItemNo_NONE_e ? dItemNo_NONE_e : ring->getCursorPos(slot);
}

void sync_ring_fields(dMenu_Ring_c* ring) {
    ring->mXButtonSlot = cursor_for_slot(ring, stored_select_slot(SELECT_ITEM_X));
    ring->mYButtonSlot = cursor_for_slot(ring, stored_select_slot(SELECT_ITEM_Y));
    ring->field_0x6ac = cursor_for_slot(ring, stored_select_slot(kZItemSlot));
    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        ring->field_0x6b4[i] = stored_select_slot(i);
        ring->field_0x6b8[i] = stored_mix_slot(i);
    }
}

void store_select_slots(const std::array<u8, kExtendedSelectItemCount>& slots,
    const std::array<u8, kExtendedSelectItemCount>& mixes) {
    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        set_stored_mix_slot(i, mixes[i]);
        set_stored_select_slot(i, slots[i]);
        sync_play_select_item(i);
    }
}

u8 extra_animation_slot(dMenu_Ring_c* ring, const u8 targetSlot) {
    if (ring == nullptr) {
        return dItemNo_NONE_e;
    }
    if (ring->field_0x6cd == targetSlot) {
        return ring->field_0x6cb;
    }
    return ring->field_0x6b4[targetSlot];
}

u8 extra_animation_item(dMenu_Ring_c* ring, const u8 targetSlot) {
    const u8 slot = extra_animation_slot(ring, targetSlot);
    if (slot == dItemNo_NONE_e) {
        return dItemNo_NONE_e;
    }
    return ring->getItem(slot, ring->field_0x6b8[targetSlot]);
}

void fix_extra_select_item_animation(dMenu_Ring_c* ring, const u8 targetSlot) {
    if (ring == nullptr || ring->field_0x674[targetSlot] == 0) {
        return;
    }

    const u8 item = extra_animation_item(ring, targetSlot);
    if (item != dItemNo_NONE_e) {
        ring->setSelectItem(targetSlot, item);
    }

    const u8 cursor = cursor_for_slot(ring, extra_animation_slot(ring, targetSlot));
    if (cursor != dItemNo_NONE_e) {
        ring->field_0x518[targetSlot] = ring->mItemSlotPosX[cursor];
        ring->field_0x528[targetSlot] = ring->mItemSlotPosY[cursor];
    }
    ring->field_0x538[targetSlot] = g_ringHIO.mSelectItemScale;
#if TARGET_PC
    ring->mSelectItemSlideElapsed[targetSlot] = 0.0f;
#endif
}

void fix_z_select_item_animation(dMenu_Ring_c* ring) {
    fix_extra_select_item_animation(ring, kZItemSlot);
}

bool extra_mix_item_on(dMenu_Ring_c* ring, const u8 targetSlot) {
    if (!extra_item_slot_enabled(targetSlot) || ring == nullptr || ring->mPlayerIsWolf ||
        dComIfGs_getItem(ring->mItemSlots[ring->mCurrentSlot], false) == dItemNo_NONE_e)
    {
        return false;
    }

    if (!bow_mix_item(dComIfGs_getItem(ring->mItemSlots[ring->mCurrentSlot], false))) {
        return false;
    }

    return (stored_select_slot(targetSlot) == SLOT_4 &&
               stored_mix_slot(targetSlot) == dItemNo_NONE_e) ||
           stored_mix_slot(targetSlot) == SLOT_4;
}

bool extra_mix_item_off(dMenu_Ring_c* ring, const u8 targetSlot) {
    return extra_item_slot_enabled(targetSlot) && ring != nullptr && !ring->mPlayerIsWolf &&
           dComIfGs_getItem(ring->mItemSlots[ring->mCurrentSlot], false) != dItemNo_NONE_e &&
           stored_mix_slot(targetSlot) == SLOT_4 &&
           ring->mItemSlots[ring->mCurrentSlot] == stored_select_slot(targetSlot);
}

bool set_extra_mix_item(dMenu_Ring_c* ring, const u8 targetSlot) {
    if (!extra_mix_item_on(ring, targetSlot) && !extra_mix_item_off(ring, targetSlot)) {
        return false;
    }

    for (int i = 0; i < MAX_SELECT_ITEM; ++i) {
        ring->setSelectItemForce(i);
    }

    std::array<u8, kExtendedSelectItemCount> slots = {
        stored_select_slot(SELECT_ITEM_X),
        stored_select_slot(SELECT_ITEM_Y),
        stored_select_slot(kZItemSlot),
        stored_select_slot(kDpadDownItemSlot),
    };
    std::array<u8, kExtendedSelectItemCount> mixes = {
        stored_mix_slot(SELECT_ITEM_X),
        stored_mix_slot(SELECT_ITEM_Y),
        stored_mix_slot(kZItemSlot),
        stored_mix_slot(kDpadDownItemSlot),
    };

    if (extra_mix_item_off(ring, targetSlot)) {
        Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_OFF, nullptr, 0, 0, 1.0f, 1.0f,
            -1.0f, -1.0f, 0);
        slots[targetSlot] = SLOT_4;
        mixes[targetSlot] = dItemNo_NONE_e;
        ring->field_0x6cb = stored_select_slot(targetSlot);
        ring->field_0x6cd = targetSlot;
    } else {
        Z2GetAudioMgr()->seStart(Z2SE_SY_ITEM_COMBINE_ON, nullptr, 0, 0, 1.0f, 1.0f,
            -1.0f, -1.0f, 0);
        slots[targetSlot] = ring->mItemSlots[ring->mCurrentSlot];
        mixes[targetSlot] = SLOT_4;
        ring->field_0x6cd = dItemNo_NONE_e;

        for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
            if (i == targetSlot || !logical_item_slot_enabled(i)) {
                continue;
            }
            if (slots[i] == slots[targetSlot]) {
                slots[i] = dItemNo_NONE_e;
                mixes[i] = dItemNo_NONE_e;
            }
        }
    }

    store_select_slots(slots, mixes);
    sync_ring_fields(ring);
    if (targetSlot == kZItemSlot) {
        ring->field_0x6ac = cursor_for_slot(ring, slots[kZItemSlot]);
    }
    ring->field_0x6b3 = targetSlot;
    ring->field_0x674[targetSlot] = 1;
    ring->setJumpItem(false);
    fix_extra_select_item_animation(ring, targetSlot);
    return true;
}

bool set_any_extra_mix_item(dMenu_Ring_c* ring) {
    if (z_item_slot_enabled() && set_extra_mix_item(ring, kZItemSlot)) {
        return true;
    }
    return dpad_down_item_slot_enabled() && set_extra_mix_item(ring, kDpadDownItemSlot);
}

void assign_current_item(dMenu_Ring_c* ring, u8 targetSlot) {
    const u8 selectedSlot = ring->mItemSlots[ring->mCurrentSlot];
    std::array<u8, kExtendedSelectItemCount> slots = {
        stored_select_slot(SELECT_ITEM_X),
        stored_select_slot(SELECT_ITEM_Y),
        stored_select_slot(kZItemSlot),
        stored_select_slot(kDpadDownItemSlot),
    };
    std::array<u8, kExtendedSelectItemCount> mixes = {
        stored_mix_slot(SELECT_ITEM_X),
        stored_mix_slot(SELECT_ITEM_Y),
        stored_mix_slot(kZItemSlot),
        stored_mix_slot(kDpadDownItemSlot),
    };

    u8 sourceSlot = dItemNo_NONE_e;
    bool selectedWasMixItem = false;
    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        if (i == targetSlot || !logical_item_slot_enabled(i)) {
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
        if (i == targetSlot || i == sourceSlot || !logical_item_slot_enabled(i)) {
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
    fix_extra_select_item_animation(ring, targetSlot);
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
        .oldTargetSlot = stored_select_slot(targetSlot),
        .oldTargetMix = stored_mix_slot(targetSlot),
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
        stored_select_slot(kZItemSlot),
        stored_select_slot(kDpadDownItemSlot),
    };
    std::array<u8, kExtendedSelectItemCount> mixes = {
        ring->field_0x6b8[SELECT_ITEM_X],
        ring->field_0x6b8[SELECT_ITEM_Y],
        stored_mix_slot(kZItemSlot),
        stored_mix_slot(kDpadDownItemSlot),
    };

    const u8 targetSlot = s_pendingAssign.targetSlot;
    const u8 selectedSlot = s_pendingAssign.selectedSlot;
    u8 sourceSlot = dItemNo_NONE_e;
    bool selectedWasMixItem = false;
    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        if (i == targetSlot || !logical_item_slot_enabled(i)) {
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
        if (i == targetSlot || i == sourceSlot || !logical_item_slot_enabled(i)) {
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
        if (is_dawnlight_item_slot(sourceSlot)) {
            fix_extra_select_item_animation(ring, sourceSlot);
        }
    }
    if (is_dawnlight_item_slot(targetSlot)) {
        fix_extra_select_item_animation(ring, targetSlot);
    }
    s_pendingAssign = {};
}

HookAction before_get_select_item(ModContext*, void* args, void* retval, void*) {
    const int index = mods::arg<int>(args, 0);
    if (!any_extra_item_slot_enabled() || index != kExtraItemProcSlot) {
        return HOOK_CONTINUE;
    }

    if (dpad_down_proc_select_context(index)) {
        *static_cast<u8*>(retval) = resolved_select_item(kDpadDownItemSlot);
        return HOOK_SKIP_ORIGINAL;
    }

    if (z_item_slot_enabled()) {
        *static_cast<u8*>(retval) = resolved_select_item(kZItemSlot);
        return HOOK_SKIP_ORIGINAL;
    }

    return HOOK_CONTINUE;
}

HookAction before_get_select_item_num(ModContext*, void* args, void* retval, void*) {
    s16 itemNum = 0;
    if (!extra_select_item_num_values(mods::arg<int>(args, 0), itemNum)) {
        return HOOK_CONTINUE;
    }

    *static_cast<s16*>(retval) = itemNum;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_get_select_item_max_num(ModContext*, void* args, void* retval, void*) {
    s16 itemNum = 0;
    int itemMax = 0;
    if (!extra_select_item_num_values(mods::arg<int>(args, 0), itemNum, &itemMax)) {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = itemMax;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_set_select_item_num(ModContext*, void* args, void*, void*) {
    if (!set_extra_select_item_num(mods::arg<int>(args, 0), mods::arg<s16>(args, 1))) {
        return HOOK_CONTINUE;
    }

    return HOOK_SKIP_ORIGINAL;
}

HookAction before_add_select_item_num(ModContext*, void* args, void*, void*) {
    if (!add_extra_select_item_num(mods::arg<int>(args, 0), mods::arg<s16>(args, 1))) {
        return HOOK_CONTINUE;
    }

    return HOOK_SKIP_ORIGINAL;
}

HookAction before_set_equip_bottle_item(ModContext*, void* args, void*, void*) {
    s_dpadDownBottleSlotRedirect = false;

    u8 slot = dItemNo_NONE_e;
    const int index = mods::arg<u8>(args, 1);
    if (!dpad_down_proc_stored_select_slot(index, &slot) || slot < SLOT_11 || slot >= SLOT_15) {
        return HOOK_CONTINUE;
    }

    s_dpadDownBottleOldSelectSlot = dComIfGs_getSelectItemIndex(kExtraItemProcSlot);
    s_dpadDownBottleSlotRedirect = true;
    dComIfGs_setSelectItemIndex(kExtraItemProcSlot, slot);
    return HOOK_CONTINUE;
}

void after_set_equip_bottle_item(ModContext*, void*, void*, void*) {
    if (!s_dpadDownBottleSlotRedirect) {
        return;
    }

    dComIfGs_setSelectItemIndex(kExtraItemProcSlot, s_dpadDownBottleOldSelectSlot);
    s_dpadDownBottleOldSelectSlot = dItemNo_NONE_e;
    s_dpadDownBottleSlotRedirect = false;
}

void after_set_select_item(ModContext*, void* args, void*, void*) {
    sync_play_select_item(mods::arg<int>(args, 0));
}

void after_pad_read(ModContext*, void*, void*, void*) {
    if (!any_extra_item_slot_enabled()) {
        s_dpadLeftHeld = false;
        s_dpadLeftTrig = false;
        s_dpadDownHeld = false;
        s_dpadDownTrig = false;
        s_touchMidnaTrig = false;
        s_skipTouchMidnaPressed = false;
        s_touchMidnaBlockStartFrames = 0;
        return;
    }

    interface_of_controller_pad& pad = mDoCPd_c::getCpadInfo(PAD_1);
    if (s_touchMidnaBlockStartFrames != 0) {
        pad.mButtonFlags &= ~PAD_BUTTON_START;
        pad.mPressedButtonFlags &= ~PAD_BUTTON_START;
        --s_touchMidnaBlockStartFrames;
    }

    if (dComIfGp_getLinkPlayer() == nullptr || daAlink_getAlinkActorClass() == nullptr ||
        z_item_menu_or_pause_context())
    {
        s_dpadLeftHeld = false;
        s_dpadLeftTrig = false;
        s_dpadDownHeld = false;
        s_dpadDownTrig = false;
        s_touchMidnaTrig = false;
        return;
    }

    if (z_item_slot_enabled()) {
        s_dpadLeftHeld = (pad.mButtonFlags & PAD_BUTTON_LEFT) != 0;
        s_dpadLeftTrig = (pad.mPressedButtonFlags & PAD_BUTTON_LEFT) != 0;
        if (s_dpadLeftHeld) {
            pad.mButtonFlags &= ~PAD_BUTTON_LEFT;
        }
        if (s_dpadLeftTrig) {
            pad.mPressedButtonFlags &= ~PAD_BUTTON_LEFT;
        }
    } else {
        s_dpadLeftHeld = false;
        s_dpadLeftTrig = false;
    }

    if (dpad_down_item_slot_enabled()) {
        s_dpadDownHeld = (pad.mButtonFlags & PAD_BUTTON_DOWN) != 0;
        s_dpadDownTrig = (pad.mPressedButtonFlags & PAD_BUTTON_DOWN) != 0;
        if (s_dpadDownHeld) {
            pad.mButtonFlags &= ~PAD_BUTTON_DOWN;
        }
        if (s_dpadDownTrig) {
            pad.mPressedButtonFlags &= ~PAD_BUTTON_DOWN;
        }
    } else {
        s_dpadDownHeld = false;
        s_dpadDownTrig = false;
    }
}

void after_ring_create(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    create_ring_z_prompt(ring);
    create_ring_dpad_down_prompt(ring);
}

HookAction before_ring_delete(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    destroy_ring_z_prompt(ring);
    destroy_ring_dpad_down_prompt(ring);
    return HOOK_CONTINUE;
}

void after_ring_draw(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    draw_ring_z_prompt(ring);
    draw_ring_dpad_down_prompt(ring);
}

HookAction before_meter_draw(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    {
        ScopedBool blockZProcLookup(s_zHudSelectLookup, true);
        update_z_hud_item(meter);
    }
    apply_round_xy_buttons(meter);
    apply_wii_u_hud_layout(meter);
    apply_xy_ammo_layout(meter);
    apply_hud_backing_visibility(meter);
    return HOOK_CONTINUE;
}

void after_meter_draw(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    {
        ScopedBool blockZProcLookup(s_zHudSelectLookup, true);
        draw_z_hud_item_meters(meter);
    }
    draw_dpad_down_hud_item(meter);
    restore_xy_ammo_layout(meter);
}

HookAction before_meter_draw_kantera(ModContext*, void* args, void*, void*) {
    if (hardcoded_hud_layout_enabled()) {
        const DuskModHudTransform transform = hud_layout_oil_transform();
        mods::arg_ref<f32>(args, 3) += transform.offset_x;
        mods::arg_ref<f32>(args, 4) += transform.offset_y;
    }
    return HOOK_CONTINUE;
}

void after_meter_draw_kantera(ModContext*, void* args, void*, void*) {
    if (hardcoded_hud_layout_enabled()) {
        auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
        if (meter != nullptr) {
            const DuskModHudTransform transform = hud_layout_oil_transform();
            meter->field_0x5cc[1] *= transform.scale;
            meter->field_0x5d8[1] *= transform.scale;
        }
    }
}

HookAction before_meter_draw_oxygen(ModContext*, void* args, void*, void*) {
    if (hardcoded_hud_layout_enabled()) {
        const DuskModHudTransform transform = hud_layout_oxygen_transform();
        mods::arg_ref<f32>(args, 3) += transform.offset_x;
        mods::arg_ref<f32>(args, 4) += transform.offset_y;
    }
    return HOOK_CONTINUE;
}

void after_meter_draw_oxygen(ModContext*, void* args, void*, void*) {
    if (hardcoded_hud_layout_enabled()) {
        auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
        if (meter != nullptr) {
            const DuskModHudTransform transform = hud_layout_oxygen_transform();
            meter->field_0x5cc[2] *= transform.scale;
            meter->field_0x5d8[2] *= transform.scale;
        }
    }
}

void after_meter_midna_alpha(ModContext*, void* args, void*, void*) {
    move_midna_hud_to_dpad(mods::arg<dMeter2Draw_c*>(args, 0));
}

void after_meter_draw_button_cross(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    if (meter == nullptr) {
        return;
    }

    const DuskModHudTransform dpadTransform = hud_layout_dpad_transform();
    apply_hud_pane_transform(HudPaneSlot::DPad, meter->mpButtonCrossParent,
        hardcoded_hud_layout_enabled(), dpadTransform.offset_x, dpadTransform.offset_y,
        dpadTransform.scale);
}

void after_meter_move_button_cross(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2_c*>(args, 0);
    if (meter == nullptr || meter->mpMeterDraw == nullptr) {
        return;
    }

    if (dpad_down_item_slot_enabled() && dMeter2Info_getWindowStatus() == 2 &&
        resolved_select_item(kDpadDownItemSlot) != dItemNo_NONE_e)
    {
        meter->mpMeterDraw->setAlphaButtonCrossAnimeMax();
        meter->mpMeterDraw->drawButtonCross(meter->mButtonCrossOFFPosX, meter->field_0x15c);
    }

    if (!hardcoded_hud_layout_enabled()) {
        return;
    }

    const DuskModHudTransform dpadTransform = hud_layout_dpad_transform();
    if (dpadTransform.parent_mode != kHudParentIndependent) {
        return;
    }

    meter->field_0x1b4 = 0;
    meter->field_0x15c = meter->mButtonCrossOFFPosY;
    meter->mpMeterDraw->drawButtonCross(meter->mButtonCrossOFFPosX, meter->mButtonCrossOFFPosY);
}

HookAction before_meter_map_draw(ModContext*, void* args, void*, void*) {
    apply_wii_u_minimap_layout(mods::arg<dMeterMap_c*>(args, 0));
    return HOOK_CONTINUE;
}

void after_meter_map_draw(ModContext*, void* args, void*, void*) {
    restore_wii_u_minimap_layout(mods::arg<dMeterMap_c*>(args, 0));
}

HookAction before_ring_set_active_cursor(ModContext*, void* args, void*, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!any_extra_item_slot_enabled() || ring == nullptr) {
        s_pendingAssign = {};
        return HOOK_CONTINUE;
    }

    if (mDoCPd_c::getTrigR(PAD_1) && set_any_extra_mix_item(ring)) {
        return HOOK_SKIP_ORIGINAL;
    }

    u8 targetSlot = dItemNo_NONE_e;
    if (z_item_slot_enabled() && mDoCPd_c::getTrigZ(PAD_1)) {
        targetSlot = kZItemSlot;
    } else if (dpad_down_item_slot_enabled() && mDoCPd_c::getTrigDown(PAD_1)) {
        targetSlot = kDpadDownItemSlot;
    }

    if (targetSlot == dItemNo_NONE_e) {
        capture_vanilla_assign(ring);
        return HOOK_CONTINUE;
    }

    s_pendingAssign = {};
    if (item_assign_allowed(ring)) {
        assign_current_item(ring, targetSlot);
        if (ring->mpItemExplain->getStatus() == 0) {
            ring->setStatus(dMenu_Ring_c::STATUS_WAIT);
            ring->stick_wait_init();
        }
    } else {
        Z2GetAudioMgr()->seStart(Z2SE_SYS_ERROR, nullptr, 0, 0, 1.0f, 1.0f, -1.0f, -1.0f, 0);
    }

    if (targetSlot == kDpadDownItemSlot) {
        interface_of_controller_pad& pad = mDoCPd_c::getCpadInfo(PAD_1);
        pad.mButtonFlags &= ~PAD_BUTTON_DOWN;
        pad.mPressedButtonFlags &= ~PAD_BUTTON_DOWN;
    }

    return HOOK_SKIP_ORIGINAL;
}

void after_ring_set_active_cursor(ModContext*, void* args, void*, void*) {
    rotate_pending_duplicate(mods::arg<dMenu_Ring_c*>(args, 0));
}

HookAction before_ring_is_mix_item_on(ModContext*, void* args, void* retval, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!(z_item_slot_enabled() && extra_mix_item_on(ring, kZItemSlot)) &&
        !(dpad_down_item_slot_enabled() && extra_mix_item_on(ring, kDpadDownItemSlot)))
    {
        return HOOK_CONTINUE;
    }

    *static_cast<bool*>(retval) = true;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_ring_is_mix_item_off(ModContext*, void* args, void* retval, void*) {
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (!(z_item_slot_enabled() && extra_mix_item_off(ring, kZItemSlot)) &&
        !(dpad_down_item_slot_enabled() && extra_mix_item_off(ring, kDpadDownItemSlot)))
    {
        return HOOK_CONTINUE;
    }

    *static_cast<bool*>(retval) = true;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_midna_talk_trigger(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<const daAlink_c*>(args, 0);
    if (!z_item_slot_enabled() || link == nullptr) {
        return HOOK_CONTINUE;
    }

    *static_cast<BOOL*>(retval) = s_dpadLeftTrig || consume_touch_midna_trigger();
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_check_item_button_change(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!any_extra_item_slot_enabled() || link == nullptr) {
        return HOOK_CONTINUE;
    }

    sync_dpad_down_item_button_state(link);
    refresh_active_extra_proc_slot(link);

    if (link->mProcID != daAlink_c::PROC_CANOE_PADDLE_PUT &&
        link->mEquipItem != dItemNo_NONE_e &&
        !link->checkEquipAnime())
    {
        for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
            if (!logical_item_slot_enabled(i)) {
                continue;
            }
            const u8 next = (i + 1) % kExtendedSelectItemCount;
            if (link->mEquipItem == resolved_select_item(i) &&
                (link->mEquipItem != resolved_select_item(next) ||
                    link->mSelectItemId != item_proc_slot(next)))
            {
                link->mSelectItemId = item_proc_slot(i);
            }
        }
    }
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_check_item_change_from_button(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!any_extra_item_slot_enabled() || link == nullptr) {
        return HOOK_CONTINUE;
    }

    sync_dpad_down_item_button_state(link);
    const bool dpadDownItemTrigger =
        s_dpadDownTrig && resolved_select_item(kDpadDownItemSlot) != dItemNo_NONE_e;

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
            link->swordTrigger() &&
            !dpadDownItemTrigger)
        {
            if (!link->checkEndResetFlg1(daPy_py_c::ERFLG1_SWORD_TRIGGER_NON)) {
                link->swordEquip(TRUE);
            }
        } else if (link->checkCanoeRide() &&
                   !link->checkStageName("F_SP103") &&
                   !link->checkCanoeSlider() &&
                   !link->checkFisingRodLure() &&
                   link->swordTrigger() &&
                   !dpadDownItemTrigger)
        {
            link->itemEquip(0x105);
        } else {
            for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
                if (!logical_item_slot_enabled(i)) {
                    continue;
                }
                if (!select_slot_trigger(link, i)) {
                    continue;
                }

                const u8 procSlot = item_proc_slot(i);
                ScopedBool dpadDownProcOverride(s_dpadDownProcSelectOverride,
                    i == kDpadDownItemSlot);
                ScopedU8 procLookupSlot(s_procSelectOverrideSlot,
                    is_dawnlight_item_slot(i) ? i : dItemNo_NONE_e);
                const int procType = link->checkNewItemChange(procSlot);
                if (procType != 0 && select_slot_trigger(link, i)) {
                    if (is_dawnlight_item_slot(i) &&
                        link->checkGroupItem(dItemNo_HVY_BOOTS_e, resolved_select_item(i)))
                    {
                        if (z_heavy_boots_input_locked(link)) {
                            continue;
                        }
                        lock_z_heavy_boots_input(link, link->checkEquipHeavyBoots(), i);
                    }
                    result = link->changeItemTriggerKeepProc(procSlot, procType);
                    if (result != FALSE) {
                        remember_active_extra_proc_slot(i);
                    }
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
                    if (!logical_item_slot_enabled(i)) {
                        continue;
                    }
                    if (resolved_select_item(i) == dItemNo_KANTERA_e) {
                        link->mSelectItemId = item_proc_slot(i);
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
    if (!any_extra_item_slot_enabled() || link == nullptr) {
        return HOOK_CONTINUE;
    }

    sync_dpad_down_item_button_state(link);

    for (u8 i = 0; i < kExtendedSelectItemCount; ++i) {
        if (!logical_item_slot_enabled(i)) {
            continue;
        }
        if (!link->checkGroupItem(itemNo, resolved_select_item(i)) ||
            !select_slot_trigger(link, i))
        {
            continue;
        }

        if (itemNo == dItemNo_HVY_BOOTS_e) {
            const u8 procSlot = item_proc_slot(i);
            ScopedBool dpadDownProcOverride(s_dpadDownProcSelectOverride,
                i == kDpadDownItemSlot);
            ScopedU8 procLookupSlot(s_procSelectOverrideSlot,
                is_dawnlight_item_slot(i) ? i : dItemNo_NONE_e);
            if (is_dawnlight_item_slot(i)) {
                if (link->checkEquipHeavyBoots()) {
                    if (!z_heavy_boots_input_locked(link) &&
                        link->checkNewItemChange(procSlot) == kItemProcBootsEquip)
                    {
                        lock_z_heavy_boots_input(link, true, i);
                        link->changeItemTriggerKeepProc(procSlot, kItemProcBootsEquip);
                        remember_active_extra_proc_slot(i);
                    }
                    *static_cast<int*>(retval) = 0;
                    return HOOK_SKIP_ORIGINAL;
                }
                if (z_heavy_boots_input_locked(link)) {
                    *static_cast<int*>(retval) = 0;
                    return HOOK_SKIP_ORIGINAL;
                }
                lock_z_heavy_boots_input(link, link->checkEquipHeavyBoots(), i);
            }
        } else {
            link->mSelectItemId = item_proc_slot(i);
            remember_active_extra_proc_slot(i);
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
    if (!any_extra_item_slot_enabled() || link == nullptr || !item_needs_z_valid_button(itemNo)) {
        return HOOK_CONTINUE;
    }

    bool heldByExtraSlot = false;
    for (u8 slot : std::array<u8, 2>{kZItemSlot, kDpadDownItemSlot}) {
        if (!extra_item_slot_enabled(slot)) {
            continue;
        }
        if (link->checkGroupItem(itemNo, resolved_select_item(slot))) {
            heldByExtraSlot = true;
            break;
        }
    }
    if (!heldByExtraSlot) {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = SELECT_ITEM_X;
    return HOOK_SKIP_ORIGINAL;
}

HookAction before_set_heavy_boots(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const int enable = mods::arg<int>(args, 1);
    const u8 bootsSlot = extra_heavy_boots_slot(link);
    if (!any_extra_item_slot_enabled() || link == nullptr || !link->checkEquipHeavyBoots() ||
        link->checkNotHeavyBootsStage() || bootsSlot == dItemNo_NONE_e)
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

    if (!link->checkEquipHeavyBoots()) {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

void after_player_execute(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!any_extra_item_slot_enabled() || link == nullptr || link->checkWolf()) {
        return;
    }

    tick_z_heavy_boots_guard(link);
    sync_play_select_item(kZItemSlot);
    if (z_item_slot_enabled() && resolved_select_item(kZItemSlot) != dItemNo_NONE_e) {
        dMeter2Info_onUseButton(METER2_USEBUTTON_Z);
    }
}

HookAction before_meter_button_set_string(ModContext*, void* args, void*, void*) {
    if (!z_item_slot_enabled()) {
        return HOOK_CONTINUE;
    }

    u8& button = mods::arg_ref<u8>(args, 2);
    if (button != dMeterButton_c::BUTTON_Z_e) {
        return HOOK_CONTINUE;
    }

    s_zPromptAsDpadLeftThisFrame = true;
    return HOOK_CONTINUE;
}

HookAction before_meter_button_execute(ModContext*, void* args, void*, void*) {
    const bool replacePrompt = s_zPromptAsDpadLeftThisFrame && z_item_slot_enabled();
    s_zPromptAsDpadLeftThisFrame = false;
    auto* meter = mods::arg<dMeterButton_c*>(args, 0);
    s_applyZPromptDpadIcon = false;
    s_hideZPromptAfterExecute = false;

    if (!replacePrompt) {
        restore_z_prompt_visuals(meter);
        return HOOK_CONTINUE;
    }

    bool& drawZ = mods::arg_ref<bool>(args, 5);
    if (!drawZ) {
        restore_z_prompt_visuals(meter);
        s_hideZPromptAfterExecute = true;
        return HOOK_CONTINUE;
    }

    s_applyZPromptDpadIcon = true;
    return HOOK_CONTINUE;
}

void after_meter_button_execute(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeterButton_c*>(args, 0);
    if (s_hideZPromptAfterExecute) {
        restore_z_prompt_visuals(meter);
        hide_z_prompt_panes(meter);
        s_hideZPromptAfterExecute = false;
    }

    if (!s_applyZPromptDpadIcon) {
        return;
    }

    if (meter == nullptr || !meter->isButtonShowBit(dMeterButton_c::BUTTON_Z_e)) {
        restore_z_prompt_visuals(meter);
        hide_z_prompt_panes(meter);
        s_applyZPromptDpadIcon = false;
        return;
    }

    apply_z_prompt_visuals(meter);
    s_applyZPromptDpadIcon = false;
}

HookAction before_touch_sync_action_bar(ModContext*, void*, void*, void*) {
    if (s_skipTouchMidnaPressed) {
        s_inTouchActionBarSync = false;
        s_touchActionBarHiddenCall = 0;
        return HOOK_SKIP_ORIGINAL;
    }

    s_inTouchActionBarSync = true;
    s_touchActionBarHiddenCall = 0;
    return HOOK_CONTINUE;
}

void after_touch_sync_action_bar(ModContext*, void*, void*, void*) {
    Rml::Element* skipElement = s_skipTouchElement;
    const bool canShowMidna = skip_touch_can_be_midna();

    s_inTouchActionBarSync = false;
    s_touchActionBarHiddenCall = 0;

    if (canShowMidna && skipElement != nullptr) {
        set_skip_touch_hidden(skipElement, false);
        sync_skip_touch_midna_button(skipElement);
    } else if (s_skipTouchMidnaPressed && skipElement != nullptr) {
        set_skip_touch_hidden(skipElement, false);
    } else if (s_skipTouchMidnaMode) {
        restore_skip_touch_button(skipElement);
    }
}

HookAction before_rml_set_pseudo_class(ModContext*, void* args, void*, void*) {
    if (!s_inTouchActionBarSync) {
        return HOOK_CONTINUE;
    }

    auto* element = mods::arg<Rml::Element*>(args, 0);
    const auto* pseudoClass = mods::arg<const Rml::String*>(args, 1);
    const bool active = mods::arg<bool>(args, 2);
    if (pseudoClass == nullptr || *pseudoClass != "hidden") {
        return HOOK_CONTINUE;
    }

    ++s_touchActionBarHiddenCall;
    if (s_touchActionBarHiddenCall != 2 || element == nullptr) {
        return HOOK_CONTINUE;
    }

    if (!skip_touch_can_be_midna()) {
        return HOOK_CONTINUE;
    }

    s_skipTouchElement = element;
    return active ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}

HookAction before_touch_set_control_pressed(ModContext*, void* args, void*, void*) {
    const auto control = mods::arg<dusk::ui::Control>(args, 1);
    const bool pressed = mods::arg<bool>(args, 2);
    if (control != dusk::ui::Control::SKIP) {
        return HOOK_CONTINUE;
    }

    const bool midnaTouch =
        s_skipTouchMidnaMode || s_skipTouchMidnaPressed || s_touchMidnaBlockStartFrames != 0;
    if (pressed) {
        if (!skip_touch_can_be_midna()) {
            return midnaTouch && !cutscene_skip_touch_visible() ? HOOK_SKIP_ORIGINAL :
                HOOK_CONTINUE;
        }

        s_skipTouchMidnaPressed = true;
        s_touchMidnaTrig = true;
        s_touchMidnaBlockStartFrames = 4;
        return HOOK_SKIP_ORIGINAL;
    }

    if (!midnaTouch) {
        return HOOK_CONTINUE;
    }

    s_skipTouchMidnaPressed = false;
    return cutscene_skip_touch_visible() ? HOOK_CONTINUE : HOOK_SKIP_ORIGINAL;
}

void after_midna_icon_source(ModContext*, void*, void* retval, void*) {
    const std::string source = z_touch_item_source();
    if (!source.empty()) {
        *static_cast<std::string*>(retval) = source;
    }
}

void after_midna_icon_revision(ModContext*, void*, void* retval, void*) {
    if (!z_touch_item_source().empty()) {
        *static_cast<uint64_t*>(retval) = z_touch_item_revision();
    }
}

ModResult add_hook(ModResult result, ModError* error) {
    return result == MOD_OK ? MOD_OK :
        mods::set_error(error, result, "failed to install Dawnlight Z item slot hooks");
}

}  // namespace

bool item_slots_proc_select_item_index(const int index, u8* slot) {
    return dpad_down_proc_stored_select_slot(index, slot);
}

ModResult install_item_slot_hooks(ModError* error) {
    ModResult result = mods::hook_add_pre<GetSelectItemHook>(svc_hook, before_get_select_item);
    if (result == MOD_OK) {
        result = mods::hook_add_post<SetSelectItemHook>(svc_hook, after_set_select_item);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<GetSelectItemNumHook>(svc_hook, before_get_select_item_num);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<GetSelectItemMaxNumHook>(svc_hook, before_get_select_item_max_num);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<SetSelectItemNumHook>(svc_hook, before_set_select_item_num);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<AddSelectItemNumHook>(svc_hook, before_add_select_item_num);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<SetEquipBottleItemInHook>(svc_hook, before_set_equip_bottle_item);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<SetEquipBottleItemInHook>(svc_hook, after_set_equip_bottle_item);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<PadReadHook>(svc_hook, after_pad_read);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<RingCreateHook>(svc_hook, after_ring_create);
    }
    (void)mods::hook_add_pre<RingDeleteHook>(svc_hook, before_ring_delete);
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
        result = mods::hook_add_pre<MeterDrawKanteraHook>(svc_hook, before_meter_draw_kantera);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterDrawKanteraHook>(svc_hook, after_meter_draw_kantera);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterDrawOxygenHook>(svc_hook, before_meter_draw_oxygen);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterDrawOxygenHook>(svc_hook, after_meter_draw_oxygen);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterMidnaAlphaHook>(svc_hook, after_meter_midna_alpha);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterDrawButtonCrossHook>(svc_hook, after_meter_draw_button_cross);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterMoveButtonCrossHook>(svc_hook, after_meter_move_button_cross);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterMapDrawHook>(svc_hook, before_meter_map_draw);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterMapDrawHook>(svc_hook, after_meter_map_draw);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<RingSetActiveCursorHook>(svc_hook, before_ring_set_active_cursor);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<RingSetActiveCursorHook>(svc_hook, after_ring_set_active_cursor);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<RingIsMixItemOnHook>(svc_hook, before_ring_is_mix_item_on);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<RingIsMixItemOffHook>(svc_hook, before_ring_is_mix_item_off);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MidnaTalkTriggerHook>(svc_hook, before_midna_talk_trigger);
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
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterButtonSetStringHook>(svc_hook, before_meter_button_set_string);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<MeterButtonExecuteHook>(svc_hook, before_meter_button_execute);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MeterButtonExecuteHook>(svc_hook, after_meter_button_execute);
    }
#if defined(__ANDROID__)
    if (result == MOD_OK) {
        result = mods::hook_add_post<MidnaIconSourceHook>(svc_hook, after_midna_icon_source);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<MidnaIconRevisionHook>(svc_hook, after_midna_icon_revision);
    }
    if (result == MOD_OK) {
        result = mods::hook::install<RmlSetInnerRMLHook>(svc_hook);
    }
    if (result == MOD_OK) {
        result = mods::hook::install<UpdateMidnaIconTextureHook>(svc_hook);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<TouchSyncActionBarHook>(svc_hook, before_touch_sync_action_bar);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_post<TouchSyncActionBarHook>(svc_hook, after_touch_sync_action_bar);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<RmlSetPseudoClassHook>(svc_hook, before_rml_set_pseudo_class);
    }
    if (result == MOD_OK) {
        result = mods::hook_add_pre<TouchSetControlPressedHook>(
            svc_hook, before_touch_set_control_pressed);
    }
#endif
    return add_hook(result, error);
}

void shutdown_item_slot_hooks() {
    clear_ring_z_prompt_refs();

    if (s_dpadPromptTextureBuffer != nullptr) {
        JKRFreeToHeap(static_cast<JKRHeap*>(mDoExt_getJ2dHeap()), s_dpadPromptTextureBuffer);
        s_dpadPromptTextureBuffer = nullptr;
        s_dpadPromptTextureSize = 0;
    }

    if (s_dpadPromptArchive != nullptr) {
        JKRUnmountArchive(s_dpadPromptArchive);
        s_dpadPromptArchive = nullptr;
    }

    if (s_dpadPromptArchiveMount != nullptr && s_dpadPromptArchiveMount->sync()) {
        s_dpadPromptArchiveMount->destroy();
        s_dpadPromptArchiveMount = nullptr;
    }
}

}  // namespace dawnlight
