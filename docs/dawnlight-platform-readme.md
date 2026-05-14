# Dawnlight Platform README

Dawnlight is based on Dusklight and keeps the upstream app identity internally for compatibility. This means saves, settings, and data folders still use `TwilitRealm/Dusklight` paths.

Most Dawnlight patches are platform-independent because they live in the shared game and engine code. This includes the HUD layout editor, Wii U controller style, ZL jump, aim modes, New Game+, intro skip, NG+ save markers, and NG+ scaling. Android-specific code is only used for Android file picker support and Android URI import/export handling. Touch controls are shared code, but they are enabled by default only on Android and iOS.

## Platform Setup

### Android

1. Install the Android APK.
2. Launch Dawnlight.
3. Select a supported Twilight Princess GameCube disc image.
4. Open Settings to configure touch controls, HUD layout, controller style, aim mode, and New Game+ options.

HUD layout import:

- Put `hud_layout_settings.json` anywhere the Android file picker can access, for example `Downloads` or `Documents`.
- In Dawnlight, open `Settings -> Overlay -> Import HUD Layout`.
- Select the JSON file.

The Android app data folder is private to the app, so manually copying the file into the internal config folder is not recommended. Use the in-app import button instead.

### iOS

1. Install the IPA with AltStore, Sideloadly, TrollStore, or another sideloading method.
2. Launch Dawnlight.
3. Select a supported Twilight Princess GameCube disc image.
4. Configure settings in-game.

HUD layout import:

- Put `hud_layout_settings.json` in the Files app, for example in iCloud Drive or On My iPhone/iPad.
- In Dawnlight, open `Settings -> Overlay -> Import HUD Layout`.
- Select the JSON file.

The default iOS data location is the app's Documents folder. HUD layout export is currently not supported by the iOS save dialog, so use import or desktop/Android export when sharing layouts.

### Windows

1. Download `Dawnlight-*-win32-x86_64.zip`.
2. Extract the ZIP.
3. Run `dusklight.exe`.
4. Select a supported Twilight Princess GameCube disc image.

HUD layout import:

- Recommended: place `hud_layout_settings.json` anywhere convenient and import it with `Settings -> Overlay -> Import HUD Layout`.
- Default data folder: `%APPDATA%\TwilitRealm\Dusklight\`
- If you use a custom data folder in Dawnlight, use that folder instead.

### Linux

1. Download `Dawnlight-*-linux-x86_64.AppImage`.
2. Make it executable, for example: `chmod +x Dawnlight-*-linux-x86_64.AppImage`.
3. Run the AppImage.
4. Select a supported Twilight Princess GameCube disc image.

HUD layout import:

- Recommended: place `hud_layout_settings.json` anywhere convenient and import it with `Settings -> Overlay -> Import HUD Layout`.
- Default data folder: `$XDG_DATA_HOME/TwilitRealm/Dusklight/`
- If `XDG_DATA_HOME` is not set, the usual fallback is `~/.local/share/TwilitRealm/Dusklight/`.
- If you use a custom data folder in Dawnlight, use that folder instead.

### macOS

1. Download the correct ZIP:
   - Apple Silicon: `Dawnlight-*-macos-arm64.zip`
   - Intel Mac: `Dawnlight-*-macos-x86_64.zip`
2. Extract the ZIP.
3. Open `Dusklight.app`. If macOS blocks it, right-click the app and choose Open.
4. Select a supported Twilight Princess GameCube disc image.

HUD layout import:

- Recommended: place `hud_layout_settings.json` anywhere convenient and import it with `Settings -> Overlay -> Import HUD Layout`.
- Default data folder: `~/Library/Application Support/TwilitRealm/Dusklight/`
- If you use a custom data folder in Dawnlight, use that folder instead.

## HUD Layout Files

`hud_layout_settings.json` is an import/export file, not a file that Dawnlight automatically loads just because it exists in the data folder. To apply it, always use:

`Settings -> Overlay -> Import HUD Layout`

To create or share one, use:

`Settings -> Overlay -> Export HUD Layout`

On desktop and Android, the export dialog suggests the filename `hud_layout_settings.json`. On iOS, exporting is currently limited by the missing save dialog support.
