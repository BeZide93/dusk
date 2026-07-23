# Local Build Shortcuts

These Windows batch files wrap the Dawnlight build commands so releases can be started without retyping the long CMake, Gradle, and GitHub CLI commands.

## Android

Debug APK:

```bat
tools\local-build\android-debug.bat
```

Outputs:

- `apk\debug\app-arm64-v8a-debug.apk`
- `local-builds\android\debug\app-arm64-v8a-debug.apk`

Signed release APK:

```bat
tools\local-build\android-release-signed.bat
```

Requirements:

- `keystores\release-signing.properties`
- the release keystore referenced by that properties file

Outputs:

- `apk\release\app-arm64-v8a-release.apk`
- `local-builds\android\release\app-arm64-v8a-release.apk`

Build both Android variants:

```bat
tools\local-build\android-all.bat
```

## Windows

```bat
tools\local-build\windows-msvc.bat
```

Requirements:

- Visual Studio Build Tools with MSVC
- Ninja
- vcpkg dependencies available through `VCPKG_ROOT` or `C:\vcpkg`

Output:

- `local-builds\windows\Dawnlight-win32-x86_64.zip`

## All CI Platforms

Windows cannot locally build Linux, macOS, and iOS artifacts with the same setup as GitHub Actions. Use this script to start the GitHub Actions workflow for every CI platform:

```bat
tools\local-build\ci-all-platforms.bat
```

It uses the current branch by default. To build a specific branch or tag:

```bat
tools\local-build\ci-all-platforms.bat v1.2.0-1
tools\local-build\ci-all-platforms.bat dawnlight
```

Requirements:

- GitHub CLI: `gh`
- authenticated with `gh auth login`

Note for Apple artifacts:

- macOS should be packaged on the macOS CI runner with ad-hoc codesign.
- iOS should be packaged as `.ipa` on the macOS CI runner, not rebuilt from a downloaded artifact on Windows, because Windows ZIP tooling can lose Unix executable permissions.
