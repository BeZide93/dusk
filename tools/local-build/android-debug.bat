@echo off
setlocal EnableExtensions

set "ROOT=%~dp0..\.."
pushd "%ROOT%" || exit /b 1

if "%ANDROID_HOME%"=="" set "ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk"
if "%ANDROID_NDK_VERSION%"=="" (
  for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$ndk = Join-Path $env:ANDROID_HOME 'ndk'; if (Test-Path $ndk) { Get-ChildItem $ndk -Directory | Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name }"`) do set "ANDROID_NDK_VERSION=%%I"
)

echo.
echo Building Dusklight Android debug APK with Dawnlight mod services
echo ANDROID_HOME=%ANDROID_HOME%
echo ANDROID_NDK_VERSION=%ANDROID_NDK_VERSION%
echo.

cmake --preset x-android-ci-arm64 -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=
if errorlevel 1 goto fail

cmake --build --preset x-android-ci-arm64 --target dusklight dawnlight_mod_package
if errorlevel 1 goto fail

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0_stage-android-arm64.ps1" -RepoRoot "%CD%"
if errorlevel 1 goto fail

call platforms\android\gradlew.bat -p platforms\android :app:packageDebug
if errorlevel 1 goto fail

if not exist "apk\debug" mkdir "apk\debug"
if not exist "local-builds\android\debug" mkdir "local-builds\android\debug"
copy /Y "platforms\android\app\build\outputs\apk\debug\app-arm64-v8a-debug.apk" "apk\debug\app-arm64-v8a-debug.apk" >nul
copy /Y "platforms\android\app\build\outputs\apk\debug\app-arm64-v8a-debug.apk" "local-builds\android\debug\app-arm64-v8a-debug.apk" >nul

echo.
echo Android debug APK ready:
echo   %CD%\apk\debug\app-arm64-v8a-debug.apk
echo   %CD%\local-builds\android\debug\app-arm64-v8a-debug.apk
popd
exit /b 0

:fail
echo.
echo Android debug build failed.
popd
exit /b 1
