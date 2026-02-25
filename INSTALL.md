# Barony Build and Setup Guide

This guide is for developers building Barony from source on Windows, macOS, and Linux.

## Choose Your Setup Path

- Windows (recommended): use `setup_barony_vs2022_x64.ps1` interactive installer.
- Windows (fallback): use manual CMake setup only if installer is intentionally skipped or troubleshooting is needed.
- macOS: Homebrew-based full-feature build.
- Linux: Docker full-feature build (recommended) or native package-based build.

## Core Build Requirements

Required for all platforms:

- CMake (recent version; tested with `3.10+`, including current `4.x`)
- Git

Platform package dependencies (SDL2/OpenGL/libpng/PhysFS/RapidJSON/etc.) are handled in the platform-specific sections below.

CMake downloads: https://cmake.org/download/

## Optional Integrations

- FMOD Core API (preferred audio path on Windows)
- Steamworks SDK
- Epic Online Services (EOS) SDK
- CURL + OpenSSL (workshop preview HTTP downloads)
- Opus (encoded voice chat)
- TheoraPlayer + Ogg/Vorbis/Theora (scripted video playback)
- PlayFab SDK (not automated by the Windows setup script)


# Windows 11 + Visual Studio (2022-Compatible)

Use the PowerShell installer by default. It handles dependency installation, configuration, build, and `INSTALL` target execution.
In automated Windows setup, open-source build dependencies are installed by the script via vcpkg.

## Automated Setup (Recommended)

### 1. Install tools

- Visual Studio with `Desktop development with C++` (downloads: https://visualstudio.microsoft.com/downloads/)
- Minimum compatibility target: VS 2022 toolchain (`Visual Studio 17 2022`, MSVC v143)
- If you use VS 2026, also install VS 2022-compatible C++ build tools/toolset
- CMake (recent version): https://cmake.org/download/
- Git: https://git-scm.com/download/win
- PowerShell 5+ or PowerShell 7+: https://learn.microsoft.com/powershell/scripting/install/installing-powershell-on-windows

Verify CMake is available:

```powershell
cmake --version
```

### 2. Place licensed SDKs for the integrations you enable

These cannot be auto-downloaded by script due to licensing/account requirements.

Download sources:

- FMOD: https://www.fmod.com/download
- Steamworks SDK: https://partner.steamgames.com/downloads/list
- EOS SDK: https://onlineservices.epicgames.com/en-US/sdk

Wizard defaults and required manual SDK layouts:

| Integration | Wizard default | Manual SDK needed | Required files |
| --- | --- | --- | --- |
| FMOD | ON | Yes | `deps\fmod\api\core\inc\fmod.hpp`, `deps\fmod\api\core\lib\x86_64\fmod_vc.lib` |
| Steamworks | ON | Yes | `deps\steamworks\sdk\public\steam\steam_api.h`, `deps\steamworks\sdk\redistributable_bin\win64\steam_api64.lib` |
| EOS | OFF | Only if enabled | `deps\eos\SDK\Include\eos_sdk.h`, `deps\eos\SDK\Lib\EOSSDK-Win64-Shipping.lib` |
| TheoraPlayer | ON | No (script auto-builds) | Script generates `deps\theoraplayer\include`, `deps\theoraplayer\lib`, `deps\theoraplayer\bin` |

Notes:

- If FMOD ships `api\core\lib\x64`, the installer mirrors it to `x86_64`.
- Runtime DLLs (`fmod.dll`, `steam_api64.dll`, `theoraplayer.dll`, etc.) must be next to the executable for local runs.

### 3. Run the installer wizard

Before running commands:

- Open PowerShell from the Windows Start menu (do not run these commands inside Visual Studio's terminal).
- Administrator privileges are not required for the standard workflow.
- Use **Run as administrator** only if you intentionally need elevated writes (for example, installing to protected system directories like `C:\Program Files`).

Then change to the repository root and run:

```powershell
cd "path\to\barony\repository"
Set-ExecutionPolicy -Scope Process Bypass
.\setup_barony_vs2022_x64.ps1
```

What the wizard prompts for:

- Feature toggles (`FMOD`, `Steamworks`, `EOS`, `CURL`, `Opus`, `TheoraPlayer`)
- `BARONY_DATADIR` auto-discovery/manual entry
- Re-check loop for missing manually placed SDK files
- Environment variable persistence
- EOS token values (only if EOS is enabled)

Non-interactive mode (CI/automation) is available via `-NonInteractive`.
Environment variable persistence is enabled by default; to disable it, use `-NoPersistUserEnvironment`.

### 4. Expected outputs and quick validation

Expected artifacts:

- Solution: `build-vs2022-x64\barony.sln`
- Game executable: typically under `build-vs2022-x64\Release\barony.exe`
- Editor executable: typically under `build-vs2022-x64\Release\editor.exe`

If `BARONY_DATADIR` is set, installer `INSTALL` step copies runtime files there.

### 5. EOS optional vs enabled

If you do not have EOS credentials, keep EOS disabled (default).

If EOS is enabled, CMake requires:

- `BUILD_ENV_PR`
- `BUILD_ENV_SA`
- `BUILD_ENV_DE`
- `BUILD_ENV_CC`
- `BUILD_ENV_CS`
- `BUILD_ENV_GSE`

## Manual Setup (Only if skipping installer or troubleshooting installer failures)

Use out-of-source builds and pick one of the command sets below.
Run these in PowerShell opened from Start menu, after changing to the repository root:

```powershell
cd "path\to\barony\repository"
```

### 0. Prepare vcpkg dependencies

```powershell
if (-not (Test-Path "$PWD\deps\vcpkg\vcpkg.exe")) {
  git clone --depth 1 https://github.com/microsoft/vcpkg "$PWD\deps\vcpkg"
  cmd /c "$PWD\deps\vcpkg\bootstrap-vcpkg.bat -disableMetrics"
}

& "$PWD\deps\vcpkg\vcpkg.exe" install `
  sdl2 sdl2-image sdl2-net sdl2-ttf `
  physfs rapidjson libpng zlib dirent glew `
  openssl curl[openssl] opus `
  libogg libvorbis libtheora nativefiledialog-extended `
  --triplet x64-windows
```

### A. Baseline manual build (minimal integrations)

```powershell
$env:BARONY_WIN32_LIBRARIES = "$PWD\deps\vcpkg\installed\x64-windows"
$env:EOS_ENABLED = "0"

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_POLICY_VERSION_MINIMUM=3.10 `
  -DCMAKE_TOOLCHAIN_FILE="$PWD\deps\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DCMAKE_INSTALL_PREFIX="$PWD\build\install-root" `
  -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON `
  -DFMOD_ENABLED=OFF -DSTEAMWORKS=OFF -DEOS=OFF -DOPENAL_ENABLED=OFF `
  -DTHEORAPLAYER=OFF -DCURL=ON -DOPUS=ON

cmake --build build --config Release --parallel
cmake --build build --config Release --target INSTALL
```

### B. Full-feature manual build (FMOD + Steamworks + TheoraPlayer)

```powershell
$env:BARONY_WIN32_LIBRARIES = "$PWD\deps\vcpkg\installed\x64-windows"
$env:FMOD_DIR = "$PWD\deps\fmod"
$env:STEAMWORKS_ROOT = "$PWD\deps\steamworks"
$env:THEORAPLAYER_DIR = "$PWD\deps\theoraplayer"
$env:EOS_ENABLED = "0"

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_POLICY_VERSION_MINIMUM=3.10 `
  -DCMAKE_TOOLCHAIN_FILE="$PWD\deps\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DCMAKE_INSTALL_PREFIX="$PWD\build\install-root" `
  -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON `
  -DFMOD_ENABLED=ON -DSTEAMWORKS=ON -DEOS=OFF -DOPENAL_ENABLED=OFF `
  -DTHEORAPLAYER=ON -DCURL=ON -DOPUS=ON

cmake --build build --config Release --parallel
cmake --build build --config Release --target INSTALL
```

Notes for full-feature manual build:

- `THEORAPLAYER=ON` requires prebuilt TheoraPlayer files under `deps\theoraplayer`.
- Required files: `deps\theoraplayer\include\theoraplayer\theoraplayer.h` and `deps\theoraplayer\lib\theoraplayer.lib`.
- If those files are not present, set `-DTHEORAPLAYER=OFF`.

If `INSTALL` tries to write to `C:\Program Files\barony` and fails with permission denied, reconfigure with a writable prefix, for example:

```powershell
cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$PWD\build\install-root"
```


# macOS (Homebrew, full-feature build)

These steps were validated with `FMOD + Steamworks + CURL + Opus + TheoraPlayer` enabled.

## 1. Install tools and dependencies

Install Xcode Command Line Tools first (if needed):

```bash
xcode-select --install
```

Install dependencies with Homebrew:

```bash
brew update
brew install \
  cmake ninja pkg-config git \
  sdl2 sdl2_image sdl2_net sdl2_ttf \
  libpng physfs rapidjson \
  mesa mesa-glu \
  curl openssl@3 opus theora \
  nativefiledialog-extended
```

Verify CMake:

```bash
cmake --version
```

## 2. Prepare SDK and third-party folders

Expected SDK layout:

- `deps/fmod/macos/api/core/inc/fmod.hpp`
- `deps/fmod/macos/api/core/lib/libfmod.dylib`
- `deps/steamworks/sdk/public/steam/steam_api.h`
- `deps/steamworks/sdk/redistributable_bin/osx/libsteam_api.dylib`

Steamworks finder compatibility workaround on macOS:

```bash
if [ ! -e deps/steamworks/sdk/redistributable_bin/osx32 ] && [ -d deps/steamworks/sdk/redistributable_bin/osx ]; then
  ln -s osx deps/steamworks/sdk/redistributable_bin/osx32
fi
```

If SDKs were downloaded with a browser and macOS blocks unsigned dylibs, clear quarantine attributes:

```bash
xattr -dr com.apple.quarantine "$PWD/deps/fmod" "$PWD/deps/steamworks"
```

Build and stage TheoraPlayer:

```bash
./scripts/build_theoraplayer.sh --prefix "$PWD/deps/theoraplayer"
```

## 3. Configure and build

```bash
export FMOD_DIR="$PWD/deps/fmod/macos"
export STEAMWORKS_ROOT="$PWD/deps/steamworks"
export THEORAPLAYER_DIR="$PWD/deps/theoraplayer"
export OPUS_DIR="$(brew --prefix opus)"
export OPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"

cmake -S . -B build-mac-all -G Ninja \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.10 \
  -DCMAKE_BUILD_TYPE=Release \
  -DFMOD_ENABLED=ON \
  -DOPENAL_ENABLED=OFF \
  -DSTEAMWORKS=ON \
  -DEOS=OFF \
  -DPLAYFAB=OFF \
  -DTHEORAPLAYER=ON \
  -DCURL=ON \
  -DOPUS=ON

cmake --build build-mac-all -j8
```

If Steamworks is enabled (`-DSTEAMWORKS=ON`), stage the runtime dylib into the app bundle:

```bash
cp "$PWD/deps/steamworks/sdk/redistributable_bin/osx/libsteam_api.dylib" \
  "$PWD/build-mac-all/Barony.app/Contents/MacOS/libsteam_api.dylib"
```

## 4. Validate run

```bash
./build-mac-all/Barony.app/Contents/MacOS/Barony \
  -datadir="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  -windowed -size=1280x720
```

Expected artifacts:

- `build-mac-all/Barony.app/Contents/MacOS/Barony`
- `build-mac-all/editor`


# Linux (Docker, recommended for full-feature build)

The repository includes a containerized full-feature build flow in `docker/`.

## 1. Install Docker and Docker Compose

Install guides:

- Docker Engine (Linux): https://docs.docker.com/engine/install/
- Docker Compose plugin (Linux): https://docs.docker.com/compose/install/linux/

Verify both commands are available:

```bash
docker --version
docker compose version
```

## 2. Place licensed SDKs (required for this full-feature lane)

Download sources:

- FMOD: https://www.fmod.com/download
- Steamworks SDK: https://partner.steamgames.com/downloads/list
- EOS SDK (only if enabling `-DEOS=ON`): https://onlineservices.epicgames.com/en-US/sdk

Required layout:

- `deps/fmod/linux/api/core/inc/fmod.hpp`
- `deps/fmod/linux/api/core/lib/x86_64/libfmod.so` (or versioned `libfmod.so.*`; build script creates `libfmod.so` symlink)
- `deps/steamworks/sdk/public/steam/steam_api.h`
- `deps/steamworks/sdk/redistributable_bin/linux64/libsteam_api.so`

EOS layout (optional, only if enabling EOS):

- `deps/eos/SDK/Include/eos_sdk.h`
- `deps/eos/SDK/Bin/libEOSSDK-Linux-Shipping.so`

## 3. Build image

```bash
docker compose -f docker/docker-compose.yml build
```

## 4. Run build

```bash
docker compose -f docker/docker-compose.yml run --rm barony-linux-build
```

Expected artifacts:

- `build-linux-all/barony`
- `build-linux-all/editor`


# Linux (native package-based full-feature build)

## 1. Install dependencies (Debian/Ubuntu example)

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build pkg-config git \
  libsdl2-dev libsdl2-image-dev libsdl2-net-dev libsdl2-ttf-dev \
  libpng-dev zlib1g-dev libphysfs-dev rapidjson-dev \
  libgl1-mesa-dev libglu1-mesa-dev libgtk-3-dev \
  libcurl4-openssl-dev libssl-dev \
  libopus-dev libogg-dev libvorbis-dev libtheora-dev
```

Verify CMake:

```bash
cmake --version
```

## 2. Place licensed SDKs (required for this full-feature lane)

Download sources:

- FMOD: https://www.fmod.com/download
- Steamworks SDK: https://partner.steamgames.com/downloads/list
- EOS SDK (only if enabling `-DEOS=ON`): https://onlineservices.epicgames.com/en-US/sdk

Required layout:

- `deps/fmod/linux/api/core/inc/fmod.hpp`
- `deps/fmod/linux/api/core/lib/x86_64/libfmod.so` (or versioned `libfmod.so.*`; configure block below creates `libfmod.so` symlink)
- `deps/steamworks/sdk/public/steam/steam_api.h`
- `deps/steamworks/sdk/redistributable_bin/linux64/libsteam_api.so`

EOS layout (optional, only if enabling EOS):

- `deps/eos/SDK/Include/eos_sdk.h`
- `deps/eos/SDK/Bin/libEOSSDK-Linux-Shipping.so`

## 3. Build NFD and TheoraPlayer

Build NFD from source (Steamworks ON requires NFD):

```bash
export NFD_PREFIX="$HOME/.local/nfd"
mkdir -p "$NFD_PREFIX"

git clone --depth 1 https://github.com/btzy/nativefiledialog-extended.git /tmp/nfd
cmake -S /tmp/nfd -B /tmp/nfd/build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DNFD_BUILD_TESTS=OFF \
  -DNFD_BUILD_SDL2_TESTS=OFF \
  -DNFD_INSTALL=ON \
  -DCMAKE_INSTALL_PREFIX="$NFD_PREFIX"
cmake --build /tmp/nfd/build -j"$(nproc)" --target install
```

Build and stage TheoraPlayer:

```bash
./scripts/build_theoraplayer.sh --prefix "$PWD/deps/theoraplayer"
```

## 4. Configure and build

```bash
export FMOD_DIR="$PWD/deps/fmod/linux"
export STEAMWORKS_ROOT="$PWD/deps/steamworks"
export NFD_DIR="$HOME/.local/nfd"
export THEORAPLAYER_DIR="$PWD/deps/theoraplayer"
export OPUS_DIR="/usr"

if [ ! -f "$FMOD_DIR/api/core/lib/x86_64/libfmod.so" ]; then
  lib="$(find "$FMOD_DIR/api/core/lib/x86_64" -maxdepth 1 -type f -name 'libfmod.so.*' | sort | head -n1)"
  [ -n "$lib" ] && ln -sfn "$(basename "$lib")" "$FMOD_DIR/api/core/lib/x86_64/libfmod.so"
fi

cmake -S . -B build-linux-all -G Ninja \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.10 \
  -DCMAKE_BUILD_TYPE=Release \
  -DFMOD_ENABLED=ON \
  -DOPENAL_ENABLED=OFF \
  -DSTEAMWORKS=ON \
  -DEOS=OFF \
  -DPLAYFAB=OFF \
  -DTHEORAPLAYER=ON \
  -DCURL=ON \
  -DOPUS=ON

cmake --build build-linux-all -j4
```

## 5. Validate run

```bash
./build-linux-all/barony -datadir=/path/to/Barony -windowed -size=1280x720
```

## 6. Package Linux Mod Release (Steam Deck-safe)

Before distributing a Linux/Steam Deck mod zip, run the packager so shared-library
symlink aliases are flattened into real files inside the archive:

```bash
scripts/mod_release/package_linux_release.sh \
  --release-dir dist/<linux-release-folder>
```

This prevents extraction edge-cases where aliases like `libcurl.so.4` become
tiny text files (`file too short`) on end-user systems.

Expected artifacts:

- `build-linux-all/barony`
- `build-linux-all/editor`


# Common Build Flags

- `-DFMOD_ENABLED=ON|OFF`
- `-DOPENAL_ENABLED=ON|OFF` (legacy/unmaintained)
- `-DSTEAMWORKS=ON|OFF` (or `-DSTEAMWORKS_ENABLED=1|0`)
- `-DEOS=ON|OFF` (or `-DEOS_ENABLED=1|0`)
- `-DCURL=ON|OFF` (or `-DCURL_ENABLED=1|0`)
- `-DOPUS=ON|OFF` (or `-DOPUS_ENABLED=1|0`)
- `-DPLAYFAB=ON|OFF` (or `-DPLAYFAB_ENABLED=1|0`, requires CURL and PlayFab tokens)
- `-DTHEORAPLAYER=ON|OFF` (or `-DTHEORAPLAYER_ENABLED=1|0`)

Example:

```bash
cmake -S . -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.10 -DCMAKE_BUILD_TYPE=Release \
  -DFMOD_ENABLED=ON -DSTEAMWORKS=ON -DTHEORAPLAYER=ON -DCURL=ON -DOPUS=ON -DEOS=OFF
```


# Troubleshooting

- `Could NOT find SDL2` (or similar core package errors):
  - Windows manual path: confirm `-DCMAKE_TOOLCHAIN_FILE` points to `deps/vcpkg/scripts/buildsystems/vcpkg.cmake`
  - Windows manual path: confirm `BARONY_WIN32_LIBRARIES` points to `deps/vcpkg/installed/x64-windows`
  - macOS/Linux: confirm required packages are installed and `pkg-config` is available

- Installer repeatedly asks for SDK files:
  - Re-check exact paths in this guide under Windows SDK layout
  - Verify you copied headers and import libraries, not only runtime DLLs

- EOS configure errors about missing tokens:
  - Set all required `BUILD_ENV_*` variables
  - Or disable EOS (`-DEOS=OFF` / keep EOS toggle off)

- `INSTALL` fails with permission denied:
  - Reconfigure with writable `CMAKE_INSTALL_PREFIX` (example in Windows manual section)

- macOS Steamworks lookup failure:
  - Apply the `osx32 -> osx` symlink workaround shown in the macOS section

- macOS runtime error `dyld: Library not loaded: @loader_path/libsteam_api.dylib`:
  - Copy `deps/steamworks/sdk/redistributable_bin/osx/libsteam_api.dylib` to `build-mac-all/Barony.app/Contents/MacOS/` before launching

- macOS dialog `"libfmod.dylib" Not Opened` (Apple could not verify malware status):
  - Clear quarantine flags on downloaded SDKs: `xattr -dr com.apple.quarantine "$PWD/deps/fmod" "$PWD/deps/steamworks"`

- CMake policy/version warnings:
  - Ensure configure commands include `-DCMAKE_POLICY_VERSION_MINIMUM=3.10` (already present in examples)
