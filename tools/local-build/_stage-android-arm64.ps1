param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path -LiteralPath $RepoRoot).Path
$src = Join-Path $root 'build/android-arm64/libmain.so'
$jniRoot = Join-Path $root 'platforms/android/app/src/main/jniLibs'
$arm64Dir = Join-Path $jniRoot 'arm64-v8a'
$dst = Join-Path $arm64Dir 'libmain.so'
$tmp = Join-Path $arm64Dir 'libmain.so.tmp'

if (!(Test-Path -LiteralPath $src)) {
    throw "Missing native library: $src"
}

New-Item -ItemType Directory -Path $arm64Dir -Force | Out-Null

# Avoid accidentally packaging stale ABI folders from older local builds.
$jniRootResolved = (Resolve-Path -LiteralPath $jniRoot).Path
foreach ($staleAbi in @('x86', 'x86_64')) {
    $candidate = Join-Path $jniRoot $staleAbi
    if (Test-Path -LiteralPath $candidate) {
        $candidateResolved = (Resolve-Path -LiteralPath $candidate).Path
        if (!$candidateResolved.StartsWith($jniRootResolved, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove path outside jniLibs: $candidateResolved"
        }
        Remove-Item -LiteralPath $candidateResolved -Recurse -Force
    }
}

Copy-Item -LiteralPath $src -Destination $tmp -Force

$androidHome = $env:ANDROID_HOME
if ([string]::IsNullOrWhiteSpace($androidHome)) {
    $androidHome = Join-Path $env:LOCALAPPDATA 'Android/Sdk'
}

$stripTool = $null
$ndkRoot = Join-Path $androidHome 'ndk'
if (Test-Path -LiteralPath $ndkRoot) {
    $ndks = Get-ChildItem -LiteralPath $ndkRoot -Directory | Sort-Object Name -Descending
    foreach ($ndk in $ndks) {
        $candidate = Join-Path $ndk.FullName 'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-strip.exe'
        if (Test-Path -LiteralPath $candidate) {
            $stripTool = $candidate
            break
        }
    }
}

if ($stripTool) {
    & $stripTool --strip-unneeded $tmp
    if ($LASTEXITCODE -ne 0) {
        throw "llvm-strip failed with exit code $LASTEXITCODE"
    }
    Write-Host "Stripped native library with $stripTool"
} else {
    Write-Host "llvm-strip not found; staging unstripped libmain.so"
}

Move-Item -LiteralPath $tmp -Destination $dst -Force

$libPng = Join-Path $arm64Dir 'libpng16.so'
if (!(Test-Path -LiteralPath $libPng)) {
    Write-Warning "libpng16.so is missing from $arm64Dir. The APK may fail at startup if libmain.so depends on it."
}

Get-ChildItem -LiteralPath $arm64Dir | Select-Object Name, Length, LastWriteTime | Format-Table -AutoSize
