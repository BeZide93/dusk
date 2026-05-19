@echo off
setlocal EnableExtensions

set "ROOT=%~dp0..\.."
pushd "%ROOT%" || exit /b 1

where cl.exe >nul 2>nul
if errorlevel 1 (
  set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%I"
    if defined VSINSTALL (
      call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=amd64
    )
  )
)

where cl.exe >nul 2>nul
if errorlevel 1 (
  echo Could not find cl.exe. Install Visual Studio Build Tools or run this from a Developer Command Prompt.
  popd
  exit /b 1
)

if "%VCPKG_ROOT%"=="" set "VCPKG_ROOT=C:\vcpkg"

echo.
echo Building Dawnlight Windows MSVC package
echo VCPKG_ROOT=%VCPKG_ROOT%
echo.

cmake --preset x-windows-ci-msvc -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=
if errorlevel 1 goto fail

cmake --build --preset x-windows-ci-msvc
if errorlevel 1 goto fail

if not exist "local-builds\windows" mkdir "local-builds\windows"
powershell -NoProfile -ExecutionPolicy Bypass -Command "if (Test-Path 'local-builds/windows/Dawnlight-win32-x86_64.zip') { Remove-Item -LiteralPath 'local-builds/windows/Dawnlight-win32-x86_64.zip' -Force }; Compress-Archive -Path 'build/install/*' -DestinationPath 'local-builds/windows/Dawnlight-win32-x86_64.zip' -Force"
if errorlevel 1 goto fail

echo.
echo Windows package ready:
echo   %CD%\local-builds\windows\Dawnlight-win32-x86_64.zip
popd
exit /b 0

:fail
echo.
echo Windows build failed.
popd
exit /b 1
