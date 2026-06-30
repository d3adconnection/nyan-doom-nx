# build-switch.ps1
# Build Nyan Doom NX for Nintendo Switch.
# Usage:  .\build-switch.ps1           # incremental (default)
#         .\build-switch.ps1 -Clean    # full clean rebuild
# Output: build-switch\nyan-doom.nro

param(
    [switch]$Clean = $false
)

$ErrorActionPreference = "Stop"

$BASH      = "C:\devkitPro\msys2\usr\bin\bash.exe"
$MSYS_REPO = "/d/Documents/nyan-doom-nx"
$WIN_REPO  = "D:\Documents\nyan-doom-nx"
$TOOLCHAIN = $MSYS_REPO + "/prboom2/cmake/SwitchToolchain.cmake"
$NRO       = $WIN_REPO  + "\build-switch\nyan-doom.nro"

Write-Host ""
Write-Host "Nyan Doom NX Switch Build" -ForegroundColor Cyan
Write-Host "Repo: $WIN_REPO"
Write-Host ""

$buildDir = $WIN_REPO + "\build-switch"
if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "[1/3] Cleaning build-switch/ ..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
} elseif (-not $Clean) {
    Write-Host "[1/3] Skipping clean (incremental build) ..." -ForegroundColor DarkGray
}

Write-Host "[2/3] Configuring ..." -ForegroundColor Yellow
$cfg = "cmake"
$cfg = $cfg + " -S " + $MSYS_REPO + "/prboom2"
$cfg = $cfg + " -B " + $MSYS_REPO + "/build-switch"
$cfg = $cfg + " -G Ninja"
$cfg = $cfg + " -DCMAKE_TOOLCHAIN_FILE=" + $TOOLCHAIN
$cfg = $cfg + " -DWITH_PORTMIDI=OFF"
$cfg = $cfg + " -DWITH_FLUIDSYNTH=ON"
$cfg = $cfg + " -DWITH_MAD=OFF"
& $BASH -lc $cfg
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

Write-Host "[3/3] Building ..." -ForegroundColor Yellow
$bld = "cmake --build " + $MSYS_REPO + "/build-switch -j8"
& $BASH -lc $bld
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host ""
Write-Host "Build complete." -ForegroundColor Green
Write-Host "Output: $NRO"
Write-Host ""
