/**
 * m_Do_controller_pad.cpp
 * JUTGamePad Wrapper and Conversion
 */

#include "m_Do/m_Do_controller_pad.h"
#include "JSystem/JAWExtSystem/JAWExtSystem.h"
#include "SSystem/SComponent/c_lib.h"
#include "d/d_com_inf_game.h"
#include "dusk/touch_controls.hpp"
#include "f_ap/f_ap_game.h"
#include "m_Do/m_Do_Reset.h"
#include "m_Do/m_Do_main.h"
#include "tracy/Tracy.hpp"
#include <SDL3/SDL_gamepad.h>

JUTGamePad* mDoCPd_c::m_gamePad[4];

interface_of_controller_pad mDoCPd_c::m_cpadInfo[4];
interface_of_controller_pad mDoCPd_c::m_debugCpadInfo[4];

namespace {
bool sWiiUPhysicalLHeld[4] = {};
bool sWiiUPhysicalZLHeld[4] = {};
bool sWiiUMappedZLHeld[4] = {};
bool sWiiUPhysicalZRHeld[4] = {};

void resetWiiUPhysicalShoulderState(u32 port) {
    if (port < 4) {
        sWiiUPhysicalLHeld[port] = false;
        sWiiUPhysicalZLHeld[port] = false;
        sWiiUMappedZLHeld[port] = false;
        sWiiUPhysicalZRHeld[port] = false;
    }
}

bool nativeButtonHeld(SDL_Gamepad* gamepad, u32 nativeButton) {
    switch (nativeButton) {
    case PAD_NATIVE_BUTTON_AXIS_LEFT_TRIGGER:
        return SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 16384;
    case PAD_NATIVE_BUTTON_AXIS_RIGHT_TRIGGER:
        return SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 16384;
    default:
        return SDL_GetGamepadButton(gamepad, static_cast<SDL_GamepadButton>(nativeButton)) != 0;
    }
}

bool mappedButtonHeld(u32 port, PADButton button) {
    const s32 index = PADGetIndexForPort(port);
    if (index < 0) {
        return false;
    }

    SDL_Gamepad* gamepad = PADGetSDLGamepadForIndex(static_cast<u32>(index));
    if (gamepad == nullptr) {
        return false;
    }

    u32 count = 0;
    PADButtonMapping* mappings = PADGetButtonMappings(port, &count);
    if (mappings == nullptr) {
        return false;
    }

    for (u32 i = 0; i < count; ++i) {
        if (mappings[i].padButton != button ||
            mappings[i].nativeButton == PAD_NATIVE_BUTTON_INVALID)
        {
            continue;
        }
        return nativeButtonHeld(gamepad, mappings[i].nativeButton);
    }

    return false;
}

bool mappedButtonHeldUnlessSameNativeAs(u32 port, PADButton button, PADButton conflictButton) {
    const s32 index = PADGetIndexForPort(port);
    if (index < 0) {
        return false;
    }

    SDL_Gamepad* gamepad = PADGetSDLGamepadForIndex(static_cast<u32>(index));
    if (gamepad == nullptr) {
        return false;
    }

    u32 count = 0;
    PADButtonMapping* mappings = PADGetButtonMappings(port, &count);
    if (mappings == nullptr) {
        return false;
    }

    for (u32 i = 0; i < count; ++i) {
        if (mappings[i].padButton != button ||
            mappings[i].nativeButton == PAD_NATIVE_BUTTON_INVALID ||
            !nativeButtonHeld(gamepad, mappings[i].nativeButton))
        {
            continue;
        }

        for (u32 j = 0; j < count; ++j) {
            if (mappings[j].padButton == conflictButton &&
                mappings[j].nativeButton == mappings[i].nativeButton)
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

bool useModernShoulderLayout(u32 port) {
    const PADControllerType type = PADGetControllerType(port);
    return type != PAD_TYPE_GAMECUBE && type != PAD_TYPE_NSO_GAMECUBE;
}

void remapWiiUPhysicalShoulders(interface_of_controller_pad* interface, u32 port) {
    #if TARGET_PC
    if (port >= 4 || PADGetIndexForPort(port) < 0 || !useModernShoulderLayout(port)) {
        resetWiiUPhysicalShoulderState(port);
        return;
    }

    const bool useWiiUStyle = dusk::UseWiiUControllerStyle();
    const bool physicalLHeld = mappedButtonHeld(port, PAD_TRIGGER_L);
    const bool physicalLPressed = physicalLHeld && !sWiiUPhysicalLHeld[port];
    sWiiUPhysicalLHeld[port] = physicalLHeld;

    const bool mappedZLHeld = useWiiUStyle && mappedButtonHeld(port, PAD_TRIGGER_ZL);
    const bool mappedZLPressed = mappedZLHeld && !sWiiUMappedZLHeld[port];
    sWiiUMappedZLHeld[port] = mappedZLHeld;
    const bool mappedZRHeld = useWiiUStyle &&
                              mappedButtonHeldUnlessSameNativeAs(
                                  port, PAD_TRIGGER_R, PAD_TRIGGER_Z);

    bool physicalZLHeld = false;
    bool physicalZLPressed = false;
    if (useWiiUStyle) {
        physicalZLHeld = sWiiUPhysicalZLHeld[port];
        if (interface->mTriggerLeft > fapGmHIO_getLROnValue()) {
            physicalZLHeld = true;
        } else if (interface->mTriggerLeft < fapGmHIO_getLROffValue()) {
            physicalZLHeld = false;
        }
        physicalZLPressed = physicalZLHeld && !sWiiUPhysicalZLHeld[port];
    }
    sWiiUPhysicalZLHeld[port] = physicalZLHeld;

    bool physicalZRHeld = false;
    bool physicalZRPressed = false;
    if (useWiiUStyle) {
        physicalZRHeld = sWiiUPhysicalZRHeld[port];
        if (interface->mTriggerRight > fapGmHIO_getLROnValue() || mappedZRHeld) {
            physicalZRHeld = true;
        } else if (interface->mTriggerRight < fapGmHIO_getLROffValue()) {
            physicalZRHeld = false;
        }
        physicalZRPressed = physicalZRHeld && !sWiiUPhysicalZRHeld[port];
    }
    sWiiUPhysicalZRHeld[port] = physicalZRHeld;

    interface->mButtonFlags &= ~(PAD_TRIGGER_L | PAD_TRIGGER_ZL);
    interface->mPressedButtonFlags &= ~(PAD_TRIGGER_L | PAD_TRIGGER_ZL);
    interface->mTriggerLeft = 0.0f;
    if (useWiiUStyle) {
        interface->mButtonFlags &= ~PAD_TRIGGER_R;
        interface->mPressedButtonFlags &= ~PAD_TRIGGER_R;
        interface->mTriggerRight = 0.0f;
    }

    if (physicalLHeld) {
        interface->mButtonFlags |= PAD_TRIGGER_L;
        interface->mTriggerLeft = 1.0f;
    }
    if (physicalLPressed) {
        interface->mPressedButtonFlags |= PAD_TRIGGER_L;
    }
    if (physicalZLHeld || mappedZLHeld) {
        interface->mButtonFlags |= PAD_TRIGGER_ZL;
    }
    if (physicalZLPressed || mappedZLPressed) {
        interface->mPressedButtonFlags |= PAD_TRIGGER_ZL;
    }
    if (physicalZRHeld) {
        interface->mButtonFlags |= PAD_TRIGGER_R;
        interface->mTriggerRight = 1.0f;
    }
    if (physicalZRPressed) {
        interface->mPressedButtonFlags |= PAD_TRIGGER_R;
    }
    #endif
}
}  // namespace

void mDoCPd_c::create() {
    #if PLATFORM_GCN || PLATFORM_SHIELD
    m_gamePad[0] = JKR_NEW JUTGamePad(JUTGamePad::EPort1);
    #endif

    if (DEBUG || mDoMain::developmentMode != 0) {
        #if PLATFORM_WII
        m_gamePad[0] = JKR_NEW JUTGamePad(JUTGamePad::EPort1);
        #endif

        m_gamePad[1] = JKR_NEW JUTGamePad(JUTGamePad::EPort2);
        m_gamePad[2] = JKR_NEW JUTGamePad(JUTGamePad::EPort3);
        m_gamePad[3] = JKR_NEW JUTGamePad(JUTGamePad::EPort4);
    } else {
        #if PLATFORM_WII
        m_gamePad[0] = NULL;
        #endif

        m_gamePad[1] = NULL;
        m_gamePad[2] = NULL;
        m_gamePad[3] = NULL;
    }

    #if PLATFORM_GCN || PLATFORM_SHIELD
    if (!mDoRst::isReset()) {
        JUTGamePad::clearResetOccurred();
        JUTGamePad::setResetCallback(mDoRst_resetCallBack, NULL);
    }
    #endif
    JUTGamePad::setAnalogMode(3);

    interface_of_controller_pad* cpad = &m_cpadInfo[0];
    for (int i = 0; i < 4; i++) {
        cpad->mHoldLockL = cpad->mTrigLockL = false;
        cpad->mHoldLockR = cpad->mTrigLockR = false;
        cpad++;
    }
}

void mDoCPd_c::read() {
    ZoneScoped;
    JUTGamePad::read();

    if (!mDoRst::isReset() && mDoRst::is3ButtonReset()) {
        if (!JUTGamePad::getGamePad(mDoRst::get3ButtonResetPort())->isPushing3ButtonReset()) {
            mDoRst::off3ButtonReset();
        }
    }

#if DEBUG
    if (m_gamePad[3]) {
        JAWExtSystem::padProc(*m_gamePad[3]);
    }
#endif

    JUTGamePad** pad = m_gamePad;
    interface_of_controller_pad* interface = m_cpadInfo;
#if DEBUG
    interface_of_controller_pad* interface2 = m_debugCpadInfo;
    if (dComIfG_isDebugMode()) {
        interface_of_controller_pad* tmp = interface;
        interface = interface2;
        interface2 = tmp;
    }
#endif

    for (u32 i = 0; i < 4; i++) {
        if (*pad == NULL) {
            cLib_memSet(interface, 0, sizeof(interface_of_controller_pad));
            resetWiiUPhysicalShoulderState(i);
        } else {
            convert(interface, *pad);
            remapWiiUPhysicalShoulders(interface, i);
        }
        if (i == PAD_1) {
            dusk::touch_controls::MergeToPad(*interface);
        }
        LRlockCheck(interface);
#if DEBUG
        cLib_memSet(interface2, 0, sizeof(interface_of_controller_pad));
#endif
        pad++;
        interface++;
#if DEBUG
        interface2++;
#endif
    }
}

void mDoCPd_c::convert(interface_of_controller_pad* pInterface, JUTGamePad* pPad) {
    pInterface->mButtonFlags = pPad->getButton();
    pInterface->mPressedButtonFlags = pPad->getTrigger();
    pInterface->mMainStickPosX = pPad->getMainStickX();
    pInterface->mMainStickPosY = pPad->getMainStickY();
    pInterface->mMainStickValue = pPad->getMainStickValue();
    pInterface->mMainStickAngle = pPad->getMainStickAngle();
    pInterface->mCStickPosX = pPad->getSubStickX();
    pInterface->mCStickPosY = pPad->getSubStickY();
    pInterface->mCStickValue = pPad->getSubStickValue();
    pInterface->mCStickAngle = pPad->getSubStickAngle();

    mDoCPd_ANALOG_CONV(pPad->getAnalogA(), pInterface->mAnalogA);
    mDoCPd_ANALOG_CONV(pPad->getAnalogB(), pInterface->mAnalogB);
    mDoCPd_TRIGGER_CONV(pPad->getAnalogL(), pInterface->mTriggerLeft);
    mDoCPd_TRIGGER_CONV(pPad->getAnalogR(), pInterface->mTriggerRight);

    pInterface->mGamepadErrorFlags = pPad->getErrorStatus();
}

void mDoCPd_c::LRlockCheck(interface_of_controller_pad* interface) {
    f32 trigger = interface->mTriggerLeft;
    interface->mTrigLockL = false;
    interface->mTrigLockR = false;

    if (trigger > fapGmHIO_getLROnValue()) {
        if (interface->mHoldLockL != true) {
            interface->mTrigLockL = true;
        }
        interface->mHoldLockL = true;
    } else if (trigger < fapGmHIO_getLROffValue()) {
        interface->mHoldLockL = false;
    }

    trigger = interface->mTriggerRight;
    if (trigger > fapGmHIO_getLROnValue()) {
        if (interface->mHoldLockR != true) {
            interface->mTrigLockR = true;
        }
        interface->mHoldLockR = true;
    } else if (trigger < fapGmHIO_getLROffValue()) {
        interface->mHoldLockR = false;
    }
}

void mDoCPd_c::recalibrate(void) {
    JUTGamePad::clearForReset();
    JUTGamePad::CRumble::setEnabled(PAD_CHAN3_BIT | PAD_CHAN2_BIT | PAD_CHAN1_BIT | PAD_CHAN0_BIT);
}
