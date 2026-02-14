# Dependencies
You need the following libraries to build Barony:

- SDL2
- SDL2_image
- SDL2_net
- SDL2_ttf
- libpng (and zlib)
- PhysFS
- RapidJSON
- dirent.h (Windows port)
- OpenGL (all platforms)
- GLEW (Windows)
- CMake

Optional audio/network integrations:

- FMOD Core API (preferred audio path on Windows)
- Steamworks SDK
- Epic Online Services (EOS) SDK
- CURL + OpenSSL (workshop preview HTTP downloads)
- Opus (encoded voice chat)
- TheoraPlayer + Ogg/Vorbis/Theora (scripted video playback)
- PlayFab SDK (not automated by the Windows setup script)


# Windows 11 + Visual Studio (2022-Compatible)

## 1. Install tools

- Visual Studio with `Desktop development with C++`
  - Minimum compatibility target: VS 2022 toolchain (`Visual Studio 17 2022`, MSVC v143)
  - If you use VS 2026, install VS 2022-compatible C++ build tools/toolset as well
- CMake (recent version)
- Git
- PowerShell 5+ or PowerShell 7+

## 2. Use the setup script

From repository root:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\setup_barony_vs2022_x64.ps1
```

What this script does:

- Uses the current repository checkout (no repo clone step)
- Uses VS 2022-compatible generator/toolchain (`Visual Studio 17 2022`, x64)
- Bootstraps `deps\vcpkg`
- Installs open-source dependencies to `deps\vcpkg\installed\x64-windows`
- Optionally installs `openssl + curl`, `opus`, and `libogg/libvorbis/libtheora` based on script toggles
- Auto-builds TheoraPlayer from source when enabled and not already present
- Configures CMake with C++17
- Builds `Release`

## 3. Optional feature toggles in `setup_barony_vs2022_x64.ps1`

Edit these at the top of the script as needed:

- `$EnableCurl = $true`  
  Installs `openssl` + `curl[openssl]` from vcpkg and enables `CURL_ENABLED`.
- `$EnableOpus = $true`  
  Installs `opus` from vcpkg and enables `OPUS_ENABLED`.
- `$EnableTheoraPlayer = $true`  
  Installs `libogg`, `libvorbis`, and `libtheora` from vcpkg, enables `THEORAPLAYER_ENABLED`, and auto-builds TheoraPlayer from source if `deps\theoraplayer` is missing.
- `$EnablePlayFab = $false` (recommended)  
  PlayFab automation is intentionally disabled in this script because it requires account-specific SDK/token setup.
- `$TheoraPlayerRepoUrl` / `$TheoraPlayerRepoRef`  
  Optional TheoraPlayer source override/pin. Defaults to `https://github.com/AprilAndFriends/theoraplayer.git`.

## 4. SDKs you must place manually

These cannot be auto-downloaded by script due to licensing/account requirements.

- EOS SDK: https://onlineservices.epicgames.com/en-US/sdk
- Steamworks SDK: https://partner.steamgames.com/downloads/list
- FMOD: https://www.fmod.com/download  
  Download **FMOD Engine**, then locate the installed SDK files and copy/extract into the repo layout below.

Required layout in this repo:

- `deps\fmod\api\core\inc\fmod.hpp`
- `deps\fmod\api\core\lib\x86_64\fmod_vc.lib`
- `deps\steamworks\sdk\public\steam\steam_api.h`
- `deps\steamworks\sdk\redistributable_bin\win64\steam_api64.lib`
- `deps\eos\SDK\Include\eos_sdk.h`
- `deps\eos\SDK\Lib\EOSSDK-Win64-Shipping.lib`

Notes:

- If FMOD ships `api\core\lib\x64`, the setup script mirrors it to `x86_64`.
- Runtime DLLs (`fmod.dll`, `steam_api64.dll`, etc.) must be next to the exe for local runs.
- For TheoraPlayer builds, the script installs `libogg/libvorbis/libtheora`, clones TheoraPlayer, builds `theoraplayer.dll/.lib`, and places files in:
  - `deps\theoraplayer\include\theoraplayer\*.h`
  - `deps\theoraplayer\lib\theoraplayer.lib`
  - `deps\theoraplayer\bin\theoraplayer.dll`

## 5. EOS optional vs enabled

If you do not have EOS credentials, build with EOS disabled.

- `setup_barony_vs2022_x64.ps1` defaults to EOS disabled.
- Manual configure should include `-DEOS=OFF` (or set `EOS_ENABLED=0` in environment).

If you enable EOS, CMake requires these values:

- `BUILD_ENV_PR`
- `BUILD_ENV_SA`
- `BUILD_ENV_DE`
- `BUILD_ENV_CC`
- `BUILD_ENV_CS`
- `BUILD_ENV_GSE`

## 6. Manual CMake fallback (without script)

```powershell
$env:BARONY_WIN32_LIBRARIES = "$PWD\deps\vcpkg\installed\x64-windows"
$env:FMOD_DIR = "$PWD\deps\fmod"
$env:STEAMWORKS_ROOT = "$PWD\deps\steamworks"
$env:EOS_ENABLED = "0"

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$PWD\deps\vcpkg\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DCMAKE_INSTALL_PREFIX="$PWD\build\install-root" `
  -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_STANDARD_REQUIRED=ON `
  -DFMOD_ENABLED=ON -DSTEAMWORKS=ON -DEOS=OFF -DOPENAL_ENABLED=OFF

cmake --build build --config Release --parallel
```

## 7. `BARONY_DATADIR` and `INSTALL` target (important)

For Visual Studio launch/debug, set `BARONY_DATADIR` to your game assets directory before running setup/configure. For example:

```powershell
$env:BARONY_DATADIR = "D:\SteamLibrary\steamapps\common\Barony"
```

Persist it for future shells:

```powershell
setx BARONY_DATADIR "D:\SteamLibrary\steamapps\common\Barony"
```

Then restart Visual Studio (and open a new terminal if you used `setx`).

Run the `INSTALL` target after building:

```powershell
cmake --build build --config Release --target INSTALL
```

This copies key runtime files to `BARONY_DATADIR`, including:

- `barony.exe`
- `editor.exe`
- `lang\en.txt`

Without this step, launching from VS often fails due to missing runtime content in the working directory.

If `INSTALL` tries to write into `C:\Program Files\barony` and fails with permission denied, reconfigure with a writable install prefix, for example:

```powershell
cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$PWD\build\install-root"
```


# macOS (Homebrew, Package-Based)

## 1. Install tools and dependencies

Install Xcode Command Line Tools first (if needed):

```bash
xcode-select --install
```

Install dependencies with Homebrew:

```bash
brew update
brew install cmake ninja pkg-config sdl2 sdl2_image sdl2_net sdl2_ttf libpng physfs rapidjson
```

## 2. Configure and build

```bash
cmake -S . -B build-mac -G Ninja \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DFMOD_ENABLED=OFF \
  -DOPENAL_ENABLED=OFF \
  -DSTEAMWORKS_ENABLED=OFF \
  -DEOS_ENABLED=OFF \
  -DPLAYFAB_ENABLED=OFF \
  -DTHEORAPLAYER_ENABLED=OFF \
  -DCURL_ENABLED=OFF \
  -DOPUS_ENABLED=OFF

cmake --build build-mac -j8
```

## 3. Run against game assets

Use the Steam resources directory as datadir:

```bash
./build-mac/barony.app/Contents/MacOS/barony \
  -datadir="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  -windowed -size=1280x720
```

## 4. Optional smoke check

```bash
timeout 20s ./build-mac/barony.app/Contents/MacOS/barony \
  -datadir="$HOME/Library/Application Support/Steam/steamapps/common/Barony/Barony.app/Contents/Resources" \
  -windowed -size=1280x720
```

Expected: exit code `124` from `timeout` after successful startup.


# Linux (Package-Based)

## 1. Install dependencies (Debian/Ubuntu example)

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build pkg-config \
  libsdl2-dev libsdl2-image-dev libsdl2-net-dev libsdl2-ttf-dev \
  libpng-dev zlib1g-dev libphysfs-dev rapidjson-dev \
  libgl1-mesa-dev libglu1-mesa-dev \
  xvfb xauth
```

## 2. Configure and build

```bash
cmake -S . -B build-linux -G Ninja \
  -DFMOD_ENABLED=OFF \
  -DOPENAL_ENABLED=OFF \
  -DSTEAMWORKS_ENABLED=OFF \
  -DEOS_ENABLED=OFF \
  -DPLAYFAB_ENABLED=OFF \
  -DTHEORAPLAYER_ENABLED=OFF \
  -DCURL_ENABLED=OFF \
  -DOPUS_ENABLED=OFF

cmake --build build-linux -j"$(nproc)" --target barony
```

## 3. Run

```bash
./build-linux/barony -datadir=/path/to/Barony.app/Contents/Resources -windowed -size=1280x720
```

## 4. Optional headless smoke check

```bash
timeout 30s xvfb-run -a ./build-linux/barony \
  -datadir=/path/to/Barony.app/Contents/Resources \
  -windowed -size=1280x720
```

Expected: exit code `124` from `timeout` after successful startup.


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
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFMOD_ENABLED=ON -DSTEAMWORKS=ON -DEOS=OFF
```
