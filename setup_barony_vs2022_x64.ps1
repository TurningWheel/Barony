<# 
Barony Windows x64 Setup (PowerShell)

- Uses this repository checkout + vcpkg
- Builds & installs open-source dependencies via vcpkg (x64-windows)
- Validates proprietary SDK folder layout (FMOD / Steamworks / EOS)
- Can auto-build TheoraPlayer from source when enabled
- Generates a Visual Studio 2022-compatible solution via CMake and builds Release once
- Copies runtime DLLs next to the built .exe (and optionally BARONY_DATADIR)

Run in PowerShell (Windows):
  PS> Set-ExecutionPolicy -Scope Process Bypass
  PS> .\setup_barony_vs2022_x64.ps1

You must manually provide licensed SDKs:
  deps\fmod
  deps\steamworks
  deps\eos
#>

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# --------------------------
# USER SETTINGS (edit here)
# --------------------------
# Enable/disable features
$EnableFmod       = $true
$EnableSteamworks = $true
$EnableEos        = $false

# Optional toggles (default off)
$EnableCurl         = $false
$EnableOpus         = $false
# PlayFab is intentionally not automated here (SDK + token provisioning is account-specific)
$EnablePlayFab      = $false
$EnableTheoraPlayer = $true
$TheoraPlayerRepoUrl = "https://github.com/AprilAndFriends/theoraplayer.git"
$TheoraPlayerRepoRef = ""

# Optional: set to your Barony assets/data folder (Steam install, etc)
# Example: $BaronyDataDir = "D:\SteamLibrary\steamapps\common\Barony"
$BaronyDataDir = ""
if (-not $BaronyDataDir -and $env:BARONY_DATADIR) {
  $BaronyDataDir = $env:BARONY_DATADIR
}

# --------------------------
# INTERNAL PATHS
# --------------------------
$Root     = $PSScriptRoot
$DepsDir  = Join-Path $Root "deps"
$SrcDir   = $Root
$BuildDir = Join-Path $Root "build-vs2022-x64"

$VcpkgDir        = Join-Path $DepsDir "vcpkg"
$VcpkgTriplet    = "x64-windows"
$VcpkgInstalled  = Join-Path $VcpkgDir "installed\$VcpkgTriplet"
$VcpkgToolchain  = Join-Path $VcpkgDir "scripts\buildsystems\vcpkg.cmake"

# Proprietary SDK locations (you must supply these)
$FmodDir        = Join-Path $DepsDir "fmod"
$SteamworksRoot = Join-Path $DepsDir "steamworks"
$EosRoot        = Join-Path $DepsDir "eos"
$TheoraPlayerRoot = Join-Path $DepsDir "theoraplayer"

function Assert-Command([string]$Name, [string]$Hint) {
  if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
    throw "Missing required command '$Name'. $Hint"
  }
}

function Get-VsInstallPath {
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Install Visual Studio with C++ build tools."
  }
  # CMake in this workflow uses the VS 2022 generator ("Visual Studio 17 2022").
  # VS 2026 users should install the VS 2022-compatible C++ toolset as well.
  $path = & $vswhere -latest -products * -version "[17.0,18.0)" -requires Microsoft.Component.MSBuild -property installationPath
  if (-not $path) {
    throw "Visual Studio 2022-compatible build tools were not found. Install VS 2022 Build Tools (or VS 2026 with VS 2022-compatible C++ toolset)."
  }
  return $path.Trim()
}

function Import-VsDevCmdEnv {
  param(
    [Parameter(Mandatory=$true)][string]$VsDevCmdBat
  )
  if (-not (Test-Path $VsDevCmdBat)) { throw "VsDevCmd.bat not found at: $VsDevCmdBat" }

  # Run VsDevCmd.bat inside cmd.exe and dump the resulting environment with `set`
  $cmd = "`"$VsDevCmdBat`" -arch=amd64 -host_arch=amd64 >nul && set"
  $output = & cmd.exe /c $cmd

  foreach ($line in $output) {
    $idx = $line.IndexOf('=')
    if ($idx -gt 0) {
      $name  = $line.Substring(0, $idx)
      $value = $line.Substring($idx + 1)
      Set-Item -Path ("Env:{0}" -f $name) -Value $value
    }
  }
}

function Convert-ToCMakePath([string]$Path) {
  return ($Path -replace '\\', '/')
}

function Remove-DirectoryIfExists([string]$Path) {
  if (-not (Test-Path $Path)) { return }
  & cmd.exe /c "rmdir /s /q `"$Path`"" | Out-Null
  if (Test-Path $Path) {
    Remove-Item -Recurse -Force $Path
  }
}

function Ensure-Vcpkg {
  if (-not (Test-Path (Join-Path $VcpkgDir "vcpkg.exe"))) {
    Write-Host "[2/6] Cloning vcpkg..."
    & git clone --depth 1 https://github.com/microsoft/vcpkg $VcpkgDir
    Write-Host "[2/6] Bootstrapping vcpkg..."
    & cmd.exe /c "`"$VcpkgDir\bootstrap-vcpkg.bat`" -disableMetrics"
  } else {
    Write-Host "[2/6] vcpkg already present: $VcpkgDir"
  }
}

function Vcpkg-Install {
  $pkgs = @(
    "sdl2", "sdl2-image", "sdl2-net", "sdl2-ttf",
    "physfs", "rapidjson", "libpng", "zlib", "dirent", "glew"
  )
  if ($EnableSteamworks) { $pkgs += "nativefiledialog-extended" }
  if ($EnableCurl) { $pkgs += @("openssl", "curl[openssl]") }
  if ($EnableOpus) { $pkgs += "opus" }
  if ($EnableTheoraPlayer) { $pkgs += @("libogg", "libvorbis", "libtheora") }
  if ($EnablePlayFab) { throw "PlayFab support is not automated by this script. Keep `$EnablePlayFab = `$false unless you have a separate PlayFab SDK/token setup." }

  Write-Host "[3/6] Installing deps via vcpkg ($VcpkgTriplet)..."
  Write-Host "       $($pkgs -join ' ')"

  & "$VcpkgDir\vcpkg.exe" install @pkgs --triplet $VcpkgTriplet

  if (-not (Test-Path (Join-Path $VcpkgInstalled "include"))) { throw "vcpkg include/ missing at $VcpkgInstalled" }
  if (-not (Test-Path (Join-Path $VcpkgInstalled "lib")))     { throw "vcpkg lib/ missing at $VcpkgInstalled" }
}

function Check-Fmod {
  $hdr = Join-Path $FmodDir "api\core\inc\fmod.hpp"
  if (-not (Test-Path $hdr)) {
    throw "FMOD headers not found at $hdr. Extract the FMOD Core API to $FmodDir"
  }

  # Patch common Windows folder mismatch (FMOD often has x64; Barony expects x86_64)
  $libX64   = Join-Path $FmodDir "api\core\lib\x64"
  $libX8664 = Join-Path $FmodDir "api\core\lib\x86_64"
  if ((Test-Path $libX64) -and (-not (Test-Path $libX8664))) {
    Write-Host "[FMOD] Mirroring api\core\lib\x64 -> api\core\lib\x86_64 ..."
    New-Item -ItemType Directory -Force -Path $libX8664 | Out-Null
    & robocopy $libX64 $libX8664 *.* /E /NFL /NDL /NJH /NJS | Out-Null
  }

  $lib = Join-Path $libX8664 "fmod_vc.lib"
  if (-not (Test-Path $lib)) { throw "FMOD import library not found: $lib" }

  $env:FMOD_DIR = $FmodDir
}

function Check-Steamworks {
  $hdr = Join-Path $SteamworksRoot "sdk\public\steam\steam_api.h"
  $lib = Join-Path $SteamworksRoot "sdk\redistributable_bin\win64\steam_api64.lib"
  if (-not (Test-Path $hdr)) { throw "Steamworks headers not found: $hdr" }
  if (-not (Test-Path $lib)) { throw "Steamworks import library not found: $lib" }
  $env:STEAMWORKS_ROOT = $SteamworksRoot
}

function Ensure-SteamAppIdFile {
  $appidPath = Join-Path $SrcDir "steam_appid.txt"
  if (Test-Path $appidPath) {
    return $appidPath
  }

  $mainHeader = Join-Path $SrcDir "src\main.hpp"
  if (-not (Test-Path $mainHeader)) {
    Write-Warning "Could not locate $mainHeader to derive STEAM_APPID."
    return $null
  }

  $match = Select-String -Path $mainHeader -Pattern '^\s*#define\s+STEAM_APPID\s+(\d+)' | Select-Object -First 1
  if (-not $match) {
    Write-Warning "Could not derive STEAM_APPID from $mainHeader."
    return $null
  }

  $steamAppId = $match.Matches[0].Groups[1].Value
  Set-Content -Path $appidPath -Value $steamAppId -Encoding ASCII
  Write-Host "[Steamworks] Created $appidPath with appid $steamAppId"
  return $appidPath
}

function Check-Eos {
  $hdr = Join-Path $EosRoot "SDK\Include\eos_sdk.h"
  $lib = Join-Path $EosRoot "SDK\Lib\EOSSDK-Win64-Shipping.lib"
  if (-not (Test-Path $hdr)) { throw "EOS headers not found: $hdr" }
  if (-not (Test-Path $lib)) { throw "EOS import library not found: $lib" }
  $env:EOS_ROOT = $EosRoot
}

function Ensure-EosTokens {
  # Barony's CMakeLists will error if EOS_ENABLED=1 and any of these are empty.
  if (-not $env:BUILD_ENV_PR) { $env:BUILD_ENV_PR = Read-Host "EOS BUILD_ENV_PR (Product ID)" }
  if (-not $env:BUILD_ENV_SA) { $env:BUILD_ENV_SA = Read-Host "EOS BUILD_ENV_SA (Sandbox ID)" }
  if (-not $env:BUILD_ENV_DE) { $env:BUILD_ENV_DE = Read-Host "EOS BUILD_ENV_DE (Deployment ID)" }
  if (-not $env:BUILD_ENV_CC) { $env:BUILD_ENV_CC = Read-Host "EOS BUILD_ENV_CC (Client ID)" }

  if (-not $env:BUILD_ENV_CS) {
    $sec = Read-Host "EOS BUILD_ENV_CS (Client Secret)" -AsSecureString
    $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($sec)
    try { $env:BUILD_ENV_CS = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr) }
    finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr) }
  }

  if (-not $env:BUILD_ENV_GSE) { $env:BUILD_ENV_GSE = Read-Host "EOS BUILD_ENV_GSE (Game Server Encryption Key)" }

  foreach ($k in "BUILD_ENV_PR","BUILD_ENV_SA","BUILD_ENV_DE","BUILD_ENV_CC","BUILD_ENV_CS","BUILD_ENV_GSE") {
    if (-not ${env:$k}) { throw "EOS token $k is empty." }
  }
}

function Check-TheoraPlayer {
  # Required by cmake/Modules/FindTheoraPlayer.cmake
  $hdr = Join-Path $TheoraPlayerRoot "include\theoraplayer\theoraplayer.h"
  $lib = Join-Path $TheoraPlayerRoot "lib\theoraplayer.lib"
  if (-not (Test-Path $hdr)) {
    throw "TheoraPlayer headers not found: $hdr. Build/provide TheoraPlayer and place headers under $TheoraPlayerRoot\include\theoraplayer\."
  }
  if (-not (Test-Path $lib)) {
    throw "TheoraPlayer import library not found: $lib. Build/provide TheoraPlayer and place theoraplayer.lib under $TheoraPlayerRoot\lib\."
  }
  $env:THEORAPLAYER_DIR = $TheoraPlayerRoot
}

function Build-TheoraPlayerFromSource {
  Write-Host "[TheoraPlayer] Building from source: $TheoraPlayerRepoUrl"

  $tmpRoot      = Join-Path $DepsDir "_tmp_theoraplayer"
  $cloneDir     = Join-Path $tmpRoot "src"
  $cmakeProjDir = Join-Path $tmpRoot "cmake"
  $cmakeBuildDir= Join-Path $tmpRoot "build"

  Remove-DirectoryIfExists $tmpRoot
  New-Item -ItemType Directory -Force -Path $cloneDir | Out-Null
  New-Item -ItemType Directory -Force -Path $cmakeProjDir | Out-Null

  try {
    & git clone --depth 1 $TheoraPlayerRepoUrl $cloneDir
    if ($LASTEXITCODE -ne 0) {
      throw "Failed to clone TheoraPlayer from $TheoraPlayerRepoUrl"
    }

    if ($TheoraPlayerRepoRef) {
      Push-Location $cloneDir
      try {
        & git fetch --depth 1 origin $TheoraPlayerRepoRef
        if ($LASTEXITCODE -ne 0) { throw "Failed to fetch TheoraPlayer ref '$TheoraPlayerRepoRef'" }
        & git checkout --force FETCH_HEAD
        if ($LASTEXITCODE -ne 0) { throw "Failed to checkout TheoraPlayer ref '$TheoraPlayerRepoRef'" }
      }
      finally {
        Pop-Location
      }
    }

    $theoraRootCMake = Convert-ToCMakePath $cloneDir
    $toolchainCMake  = Convert-ToCMakePath $VcpkgToolchain

    $cmakeLists = @'
cmake_minimum_required(VERSION 3.20)
project(theoraplayer_builder LANGUAGES C CXX)

find_package(Ogg CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(unofficial-theora CONFIG REQUIRED)

add_library(theoraplayer SHARED
  ${THEORAPLAYER_ROOT}/theoraplayer/src/AudioInterface.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/AudioInterfaceFactory.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/AudioPacketQueue.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/DataSource.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Exception.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/FileDataSource.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/FrameQueue.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Manager.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/MemoryDataSource.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Mutex.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/theoraplayer.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Thread.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Timer.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/Utility.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/VideoClip.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/VideoFrame.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/WorkerThread.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/formats/Theora/VideoClip_Theora.cpp
  ${THEORAPLAYER_ROOT}/theoraplayer/src/YUV/yuv_util.c
  ${THEORAPLAYER_ROOT}/theoraplayer/src/YUV/C/yuv420_grey_c.c
  ${THEORAPLAYER_ROOT}/theoraplayer/src/YUV/C/yuv420_rgb_c.c
  ${THEORAPLAYER_ROOT}/theoraplayer/src/YUV/C/yuv420_yuv_c.c
)

target_include_directories(theoraplayer PRIVATE
  ${THEORAPLAYER_ROOT}/theoraplayer/include
  ${THEORAPLAYER_ROOT}/theoraplayer/include/theoraplayer
  ${THEORAPLAYER_ROOT}/theoraplayer/src
  ${THEORAPLAYER_ROOT}/theoraplayer/src/formats
  ${THEORAPLAYER_ROOT}/theoraplayer/src/YUV
)

target_compile_definitions(theoraplayer PRIVATE
  _USE_THEORA
  _YUV_C
  THEORAPLAYER_EXPORTS
  _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(theoraplayer PRIVATE
  Ogg::ogg
  Vorbis::vorbis
  Vorbis::vorbisfile
  Vorbis::vorbisenc
  unofficial::theora::theora
  unofficial::theora::theoradec
  unofficial::theora::theoraenc
)

set_target_properties(theoraplayer PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  OUTPUT_NAME theoraplayer
)
'@
    $tempCMake = Join-Path $cmakeProjDir "CMakeLists.txt"
    Set-Content -Path $tempCMake -Value $cmakeLists -Encoding ASCII

    & cmake -S $cmakeProjDir -B $cmakeBuildDir -G "Visual Studio 17 2022" -A x64 `
      -DTHEORAPLAYER_ROOT="$theoraRootCMake" `
      -DCMAKE_TOOLCHAIN_FILE="$toolchainCMake" `
      -DVCPKG_TARGET_TRIPLET="$VcpkgTriplet"
    if ($LASTEXITCODE -ne 0) {
      throw "Failed to configure TheoraPlayer build."
    }

    & cmake --build $cmakeBuildDir --config Release --target theoraplayer
    if ($LASTEXITCODE -ne 0) {
      throw "Failed to build TheoraPlayer."
    }

    $releaseDir = Join-Path $cmakeBuildDir "Release"
    $builtLib   = Join-Path $releaseDir "theoraplayer.lib"
    $builtDll   = Join-Path $releaseDir "theoraplayer.dll"
    if (-not (Test-Path $builtLib)) { throw "Expected build artifact missing: $builtLib" }
    if (-not (Test-Path $builtDll)) { throw "Expected build artifact missing: $builtDll" }

    $sourceIncludeDir = Join-Path $cloneDir "theoraplayer\include\theoraplayer"
    if (-not (Test-Path $sourceIncludeDir)) {
      throw "TheoraPlayer include directory not found: $sourceIncludeDir"
    }

    $targetIncludeDir = Join-Path $TheoraPlayerRoot "include\theoraplayer"
    $targetLibDir     = Join-Path $TheoraPlayerRoot "lib"
    $targetBinDir     = Join-Path $TheoraPlayerRoot "bin"
    New-Item -ItemType Directory -Force -Path $targetIncludeDir | Out-Null
    New-Item -ItemType Directory -Force -Path $targetLibDir | Out-Null
    New-Item -ItemType Directory -Force -Path $targetBinDir | Out-Null

    & robocopy $sourceIncludeDir $targetIncludeDir *.h /E /NFL /NDL /NJH /NJS | Out-Null
    if ($LASTEXITCODE -ge 8) {
      throw "Failed to copy TheoraPlayer headers."
    }

    Copy-Item -Force $builtLib (Join-Path $targetLibDir "theoraplayer.lib")
    Copy-Item -Force $builtDll (Join-Path $targetBinDir "theoraplayer.dll")
    Write-Host "[TheoraPlayer] Installed to: $TheoraPlayerRoot"
  }
  finally {
    Remove-DirectoryIfExists $tmpRoot
  }
}

function Ensure-TheoraPlayer {
  $hdr = Join-Path $TheoraPlayerRoot "include\theoraplayer\theoraplayer.h"
  $lib = Join-Path $TheoraPlayerRoot "lib\theoraplayer.lib"
  if ((Test-Path $hdr) -and (Test-Path $lib)) {
    Write-Host "[TheoraPlayer] Using existing installation at: $TheoraPlayerRoot"
  } else {
    Build-TheoraPlayerFromSource
  }
  Check-TheoraPlayer
}

function Copy-RuntimeDlls {
  Write-Host "Copying runtime DLLs next to built executables..."

  $baronyExes = Get-ChildItem -Path $BuildDir -Recurse -Filter "barony.exe" -ErrorAction SilentlyContinue
  if (-not $baronyExes) {
    Write-Warning "Could not locate barony.exe under $BuildDir. Skipping runtime copy."
    return
  }

  # Always populate both standard VS config output dirs to avoid first-run DLL errors
  # when users switch configs after setup (e.g. building Debug in Visual Studio).
  $outDirMap = @{}
  foreach ($exe in $baronyExes) {
    $outDirMap[$exe.Directory.FullName] = $true
  }
  foreach ($cfg in @("Debug", "Release")) {
    $cfgOutDir = Join-Path $BuildDir $cfg
    New-Item -ItemType Directory -Force -Path $cfgOutDir | Out-Null
    $outDirMap[$cfgOutDir] = $true
  }
  $outDirs = $outDirMap.Keys | Sort-Object

  foreach ($outDir in $outDirs) {
    Write-Host "  output dir: $outDir"

    # vcpkg DLLs (SDL2, etc.)
    $vcpkgBin = Join-Path $VcpkgInstalled "bin"
    if (Test-Path $vcpkgBin) {
      & robocopy $vcpkgBin $outDir *.dll /NFL /NDL /NJH /NJS | Out-Null
    }

    # FMOD DLLs
    if ($EnableFmod) {
      $fmodBin = Join-Path $FmodDir "api\core\lib\x86_64"
      if (Test-Path $fmodBin) {
        & robocopy $fmodBin $outDir *.dll /NFL /NDL /NJH /NJS | Out-Null
      }
    }

    # TheoraPlayer DLLs (if enabled and provided)
    if ($EnableTheoraPlayer) {
      $theoraBin = Join-Path $TheoraPlayerRoot "bin"
      if (Test-Path $theoraBin) {
        & robocopy $theoraBin $outDir theoraplayer*.dll /NFL /NDL /NJH /NJS | Out-Null
      }
    }

    # Steamworks DLLs + steam_appid.txt
    if ($EnableSteamworks) {
      $steamBin = Join-Path $SteamworksRoot "sdk\redistributable_bin\win64"
      if (Test-Path $steamBin) {
        & robocopy $steamBin $outDir *.dll /NFL /NDL /NJH /NJS | Out-Null
      }
      $appid = Ensure-SteamAppIdFile
      if (Test-Path $appid) { Copy-Item -Force $appid $outDir }
    }

    # EOS DLL
    if ($EnableEos) {
      $eosDll = Get-ChildItem -Path $EosRoot -Recurse -Filter "EOSSDK-Win64-Shipping.dll" -ErrorAction SilentlyContinue | Select-Object -First 1
      if ($eosDll) {
        Copy-Item -Force $eosDll.FullName $outDir
      } else {
        Write-Warning "EOS enabled but EOSSDK-Win64-Shipping.dll not found under $EosRoot"
      }
    }
  }

  # Optionally also copy everything to BaronyDataDir
  if ($BaronyDataDir) {
    if (-not (Test-Path $BaronyDataDir)) {
      Write-Warning "BARONY_DATADIR '$BaronyDataDir' does not exist; skipping."
      return
    }

    $preferredExe = $baronyExes | Where-Object { $_.Directory.FullName -like "*\Release" } | Select-Object -First 1
    if (-not $preferredExe) {
      $preferredExe = $baronyExes | Select-Object -First 1
    }
    $preferredOutDir = $preferredExe.Directory.FullName

    Write-Host "  also copying to BARONY_DATADIR: $BaronyDataDir"
    Copy-Item -Force $preferredExe.FullName $BaronyDataDir

    $editorExe = Get-ChildItem -Path $preferredOutDir -Filter "editor.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($editorExe) { Copy-Item -Force $editorExe.FullName $BaronyDataDir }

    & robocopy $preferredOutDir $BaronyDataDir *.dll /NFL /NDL /NJH /NJS | Out-Null

    $langEn = Join-Path $SrcDir "lang\en.txt"
    if (Test-Path $langEn) {
      $langDir = Join-Path $BaronyDataDir "lang"
      New-Item -ItemType Directory -Force -Path $langDir | Out-Null
      Copy-Item -Force $langEn (Join-Path $langDir "en.txt")
    }
  }

  Write-Host "  Done."
}

# --------------------------
# MAIN
# --------------------------
Write-Host "=============================================================="
Write-Host "  Barony setup (VS 2022-compatible, x64)"
Write-Host "  Root: $Root"
Write-Host "=============================================================="
Write-Host ""

Assert-Command git   "Install Git and ensure git.exe is on PATH."
Assert-Command cmake "Install CMake and ensure cmake.exe is on PATH."

if (-not (Test-Path (Join-Path $SrcDir "CMakeLists.txt"))) {
  throw "CMakeLists.txt not found in '$SrcDir'. Run this script from the repository root."
}

New-Item -ItemType Directory -Force -Path $DepsDir | Out-Null

$vsInstall = Get-VsInstallPath
$vsDevCmd  = Join-Path $vsInstall "Common7\Tools\VsDevCmd.bat"
Import-VsDevCmdEnv -VsDevCmdBat $vsDevCmd

Write-Host "[1/6] Using repository at: $SrcDir"
# Avoid pulling an unrelated global vcpkg root/toolchain from another VS install.
if (Test-Path Env:VCPKG_ROOT) {
  Remove-Item Env:VCPKG_ROOT -ErrorAction SilentlyContinue
}
Ensure-Vcpkg
Vcpkg-Install

# Barony expects BARONY_WIN32_LIBRARIES to be a prefix containing include/ + lib/
$env:BARONY_WIN32_LIBRARIES = $VcpkgInstalled

# Feature env vars used by Barony's CMakeLists.txt
$env:STEAMWORKS_ENABLED   = [int]$EnableSteamworks
$env:EOS_ENABLED          = [int]$EnableEos
$env:CURL_ENABLED         = [int]$EnableCurl
$env:OPUS_ENABLED         = [int]$EnableOpus
$env:PLAYFAB_ENABLED      = [int]$EnablePlayFab
$env:THEORAPLAYER_ENABLED = [int]$EnableTheoraPlayer
if ($EnableOpus) { $env:OPUS_DIR = $VcpkgInstalled }

if ($BaronyDataDir) {
  $env:BARONY_DATADIR = $BaronyDataDir
  Write-Host "[Config] BARONY_DATADIR: $BaronyDataDir"
}

# Validate proprietary SDKs
if ($EnableFmod)       { Check-Fmod }
if ($EnableSteamworks) { Check-Steamworks }
if ($EnableEos)        { Check-Eos; Ensure-EosTokens }
if ($EnableTheoraPlayer) { Ensure-TheoraPlayer }

# Configure + build
Write-Host "[5/6] Configuring CMake (Visual Studio 17 2022, x64)..."
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$fmodFlag = if ($EnableFmod) { "ON" } else { "OFF" }
$InstallPrefix = if ($BaronyDataDir) { $BaronyDataDir } else { Join-Path $BuildDir "install-root" }
if (-not $BaronyDataDir) {
  New-Item -ItemType Directory -Force -Path $InstallPrefix | Out-Null
}
Write-Host "       Install prefix: $InstallPrefix"

& cmake -S $SrcDir -B $BuildDir -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$VcpkgToolchain" -DVCPKG_TARGET_TRIPLET=$VcpkgTriplet `
  -DFMOD_ENABLED=$fmodFlag -DOPENAL_ENABLED=OFF `
  -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON `
  -DCMAKE_INSTALL_PREFIX="$InstallPrefix"

Write-Host "[6/6] Building Release..."
& cmake --build $BuildDir --config Release

Copy-RuntimeDlls

Write-Host ""
Write-Host "=============================================================="
Write-Host "SUCCESS"
Write-Host "  Source   : $SrcDir"
Write-Host "  Build dir : $BuildDir"
Write-Host "  Solution : $(Join-Path $BuildDir 'barony.sln')"
Write-Host "=============================================================="
