@echo off
setlocal EnableExtensions

set "ROOT=%~dp0..\.."
pushd "%ROOT%" || exit /b 1

if not exist "keystores\release-signing.properties" (
  echo Missing keystores\release-signing.properties
  echo Create it with storeFile, storePassword, keyAlias and keyPassword before building a signed release APK.
  popd
  exit /b 1
)

if "%ANDROID_HOME%"=="" set "ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk"
if "%ANDROID_NDK_VERSION%"=="" (
  for /f "usebackq delims=" %%I in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$ndk = Join-Path $env:ANDROID_HOME 'ndk'; if (Test-Path $ndk) { Get-ChildItem $ndk -Directory | Sort-Object Name -Descending | Select-Object -First 1 -ExpandProperty Name }"`) do set "ANDROID_NDK_VERSION=%%I"
)

echo.
echo Building Dawnlight signed Android release APK
echo ANDROID_HOME=%ANDROID_HOME%
echo ANDROID_NDK_VERSION=%ANDROID_NDK_VERSION%
echo.

cmake --preset x-android-ci-arm64 -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=
if errorlevel 1 goto fail

cmake --build --preset x-android-ci-arm64 --target dawnlight
if errorlevel 1 goto fail

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0_stage-android-arm64.ps1" -RepoRoot "%CD%"
if errorlevel 1 goto fail

call platforms\android\gradlew.bat -p platforms\android :app:packageRelease
if errorlevel 1 goto fail

if not exist "apk\release" mkdir "apk\release"
if not exist "local-builds\android\release" mkdir "local-builds\android\release"
copy /Y "platforms\android\app\build\outputs\apk\release\app-arm64-v8a-release.apk" "apk\release\app-arm64-v8a-release.apk" >nul
copy /Y "platforms\android\app\build\outputs\apk\release\app-arm64-v8a-release.apk" "local-builds\android\release\app-arm64-v8a-release.apk" >nul

echo.
echo Signed Android release APK ready:
echo   %CD%\apk\release\app-arm64-v8a-release.apk
echo   %CD%\local-builds\android\release\app-arm64-v8a-release.apk
popd
exit /b 0

:fail
echo.
echo Signed Android release build failed.
popd
exit /b 1
