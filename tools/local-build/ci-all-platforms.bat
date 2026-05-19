@echo off
setlocal EnableExtensions

set "ROOT=%~dp0..\.."
pushd "%ROOT%" || exit /b 1

set "REF=%~1"
if "%REF%"=="" (
  for /f "usebackq delims=" %%I in (`git branch --show-current`) do set "REF=%%I"
)

where gh.exe >nul 2>nul
if errorlevel 1 (
  echo GitHub CLI gh.exe was not found in PATH.
  echo Install GitHub CLI or run the local Android/Windows scripts instead.
  popd
  exit /b 1
)

gh auth status >nul
if errorlevel 1 (
  echo GitHub CLI is not authenticated. Run: gh auth login
  popd
  exit /b 1
)

echo.
echo Starting GitHub Actions Build workflow for all CI platforms.
echo Ref: %REF%
echo.

gh workflow run build.yml --ref "%REF%"
if errorlevel 1 goto fail

echo.
echo Recent Build workflow runs:
gh run list --workflow build.yml --branch "%REF%" --limit 5

echo.
echo CI build started. Open the run URL above or use:
echo   gh run watch
popd
exit /b 0

:fail
echo.
echo Failed to start GitHub Actions workflow.
popd
exit /b 1
