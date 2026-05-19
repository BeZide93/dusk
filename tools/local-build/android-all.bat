@echo off
setlocal EnableExtensions

call "%~dp0android-debug.bat"
if errorlevel 1 exit /b 1

call "%~dp0android-release-signed.bat"
if errorlevel 1 exit /b 1

echo.
echo Android debug and signed release builds are ready.
exit /b 0
