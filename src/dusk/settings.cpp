#include "dusk/settings.h"
#include "dusk/config.hpp"

namespace dusk {

#if defined(__ANDROID__) || (defined(TARGET_OS_IOS) && TARGET_OS_IOS) || \
    (defined(TARGET_OS_TV) && TARGET_OS_TV)
constexpr bool kDefaultTouchControlsEnabled = true;
#else
constexpr bool kDefaultTouchControlsEnabled = false;
#endif

UserSettings g_userSettings = {
    .video = {
        .enableFullscreen {"video.enableFullscreen", false},
        .enableVsync {"video.enableVsync", true},
        .lockAspectRatio {"video.lockAspectRatio", false},
        .enableFpsOverlay {"game.enableFpsOverlay", false},
        .fpsOverlayCorner {"game.fpsOverlayCorner", 0},
        .maxFrameRate {"video.maxFrameRate", 240},
    },

    .audio = {
        .masterVolume {"audio.masterVolume", 60},
        .mainMusicVolume {"audio.mainMusicVolume", 100},
        .subMusicVolume {"audio.subMusicVolume", 100},
        .soundEffectsVolume {"audio.soundEffectsVolume", 100},
        .fanfareVolume {"audio.fanfareVolume", 100},
        .enableReverb {"audio.enableReverb", true},
        .enableHrtf {"audio.enableHrtf", false},
        .menuSounds {"audio.menuSounds", true},
    },

    .game = {
        .language { "game.language", GameLanguage::English },

        // Quality of Life
        .enableQuickTransform {"game.enableQuickTransform", false},
        .hideTvSettingsScreen {"game.hideTvSettingsScreen", true},
        .biggerWallets {"game.biggerWallets", false},
        .noReturnRupees {"game.noReturnRupees", false},
        .disableRupeeCutscenes {"game.disableRupeeCutscenes", false},
        .noSwordRecoil {"game.noSwordRecoil", false},
        .enableJumpButton {"game.enableJumpButton", true},
        .manualShielding {"game.manualShielding", false},
        .damageMultiplier {"game.damageMultiplier", 1},
        .newGamePlusHealthScalePercent {"game.newGamePlusHealthScalePercent", 0},
        .noHeartDrops {"game.noHeartDrops", false},
        .instantDeath {"game.instantDeath", false},
        .fastClimbing {"game.fastClimbing", false},
        .noMissClimbing {"game.noMissClimbing", false},
        .fastTears {"game.fastTears", false},
        .no2ndFishForCat {"game.no2ndFishForCat", false},
        .instantSaves {"game.instantSaves", false},
        .instantText {"game.instantText", false},
        .sunsSong {"game.sunsSong", false},
        .autoSave {"game.autoSave", false},

        // Preferences
        .enableMirrorMode {"game.enableMirrorMode", false},
        .minimalHUD {"game.minimalHUD", false},
        .pauseOnFocusLost {"game.pauseOnFocusLost", false},
        .enableLinkDollRotation {"game.enableLinkDollRotation", false},
        .enableAchievementToasts {"game.enableAchievementToasts", true},
        .enableControllerToasts {"game.enableControllerToasts", true},
        .enableDiscordPresence {"game.enableDiscordPresence", true},

        // Graphics
        .bloomMode {"game.bloomMode", BloomMode::Dusk},
        .bloomMultiplier {"game.bloomMultiplier", 1.0f},
        .disableWaterRefraction {"game.disableWaterRefraction", false},
        .enableTextureReplacements {"game.enableTextureReplacements", true},
        .enableFrameInterpolation {"game.enableFrameInterpolation", FrameInterpMode::Off},
        .internalResolutionScale {"game.internalResolutionScale", 0},
        .shadowResolutionMultiplier {"game.shadowResolutionMultiplier", 1},
        .resampler {"game.resampler", Resampler::Bilinear},
        .enableDepthOfField {"game.enableDepthOfField", true},
        .enableMapBackground {"game.enableMapBackground", true},
        .disableCutscenePillarboxing {"game.disableCutscenePillarboxing", false},

        // Audio
        .noLowHpSound {"game.noLowHpSound", false},
        .midnasLamentNonStop {"game.midnasLamentNonStop", false},

        // Input
        .gyroMode {"game.gyroMode", GyroMode::Sensor},
        .enableGyroAim {"game.enableGyroAim", false},
        .enableGyroRollgoal {"game.enableGyroRollgoal", false},
        .gyroSensitivityX {"game.gyroSensitivityX", 1.0f},
        .gyroSensitivityY {"game.gyroSensitivityY", 1.0f},
        .gyroSensitivityRollgoal {"game.gyroSensitivityRollgoal", 1.0f},
        .gyroSmoothing {"game.gyroSmoothing", 0.65f},
        .gyroDeadband {"game.gyroDeadband", 0.04f},
        .gyroInvertPitch {"game.gyroInvertPitch", false},
        .gyroInvertYaw {"game.gyroInvertYaw", false},
        .freeCamera {"game.freeCamera", false},
        .invertCameraXAxis {"game.invertCameraXAxis", false},
        .invertCameraYAxis {"game.invertCameraYAxis", false},
        .invertFirstPersonXAxis {"game.invertFirstPersonXAxis", false},
        .invertFirstPersonYAxis {"game.invertFirstPersonYAxis", false},
        .enableAimMovement {"game.enableAimMovement", false},
        .aimMode {"game.aimMode", AimMode::Vanilla},
        .enableThirdPersonAim {"game.enableThirdPersonAim", false},
        .freeCameraSensitivity {"game.freeCameraSensitivity", 1.0f},
        .debugFlyCam {"game.debugFlyCam", false},
        .debugFlyCamLockEvents {"game.debugFlyCamLockEvents", true},
        .allowBackgroundInput {"game.allowBackgroundInput", true},
        .controllerStyle {"game.controllerStyle", ControllerStyle::GameCube},
        .enableTouchControls {"game.enableTouchControls", kDefaultTouchControlsEnabled},
        .touchControlsPreset {"game.touchControlsPreset", ControllerOverlayLayout::GameCube},
        .touchControlsScale {"game.touchControlsScale", 1.0f},
        .touchControlsOpacity {"game.touchControlsOpacity", 0.72f},
        .touchControlsEditMode {"game.touchControlsEditMode", false},

        // Cheats
        .infiniteHearts {"game.infiniteHearts", false},
        .infiniteArrows {"game.infiniteArrows", false},
        .infiniteSeeds {"game.infiniteSeeds", false},
        .infiniteBombs {"game.infiniteBombs", false},
        .infiniteOil {"game.infiniteOil", false},
        .infiniteOxygen {"game.infiniteOxygen", false},
        .infiniteRupees {"game.infiniteRupees", false},
        .enableIndefiniteItemDrops {"game.enableIndefiniteItemDrops", false},
        .moonJump {"game.moonJump", false},
        .superClawshot {"game.superClawshot", false},
        .alwaysGreatspin {"game.alwaysGreatspin", false},
        .enableFastIronBoots {"game.enableFastIronBoots", false},
        .canTransformAnywhere {"game.canTransformAnywhere", false},
        .fastRoll {"game.fastRoll", false},
        .fastSpinner {"game.fastSpinner", false},
        .freeMagicArmor {"game.freeMagicArmor", false},
        .invincibleEnemies {"game.invincibleEnemies", false},

        // Technical
        .restoreWiiGlitches {"game.restoreWiiGlitches", false},

        // Controls
        .enableTurboKeybind {"game.enableTurboKeybind", false},
        .enableResetKeybind {"game.enableResetKeybind", false},
        .inputViewerLayout {"game.inputViewerLayout", ControllerOverlayLayout::GameCube},
        .inputViewerScale {"game.inputViewerScale", 1.0f},
        .hudButtonBackground {"game.hudButtonBackground", true},
        .hudRoundXYButtons {"game.hudRoundXYButtons", true},
        .hudButtonEditTarget {"game.hudButtonEditTarget", 0},
        .hudButtonBackgroundOffsetX {"game.hudButtonBackgroundOffsetX", 0.0f},
        .hudButtonBackgroundOffsetY {"game.hudButtonBackgroundOffsetY", 0.0f},
        .hudButtonBackgroundScale {"game.hudButtonBackgroundScale", 1.0f},
        .hudButtonAOffsetX {"game.hudButtonAOffsetX", 0.0f},
        .hudButtonAOffsetY {"game.hudButtonAOffsetY", 0.0f},
        .hudButtonAScale {"game.hudButtonAScale", 1.0f},
        .hudButtonATextAnchor {"game.hudButtonATextAnchor", 0},
        .hudButtonATextScale {"game.hudButtonATextScale", 1.0f},
        .hudButtonBOffsetX {"game.hudButtonBOffsetX", 0.0f},
        .hudButtonBOffsetY {"game.hudButtonBOffsetY", 0.0f},
        .hudButtonBScale {"game.hudButtonBScale", 1.0f},
        .hudButtonBItemAnchor {"game.hudButtonBItemAnchor", 1},
        .hudButtonBItemOffsetX {"game.hudButtonBItemOffsetX", 0.0f},
        .hudButtonBItemOffsetY {"game.hudButtonBItemOffsetY", 0.0f},
        .hudButtonBItemScale {"game.hudButtonBItemScale", 1.0f},
        .hudButtonBTextAnchor {"game.hudButtonBTextAnchor", 0},
        .hudButtonBTextScale {"game.hudButtonBTextScale", 1.0f},
        .hudButtonXOffsetX {"game.hudButtonXOffsetX", 0.0f},
        .hudButtonXOffsetY {"game.hudButtonXOffsetY", 0.0f},
        .hudButtonXScale {"game.hudButtonXScale", 1.0f},
        .hudButtonXItemAnchor {"game.hudButtonXItemAnchor", 1},
        .hudButtonXItemOffsetX {"game.hudButtonXItemOffsetX", 0.0f},
        .hudButtonXItemOffsetY {"game.hudButtonXItemOffsetY", 0.0f},
        .hudButtonXItemScale {"game.hudButtonXItemScale", 1.0f},
        .hudButtonXAmmoOffsetX {"game.hudButtonXAmmoOffsetX", 0.0f},
        .hudButtonXAmmoOffsetY {"game.hudButtonXAmmoOffsetY", 0.0f},
        .hudButtonXAmmoScale {"game.hudButtonXAmmoScale", 1.0f},
        .hudButtonXTextAnchor {"game.hudButtonXTextAnchor", 0},
        .hudButtonXTextScale {"game.hudButtonXTextScale", 1.0f},
        .hudButtonYOffsetX {"game.hudButtonYOffsetX", 0.0f},
        .hudButtonYOffsetY {"game.hudButtonYOffsetY", 0.0f},
        .hudButtonYScale {"game.hudButtonYScale", 1.0f},
        .hudButtonYItemAnchor {"game.hudButtonYItemAnchor", 0},
        .hudButtonYItemOffsetX {"game.hudButtonYItemOffsetX", 0.0f},
        .hudButtonYItemOffsetY {"game.hudButtonYItemOffsetY", 0.0f},
        .hudButtonYItemScale {"game.hudButtonYItemScale", 1.0f},
        .hudButtonYAmmoOffsetX {"game.hudButtonYAmmoOffsetX", 0.0f},
        .hudButtonYAmmoOffsetY {"game.hudButtonYAmmoOffsetY", 0.0f},
        .hudButtonYAmmoScale {"game.hudButtonYAmmoScale", 1.0f},
        .hudButtonYTextAnchor {"game.hudButtonYTextAnchor", 0},
        .hudButtonYTextScale {"game.hudButtonYTextScale", 1.0f},
        .hudButtonZOffsetX {"game.hudButtonZOffsetX", 0.0f},
        .hudButtonZOffsetY {"game.hudButtonZOffsetY", 0.0f},
        .hudButtonZScale {"game.hudButtonZScale", 1.0f},
        .hudButtonZItemOffsetX {"game.hudButtonZItemOffsetX", 0.0f},
        .hudButtonZItemOffsetY {"game.hudButtonZItemOffsetY", 0.0f},
        .hudButtonZItemScale {"game.hudButtonZItemScale", 1.0f},
        .hudButtonZAmmoOffsetX {"game.hudButtonZAmmoOffsetX", 0.0f},
        .hudButtonZAmmoOffsetY {"game.hudButtonZAmmoOffsetY", 0.0f},
        .hudButtonZAmmoScale {"game.hudButtonZAmmoScale", 1.0f},
        .hudButtonZTextScale {"game.hudButtonZTextScale", 1.0f},
        .hudMidnaOffsetX {"game.hudMidnaOffsetX", 0.0f},
        .hudMidnaOffsetY {"game.hudMidnaOffsetY", 0.0f},
        .hudMidnaScale {"game.hudMidnaScale", 1.0f},
        .hudHeartsOffsetX {"game.hudHeartsOffsetX", 0.0f},
        .hudHeartsOffsetY {"game.hudHeartsOffsetY", 0.0f},
        .hudHeartsScale {"game.hudHeartsScale", 1.0f},
        .hudRupeesOffsetX {"game.hudRupeesOffsetX", 0.0f},
        .hudRupeesOffsetY {"game.hudRupeesOffsetY", 0.0f},
        .hudRupeesScale {"game.hudRupeesScale", 1.0f},
        .hudKeysOffsetX {"game.hudKeysOffsetX", 0.0f},
        .hudKeysOffsetY {"game.hudKeysOffsetY", 0.0f},
        .hudKeysScale {"game.hudKeysScale", 1.0f},
        .hudOilOffsetX {"game.hudOilOffsetX", 0.0f},
        .hudOilOffsetY {"game.hudOilOffsetY", 0.0f},
        .hudOilScale {"game.hudOilScale", 1.0f},
        .hudDPadOffsetX {"game.hudDPadOffsetX", 0.0f},
        .hudDPadOffsetY {"game.hudDPadOffsetY", 0.0f},
        .hudDPadScale {"game.hudDPadScale", 1.0f},
        .hudDPadFollowMinimap {"game.hudDPadFollowMinimap", true},
        .hudMinimapOffsetX {"game.hudMinimapOffsetX", 0.0f},
        .hudMinimapOffsetY {"game.hudMinimapOffsetY", 0.0f},
        .hudMinimapScale {"game.hudMinimapScale", 1.0f},
        .hudMinimapSlideDirection {"game.hudMinimapSlideDirection",
                                   MinimapSlideDirection::LeftToRight},

        // Tools
        .speedrunMode {"game.speedrunMode", false},
        .liveSplitEnabled {"game.liveSplitEnabled", false},
        .showSpeedrunRTATimer {"game.showSpeedrunRTATimer", true},
        .recordingMode {"game.recordingMode", false},
        .showInputViewer {"game.showInputViewer", false},
        .showInputViewerGyro {"game.showInputViewerGyro", false}
    },

    .backend = {
        .isoPath {"backend.isoPath", ""},
        .isoVerification {"backend.isoVerification", DiscVerificationState::Unknown},
        .graphicsBackend {"backend.graphicsBackend", "auto"},
        .skipPreLaunchUI {"backend.skipPreLaunchUI", false},
        .showPipelineCompilation {"backend.showPipelineCompilation", false},
        .wasPresetChosen {"backend.wasPresetChosen", false},
        .checkForUpdates {"backend.checkForUpdates", true},
        .cardFileType {"backend.cardFileType", static_cast<int>(CARD_GCIFOLDER)},
        .enableAdvancedSettings {"backend.enableAdvancedSettings", false},
    },

    // Not sure if there's a better way to declare this
    .actionBindings = {
        .firstPersonCamera {
            ActionBindConfigVar{"actionBindings.firstPersonCamera_port0", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.firstPersonCamera_port1", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.firstPersonCamera_port2", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.firstPersonCamera_port3", PAD_NATIVE_BUTTON_INVALID},
        },
        .callMidna {
            ActionBindConfigVar{"actionBindings.callMidna_port0", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.callMidna_port1", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.callMidna_port2", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.callMidna_port3", PAD_NATIVE_BUTTON_INVALID},
        },
        .openDawnlightMenu {
            ActionBindConfigVar{"actionBindings.openDawnlightMenu_port0", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.openDawnlightMenu_port1", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.openDawnlightMenu_port2", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.openDawnlightMenu_port3", PAD_NATIVE_BUTTON_INVALID},
        },
        .turboSpeedButton {
            ActionBindConfigVar{"actionBindings.turboButton_port0", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.turboButton_port1", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.turboButton_port2", PAD_NATIVE_BUTTON_INVALID},
            ActionBindConfigVar{"actionBindings.turboButton_port3", PAD_NATIVE_BUTTON_INVALID},
        },
    }
};

UserSettings& getSettings() {
    return g_userSettings;
}

AimMode GetAimMode() noexcept {
#if TARGET_PC
    auto& settings = getSettings().game;
    if (settings.aimMode.getLayer() == config::ConfigVarLayer::Default &&
        settings.enableThirdPersonAim.getValue())
    {
        return AimMode::ThirdPerson;
    }

    return settings.aimMode.getValue();
#else
    return AimMode::Vanilla;
#endif
}

const char* AimModeName(AimMode mode) noexcept {
    switch (mode) {
    case AimMode::ThirdPerson:
        return "3rd Person";
    case AimMode::Cinema:
        return "Cinema";
    case AimMode::Vanilla:
    default:
        return "Vanilla";
    }
}

bool UseThirdPersonAim() noexcept {
    return GetAimMode() == AimMode::ThirdPerson;
}

bool UseCinemaAim() noexcept {
    return GetAimMode() == AimMode::Cinema;
}

const char* ControllerStyleName(ControllerStyle style) noexcept {
    switch (style) {
    case ControllerStyle::WiiU:
        return "Wii U";
    case ControllerStyle::GameCube:
    default:
        return "GameCube";
    }
}

bool UseWiiUControllerStyle() noexcept {
#if TARGET_PC
    auto& controllerStyle = getSettings().game.controllerStyle;
    if (controllerStyle.getLayer() == config::ConfigVarLayer::Default) {
        return false;
    }

    return controllerStyle.getValue() == ControllerStyle::WiiU;
#else
    return false;
#endif
}

const char* MinimapSlideDirectionName(MinimapSlideDirection direction) noexcept {
    switch (direction) {
    case MinimapSlideDirection::RightToLeft:
        return "Right -> Left";
    case MinimapSlideDirection::LeftToRight:
    default:
        return "Left -> Right";
    }
}

void registerSettings() {
    // Video
    Register(g_userSettings.video.enableFullscreen);
    Register(g_userSettings.video.enableVsync);
    Register(g_userSettings.video.lockAspectRatio);
    Register(g_userSettings.video.enableFpsOverlay);
    Register(g_userSettings.video.fpsOverlayCorner);
    Register(g_userSettings.video.maxFrameRate);

    // Audio
    Register(g_userSettings.audio.masterVolume);
    Register(g_userSettings.audio.mainMusicVolume);
    Register(g_userSettings.audio.subMusicVolume);
    Register(g_userSettings.audio.soundEffectsVolume);
    Register(g_userSettings.audio.fanfareVolume);
    Register(g_userSettings.audio.enableReverb);
    Register(g_userSettings.audio.enableHrtf);
    Register(g_userSettings.audio.menuSounds);

    // Game
    Register(g_userSettings.game.language);
    Register(g_userSettings.game.enableQuickTransform);
    Register(g_userSettings.game.hideTvSettingsScreen);
    Register(g_userSettings.game.biggerWallets);
    Register(g_userSettings.game.noReturnRupees);
    Register(g_userSettings.game.disableRupeeCutscenes);
    Register(g_userSettings.game.noSwordRecoil);
    Register(g_userSettings.game.enableJumpButton);
    Register(g_userSettings.game.manualShielding);
    Register(g_userSettings.game.damageMultiplier);
    Register(g_userSettings.game.newGamePlusHealthScalePercent);
    Register(g_userSettings.game.noHeartDrops);
    Register(g_userSettings.game.instantDeath);
    Register(g_userSettings.game.fastClimbing);
    Register(g_userSettings.game.fastTears);
    Register(g_userSettings.game.no2ndFishForCat);
    Register(g_userSettings.game.instantSaves);
    Register(g_userSettings.game.instantText);
    Register(g_userSettings.game.sunsSong);
    Register(g_userSettings.game.autoSave);
    Register(g_userSettings.game.enableMirrorMode);
    Register(g_userSettings.game.invertCameraXAxis);
    Register(g_userSettings.game.invertCameraYAxis);
    Register(g_userSettings.game.invertFirstPersonXAxis);
    Register(g_userSettings.game.invertFirstPersonYAxis);
    Register(g_userSettings.game.enableAimMovement);
    Register(g_userSettings.game.aimMode);
    Register(g_userSettings.game.enableThirdPersonAim);
    Register(g_userSettings.game.freeCameraSensitivity);
    Register(g_userSettings.game.minimalHUD);
    Register(g_userSettings.game.pauseOnFocusLost);
    Register(g_userSettings.game.enableDiscordPresence);
    Register(g_userSettings.game.bloomMode);
    Register(g_userSettings.game.bloomMultiplier);
    Register(g_userSettings.game.disableWaterRefraction);
    Register(g_userSettings.game.enableTextureReplacements);
    Register(g_userSettings.game.internalResolutionScale);
    Register(g_userSettings.game.resampler);
    Register(g_userSettings.game.shadowResolutionMultiplier);
    Register(g_userSettings.game.enableDepthOfField);
    Register(g_userSettings.game.enableMapBackground);
    Register(g_userSettings.game.disableCutscenePillarboxing);
    Register(g_userSettings.game.enableFastIronBoots);
    Register(g_userSettings.game.canTransformAnywhere);
    Register(g_userSettings.game.fastRoll);
    Register(g_userSettings.game.freeMagicArmor);
    Register(g_userSettings.game.restoreWiiGlitches);
    Register(g_userSettings.game.enableLinkDollRotation);
    Register(g_userSettings.game.enableAchievementToasts);
    Register(g_userSettings.game.enableControllerToasts);
    Register(g_userSettings.game.noMissClimbing);
    Register(g_userSettings.game.noLowHpSound);
    Register(g_userSettings.game.midnasLamentNonStop);
    Register(g_userSettings.game.enableTurboKeybind);
    Register(g_userSettings.game.enableResetKeybind);
    Register(g_userSettings.game.speedrunMode);
    Register(g_userSettings.game.liveSplitEnabled);
    Register(g_userSettings.game.showSpeedrunRTATimer);
    Register(g_userSettings.game.recordingMode);
    Register(g_userSettings.game.showInputViewer);
    Register(g_userSettings.game.showInputViewerGyro);
    Register(g_userSettings.game.fastSpinner);
    Register(g_userSettings.game.infiniteHearts);
    Register(g_userSettings.game.infiniteArrows);
    Register(g_userSettings.game.infiniteSeeds);
    Register(g_userSettings.game.infiniteBombs);
    Register(g_userSettings.game.infiniteOil);
    Register(g_userSettings.game.infiniteOxygen);
    Register(g_userSettings.game.infiniteRupees);
    Register(g_userSettings.game.enableIndefiniteItemDrops);
    Register(g_userSettings.game.moonJump);
    Register(g_userSettings.game.superClawshot);
    Register(g_userSettings.game.alwaysGreatspin);
    Register(g_userSettings.game.invincibleEnemies);
    Register(g_userSettings.game.enableFrameInterpolation);
    Register(g_userSettings.game.gyroMode);
    Register(g_userSettings.game.enableGyroAim);
    Register(g_userSettings.game.enableGyroRollgoal);
    Register(g_userSettings.game.gyroSensitivityX);
    Register(g_userSettings.game.gyroSensitivityY);
    Register(g_userSettings.game.gyroSensitivityRollgoal);
    Register(g_userSettings.game.gyroDeadband);
    Register(g_userSettings.game.gyroSmoothing);
    Register(g_userSettings.game.gyroInvertPitch);
    Register(g_userSettings.game.gyroInvertYaw);
    Register(g_userSettings.game.freeCamera);
    Register(g_userSettings.game.debugFlyCam);
    Register(g_userSettings.game.debugFlyCamLockEvents);
    Register(g_userSettings.game.allowBackgroundInput);
    Register(g_userSettings.game.controllerStyle);
    Register(g_userSettings.game.enableTouchControls);
    Register(g_userSettings.game.touchControlsPreset);
    Register(g_userSettings.game.touchControlsScale);
    Register(g_userSettings.game.touchControlsOpacity);
    Register(g_userSettings.game.touchControlsEditMode);
    Register(g_userSettings.game.inputViewerLayout);
    Register(g_userSettings.game.inputViewerScale);
    Register(g_userSettings.game.hudButtonBackground);
    Register(g_userSettings.game.hudRoundXYButtons);
    Register(g_userSettings.game.hudButtonEditTarget);
    Register(g_userSettings.game.hudButtonBackgroundOffsetX);
    Register(g_userSettings.game.hudButtonBackgroundOffsetY);
    Register(g_userSettings.game.hudButtonBackgroundScale);
    Register(g_userSettings.game.hudButtonAOffsetX);
    Register(g_userSettings.game.hudButtonAOffsetY);
    Register(g_userSettings.game.hudButtonAScale);
    Register(g_userSettings.game.hudButtonATextAnchor);
    Register(g_userSettings.game.hudButtonATextScale);
    Register(g_userSettings.game.hudButtonBOffsetX);
    Register(g_userSettings.game.hudButtonBOffsetY);
    Register(g_userSettings.game.hudButtonBScale);
    Register(g_userSettings.game.hudButtonBItemAnchor);
    Register(g_userSettings.game.hudButtonBItemOffsetX);
    Register(g_userSettings.game.hudButtonBItemOffsetY);
    Register(g_userSettings.game.hudButtonBItemScale);
    Register(g_userSettings.game.hudButtonBTextAnchor);
    Register(g_userSettings.game.hudButtonBTextScale);
    Register(g_userSettings.game.hudButtonXOffsetX);
    Register(g_userSettings.game.hudButtonXOffsetY);
    Register(g_userSettings.game.hudButtonXScale);
    Register(g_userSettings.game.hudButtonXItemAnchor);
    Register(g_userSettings.game.hudButtonXItemOffsetX);
    Register(g_userSettings.game.hudButtonXItemOffsetY);
    Register(g_userSettings.game.hudButtonXItemScale);
    Register(g_userSettings.game.hudButtonXAmmoOffsetX);
    Register(g_userSettings.game.hudButtonXAmmoOffsetY);
    Register(g_userSettings.game.hudButtonXAmmoScale);
    Register(g_userSettings.game.hudButtonXTextAnchor);
    Register(g_userSettings.game.hudButtonXTextScale);
    Register(g_userSettings.game.hudButtonYOffsetX);
    Register(g_userSettings.game.hudButtonYOffsetY);
    Register(g_userSettings.game.hudButtonYScale);
    Register(g_userSettings.game.hudButtonYItemAnchor);
    Register(g_userSettings.game.hudButtonYItemOffsetX);
    Register(g_userSettings.game.hudButtonYItemOffsetY);
    Register(g_userSettings.game.hudButtonYItemScale);
    Register(g_userSettings.game.hudButtonYAmmoOffsetX);
    Register(g_userSettings.game.hudButtonYAmmoOffsetY);
    Register(g_userSettings.game.hudButtonYAmmoScale);
    Register(g_userSettings.game.hudButtonYTextAnchor);
    Register(g_userSettings.game.hudButtonYTextScale);
    Register(g_userSettings.game.hudButtonZOffsetX);
    Register(g_userSettings.game.hudButtonZOffsetY);
    Register(g_userSettings.game.hudButtonZScale);
    Register(g_userSettings.game.hudButtonZItemOffsetX);
    Register(g_userSettings.game.hudButtonZItemOffsetY);
    Register(g_userSettings.game.hudButtonZItemScale);
    Register(g_userSettings.game.hudButtonZAmmoOffsetX);
    Register(g_userSettings.game.hudButtonZAmmoOffsetY);
    Register(g_userSettings.game.hudButtonZAmmoScale);
    Register(g_userSettings.game.hudButtonZTextScale);
    Register(g_userSettings.game.hudMidnaOffsetX);
    Register(g_userSettings.game.hudMidnaOffsetY);
    Register(g_userSettings.game.hudMidnaScale);
    Register(g_userSettings.game.hudHeartsOffsetX);
    Register(g_userSettings.game.hudHeartsOffsetY);
    Register(g_userSettings.game.hudHeartsScale);
    Register(g_userSettings.game.hudRupeesOffsetX);
    Register(g_userSettings.game.hudRupeesOffsetY);
    Register(g_userSettings.game.hudRupeesScale);
    Register(g_userSettings.game.hudKeysOffsetX);
    Register(g_userSettings.game.hudKeysOffsetY);
    Register(g_userSettings.game.hudKeysScale);
    Register(g_userSettings.game.hudOilOffsetX);
    Register(g_userSettings.game.hudOilOffsetY);
    Register(g_userSettings.game.hudOilScale);
    Register(g_userSettings.game.hudDPadOffsetX);
    Register(g_userSettings.game.hudDPadOffsetY);
    Register(g_userSettings.game.hudDPadScale);
    Register(g_userSettings.game.hudDPadFollowMinimap);
    Register(g_userSettings.game.hudMinimapOffsetX);
    Register(g_userSettings.game.hudMinimapOffsetY);
    Register(g_userSettings.game.hudMinimapScale);
    Register(g_userSettings.game.hudMinimapSlideDirection);

    Register(g_userSettings.backend.isoPath);
    Register(g_userSettings.backend.isoVerification);
    Register(g_userSettings.backend.graphicsBackend);
    Register(g_userSettings.backend.skipPreLaunchUI);
    Register(g_userSettings.backend.showPipelineCompilation);
    Register(g_userSettings.backend.wasPresetChosen);
    Register(g_userSettings.backend.checkForUpdates);
    Register(g_userSettings.backend.cardFileType);
    Register(g_userSettings.backend.enableAdvancedSettings);

    Register(g_userSettings.actionBindings.firstPersonCamera[0]);
    Register(g_userSettings.actionBindings.firstPersonCamera[1]);
    Register(g_userSettings.actionBindings.firstPersonCamera[2]);
    Register(g_userSettings.actionBindings.firstPersonCamera[3]);
    Register(g_userSettings.actionBindings.callMidna[0]);
    Register(g_userSettings.actionBindings.callMidna[1]);
    Register(g_userSettings.actionBindings.callMidna[2]);
    Register(g_userSettings.actionBindings.callMidna[3]);
    Register(g_userSettings.actionBindings.openDawnlightMenu[0]);
    Register(g_userSettings.actionBindings.openDawnlightMenu[1]);
    Register(g_userSettings.actionBindings.openDawnlightMenu[2]);
    Register(g_userSettings.actionBindings.openDawnlightMenu[3]);
    Register(g_userSettings.actionBindings.turboSpeedButton[0]);
    Register(g_userSettings.actionBindings.turboSpeedButton[1]);
    Register(g_userSettings.actionBindings.turboSpeedButton[2]);
    Register(g_userSettings.actionBindings.turboSpeedButton[3]);
}

// Transient settings

static TransientSettings g_transientSettings = {
    .collisionView = {
        .enableTerrainView = false,
        .enableWireframe = false,
        .enableAtView = false,
        .enableTgView = false,
        .enableCoView = false,
        .terrainViewOpacity = 50.0f,
        .colliderViewOpacity = 50.0f,
        .drawRange = 100.0f,
    },
    .skipFrameRateLimit = false,
};

TransientSettings& getTransientSettings() {
    return g_transientSettings;
}

}
