#ifndef DUSK_CONFIG_H
#define DUSK_CONFIG_H

#include <array>

#include "dusk/config_var.hpp"

namespace dusk {

using namespace config;

enum class BloomMode : int {
    Off = 0,
    Classic = 1,
    Dusk = 2,
};

enum class GameLanguage : u8 {
    English = OS_LANGUAGE_ENGLISH,
    German = OS_LANGUAGE_GERMAN,
    French = OS_LANGUAGE_FRENCH,
    Spanish = OS_LANGUAGE_SPANISH,
    Italian = OS_LANGUAGE_ITALIAN,
};

enum class DiscVerificationState : u8 {
    Unknown = 0,
    Success,
    HashMismatch,
};

enum class GyroMode : u8 {
    Sensor = 0,
    Mouse = 1,
};

enum class AimMode : u8 {
    Vanilla = 0,
    ThirdPerson = 1,
    Cinema = 2,
};

enum class ControllerOverlayLayout : u8 {
    GameCube = 0,
    WiiU = 1,
    XBox = 2,
};

enum class ControllerStyle : u8 {
    GameCube = 0,
    WiiU = 1,
};

enum class MinimapSlideDirection : u8 {
    LeftToRight = 0,
    RightToLeft = 1,
};

namespace config {
template <>
struct ConfigEnumRange<BloomMode> {
    static constexpr auto min = BloomMode::Off;
    static constexpr auto max = BloomMode::Dusk;
};

template <>
struct ConfigEnumRange<GameLanguage> {
    static constexpr auto min = GameLanguage::English;
    static constexpr auto max = GameLanguage::Italian;
};

template <>
struct ConfigEnumRange<DiscVerificationState> {
    static constexpr auto min = DiscVerificationState::Unknown;
    static constexpr auto max = DiscVerificationState::HashMismatch;
};

template <>
struct ConfigEnumRange<GyroMode> {
    static constexpr auto min = GyroMode::Sensor;
    static constexpr auto max = GyroMode::Mouse;
};

template <>
struct ConfigEnumRange<AimMode> {
    static constexpr auto min = AimMode::Vanilla;
    static constexpr auto max = AimMode::Cinema;
};

template <>
struct ConfigEnumRange<ControllerOverlayLayout> {
    static constexpr auto min = ControllerOverlayLayout::GameCube;
    static constexpr auto max = ControllerOverlayLayout::XBox;
};

template <>
struct ConfigEnumRange<ControllerStyle> {
    static constexpr auto min = ControllerStyle::GameCube;
    static constexpr auto max = ControllerStyle::WiiU;
};

template <>
struct ConfigEnumRange<MinimapSlideDirection> {
    static constexpr auto min = MinimapSlideDirection::LeftToRight;
    static constexpr auto max = MinimapSlideDirection::RightToLeft;
};
}

// Persistent user settings

struct UserSettings {
    // Program settings

    struct {
        // Video
        ConfigVar<bool> enableFullscreen;
        ConfigVar<bool> enableVsync;
        ConfigVar<bool> lockAspectRatio;
        ConfigVar<bool> enableFpsOverlay;
        ConfigVar<int> fpsOverlayCorner;
    } video;

    struct {
        // Audio
        ConfigVar<int> masterVolume;
        ConfigVar<int> mainMusicVolume;
        ConfigVar<int> subMusicVolume;
        ConfigVar<int> soundEffectsVolume;
        ConfigVar<int> fanfareVolume;
        ConfigVar<bool> enableReverb;
        ConfigVar<bool> enableHrtf;
        ConfigVar<bool> menuSounds;
    } audio;

    // Game settings

    struct {
        ConfigVar<GameLanguage> language;

        // QoL
        ConfigVar<bool> enableQuickTransform;
        ConfigVar<bool> hideTvSettingsScreen;
        ConfigVar<bool> biggerWallets;
        ConfigVar<bool> noReturnRupees;
        ConfigVar<bool> disableRupeeCutscenes;
        ConfigVar<bool> noSwordRecoil;
        ConfigVar<bool> enableJumpButton;
        ConfigVar<int> damageMultiplier;
        ConfigVar<int> newGamePlusHealthScalePercent;
        ConfigVar<bool> noHeartDrops;
        ConfigVar<bool> instantDeath;
        ConfigVar<bool> fastClimbing;
        ConfigVar<bool> noMissClimbing;
        ConfigVar<bool> fastTears;
        ConfigVar<bool> no2ndFishForCat;
        ConfigVar<bool> instantSaves;
        ConfigVar<bool> instantText;
        ConfigVar<bool> sunsSong;
        ConfigVar<bool> autoSave;

        // Preferences
        ConfigVar<bool> enableMirrorMode;
        ConfigVar<bool> minimalHUD;
        ConfigVar<bool> pauseOnFocusLost;
        ConfigVar<bool> enableLinkDollRotation;
        ConfigVar<bool> enableAchievementToasts;
        ConfigVar<bool> enableControllerToasts;
        ConfigVar<bool> enableDiscordPresence;

        // Graphics
        ConfigVar<BloomMode> bloomMode;
        ConfigVar<float> bloomMultiplier;
        ConfigVar<bool> disableWaterRefraction;
        ConfigVar<bool> enableFrameInterpolation;
        ConfigVar<int> internalResolutionScale;
        ConfigVar<int> shadowResolutionMultiplier;
        ConfigVar<bool> enableDepthOfField;
        ConfigVar<bool> enableMapBackground;
        ConfigVar<bool> disableCutscenePillarboxing;

        // Audio
        ConfigVar<bool> noLowHpSound;
        ConfigVar<bool> midnasLamentNonStop;

        // Input
        ConfigVar<GyroMode> gyroMode;
        ConfigVar<bool> enableGyroAim;
        ConfigVar<bool> enableGyroRollgoal;
        ConfigVar<float> gyroSensitivityX;
        ConfigVar<float> gyroSensitivityY;
        ConfigVar<float> gyroSensitivityRollgoal;
        ConfigVar<float> gyroSmoothing;
        ConfigVar<float> gyroDeadband;
        ConfigVar<bool> gyroInvertPitch;
        ConfigVar<bool> gyroInvertYaw;
        ConfigVar<bool> freeCamera;
        ConfigVar<bool> invertCameraXAxis;
        ConfigVar<bool> invertCameraYAxis;
        ConfigVar<bool> invertFirstPersonXAxis;
        ConfigVar<bool> invertFirstPersonYAxis;
        ConfigVar<bool> enableAimMovement;
        ConfigVar<AimMode> aimMode;
        ConfigVar<bool> enableThirdPersonAim;
        ConfigVar<float> freeCameraSensitivity;
        ConfigVar<bool> debugFlyCam;
        ConfigVar<bool> debugFlyCamLockEvents;
        ConfigVar<bool> allowBackgroundInput;
        ConfigVar<ControllerStyle> controllerStyle;
        ConfigVar<bool> enableTouchControls;
        ConfigVar<ControllerOverlayLayout> touchControlsPreset;
        ConfigVar<float> touchControlsScale;
        ConfigVar<float> touchControlsOpacity;
        ConfigVar<bool> touchControlsEditMode;

        // Cheats
        ConfigVar<bool> infiniteHearts;
        ConfigVar<bool> infiniteArrows;
        ConfigVar<bool> infiniteSeeds;
        ConfigVar<bool> infiniteBombs;
        ConfigVar<bool> infiniteOil;
        ConfigVar<bool> infiniteOxygen;
        ConfigVar<bool> infiniteRupees;
        ConfigVar<bool> enableIndefiniteItemDrops;
        ConfigVar<bool> moonJump;
        ConfigVar<bool> superClawshot;
        ConfigVar<bool> alwaysGreatspin;
        ConfigVar<bool> enableFastIronBoots;
        ConfigVar<bool> canTransformAnywhere;
        ConfigVar<bool> fastRoll;
        ConfigVar<bool> fastSpinner;
        ConfigVar<bool> freeMagicArmor;
        ConfigVar<bool> invincibleEnemies;

        // Technical
        ConfigVar<bool> restoreWiiGlitches;

        // Controls
        ConfigVar<bool> enableTurboKeybind;
        ConfigVar<bool> enableResetKeybind;
        ConfigVar<ControllerOverlayLayout> inputViewerLayout;
        ConfigVar<float> inputViewerScale;
        ConfigVar<bool> hudButtonBackground;
        ConfigVar<bool> hudRoundXYButtons;
        ConfigVar<int> hudButtonEditTarget;
        ConfigVar<float> hudButtonBackgroundOffsetX;
        ConfigVar<float> hudButtonBackgroundOffsetY;
        ConfigVar<float> hudButtonBackgroundScale;
        ConfigVar<float> hudButtonAOffsetX;
        ConfigVar<float> hudButtonAOffsetY;
        ConfigVar<float> hudButtonAScale;
        ConfigVar<int> hudButtonATextAnchor;
        ConfigVar<float> hudButtonATextScale;
        ConfigVar<float> hudButtonBOffsetX;
        ConfigVar<float> hudButtonBOffsetY;
        ConfigVar<float> hudButtonBScale;
        ConfigVar<int> hudButtonBItemAnchor;
        ConfigVar<float> hudButtonBItemOffsetX;
        ConfigVar<float> hudButtonBItemOffsetY;
        ConfigVar<float> hudButtonBItemScale;
        ConfigVar<int> hudButtonBTextAnchor;
        ConfigVar<float> hudButtonBTextScale;
        ConfigVar<float> hudButtonXOffsetX;
        ConfigVar<float> hudButtonXOffsetY;
        ConfigVar<float> hudButtonXScale;
        ConfigVar<int> hudButtonXItemAnchor;
        ConfigVar<float> hudButtonXItemOffsetX;
        ConfigVar<float> hudButtonXItemOffsetY;
        ConfigVar<float> hudButtonXItemScale;
        ConfigVar<float> hudButtonXAmmoOffsetX;
        ConfigVar<float> hudButtonXAmmoOffsetY;
        ConfigVar<float> hudButtonXAmmoScale;
        ConfigVar<int> hudButtonXTextAnchor;
        ConfigVar<float> hudButtonXTextScale;
        ConfigVar<float> hudButtonYOffsetX;
        ConfigVar<float> hudButtonYOffsetY;
        ConfigVar<float> hudButtonYScale;
        ConfigVar<int> hudButtonYItemAnchor;
        ConfigVar<float> hudButtonYItemOffsetX;
        ConfigVar<float> hudButtonYItemOffsetY;
        ConfigVar<float> hudButtonYItemScale;
        ConfigVar<float> hudButtonYAmmoOffsetX;
        ConfigVar<float> hudButtonYAmmoOffsetY;
        ConfigVar<float> hudButtonYAmmoScale;
        ConfigVar<int> hudButtonYTextAnchor;
        ConfigVar<float> hudButtonYTextScale;
        ConfigVar<float> hudButtonZOffsetX;
        ConfigVar<float> hudButtonZOffsetY;
        ConfigVar<float> hudButtonZScale;
        ConfigVar<float> hudButtonZItemOffsetX;
        ConfigVar<float> hudButtonZItemOffsetY;
        ConfigVar<float> hudButtonZItemScale;
        ConfigVar<float> hudButtonZAmmoOffsetX;
        ConfigVar<float> hudButtonZAmmoOffsetY;
        ConfigVar<float> hudButtonZAmmoScale;
        ConfigVar<float> hudButtonZTextScale;
        ConfigVar<float> hudMidnaOffsetX;
        ConfigVar<float> hudMidnaOffsetY;
        ConfigVar<float> hudMidnaScale;
        ConfigVar<float> hudHeartsOffsetX;
        ConfigVar<float> hudHeartsOffsetY;
        ConfigVar<float> hudHeartsScale;
        ConfigVar<float> hudRupeesOffsetX;
        ConfigVar<float> hudRupeesOffsetY;
        ConfigVar<float> hudRupeesScale;
        ConfigVar<float> hudKeysOffsetX;
        ConfigVar<float> hudKeysOffsetY;
        ConfigVar<float> hudKeysScale;
        ConfigVar<float> hudOilOffsetX;
        ConfigVar<float> hudOilOffsetY;
        ConfigVar<float> hudOilScale;
        ConfigVar<float> hudDPadOffsetX;
        ConfigVar<float> hudDPadOffsetY;
        ConfigVar<float> hudDPadScale;
        ConfigVar<bool> hudDPadFollowMinimap;
        ConfigVar<float> hudMinimapOffsetX;
        ConfigVar<float> hudMinimapOffsetY;
        ConfigVar<float> hudMinimapScale;
        ConfigVar<MinimapSlideDirection> hudMinimapSlideDirection;

        // Tools
        ConfigVar<bool> speedrunMode;
        ConfigVar<bool> liveSplitEnabled;
        ConfigVar<bool> showSpeedrunRTATimer;
        ConfigVar<bool> recordingMode;
        ConfigVar<bool> showInputViewer;
        ConfigVar<bool> showInputViewerGyro;
    } game;

    struct {
        ConfigVar<std::string> isoPath;
        ConfigVar<DiscVerificationState> isoVerification;
        ConfigVar<std::string> graphicsBackend;
        ConfigVar<bool> skipPreLaunchUI;
        ConfigVar<bool> showPipelineCompilation;
        ConfigVar<bool> wasPresetChosen;
        ConfigVar<bool> checkForUpdates;
        ConfigVar<int> cardFileType;
        ConfigVar<bool> enableAdvancedSettings;
    } backend;

    // Arrays of size 4 for 4 ports
    struct {
        std::array<ActionBindConfigVar, 4> firstPersonCamera;
        std::array<ActionBindConfigVar, 4> callMidna;
        std::array<ActionBindConfigVar, 4> openDusklightMenu;
        std::array<ActionBindConfigVar, 4> turboSpeedButton;
    } actionBindings;
};

UserSettings& getSettings();
AimMode GetAimMode() noexcept;
const char* AimModeName(AimMode mode) noexcept;
bool UseThirdPersonAim() noexcept;
bool UseCinemaAim() noexcept;
const char* ControllerStyleName(ControllerStyle style) noexcept;
bool UseWiiUControllerStyle() noexcept;
const char* MinimapSlideDirectionName(MinimapSlideDirection direction) noexcept;

void registerSettings();

// Transient settings

struct CollisionViewSettings {
    bool enableTerrainView;
    bool enableWireframe;
    bool enableAtView;
    bool enableTgView;
    bool enableCoView;
    float terrainViewOpacity;
    float colliderViewOpacity;
    float drawRange;
};

struct TransientSettings {
    CollisionViewSettings collisionView;
    bool skipFrameRateLimit;
    bool moveLinkActive;
    bool stateShareLoadActive;
};

TransientSettings& getTransientSettings();

}

#endif // DUSK_CONFIG_H
